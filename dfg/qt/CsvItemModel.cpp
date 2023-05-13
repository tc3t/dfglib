#include "CsvItemModel.hpp"
#include "qtIncludeHelpers.hpp"
#include "PropertyHelper.hpp"
#include "../cont/tableCsv.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QUndoStack>
//#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCompleter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QReadWriteLock>
#include <QTextStream>
#include <QStringListModel>

// SQL includes
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

DFG_END_INCLUDE_QT_HEADERS

#include <set>
#include "../dfgBase.hpp"
#include "../io.hpp"
#include "../str/string.hpp"
#include "qtBasic.hpp"
#include "../io/DelimitedTextReader.hpp"
#include "../time/timerCpu.hpp"
#include "../cont/CsvConfig.hpp"
#include "../cont/SetVector.hpp"
#include "../str/strTo.hpp"
#include "../os.hpp"
#include "../os/OutputFile.hpp"
#include "../cont/MapVector.hpp"
#include "../cont/IntervalSet.hpp"
#include "../cont/IntervalSetSerialization.hpp"
#include "StringMatchDefinition.hpp"
#include "sqlTools.hpp"

/////////////////////////////////
// Start of dfg::qt namespace
DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt)
{

namespace
{
    enum CsvItemModelPropertyId
    {
        CsvItemModelPropertyId_completerEnabledColumnIndexes,
        CsvItemModelPropertyId_completerEnabledSizeLimit, // Defines maximum size (in bytes) for completer enabled tables, i.e. input bigger than this limit will have no completer enabled.
        CsvItemModelPropertyId_maxFileSizeForMemoryStreamWrite, // If output file size estimate is less than this value, writing is tried with memory stream.
        CsvItemModelPropertyId_defaultFormatSeparator,
        CsvItemModelPropertyId_defaultFormatEnclosingChar,
        CsvItemModelPropertyId_defaultFormatEndOfLine,
        CsvItemModelPropertyId_defaultReadThreadBlockSizeMinimum,
        CsvItemModelPropertyId_defaultReadThreadCountMaximum
    };

    template <CsvItemModelPropertyId Id_T>
    DFG_ROOT_NS::uint8 variantToFormatValue(const QVariant& v);

    DFG_QT_DEFINE_OBJECT_PROPERTY_CLASS(CsvItemModel);

    // Properties
    DFG_QT_DEFINE_OBJECT_PROPERTY("CsvItemModel_completerEnabledColumnIndexes",
                                  CsvItemModel,
                                  CsvItemModelPropertyId_completerEnabledColumnIndexes,
                                  QStringList,
                                  PropertyType);
    DFG_QT_DEFINE_OBJECT_PROPERTY("CsvItemModel_completerEnabledSizeLimit",
                                  CsvItemModel,
                                  CsvItemModelPropertyId_completerEnabledSizeLimit,
                                  DFG_ROOT_NS::uint64,
                                  []() { return DFG_ROOT_NS::uint64(10000000); } );
    DFG_QT_DEFINE_OBJECT_PROPERTY("CsvItemModel_maxFileSizeForMemoryStreamWrite",
                                  CsvItemModel,
                                  CsvItemModelPropertyId_maxFileSizeForMemoryStreamWrite,
                                  DFG_ROOT_NS::int64,
                                  []() { return -1; }); // -1 means 'auto' (i.e. let implementation decide when to use)
    DFG_QT_DEFINE_OBJECT_PROPERTY_CUSTOM_TYPE("CsvItemModel_defaultFormatSeparator",
                                  CsvItemModel,
                                  CsvItemModelPropertyId_defaultFormatSeparator,
                                  DFG_ROOT_NS::uint8,
                                  []() { return DFG_ROOT_NS::uint8('\x1f'); },
                                  variantToFormatValue<CsvItemModelPropertyId_defaultFormatSeparator>
                                );
    DFG_QT_DEFINE_OBJECT_PROPERTY_CUSTOM_TYPE("CsvItemModel_defaultFormatEnclosingChar",
                                  CsvItemModel,
                                  CsvItemModelPropertyId_defaultFormatEnclosingChar,
                                  DFG_ROOT_NS::uint8,
                                  []() { return DFG_ROOT_NS::uint8('"'); },
                                  variantToFormatValue<CsvItemModelPropertyId_defaultFormatEnclosingChar>
                                );
    DFG_QT_DEFINE_OBJECT_PROPERTY("CsvItemModel_defaultFormatEndOfLine",
                                  CsvItemModel,
                                  CsvItemModelPropertyId_defaultFormatEndOfLine,
                                  QString,
                                  []() { return "\n"; });

    DFG_QT_DEFINE_OBJECT_PROPERTY("CsvItemModel_defaultReadThreadBlockSizeMinimum",
                                  CsvItemModel,
                                  CsvItemModelPropertyId_defaultReadThreadBlockSizeMinimum,
                                  uint64,
                                  []() { return 10000000; });

    DFG_QT_DEFINE_OBJECT_PROPERTY("CsvItemModel_defaultReadThreadCountMaximum",
                                  CsvItemModel,
                                  CsvItemModelPropertyId_defaultReadThreadCountMaximum,
                                  uint32,
                                  []() { return 0; }); // 0 means "let TableCsv decide"

    template <CsvItemModelPropertyId ID>
    auto getCsvItemModelProperty(const CsvItemModel* pModel) -> typename DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CsvItemModel)<ID>::PropertyType
    {
        return getProperty<DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CsvItemModel)<ID>>(pModel);
    }

    template <CsvItemModelPropertyId Id_T>
    DFG_ROOT_NS::uint8 variantToFormatValue(const QVariant& v)
    {
        auto s = v.toString();
        auto rv = DFG_MODULE_NS(str)::stringLiteralCharToValue<DFG_ROOT_NS::uint8>(s.toStdString());
        return (rv.first) ? rv.second : getCsvItemModelProperty<Id_T>(nullptr);
    }

    class Completer : public QCompleter
    {
    public:
        typedef QCompleter BaseClass;

        Completer(QObject* parent = nullptr) :
            BaseClass(parent)
        {}

        ~Completer()
        {

        }
    };
} // unnamed namesapce

class CsvItemModel::OpaqueTypeDefs
{
public:
    using RawDataTable = ::DFG_MODULE_NS(cont)::TableSz<char, Index, ::DFG_MODULE_NS(io)::encodingUTF8>;
    using DataTable    = ::DFG_MODULE_NS(cont)::TableCsv<char, Index, ::DFG_MODULE_NS(io)::encodingUTF8>;
};

namespace DFG_DETAIL_NS
{
    // Internal hack to allow CsvItemModel implementation to use table() -function to refer to actual datatable
    // while still being able to declare the table() function in header without including actual implementation header.
    class CsvItemModelTable::TableRef : public CsvItemModel::OpaqueTypeDefs::DataTable
    {
        TableRef() = delete;
        ~TableRef() = delete;
    }; // class CsvItemModelTable::TableRef
}

const QString CsvItemModel::s_sEmpty;

#define DFG_TEMP_SAVEOPTIONS_BASECLASS_INIT BaseClass(',', '"', DFG_MODULE_NS(io)::EndOfLineTypeN, DFG_MODULE_NS(io)::encodingUTF8)

CsvItemModel::SaveOptions::SaveOptions(CsvItemModel* pItemModel)
    : DFG_TEMP_SAVEOPTIONS_BASECLASS_INIT
{
    initFromItemModelPtr(pItemModel);
}

CsvItemModel::SaveOptions::SaveOptions(const CsvItemModel* pItemModel)
    : DFG_TEMP_SAVEOPTIONS_BASECLASS_INIT
{
    initFromItemModelPtr(pItemModel);
}

#undef DFG_TEMP_SAVEOPTIONS_BASECLASS_INIT

namespace
{
    static CsvItemModel::SaveOptions defaultSaveOptions(const CsvItemModel* pItemModel)
    {
        typedef CsvItemModel::SaveOptions SaveOptionsT;
        if (!pItemModel)
            return SaveOptionsT(pItemModel);
        SaveOptionsT rv = pItemModel->table().saveFormat();
        rv.separatorChar(getCsvItemModelProperty<CsvItemModelPropertyId_defaultFormatSeparator>(pItemModel));
        rv.enclosingChar(getCsvItemModelProperty<CsvItemModelPropertyId_defaultFormatEnclosingChar>(pItemModel));
        rv.eolType(DFG_MODULE_NS(io)::endOfLineTypeFromStr(getCsvItemModelProperty<CsvItemModelPropertyId_defaultFormatEndOfLine>(pItemModel).toStdString()));
        return rv;
    }
}

void CsvItemModel::SaveOptions::initFromItemModelPtr(const CsvItemModel* pItemModel)
{
    if (pItemModel)
    {
        // If model seems to have been opened from existing input (file/memory), use format of m_table, otherwise use save format from settings.
        *this = (pItemModel->latestReadTimeInSeconds() >= 0) ? pItemModel->table().saveFormat() : defaultSaveOptions(pItemModel);
        if (::DFG_MODULE_NS(io)::DelimitedTextReader::isMetaChar(this->separatorChar()))
            this->separatorChar(defaultSaveOptions(pItemModel).separatorChar());
        if (!pItemModel->isSupportedEncodingForSaving(textEncoding()))
        {
            // Encoding is not supported, fallback to UTF-8.
            textEncoding(::DFG_MODULE_NS(io)::encodingUTF8);
        }
    }
}

CsvItemModel::LoadOptions::LoadOptions(const CsvItemModel* pCsvItemModel)
    : CommonOptionsBase(
        ::DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharAutoDetect,
        '"',
        DFG_MODULE_NS(io)::EndOfLineTypeN,
        DFG_MODULE_NS(io)::encodingUnknown)
{
    this->setDefaultProperties(pCsvItemModel);
}

void CsvItemModel::LoadOptions::setDefaultProperties(const CsvItemModel* pCsvItemModel)
{
    using namespace ::DFG_MODULE_NS(str);
    const auto sDefaultReadThreadCountMaximum = toStrC(getCsvItemModelProperty<CsvItemModelPropertyId_defaultReadThreadCountMaximum>(pCsvItemModel));
    this->setPropertyT<PropertyId::readOpt_threadCount>(strTo<uint32>(this->getProperty(CsvOptionProperty_readThreadCountMaximum, sDefaultReadThreadCountMaximum)));

    const auto sDefaultReadThreadBlockSizeMinimum = toStrC(getCsvItemModelProperty<CsvItemModelPropertyId_defaultReadThreadBlockSizeMinimum>(pCsvItemModel));
    this->setPropertyT<PropertyId::readOpt_threadBlockSizeMinimum>(strTo<uint64>(this->getProperty(CsvOptionProperty_readThreadBlockSizeMinimum, sDefaultReadThreadBlockSizeMinimum)));
}

auto CsvItemModel::LoadOptions::constructFromConfig(const CsvConfig& config, const CsvItemModel* pCsvItemModel) -> LoadOptions
{
    LoadOptions options(pCsvItemModel);
    options.fromConfig(config);
    options.setDefaultProperties(pCsvItemModel);
    return options;
}

bool CsvItemModel::LoadOptions::isFilteredRead() const
{
    return isFilteredRead(getProperty(CsvOptionProperty_includeRows, ""),
        getProperty(CsvOptionProperty_includeColumns, ""),
        getProperty(CsvOptionProperty_readFilters, ""));
}

bool CsvItemModel::LoadOptions::isFilteredRead(const std::string& sIncludeRows, const std::string& sIncludeColumns, const std::string& sFilterItems) const
{
    return !sIncludeRows.empty() || !sIncludeColumns.empty() || !sFilterItems.empty();
}

QVariant DFG_DETAIL_NS::HighlightDefinition::data(const QAbstractItemModel& model, const QModelIndex& index, const int role) const
{
    DFG_ASSERT_CORRECTNESS(role != Qt::DisplayRole);
    if (!index.isValid() || role != Qt::BackgroundRole)
        return QVariant();
    // Negative row/column setting is interpreted as 'match any'.
    const auto isApplicableIndex = [](const int nMyIndex, const int nQueryIndex) { return (nMyIndex < 0 || nQueryIndex == nMyIndex); };
    if (!isApplicableIndex(m_nRow, index.row()) || !isApplicableIndex(m_nColumn, index.column()))
        return QVariant();
    auto displayData = model.data(index);
    if (m_matcher.isMatchWith(displayData.toString()))
        return m_highlightBrush;
    else
        return QVariant();
}

