#include "CsvItemModel.hpp"
#include "qtIncludeHelpers.hpp"
#include "PropertyHelper.hpp"

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
#include "../cont/SortedSequence.hpp"
#include "../dfgBase.hpp"
#include "../io.hpp"
#include "../str/string.hpp"
#include "qtBasic.hpp"
#include "../io/DelimitedTextReader.hpp"
#include "../io/OfStream.hpp"
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

namespace
{
    enum CsvItemModelPropertyId
    {
        CsvItemModelPropertyId_completerEnabledColumnIndexes,
        CsvItemModelPropertyId_completerEnabledSizeLimit, // Defines maximum size (in bytes) for completer enabled tables, i.e. input bigger than this limit will have no completer enabled.
        CsvItemModelPropertyId_maxFileSizeForMemoryStreamWrite, // If output file size estimate is less than this value, writing is tried with memory stream.
        CsvItemModelPropertyId_defaultFormatSeparator,
        CsvItemModelPropertyId_defaultFormatEnclosingChar,
        CsvItemModelPropertyId_defaultFormatEndOfLine
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

    template <CsvItemModelPropertyId ID>
    auto getCsvItemModelProperty(const DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)* pModel) -> typename DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CsvItemModel)<ID>::PropertyType
    {
        return DFG_MODULE_NS(qt)::getProperty<DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CsvItemModel)<ID>>(pModel);
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

const QString DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::s_sEmpty;

#define DFG_TEMP_SAVEOPTIONS_BASECLASS_INIT BaseClass(',', '"', DFG_MODULE_NS(io)::EndOfLineTypeN, DFG_MODULE_NS(io)::encodingUTF8)

DFG_MODULE_NS(qt)::CsvItemModel::SaveOptions::SaveOptions(CsvItemModel* pItemModel)
    : DFG_TEMP_SAVEOPTIONS_BASECLASS_INIT
{
    initFromItemModelPtr(pItemModel);
}

DFG_MODULE_NS(qt)::CsvItemModel::SaveOptions::SaveOptions(const CsvItemModel* pItemModel)
    : DFG_TEMP_SAVEOPTIONS_BASECLASS_INIT
{
    initFromItemModelPtr(pItemModel);
}

#undef DFG_TEMP_SAVEOPTIONS_BASECLASS_INIT

namespace
{
    static DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::SaveOptions defaultSaveOptions(const DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)* pItemModel)
    {
        typedef DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::SaveOptions SaveOptionsT;
        if (!pItemModel)
            return SaveOptionsT(pItemModel);
        SaveOptionsT rv = pItemModel->m_table.saveFormat();
        rv.separatorChar(getCsvItemModelProperty<CsvItemModelPropertyId_defaultFormatSeparator>(pItemModel));
        rv.enclosingChar(getCsvItemModelProperty<CsvItemModelPropertyId_defaultFormatEnclosingChar>(pItemModel));
        rv.eolType(DFG_MODULE_NS(io)::endOfLineTypeFromStr(getCsvItemModelProperty<CsvItemModelPropertyId_defaultFormatEndOfLine>(pItemModel).toStdString()));
        return rv;
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::SaveOptions::initFromItemModelPtr(const DFG_CLASS_NAME(CsvItemModel)* pItemModel)
{
    if (pItemModel)
    {
        // If model seems to have been opened from existing input (file/memory), use format of m_table, otherwise use save format from settings.
        *this = (pItemModel->latestReadTimeInSeconds() >= 0) ? pItemModel->m_table.saveFormat() : defaultSaveOptions(pItemModel);
        if (::DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::isMetaChar(this->separatorChar()))
            this->separatorChar(defaultSaveOptions(pItemModel).separatorChar());
        if (!pItemModel->isSupportedEncodingForSaving(textEncoding()))
        {
            // Encoding is not supported, fallback to UTF-8.
            textEncoding(DFG_MODULE_NS(io)::encodingUTF8);
        }
    }
}

QVariant DFG_MODULE_NS(qt)::DFG_DETAIL_NS::HighlightDefinition::data(const QAbstractItemModel& model, const QModelIndex& index, const int role) const
{
    DFG_ASSERT_CORRECTNESS(role != Qt::DisplayRole);
    // Negative column setting is interpreted as 'match any'.
    if (!index.isValid() || (m_nColumn >= 0 && index.column() != m_nColumn) || role != Qt::BackgroundRole)
        return QVariant();
    auto displayData = model.data(index);
    if (m_matcher.isMatchWith(displayData.toString()))
        return m_highlightBrush;
    else
        return QVariant();
}

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) { namespace
{
    class DFG_CLASS_NAME(CsvTableModelActionCellEdit) : public QUndoCommand
    {
        typedef DFG_CLASS_NAME(CsvItemModel) ModelT;
    public:
        DFG_CLASS_NAME(CsvTableModelActionCellEdit)(ModelT& rDataModel, const QModelIndex& index, const QString& sNew)
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
    }; // DFG_CLASS_NAME(CsvTableModelActionCellEdit)

} } } // dfg::qt::unnamed_namespace

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::ColInfo::CompleterDeleter::operator()(QCompleter* p) const
{
    if (p)
        p->deleteLater(); // Can't delete directly due to thread affinity (i.e. might get deleted from wrong thread triggering Qt asserts).
}

DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(qt)::CsvItemModel)
{
    std::shared_ptr<QReadWriteLock> m_spReadWriteLock;
};

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::DFG_CLASS_NAME(CsvItemModel)() :
    m_pUndoStack(nullptr),
    m_nRowCount(0),
    m_bModified(false),
    m_bResetting(false),
    m_readTimeInSeconds(-1),
    m_writeTimeInSeconds(-1)
{
    DFG_OPAQUE_REF().m_spReadWriteLock = std::make_shared<QReadWriteLock>(QReadWriteLock::Recursive);
}

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::~DFG_CLASS_NAME(CsvItemModel)()
{
}

auto ::DFG_MODULE_NS(qt)::CsvItemModel::getReadWriteLock() -> std::shared_ptr<QReadWriteLock>
{
    return DFG_OPAQUE_REF().m_spReadWriteLock;
}

auto ::DFG_MODULE_NS(qt)::CsvItemModel::tryLockForEdit() -> LockReleaser
{
    auto& spLock = DFG_OPAQUE_REF().m_spReadWriteLock;
    return (spLock && spLock->tryLockForWrite()) ? LockReleaser(spLock.get()) : LockReleaser();
}

auto ::DFG_MODULE_NS(qt)::CsvItemModel::tryLockForRead() const -> LockReleaser
{
    auto pOpaque = DFG_OPAQUE_PTR();
    if (!pOpaque)
        return LockReleaser();
    auto& spLock = pOpaque->m_spReadWriteLock;
    return (spLock && spLock->tryLockForRead()) ? LockReleaser(spLock.get()) : LockReleaser();
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setFilePathWithoutSignalEmit(QString s)
{
    m_sFilePath = std::move(s);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setFilePathWithSignalEmit(QString s)
{
    setFilePathWithoutSignalEmit(std::move(s));
    Q_EMIT sigSourcePathChanged();
}

QString DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::getTableTitle(const QString& sDefault) const
{
    if (!m_sTitle.isEmpty())
        return m_sTitle;
    const auto sFileName = QFileInfo(m_sFilePath).fileName();
    return (!sFileName.isEmpty()) ? sFileName : sDefault;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::isSupportedEncodingForSaving(const DFG_MODULE_NS(io)::TextEncoding encoding) const
{
    return (encoding == DFG_MODULE_NS(io)::encodingUTF8) || (encoding == DFG_MODULE_NS(io)::encodingLatin1);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFile()
{
    return saveToFile(m_sFilePath);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFile(const QString& sPath)
{
    return saveToFile(sPath, SaveOptions(this));
}

template <class OutFile_T, class Stream_T>
bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFileImpl(const QString& sPath, OutFile_T& outFile, Stream_T& strm, const SaveOptions& options)
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

auto DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::getOutputFileSizeEstimate() const -> uint64
{
    const uint64 nRows = static_cast<uint64>(m_table.rowCountByMaxRowIndex());
    const int nColCount = getColumnCount();
    uint64 nBytes = 0;
    for (int i = 0; i < nColCount; ++i)
        nBytes += static_cast<uint32>(getHeaderName(i).size()); // Underestimates if header name has non-ascii.
    nBytes += nRows * static_cast<uint64>(nColCount); // Separators and eol (assuming single-char eol)
    return 5 * (nBytes + m_table.contentStorageSizeInBytes()) / 4; // Put a little extra for enclosing chars etc.
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFile(const QString& sPath, const SaveOptions& options)
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

bool ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::exportAsSQLiteFile(const QString& sPath)
{
    return exportAsSQLiteFile(sPath, defaultSaveOptions(this));
}

bool ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::exportAsSQLiteFile(const QString& sPath, const SaveOptions& options)
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
            auto tpsz = m_table(r, c);
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

auto DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::getSaveOptions() const -> SaveOptions
{
    return SaveOptions(this);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::save(StreamT& strm)
{
    return save(strm, SaveOptions(this));
}

namespace
{
    template <class Strm_T>
    class CsvWritePolicy : public ::DFG_MODULE_NS(qt)::CsvItemModel::DataTable::WritePolicySimple<Strm_T>
    {
    public:
        typedef ::DFG_MODULE_NS(qt)::CsvItemModel::DataTable::WritePolicySimple<Strm_T> BaseClass;
        using CsvItemModel = ::DFG_MODULE_NS(qt)::CsvItemModel;

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

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::save(StreamT& strm, const SaveOptions& options)
{
    return saveImpl(strm, options);
}

template <class Stream_T>
bool DFG_MODULE_NS(qt)::CsvItemModel::saveImpl(Stream_T& strm, const SaveOptions& options)
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
            DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellFromStrIter(std::back_inserter(sEncodedTemp),
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
    m_table.writeToStream(strm, writePolicy);
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
    m_table.saveFormat(options);

    m_writeTimeInSeconds = static_cast<decltype(m_writeTimeInSeconds)>(writeTimer.elapsedWallSeconds());

    return strm.good();
}

bool ::DFG_MODULE_NS(qt)::CsvItemModel::setItem(const int nRow, const int nCol, const StringViewUtf8& sv)
{
    const auto bRv = m_table.addString(sv, nRow, nCol);
    DFG_ASSERT(bRv); // Triggering ASSERT means that string couldn't be added to table.
    return bRv;
}

bool ::DFG_MODULE_NS(qt)::CsvItemModel::setItem(const int nRow, const int nCol, const QString& str)
{
    return setItem(nRow, nCol, SzPtrUtf8R(str.toUtf8()));
}

void ::DFG_MODULE_NS(qt)::CsvItemModel::setRow(const int nRow, QString sLine)
{
    if (!this->isValidRow(nRow))
        return;
    const bool bHasTabs = (sLine.indexOf('\t') != -1);
    const wchar_t cSeparator = (bHasTabs) ? L'\t' : L',';
    const wchar_t cEnclosing = (cSeparator == ',') ? L'"' : L'\0';
    QTextStream strm(&sLine);
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::readRow<wchar_t>(strm, cSeparator, cEnclosing, L'\n', [&](const size_t nCol, const wchar_t* const p, const size_t nDataLength)
    {
        const auto nSize = (nDataLength > NumericTraits<int>::maxValue) ? NumericTraits<int>::maxValue : static_cast<int>(nDataLength);
        setItem(nRow, static_cast<int>(nCol), QString::fromWCharArray(p, nSize));
    });

    if (!m_bResetting)
        Q_EMIT dataChanged(this->index(nRow, 0), this->index(nRow, getColumnCount()-1));
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::clear()
{
    m_table.clear();
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
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openStream(QTextStream& strm)
{
    return openStream(strm, LoadOptions());
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openStream(QTextStream& strm, const LoadOptions& loadOptions)
{
    auto s = strm.readAll();
    return openString(s, loadOptions);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openFromMemory(const char* data, const size_t nSize, LoadOptions loadOptions)
{
    setCompleterHandlingFromInputSize(loadOptions, nSize);
    return readData(loadOptions, [&]()
    {
        m_table.readFromMemory(data, nSize, loadOptions);
        return true;
    });
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::readData(const LoadOptions& options, std::function<bool()> tableFiller)
{
    DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) readTimer;

    beginResetModel();
    m_bResetting = true;
    clear();

    // Actual read happens here
    const auto bTableFillerRv = tableFiller();

    m_nRowCount = m_table.rowCountByMaxRowIndex();

    const QString optionsCompleterColumns(options.getProperty(CsvOptionProperty_completerColumns, "not_given").c_str());
    const auto completerEnabledColumnsStrItems = optionsCompleterColumns != "not_given" ? optionsCompleterColumns.split(',') : getCsvItemModelProperty<CsvItemModelPropertyId_completerEnabledColumnIndexes>(this);
    const auto completerEnabledInAll = (completerEnabledColumnsStrItems.size() == 1 && completerEnabledColumnsStrItems[0].trimmed() == "*");
    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(SetVector)<int> completerEnabledColumns;

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
    const auto nMaxColCount = m_table.colCountByMaxColIndex();
    m_vecColInfo.reserve(static_cast<size_t>(nMaxColCount));
    for (int c = 0; c < nMaxColCount; ++c)
    {
        SzPtrUtf8R p = m_table(0, c); // HACK: assumes header to be on row 0 and UTF8-encoding.
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
    }
    // Since the header is stored separately in this model, remove it from the table.
    m_table.removeRows(0, 1);
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

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::initCompletionFeature()
{
    std::vector<std::set<QString>> vecCompletionSet;

    m_table.forEachFwdColumnIndex([&](const int nCol)
    {
        auto pColInfo = getColInfo(nCol);
        if (!pColInfo || !pColInfo->hasCompleter())
            return;
        if (!isValidIndex(vecCompletionSet, nCol))
            vecCompletionSet.resize(static_cast<size_t>(nCol) + 1);
        auto& completionSetForCurrentCol = vecCompletionSet[nCol];
        m_table.forEachFwdRowInColumn(nCol, [&](const int /*nRow*/, const SzPtrUtf8R pData)
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

int DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::findColumnIndexByName(const QString& sHeaderName, const int returnValueIfNotFound) const
{
    for (auto i = 0, nCount = getColumnCount(); i < nCount; ++i)
    {
        if (getHeaderName(i) == sHeaderName)
            return i;
    }
    return returnValueIfNotFound;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openNewTable()
{
    auto rv = readData(LoadOptions(), [&]()
    {
        // Create 2x2 table by default, seems to be a better choice than completely empty.
        // Note that row 0 is omitted as it is interpreted as header by readData()
        m_table.setElement(1, 0, DFG_UTF8(""));
        m_table.setElement(1, 1, DFG_UTF8(""));
        m_table.setElement(2, 0, DFG_UTF8(""));
        m_table.setElement(2, 1, DFG_UTF8(""));
        m_sFilePath.clear();
        setFilePathWithoutSignalEmit(QString());
        return true;
    });
    m_readTimeInSeconds = -1;
    return rv;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::mergeAnotherTableToThis(const DFG_CLASS_NAME(CsvItemModel)& other)
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
    other.m_table.forEachNonNullCell([&](const int r, const int c, SzPtrUtf8R s)
    {
        m_table.setElement(nThisOriginalRowCount + r, mapOtherColumnIndexToMerged[c], s);
    });
    m_nRowCount = nNewRowCount;
    endInsertRows();
    return true;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openString(const QString& str)
{
    return openString(str, LoadOptions());
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openString(const QString& str, const LoadOptions& loadOptions)
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

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::populateConfig(DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig)& config) const
{
    m_table.m_readFormat.appendToConfig(config);
}

auto DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::getLoadOptionsForFile(const QString& sFilePath) -> LoadOptions
{
    auto sConfFilePath = DFG_CLASS_NAME(CsvFormatDefinition)::csvFilePathToConfigFilePath(sFilePath);
    if (!QFileInfo::exists(sConfFilePath))
    {
        LoadOptions loadOptions;
        if (!::DFG_MODULE_NS(sql)::SQLiteDatabase::isSQLiteFile(sFilePath))
        {
            const auto s8bitPath = qStringToFileApi8Bit(sFilePath);
            const auto peekedFormat = peekCsvFormatFromFile(s8bitPath, Min(1024, saturateCast<int>(::DFG_MODULE_NS(os)::fileSize(s8bitPath))));
            loadOptions.eolType(peekedFormat.eolType());
        }
        return loadOptions;
    }
    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig) config;
    config.loadFromFile(qStringToFileApi8Bit(sConfFilePath));
    LoadOptions loadOptions;
    loadOptions.fromConfig(config);
    return loadOptions;
}

auto DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::getLoadOptionsFromConfFile() const -> LoadOptions
{
    return getLoadOptionsForFile(getFilePath());
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openFile(const QString& sDbFilePath)
{
    return openFile(sDbFilePath, getLoadOptionsForFile(sDbFilePath));
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::importFiles(const QStringList& paths)
{
    if (paths.empty())
        return true;

    for (auto iter = paths.cbegin(), iterEnd = paths.cend(); iter != iterEnd; ++iter)
    {
        DFG_CLASS_NAME(CsvItemModel) temp;
        temp.openFile(*iter);
        mergeAnotherTableToThis(temp);
    }
    return true; // TODO: more detailed return value (e.g. that how many were read successfully).
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setCompleterHandlingFromInputSize(LoadOptions& loadOptions, const uint64 nSizeInBytes, const CsvItemModel* pModel)
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

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {
 namespace DFG_DETAIL_NS
{
    ::DFG_MODULE_NS(cont)::IntervalSet<int> columnIntervalSetFromText(const StringViewSzC& sv)
    {
        using namespace ::DFG_MODULE_NS(cont);
        using namespace ::DFG_MODULE_NS(qt);
        return intervalSetFromString<int>(sv, (std::numeric_limits<int>::max)()).shift_raw(-CsvItemModel::internalColumnToVisibleShift());
    }

    class CancellableReader : public DFG_MODULE_NS(qt)::CsvItemModel::DataTable::DefaultCellHandler
    {
    public:
        using BaseClass = ::DFG_MODULE_NS(qt)::CsvItemModel::DataTable::DefaultCellHandler;
        using ItemModel = ::DFG_MODULE_NS(qt)::CsvItemModel;
        using ProgressController = ItemModel::LoadOptions::ProgressController;

        CancellableReader(ItemModel::DataTable& rTable, ProgressController& rProgressController)
            : BaseClass(rTable)
            , m_rProgressController(rProgressController)
        {}

        // Returns approximate increament in progress (in bytes).
        static uint64 handleProgressController(ProgressController& rProcessController, const uint64 processedCount, const size_t nRow, const size_t nCol, const size_t nCount)
        {
            if (rProcessController)
            {
                if (rProcessController.isTimeToUpdateProgress(nRow, nCol))
                {
                    if (!rProcessController(processedCount))
                    {
                        rProcessController.setCancelled(true);
                        throw CsvItemModel::DataTable::OperationCancelledException();
                    }
                }
            }
            return nCount + 1; // +1 is for separator/single-char eol
        }

        void handleProgressController(const size_t nRow, const size_t nCol, const size_t nCount)
        {
            m_nProcessedByteCount += handleProgressController(m_rProgressController, m_nProcessedByteCount, nRow, nCol, nCount);
        }

        void operator()(const size_t nRow, const size_t nCol, const char* pData, const size_t nCount)
        {
            BaseClass::operator()(nRow, nCol, pData, nCount);
            handleProgressController(nRow, nCol, nCount);
        }

        uint64 m_nProcessedByteCount = 0; // Approximate value of processed input bytes; things like \r\n and quoting are ignored so this often undercounts.
        ProgressController& m_rProgressController;
    }; // CancellableReader

    class CancellableFilterReader : public CancellableReader
    {
    public:
        using BaseClass = CancellableReader;
        using ItemModel = ::DFG_MODULE_NS(qt)::CsvItemModel;
        using DataTable = ::DFG_MODULE_NS(qt)::CsvItemModel::DataTable;
        using FilterCellHandler = decltype(DataTable().createFilterCellHandler());

        CancellableFilterReader(ItemModel::DataTable& rTable, ProgressController& rProgressController, FilterCellHandler& rFilterCellHandler)
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
        using ItemModel          = ::DFG_MODULE_NS(qt)::CsvItemModel;
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

        bool isMatch(const int nInputRow, const CsvItemModel::DataTable::RowContentFilterBuffer& rowBuffer)
        {
            CancellableReader::handleProgressController(m_rProgressController, 0, static_cast<size_t>(nInputRow), 0, 0);
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
} }} // namespace dfg::qt::DFG_DETAIL_NS

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openFile(QString sDbFilePath, LoadOptions loadOptions)
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
                    auto filter = m_table.createFilterCellHandler(DFG_DETAIL_NS::TextFilterMatcher(SzPtrUtf8(sFilterItems.c_str()), loadOptions.m_progressController));
                    if (!sIncludeRows.empty())
                        filter.setIncludeRows(intervalSetFromString<int>(sIncludeRows));
                    if (!sIncludeColumns.empty())
                        filter.setIncludeColumns(DFG_DETAIL_NS::columnIntervalSetFromText(sIncludeColumns));
                    m_table.readFromFile(sReadPath, loadOptions, filter);
                }
                else // Case: not having readFilters, only row/column include sets.
                {
                    auto filter = m_table.createFilterCellHandler();
                    DFG_DETAIL_NS::CancellableFilterReader reader(this->m_table, loadOptions.m_progressController, filter);
                    if (!sIncludeRows.empty())
                        filter.setIncludeRows(intervalSetFromString<int>(sIncludeRows));
                    else
                        filter.setIncludeRows(IntervalSet<int>::makeSingleInterval(0, maxValueOfType<int>()));
                    if (!sIncludeColumns.empty())
                        filter.setIncludeColumns(DFG_DETAIL_NS::columnIntervalSetFromText(sIncludeColumns));
                    m_table.readFromFile(sReadPath, loadOptions, reader);
                }
                m_sTitle = tr("%1 (Filtered open)").arg(QFileInfo(sDbFilePath).fileName());
            }
            else // Case: normal (non-filtered) read
            {
                m_table.readFromFile(sReadPath, loadOptions, DFG_DETAIL_NS::CancellableReader(this->m_table, loadOptions.m_progressController));
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
            return true; // Currently there's no error detection so always returning true (=success)
        });

        return rv;
    }
}

bool ::DFG_MODULE_NS(qt)::CsvItemModel::readDataFromSqlite(const QString& sDbFilePath, const QString& sQuery, LoadOptions& loadOptions)
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
        this->m_table.setElement(nRow, saturateCast<size_t>(nCol), SzPtrUtf8(var.toString().toUtf8())); // TODO: should allow storing utf16 directly without redundant utf8 QByteArray temporary (similar to MapToStringViews::insertRaw())
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

bool ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openFromSqlite(const QString& sDbFilePath, const QString& sQuery)
{
    auto loadOptions = getLoadOptionsForFile(sDbFilePath);
    return openFromSqlite(sDbFilePath, sQuery, loadOptions);
}

bool ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openFromSqlite(const QString& sDbFilePath, const QString& sQuery, LoadOptions& loadOptions)
{
    m_messagesFromLatestOpen.clear();
    // Limiting completer usage by file size is highly coarse for databases, but at least this can prevent simple huge query cases from using completers.
    setCompleterHandlingFromInputSize(loadOptions, static_cast<uint64>(QFileInfo(sDbFilePath).size()));
    return this->readData(loadOptions, [&]() { return readDataFromSqlite(sDbFilePath, sQuery, loadOptions); });
}

//Note: When implementing a table based model, rowCount() should return 0 when the parent is valid.
int DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    return (!parent.isValid()) ? getRowCount() : 0;
}


int DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    return (!parent.isValid()) ? getColumnCount() : 0;
}

auto DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::getColumnNames() const -> QStringList
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

auto DFG_MODULE_NS(qt)::CsvItemModel::RawStringPtrAt(const int nRow, const int nCol) const -> SzPtrUtf8R
{
    return SzPtrUtf8R(m_table(nRow, nCol));
}

auto DFG_MODULE_NS(qt)::CsvItemModel::rawStringViewAt(const int nRow, const int nCol) const -> StringViewUtf8
{
    return StringViewUtf8(m_table(nRow, nCol));
}

auto DFG_MODULE_NS(qt)::CsvItemModel::rawStringViewAt(const QModelIndex& index) const -> StringViewUtf8
{
    return rawStringViewAt(index.row(), index.column());
}

QVariant DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
    if (!index.isValid())
        return QVariant();
    const int nRow = index.row();
    const int nCol = index.column();

    if (!this->hasIndex(nRow, nCol))
        return QVariant();

    if ((role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole))
    {
        const SzPtrUtf8R p = m_table(nRow, nCol);
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

QVariant DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
        return QVariant();
    if (orientation == Qt::Horizontal)
    {
        if (section >= 0 && section < getColumnCount())
            return getHeaderName(section);
        else
            return "";
    }

    else
        return QVariant(QString("%1").arg(internalRowIndexToVisible(section)));
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setDataByBatch_noUndo(const RawDataTable& table, const SzPtrUtf8R pFill)
{
    using IntervalContainer = ::DFG_MODULE_NS(cont)::MapVectorSoA<int, ::DFG_MODULE_NS(cont) ::IntervalSet<int>>;
    IntervalContainer intervalsByColumn; 
    table.forEachFwdColumnIndex([&](const int c)
    {
        table.forEachFwdRowInColumn(c, [&](const int r, SzPtrUtf8R tpsz)
        {
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
        intervalSet.forEachContiguousRange([&](const int up, const int down)
        {
            Q_EMIT dataChanged(this->index(up, nCol), this->index(down, nCol));
        });
    }
    
    setModifiedStatus(true);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::privSetDataToTable(const int nRow, const int nCol, SzPtrUtf8R pszU8)
{
    if (!isValidRow(nRow) || !isValidColumn(nCol))
    {
        DFG_ASSERT(false);
        return false;
    }

    // Check whether the new value is different from old to avoid setting modified even if nothing changes.
    const SzPtrUtf8R pExisting = m_table(nRow, nCol);
    if (pExisting && std::strlen(pszU8.c_str()) <= std::strlen(pExisting.c_str()))
    {
        if (std::strcmp(pExisting.c_str(), pszU8.c_str()) == 0) // Identical item? If yes, skip rest to avoid setting modified.
            return false;
    }

    return setItem(nRow, nCol, pszU8);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setDataNoUndo(const int nRow, const int nCol, SzPtrUtf8R pszU8)
{
    if (!privSetDataToTable(nRow, nCol, pszU8))
        return;

    auto indexItem = index(nRow, nCol);
    Q_EMIT dataChanged(indexItem, indexItem);
    setModifiedStatus(true);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setDataNoUndo(const QModelIndex& index, const SzPtrUtf8R pszU8)
{
    setDataNoUndo(index.row(), index.column(), pszU8);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setDataNoUndo(const QModelIndex& index, const QString& str)
{
    setDataNoUndo(index.row(), index.column(), str);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setDataNoUndo(const int nRow, const int nCol, const QString& str)
{
    setDataNoUndo(nRow, nCol, SzPtrUtf8(str.toUtf8().data()));
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setData(const QModelIndex& index, const QVariant& value, int role /*= Qt::EditRole*/)
{
    // Sets the role data for the item at index to value.
    // Returns true if successful; otherwise returns false.
    // The dataChanged() signal should be emitted if the data was successfully set.

    if (index.isValid() && role == Qt::EditRole && isValidRow(index.row()) && isValidColumn(index.column()))
    {
        if (m_pUndoStack)
            m_pUndoStack->push(new DFG_CLASS_NAME(CsvTableModelActionCellEdit)(*this, index, value.toString()));
        else
            setDataNoUndo(index, value.toString());
        return true;
     }
     return false;
}

bool ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setHeaderData(const int section, Qt::Orientation orientation, const QVariant &value, const int role)
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

Qt::ItemFlags DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::flags(const QModelIndex& index) const
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
            return (f | Qt::ItemIsEnabled);
    #endif

}

int DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::getRowCountUpperBound() const
{
    // For now mostly a dedicated replacement for
    // NumericTraits<int>::maxValue for integer addition overflow control.
    return NumericTraits<int>::maxValue - 1;
}

int DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::getColumnCountUpperBound() const
{
    // For now mostly a dedicated replacement for
    // NumericTraits<int>::maxValue for integer addition overflow control.
    return NumericTraits<int>::maxValue - 1;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::insertRows(int position, const int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    const auto nOldRowCount = getRowCount();
    if (position < 0)
        position = nOldRowCount;
    if (parent.isValid() || count <= 0 || position < 0 || position > nOldRowCount || getRowCountUpperBound() - nOldRowCount < count)
        return false;
    const auto nLastNewRowIndex = position + count - 1;
    beginInsertRows(QModelIndex(), position, nLastNewRowIndex);
    m_table.insertRowsAt(position, count);
    m_nRowCount += count;
    endInsertRows();
    setModifiedStatus(true);
    return true;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::removeRows(int position, int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    if (count <= 0 || parent.isValid() || !isValidRow(position) || getRowCountUpperBound() - position < count  || !isValidRow(position + count - 1))
        return false;

    const auto nOriginalCount = getRowCount();

    beginRemoveRows(QModelIndex(), position, position + count - 1);

    m_table.removeRows(position, count);
    m_nRowCount -= count;

    endRemoveRows();

    const auto nNewCount = getRowCount();

    DFG_ASSERT_CORRECTNESS(nOriginalCount - nNewCount == count);
    DFG_UNUSED(nOriginalCount); // To avoid warning in release-build
    DFG_UNUSED(nNewCount); // To avoid warning in release-build

    setModifiedStatus(true);
    return true;
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::insertColumnsImpl(int position, int count)
{
    m_table.insertColumnsAt(position, count);
    for(int i = position; i<position + count; ++i)
    {
        m_vecColInfo.insert(m_vecColInfo.begin() + i, ColInfo(this));
    }
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::insertColumns(int position, const int count, const QModelIndex& parent /*= QModelIndex()*/)
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

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::removeColumns(int position, int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    const int nLast = position + count - 1;
    if (count < 0 || parent.isValid() || !isValidColumn(position) || !isValidColumn(nLast))
        return false;
    beginRemoveColumns(QModelIndex(), position, position + count - 1);

    m_table.eraseColumnsByPosAndCount(position, count);
    m_vecColInfo.erase(m_vecColInfo.begin() + position, m_vecColInfo.begin() + nLast + 1);

    endRemoveColumns();
    setModifiedStatus(true);
    return true;
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setColumnName(const int nCol, const QString& sName)
{
    auto pColInfo = getColInfo(nCol);
    if (pColInfo)
    {
        pColInfo->m_name = sName;
        setModifiedStatus(true);
        Q_EMIT headerDataChanged(Qt::Horizontal, nCol, nCol);
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::removeRows(const ::DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ArrayWrapperT)<const int>& indexesAscSorted)
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

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::columnToStrings(const int nCol, std::vector<QString>& vecStrings)
{
    vecStrings.clear();
    if (!isValidColumn(nCol))
        return;
    vecStrings.reserve(static_cast<size_t>(getColumnCount()));
    m_table.forEachFwdRowInColumn(nCol, [&](const int /*row*/, const SzPtrUtf8R tpsz)
    {
        vecStrings.push_back(QString::fromUtf8(tpsz.c_str()));

    });
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setColumnCells(const int nCol, const std::vector<QString>& vecStrings)
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

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setModifiedStatus(const bool bMod /*= true*/)
{
    if (bMod != m_bModified)
    {
        m_bModified = bMod;
        Q_EMIT sigModifiedStatusChanged(m_bModified);
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setColumnType(const int nCol, const ColType colType)
{
    auto pColInfo = getColInfo(nCol);
    if (!pColInfo)
        return;
    pColInfo->m_type = colType;
    setModifiedStatus(true);
    Q_EMIT headerDataChanged(Qt::Horizontal, nCol, nCol);
}

QString& DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::dataCellToString(const QString& sSrc, QString& sDst, const QChar cDelim)
{
    QTextStream strm(&sDst);
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellFromStrStrm(strm,
        sSrc,
        cDelim,
        QChar('"'),
        QChar('\n'),
        DFG_MODULE_NS(io)::EbEncloseIfNeeded);
    return sDst;
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::rowToString(const int nRow, QString& str, const QChar cDelim, const IndexSet* pSetIgnoreColumns /*= nullptr*/) const
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
        SzPtrUtf8R p = m_table(nRow, nCol);
        if (!p)
            continue;
        dataCellToString(QString::fromUtf8(p.c_str()), str, cDelim);
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setUndoStack(QUndoStack* pStack)
{
    m_pUndoStack = pStack;
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setHighlighter(HighlightDefinition hld)
{
    beginResetModel();
    auto existing = std::find_if(m_highlighters.begin(), m_highlighters.end(), [&](const HighlightDefinition& a) { return hld.m_id == a.m_id; });
    if (existing != m_highlighters.end())
        *existing = hld;
    else
        m_highlighters.push_back(hld);
    endResetModel();
}

QModelIndexList DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::match(const QModelIndex& start, int role, const QVariant& value, const int hits, const Qt::MatchFlags flags) const
{
    // match() is not adequate for needed find functionality (e.g. misses backward find, https://bugreports.qt.io/browse/QTBUG-344)
    // so find implementation is not using match().
    return BaseClass::match(start, role, value, hits, flags);
}

QModelIndex DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::nextCellByFinderAdvance(const QModelIndex& seedIndex, const FindDirection direction, const FindAdvanceStyle advanceStyle) const
{
    int r = Max(0, seedIndex.row());
    int c = Max(0, seedIndex.column());
    nextCellByFinderAdvance(r, c, direction, advanceStyle);
    return index(r, c);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::nextCellByFinderAdvance(int& r, int& c, const FindDirection direction, const FindAdvanceStyle advanceStyle) const
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

auto DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::wrappedDistance(const QModelIndex& from, const QModelIndex& to, const FindDirection direction) const -> LinearIndex
{
    if (!from.isValid() || !to.isValid())
        return 0;
    const auto fromIndex = DFG_ROOT_NS::pairIndexToLinear<LinearIndex>(from.row(), from.column(), getColumnCount());
    const auto toIndex = DFG_ROOT_NS::pairIndexToLinear<LinearIndex>(to.row(), to.column(), getColumnCount());
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

QModelIndex DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::findNextHighlighterMatch(QModelIndex seedIndex, // Seed which is advanced before doing first actual match.
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

    const auto& table = m_table;

    const auto isMatchWith = [&](const QModelIndex& index)
    {
        return matcher.isMatchWith(table(index.row(), index.column()));
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

/////////////////////////////////
// Start of dfg::qt namespace
DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt)
{

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

    m_bResetting = true;
    beginResetModel();

    insertRows(nOldRowCount, nMaxRowCount - nOldRowCount);
    insertColumns(nOldColumnCount, nMaxColumnCount - nOldColumnCount);

    for (Index r = 0; r < nOldRowCount; ++r)
    {
        for (Index c = r + 1; c < nMaxColumnCount; ++c)
        {
            this->m_table.swapCellContent(r, c, c, r);
        }
    }
    removeRows(nNewRowCount, nOldRowCount - nNewRowCount);
    removeColumns(nNewColumnCount, nOldColumnCount - nNewColumnCount);

    endResetModel();
    m_bResetting = false;
    setModifiedStatus(true);

    if (m_pUndoStack)
        m_pUndoStack->clear();

    return true;
}

template <class This_T, class Func_T>
void CsvItemModel::forEachColInfoWhileImpl(This_T& rThis, Func_T&& func)
{
    if (!func)
        return;
    for (auto& colInfo : rThis.m_vecColInfo)
    {
        if (!func(colInfo))
            break;
    }
}

void CsvItemModel::forEachColInfoWhile(std::function<bool(ColInfo&)> func)
{
    forEachColInfoWhileImpl(*this, std::move(func));
}

void CsvItemModel::forEachColInfoWhile(std::function<bool(const ColInfo&)> func) const
{
    forEachColInfoWhileImpl(*this, std::move(func));
}

//////////////////////////////////////////////////////////////////////////
//
// class CsvitemModel::ColInfo
//
//////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(qt)::CsvItemModel::ColInfo)
{
    template <class K_T, class V_T> using MapT = ::DFG_MODULE_NS(cont)::MapVectorSoA<K_T, V_T>;
    MapT<uintptr_t, MapT<StringUtf8, QVariant>> m_properties;
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

QVariant CsvItemModel::ColInfo::getProperty(const uintptr_t& contextId, const StringViewUtf8& svPropertyId, const QVariant& defaultVal) const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    if (!pOpaq)
        return defaultVal;
    auto iter = pOpaq->m_properties.find(contextId);
    if (iter == pOpaq->m_properties.end())
        return defaultVal;
    return iter->second.valueCopyOr(svPropertyId, defaultVal);
}

bool CsvItemModel::ColInfo::setProperty(const uintptr_t& contextId, const StringViewUtf8& svPropertyId, const QVariant& value)
{
    auto& m = DFG_OPAQUE_REF().m_properties[contextId];
    auto iter = m.find(svPropertyId);
    bool bChanged = false;
    if (iter == m.end())
    {
        bChanged = true;
        m[svPropertyId.toString()] = value;
    }
    else if (iter->second != value)
    {
        bChanged = true;
        iter->second = value;
    }
    return bChanged;
}

}} // namespace dfg::qt
/////////////////////////////////



#if DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS
static const QString sMimeTypeStr = "text/csv";

QStringList DFG_CLASS_NAME(CsvItemModel)::mimeTypes() const
{
    QStringList types;
    //types << "application/vnd.text.list";
    types << sMimeTypeStr;
    return types;
}

QMimeData* DFG_CLASS_NAME(CsvItemModel)::mimeData(const QModelIndexList& indexes) const
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

bool DFG_CLASS_NAME(CsvItemModel)::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    Q_UNUSED(column);
    if (parent.isValid() || action == Qt::IgnoreAction || data == nullptr || !data->hasFormat(sMimeTypeStr))
        return false;
    QByteArray ba = data->data(sMimeTypeStr);
    QString s = ba;
    if (IsValidRow(row))
        setRow(row, s);

    return true;
}

#endif // DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS
