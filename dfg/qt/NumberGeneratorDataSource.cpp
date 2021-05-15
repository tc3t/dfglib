#include "NumberGeneratorDataSource.hpp"
#include "../alg.hpp"
#include "../numericTypeTools.hpp"
#include "../math/sign.hpp"

DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(qt)::NumberGeneratorDataSource)
{
public:
    using DataSourceIndex = ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::DataSourceIndex;
    DataSourceIndex m_nRowCount = 0;
    double m_first = std::numeric_limits<double>::quiet_NaN();
    double m_step = std::numeric_limits<double>::quiet_NaN();
    DataSourceIndex m_nTransferBlockSize = NumberGeneratorDataSource::defaultTransferBlockSize();
};

::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::NumberGeneratorDataSource(QString sId, const DataSourceIndex nRowCount)
    : NumberGeneratorDataSource(std::move(sId), nRowCount, defaultTransferBlockSize())
{
}

::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::NumberGeneratorDataSource(QString sId, const DataSourceIndex nRowCount, const DataSourceIndex transferBlockSize)
    : NumberGeneratorDataSource(std::move(sId), nRowCount, 0, 1, transferBlockSize)
{
}

::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::NumberGeneratorDataSource(QString sId, const DataSourceIndex nRowCount, const double first, const double step, const DataSourceIndex transferBlockSize)
{
    this->m_uniqueId = std::move(sId);
    this->m_bAreChangesSignaled = true;
    DFG_OPAQUE_REF().m_nRowCount = Min(nRowCount, maxRowCount());
    DFG_OPAQUE_REF().m_first = first;
    DFG_OPAQUE_REF().m_step = step;
    DFG_OPAQUE_REF().m_nTransferBlockSize = Max(DataSourceIndex(1), Min(transferBlockSize, DFG_OPAQUE_REF().m_nRowCount));
}

::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::~NumberGeneratorDataSource() = default;

auto ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::createByCountFirstStep(QString sId, DataSourceIndex nRowCount, double first, double step) -> std::unique_ptr<NumberGeneratorDataSource>
{
    return std::unique_ptr<NumberGeneratorDataSource>(new NumberGeneratorDataSource(std::move(sId), nRowCount, first, step, defaultTransferBlockSize()));
}

auto ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::createByCountFirstLast(QString sId, DataSourceIndex nRowCount, double first, double last) -> std::unique_ptr<NumberGeneratorDataSource>
{
    using namespace ::DFG_MODULE_NS(math);
    if (nRowCount <= 0 || (nRowCount == 1 && first != last))
        return nullptr;
    const auto step = (nRowCount > 1) ? (last - first) / static_cast<double>(nRowCount - 1) : 0;
    if (!isFinite(first) || !isFinite(step) || signBit(last - first) != signBit(step))
        return nullptr;
    return std::unique_ptr<NumberGeneratorDataSource>(new NumberGeneratorDataSource(std::move(sId), nRowCount, first, step, defaultTransferBlockSize()));
}

auto ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::createByfirstLastStep(QString sId, const double first, const double last, const double step) -> std::unique_ptr<NumberGeneratorDataSource>
{
    using namespace ::DFG_MODULE_NS(math);
    if (!isFinite(first) || !isFinite(last) || !isFinite(step) || signBit(last - first) != signBit(step) || (last != first && step == 0))
        return nullptr;
    const double doubleStepCount = (last - first) / step;
    const double doubleStepCountFloored = std::floor(doubleStepCount);
    DataSourceIndex nCount = 0;
    if (!isFloatConvertibleTo(doubleStepCountFloored, &nCount))
        return nullptr;
    ++nCount; // Adding first
    return std::unique_ptr<NumberGeneratorDataSource>(new NumberGeneratorDataSource(std::move(sId), nCount, first, step, defaultTransferBlockSize()));
}

auto ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::columnCount() const -> DataSourceIndex
{
    return 1;
}

auto ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::defaultTransferBlockSize() -> DataSourceIndex
{
    return ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::SourceSpanBuffer::contentBlockSize();
}

auto ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::maxRowCount() -> DataSourceIndex
{
    return (std::numeric_limits<DataSourceIndex>::max)() - 1;
}

auto ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::rowCount() const -> DataSourceIndex
{
    auto p = DFG_OPAQUE_PTR();
    return (p) ? p->m_nRowCount : 0;
}

auto ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::columnIndexes() const -> IndexList
{
    return {0};
}

bool ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::enable(bool b)
{
    return b; // This source does not emit anything or have internal data structures to update, so enabling/disabling does not affect it; might as well tell the caller that requested mode is used.
}

auto ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::columnNames() const -> ColumnNameMap
{
    ColumnNameMap names;
    for (DataSourceIndex i = 0, nCount = columnCount(); i < nCount; ++i)
    {
        names.insert(i, String());
    }
    return names;
}

auto ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::columnDataTypes() const -> ColumnDataTypeMap
{
    ColumnDataTypeMap types;
    for (DataSourceIndex i = 0, nCount = columnCount(); i < nCount; ++i)
    {
        types.insert(i, ChartDataType::unknown);
    }
    return types;
}

auto ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::columnDataType(const DataSourceIndex) const -> ChartDataType
{
    return ChartDataType::unknown;
}

void ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::forEachElement_byColumn(const DataSourceIndex nCol, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler)
{
    if (!handler || nCol >= this->columnCount() || !queryDetails.areNumbersRequested())
        return;

    const auto nTotalRowCount = this->rowCount();
    const auto nTransferBlockSize = (DFG_OPAQUE_PTR()) ? DFG_OPAQUE_PTR()->m_nTransferBlockSize : defaultTransferBlockSize();

    const auto first = DFG_OPAQUE_REF().m_first;
    const auto step = DFG_OPAQUE_REF().m_step;

    std::vector<double> buffer(Min(nTotalRowCount, nTransferBlockSize));
    for (DataSourceIndex i = 0; i < nTotalRowCount; i = saturateAdd<decltype(i)>(i, buffer.size()))
    {
        const auto nBufferSize = Min(nTotalRowCount - i, buffer.size());
        auto effectiveRange = makeRange(buffer.begin(), buffer.begin() + nBufferSize);
        ::DFG_MODULE_NS(alg)::generateAdjacent(effectiveRange, first + i * step, step);
        SourceDataSpan dataBlob;
        dataBlob.set(effectiveRange);
        handler(dataBlob);
    }
}

void ::DFG_MODULE_NS(qt)::NumberGeneratorDataSource::fetchColumnNumberData(GraphDataSourceDataPipe&& pipe, const DataSourceIndex nCol, const DataQueryDetails& queryDetails)
{
    if (nCol >= this->columnCount() || !queryDetails.areNumbersRequested())
        return;

    const auto nTotalRowCount = this->rowCount();

    double* pRows = nullptr;
    double* pValues = nullptr;
    pipe.getFillBuffers(nTotalRowCount, (queryDetails.areRowsRequested()) ? &pRows : nullptr, &pValues);
    if (pValues == nullptr)
        return; // Unable to allocate storage.

    const auto first = DFG_OPAQUE_REF().m_first;
    const auto step = DFG_OPAQUE_REF().m_step;

    if (pRows)
        ::DFG_MODULE_NS(alg)::generateAdjacent(makeRange(pRows, pRows + nTotalRowCount), 0, 1);
    ::DFG_MODULE_NS(alg)::generateAdjacent(makeRange(pValues, pValues + nTotalRowCount), first, step);
}
