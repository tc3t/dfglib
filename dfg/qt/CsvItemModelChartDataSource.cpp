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
    setChangeSignaling(true);
    
    this->m_bAreChangesSignaled = true;
}

::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::~CsvItemModelChartDataSource()
{

}

void ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::setChangeSignaling(const bool bEnable)
{
    if (bEnable)
    {
        DFG_QT_VERIFY_CONNECT(connect(m_spModel.data(), &CsvItemModel::dataChanged, this, &GraphDataSource::sigChanged));
        DFG_QT_VERIFY_CONNECT(connect(m_spModel.data(), &CsvItemModel::modelReset, this, &GraphDataSource::sigChanged));
    }
    else
    {
        DFG_VERIFY(disconnect(m_spModel.data(), &CsvItemModel::dataChanged, this, &GraphDataSource::sigChanged));
        DFG_VERIFY(disconnect(m_spModel.data(), &CsvItemModel::modelReset, this, &GraphDataSource::sigChanged));
    }
}

QObject* ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::underlyingSource()
{
    return nullptr; // modifiable ptr is not available
}

void ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::forEachElement_byColumn(const DataSourceIndex c, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler)
{
    auto pTable = (handler) ? privGetDataTable() : nullptr;
    if (!pTable)
        return;

    const auto& rTable = *pTable;

    // Peeking at the data to determine type of current column.
    bool bFound = false;
    auto whileFunc = [&](const int) { return !bFound; };
    rTable.forEachFwdRowInColumnWhile(static_cast<int>(c), whileFunc, [&](const int /*r*/, const SzPtrUtf8R psz)
    {
        const auto val = GraphDataSource::cellStringToDouble(psz, c, m_columnTypes);
        if (!::DFG_MODULE_NS(math)::isNan(val))
            bFound = true;
        // Simply taking column type from first recognized type.
    });

    const auto colType = m_columnTypes[c];

    using ValueVector = ::DFG_MODULE_NS(cont)::ValueVector<double>;
    using StringViewVector = ::DFG_MODULE_NS(cont)::Vector<StringViewUtf8>;
    ValueVector rows;
    ValueVector vals;
    StringViewVector stringViews;
    const auto nBlockSize = static_cast<size_t>(Min(1024, rTable.rowCountByMaxRowIndex()));
    if (queryDetails.areRowsRequested())
        rows.reserve(nBlockSize);
    if (queryDetails.areNumbersRequested())
        vals.reserve(nBlockSize);
    if (queryDetails.areStringsRequested())
        stringViews.reserve(nBlockSize);

    const auto callHandler = [&]()
    {
        SourceDataSpan sourceData;
        if (!rows.empty())
            sourceData.setRows(rows);
        if (!vals.empty())
            sourceData.set(vals);
        if (!stringViews.empty())
            sourceData.set(stringViews);
        handler(sourceData);
    };

    // Calling given handler for each row in column but instead of doing it for each cell individually, gathering blocks and passing blocks to handler (performance optimization).
    rTable.forEachFwdRowInColumn(static_cast<int>(c), [&](const int r, const SzPtrUtf8R psz)
    {
        if (rows.size() >= nBlockSize || vals.size() >= nBlockSize || stringViews.size() >= nBlockSize)
        {
            callHandler();
            rows.clear();
            vals.clear();
            stringViews.clear();
        }
        if (queryDetails.areRowsRequested())
            rows.push_back(static_cast<double>(r));
        if (queryDetails.areNumbersRequested())
        {
            if (colType == ChartDataType::unknown && std::strchr(psz.c_str(), ',') == nullptr)
                vals.push_back(::DFG_MODULE_NS(str)::strTo<double>(psz));
            else
                vals.push_back(cellStringToDouble(psz, c, m_columnTypes));
        }
        if (queryDetails.areStringsRequested())
            stringViews.push_back(psz);
    });
    if (!rows.empty() || !vals.empty() || !stringViews.empty())
        callHandler();
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
    auto rv = std::make_shared<DoubleValueVector>();
    auto& outputVec = *rv;

    forEachElement_byColumn(nColIndex, DataQueryDetails(DataQueryDetails::DataMaskNumerics), [&](const SourceDataSpan& sourceData)
    {
        const auto doubleRange = sourceData.doubles();
        outputVec.insert(outputVec.end(), doubleRange.cbegin(), doubleRange.cend());
    });
    return std::move(rv); // explicit move to avoid "call 'std::move' explicitly to avoid copying on older compilers"-warning in Qt Creator
}

void ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::enable(const bool b)
{
    setChangeSignaling(b);
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