namespace
{
    class CsvTableModelActionCellEdit : public QUndoCommand
    {
        typedef CsvItemModel ModelT;
    public:
        CsvTableModelActionCellEdit(ModelT& rDataModel, const QModelIndex& index, const QString& sNew)
            : m_pDataModel(&rDataModel), m_index(index), m_sNewData(sNew)
        {
            m_sOldData = m_pDataModel->data(m_index).toString();

            QString sDesc;
            sDesc = QString("Edit cell (%1, %2)")
                        .arg(m_pDataModel->internalRowIndexToVisible(index.row()))
                        .arg(index.column());
            setText(sDesc);
        }

        void undo()
        {
            m_pDataModel->setDataNoUndo(m_index, m_sOldData);
        }

        void redo()
        {
            if (m_index.isValid())
                m_pDataModel->setDataNoUndo(m_index, m_sNewData);
        }

    private:
        ModelT* m_pDataModel;
        QModelIndex m_index;
        QString m_sOldData;
        QString m_sNewData;
    }; // CsvTableModelActionCellEdit

} // unnamed namespace

void CsvItemModel::ColInfo::CompleterDeleter::operator()(QCompleter* p) const
{
    if (p)
        p->deleteLater(); // Can't delete directly due to thread affinity (i.e. might get deleted from wrong thread triggering Qt asserts).
}

DFG_OPAQUE_PTR_DEFINE(CsvItemModel)
{
    std::shared_ptr<QReadWriteLock> m_spReadWriteLock;
    ::DFG_MODULE_NS(cont)::SetVector<IndexPairInteger> m_readOnlyCells;
    // Note: if adding members, check if it needs to be handled in clear()
};

CsvItemModel::CsvItemModel() :
    m_pUndoStack(nullptr),
    m_nRowCount(0),
    m_bModified(false),
    m_bResetting(false),
    m_readTimeInSeconds(-1),
    m_writeTimeInSeconds(-1)
{
    static bool sbMetaTypesRegistered = false;
    if (!sbMetaTypesRegistered)
    {
        sbMetaTypesRegistered = true;
        qRegisterMetaType<QVector<int>>("QVector<int>"); // For dataChanged() (https://bugreports.qt.io/browse/QTBUG-46517)
    }
    DFG_OPAQUE_REF().m_spReadWriteLock = std::make_shared<QReadWriteLock>(QReadWriteLock::Recursive);
}

CsvItemModel::~CsvItemModel()
{
}

auto CsvItemModel::getReadWriteLock() -> std::shared_ptr<QReadWriteLock>
{
    return DFG_OPAQUE_REF().m_spReadWriteLock;
}

auto CsvItemModel::tryLockForEdit() -> LockReleaser
{
    auto& spLock = DFG_OPAQUE_REF().m_spReadWriteLock;
    return (spLock && spLock->tryLockForWrite()) ? LockReleaser(spLock.get()) : LockReleaser();
}

auto CsvItemModel::tryLockForRead() const -> LockReleaser
{
    auto pOpaque = DFG_OPAQUE_PTR();
    if (!pOpaque)
        return LockReleaser();
    auto& spLock = pOpaque->m_spReadWriteLock;
    return (spLock && spLock->tryLockForRead()) ? LockReleaser(spLock.get()) : LockReleaser();
}

void CsvItemModel::setFilePathWithoutSignalEmit(QString s)
{
    m_sFilePath = std::move(s);
}

void CsvItemModel::setFilePathWithSignalEmit(QString s)
{
    setFilePathWithoutSignalEmit(std::move(s));
    Q_EMIT sigSourcePathChanged();
}

QString CsvItemModel::getTableTitle(const QString& sDefault) const
{
    if (!m_sTitle.isEmpty())
        return m_sTitle;
    const auto sFileName = QFileInfo(m_sFilePath).fileName();
    return (!sFileName.isEmpty()) ? sFileName : sDefault;
}

::DFG_MODULE_NS(io)::TextEncoding CsvItemModel::getFileEncoding() const
{
    return getFileEncoding(getFilePath());
}

::DFG_MODULE_NS(io)::TextEncoding CsvItemModel::getFileEncoding(const QString& sFilePath)
{
    const auto encoding = ::DFG_MODULE_NS(io)::checkBOMFromFile(qStringToFileApi8Bit(sFilePath));
    return encoding;
}

bool CsvItemModel::isSupportedEncodingForSaving(const DFG_MODULE_NS(io)::TextEncoding encoding) const
{
    return (encoding == DFG_MODULE_NS(io)::encodingUTF8) || (encoding == DFG_MODULE_NS(io)::encodingLatin1);
}

std::string CsvItemModel::saveToByteString(const SaveOptions& options)
{
    ::DFG_MODULE_NS(io)::BasicOmcByteStream<std::string> ostrm;
    saveImpl(ostrm, options);
    return ostrm.releaseData();
}

auto CsvItemModel::saveToByteString() -> std::string
{
    return saveToByteString(SaveOptions(this));
}

bool CsvItemModel::saveToFile()
{
    return saveToFile(m_sFilePath);
}

bool CsvItemModel::saveToFile(const QString& sPath)
{
    return saveToFile(sPath, SaveOptions(this));
}

template <class OutFile_T, class Stream_T>
bool CsvItemModel::saveToFileImpl(const QString& sPath, OutFile_T& outFile, Stream_T& strm, const SaveOptions& options)
{
    const bool bSuccess = saveImpl(strm, options) && (outFile.writeIntermediateToFinalLocation() == 0);
    if (bSuccess)
    {
        setFilePathWithSignalEmit(sPath);
        setModifiedStatus(false);
    }
    else
        outFile.discardIntermediate();
    Q_EMIT sigOnSaveToFileCompleted(bSuccess, static_cast<double>(m_writeTimeInSeconds));
    return bSuccess;
}

auto CsvItemModel::getOutputFileSizeEstimate() const -> uint64
{
    const uint64 nRows = static_cast<uint64>(table().rowCountByMaxRowIndex());
    const int nColCount = getColumnCount();
    uint64 nBytes = 0;
    for (int i = 0; i < nColCount; ++i)
        nBytes += static_cast<uint32>(getHeaderName(i).size()); // Underestimates if header name has non-ascii.
    nBytes += nRows * static_cast<uint64>(nColCount); // Separators and eol (assuming single-char eol)
    return 5 * (nBytes + table().contentStorageSizeInBytes()) / 4; // Put a little extra for enclosing chars etc.
}

bool CsvItemModel::saveToFile(const QString& sPath, const SaveOptions& options)
{
    QFileInfo fileInfo(sPath);
    if (!QDir().mkpath(fileInfo.absolutePath())) // Making sure that the target folder exists, otherwise opening the file will fail.
        return false;
    if (fileInfo.exists() && !fileInfo.isWritable())
        return false;

    DFG_MODULE_NS(os)::OutputFile_completeOrNone<> outFile(qStringToFileApi8Bit(sPath));

    const auto nSizeHint = getOutputFileSizeEstimate();

    const int64 nMemStreamSizeHint = getCsvItemModelProperty<CsvItemModelPropertyId_maxFileSizeForMemoryStreamWrite>(this);

    size_t nEffectiveMemStreamLimit = 0;
    if (nMemStreamSizeHint == -1) // If default setting
        nEffectiveMemStreamLimit = 50000000; // TODO: improve, use OS memory statistics.
    else if (nMemStreamSizeHint > 0)
        nEffectiveMemStreamLimit = (static_cast<uint64>(nMemStreamSizeHint) > NumericTraits<size_t>::maxValue) ? NumericTraits<size_t>::maxValue : static_cast<size_t>(nMemStreamSizeHint);

    // Try to use memory stream if
    //  -expected size is less than memory stream usage limit setting
    //   and
    //  -intermediate memory stream has expected capacity.
    // Otherwise write with file stream.
    // Reason to try memory stream is performance: memory stream provided by outFile is a non-virtual ostream replacement
    // that in many example cases have increased write performance by 100% in MSVC release builds.
    if (nSizeHint < nEffectiveMemStreamLimit)
    {
        auto& memStrm = outFile.intermediateMemoryStream(nEffectiveMemStreamLimit);
        if (memStrm.capacity() >= nSizeHint)
        {
            // Reserve was successful, write with memory stream.
            return saveToFileImpl(sPath, outFile, memStrm, options);
        }
    }
    return saveToFileImpl(sPath, outFile, outFile.intermediateFileStream(), options);
}

bool CsvItemModel::exportAsSQLiteFile(const QString& sPath)
{
    return exportAsSQLiteFile(sPath, defaultSaveOptions(this));
}

bool CsvItemModel::exportAsSQLiteFile(const QString& sPath, const SaveOptions& options)
{
    using SQLiteDatabase = ::DFG_MODULE_NS(sql)::SQLiteDatabase;
    DFG_UNUSED(options);
    m_messagesFromLatestSave.clear();
    const auto bFileExisted = QFileInfo::exists(sPath);
    SQLiteDatabase db(sPath, SQLiteDatabase::defaultConnectOptionsForWriting());
    bool rv = false;
    const auto exitHandler = makeScopedCaller([] {}, [&]()
        {
            // If writing fails and file didn't original exist, removing it.
            if (!rv && !bFileExisted)
            {
                db.close();
                QFile::remove(sPath);
            }
        });

    if (!db.isOpen())
    {
        rv = false;
        m_messagesFromLatestSave << tr("Unable to open database");
        return rv;
    }

    const auto toDatabaseName = [](const QString& s, const QString& sValueIfEmpty)
        {
            QString sRv = s.toLatin1();
            sRv.replace('\'', '_');
            sRv.replace('.', '_'); // Having dot in table name seemed to cause dfg::sql::getSQLiteFileTableColumnNames() to fail retrieving column names even though reading data with "SELECT * FROM <tablename>" worked.
            return (!sRv.isEmpty()) ? sRv : sValueIfEmpty;
        };

    auto createStatement = db.createQuery();
    const QString sTableName = toDatabaseName(this->getTableTitle(), "table_from_csv");
    QString sCreateStatement = QString("CREATE TABLE '%1' (").arg(sTableName);
    QString sInsertStatement = QString("INSERT INTO '%1' VALUES (").arg(sTableName);
    const auto nColCount = this->getColumnCount();
    for (int c = 0; c < nColCount; ++c)
    {
        sCreateStatement += QString((c == 0) ? "'%1' TEXT" : ", '%1' TEXT").arg(toDatabaseName(this->getHeaderName(c), QString("Col %1").arg(c)));
        sInsertStatement += QString((c == 0) ? "?" : ",?");
    }
    sCreateStatement += QLatin1String(");");
    sInsertStatement += QLatin1String(");");
    
    if (!createStatement.prepare(sCreateStatement))
    {
        m_messagesFromLatestSave << tr("Preparing table creation statement '%1' failed with error: '%2'").arg(sCreateStatement, createStatement.lastError().text());
        rv = false;
        return rv;
    }
    if (!createStatement.exec())
    {
        m_messagesFromLatestSave << tr("Executing table creation statement '%1' failed with error: '%2'").arg(sCreateStatement, createStatement.lastError().text());
        rv = false;
        return rv;
    }
    const auto nRowCount = this->rowCount();

    const auto beginTransaction = [&]()
        {
            if (db.transaction())
                return true;
            else
            {
                m_messagesFromLatestSave << tr("Beginning transaction failed with error: '%1'").arg(db.lastErrorText());
                rv = false;
                return false;
            }
        };
    const auto commit = [&]()
        {
            if (db.commit())
                return true;
            else
            {
                m_messagesFromLatestSave << tr("Committing transaction failed with error: '%1'").arg(db.lastErrorText());
                rv = false;
                return false;
            }
        };

    if (!beginTransaction())
        return rv;
    auto insertStatement = db.createQuery();
    if (!insertStatement.prepare(sInsertStatement))
    {
        m_messagesFromLatestSave << tr("Preparing insert statement '%1' failed with error: '%2'").arg(sInsertStatement, insertStatement.lastError().text());
        rv = false;
        return rv;
    }

    size_t nPendingInserts = 0;
    rv = true;
    for (int r = 0; r < nRowCount; ++r)
    {
        for (int c = 0; c < nColCount; ++c)
        {
            auto tpsz = table()(r, c);
            auto sv = StringViewUtf8(tpsz);
            insertStatement.bindValue(c, tpsz ? QVariant(viewToQString(sv)) : QVariant());
        }
        if (insertStatement.exec())
            ++nPendingInserts;
        else
        {
            m_messagesFromLatestSave << tr("Executing insert statement '%1' failed with error: '%2'").arg(sInsertStatement, insertStatement.lastError().text());
            rv = false;
            return rv;
        }
        if (nPendingInserts >= 100000) // Limit to avoid too big transactions, limit is chosen arbitrarily and may need adjusting.
        {
            if (!commit())
                return rv;
            if (!beginTransaction())
                return rv;
            nPendingInserts = 0;
        }
    }
    if (nPendingInserts > 0)
        rv = commit();

    return rv;
}

