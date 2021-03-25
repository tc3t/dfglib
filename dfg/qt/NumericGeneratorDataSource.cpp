#include "NumericGeneratorDataSource.hpp"
#include "../alg.hpp"
#include "../numericTypeTools.hpp"

DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(qt)::NumericGeneratorDataSource)
{
public:
    using DataSourceIndex = ::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::DataSourceIndex;
    DataSourceIndex m_nRowCount = 0;
    DataSourceIndex m_nTransferBlockSize = NumericGeneratorDataSource::defaultTransferBlockSize();
};

::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::NumericGeneratorDataSource(QString sId, const DataSourceIndex nRowCount)
    : NumericGeneratorDataSource(std::move(sId), nRowCount, defaultTransferBlockSize())
{
}

::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::NumericGeneratorDataSource(QString sId, const DataSourceIndex nRowCount, const DataSourceIndex transferBlockSize)
{
    this->m_uniqueId = std::move(sId);
    this->m_bAreChangesSignaled = true;
    DFG_OPAQUE_REF().m_nRowCount = Min(nRowCount, maxRowCount());
    DFG_OPAQUE_REF().m_nTransferBlockSize = Max(DataSourceIndex(1), Min(transferBlockSize, DFG_OPAQUE_REF().m_nRowCount));
}

::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::~NumericGeneratorDataSource() = default;

auto ::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::columnCount() const -> DataSourceIndex
{
    return 1;
}

auto ::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::defaultTransferBlockSize() -> DataSourceIndex
{
    return ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::SourceSpanBuffer::contentBlockSize();
}

auto ::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::maxRowCount() -> DataSourceIndex
{
    return (std::numeric_limits<DataSourceIndex>::max)() - 1;
}

auto ::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::rowCount() const -> DataSourceIndex
{
    auto p = DFG_OPAQUE_PTR();
    return (p) ? p->m_nRowCount : 0;
}

auto ::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::columnIndexes() const -> IndexList
{
    return {0};
}

void ::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::enable(bool b)
{
    DFG_UNUSED(b);
}

auto ::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::columnNames() const -> ColumnNameMap
{
    ColumnNameMap names;
    for (DataSourceIndex i = 0, nCount = columnCount(); i < nCount; ++i)
    {
        names.insert(i, String());
    }
    return names;
}

auto ::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::columnDataTypes() const -> ColumnDataTypeMap
{
    ColumnDataTypeMap types;
    for (DataSourceIndex i = 0, nCount = columnCount(); i < nCount; ++i)
    {
        types.insert(i, ChartDataType::unknown);
    }
    return types;
}

auto ::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::columnDataType(const DataSourceIndex) const -> ChartDataType
{
    return ChartDataType::unknown;
}

void ::DFG_MODULE_NS(qt)::NumericGeneratorDataSource::forEachElement_byColumn(const DataSourceIndex nCol, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler)
{
    if (!handler || nCol >= this->columnCount() || !queryDetails.areNumbersRequested())
        return;

    const auto nTotalRowCount = this->rowCount();
    const auto nTransferBlockSize = (DFG_OPAQUE_PTR()) ? DFG_OPAQUE_PTR()->m_nTransferBlockSize : defaultTransferBlockSize();

    std::vector<double> buffer(Min(nTotalRowCount, nTransferBlockSize));
    for (DataSourceIndex i = 0; i < nTotalRowCount; i = saturateAdd<decltype(i)>(i, buffer.size()))
    {
        const auto nBufferSize = Min(nTotalRowCount - i, buffer.size());
        auto effectiveRange = makeRange(buffer.begin(), buffer.begin() + nBufferSize);
        ::DFG_MODULE_NS(alg)::generateAdjacent(effectiveRange, i, 1);
        SourceDataSpan dataBlob;
        dataBlob.set(effectiveRange);
        handler(dataBlob);
    }
}
