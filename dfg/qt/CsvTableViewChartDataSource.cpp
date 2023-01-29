#include "CsvTableViewChartDataSource.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QDateTime>
DFG_END_INCLUDE_QT_HEADERS

#include "connectHelper.hpp"
#include "CsvItemModel.hpp"
#include "../cont/ViewableSharedPtr.hpp"
#include "../cont/MapVector.hpp"

#include "../alg.hpp"
#include "../cont/valueArray.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

/////////////////////////////////////////////////////////////////////////////////////
// 
// CsvTableViewChartDataSource::DataSourceNumberCache
//
/////////////////////////////////////////////////////////////////////////////////////

class CsvTableViewChartDataSource::DataSourceNumberCache
{
public:
    using Index = CsvItemModel::Index;
    using NumberContT = ::DFG_MODULE_NS(cont)::ValueVector<double>;
    template <class T> using SharedStorage = std::shared_ptr<T>;

    bool hasColumn(const ColumnIndex_data col) const;

    // Returns new cache object for single column with reserved memory for requested elements, or null-storage if unable to create (e.g. lack of memory)
    SharedStorage<NumberContT> makeNewColumnCacheObject(const Index nReserveCount);

    void setColumnCache(const ColumnIndex_data col, SharedStorage<const NumberContT> newData, ChartDataType dataType);

    Span<const double> getSpanFromColumn(const ColumnIndex_data col, Index nFirstRow, Index nLastRow) const;

    // Returns whole column span
    Span<const double> getSpanFromColumn(const ColumnIndex_data col) const;

    ChartDataType getColumnDataType(const ColumnIndex_data col) const;
        
    ::DFG_MODULE_NS(cont)::MapVectorSoA<Index, SharedStorage<const NumberContT>> m_mapColToNumbers;
    GraphDataSource::ColumnDataTypeMap m_mapColToDataTypes;
}; // class CsvTableViewChartDataSource::DataSourceNumberCache


bool CsvTableViewChartDataSource::DataSourceNumberCache::hasColumn(const ColumnIndex_data col) const
{
    const auto iter = m_mapColToNumbers.find(col.value());
    return (iter != m_mapColToNumbers.end() && iter->second && !iter->second->empty());
}

// Returns new cache object for single column with reserved memory for requested element count, or null-storage if unable to create (e.g. lack of memory)
auto CsvTableViewChartDataSource::DataSourceNumberCache::makeNewColumnCacheObject(const Index nReserveCount) -> SharedStorage<NumberContT>
{
    try
    {
        SharedStorage<NumberContT> spNewData(new NumberContT);
        spNewData->reserve(static_cast<size_t>(nReserveCount));
        return spNewData;
    }
    catch (...)
    {
        return nullptr;
    }
}

void CsvTableViewChartDataSource::DataSourceNumberCache::setColumnCache(const ColumnIndex_data col, SharedStorage<const NumberContT> newData, const ChartDataType dataType)
{
    m_mapColToNumbers[col.value()] = std::move(newData);
    m_mapColToDataTypes[col.value()] = dataType;
}

auto CsvTableViewChartDataSource::DataSourceNumberCache::getColumnDataType(const ColumnIndex_data col) const -> ChartDataType
{
    return m_mapColToDataTypes.valueCopyOr(col.value(), ChartDataType::unknown);
}

Span<const double> CsvTableViewChartDataSource::DataSourceNumberCache::getSpanFromColumn(const ColumnIndex_data col, const Index nFirstRow, const Index nLastRow) const
{
    auto iter = this->m_mapColToNumbers.find(col.value());
    return (iter != m_mapColToNumbers.end() && iter->second) ? Span<const double>(makeRangeFromStartAndEndIndex(*iter->second, nFirstRow, nLastRow + 1)) : Span<const double>();
}

// Returns whole column span
Span<const double> CsvTableViewChartDataSource::DataSourceNumberCache::getSpanFromColumn(const ColumnIndex_data col) const
{
    auto iter = this->m_mapColToNumbers.find(col.value());
    if (iter != this->m_mapColToNumbers.end() && iter->second && !iter->second->empty())
        return getSpanFromColumn(col, 0, saturateCast<Index>(iter->second->size() - 1));
    else
        return Span<const double>();
}


} } // namespace qt


/////////////////////////////////////////////////////////////////////////////////////
// 
// SelectionAnalyzerForGraphing
//
/////////////////////////////////////////////////////////////////////////////////////


