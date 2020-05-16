#include "CsvTableViewChartDataSource.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QDateTime>
DFG_END_INCLUDE_QT_HEADERS

#include "connectHelper.hpp"
#include "CsvItemModel.hpp"

#include "../alg.hpp"
#include "../cont/valueArray.hpp"


void ::DFG_MODULE_NS(qt)::SelectionAnalyzerForGraphing::analyzeImpl(const QItemSelection selection)
{
    auto pView = qobject_cast<CsvTableView*>(this->m_spView.data());
    if (!pView || !m_spTable)
        return;

    m_spTable.edit([&](Table& rTable, const Table* pOld)
    {
        // TODO: this may be storing way too much data: e.g. if all items are selected, graphing may not use any or perhaps only two columns.
        //       -> filter according to actual needs. Note that in order to be able to do so, would need to know something about graph definitions here.
        // TODO: which indexes to use e.g. in case of filtered table: source row indexes or view rows?
        //      -e.g. if table of 1000 rows is filtered so that only rows 100, 600 and 700 are shown, should single column graph use
        //       values 1, 2, 3 or 100, 600, 700 as x-coordinate value? This might be something to define in graph definition -> another reason why
        //       graph definitions should be somehow known at this point.

        DFG_UNUSED(pOld);

        ::DFG_MODULE_NS(cont)::SetVector<int> presentColumnIndexes;
        presentColumnIndexes.setSorting(true); // Sorting is needed for std::set_difference

        // Clearing all values from every column so that those would actually contain current values, not remnants from previous update.
        // Note that removing unused columns is done after going through the selection.
        ::DFG_MODULE_NS(alg)::forEachFwd(rTable.valueRange(), [](RowToValueMap& columnValues) { columnValues.clear(); });

        // Sorting is needed for std::set_difference
        rTable.setSorting(true);

        CompletionStatus completionStatus = CompletionStatus_started;
        rTable.m_columnTypes.clear();
        rTable.m_columnNames.clear();

        // Getting column names
        {
            auto pModel = pView->csvModel();
            if (pModel)
            {
                const auto nCount = pModel->columnCount();
                for (int i = 0; i < nCount; ++i)
                    rTable.m_columnNames[i] = pModel->getHeaderName(i);
            }
        }

        CsvTableView::forEachIndexInSelection(*pView, selection, CsvTableView::ModelIndexTypeView, [&](const QModelIndex& mi, bool& rbContinue)
        {
            if (m_abNewSelectionPending)
            {
                completionStatus = ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus_terminatedByNewSelection;
                rbContinue = false;
                return;
            }
            else if (!m_abIsEnabled.load(std::memory_order_relaxed))
            {
                completionStatus = ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus_terminatedByDisabling;
                rbContinue = false;
                return;
            }

            if (!mi.isValid())
                return;
            auto sVal = mi.data().toString();
            presentColumnIndexes.insert(mi.column());
            // Note that indexes are view indexes, not source model indexes (e.g. in case of filtered table, row indexes in filtered table)
            const auto nCol = mi.column();
            rTable[mi.column()][CsvItemModel::internalRowIndexToVisible(mi.row())] = GraphDataSource::cellStringToDouble(sVal, nCol, rTable.m_columnTypes);
        });

        // Removing unused columns from rTable.
        std::vector<int> colsToRemove;
        const auto colsInTable = rTable.keyRange();
        std::set_difference(colsInTable.cbegin(), colsInTable.cend(), presentColumnIndexes.cbegin(), presentColumnIndexes.cend(), std::back_inserter(colsToRemove));
        for (auto iter = colsToRemove.cbegin(), iterEnd = colsToRemove.cend(); iter != iterEnd; ++iter)
        {
            rTable.erase(*iter);
        }
    });

    Q_EMIT sigAnalyzeCompleted();
}

