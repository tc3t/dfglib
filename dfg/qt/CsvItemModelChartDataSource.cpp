#include "CsvItemModelChartDataSource.hpp"
#include "connectHelper.hpp"
#include "../cont/valueArray.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS

DFG_END_INCLUDE_QT_HEADERS

#include "../alg.hpp"

::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::CsvItemModelChartDataSource(CsvItemModel* pModel, QString sId)
    : m_spModel(pModel)
{
    this->m_uniqueId = std::move(sId);
    if (!m_spModel)
        return;
    DFG_QT_VERIFY_CONNECT(connect(m_spModel.data(), &CsvItemModel::dataChanged, this, &GraphDataSource::sigChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spModel.data(), &CsvItemModel::modelReset, this, &GraphDataSource::sigChanged));
    this->m_bAreChangesSignaled = true;
}

::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::~CsvItemModelChartDataSource()
{

}

QObject* ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::underlyingSource()
{
    return nullptr; // modifiable ptr is not available
}

void ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::forEachElement_byColumn(const DataSourceIndex c, ForEachElementByColumHandler handler)
{
    auto pTable = (handler) ? privGetDataTable() : nullptr;
    if (!pTable)
        return;

    const auto& rTable = *pTable;

    bool bFound = false;
    auto whileFunc = [&](const int) { return !bFound; };
    rTable.forEachFwdRowInColumnWhile(static_cast<int>(c), whileFunc, [&](const int /*r*/, const SzPtrUtf8R psz)
    {
        const auto val = GraphDataSource::cellStringToDouble(QString::fromUtf8(psz.c_str()), c, m_columnTypes);
        if (!::DFG_MODULE_NS(math)::isNan(val))
            bFound = true;
        // Simply taking column type from first recognized type.
    });

    const auto colType = m_columnTypes[c];

    using ValueVector = ::DFG_MODULE_NS(cont)::ValueVector<double>;
    ValueVector rows;
    ValueVector vals;
    const auto nBlockSize = Min(1024, rTable.rowCountByMaxRowIndex());
    rows.reserve(nBlockSize);
    vals.reserve(nBlockSize);
    rTable.forEachFwdRowInColumn(static_cast<int>(c), [&](const int r, const SzPtrUtf8R psz)
    {
        if (vals.size() >= nBlockSize)
        {
            handler(rows.data(), vals.data(), nullptr, vals.size());
            rows.clear();
            vals.clear();
        }
        rows.push_back(static_cast<double>(r));
        if (colType == ChartDataType::unknown && std::strchr(psz.c_str(), ',') == nullptr)
            vals.push_back(::DFG_MODULE_NS(str)::strTo<double>(psz));
        else
            vals.push_back(cellStringToDouble(QString::fromUtf8(psz.c_str()), c, m_columnTypes));
    });
    if (!vals.empty())
        handler(rows.data(), vals.data(), nullptr, vals.size());
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnCount() const -> DataSourceIndex
{
    auto pModel = privGetCsvModel();
    return (pModel) ? static_cast<DataSourceIndex>(pModel->columnCount()) : 0;
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnIndexes() const -> IndexList
{
    auto pModel = privGetCsvModel();
    if (pModel)
    {
        IndexList indexList(pModel->columnCount());
        ::DFG_MODULE_NS(alg)::generateAdjacent(indexList, 0, 1);
        return indexList;
    }
    else
        return IndexList();
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnIndexByName(const StringViewUtf8 sv) const -> DataSourceIndex
{
    auto rv = m_spModel->findColumnIndexByName(viewToQString(sv), -1);
    return (rv != -1) ? static_cast<DataSourceIndex>(rv) : invalidIndex();
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::singleColumnDoubleValues_byOffsetFromFirst(const DataSourceIndex offsetFromFirst) -> SingleColumnDoubleValuesOptional
{
    return singleColumnDoubleValues_byColumnIndex(offsetFromFirst);
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::singleColumnDoubleValues_byColumnIndex(const DataSourceIndex nColIndex) -> SingleColumnDoubleValuesOptional
{
    auto pCsvTable = privGetDataTable();
    if (!pCsvTable)
        return SingleColumnDoubleValuesOptional();
    auto rv = std::make_shared<DoubleValueVector>();
    auto& outputVec = *rv;

    forEachElement_byColumn(nColIndex, [&](const double* /*pRows*/, const double* pVals, const QVariant*, DataSourceIndex nArrSize)
    {
        outputVec.insert(outputVec.end(), pVals, pVals + nArrSize);
    });
    return std::move(rv); // explicit move to avoid "call 'std::move' explicitly to avoid copying on older compilers"-warning in Qt Creator
}

void ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::enable(const bool /*b*/)
{
    // Not supported: there's no hidden activity (e.g data fetching) so no need to enable/disable anything.
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnDataTypes() const -> ColumnDataTypeMap
{
    return m_columnTypes;
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnNames() const -> ColumnNameMap
{
    ColumnNameMap rv;
    auto pModel = privGetCsvModel();
    if (!pModel)
        return rv;
    for (int i = 0, nColCount = pModel->columnCount(); i < nColCount; ++i)
        rv[DataSourceIndex(i)] = pModel->getHeaderName(i);
    return rv;
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::privGetCsvModel() const -> const CsvItemModel*
{
    return m_spModel.data();
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::privGetDataTable() const -> const CsvItemModel::DataTable*
{
    auto pModel = privGetCsvModel();
    return (pModel) ? &pModel->m_table : nullptr;
}