void ::DFG_MODULE_NS(qt)::SelectionAnalyzerForGraphing::setChartDefinitionViewer(ChartDefinitionViewer viewer)
{
    if (!m_chartDefinitionViewer.isNull())
    {
        DFG_ASSERT_INVALID_ARGUMENT(false, "Viewer can be set only once"); // For now not supporting resetting viewer.
        return;
    }
    m_chartDefinitionViewer = std::move(viewer);
    m_apChartDefinitionViewer = &m_chartDefinitionViewer;
}

void ::DFG_MODULE_NS(qt)::SelectionAnalyzerForGraphing::analyzeImpl(const QItemSelection selection)
{
    auto pView = qobject_cast<CsvTableView*>(this->m_spView.data());
    if (!pView || !m_spSelectionInfo)
        return;

    // Note: there's no CsvTableView read-locking here since parent class handles it.

    ChartDefinitionViewer* pViewer = m_apChartDefinitionViewer;
    decltype(pViewer->view()) spChartDefinitionView;
    if (pViewer)
        spChartDefinitionView = pViewer->view();

    bool bSignalChange = true;



    m_spSelectionInfo.edit([&](SelectionInfo& rSelectionInfo, const SelectionInfo* pOld)
    {
        DFG_UNUSED(pOld);

        rSelectionInfo.m_columnNames.clear();

        bool bErrorEntries = false;
        if (spChartDefinitionView && !spChartDefinitionView->isSourceUsed(m_sSourceId, &bErrorEntries))
        {
            rSelectionInfo.clear();
            if (!bErrorEntries) // If there are no error entries, considering this source as definitely not used so not signaling changes.
                bSignalChange = false;
            return;
        }

        static_cast<QItemSelection&>(rSelectionInfo) = selection;

        // Getting column names from columns present in selection.
        {
            auto pModel = pView->csvModel();
            if (pModel)
            {
                for (const auto& item : rSelectionInfo)
                {
                    for (auto i = item.left(), right = item.right(); i <= right; ++i)
                        rSelectionInfo.m_columnNames[static_cast<DataSourceIndex>(i)] = pModel->getHeaderName(pView->columnIndexViewToData(ColumnIndex_view(i)).value());
                }
            }
        }
    });

    if (bSignalChange)
        Q_EMIT sigAnalyzeCompleted();
}

/////////////////////////////////////////////////////////////////////////////////////
// 
// CsvTableViewChartDataSource
//
/////////////////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource)
{
    ::DFG_MODULE_NS(cont)::ViewableSharedPtr<DataSourceNumberCache> m_cache;
    ::DFG_MODULE_NS(cont)::ViewableSharedPtrViewer<DataSourceNumberCache> m_cacheViewer;

    ::DFG_MODULE_NS(cont)::ViewableSharedPtr<GraphDataSource::ColumnDataTypeMap> m_viewableMapColToDataType;
    ::DFG_MODULE_NS(cont)::ViewableSharedPtrViewer<GraphDataSource::ColumnDataTypeMap> m_viewerMapColToDataType;
    bool m_bCachingAllowed = true;
};

::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::CsvTableViewChartDataSource(CsvTableView* view)
    : m_spView(view)
{
    if (!m_spView)
        return;
    m_uniqueId = QString("viewSelection_%1").arg(QString::number(QDateTime::currentMSecsSinceEpoch() % 1000000));
    m_spSelectionAnalyzer = std::make_shared<SelectionAnalyzerForGraphing>();
    m_spSelectionAnalyzer->m_spSelectionInfo.reset(std::make_shared<SelectionAnalyzerForGraphing::SelectionInfo>());
    m_spSelectionAnalyzer->m_sSourceId = this->uniqueId();
    m_selectionViewer = m_spSelectionAnalyzer->m_spSelectionInfo.createViewer();
    DFG_QT_VERIFY_CONNECT(connect(m_spSelectionAnalyzer.get(), &CsvTableViewSelectionAnalyzer::sigAnalyzeCompleted, this, &CsvTableViewChartDataSource::onSelectionAnalysisCompleted));
    this->m_bAreChangesSignaled = true;
    m_spView->addSelectionAnalyzer(m_spSelectionAnalyzer);

    DFG_OPAQUE_REF().m_cacheViewer = DFG_OPAQUE_REF().m_cache.createViewer();
    DFG_OPAQUE_REF().m_viewerMapColToDataType = DFG_OPAQUE_REF().m_viewableMapColToDataType.createViewer();

    auto pCsvModel = m_spView->csvModel();
    if (pCsvModel)
    {
        // Simply reset cache on any changes to CsvModel
        DFG_QT_VERIFY_CONNECT(connect(pCsvModel, &CsvItemModel::dataChanged, this, &CsvTableViewChartDataSource::resetCache));
        DFG_QT_VERIFY_CONNECT(connect(pCsvModel, &CsvItemModel::sigOnNewSourceOpened, this, &CsvTableViewChartDataSource::resetCache));
        DFG_QT_VERIFY_CONNECT(connect(pCsvModel, &CsvItemModel::columnsInserted, this, &CsvTableViewChartDataSource::resetCache));
        DFG_QT_VERIFY_CONNECT(connect(pCsvModel, &CsvItemModel::columnsRemoved, this, &CsvTableViewChartDataSource::resetCache));
        DFG_QT_VERIFY_CONNECT(connect(pCsvModel, &CsvItemModel::columnsMoved, this, &CsvTableViewChartDataSource::resetCache));
        DFG_QT_VERIFY_CONNECT(connect(pCsvModel, &CsvItemModel::rowsInserted, this, &CsvTableViewChartDataSource::resetCache));
        DFG_QT_VERIFY_CONNECT(connect(pCsvModel, &CsvItemModel::rowsRemoved, this, &CsvTableViewChartDataSource::resetCache));
        DFG_QT_VERIFY_CONNECT(connect(pCsvModel, &CsvItemModel::rowsMoved, this, &CsvTableViewChartDataSource::resetCache));
    }
}

