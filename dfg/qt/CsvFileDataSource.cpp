#include "CsvFileDataSource.hpp"
#include "qtBasic.hpp"
#include "../os.hpp"

::DFG_MODULE_NS(qt)::CsvFileDataSource::CsvFileDataSource(const QString& sPath, QString sId)
    : m_format(',', '"', ::DFG_MODULE_NS(io)::EndOfLineTypeN, ::DFG_MODULE_NS(io)::encodingUTF8)
{
    this->m_uniqueId = std::move(sId);
    this->m_bAreChangesSignaled = true; // Changes in file content are not signaled, but interpreting source as unchanging so from user's perspective this is as good as signaling.
    m_sPath = qStringToFileApi8Bit(sPath);

    if (!privUpdateStatusAndAvailability())
        return;
}

bool ::DFG_MODULE_NS(qt)::CsvFileDataSource::privUpdateStatusAndAvailability()
{
    using namespace DFG_MODULE_NS(io);
    using namespace DFG_MODULE_NS(os);
    const auto bOldAvailability = isAvailable();
    bool bAvailable = true;
    if (!isPathFileAvailable(m_sPath, FileModeExists))
    {
        m_sStatusDescription = tr("No file exists in given path '%1'").arg(fileApi8BitToQString(m_sPath));
        bAvailable = false;
    }
    else if (!isPathFileAvailable(m_sPath, FileModeRead))
    {
        m_sStatusDescription = tr("File exists but is not readable in path '%1'").arg(fileApi8BitToQString(m_sPath));
        bAvailable = false;
    }

    if (bAvailable)
    {
        if (bOldAvailability != bAvailable || m_columnIndexToColumnName.empty())
        {
            m_format = peekCsvFormatFromFile(m_sPath, 2048);
            auto istrm = createInputStreamBinaryFile(m_sPath);
            DelimitedTextReader::readRow<char>(istrm, m_format, [&](const size_t nCol, const char* const p, const size_t nSize)
            {
                m_columnIndexToColumnName.insert(saturateCast<DataSourceIndex>(nCol), StringViewUtf8(TypedCharPtrUtf8R(p), nSize));
            });
            m_sStatusDescription.clear();
        }
    }

    setAvailability(bAvailable);
    return true;
}

auto ::DFG_MODULE_NS(qt)::CsvFileDataSource::statusDescriptionImpl() const -> String
{
    return m_sStatusDescription;
}

QObject* ::DFG_MODULE_NS(qt)::CsvFileDataSource::underlyingSource()
{
    return nullptr; // Don't have an underlying source as QObject
}

namespace DFG_ROOT_NS { DFG_SUB_NS(qt) { namespace DFG_DETAIL_NS {

    class CsvCellHandler : public ::DFG_MODULE_NS(cont)::TableCsv<char, uint32>::CellHandlerBase
    {
    public:
        using QueryCallback = CsvFileDataSource::ForEachElementByColumHandler;
        using ColumnDataTypeMap = CsvFileDataSource::ColumnDataTypeMap;

        CsvCellHandler(DataSourceIndex nCol, const DataQueryDetails& queryDetails, ColumnDataTypeMap* pColumnDataTypeMap, QueryCallback func)
            : m_nColumn(nCol)
            , m_queryDetails(queryDetails)
            , m_pColumnDataTypeMap(pColumnDataTypeMap)
            , m_queryCallback(func)
        {
            DFG_REQUIRE(m_queryCallback.operator bool());
            m_stringBuffer.reserveContentStorage_byBaseCharCount(contentBlockSize() + 500);
        }

        static constexpr size_t contentBlockSize()
        {
            return 50000;
        }

        void operator()(const size_t nRow, const size_t nCol, const char* pData, const size_t nCount)
        {
            using namespace ::DFG_MODULE_NS(cont);
            if (nCol != m_nColumn)
                return;
            // Collecting strings to a separate buffer and sending data to query callback in batches.
            // If optimization is needed, note that in optimal case where non-enclosed UTF8 input is read from memory mapped file, pData is stable so could probably avoid the need for m_stringBuffer completely.
            m_stringBuffer.insert(nRow, StringViewUtf8(TypedCharPtrUtf8R(pData), nCount));
            if (m_stringBuffer.contentStorageSize() > contentBlockSize())
                callQueryCallback();
        }

