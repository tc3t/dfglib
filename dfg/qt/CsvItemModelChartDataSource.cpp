#include "CsvItemModelChartDataSource.hpp"
#include "connectHelper.hpp"
#include "../cont/valueArray.hpp"
#include "detail//CsvItemModel/chartDoubleAccesser.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS

DFG_END_INCLUDE_QT_HEADERS

#include "../alg.hpp"

DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(qt)::CsvItemModelChartDataSource)
{
    std::atomic<uint64> m_anChangeCounter{ 0 };
};

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
    const auto changeHandler = &CsvItemModelChartDataSource::onModelChanged;
    if (bEnable)
    {
        DFG_QT_VERIFY_CONNECT(connect(m_spModel.data(), &CsvItemModel::dataChanged, this, changeHandler));
        DFG_QT_VERIFY_CONNECT(connect(m_spModel.data(), &CsvItemModel::headerDataChanged, this, changeHandler));
        DFG_QT_VERIFY_CONNECT(connect(m_spModel.data(), &CsvItemModel::modelReset, this, changeHandler));
        DFG_QT_VERIFY_CONNECT(connect(m_spModel.data(), &CsvItemModel::sigColumnNumberDataInterpretationChanged, this, changeHandler));
    }
    else
    {
        DFG_VERIFY(disconnect(m_spModel.data(), &CsvItemModel::dataChanged, this, changeHandler));
        DFG_VERIFY(disconnect(m_spModel.data(), &CsvItemModel::headerDataChanged, this, changeHandler));
        DFG_VERIFY(disconnect(m_spModel.data(), &CsvItemModel::modelReset, this, changeHandler));
        DFG_VERIFY(disconnect(m_spModel.data(), &CsvItemModel::sigColumnNumberDataInterpretationChanged, this, changeHandler));
    }
}

QObject* ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::underlyingSource()
{
    return nullptr; // modifiable ptr is not available
}

void ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::forEachElement_byColumn(const DataSourceIndex c, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler)
{
    if (!handler)
        return;

    auto [pCsvModel, lockReleaser] = privGetCsvModel();

    if (!pCsvModel)
        return;

    const auto& rCsvModel = *pCsvModel;
    const auto& rTable = pCsvModel->m_table;
    using CellIndex = CsvItemModel::Index;
    const auto nColCellIndex = static_cast<CellIndex>(c);

    using ModelDoubleAccesser = DFG_DETAIL_NS::CsvItemModelChartDoubleAccesser;

    // Peeking at the data to determine type of current column.
    {
        bool bFound = false;
        m_columnTypes.erase(c); // Erasing old type to make sure that it won't break detection.
        auto whileFunc = [&](const int) { return !bFound; };
        rTable.forEachFwdRowInColumnWhile(nColCellIndex, whileFunc, [&](const CellIndex r, const SzPtrUtf8R psz)
            {
                const auto val = ModelDoubleAccesser(r, nColCellIndex, rCsvModel, m_columnTypes, c)(psz);
                if (!::DFG_MODULE_NS(math)::isNan(val))
                    bFound = true;
                // Simply taking column type from first recognized type.
            });
    }

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
    const auto nRowCount = rTable.rowCountByMaxRowIndex();
    const auto colAsInt = saturateCast<int>(c);
    for (int r = 0; r < nRowCount; ++r) // Note: not using rTable.forEachFwdRowInColumn() since it skips null-cells
    {
        auto psz = rTable(r, colAsInt);
        if (psz == nullptr)
            psz = DFG_UTF8("");
        if (rows.size() >= nBlockSize || vals.size() >= nBlockSize || stringViews.size() >= nBlockSize)
        {
            callHandler();
            rows.clear();
            vals.clear();
            stringViews.clear();
        }
        if (queryDetails.areRowsRequested())
            rows.push_back(static_cast<double>(CsvItemModel::internalRowIndexToVisible(r)));
        if (queryDetails.areNumbersRequested())
        {
            // When colType is unknown (double) and it has no comma, parsing string directly as double...
            if (colType == ChartDataType::unknown && std::strchr(psz.c_str(), ',') == nullptr)
                vals.push_back(stringToDouble(StringViewSzC(psz.c_str())));
            else // ...otherwise getting it through model access with datetime etc. parsing capabilities.
                vals.push_back(ModelDoubleAccesser(r, nColCellIndex, rCsvModel, m_columnTypes, c)(psz));
        }
        if (queryDetails.areStringsRequested())
            stringViews.push_back(psz);
    }
    if (!rows.empty() || !vals.empty() || !stringViews.empty())
        callHandler();
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnCount() const -> DataSourceIndex
{
    auto modelPtrAndLockReleaser = privGetCsvModel();
    return (modelPtrAndLockReleaser.first) ? static_cast<DataSourceIndex>(modelPtrAndLockReleaser.first->columnCount()) : 0;
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnIndexes() const -> IndexList
{
    auto modelPtrAndLockReleaser = privGetCsvModel();
    if (modelPtrAndLockReleaser.first)
    {
        IndexList indexList(modelPtrAndLockReleaser.first->columnCount());
        ::DFG_MODULE_NS(alg)::generateAdjacent(indexList, 0, 1);
        return indexList;
    }
    else
        return IndexList();
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnIndexByName(const StringViewUtf8 sv) const -> DataSourceIndex
{
    auto modelPtrAndLockReleaser = privGetCsvModel();
    const auto rv =  (modelPtrAndLockReleaser.first) ? modelPtrAndLockReleaser.first->findColumnIndexByName(viewToQString(sv), -1) : -1;
    return (rv != -1) ? static_cast<DataSourceIndex>(rv) : invalidIndex();
}

bool ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::enable(const bool b)
{
    setChangeSignaling(b);
    return b;
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnDataTypes() const -> ColumnDataTypeMap
{
    return m_columnTypes;
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::columnNames() const -> ColumnNameMap
{
    ColumnNameMap rv;
    auto modelPtrAndLockReleaser = privGetCsvModel();
    if (!modelPtrAndLockReleaser.first)
        return rv;
    auto& rModel = *modelPtrAndLockReleaser.first;
    for (int i = 0, nColCount = rModel.columnCount(); i < nColCount; ++i)
        rv[DataSourceIndex(i)] = rModel.getHeaderName(i);
    return rv;
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::privGetCsvModel() const -> std::pair<const CsvItemModel*, LockReleaser>
{
    auto pModel = m_spModel.data();
    auto lockReleaser = (pModel) ? pModel->tryLockForRead() : LockReleaser();
    return std::pair<const CsvItemModel*, LockReleaser>(lockReleaser.isLocked() ? pModel : nullptr, std::move(lockReleaser));
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::privGetDataTable() const -> std::pair<const CsvItemModel::DataTable*, LockReleaser>
{
    auto modelLockReleaserPair = privGetCsvModel();
    const CsvItemModel::DataTable* pTable = (modelLockReleaserPair.first) ? &modelLockReleaserPair.first->m_table : nullptr;
    return std::pair<const CsvItemModel::DataTable*, LockReleaser>(pTable, std::move(modelLockReleaserPair.second));
}

bool ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::isSafeToQueryDataFromThreadImpl(const QThread*) const
{
    return true;
}

auto ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::snapshotIdImpl() const -> std::optional<SnapshotId>
{
    return (DFG_OPAQUE_PTR()) ? SnapshotId(DFG_OPAQUE_PTR()->m_anChangeCounter) : std::optional<SnapshotId>(std::nullopt);
}

void ::DFG_MODULE_NS(qt)::CsvItemModelChartDataSource::onModelChanged()
{
    DFG_OPAQUE_REF().m_anChangeCounter++;
    Q_EMIT sigChanged();
}