::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::CsvTableViewChartDataSource(CsvTableView* view)
    : m_spView(view)
{
    if (!m_spView)
        return;
    m_uniqueId = QString("viewSelection_%1").arg(QString::number(QDateTime::currentMSecsSinceEpoch() % 1000000));
    m_spSelectionAnalyzer = std::make_shared<SelectionAnalyzerForGraphing>();
    m_spSelectionAnalyzer->m_spTable.reset(std::make_shared<SelectionAnalyzerForGraphing::Table>());
    m_spDataViewer = m_spSelectionAnalyzer->m_spTable.createViewer();
    DFG_QT_VERIFY_CONNECT(connect(m_spSelectionAnalyzer.get(), &CsvTableViewSelectionAnalyzer::sigAnalyzeCompleted, this, &GraphDataSource::sigChanged));
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

void ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::forEachElement_byColumn(DataSourceIndex c, ForEachElementByColumHandler handler)
{
    if (!handler || !m_spView)
        return;

    auto spTableView = (m_spDataViewer) ? m_spDataViewer->view() : nullptr;
    if (!spTableView) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
        return;
    const auto& rTable = *spTableView;

    auto iter = rTable.find(c);

    if (iter == rTable.end())
        return;

    const auto& columnData = iter->second;

    handler(columnData.m_keyStorage.data(), columnData.m_valueStorage.data(), nullptr, columnData.size());
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::columnIndexes() const -> IndexList
{
    IndexList rv;
    auto spTableView = privGetTableView();
    if (spTableView)
        rv.assign(spTableView->keyRange());
    return rv;
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::columnCount() const -> DataSourceIndex
{
    auto spTableView = privGetTableView();
    return (spTableView) ? spTableView->size() : 0;
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::columnIndexByName(const StringViewUtf8 sv) const -> DataSourceIndex
{
    auto pModel = (m_spView) ? m_spView->csvModel() : nullptr;
    if (!pModel)
        return invalidIndex();
    auto nIndex = pModel->findColumnIndexByName(QString::fromUtf8(sv.beginRaw(), static_cast<int>(sv.size())), -1);
    if (nIndex < 0)
        return invalidIndex();
    // Getting here means that column was found from the CsvItemModel table; must still check if the column is present in selection.
    auto spTableView = privGetTableView();
    auto pTable = (spTableView) ? &*spTableView : nullptr;
    if (!pTable)
        return invalidIndex();
    auto iter = pTable->find(nIndex);
    return (iter != pTable->end()) ? static_cast<DataSourceIndex>(iter->first) : invalidIndex();
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::singleColumnDoubleValues_byOffsetFromFirst(const DataSourceIndex offsetFromFirst) -> SingleColumnDoubleValuesOptional
{
    auto spTableView = privGetTableView();
    if (!spTableView) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
        return SingleColumnDoubleValuesOptional();
    const auto& rTable = *spTableView;
    if (rTable.empty())
        return SingleColumnDoubleValuesOptional();
    const DataSourceIndex nFirstCol = static_cast<DataSourceIndex>(rTable.frontKey());
    const auto nTargetCol = nFirstCol + offsetFromFirst;
    return singleColumnDoubleValues_byColumnIndex(nTargetCol);
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::singleColumnDoubleValues_byColumnIndex(const DataSourceIndex nColIndex) -> SingleColumnDoubleValuesOptional
{
    auto spTableView = privGetTableView();
    if (!spTableView) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
        return SingleColumnDoubleValuesOptional();
    const auto& rTable = *spTableView;
    if (rTable.empty())
        return SingleColumnDoubleValuesOptional();
    auto iter = rTable.find(nColIndex);
    if (iter == rTable.end())
        return SingleColumnDoubleValuesOptional();
    const auto& values = iter->second.valueRange();
    auto rv = std::make_shared<DoubleValueVector>();
    rv->resize(values.size());
    std::copy(values.cbegin(), values.cend(), rv->begin());
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
    auto spViewer = privGetTableView();
    return (spViewer) ? spViewer->m_columnTypes : ColumnDataTypeMap();
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::columnNames() const -> ColumnNameMap
{
    auto spViewer = privGetTableView();
    return (spViewer) ? spViewer->m_columnNames : ColumnNameMap();
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewChartDataSource::privGetTableView() const -> std::shared_ptr<const SelectionAnalyzerForGraphing::Table>
{
    return (m_spDataViewer) ? m_spDataViewer->view() : nullptr;
}