auto CsvItemModel::getSaveOptions() const -> SaveOptions
{
    return SaveOptions(this);
}

bool CsvItemModel::save(StreamT& strm)
{
    return save(strm, SaveOptions(this));
}

namespace
{
    template <class Strm_T>
    class CsvWritePolicy : public CsvItemModel::OpaqueTypeDefs::DataTable::WritePolicySimple<Strm_T>
    {
    public:
        typedef CsvItemModel::OpaqueTypeDefs::DataTable::WritePolicySimple<Strm_T> BaseClass;

        DFG_BASE_CONSTRUCTOR_DELEGATE_1(CsvWritePolicy, BaseClass)
        {
        }

        void write(Strm_T& strm, const char* pData, const int nRow, const int nCol)
        {
            DFG_STATIC_ASSERT(CsvItemModel::internalEncoding() == ::DFG_MODULE_NS(io)::encodingUTF8, "");
            BaseClass::write(strm, pData, nRow, nCol);
            if (!utf8::is_valid(pData, pData + std::strlen(pData)))
            {
                if (!::DFG_ROOT_NS::isValidIndex(this->m_colToInvalidContentRowList, nCol))
                    m_colToInvalidContentRowList.resize(nCol + 1);
                m_colToInvalidContentRowList[nCol].push_back(nRow);
            }
        }

        std::vector<std::vector<CsvItemModel::Index>> m_colToInvalidContentRowList; // m_colToInvalidContentRowList[c] is a list of rows in column c which had invalid utf and consequently written file may differ from input.
    }; // class CsvWritePolicy

}

bool CsvItemModel::save(StreamT& strm, const SaveOptions& options)
{
    return saveImpl(strm, options);
}

template <class Stream_T>
bool CsvItemModel::saveImpl(Stream_T& strm, const SaveOptions& options)
{
    DFG_MODULE_NS(time)::TimerCpu writeTimer;
    m_messagesFromLatestSave.clear();

    const QChar cSep = (DFG_MODULE_NS(io)::DelimitedTextReader::isMetaChar(options.separatorChar())) ? QChar(',') : QChar(options.separatorChar());
    const QChar cEnc(options.enclosingChar());
    const QChar cEol = DFG_MODULE_NS(io)::eolCharFromEndOfLineType(options.eolType());
    const auto sEol = DFG_MODULE_NS(io)::eolStrFromEndOfLineType(options.eolType());

    const auto encoding = (options.textEncoding() != DFG_MODULE_NS(io)::encodingUnknown) ? options.textEncoding() : DFG_MODULE_NS(io)::encodingUTF8;
    if (!isSupportedEncodingForSaving(encoding)) // Unsupported encoding type?
        return false;

    const auto qStringToEncodedBytes = [&](const QString& s) -> std::string
                                        {
                                            return (encoding == DFG_MODULE_NS(io)::encodingUTF8) ? std::string(s.toUtf8().data()) : std::string(s.toLatin1());
                                        };

    const std::string sSepEncoded = qStringToEncodedBytes(cSep);

    if (options.bomWriting())
    {
        const auto bomBytes = DFG_MODULE_NS(utf)::encodingToBom(encoding);
        strm.write(bomBytes.data(), std::streamsize(bomBytes.size()));
    }

    if (options.headerWriting())
    {
        QString sEncodedTemp;
        DFG_MODULE_NS(io)::writeDelimited(strm, m_vecColInfo, sSepEncoded, [&](Stream_T& strm, const ColInfo& colInfo)
        {
            sEncodedTemp.clear();
            DFG_MODULE_NS(io)::DelimitedTextCellWriter::writeCellFromStrIter(std::back_inserter(sEncodedTemp),
                                                                                             colInfo.m_name,
                                                                                             cSep,
                                                                                             cEnc,
                                                                                             cEol,
                                                                                             options.enclosementBehaviour());
            auto utf8Bytes = qStringToEncodedBytes(sEncodedTemp);
            strm.write(utf8Bytes.data(), static_cast<std::streamsize>(utf8Bytes.size()));
        });
        strm.write(sEol.data(), std::streamsize(sEol.size()));
    }

    auto mainTableSaveOptions = options;
    mainTableSaveOptions.bomWriting(false); // BOM has already been handled.
    mainTableSaveOptions.textEncoding(encoding);
    CsvWritePolicy<decltype(strm)> writePolicy(mainTableSaveOptions);
    table().writeToStream(strm, writePolicy);
    if (!writePolicy.m_colToInvalidContentRowList.empty())
    {
        size_t nInvalidCount = 0;
        const size_t nMaxMessageCount = 10;
        // At least one cell had invalid utf8 and might written, formulating a message about this for user.
        for (size_t c = 0; c < writePolicy.m_colToInvalidContentRowList.size(); ++c)
        {
            for (const auto r : writePolicy.m_colToInvalidContentRowList[c])
            {
                if (nInvalidCount < nMaxMessageCount)
                    this->m_messagesFromLatestSave.push_back(tr("Warning: cell (%1, %2) couldn't be written correctly, cell had invalid %3")
                        .arg(internalRowIndexToVisible(r))
                        .arg(internalColumnIndexToVisible(static_cast<Index>(c)))
                        .arg(::DFG_MODULE_NS(io)::encodingToStrId(internalEncoding())));
                ++nInvalidCount;
            }
        }
        if (nInvalidCount > nMaxMessageCount)
            this->m_messagesFromLatestSave.push_back(tr("Warning: %1 more cell(s) which couldn't be written correctly, cell(s) had invalid %2.").arg(nInvalidCount - nMaxMessageCount).arg(::DFG_MODULE_NS(io)::encodingToStrId(internalEncoding())));
    }
    table().saveFormat(options);

    m_writeTimeInSeconds = static_cast<decltype(m_writeTimeInSeconds)>(writeTimer.elapsedWallSeconds());

    return strm.good();
}

bool CsvItemModel::setItem(const int nRow, const int nCol, const StringViewUtf8& sv)
{
    const auto bRv = table().addString(sv, nRow, nCol);
    DFG_ASSERT(bRv); // Triggering ASSERT means that string couldn't be added to table.
    return bRv;
}

bool CsvItemModel::setItem(const int nRow, const int nCol, const QString& str)
{
    return setItem(nRow, nCol, SzPtrUtf8R(str.toUtf8()));
}

void CsvItemModel::setRow(const int nRow, QString sLine)
{
    if (!this->isValidRow(nRow))
        return;
    const bool bHasTabs = (sLine.indexOf('\t') != -1);
    const wchar_t cSeparator = (bHasTabs) ? L'\t' : L',';
    const wchar_t cEnclosing = (cSeparator == ',') ? L'"' : L'\0';
    QTextStream strm(&sLine);
    DFG_MODULE_NS(io)::DelimitedTextReader::readRow<wchar_t>(strm, cSeparator, cEnclosing, L'\n', [&](const size_t nCol, const wchar_t* const p, const size_t nDataLength)
    {
        const auto nSize = (nDataLength > NumericTraits<int>::maxValue) ? NumericTraits<int>::maxValue : static_cast<int>(nDataLength);
        setItem(nRow, static_cast<int>(nCol), QString::fromWCharArray(p, nSize));
    });

    if (!m_bResetting)
        Q_EMIT dataChanged(this->index(nRow, 0), this->index(nRow, getColumnCount()-1));
}

void CsvItemModel::clear()
{
    table().clear();
    m_vecColInfo.clear();
    setFilePathWithSignalEmit(QString());
    m_bModified = false;
    m_nRowCount = 0;
    m_sTitle.clear();
    m_loadOptionsInOpen = LoadOptions();
    m_messagesFromLatestOpen.clear();
    m_messagesFromLatestSave.clear();
    if (m_pUndoStack)
        m_pUndoStack->clear();

    DFG_OPAQUE_REF().m_readOnlyCells.clear();
}

bool CsvItemModel::openStream(QTextStream& strm)
{
    return openStream(strm, LoadOptions(this));
}

bool CsvItemModel::openStream(QTextStream& strm, const LoadOptions& loadOptions)
{
    auto s = strm.readAll();
    return openString(s, loadOptions);
}

bool CsvItemModel::openFromMemory(const char* data, const size_t nSize, LoadOptions loadOptions)
{
    setCompleterHandlingFromInputSize(loadOptions, nSize);
    return readData(loadOptions, [&]()
    {
        table().readFromMemory(data, nSize, loadOptions);
        return true;
    });
}

bool CsvItemModel::readData(const LoadOptions& options, std::function<bool()> tableFiller)
{
    ::DFG_MODULE_NS(time)::TimerCpu readTimer;

    beginResetModel();
    m_bResetting = true;
    clear();

    // Actual read happens here
    const auto bTableFillerRv = tableFiller();

    m_nRowCount = table().rowCountByMaxRowIndex();

    const QString optionsCompleterColumns(options.getProperty(CsvOptionProperty_completerColumns, "not_given").c_str());
    const auto completerEnabledColumnsStrItems = optionsCompleterColumns != "not_given" ? optionsCompleterColumns.split(',') : getCsvItemModelProperty<CsvItemModelPropertyId_completerEnabledColumnIndexes>(this);
    const auto completerEnabledInAll = (completerEnabledColumnsStrItems.size() == 1 && completerEnabledColumnsStrItems[0].trimmed() == "*");
    DFG_MODULE_NS(cont)::SetVector<Index> completerEnabledColumns;

    if (!completerEnabledInAll)
    {
        for(auto iter = completerEnabledColumnsStrItems.cbegin(), iterEnd = completerEnabledColumnsStrItems.cend(); iter != iterEnd; ++iter)
        {
            bool ok = false;
            const auto nCol = iter->toInt(&ok);
            if (ok)
                completerEnabledColumns.insert(visibleColumnIndexToInternal(nCol));
        }
    }

    const auto isCompleterEnabledInColumn = [&](const int nCol)
            {
                return completerEnabledInAll || completerEnabledColumns.hasKey(nCol);
            };

    // Setting headers.
    const auto config = this->getConfig();
    const auto nMaxColCount = table().colCountByMaxColIndex();
    char szUrlBuffer[64];
    m_vecColInfo.reserve(static_cast<size_t>(nMaxColCount));
    for (Index c = 0; c < nMaxColCount; ++c)
    {
        SzPtrUtf8R p = table()(0, c); // HACK: assumes header to be on row 0 and UTF8-encoding.
        m_vecColInfo.push_back(ColInfo(this, (p) ? QString::fromUtf8(p.c_str()) : QString()));
        if (isCompleterEnabledInColumn(c))
        {
            // Note: can't set 'this' as parent for completer as this function may get called from thread different
            //       from this->thread(). Also setParent() after moveToThread() caused fatal error.
            m_vecColInfo.back().m_spCompleter.reset(new Completer(nullptr));
            m_vecColInfo.back().m_spCompleter->moveToThread(this->thread());
            m_vecColInfo.back().m_spCompleter->setCaseSensitivity(Qt::CaseInsensitive);
            m_vecColInfo.back().m_completerType = CompleterTypeTexts;
        }

        const auto getConfigColumnPropertyC = [&](const char* pszPropertyName)
        {
            ::DFG_MODULE_NS(str)::DFG_DETAIL_NS::sprintf_s(szUrlBuffer, sizeof(szUrlBuffer), "columnsByIndex/%d/%s", internalColumnIndexToVisible(c), pszPropertyName);
            return config.value(SzPtrUtf8(szUrlBuffer)).rawStorage();
        };

        // Setting column type
        this->setColumnType(c, getConfigColumnPropertyC("datatype"));

        // Setting read-only mode
        {
            const auto sReadOnly = getConfigColumnPropertyC("readOnly");
            if (sReadOnly == "1")
                setColumnProperty(c, CsvItemModelColumnProperty::readOnly, true);
        }
    }
    // Since the header is stored separately in this model, remove it from the table. This can be a bit heavy operation for big tables
    table().removeRows(0, 1);
    if (m_nRowCount >= 1)
        m_nRowCount--;

    initCompletionFeature();

    m_bModified = false;
    endResetModel();
    m_bResetting = false;

    m_readTimeInSeconds = static_cast<decltype(m_readTimeInSeconds)>(readTimer.elapsedWallSeconds());
    if (m_pUndoStack)
        m_pUndoStack->clear();
    this->m_loadOptionsInOpen = options;
    Q_EMIT sigOnNewSourceOpened();

    return bTableFillerRv;
}

