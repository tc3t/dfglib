#include "../buildConfig.hpp" // To get rid of C4996 "Function call with parameters that may be unsafe" in MSVC.
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
#include <QTextStream>
#include <QStringListModel>
DFG_END_INCLUDE_QT_HEADERS

#include <set>
#include "../cont/SortedSequence.hpp"
#include "../dfgBase.hpp"
#include "../io.hpp"
#include "../str/string.hpp"
#include "qtBasic.hpp"
#include "../io/DelimitedTextReader.hpp"
#include <boost/range/irange.hpp>
#include "../io/OfStream.hpp"
#include "../time/timerCpu.hpp"
#include "../cont/SetVector.hpp"

namespace
{
    enum CsvItemModelPropertyId
    {
        CsvItemModelPropertyId_completerEnabledColumnIndexes
    };

    DFG_QT_DEFINE_OBJECT_PROPERTY_CLASS(CsvItemModel);

    // Properties
    DFG_QT_DEFINE_OBJECT_PROPERTY("CsvItemModel_completerEnabledColumnIndexes",
                                  CsvItemModel,
                                  CsvItemModelPropertyId_completerEnabledColumnIndexes,
                                  QStringList,
                                  PropertyType);

    template <CsvItemModelPropertyId ID>
    auto getCsvItemModelProperty(DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)* pModel) -> typename DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CsvItemModel)<ID>::PropertyType
    {
        return DFG_MODULE_NS(qt)::getProperty<DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CsvItemModel)<ID>>(pModel);
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

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::ColInfo::~ColInfo()
{
}

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::DFG_CLASS_NAME(CsvItemModel)() :
    m_pUndoStack(nullptr),
    m_bModified(false),
    m_bResetting(false),
    m_readTimeInSeconds(-1),
    m_writeTimeInSeconds(-1)
{
}

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::~DFG_CLASS_NAME(CsvItemModel)()
{
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

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFile()
{
    return saveToFile(m_sFilePath);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFile(const QString& sPath)
{
    return saveToFile(sPath, SaveOptions());
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFile(const QString& sPath, const SaveOptions& options)
{
    QDir().mkpath(QFileInfo(sPath).absolutePath()); // Make sure that the target folder exists, otherwise opening the file will fail.

    // Note: This should be non-encoding stream. Encoding stream without encoding property is used here probably because it had the wchar_t-path version working
    //       at the time of writing. TODO: use plain ofstream.
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStreamWithEncoding) strm(sPath.toStdWString(), DFG_MODULE_NS(io)::encodingUnknown);

    if (!strm.is_open())
        return false;

    const bool bSuccess = save(strm, options);
    if (bSuccess)
    {
        setFilePathWithSignalEmit(sPath);
        setModifiedStatus(false);
    }
    Q_EMIT sigOnSaveToFileCompleted(bSuccess, m_writeTimeInSeconds);
    return bSuccess;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::save(StreamT& strm)
{
    return save(strm, SaveOptions());
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::save(StreamT& strm, const SaveOptions& options)
{
    DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) writeTimer;

    const QChar cSep = (DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::isMetaChar(options.separatorChar())) ? ',' : options.separatorChar();
    const QChar cEnc = options.enclosingChar();
    const QChar cEol = DFG_MODULE_NS(io)::eolCharFromEndOfLineType(options.eolType());
    const auto sEolDummy = DFG_MODULE_NS(io)::eolStrFromEndOfLineType(options.eolType());
    const auto sEol = sEolDummy.c_str();

    const auto encoding = (options.textEncoding() != DFG_MODULE_NS(io)::encodingUnknown) ? options.textEncoding() : DFG_MODULE_NS(io)::encodingUTF8;
    if (encoding != DFG_MODULE_NS(io)::encodingLatin1 && encoding != DFG_MODULE_NS(io)::encodingUTF8) // Unsupported encoding type.
        return false;

    const auto qStringToEncodedBytes = [&](const QString& s) -> std::string
                                        {
                                            return (encoding == DFG_MODULE_NS(io)::encodingUTF8) ? s.toUtf8().data() : s.toLatin1();
                                        };

    const std::string sSepEncoded = qStringToEncodedBytes(cSep);

    if (options.bomWriting())
    {
        const auto bomBytes = DFG_MODULE_NS(utf)::encodingToBom(encoding);
        strm.write(bomBytes.data(), bomBytes.size());
    }

    if (options.headerWriting())
    {
        QString sEncodedTemp;
        const auto headerRange = boost::irange<int>(0, static_cast<int>(m_vecColInfo.size()));
        DFG_MODULE_NS(io)::writeDelimited(strm, DFG_ROOT_NS::makeRange(headerRange.begin(), headerRange.end()), sSepEncoded, [&](StreamT& strm, int i)
        {
            sEncodedTemp.clear();
            DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellFromStrIter(std::back_inserter(sEncodedTemp), getHeaderName(i), cSep, cEnc, cEol, DFG_MODULE_NS(io)::EbEncloseIfNeeded);
            auto utf8Bytes = qStringToEncodedBytes(sEncodedTemp);
            strm.write(utf8Bytes.data(), utf8Bytes.size());
        });
        strm << sEol;
    }

    QString sLine;
    const int nRowCount = m_table.rowCountByMaxRowIndex();
    for (int r = 0; r<nRowCount; ++r)
    {
        sLine.clear();
        rowToString(r, sLine, cSep);
        const auto& utf8 = qStringToEncodedBytes(sLine);
        strm.write(utf8.data(), utf8.size());
        if (r + 1 < nRowCount)
            strm << sEol;
    }

    m_writeTimeInSeconds = static_cast<decltype(m_writeTimeInSeconds)>(writeTimer.elapsedWallSeconds());

    return strm.good();
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setItem(const int nRow, const int nCol, SzPtrUtf8R psz)
{
    const auto bRv = m_table.addString(psz, nRow, nCol);
    DFG_ASSERT(bRv); // Triggering ASSERT means that string couldn't be added to table.
    return bRv;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setItem(const int nRow, const int nCol, const QString str)
{
    return setItem(nRow, nCol, SzPtrUtf8R(str.toUtf8()));
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setRow(const int nRow, QString sLine)
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

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openFromMemory(const char* data, const size_t nSize, const LoadOptions& loadOptions)
{
    return readData(loadOptions, [&]()
    {
        m_table.readFromMemory(data, nSize, loadOptions);
    });
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::readData(const LoadOptions& options, std::function<void()> tableFiller)
{
    DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) readTimer;

    beginResetModel();
    m_bResetting = true;
    clear();

    tableFiller();

    const QString optionsCompleterColumns(options.getProperty("completerColumns", "not_given").c_str());
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
                completerEnabledColumns.insert(nCol);
        }
    }

    const auto isCompleterEnabledInColumn = [&](const int nCol)
            {
                return completerEnabledInAll || completerEnabledColumns.hasKey(nCol);
            };

    // Set headers.
    const auto nMaxColCount = m_table.colCountByMaxColIndex();
    m_vecColInfo.reserve(nMaxColCount);
    for (int c = 0; c < nMaxColCount; ++c)
    {
        SzPtrUtf8R p = m_table(0, c); // HACK: assumes header to be on row 0 and UTF8-encoding.
        m_vecColInfo.push_back(ColInfo((p) ? QString::fromUtf8(p.c_str()) : QString()));
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

    initCompletionFeature();

    m_bModified = false;
    endResetModel();
    m_bResetting = false;

    m_readTimeInSeconds = static_cast<decltype(m_readTimeInSeconds)>(readTimer.elapsedWallSeconds());
    if (m_pUndoStack)
        m_pUndoStack->clear();
    Q_EMIT sigOnNewSourceOpened();

    return true;
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
            vecCompletionSet.resize(nCol + 1);
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
        for each (const QString& str in vecCompletionSet[i])
            stringlist << str;
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

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::mergeAnotherTableToThis(const DFG_CLASS_NAME(CsvItemModel)& other)
{
    const auto nOtherRowCount = other.getRowCount();
    if (nOtherRowCount < 1)
        return false;
    const auto nOtherColumnCount = other.getColumnCount();
    std::vector<int> mapOtherColumnIndexToMerged(nOtherColumnCount, getColumnCount());
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
    
    // TODO: implement merging in m_table implementation. For example when merging big tables, this current implementation 
    //       copies data one-by-one while m_table could probably do it more efficiently by directly copying the string storage. In case of modifiable 'other',
    //       this.m_table could adopt the string storage.
    const auto nThisOriginalRowCount = getRowCount();
    beginInsertRows(QModelIndex(), nThisOriginalRowCount, nThisOriginalRowCount + nOtherRowCount - 1);
    other.m_table.forEachNonNullCell([&](const uint32 r, const uint32 c, SzPtrUtf8R s)
    {
        m_table.setElement(nThisOriginalRowCount + r, mapOtherColumnIndexToMerged[c], s);
    });
    endInsertRows();
    return true;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openString(const QString& str)
{
    return openString(str, LoadOptions());
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openString(QString str, const LoadOptions& loadOptions)
{
    const auto bytes = str.toUtf8();
    if (loadOptions.textEncoding() == DFG_MODULE_NS(io)::encodingUTF8)
        return openFromMemory(bytes.data(), bytes.size(), loadOptions);
    else
    {
        auto loadOptionsTemp = loadOptions;
        loadOptionsTemp.textEncoding(DFG_MODULE_NS(io)::encodingUTF8);
        return openFromMemory(bytes.data(), bytes.size(), loadOptionsTemp);
    }
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openFile(const QString& sDbFilePath)
{
    return openFile(sDbFilePath, LoadOptions());
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

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openFile(QString sDbFilePath, const LoadOptions& loadOptions)
{
    if (sDbFilePath.isEmpty())
        return false;

    const QFileInfo fileInfo(sDbFilePath);

    if (fileInfo.isFile() && fileInfo.isReadable())
    {
        sDbFilePath = fileInfo.absoluteFilePath();
        auto rv = readData(loadOptions, [&]()
        {
            m_table.readFromFile(sDbFilePath.toLocal8Bit().data(), loadOptions); // TODO: return value,
                                                                                 // TODO: non-lossy conversion from QString to string type that readFromFile() accepts, 
                                                                                 //       (essentially means that readFromFile should accept std::wstring)
            setFilePathWithoutSignalEmit(std::move(sDbFilePath));
        });      
        
        return rv;
    }
    else
    {
        return false;
    }

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

auto DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::RawStringPtrAt(const int nRow, const int nCol) const -> SzPtrUtf8R
{
    return SzPtrUtf8R(m_table(nRow, nCol));
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
        return (p) ? QString::fromUtf8(p.c_str()) : QVariant();
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

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setDataNoUndo(const int nRow, const int nCol, SzPtrUtf8R pszU8)
{
    if (!isValidRow(nRow) || !isValidColumn(nCol))
    {
        DFG_ASSERT(false);
        return;
    }

    // Check whether the new value is different from old to avoid setting modified even if nothing changes.
    const SzPtrUtf8R pExisting = m_table(nRow, nCol);
    if (pExisting && std::strlen(pszU8.c_str()) <= std::strlen(pExisting.c_str()))
    {
        if (std::strcmp(pExisting.c_str(), pszU8.c_str()) == 0) // Identical item? If yes, skip rest to avoid setting modified.
            return;
    }

    setItem(nRow, nCol, pszU8);

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

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::insertRows(int position, int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    const auto nOldRowCount = m_table.rowCountByMaxRowIndex();
    if (position < 0)
        position = nOldRowCount;
    if (parent.isValid() || position < 0 || position > nOldRowCount)
        return false;
    const auto nLastNewRowIndex = position + count - 1;
    beginInsertRows(QModelIndex(), position, nLastNewRowIndex);

    m_table.insertRowsAt(position, count);
    if (position == nOldRowCount) // If appending at end, must add an empty element as otherwise table size won't change.
        m_table.setElement(nLastNewRowIndex, 0, SzPtrUtf8(""));
    endInsertRows();
    setModifiedStatus(true);
    return true;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::removeRows(int position, int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    if (count < 0 || parent.isValid() || !isValidRow(position) || !isValidRow(position + count - 1))
        return false;
    beginRemoveRows(QModelIndex(), position, position + count - 1);

    m_table.removeRows(position, count);
    
    endRemoveRows();
    setModifiedStatus(true);
    return true;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::insertColumns(int position, int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    if (position < 0)
        position = getColumnCount();
    if (parent.isValid() || position < 0 || position > getColumnCount())
        return false;
    beginInsertColumns(QModelIndex(), position, position + count - 1);
    m_table.insertColumnsAt(position, count);
    for(int i = position; i<position + count; ++i)
    {
        m_vecColInfo.insert(m_vecColInfo.begin() + i, ColInfo());
    }

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
    if (isValidColumn(nCol))
    {
        m_vecColInfo[nCol].m_name = sName;
        setModifiedStatus(true);
        Q_EMIT headerDataChanged(Qt::Horizontal, nCol, nCol);
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::removeRows(const std::vector<int>& vecIndexesAscSorted)
{
    for(std::vector<int>::const_reverse_iterator iter = vecIndexesAscSorted.rbegin(); iter != vecIndexesAscSorted.rend(); ++iter)
    {
        this->removeRows(*iter, 1);
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::columnToStrings(const int nCol, std::vector<QString>& vecStrings)
{
    vecStrings.clear();
    if (!isValidColumn(nCol))
        return;
    vecStrings.reserve(getColumnCount());
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
        setItem(r, nCol, vecStrings[r]);
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
    if (!isValidColumn(nCol))
        return;
    m_vecColInfo[nCol].m_type = colType;
    setModifiedStatus(true);
    Q_EMIT headerDataChanged(Qt::Horizontal, nCol, nCol);
}

QString& DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::dataCellToString(const QString& sSrc, QString& sDst, const QChar cDelim) const
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
    bool bNoneAdded = true;
    for (int nCol = 0; nCol < nColCount; ++nCol)
    {
        if (pSetIgnoreColumns != nullptr && pSetIgnoreColumns->find(nCol) != pSetIgnoreColumns->end())
            continue;
        if (!bNoneAdded)
            str.push_back(cDelim);
        SzPtrUtf8R p = m_table(nRow, nCol);
        if (!p)
            continue;
        dataCellToString(QString::fromUtf8(p.c_str()), str, cDelim);
        bNoneAdded = false;
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
