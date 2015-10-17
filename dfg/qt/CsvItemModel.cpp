#include "../buildConfig.hpp" // To get rid of C4996 "Function call with parameters that may be unsafe" in MSVC.
#include "CsvItemModel.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
//#include <QUndoStack>
//#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
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
#include "../io/OfStream.hpp"

const QString DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::s_sEmpty;

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::DFG_CLASS_NAME(CsvItemModel)() :
    m_bModified(false),
    //m_pUndoStack(nullptr),
    m_bResetting(false),
    m_bEnableCompleter(false)
{
}

QString DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFile()
{
    const bool bSuccess = saveToFile(m_sFilePath);
    if (bSuccess)
        m_bModified = false;
    return (bSuccess) ? m_sFilePath : QString();
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::saveToFile(const QString& sPath, const SaveOptions& options)
{
    QDir().mkpath(QFileInfo(sPath).absolutePath()); // Make sure that the target folder exists, otherwise opening the file will fail.

    // Note: This should be non-encoding stream. Encoding stream without encoding property is used here probably because it had the wchar_t-path version working
    //       at the time of writing. TODO: use plain ofstream.
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStreamWithEncoding) strm(sPath.toStdWString(), DFG_MODULE_NS(io)::encodingUnknown);

    if (!strm.is_open())
        return false;

    return save(strm, options);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::save(StreamT& strm, const SaveOptions& options)
{
    const QChar cSep = (DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::isMetaChar(options.separatorChar())) ? ',' : options.separatorChar();
    const QChar cEnc = options.enclosingChar();
    const QChar cEol = DFG_MODULE_NS(io)::eolCharFromEndOfLineType(options.eolType());
    const auto sEolDummy = DFG_MODULE_NS(io)::eolStrFromEndOfLineType(options.eolType());
    const auto sEol = sEolDummy.c_str();

    const auto encoding = options.textEncoding();
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

    return (strm.good());
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setItem(const int nRow, const int nCol, const QString str)
{
    const auto bRv = m_table.addString(str.toUtf8(), nRow, nCol);
    DFG_ASSERT(bRv); // Triggering ASSERT means that string couldn't be added to table.
    return bRv;
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setRow(const int nRow, QString sLine)
{
    if (!this->isValidRow(nRow))
        return;
    const bool bHasTabs = (sLine.indexOf('\t') != -1);
    const wchar_t cSeparator = (bHasTabs) ? L'\t' : L',';
    const wchar_t cEnclosing = (cSeparator == ',') ? L'"' : L'\0';
    QTextStream strm(&sLine);
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::readRow<wchar_t>(strm, cSeparator, cEnclosing, L'\n', [&](const size_t nCol, const wchar_t* const pszData, const size_t /*nDataLength*/)
    {
        setItem(nRow, static_cast<int>(nCol), QString::fromWCharArray(pszData));
    });

    if (!m_bResetting)
        emit dataChanged(this->index(nRow, 0), this->index(nRow, getColumnCount()-1));
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::clear()
{
    m_table.clear();
    m_vecColInfo.clear();
    m_sFilePath.clear();
    m_bModified = false;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::openStream(QTextStream& strm, const LoadOptions& loadOptions)
{
    beginResetModel();
    m_bResetting = true;
    clear();
    
    const auto cSeparator = loadOptions.separatorChar();
    const auto cEnclosing = loadOptions.enclosingChar();
    const auto cEol = DFG_MODULE_NS(io)::eolCharFromEndOfLineType(loadOptions.eolType());

    // Read header cols ->
    QStringList headerNames;
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::readRow<wchar_t>(strm, cSeparator, cEnclosing, cEol, [&](const size_t /*nCol*/, const wchar_t* const pszData, const size_t /*nDataLength*/)
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

    QString s;
    for(int nRow = 0; DFG_MODULE_NS(io)::isStreamInReadableState(strm); ++nRow)
    {
        //m_vecData.push_back(std::vector<QString>(getColumnCount()));
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::readRow<wchar_t>(strm, cSeparator, cEnclosing, cEol, [&](const size_t nCol, const wchar_t* const pszData, const size_t /*nDataLength*/)
        {
            if (nCol >= size_t(getColumnCount()))
                return;

            s = QString::fromWCharArray(pszData);

            setItem(nRow, static_cast<int>(nCol), s);

            if (m_bEnableCompleter)
            {
                if (isValidIndex(vecCompletionSet, nCol))
                    vecCompletionSet[nCol].insert(s);
            }

        });

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

    Q_EMIT sigOnNewSourceOpened();

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
        QTextStream strmFile(&fileData);
        const auto rv = openStream(strmFile, loadOptions);
        m_sFilePath = sDbFilePath; // Note: openStream() calls clear() so this line must be after it.
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

QVariant DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
    if (!index.isValid())
         return QVariant();
    const int nRow = index.row();
    const int nCol = index.column();

    if ((role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) && this->hasIndex(nRow, nCol))
    {
        const auto p = m_table(nRow, nCol);
        return (p) ? QString::fromUtf8(p) : QVariant();
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
    const auto nOldRowCount = m_table.rowCountByMaxRowIndex();
    if (position < 0)
        position = nOldRowCount;
    if (parent.isValid() || position < 0 || position > nOldRowCount)
        return false;
    const auto nLastNewRowIndex = position + rows - 1;
    beginInsertRows(QModelIndex(), position, nLastNewRowIndex);

    m_table.insertRowsAt(position, rows);
    if (position == nOldRowCount) // If appending at end, must add an empty element as otherwise table size won't change.
        m_table.setElement(nLastNewRowIndex, 0, "");
    endInsertRows();
    setModifiedStatus(true);
    return true;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::removeRows(int position, int rows, const QModelIndex& parent /*= QModelIndex()*/)
{
    if (rows < 0 || parent.isValid() || !isValidRow(position) || !isValidRow(position + rows - 1))
        return false;
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    m_table.removeRows(position, rows);

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
    m_table.insertColumnsAt(position, columns);
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

    m_table.eraseColumnsByPosAndCount(position, columns);
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
    vecStrings.reserve(getColumnCount());
    m_table.forEachFwdRowInColumn(nCol, [&](const int /*row*/, const char* psz)
    {
        vecStrings.push_back(QString::fromUtf8(psz));

    });
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::setColumnCells(const int nCol, const std::vector<QString>& vecStrings)
{
    if (!isValidColumn(nCol))
        return;
    const auto nRowCount = getRowCount();
    for (int r = 0; r<nRowCount; ++r)
    {
        setItem(r, nCol, vecStrings[r]);
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