void CsvItemModel::initCompletionFeature()
{
    std::vector<std::set<QString>> vecCompletionSet;

    table().forEachFwdColumnIndex([&](const int nCol)
    {
        auto pColInfo = getColInfo(nCol);
        if (!pColInfo || !pColInfo->hasCompleter())
            return;
        if (!isValidIndex(vecCompletionSet, nCol))
            vecCompletionSet.resize(static_cast<size_t>(nCol) + 1);
        auto& completionSetForCurrentCol = vecCompletionSet[nCol];
        table().forEachFwdRowInColumn(nCol, [&](const int /*nRow*/, const SzPtrUtf8R pData)
        {
            if (pData)
                completionSetForCurrentCol.insert(QString::fromUtf8(pData.c_str()));
        });
    });
    for (size_t i = 0; i<vecCompletionSet.size(); ++i)
    {
        if (!m_vecColInfo[i].m_spCompleter)
            continue;
        QStringList stringlist;
        stringlist.reserve(static_cast<int>(vecCompletionSet[i].size()));
        std::copy(vecCompletionSet[i].begin(), vecCompletionSet[i].end(), std::back_inserter(stringlist));
        m_vecColInfo[i].m_spCompleter->setModel(new QStringListModel(stringlist));
    }
}

int CsvItemModel::findColumnIndexByName(const QString& sHeaderName, const int returnValueIfNotFound) const
{
    for (auto i = 0, nCount = getColumnCount(); i < nCount; ++i)
    {
        if (getHeaderName(i) == sHeaderName)
            return i;
    }
    return returnValueIfNotFound;
}

bool CsvItemModel::openNewTable()
{
    auto rv = readData(LoadOptions(this), [&]()
    {
        // Create 2x2 table by default, seems to be a better choice than completely empty.
        // Note that row 0 is omitted as it is interpreted as header by readData()
        table().setElement(1, 0, DFG_UTF8(""));
        table().setElement(1, 1, DFG_UTF8(""));
        table().setElement(2, 0, DFG_UTF8(""));
        table().setElement(2, 1, DFG_UTF8(""));
        m_sFilePath.clear();
        setFilePathWithoutSignalEmit(QString());
        return true;
    });
    m_readTimeInSeconds = -1;
    return rv;
}

bool CsvItemModel::mergeAnotherTableToThis(const CsvItemModel& other)
{
    const auto nOtherRowCount = other.getRowCount();
    if (nOtherRowCount < 1 || getRowCount() >= getRowCountUpperBound())
        return false;
    const auto nOtherColumnCount = other.getColumnCount();
    QVector<int> mapOtherColumnIndexToMerged(nOtherColumnCount, getColumnCount());
    for (auto i = 0; i < nOtherColumnCount; ++i)
    {
        const auto nCurrentColCount = getColumnCount();
        const auto nCol = findColumnIndexByName(other.getHeaderName(i), nCurrentColCount);
        if (nCol == nCurrentColCount)
        {
            insertColumn(nCurrentColCount);
            setColumnName(nCol, other.getHeaderName(i));
        }
        mapOtherColumnIndexToMerged[i] = nCol;
    }

    const auto nThisOriginalRowCount = getRowCount();

    const auto nNewRowCount = (getRowCountUpperBound() - nOtherRowCount > nThisOriginalRowCount) ?
                nThisOriginalRowCount + nOtherRowCount
              : getRowCountUpperBound();

    // TODO: implement merging in m_table implementation. For example when merging big tables, this current implementation
    //       copies data one-by-one while m_table could probably do it more efficiently by directly copying the string storage. In case of modifiable 'other',
    //       this.m_table could adopt the string storage.
    beginInsertRows(QModelIndex(), nThisOriginalRowCount, nNewRowCount - 1);
    other.table().forEachNonNullCell([&](const int r, const int c, SzPtrUtf8R s)
    {
        table().setElement(nThisOriginalRowCount + r, mapOtherColumnIndexToMerged[c], s);
    });
    m_nRowCount = nNewRowCount;
    endInsertRows();
    return true;
}

bool CsvItemModel::openString(const QString& str)
{
    return openString(str, LoadOptions(this));
}

bool CsvItemModel::openString(const QString& str, const LoadOptions& loadOptions)
{
    const auto bytes = str.toUtf8();
    if (loadOptions.textEncoding() == DFG_MODULE_NS(io)::encodingUTF8)
        return openFromMemory(bytes.data(), static_cast<size_t>(bytes.size()), loadOptions);
    else
    {
        auto loadOptionsTemp = loadOptions;
        loadOptionsTemp.textEncoding(DFG_MODULE_NS(io)::encodingUTF8);
        return openFromMemory(bytes.data(), static_cast<size_t>(bytes.size()), loadOptionsTemp);
    }
}

void CsvItemModel::populateConfig(DFG_MODULE_NS(cont)::CsvConfig& config) const
{
    table().m_readFormat.appendToConfig(config);

    const auto saveOptions = SaveOptions(this);

    // In some cases table's readFormat may differ from SaveOptions so setting format options manually.
    config.setKeyValue_fromUntyped("enclosing_char", saveOptions.enclosingCharAsString());
    config.setKeyValue_fromUntyped("separator_char", saveOptions.separatorCharAsString());
    config.setKeyValue_fromUntyped("end_of_line_type", saveOptions.eolTypeAsString());
    config.setKeyValue_fromUntyped("encoding", saveOptions.textEncodingAsString());
    config.setKeyValue_fromUntyped("bom_writing", saveOptions.bomWriting() ? "1" : "0");

    // Adding column datatype
    {
        const auto existingConfig = getConfig();
        char szBuffer[64];
        const auto formatToBuffer = [&](const char* pszFormat, const int nCol) { ::DFG_MODULE_NS(str)::DFG_DETAIL_NS::sprintf_s(szBuffer, sizeof(szBuffer), pszFormat, nCol); };
        for (int c = 0, nCount = this->columnCount(); c < nCount; ++c)
        {
            const auto nVisibleColumnIndex = internalColumnIndexToVisible(c);
            formatToBuffer("columnsByIndex/%d/datatype", nVisibleColumnIndex);
            // Adding only if differs from default or if existing config already has the field)
            if (this->getColType(c) != CsvItemModel::ColTypeText || existingConfig.contains(SzPtrUtf8(szBuffer)))
            {
                config.setKeyValue(StringUtf8(SzPtrUtf8(szBuffer)), this->getColTypeAsString(c).toString());
            }
        }
    }
}

auto CsvItemModel::getConfig() const -> ::DFG_MODULE_NS(cont)::CsvConfig
{
    return getConfig(CsvFormatDefinition::csvFilePathToConfigFilePath(getFilePath()));
}

auto CsvItemModel::getConfig(const QString& sConfFilePath) -> ::DFG_MODULE_NS(cont)::CsvConfig
{
    ::DFG_MODULE_NS(cont)::CsvConfig config;
    config.loadFromFile(qStringToFileApi8Bit(sConfFilePath));
    return config;
}

auto CsvItemModel::getLoadOptionsForFile(const QString& sFilePath) const -> LoadOptions
{
    return getLoadOptionsForFile(sFilePath, this);
}

auto CsvItemModel::getLoadOptionsForFile(const QString& sFilePath, const CsvItemModel* pModel) -> LoadOptions
{
    auto sConfFilePath = CsvFormatDefinition::csvFilePathToConfigFilePath(sFilePath);
    if (!QFileInfo::exists(sConfFilePath))
    {
        LoadOptions loadOptions(pModel);
        if (!::DFG_MODULE_NS(sql)::SQLiteDatabase::isSQLiteFile(sFilePath))
        {
            const auto s8bitPath = qStringToFileApi8Bit(sFilePath);
            const auto peekedFormat = peekCsvFormatFromFile(sFilePath, Min(1024, saturateCast<int>(::DFG_MODULE_NS(os)::fileSize(s8bitPath))));
            loadOptions.eolType(peekedFormat.eolType());
        }
        return loadOptions;
    }
    else
        return LoadOptions::constructFromConfig(getConfig(sConfFilePath), pModel);
}

auto CsvItemModel::getLoadOptionsFromConfFile() const -> LoadOptions
{
    return getLoadOptionsForFile(getFilePath(), this);
}

bool CsvItemModel::openFile(const QString& sDbFilePath)
{
    return openFile(sDbFilePath, getLoadOptionsForFile(sDbFilePath, this));
}

bool CsvItemModel::importFiles(const QStringList& paths)
{
    if (paths.empty())
        return true;

    for (auto iter = paths.cbegin(), iterEnd = paths.cend(); iter != iterEnd; ++iter)
    {
        CsvItemModel temp;
        temp.openFile(*iter);
        mergeAnotherTableToThis(temp);
    }
    return true; // TODO: more detailed return value (e.g. that how many were read successfully).
}

void CsvItemModel::setCompleterHandlingFromInputSize(LoadOptions& loadOptions, const uint64 nSizeInBytes, const CsvItemModel* pModel)
{
    const auto optionsHasCompleterLimit = loadOptions.hasProperty(CsvOptionProperty_completerEnabledSizeLimit);
    const auto limit = (optionsHasCompleterLimit)
                ?
                DFG_MODULE_NS(str)::strTo<uint64>(loadOptions.getProperty(CsvOptionProperty_completerEnabledSizeLimit, "0").c_str())
                :
                getCsvItemModelProperty<CsvItemModelPropertyId_completerEnabledSizeLimit>(pModel);
    if (nSizeInBytes > limit)
    {
        // If size is bigger than limit, disable completer by removing all columns from completer columns.
        loadOptions.setProperty(CsvOptionProperty_completerColumns, "");
    }
}

namespace DFG_DETAIL_NS
{
    ::DFG_MODULE_NS(cont)::IntervalSet<int> columnIntervalSetFromText(const StringViewSzC& sv)
    {
        using namespace ::DFG_MODULE_NS(cont);
        return intervalSetFromString<int>(sv, (std::numeric_limits<int>::max)()).shift_raw(-CsvItemModel::internalColumnToVisibleShift());
    }

    class CancellableReader : public CsvItemModel::OpaqueTypeDefs::DataTable::DefaultCellHandler
    {
    public:
        using BaseClass = CsvItemModel::OpaqueTypeDefs::DataTable::DefaultCellHandler;
        using ItemModel = CsvItemModel;
        using ProgressController = ItemModel::LoadOptions::ProgressController;

        CancellableReader(ItemModel::OpaqueTypeDefs::DataTable& rTable, ProgressController& rProgressController)
            : BaseClass(rTable)
            , m_rProgressController(rProgressController)
        {}

        template <class Derived_T>
        Derived_T makeConcurrencyClone(CsvItemModel::OpaqueTypeDefs::DataTable& rTable)
        { 
            CancellableReader cr(rTable, m_rProgressController);
            if (!this->m_spSharedProcessedByteCount)
            {
                this->m_spSharedProcessedByteCount = std::make_shared<std::atomic<uint64>>();
            }
            cr.m_spSharedProcessedByteCount = this->m_spSharedProcessedByteCount;
            m_rProgressController.incrementOperationThreadCount();
            return cr;
        }