        void onReadDone()
        {
            callQueryCallback(); // Sending remaining items if any.
        }

        void callQueryCallback()
        {
            if (m_stringBuffer.empty())
                return;
            // Note: queryMask is ignored for now; simply passing everything regardless of the actual request.
            const auto nCount = m_stringBuffer.size();
            m_rowBuffer.resize(nCount);
            m_valueBuffer.resize(nCount);
            m_stringViewBuffer.resize(nCount);
            auto iterRow = m_rowBuffer.begin();
            auto iterValue = m_valueBuffer.begin();
            auto iterViewBuffer = m_stringViewBuffer.begin();
            for (const auto& item : m_stringBuffer)
            {
                *iterRow++ = static_cast<double>(item.first);
                const auto view = item.second(m_stringBuffer);
                *iterValue++ = GraphDataSource::cellStringToDouble(view, m_nColumn, m_pColumnDataTypeMap);
                *iterViewBuffer++ = view.toStringView();
            }
            SourceDataSpan dataSpan;
            dataSpan.setRows(m_rowBuffer);
            dataSpan.set(m_valueBuffer);
            dataSpan.set(m_stringViewBuffer);
            m_queryCallback(dataSpan);
            m_stringBuffer.clear_noDealloc();
        }

        DataSourceIndex m_nColumn;
        DataQueryDetails m_queryDetails;
        QueryCallback m_queryCallback;
        ColumnDataTypeMap* m_pColumnDataTypeMap;
        ::DFG_MODULE_NS(cont)::MapToStringViews<DataSourceIndex, StringUtf8> m_stringBuffer;
        std::vector<double> m_rowBuffer;
        std::vector<double> m_valueBuffer;
        std::vector<StringViewUtf8> m_stringViewBuffer;
    }; // class CsvCellHandler

}}}

void ::DFG_MODULE_NS(qt)::CsvFileDataSource::forEachElement_byColumn(DataSourceIndex nCol, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler)
{
    ::DFG_MODULE_NS(cont)::TableCsv<char, uint32> table;
    table.readFromFile(m_sPath, m_format, DFG_DETAIL_NS::CsvCellHandler(nCol, queryDetails, &m_columnDataTypes, handler));
}

auto ::DFG_MODULE_NS(qt)::CsvFileDataSource::columnCount() const -> DataSourceIndex
{
    return (!m_columnIndexToColumnName.empty()) ? m_columnIndexToColumnName.backKey() + 1 : 0;
}

auto ::DFG_MODULE_NS(qt)::CsvFileDataSource::columnIndexes() const -> IndexList
{
    IndexList indexes;
    for (const auto& item : m_columnIndexToColumnName)
    {
        indexes.push_back(item.first);
    }
    return indexes;
}

auto ::DFG_MODULE_NS(qt)::CsvFileDataSource::columnIndexByName(const StringViewUtf8 sv) const -> DataSourceIndex
{
    return m_columnIndexToColumnName.keyByValue(sv, GraphDataSource::invalidIndex());
}

void ::DFG_MODULE_NS(qt)::CsvFileDataSource::enable(bool b)
{
    DFG_UNUSED(b);
}

auto ::DFG_MODULE_NS(qt)::CsvFileDataSource::columnDataTypes() const -> ColumnDataTypeMap
{
    return m_columnDataTypes;
}

auto ::DFG_MODULE_NS(qt)::CsvFileDataSource::columnNames() const -> ColumnNameMap
{
    ColumnNameMap rv;
    for (const auto& item : m_columnIndexToColumnName)
    {
        rv[item.first] = QString::fromUtf8(item.second(m_columnIndexToColumnName).c_str().c_str());
    }
    return rv;
}
