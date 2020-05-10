#include "CsvItemModelChartDataSource.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QVariant>
DFG_END_INCLUDE_QT_HEADERS

::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::CsvItemModelChartDataSource(CsvItemModel* pModel, QString sId)
    : m_spModel(pModel)
{
    this->m_uniqueId = std::move(sId);
    if (!m_spModel)
        return;
}

::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::~CsvItemModelChartDataSource()
{

}

QObject* ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::underlyingSource()
{
    return nullptr; // modifiable ptr is not available
}

void ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::forEachElement_fromTableSelection(std::function<void(DataSourceIndex, DataSourceIndex, QVariant)> handler)
{
    auto pTable = (handler) ? privGetDataTable() : nullptr;
    if (!pTable)
        return;

    const auto& rTable = *pTable;

    rTable.forEachFwdColumnIndex([&](const int c)
    {
        rTable.forEachFwdRowInColumn(c, [&](const int r, const SzPtrUtf8R psz)
        {
            // TODO: makes redundant QString (inside QVariant) on each call.
            handler(static_cast<DataSourceIndex>(r), static_cast<DataSourceIndex>(c), psz.rawPtr());
        });
    });
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnCount() const -> DataSourceIndex
{
    auto pModel = privGetCsvModel();
    return (pModel) ? pModel->columnCount() : 0;
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnIndexByName(const StringViewUtf8 sv) const -> DataSourceIndex
{
    auto rv = m_spModel->findColumnIndexByName(viewToQString(sv), -1);
    return (rv != -1) ? rv : invalidIndex();
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
    pCsvTable->forEachFwdRowInColumn(static_cast<int>(nColIndex), [&](const int /*r*/, const SzPtrUtf8R psz)
    {
        outputVec.push_back(::DFG_MODULE_NS(str)::strTo<double>(psz));
    });
    return rv;
}

void ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::enable(const bool /*b*/)
{
    // Not supported: there's no hidden activity (e.g data fetching) so no need to enable/disable anything.
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnDataTypes() const -> ColumnDataTypeMap
{
    // TODO
    return ColumnDataTypeMap();
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