        // Returns approximate increament in progress (in bytes).
        static uint64 handleProgressController(ProgressController& rProcessController, uint64& processedCount, std::atomic<uint64>* pSharedCounter, const size_t nRow, const size_t nCol, const size_t nCount)
        {
            if (rProcessController)
            {
                if (rProcessController.isTimeToUpdateProgress(nRow, nCol)) // In multithreaded read, this gets called at frequency multiplied by thread count compared to single threaded read.
                {                                                          // Not optimal, but not expected to be a problem either.
                    auto nTotalProcessCount = processedCount;
                    if (pSharedCounter)
                    {
                        pSharedCounter->fetch_add(processedCount);
                        nTotalProcessCount = pSharedCounter->load();
                        processedCount = 0;
                    }
                    if (!rProcessController(CsvItemModel::IoOperationProgressController::ProgressCallbackParamT(nTotalProcessCount, rProcessController.getCurrentOperationThreadCount())))
                    {
                        rProcessController.setCancelled(true);
                        // As of 2022-06 (dfgQtTableEditor 2.4.0), exception is caught in TableCsv::read()
                        throw CsvItemModel::OpaqueTypeDefs::DataTable::OperationCancelledException();
                    }
                }
            }
            return nCount + 1; // +1 is for separator/single-char eol
        }

        void handleProgressController(const size_t nRow, const size_t nCol, const size_t nCount)
        {
            m_nProcessedByteCount += handleProgressController(m_rProgressController, m_nProcessedByteCount, m_spSharedProcessedByteCount.get(), nRow, nCol, nCount);
        }

        void operator()(const size_t nRow, const size_t nCol, const char* pData, const size_t nCount)
        {
            BaseClass::operator()(nRow, nCol, pData, nCount);
            handleProgressController(nRow, nCol, nCount);
        }

        uint64 m_nProcessedByteCount = 0; // Approximate value of processed input bytes not yet added to shared counter. things like \r\n and quoting are ignored so this often undercounts.
        std::shared_ptr<std::atomic<uint64>> m_spSharedProcessedByteCount; // Atomic progress counter shared by all clones to count total bytes processed by all handlers.

        ProgressController& m_rProgressController;
    }; // CancellableReader

    class CancellableFilterReader : public CancellableReader
    {
    public:
        using BaseClass = CancellableReader;
        using ItemModel = CsvItemModel;
        using DataTable = CsvItemModel::DataTable;
        using FilterCellHandler = decltype(ItemModel::OpaqueTypeDefs::DataTable().createFilterCellHandler());
        using IsBaseClassConcurrencySafeT = std::is_same<decltype(BaseClass::isConcurrencySafeT()), ::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::ConcurrencySafeCellHandlerYes>;
        using IsFilterReaderConcurrencySafeT = std::is_same<decltype(FilterCellHandler::isConcurrencySafeT()), ::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::ConcurrencySafeCellHandlerYes>;
        using MyConcurrencySafetyT = std::conditional<
            IsBaseClassConcurrencySafeT::value && IsFilterReaderConcurrencySafeT::value,
            ::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::ConcurrencySafeCellHandlerYes,
            ::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::ConcurrencySafeCellHandlerNo
            >::type;
        
        static constexpr MyConcurrencySafetyT isConcurrencySafeT() { return MyConcurrencySafetyT(); }

        CancellableFilterReader(ItemModel::OpaqueTypeDefs::DataTable& rTable, ProgressController& rProgressController, FilterCellHandler& rFilterCellHandler)
            : BaseClass(rTable, rProgressController)
            , m_rFilterCellHandler(rFilterCellHandler)
        {}

        void operator()(const size_t nRow, const size_t nCol, const char* pData, const size_t nCount)
        {
            handleProgressController(nRow, nCol, nCount);
            m_rFilterCellHandler(nRow, nCol, pData, nCount);
        }

        FilterCellHandler& m_rFilterCellHandler;
    }; // CancellableFilterReader

    // Provides matching implementation for filter reading.
    class TextFilterMatcher
    {
    public:
        using ItemModel          = CsvItemModel;
        using ProgressController = ItemModel::LoadOptions::ProgressController;

        using MatcherDefinition = CsvItemModelStringMatcher;
        using MultiMatchDef = MultiMatchDefinition<MatcherDefinition>;

        TextFilterMatcher(const StringViewUtf8& sv, ProgressController& rProgressController)
            : m_rProgressController(rProgressController)
        {
            m_multiMatcher = MultiMatchDef::fromJson(sv);
        }

        // This is not expected to be used, but must be defined in order to get compiled.
        // This gets called only when m_nFilterCol != any, which in turn gets defined by call
        // to createFilterCellHandler(), but none of the calls in this file define that.
        bool isMatch(const int nInputRow, const StringViewUtf8) const
        {
            DFG_UNUSED(nInputRow);
            DFG_ASSERT_IMPLEMENTED(false);
            return false;
        }

        bool isMatch(const int nInputRow, const CsvItemModel::OpaqueTypeDefs::DataTable::RowContentFilterBuffer& rowBuffer)
        {
            uint64 nProcessedCount = 0;
            CancellableReader::handleProgressController(m_rProgressController, nProcessedCount, nullptr, static_cast<size_t>(nInputRow), 0, 0);
            const bool bIsMatch = m_multiMatcher.isMatchByCallback([&](const MatcherDefinition& matcher)
            {
                // Checking if current matcher matches with current buffer.
                bool bIsMatch = true;
                for (const auto& colAndStr : rowBuffer) if (matcher.isApplyColumn(colAndStr.first))
                {
                    bIsMatch = matcher.isMatchWith(nInputRow, colAndStr.first, colAndStr.second(rowBuffer));
                    if (bIsMatch)
                        break; // Some column match, don't need to check rest.
                }
                return bIsMatch;
            });
            return bIsMatch;
        }

        MultiMatchDef m_multiMatcher;
        ProgressController& m_rProgressController;
    }; // TextFilterMatcher
} // namespace DFG_DETAIL_NS

bool CsvItemModel::openFile(QString sDbFilePath, LoadOptions loadOptions)
{
    if (sDbFilePath.isEmpty())
        return false;

    const QFileInfo fileInfo(sDbFilePath);

    m_messagesFromLatestOpen.clear();

    if (!fileInfo.isFile())
    {
        m_messagesFromLatestOpen << tr("Path is not a file");
        return false;
    }
    // Note: not using QFileInfo::isReadable() because it doesn't check permissions on Windows.
    //       From Qt documentation: "If the NTFS permissions check has not been enabled, the result on Windows will merely reflect whether the file exists."
    if (!::DFG_MODULE_NS(os)::isPathFileAvailable(qStringToFileApi8Bit(sDbFilePath), DFG_MODULE_NS(os)::FileModeRead))
    {
        m_messagesFromLatestOpen << tr("File is not readable");
        return false;
    }
    {
        sDbFilePath = fileInfo.absoluteFilePath();
        setCompleterHandlingFromInputSize(loadOptions, static_cast<uint64>(fileInfo.size()));
        auto rv = readData(loadOptions, [&]()
        {
            const auto sIncludeRows = loadOptions.getProperty(CsvOptionProperty_includeRows, "");
            const auto sIncludeColumns = loadOptions.getProperty(CsvOptionProperty_includeColumns, "");
            const auto sFilterItems = loadOptions.getProperty(CsvOptionProperty_readFilters, "");
            const auto sReadPath = qStringToFileApi8Bit(sDbFilePath);
            if (loadOptions.isFilteredRead(sIncludeRows, sIncludeColumns, sFilterItems))
            {
                // Case: filtered read
                using namespace ::DFG_MODULE_NS(cont);
                if (!sFilterItems.empty())
                {
                    auto filter = table().createFilterCellHandler(DFG_DETAIL_NS::TextFilterMatcher(SzPtrUtf8(sFilterItems.c_str()), loadOptions.m_progressController));
                    if (!sIncludeRows.empty())
                        filter.setIncludeRows(intervalSetFromString<int>(sIncludeRows));
                    if (!sIncludeColumns.empty())
                        filter.setIncludeColumns(DFG_DETAIL_NS::columnIntervalSetFromText(sIncludeColumns));
                    table().readFromFile(sReadPath, loadOptions, filter);
                }
                else // Case: not having readFilters, only row/column include sets.
                {
                    auto filter = table().createFilterCellHandler();
                    DFG_DETAIL_NS::CancellableFilterReader reader(this->table(), loadOptions.m_progressController, filter);
                    if (!sIncludeRows.empty())
                        filter.setIncludeRows(intervalSetFromString<int>(sIncludeRows));
                    else
                        filter.setIncludeRows(IntervalSet<int>::makeSingleInterval(0, maxValueOfType<int>()));
                    if (!sIncludeColumns.empty())
                        filter.setIncludeColumns(DFG_DETAIL_NS::columnIntervalSetFromText(sIncludeColumns));
                    table().readFromFile(sReadPath, loadOptions, reader);
                }
                m_sTitle = tr("%1 (Filtered open)").arg(QFileInfo(sDbFilePath).fileName());
            }
            else // Case: normal (non-filtered) read
            {
                table().readFromFile(sReadPath, loadOptions, DFG_DETAIL_NS::CancellableReader(this->table(), loadOptions.m_progressController));
                // Note: setting file path is done only for non-filtered reads because after filtered read it makes no sense
                //       to conveniently overwrite source file given the (possibly) lossy opening.
                const auto bWasCancelled = loadOptions.m_progressController.isCancelled();
                if (!bWasCancelled)
                    setFilePathWithoutSignalEmit(std::move(sDbFilePath));
                else
                {
                    // When reading gets cancelled, not setting path so that saving the partly read file won't by default overwrite the original file.
                    m_sTitle = tr("%1 (cancelled open)").arg(QFileInfo(sDbFilePath).fileName());
                }
            }

            const auto errorInfo = table().readFormat().getReadStat<::DFG_MODULE_NS(cont)::TableCsvReadStat::errorInfo>();
            if (errorInfo.empty() || loadOptions.m_progressController.isCancelled())
                return true;
            else
            {
                using namespace ::DFG_MODULE_NS(cont);
                errorInfo.forEachStartingWith(DFG_UTF8("threads/thread_"), [&](const StringViewUtf8 svKey, const StringViewUtf8 svValue)
                    {
                        if (svValue.empty())
                            return;
                        // Checking if error_msg-field is included
                        const auto svFieldName = CsvConfig::uriPart(svKey, 1);
                        if (svFieldName != TableCsvErrorInfoFields::errorMsg)
                            return;
                        
                        const auto svThreadIndex = CsvConfig::uriPart(svKey, 0);
                        const auto nThreadIndex = ::DFG_MODULE_NS(str)::strTo<uint64>(svThreadIndex);
                        
                        m_messagesFromLatestOpen << tr("Thread %1: %2").arg(nThreadIndex).arg(viewToQString(svValue));
                    });
                // In single-threaded read, error_msg-field does not have thread-prefixes.
                const auto errorMsg = errorInfo.value(TableCsvErrorInfoFields::errorMsg);
                if (!errorMsg.empty())
                    m_messagesFromLatestOpen << viewToQString(errorMsg);
                return false;
            }
        });

        return rv;
    }
}

