#include "CsvFileDataSource.hpp"
#include "qtBasic.hpp"
#include "../os.hpp"
#include "qtIncludeHelpers.hpp"
#include "connectHelper.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QFileSystemWatcher>
DFG_END_INCLUDE_QT_HEADERS

::DFG_MODULE_NS(qt)::CsvFileDataSource::CsvFileDataSource(const QString& sPath, QString sId)
    : FileDataSource(sPath, sId)
    , m_format(',', '"', ::DFG_MODULE_NS(io)::EndOfLineTypeN, ::DFG_MODULE_NS(io)::encodingUTF8)
{
    if (!privUpdateStatusAndAvailability())
        return;
}

::DFG_MODULE_NS(qt)::CsvFileDataSource::~CsvFileDataSource() = default;

bool ::DFG_MODULE_NS(qt)::CsvFileDataSource::updateColumnInfoImpl()
{
    using namespace DFG_MODULE_NS(io);

    m_format = peekCsvFormatFromFile(m_sPath, 2048);
    auto istrm = createInputStreamBinaryFile(m_sPath);
    m_columnIndexToColumnName.clear_noDealloc();
    DelimitedTextReader::readRow<char>(istrm, m_format, [&](const size_t nCol, const char* const p, const size_t nSize)
    {
        m_columnIndexToColumnName.insert(saturateCast<DataSourceIndex>(nCol), StringViewUtf8(TypedCharPtrUtf8R(p), nSize));
    });
    return true;
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
            , m_sourceSpanBuffer(nCol, queryDetails, pColumnDataTypeMap, func)
        {
            DFG_REQUIRE(func.operator bool());
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
            m_sourceSpanBuffer.storeToBuffer(nRow, StringViewUtf8(TypedCharPtrUtf8R(pData), nCount));
        }

        void onReadDone()
        {
            callQueryCallback(); // Sending remaining items if any.
        }

        void callQueryCallback()
        {
            m_sourceSpanBuffer.submitData();
        }

        DataSourceIndex m_nColumn;
        ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::SourceSpanBuffer m_sourceSpanBuffer;
    }; // class CsvCellHandler

}}}

void ::DFG_MODULE_NS(qt)::CsvFileDataSource::forEachElement_byColumn(DataSourceIndex nCol, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler)
{
    ::DFG_MODULE_NS(cont)::TableCsv<char, uint32> table;
    table.readFromFile(m_sPath, m_format, DFG_DETAIL_NS::CsvCellHandler(nCol, queryDetails, &m_columnDataTypes, handler));
}
