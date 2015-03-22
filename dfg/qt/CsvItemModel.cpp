#include "../buildConfig.hpp" // To get rid of C4996 "Function call with parameters that may be unsafe" in MSVC.
#include "CsvItemModel.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
//#include <QUndoStack>
//#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QCompleter>
#include <QTextStream>
#include <QStringListModel>
DFG_END_INCLUDE_QT_HEADERS

#include <set>
#include "../dfgBase.hpp"
#include "../io.hpp"
#include "../str/string.hpp"
#include "qtBasic.hpp"
#include "../io/DelimitedTextReader.hpp"
#include <boost/range/irange.hpp>

const QString DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::s_sEmpty;

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::DFG_CLASS_NAME(CsvItemModel)() :
    m_bModified(false),
    //m_pUndoStack(nullptr),
    m_bResetting(false),
    m_bEnableCompleter(false)
{
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFile()
{
    QString sNewPath = saveToFile(m_sFilePath);
    if (!sNewPath.isEmpty())
    {
        m_sFilePath = sNewPath;
        m_bModified = false;
    }
    return !sNewPath.isEmpty();
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFile(QString sPath, const SaveOptions& options)
{
    QFile fileData(sPath);
    // Commented out QFile::Text to prevent unwanted \n -> \r\n conversions.
    if (!fileData.open(QFile::WriteOnly | QFile::Truncate/* | QFile::Text*/))
    {
        return false;
    }

    return saveToFile(fileData, options);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFile(QFile& file, const SaveOptions& options)
{
    QTextStream strm(&file);
    return save(strm, options);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::save(QTextStream& strm, const SaveOptions& options)
{
    // TODO: revise implementation
    strm.setCodec("UTF-8");
    strm.setGenerateByteOrderMark(true);

    const char cSep = options.separatorChar();
    const char cEnc = options.enclosingChar();
    const auto cEol = DFG_MODULE_NS(io)::eolCharFromEndOfLineType(options.eolType());
    const auto sEolDummy = DFG_MODULE_NS(io)::eolStrFromEndOfLineType(options.eolType());
    const auto sEol = sEolDummy.c_str();

    if (options.saveHeader())
    {
        const auto headerRange = boost::irange<int>(0, m_vecColInfo.size());
        DFG_MODULE_NS(io)::writeDelimited(strm, dfg::makeRange(headerRange.begin(), headerRange.end()), cSep, [&](QTextStream& strm, int i)
        {
            DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellFromStrStrm(strm, getHeaderName(i), cSep, cEnc, cEol, DFG_MODULE_NS(io)::EbEncloseIfNeeded);
        });
        strm << sEol;
    }

    QString sLine;
    for (int r = 0; r<(int)m_vecData.size(); ++r)
    {
        sLine.clear();
        rowToString(r, sLine, cSep);
        strm << sLine;
        if (r + 1 < (int)m_vecData.size())
            strm << sEol;
    }

    return (strm.status() == QTextStream::Ok);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setItem(const int nRow, const int nCol, const QString str)
{
    if (!DFG_ROOT_NS::isValidIndex(m_vecData, nRow))
        m_vecData.resize(nRow+1);
    if (!DFG_ROOT_NS::isValidIndex(m_vecData[nRow], nCol))
        m_vecData[nRow].resize(nCol+1);
    this->m_vecData[nRow][nCol] = str;
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setRow(const int nRow, QString sLine)
{
    if (!this->isValidRow(nRow))
        return;
    const bool bHasTabs = (sLine.indexOf('\t') != -1);
    const wchar_t cSeparator = (bHasTabs) ? L'\t' : L',';
    const wchar_t cEnclosing = (cSeparator == ',') ? L'"' : L'\0';
    QTextStream strm(&sLine);
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::readRow(strm, cSeparator, cEnclosing, L'\n', [&](const size_t nCol, const wchar_t* const pszData, const size_t /*nDataLength*/)
    {
        setItem(nRow, nCol, QString::fromWCharArray(pszData));
    });

    if (!m_bResetting)
        emit dataChanged(this->index(nRow, 0), this->index(nRow, getColumnCount()-1));
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::clear()
{
    m_vecData.clear();
    m_vecColInfo.clear();
    m_sFilePath.clear();
    m_bModified = false;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openStream(QTextStream& strm, const LoadOptions& loadOptions)
{
    beginResetModel();
    m_bResetting = true;
    clear();
    
    const wchar_t cSeparator = loadOptions.separatorChar();
    const wchar_t cEnclosing = loadOptions.enclosingChar();
    const wchar_t cEol = DFG_MODULE_NS(io)::eolCharFromEndOfLineType(loadOptions.eolType());

    // Read header cols ->
    QStringList headerNames;
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::readRow(strm, cSeparator, cEnclosing, cEol, [&](const size_t /*nCol*/, const wchar_t* const pszData, const size_t /*nDataLength*/)
    {
        headerNames.push_back(QString::fromWCharArray(pszData));
    });
    m_vecColInfo.reserve(headerNames.size());
    // <- read header cols

    std::vector<std::set<QString>> vecCompletionSet;
    for (QStringList::const_iterator iter = headerNames.begin(); iter != headerNames.end(); ++iter)
    {
        m_vecColInfo.push_back(ColInfo(*iter));
        if (m_bEnableCompleter)
        {
            m_vecColInfo.back().m_spCompleter.reset(new QCompleter(this));
            m_vecColInfo.back().m_spCompleter->setCaseSensitivity(Qt::CaseInsensitive);
            m_vecColInfo.back().m_completerType = CompleterTypeTexts;
        }
        vecCompletionSet.push_back(std::set<QString>());
    }

    while (DFG_MODULE_NS(io)::isStreamInReadableState(strm))
    {
        m_vecData.push_back(std::vector<QString>(getColumnCount()));
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::readRow(strm, cSeparator, cEnclosing, cEol, [&](const size_t nCol, const wchar_t* const pszData, const size_t /*nDataLength*/)
        {
            if (nCol >= size_t(getColumnCount()))
                return;

            setItem(int(m_vecData.size() - 1), nCol, QString::fromWCharArray(pszData));

        });

        if (m_bEnableCompleter)
        {
            for (size_t i = 0; i < m_vecData.back().size(); ++i)
            {
                if (isValidIndex(vecCompletionSet, i))
                    vecCompletionSet[i].insert(m_vecData.back()[i]);
            }
        }
    }
    for (size_t i = 0; i<vecCompletionSet.size(); ++i)
    {
        QStringList stringlist;
        for each (const QString& str in vecCompletionSet[i])
            stringlist << str;
        if (m_vecColInfo[i].m_spCompleter)
            m_vecColInfo[i].m_spCompleter->setModel(new QStringListModel(stringlist));
    }
    m_bModified = false;
    endResetModel();
    m_bResetting = false;

    return true;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openString(QString str, const LoadOptions& loadOptions)
{
    QTextStream strm(&str);
    return openStream(strm, loadOptions);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openFile(QString sDbFilePath, const LoadOptions& loadOptions)
{
    if (sDbFilePath.isEmpty())
        return false;

    sDbFilePath = QFileInfo(sDbFilePath).absoluteFilePath();

    QFile fileData(sDbFilePath);

    if (fileData.open(QFile::ReadOnly))
    {
        m_sFilePath = sDbFilePath;
        QTextStream strmFile(&fileData);
        return openStream(strmFile, loadOptions);
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

QVariant DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
    if (!index.isValid())
         return QVariant();
    const int nRow = index.row();
    const int nCol = index.column();

    if ((role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) && this->hasIndex(nRow, nCol))
    {
        if (isValidIndex(m_vecData, nRow) && isValidIndex(m_vecData[nRow], nCol))
            return m_vecData[nRow][nCol];
        else
        {
            DFG_ASSERT(false);
            return QVariant();
        }
    }
    else
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

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setDataNoUndo(const QModelIndex& index, const QString& str)
{
    if (!isValidRow(index.row()) || !isValidColumn(index.column()))
    {
        DFG_ASSERT(false);
        return;
    }
    setItem(index.row(), index.column(), str);

    emit dataChanged(index, index);
    setModifiedStatus(true);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setData(const QModelIndex& index, const QVariant& value, int role /*= Qt::EditRole*/)
{
    // Sets the role data for the item at index to value.
    // Returns true if successful; otherwise returns false.
    // The dataChanged() signal should be emitted if the data was successfully set.

    if (index.isValid() && role == Qt::EditRole && isValidRow(index.row()) && isValidColumn(index.column()))
    {
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

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::insertRows(int position, int rows, const QModelIndex& parent /*= QModelIndex()*/)
{
    if (position < 0)
        position = m_vecData.size();
    if (parent.isValid() || position < 0 || position > (int)m_vecData.size())
        return false;
    beginInsertRows(QModelIndex(), position, position+rows-1);

    m_vecData.insert(m_vecData.begin() + position, rows, std::vector<QString>(getColumnCount(), ""));
    endInsertRows();
    setModifiedStatus(true);
    return true;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::removeRows(int position, int rows, const QModelIndex& parent /*= QModelIndex()*/)
{
    if (rows < 0 || parent.isValid() || !isValidRow(position) || !isValidRow(position + rows - 1))
        return false;
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    m_vecData.erase(m_vecData.begin() + position, m_vecData.begin() + position + rows);

    endRemoveRows();
    setModifiedStatus(true);
    return true;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::insertColumns(int position, int columns, const QModelIndex& parent /*= QModelIndex()*/)
{
    if (position < 0)
        position = getColumnCount();
    if (parent.isValid() || position < 0 || position > getColumnCount())
        return false;
    beginInsertColumns(QModelIndex(), position, position+columns-1);
    for(size_t i = 0; i<m_vecData.size(); ++i)
    {
        m_vecData[i].insert(m_vecData[i].begin() + position, columns, "");
    }
    for(int i = position; i<position + columns; ++i)
    {
        m_vecColInfo.insert(m_vecColInfo.begin() + i, ColInfo());
    }

    endInsertColumns();
    setModifiedStatus(true);
    return true;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::removeColumns(int position, int columns, const QModelIndex& parent /*= QModelIndex()*/)
{
    const int nLast = position + columns - 1;
    if (columns < 0 || parent.isValid() || !isValidColumn(position) || !isValidColumn(nLast))
        return false;
    beginRemoveColumns(QModelIndex(), position, position+columns-1);

    for(size_t i = 0; i<m_vecData.size(); ++i)
    {
        m_vecData[i].erase(m_vecData[i].begin() + position, m_vecData[i].begin() + nLast + 1);
    }
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
        emit headerDataChanged(Qt::Horizontal, nCol, nCol);
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
    vecStrings.reserve(m_vecData.size());
    for(size_t i = 0; i<m_vecData.size(); ++i)
    {
        vecStrings.push_back(m_vecData[i][nCol]);
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setColumnCells(const int nCol, const std::vector<QString>& vecStrings)
{
    if (!isValidColumn(nCol))
        return;
    for(size_t i = 0; i<m_vecData.size(); ++i)
    {
        m_vecData[i][nCol] = vecStrings[i];
    }
    if (!m_bResetting)
        emit dataChanged(this->index(0, nCol), this->index(getRowCount()-1, nCol));
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setModifiedStatus(const bool bMod /*= true*/)
{
    if (bMod != m_bModified)
    {
        m_bModified = bMod;
        emit sigModifiedStatusChanged(m_bModified);
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setColumnType(const int nCol, const ColType colType)
{
    if (!isValidColumn(nCol))
        return;
    m_vecColInfo[nCol].m_type = colType;
    setModifiedStatus(true);
    emit headerDataChanged(Qt::Horizontal, nCol, nCol);
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