bool CsvItemModel::readDataFromSqlite(const QString& sDbFilePath, const QString& sQuery, LoadOptions& loadOptions)
{
    if (!QFileInfo::exists(sDbFilePath))
    {
        m_messagesFromLatestOpen << tr("File does not exist");
        return false;
    }
    ::DFG_MODULE_NS(sql)::SQLiteDatabase database(sDbFilePath);
    if (!database.isOpen())
    {
        const auto sLastError = (database.m_spDatabase) ? database.m_spDatabase->lastError().text() : QString();
        m_messagesFromLatestOpen << tr("File exists, but can't be opened, last error = '%1'").arg(sLastError);
        return false;
    }

    auto query = database.createQuery();
    if (!query.prepare(sQuery))
    {
        m_messagesFromLatestOpen << tr("Failed to prepare query, error = '%1'").arg(query.lastError().text());
        return false;
    }

    const bool bExecRv = query.exec();
    if (!bExecRv)
    {
        m_messagesFromLatestOpen << tr("Executing query failed, error = '%1'").arg(query.lastError().text());
        return false;
    }

    const auto cellToStorage = [&](const size_t nRow, const int nCol, const QVariant& var)
    {
        this->table().setElement(nRow, saturateCast<size_t>(nCol), SzPtrUtf8(var.toString().toUtf8())); // TODO: should allow storing utf16 directly without redundant utf8 QByteArray temporary (similar to MapToStringViews::insertRaw())
    };

    auto rec = query.record();
    const auto nColCount = rec.count();

    // Reading column names
    // Underlying read infrastructure expects header on row 0 so putting names there instead of filling names directly to header.
    for (int c = 0; c < nColCount; ++c)
        cellToStorage(0, c, rec.fieldName(c)); // For unknown reason here QSqlRecord seems to remove quotes from column names even though SQLiteDatabase::getSQLiteFileTableColumnNames() returns them correctly.

    // Reading records and filling table.
    bool bCancelled = false;
    for (size_t nRow = 1; !bCancelled && query.next(); ++nRow)
    {
        for (int c = 0; c < nColCount; ++c)
            cellToStorage(nRow, c, query.value(c));
        if (loadOptions.m_progressController.isTimeToUpdateProgress(nRow, 0))
            bCancelled = !loadOptions.m_progressController(nRow); // In general it's hard to interpret meaning of nRow since knowing the read ratio can be difficult/slow given that query can be complex, use multiple tables etc.
    }

    const QString sCancelledPart = (bCancelled) ? tr("cancelled ") : QString();
    m_sTitle = tr("%1 (%3query '%2')").arg(QFileInfo(sDbFilePath).fileName(), sQuery.mid(0, Min(32, saturateCast<int>(sQuery.size()))), sCancelledPart);
    return true;
}

bool CsvItemModel::openFromSqlite(const QString& sDbFilePath, const QString& sQuery)
{
    auto loadOptions = getLoadOptionsForFile(sDbFilePath, this);
    return openFromSqlite(sDbFilePath, sQuery, loadOptions);
}

bool CsvItemModel::openFromSqlite(const QString& sDbFilePath, const QString& sQuery, LoadOptions& loadOptions)
{
    m_messagesFromLatestOpen.clear();
    // Limiting completer usage by file size is highly coarse for databases, but at least this can prevent simple huge query cases from using completers.
    setCompleterHandlingFromInputSize(loadOptions, static_cast<uint64>(QFileInfo(sDbFilePath).size()));
    return this->readData(loadOptions, [&]() { return readDataFromSqlite(sDbFilePath, sQuery, loadOptions); });
}

//Note: When implementing a table based model, rowCount() should return 0 when the parent is valid.
int CsvItemModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    return (!parent.isValid()) ? getRowCount() : 0;
}


int CsvItemModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    return (!parent.isValid()) ? getColumnCount() : 0;
}

auto CsvItemModel::getColumnNames() const -> QStringList
{
    const auto nCount = columnCount();
    QStringList names;
    names.reserve(nCount);
    for(int c = 0; c < nCount; ++c)
    {
        names.push_back(this->getHeaderName(c));
    }
    return names;
}

auto CsvItemModel::rawStringPtrAt(const int nRow, const int nCol) const -> SzPtrUtf8R
{
    return SzPtrUtf8R(table()(nRow, nCol));
}

auto CsvItemModel::rawStringPtrAt(const QModelIndex& index) const -> SzPtrUtf8R
{
    return rawStringPtrAt(index.row(), index.column());
}

auto CsvItemModel::rawStringViewAt(const int nRow, const int nCol) const -> StringViewUtf8
{
    return StringViewUtf8(table()(nRow, nCol));
}

auto CsvItemModel::rawStringViewAt(const QModelIndex& index) const -> StringViewUtf8
{
    return rawStringViewAt(index.row(), index.column());
}

QVariant CsvItemModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
    if (!index.isValid())
        return QVariant();
    const int nRow = index.row();
    const int nCol = index.column();

    if (!this->hasIndex(nRow, nCol))
        return QVariant();

    if ((role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole))
    {
        const SzPtrUtf8R p = table()(nRow, nCol);
        // Note: also checking for empty string as fromUtf8() does allocation if given an empty string (at least 5.13.1 and earlier).
        // Note: At least ctrl+arrow behaviour in TableView is dependent on the distinction between returning QString("") and QVariant().
        //       As of 2019-12-08, implementation requires empty cells to be QVariant() instead of QVariant(QString(""))
        return (p && !::DFG_MODULE_NS(str)::isEmptyStr(p)) ? QString::fromUtf8(p.c_str()) : QVariant();
    }
    else if (role == Qt::BackgroundRole && !m_highlighters.empty())
    {
        const auto hlCount = m_highlighters.size();
        for(size_t i = 0; i < hlCount; ++i)
        {
            auto var = m_highlighters[i].data(*this, index, role);
            if (var.isValid())
                return var; // For now supporting only one highlighter per cell.
        }
    }
    return QVariant();
}

QVariant CsvItemModel::headerData(const int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
        return QVariant();
    if (orientation == Qt::Horizontal)
    {
        if (!isValidColumn(section))
            return QVariant();
        const auto sHeaderName = getHeaderName(section);
        if (role != Qt::ToolTipRole)
            return sHeaderName;
        else
            return tr("%1\n(column %2/%3)")
            .arg((!sHeaderName.isEmpty()) ? sHeaderName : tr("<unnamed column>"))
            .arg(internalColumnIndexToVisible(section))
            .arg(getColumnCount());
    }
    else
        return QVariant(QString("%1").arg(internalRowIndexToVisible(section)));
}

void CsvItemModel::setDataByBatch_noUndo(const RawDataTable& table, const SzPtrUtf8R pFill, std::function<bool()> isCancelledFunc)
{
    using IntervalContainer = ::DFG_MODULE_NS(cont)::MapVectorSoA<Index, ::DFG_MODULE_NS(cont) ::IntervalSet<Index>>;
    IntervalContainer intervalsByColumn; 
    table.impl().forEachFwdColumnIndex([&](const Index c)
    {
        if (isReadOnlyColumn(c))
            return;
        table.impl().forEachFwdRowInColumn(c, [&](const Index r, SzPtrUtf8R tpsz)
        {
            if (isCancelledFunc && isCancelledFunc())
                return; // TODO: should break loop
            auto tpszEffective = (pFill) ? pFill : tpsz;
            if (tpszEffective && privSetDataToTable(r, c, tpszEffective))
                intervalsByColumn[c].insert(r);
        });
    });

    if (intervalsByColumn.empty())
        return; // Nothing was changed.

    // Now sending dataChanged-signal for each change block
    for (const auto& kv : intervalsByColumn)
    {
        const auto nCol = kv.first;
        const auto& intervalSet = kv.second;
        intervalSet.forEachContiguousRange([&](const Index top, const Index bottom)
        {
            Q_EMIT dataChanged(this->index(top, nCol), this->index(bottom, nCol));
        });
    }
    
    setModifiedStatus(true);
}

bool CsvItemModel::isReadOnlyColumn(Index c) const
{
    return isValidColumn(c) && m_vecColInfo[c].getProperty(CsvItemModelColumnProperty::readOnly).toBool();
}

bool CsvItemModel::isCellEditable(const Index nRow, const Index nCol) const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return !isReadOnlyColumn(nCol) && (!pOpaq || pOpaq->m_readOnlyCells.empty() || !pOpaq->m_readOnlyCells.hasKey(cellRowColumnPairToIndexPairInteger(nRow, nCol)));
}

bool CsvItemModel::isCellEditable(const QModelIndex& index)
{
    return isCellEditable(index.row(), index.column());
}

bool CsvItemModel::privSetDataToTable(const int nRow, const int nCol, const StringViewUtf8 sv)
{
    if (!isValidRow(nRow) || !isValidColumn(nCol))
    {
        DFG_ASSERT(false);
        return false;
    }

    if (!isCellEditable(nRow, nCol))
        return false;

    // Checking whether the new value is different from old to avoid setting modified even if nothing changes.
    const auto svExisting = table().viewAt(nRow, nCol);
    if (svExisting == sv)
        return false;

    return setItem(nRow, nCol, sv);
}

bool CsvItemModel::setDataNoUndo(const Index nRow, const Index nCol, const StringViewUtf8 sv)
{
    if (!privSetDataToTable(nRow, nCol, sv))
        return false;

    auto indexItem = index(nRow, nCol);
    Q_EMIT dataChanged(indexItem, indexItem);
    setModifiedStatus(true);
    return true;
}

bool CsvItemModel::setDataNoUndo(const QModelIndex& index, const StringViewUtf8 sv)
{
    return setDataNoUndo(index.row(), index.column(), sv);
}

bool CsvItemModel::setDataNoUndo(const QModelIndex& index, const QString& str)
{
    return setDataNoUndo(index.row(), index.column(), str);
}

bool CsvItemModel::setDataNoUndo(const Index nRow, const Index nCol, const QString& str)
{
    return setDataNoUndo(nRow, nCol, SzPtrUtf8(str.toUtf8().data()));
}

bool CsvItemModel::setData(const QModelIndex& index, const QVariant& value, int role /*= Qt::EditRole*/)
{
    // Sets the role data for the item at index to value.
    // Returns true if successful; otherwise returns false.
    // The dataChanged() signal should be emitted if the data was successfully set.

    if (index.isValid() && role == Qt::EditRole && isValidRow(index.row()) && isValidColumn(index.column()))
    {
        if (m_pUndoStack)
            m_pUndoStack->push(new CsvTableModelActionCellEdit(*this, index, value.toString()));
        else
            setDataNoUndo(index, value.toString());
        return true;
     }
     return false;
}

bool CsvItemModel::setHeaderData(const int section, Qt::Orientation orientation, const QVariant &value, const int role)
{
    if (orientation == Qt::Horizontal && role == Qt::EditRole && section >= 0 && section < getColumnCount())
    {
        const auto sValue = value.toString();
        if (getHeaderName(section) != sValue)
        {
            setColumnName(section, sValue);
            Q_EMIT headerDataChanged(orientation, section, section);
            return true;
        }
        else
            return false;
    }
    else
        return BaseClass::setHeaderData(section, orientation, value, role);
}

