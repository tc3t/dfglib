#include "CsvTableViewChartDataSource.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QDateTime>
DFG_END_INCLUDE_QT_HEADERS

#include "connectHelper.hpp"
#include "CsvItemModel.hpp"

#include "../alg.hpp"
#include "../cont/valueArray.hpp"


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
                    for (int i = item.left(), right = item.right(); i <= right; ++i)
                        rSelectionInfo.m_columnNames[static_cast<DataSourceIndex>(i)] = pModel->getHeaderName(i);
                }
            }
        }
    });

    if (bSignalChange)
        Q_EMIT sigAnalyzeCompleted();
}

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
}

::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::~CsvTableViewChartDataSource()
{

}

QObject* ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::underlyingSource()
{
    return m_spView;
}

void ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::forEachElement_byColumn(DataSourceIndex nColRaw, ForEachElementByColumHandler handler)
{
    if (!handler || !m_spView)
        return;

    auto pModel = m_spView->model();
    if (!pModel)
        return;

    auto spSelectionViewer = this->privGetSelectionViewer();
    if (!spSelectionViewer) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
        return;

    auto editLockReleaser = m_spView->tryLockForEdit();
    if (!editLockReleaser.isLocked())
        return;

    const QItemSelection& selection  = *spSelectionViewer;

    const auto c = saturateCast<int>(nColRaw);

    for (const auto& item : selection)
    {
        if (item.left() > c || item.right() < c)
            continue;
        for (int r = item.top(), rBottom = item.bottom(); r <= rBottom; ++r)
        {
            auto data = pModel->data(pModel->index(r, c));
            if (data.isNull())
                return;
            auto sVal = data.toString();
            // Note that indexes are view indexes, not source model indexes (e.g. in case of filtered table, row indexes in filtered table)
            const double x = CsvItemModel::internalRowIndexToVisible(r);
            const double y = GraphDataSource::cellStringToDouble(sVal, c, m_columnTypes);
            handler(&x, &y, nullptr, 1);
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

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::singleColumnDoubleValues_byOffsetFromFirst(const DataSourceIndex offsetFromFirst) -> SingleColumnDoubleValuesOptional
{
    auto spSelectionViewer = privGetSelectionViewer();
    if (!spSelectionViewer) // This may happen e.g. if the selection is being updated in analyzeImpl() (in another thread). 
        return SingleColumnDoubleValuesOptional();
    if (offsetFromFirst < 0 || offsetFromFirst >= spSelectionViewer->m_columnNames.size())
        return SingleColumnDoubleValuesOptional();
    return singleColumnDoubleValues_byColumnIndex((spSelectionViewer->m_columnNames.begin() + offsetFromFirst)->first);
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::singleColumnDoubleValues_byColumnIndex(const DataSourceIndex nColIndex) -> SingleColumnDoubleValuesOptional
{
    auto rv = std::make_shared<DoubleValueVector>();
    forEachElement_byColumn(nColIndex, [&](const double* pX, const double* pY, Dummy, const DataSourceIndex n)
    {
        DFG_UNUSED(pX);
        rv->insert(rv->end(), pY, pY + n);
    });
    return std::move(rv); // explicit move to avoid "call 'std::move' explicitly to avoid copying on older compilers"-warning in Qt Creator   
}

void ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::enable(const bool b)
{
    if (!m_spView)
        return;
    if (b)
    {
        m_spView->addSelectionAnalyzer(m_spSelectionAnalyzer);
        m_spSelectionAnalyzer->addSelectionToQueue(m_spView->getSelection());
    }
    else
        m_spView->removeSelectionAnalyzer(m_spSelectionAnalyzer.get());
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::columnDataTypes() const -> ColumnDataTypeMap
{
    return m_columnTypes;
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
    m_columnTypes.clear();
    Q_EMIT sigChanged();
}