::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::~CsvTableViewChartDataSource()
{

}

QObject* ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::underlyingSource()
{
    return m_spView;
}

namespace
{
    static double cellStringToDoubleImpl(::DFG_ROOT_NS::SzPtrUtf8R pszData, const ::DFG_MODULE_NS(qt)::ColumnIndex_view nColView, ::DFG_MODULE_NS(qt)::GraphDataSource::ColumnDataTypeMap& rColumnTypes)
    {
        return (pszData) ? ::DFG_MODULE_NS(qt)::GraphDataSource::cellStringToDouble(pszData, nColView.value(), &rColumnTypes) : std::numeric_limits<double>::quiet_NaN();
    }
}

void ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::fetchColumnNumberData(GraphDataSourceDataPipe& pipe, const DataSourceIndex nColumn, const DataQueryDetails& queryDetails)
{
    BaseClass::fetchColumnNumberData(pipe, nColumn, queryDetails);
}

void ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::setCachingAllowed(const bool bAllowed)
{
    DFG_OPAQUE_REF().m_bCachingAllowed = bAllowed;
}

bool ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::isCachingAllowed() const
{
    return DFG_OPAQUE_PTR() && DFG_OPAQUE_PTR()->m_bCachingAllowed;
}

void ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::forEachElement_byColumn(const DataSourceIndex nColRaw, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler)
{
    if (!handler || !m_spView)
        return;

    auto pCsvModel = m_spView->csvModel();
    auto pModel = m_spView->model();
    if (!pCsvModel || !pModel)
        return;

    auto spSelectionViewer = this->privGetSelectionViewer();
    if (!spSelectionViewer) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
        return;

    auto readLockReleaser = m_spView->tryLockForRead();
    if (!readLockReleaser.isLocked())
        return;

    auto cacheViewer = DFG_OPAQUE_REF().m_cache.createViewer();
    auto spCacheView = cacheViewer.view();

    const QItemSelection& selection  = *spSelectionViewer;

    const ColumnIndex_view nColView(saturateCast<CsvTableView::Index>(nColRaw));
    const auto nColData = m_spView->columnIndexViewToData(nColView);

    // Checking if string data should be cached as double-data. Note that caching gets done even if row index mapping is needed because in that case
    // cache still makes it possible to omit string-to-double conversion and instead use the already-converted double from the cache.
    if (this->isCachingAllowed() && queryDetails.areNumbersRequested() && (!spCacheView || !spCacheView->hasColumn(nColData)))
    {
        // Constructing caches only if selection has single rectangle and it covers over 90 % of rows in the whole table.
        // This is somewhat arbitrary condition - the point is to avoid possible costly caching of millions of rows if user selects just one cell.
        const auto nTotalRowCount = pCsvModel->rowCount();
        if (selection.size() == 1 && double(selection.front().height()) / double(pCsvModel->rowCount()) > 0.9)
        {
            std::unique_ptr<DataSourceNumberCache> newCache(new DataSourceNumberCache(spCacheView ? *spCacheView : DataSourceNumberCache()));
            auto spColumnCache = newCache->makeNewColumnCacheObject(nTotalRowCount);
            if (spColumnCache)
            {
                GraphDataSource::ColumnDataTypeMap mapColToDataType;
                for (CsvItemModel::Index r = 0; r < nTotalRowCount; ++r)
                {
                    const auto pszData = pCsvModel->rawStringPtrAt(r, nColData.value());
                    const auto val = cellStringToDoubleImpl(pszData, nColView, mapColToDataType);
                    spColumnCache->push_back(val);
                }
                newCache->setColumnCache(nColData, std::move(spColumnCache), mapColToDataType.valueCopyOr(nColView.value(), ChartDataType::unknown));
                DFG_OPAQUE_REF().m_cache.reset(std::move(newCache));
                spCacheView = cacheViewer.view();
            }
        }
    }

    const bool bIsRowIndexMappingNeeded = m_spView->isRowIndexMappingNeeded();
    const bool bNumberCacheIsAvailable = (spCacheView && spCacheView->hasColumn(nColData));
    const auto hasSelectionRangeRequestedColumn = [&](const QItemSelectionRange& sr) { return nColView.value() >= sr.left() && nColView.value() <= sr.right(); };

    const auto iterSelectionRangeEffectiveEnd = [&]()
        {
            auto iterEffectiveEnd = selection.begin();
            for (auto iter = selection.cbegin(); iter != selection.cend(); ++iter)
            {
                if (hasSelectionRangeRequestedColumn(*iter))
                    iterEffectiveEnd = iter + 1;
            }
            return iterEffectiveEnd;

        }();

    const auto insertColumnMetaData = [&](SourceDataSpan& targetSpan, const bool bEfficientFetchable, const ChartDataType dataType)
        {
            ColumnMetaData metaData;
            metaData.efficientlyFetchable(bEfficientFetchable);
            metaData.name(spSelectionViewer->m_columnNames.valueCopyOr(nColView.value()));
            metaData.columnDataType(dataType);
            targetSpan.metaData() = std::move(metaData);
        };

    for (auto iterSelection = selection.cbegin(); iterSelection != iterSelectionRangeEffectiveEnd; ++iterSelection)
    {
        const auto& item = *iterSelection;
        if (!hasSelectionRangeRequestedColumn(item))
            continue; // Skipping selection item if it does not have requested column.

        // Checking if can use cached number directly, requires:
        //      -Having cached numbers available
        //      -View's proxy model having the same row indexes as underlying model.
        //      -Request needed only rows or numbers (strings are not cached).
        if (bNumberCacheIsAvailable && !bIsRowIndexMappingNeeded && queryDetails.areOnlyRowsOrNumbersRequested())
        {
            SourceDataSpan dataSpan;
            const auto nFirstRow = item.top();
            const auto nLastRow = item.bottom();
            const auto nRowCount = ::DFG_MODULE_NS(math)::numericDistance(nFirstRow, nLastRow) + 1u;
            if (queryDetails.areRowsRequested())
                dataSpan.setRowsAsIndexSequenceByCountFirst(nRowCount, CsvItemModel::internalRowIndexToVisible(nFirstRow));
            if (queryDetails.areNumbersRequested())
                dataSpan.set(spCacheView->getSpanFromColumn(nColData, nFirstRow, nLastRow));
            // If current selection range is the last one, filling column meta data.
            if (iterSelection + 1 == iterSelectionRangeEffectiveEnd)
                insertColumnMetaData(dataSpan, true, spCacheView->getColumnDataType(nColData));
            handler(dataSpan);
            continue;
        }

        const auto cachedColumn = (spCacheView) ? spCacheView->getSpanFromColumn(nColData) : Span<const double>();

        // Getting here means that couldn't satisfy request from cache; visiting rows one-by-one and calling handler with single-row spans.
        GraphDataSource::ColumnDataTypeMap mapColToDataType;
        for (auto r = item.top(), rBottom = item.bottom(); r <= rBottom; ++r)
        {
            // Note that indexes are view indexes, not source model indexes (e.g. in case of filtered table, row indexes in filtered table)
            const auto sourceModelIndex = (bIsRowIndexMappingNeeded) ? m_spView->mapToDataModel(pModel->index(r, nColView.value())) : pCsvModel->index(r, nColData.value());
            const auto pszData = (!bNumberCacheIsAvailable || queryDetails.areStringsRequested()) ? pCsvModel->rawStringPtrAt(sourceModelIndex) : SzPtrUtf8R(nullptr);
            const auto nSourceModelRow = sourceModelIndex.row();
            SourceDataSpan dataSpan;
            const double row = CsvItemModel::internalRowIndexToVisible(r); // Note: this must have the same lifetime as dataSpan.
            double val = std::numeric_limits<double>::quiet_NaN(); // Note: this must have the same lifetime as dataSpan.
            StringViewUtf8 sv; // Note: this must have the same lifetime as dataSpan.
            if (queryDetails.areRowsRequested())
                dataSpan.setRows(makeRange(&row, &row + 1));
            if (queryDetails.areNumbersRequested())
            {
                if (isValidIndex(cachedColumn, nSourceModelRow))
                    val = cachedColumn[nSourceModelRow];
                else
                    val = cellStringToDoubleImpl(pszData, nColView, mapColToDataType);
                dataSpan.set(makeRange(&val, &val + 1));
            }
            if (queryDetails.areStringsRequested())
            {
                sv = (pszData) ? pszData : StringViewUtf8();
                dataSpan.set(makeRange(&sv, &sv + 1));
            }
            if (r == rBottom) // On last row, inserting column metadata to dataspan
            {
                insertColumnMetaData(dataSpan, false, (bNumberCacheIsAvailable) ? spCacheView->getColumnDataType(nColData) : mapColToDataType.valueCopyOr(nColView.value()));
                DFG_OPAQUE_REF().m_viewableMapColToDataType.edit([&](auto& rEditable, auto pOld) { DFG_UNUSED(pOld); rEditable = std::move(mapColToDataType); });
            }
            handler(dataSpan);
        }
    }
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::columnIndexes() const -> IndexList
{
    IndexList rv;
    auto spSelectionViewer = privGetSelectionViewer();
    if (spSelectionViewer)
        std::transform(spSelectionViewer->m_columnNames.begin(), spSelectionViewer->m_columnNames.end(), std::back_inserter(rv), [](const auto& keyVal) { return keyVal.first; });
    return rv;
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::columnCount() const -> DataSourceIndex
{
    auto spSelectionViewer = privGetSelectionViewer();
    return (spSelectionViewer) ? spSelectionViewer->m_columnNames.size() : 0;
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::columnIndexByName(const StringViewUtf8 sv) const -> DataSourceIndex
{
    auto spSelectionViewer = privGetSelectionViewer();
    if (!spSelectionViewer)
        return invalidIndex();
    const auto sSearch = viewToQString(sv);
    // Linear search from values of (index, column name) map
    for (const auto& kv : spSelectionViewer->m_columnNames)
    {
        if (kv.second == sSearch)
            return kv.first;
    }
    return invalidIndex();
}

bool ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::enable(const bool b)
{
    if (!m_spView)
        return false;
    if (b)
    {
        m_spView->addSelectionAnalyzer(m_spSelectionAnalyzer);
        m_spSelectionAnalyzer->addSelectionToQueue(m_spView->getSelection());
        return true;
    }
    else
    {
        m_spView->removeSelectionAnalyzer(m_spSelectionAnalyzer.get());
        return false;
    }
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::columnDataTypes() const -> ColumnDataTypeMap
{
    auto p = DFG_OPAQUE_PTR();
    if (!p)
        return ColumnDataTypeMap();
    auto spCacheView = p->m_cacheViewer.view();
    if (spCacheView && !spCacheView->m_mapColToDataTypes.empty())
        return spCacheView->m_mapColToDataTypes;
    auto spDataTypeMapView = p->m_viewerMapColToDataType.view();
    return (spDataTypeMapView) ? *spDataTypeMapView : ColumnDataTypeMap();
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::columnNames() const -> ColumnNameMap
{
    auto spSelectionViewer = privGetSelectionViewer();
    return (spSelectionViewer) ? spSelectionViewer->m_columnNames : ColumnNameMap();
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::privGetSelectionViewer() const -> std::shared_ptr<const SelectionAnalyzerForGraphing::SelectionInfo>
{
    return m_selectionViewer.view();
}

void ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::setChartDefinitionViewer(ChartDefinitionViewer sp)
{
    if (m_spSelectionAnalyzer)
        m_spSelectionAnalyzer->setChartDefinitionViewer(std::move(sp));
}

void ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::onSelectionAnalysisCompleted()
{
    Q_EMIT sigChanged();
}

bool ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::isSafeToQueryDataFromThreadImpl(const QThread*) const
{
    return true;
}

void ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::resetCache()
{
    DFG_OPAQUE_REF().m_cache.reset(); // TODO: make less coarse-grained; this bluntly wipes out all caches without retaining any of the allocated memory, which often could be reused for future caching.
}