Qt::ItemFlags CsvItemModel::flags(const QModelIndex& index) const
{
    #if DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS
        const Qt::ItemFlags f = QAbstractTableModel::flags(index);
        if (index.isValid())
            return (f | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
        else
            return (f | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    #else
        const Qt::ItemFlags f = QAbstractTableModel::flags(index);
        if (index.isValid())
            return (f | Qt::ItemIsEditable);
        else
            return f;
    #endif
}

int CsvItemModel::getRowCountUpperBound() const
{
    // For now mostly a dedicated replacement for
    // NumericTraits<int>::maxValue for integer addition overflow control.
    return NumericTraits<int>::maxValue - 1;
}

int CsvItemModel::getColumnCountUpperBound() const
{
    // For now mostly a dedicated replacement for
    // NumericTraits<int>::maxValue for integer addition overflow control.
    return NumericTraits<int>::maxValue - 1;
}

bool CsvItemModel::insertRows(int position, const int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    const auto nOldRowCount = getRowCount();
    if (position < 0)
        position = nOldRowCount;
    if (parent.isValid() || count <= 0 || position < 0 || position > nOldRowCount || getRowCountUpperBound() - nOldRowCount < count)
        return false;
    const auto nLastNewRowIndex = position + count - 1;
    beginInsertRows(QModelIndex(), position, nLastNewRowIndex);
    table().insertRowsAt(position, count);
    m_nRowCount += count;
    endInsertRows();
    setModifiedStatus(true);
    return true;
}

bool CsvItemModel::removeRows(int position, int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    if (count <= 0 || parent.isValid() || !isValidRow(position) || getRowCountUpperBound() - position < count  || !isValidRow(position + count - 1))
        return false;

    const auto nOriginalCount = getRowCount();

    beginRemoveRows(QModelIndex(), position, position + count - 1);

    table().removeRows(position, count);
    m_nRowCount -= count;

    endRemoveRows();

    const auto nNewCount = getRowCount();

    DFG_ASSERT_CORRECTNESS(nOriginalCount - nNewCount == count);
    DFG_UNUSED(nOriginalCount); // To avoid warning in release-build
    DFG_UNUSED(nNewCount); // To avoid warning in release-build

    setModifiedStatus(true);
    return true;
}

void CsvItemModel::insertColumnsImpl(int position, int count)
{
    table().insertColumnsAt(position, count);
    for(int i = position; i<position + count; ++i)
    {
        m_vecColInfo.insert(m_vecColInfo.begin() + i, ColInfo(this));
    }
}

bool CsvItemModel::insertColumns(int position, const int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    if (position < 0)
        position = getColumnCount();
    if (parent.isValid() || count <= 0 || position < 0 || position > getColumnCount() || getColumnCountUpperBound() - position < count)
        return false;
    beginInsertColumns(QModelIndex(), position, position + count - 1);
    insertColumnsImpl(position, count);
    endInsertColumns();
    setModifiedStatus(true);
    return true;
}

bool CsvItemModel::removeColumns(int position, int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    const int nLast = position + count - 1;
    if (count < 0 || parent.isValid() || !isValidColumn(position) || !isValidColumn(nLast))
        return false;
    beginRemoveColumns(QModelIndex(), position, position + count - 1);

    table().eraseColumnsByPosAndCount(position, count);
    m_vecColInfo.erase(m_vecColInfo.begin() + position, m_vecColInfo.begin() + nLast + 1);

    endRemoveColumns();
    setModifiedStatus(true);
    return true;
}

void CsvItemModel::setColumnName(const int nCol, const QString& sName)
{
    auto pColInfo = getColInfo(nCol);
    if (pColInfo && !pColInfo->getProperty(CsvItemModelColumnProperty::readOnly).toBool())
    {
        pColInfo->m_name = sName;
        setModifiedStatus(true);
        Q_EMIT headerDataChanged(Qt::Horizontal, nCol, nCol);
    }
}

void CsvItemModel::removeRows(const ::DFG_MODULE_NS(cont)::ArrayWrapperT<const int>& indexesAscSorted)
{
    if (indexesAscSorted.empty())
        return;

    auto iterEffectiveBegin = indexesAscSorted.begin();
    // Skipping negative indexes from beginning.
    while (iterEffectiveBegin != indexesAscSorted.end() && *iterEffectiveBegin < 0)
        ++iterEffectiveBegin;

    if (iterEffectiveBegin == indexesAscSorted.end())
        return; // In this case there were only negative indexes.

    auto iter = indexesAscSorted.end() - 1;

    // Skipping past-end items
    while (*iter >= getRowCount())
    {
        if (iter == iterEffectiveBegin) // In this case there were no valid indexes.
            return;
        --iter;
    }

    // Removing from end in order to not invalidate later indexes (e.g. removing first when having indexes 0 and 2 would make later removal of row 2 behave wrong).
    for(;;)
    {
        // Removing in blocks for performance reasons, i.e. if indexes are 0,1,2 and 7,8,9, removing with two calls removeRows(7, 3) and removeRows(0, 3)
        auto iterBlockLast = iter; // Note: this is inclusive, not one past end.
        auto iterBlockFirst = iterBlockLast;
        while (iterBlockFirst != iterEffectiveBegin)
        {
            const auto currentRow = *iterBlockFirst;
            const auto previousRow = *(iterBlockFirst - 1);
            if (currentRow == previousRow + 1)
                --iterBlockFirst;
            else
                break;
        }
        this->removeRows(*iterBlockFirst, static_cast<int>(std::distance(iterBlockFirst, iterBlockLast) + 1));
        if (iterBlockFirst == iterEffectiveBegin)
            break;
        else
            iter = iterBlockFirst - 1;
    }
}

void CsvItemModel::columnToStrings(const int nCol, std::vector<QString>& vecStrings)
{
    vecStrings.clear();
    if (!isValidColumn(nCol))
        return;
    vecStrings.reserve(static_cast<size_t>(getColumnCount()));
    table().forEachFwdRowInColumn(nCol, [&](const int /*row*/, const SzPtrUtf8R tpsz)
    {
        vecStrings.push_back(QString::fromUtf8(tpsz.c_str()));

    });
}

void CsvItemModel::setColumnCells(const int nCol, const std::vector<QString>& vecStrings)
{
    if (!isValidColumn(nCol))
        return;
    const auto nRowCount = Min(getRowCount(), static_cast<int>(vecStrings.size()));
    for (int r = 0; r<nRowCount; ++r)
    {
        setItem(r, nCol, vecStrings[static_cast<size_t>(r)]);
    }
    if (!m_bResetting)
        Q_EMIT dataChanged(this->index(0, nCol), this->index(getRowCount()-1, nCol));
}

void CsvItemModel::setModifiedStatus(const bool bMod /*= true*/)
{
    if (bMod != m_bModified)
    {
        m_bModified = bMod;
        Q_EMIT sigModifiedStatusChanged(m_bModified);
    }
}

void CsvItemModel::setColumnType(const Index nCol, const ColType colType)
{
    auto pColInfo = getColInfo(nCol);
    if (!pColInfo || pColInfo->m_type == colType)
        return;
    pColInfo->m_type = colType;
    setModifiedStatus(true);
    Q_EMIT headerDataChanged(Qt::Horizontal, nCol, nCol);
}

void CsvItemModel::setColumnType(const Index nCol, const StringViewC sColType)
{
    if (!isValidColumn(nCol))
        return;
    if (sColType == ColInfo::columnTypeAsString(ColTypeText).asUntypedView())
        setColumnType(nCol, ColTypeText);
    else if (sColType == ColInfo::columnTypeAsString(ColTypeNumber).asUntypedView())
        setColumnType(nCol, ColTypeNumber);
    else if (!sColType.empty() && sColType != "default")
    {
        DFG_ASSERT_INVALID_ARGUMENT(false, "Unexpected column type");
    }
}

QVariant CsvItemModel::getColumnProperty(const Index nCol, const CsvItemModelColumnProperty propertyId, QVariant defaultValue)
{
    auto pColInfo = getColInfo(nCol);
    return (pColInfo) ? pColInfo->getProperty(propertyId, defaultValue) : defaultValue;
}

void CsvItemModel::setColumnProperty(const Index nCol, const CsvItemModelColumnProperty propertyId, QVariant value)
{
    auto pColInfo = getColInfo(nCol);
    if (!pColInfo)
        return;
    pColInfo->setProperty(propertyId, value);
}

QString& CsvItemModel::dataCellToString(const QString& sSrc, QString& sDst, const QChar cDelim)
{
    QTextStream strm(&sDst);
    DFG_MODULE_NS(io)::DelimitedTextCellWriter::writeCellFromStrStrm(strm,
        sSrc,
        cDelim,
        QChar('"'),
        QChar('\n'),
        DFG_MODULE_NS(io)::EbEncloseIfNeeded);
    return sDst;
}

void CsvItemModel::rowToString(const int nRow, QString& str, const QChar cDelim, const IndexSet* pSetIgnoreColumns /*= nullptr*/) const
{
    if (!isValidRow(nRow))
        return;
    const auto nColCount = getColumnCount();
    bool bFirstItem = true;
    for (int nCol = 0; nCol < nColCount; ++nCol)
    {
        if (pSetIgnoreColumns != nullptr && pSetIgnoreColumns->find(nCol) != pSetIgnoreColumns->end())
            continue;
        if (!bFirstItem) // To make sure that there won't be a delim in front of first item.
            str.push_back(cDelim);
        bFirstItem = false;
        SzPtrUtf8R p = table()(nRow, nCol);
        if (!p)
            continue;
        dataCellToString(QString::fromUtf8(p.c_str()), str, cDelim);
    }
}

void CsvItemModel::setUndoStack(QUndoStack* pStack)
{
    m_pUndoStack = pStack;
}

void CsvItemModel::setHighlighter(HighlightDefinition hld)
{
    auto existing = std::find_if(m_highlighters.begin(), m_highlighters.end(), [&](const HighlightDefinition& a) { return hld.m_id == a.m_id; });

    const bool bResetModel = (existing == m_highlighters.end() || existing->hasMatchString() || hld.hasMatchString());
    if (bResetModel) // Resetting model only if something relavant has changed, e.g. if there is no match text and column changes, don't want to reset model as it would cause various signals to be emitted that may trigger "something changed"-actions.
        beginResetModel();
    if (existing != m_highlighters.end())
        *existing = hld;
    else
        m_highlighters.push_back(hld);
    if (bResetModel)
        endResetModel();
}

QModelIndexList CsvItemModel::match(const QModelIndex& start, int role, const QVariant& value, const int hits, const Qt::MatchFlags flags) const
{
    // match() is not adequate for needed find functionality (e.g. misses backward find, https://bugreports.qt.io/browse/QTBUG-344)
    // so find implementation is not using match().
    return BaseClass::match(start, role, value, hits, flags);
}

QModelIndex CsvItemModel::nextCellByFinderAdvance(const QModelIndex& seedIndex, const FindDirection direction, const FindAdvanceStyle advanceStyle) const
{
    int r = Max(0, seedIndex.row());
    int c = Max(0, seedIndex.column());
    nextCellByFinderAdvance(r, c, direction, advanceStyle);
    return index(r, c);
}

void CsvItemModel::nextCellByFinderAdvance(int& r, int& c, const FindDirection direction, const FindAdvanceStyle advanceStyle) const
{
    if (advanceStyle == FindAdvanceStyleRowIncrement)
    {
        const int modifier = (direction == FindDirectionForward) ? 1 : -1;
        r += modifier;
    }
    else if (advanceStyle == FindAdvanceStyleLinear)
    {
        if (direction == FindDirectionForward)
        {
            if (c + 1 < getColumnCount())
                c++;
            else
            {
                r = r+1;
                c = 0;
            }
        }
        else // case: backward direction
        {
            if (c == 0)
            {
                r = r-1;
                c = getColumnCount() - 1;
            }
            else
                c--;
        }
    }
    else
    {
        DFG_ASSERT_IMPLEMENTED(false);
    }

    // Wrapping
    if (r >= getRowCount())
    {
        r = 0;
        if (advanceStyle == FindAdvanceStyleLinear)
            c = 0;
    }
    else if (r < 0)
    {
        r = getRowCount() - 1;
        if (advanceStyle == FindAdvanceStyleLinear)
            c = getColumnCount() - 1;
    }
}

auto CsvItemModel::cellRowColumnPairToLinearIndex(const Index r, const Index c) const -> LinearIndex
{
    return pairIndexToLinear<LinearIndex>(r, c, getColumnCount());
}

auto CsvItemModel::cellRowColumnPairToIndexPairInteger(Index r, Index c) -> IndexPairInteger
{
    DFG_STATIC_ASSERT(sizeof(r) <= 4, "Bitshift logics expects Index to be at most 4 bytes in size");
    return (static_cast<uint64>(r) << 32) | static_cast<uint64>(c);
}

auto CsvItemModel::wrappedDistance(const QModelIndex& from, const QModelIndex& to, const FindDirection direction) const -> LinearIndex
{
    if (!from.isValid() || !to.isValid())
        return 0;
    const auto fromIndex = cellRowColumnPairToLinearIndex(from.row(), from.column());
    const auto toIndex = cellRowColumnPairToLinearIndex(to.row(), to.column());
    if (direction == FindDirectionForward)
    {
        if (toIndex >= fromIndex)
            return toIndex - fromIndex;
        else
            return LinearIndex(getColumnCount()) * getRowCount() - (fromIndex - toIndex);
    }
    else // case: backward find
    {
        if (toIndex <= fromIndex)
            return fromIndex - toIndex;
        else
            return LinearIndex(getColumnCount()) * getRowCount() - (toIndex - fromIndex);
    }
}

QModelIndex CsvItemModel::findNextHighlighterMatch(QModelIndex seedIndex, // Seed which is advanced before doing first actual match.
                                                                                      const FindDirection direction)
{
    if (m_highlighters.empty())
        return QModelIndex();
    if (!seedIndex.isValid())
        seedIndex = index(0, 0);

    const auto usedHighlighter = m_highlighters.front();
    const auto advanceStyle = (usedHighlighter.m_nColumn >= 0) ? FindAdvanceStyleRowIncrement : FindAdvanceStyleLinear;
    const auto matcher = usedHighlighter.matcher();

    auto searchIndex = nextCellByFinderAdvance(seedIndex, direction, advanceStyle);

    const auto isMatchWith = [&](const QModelIndex& index)
    {
        return matcher.isMatchWith(table()(index.row(), index.column()));
    };

   if (searchIndex == seedIndex)
       return (isMatchWith(searchIndex)) ? searchIndex : QModelIndex();

    auto nextWrappedDistance = wrappedDistance(seedIndex, searchIndex, direction);
    decltype(nextWrappedDistance) previousWrappedDistance = -1;
    while (previousWrappedDistance < nextWrappedDistance)
    {
        previousWrappedDistance = nextWrappedDistance;
        if (isMatchWith(searchIndex))
            return searchIndex;
        searchIndex = nextCellByFinderAdvance(searchIndex, direction, advanceStyle);
        nextWrappedDistance = wrappedDistance(seedIndex, searchIndex, direction);
    }
    return QModelIndex();
}

bool CsvItemModel::setSize(Index nNewRowCount, Index nNewColCount)
{
    const auto nCurrentRowCount = rowCount();
    const auto nCurrentColCount = columnCount();

    nNewRowCount = (nNewRowCount >= 0) ? nNewRowCount : nCurrentRowCount;
    nNewColCount = (nNewColCount >= 0) ? nNewColCount : nCurrentColCount;

    bool bChanged = false;

    // Changing row count
    if (nNewRowCount != nCurrentRowCount)
    {
        const auto nPositiveCount = (nCurrentRowCount < nNewRowCount) ? nNewRowCount - nCurrentRowCount : nCurrentRowCount - nNewRowCount;
        if (nCurrentRowCount < nNewRowCount)
            this->insertRows(nCurrentRowCount, nPositiveCount);
        else
            this->removeRows(nCurrentRowCount - nPositiveCount, nPositiveCount);
        bChanged = true;
    }

    // Changing column count
    if (nNewColCount != nCurrentColCount)
    {
        const auto nPositiveCount = (nCurrentColCount < nNewColCount) ? nNewColCount - nCurrentColCount : nCurrentColCount - nNewColCount;
        if (nCurrentColCount < nNewColCount)
            this->insertColumns(nCurrentColCount, nPositiveCount);
        else
            this->removeColumns(nCurrentColCount - nPositiveCount, nPositiveCount);
        bChanged = true;
    }

    return bChanged;
}

bool CsvItemModel::transpose()
{
    const auto nOldRowCount = rowCount();
    const auto nOldColumnCount = columnCount();
    const auto nNewRowCount = nOldColumnCount;
    const auto nNewColumnCount = nOldRowCount;
    const auto nMaxRowCount = (std::max)(nOldRowCount, nNewRowCount);
    const auto nMaxColumnCount = nMaxRowCount; // This is equal to (std::max)(nOldColumnCount, nNewColumnCount);

    insertRows(nOldRowCount, nMaxRowCount - nOldRowCount);
    insertColumns(nOldColumnCount, nMaxColumnCount - nOldColumnCount);

    // Not running row and column inserts and removals inside resetModel() since QAbstractItemModelTester
    // in Qt 6.4.1 would cause test failures if those were called while in reset-phase.
    m_bResetting = true;
    beginResetModel();
    for (Index r = 0; r < nOldRowCount; ++r)
    {
        for (Index c = r + 1; c < nMaxColumnCount; ++c)
        {
            this->table().swapCellContent(r, c, c, r);
        }
    }
    endResetModel();
    m_bResetting = false;

    removeRows(nNewRowCount, nOldRowCount - nNewRowCount);
    removeColumns(nNewColumnCount, nOldColumnCount - nNewColumnCount);

    setModifiedStatus(true);

    if (m_pUndoStack)
        m_pUndoStack->clear();

    return true;
}

template <class This_T, class Func_T>
bool CsvItemModel::forEachColInfoWhileImpl(This_T& rThis, const LockReleaser& lockReleaser, Func_T&& func)
{
    if (!func || !lockReleaser.isLocked())
        return false;
    for (auto& colInfo : rThis.m_vecColInfo)
    {
        if (!func(colInfo))
            break;
    }
    return true;
}

bool CsvItemModel::forEachColInfoWhile(const LockReleaser& lockReleaser, std::function<bool(ColInfo&)> func)
{
    return forEachColInfoWhileImpl(*this, lockReleaser, std::move(func));
}

bool CsvItemModel::forEachColInfoWhile(const LockReleaser& lockReleaser, std::function<bool(const ColInfo&)> func) const
{
    return forEachColInfoWhileImpl(*this, lockReleaser, std::move(func));
}

auto CsvItemModel::table()       -> DataTableRef      { return m_table.impl(); }
auto CsvItemModel::table() const -> DataTableConstRef { return m_table.impl(); }

auto CsvItemModel::peekCsvFormatFromFile(const QString& sPath, const size_t nPeekLimitAsBaseChars) -> CsvFormatDefinition
{
    return ::DFG_ROOT_NS::peekCsvFormatFromFile(qStringToFileApi8Bit(sPath), nPeekLimitAsBaseChars);
}


//////////////////////////////////////////////////////////////////////////
//
// class CsvitemModel::ColInfo
//
//////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(CsvItemModel::ColInfo)
{
    template <class K_T, class V_T> using MapT = ::DFG_MODULE_NS(cont)::MapVectorSoA<K_T, V_T>;
    MapT<uintptr_t, MapT<StringUtf8, QVariant>> m_contextProperties; // Properties that column have in some context, e.g. visibility in some view.
    MapT<CsvItemModelColumnProperty, QVariant> m_properties; // Properties that column itself have
};

CsvItemModel::ColInfo::ColInfo(CsvItemModel* pModel, QString sName, ColType type, CompleterType complType)
    : m_spModel(pModel)
    , m_name(sName)
    , m_type(type)
    , m_completerType(complType)
{}

QString CsvItemModel::ColInfo::name() const
{
    return this->m_name;
}

auto CsvItemModel::ColInfo::index() const -> Index
{
    CsvItemModel* pModel = m_spModel;
    if (!pModel)
        return -1;
    auto iter = std::find_if(pModel->m_vecColInfo.begin(), pModel->m_vecColInfo.end(), [=](const ColInfo& colInfo) { return this == &colInfo; });
    return static_cast<Index>(iter - pModel->m_vecColInfo.begin());
}

QVariant CsvItemModel::ColInfo::getProperty(const CsvItemModelColumnProperty propertyId, const QVariant& defaultVal) const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return (pOpaq) ? pOpaq->m_properties.valueCopyOr(propertyId, defaultVal) : defaultVal;
}

QVariant CsvItemModel::ColInfo::getProperty(const uintptr_t& contextId, const StringViewUtf8& svPropertyId, const QVariant& defaultVal) const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    if (!pOpaq)
        return defaultVal;
    auto iter = pOpaq->m_contextProperties.find(contextId);
    if (iter == pOpaq->m_contextProperties.end())
        return defaultVal;
    return iter->second.valueCopyOr(svPropertyId, defaultVal);
}

template <class Cont_T, class Key_T, class ToStorageKey_T>
bool CsvItemModel::ColInfo::setPropertyImpl(Cont_T& cont, const Key_T& key, const QVariant& value, ToStorageKey_T funcToKey)
{
    auto iter = cont.find(key);
    bool bChanged = false;
    if (iter == cont.end())
    {
        bChanged = true;
        cont[funcToKey(key)] = value;
    }
    else if (iter->second != value)
    {
        bChanged = true;
        iter->second = value;
    }
    return bChanged;
}

template <class Cont_T, class Key_T>
bool CsvItemModel::ColInfo::setPropertyImpl(Cont_T& cont, const Key_T& key, const QVariant& value)
{
    return setPropertyImpl(cont, key, value, [](const Key_T& a) { return a; });
}

bool CsvItemModel::ColInfo::setProperty(const uintptr_t& contextId, const StringViewUtf8& svPropertyId, const QVariant& value)
{
    return setPropertyImpl(DFG_OPAQUE_REF().m_contextProperties[contextId], svPropertyId, value, [](const StringViewUtf8& sv) { return sv.toString(); });
}

bool CsvItemModel::ColInfo::setProperty(const CsvItemModelColumnProperty& columnProperty, const QVariant& value)
{
    return setPropertyImpl(DFG_OPAQUE_REF().m_properties, columnProperty, value);
}

auto CsvItemModel::ColInfo::columnTypeAsString() const -> StringViewUtf8
{
    return columnTypeAsString(this->m_type);
}

auto CsvItemModel::ColInfo::columnTypeAsString(const ColType colType) -> StringViewUtf8
{
    switch (colType)
    {
        case ColTypeNumber: return DFG_UTF8("number");
        default           : return DFG_UTF8("text");
    }
}

void CsvItemModel::setCellReadOnlyStatus(const Index nRow, const Index nCol, const bool bReadOnly)
{
    const auto key = cellRowColumnPairToIndexPairInteger(nRow, nCol);
    if (bReadOnly)
        DFG_OPAQUE_REF().m_readOnlyCells.insert(key);
    else
        DFG_OPAQUE_REF().m_readOnlyCells.erase(key);
}

namespace DFG_DETAIL_NS
{
    DFG_OPAQUE_PTR_DEFINE(CsvItemModelTable)
    {
        using DataTable = CsvItemModel::OpaqueTypeDefs::DataTable;

        DataTable m_table;
    };

    CsvItemModelTable::CsvItemModelTable() = default;
    CsvItemModelTable::~CsvItemModelTable() = default;

    auto CsvItemModelTable::impl() -> TableRef&
    {
        return static_cast<TableRef&>(DFG_OPAQUE_REF().m_table);
    }

    auto CsvItemModelTable::impl() const -> const TableRef&
    {
        return static_cast<const TableRef&>(DFG_OPAQUE_PTR()->m_table);
    }

    auto CsvItemModelTable::operator()(Index nRow, Index nCol) const -> SzPtrUtf8R
    {
        return impl()(nRow, nCol);
    }

    void CsvItemModelTable::setElement(Index nRow, Index nCol, StringViewUtf8 sv)
    {
        impl().setElement(nRow, nCol, sv);
    }

    void CsvItemModelTable::forEachFwdRowInColumnWhile(Index nCol, std::function<bool (Index)> whileFunc, std::function<void (Index, SzPtrUtf8R)> func) const
    {
        impl().forEachFwdRowInColumnWhile(nCol, whileFunc, func);
    }

    void CsvItemModelTable::forEachFwdRowInColumn(Index nCol, std::function<void (Index, SzPtrUtf8R)> func) const
    {
        impl().forEachFwdRowInColumn(nCol, func);
    }

    auto CsvItemModelTable::rowCountByMaxRowIndex() const -> Index
    {
        return impl().rowCountByMaxRowIndex();
    }

    auto CsvItemModelTable::colCountByMaxColIndex() const -> Index
    {
        return impl().colCountByMaxColIndex();
    }

    auto CsvItemModelTable::cellCountNonEmpty() const -> Index
    {
        return impl().cellCountNonEmpty();
    }

} // namespace DFG_DETAIL_NS


#if DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS
static const QString sMimeTypeStr = "text/csv";

QStringList CsvItemModel::mimeTypes() const
{
    QStringList types;
    //types << "application/vnd.text.list";
    types << sMimeTypeStr;
    return types;
}

QMimeData* CsvItemModel::mimeData(const QModelIndexList& indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    QString sRow;
    std::set<int> setRows;
    foreach(const QModelIndex& index, indexes)
    {
        if (index.isValid())
            setRows.insert(index.row());
    }
    foreach(const int nRow, setRows)
    {
        RowToString(nRow, sRow);
        stream << sRow;
    }

    mimeData->setData(sMimeTypeStr, encodedData);
    return mimeData;
}

bool CsvItemModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    Q_UNUSED(column);
    if (parent.isValid() || action == Qt::IgnoreAction || data == nullptr || !data->hasFormat(sMimeTypeStr))
        return false;
    QByteArray ba = data->data(sMimeTypeStr);
    QString s = ba;
    if (isValidRow(row))
        setRow(row, s);

    return true;
}

bool CsvItemModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    qDebug() << "CsvItemModel::canDropMimeData";
    return BaseClass::canDropMimeData(data, action, row, column, parent);
}

#endif // DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS

}} // namespace dfg::qt
/////////////////////////////////
