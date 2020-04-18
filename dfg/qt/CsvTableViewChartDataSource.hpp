DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QObject>
#include <QPointer>
#include <QVariant>
DFG_END_INCLUDE_QT_HEADERS

#include "graphTools.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"
#include "../cont/MapVector.hpp"
#include "../cont/ViewableSharedPtr.hpp"
#include "../alg.hpp"
#include <memory>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {

// Selection analyzer that gathers data from selection for graphs.
class SelectionAnalyzerForGraphing : public dfg::qt::CsvTableViewSelectionAnalyzer
{
public:
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<double, double> RowToValueMap;
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<int, RowToValueMap> ColumnToValuesMap;
    typedef ColumnToValuesMap Table;

    void analyzeImpl(const QItemSelection selection) override
    {
        auto pView = qobject_cast<dfg::qt::CsvTableView*>(this->m_spView.data());
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

            dfg::cont::SetVector<int> presentColumnIndexes;
            presentColumnIndexes.setSorting(true); // Sorting is needed for std::set_difference

            // Clearing all values from every column so that those would actually contain current values, not remnants from previous update.
            // Note that removing unused columns is done after going through the selection.
            dfg::alg::forEachFwd(rTable.valueRange(), [](RowToValueMap& columnValues) { columnValues.clear(); });

            // Sorting is needed for std::set_difference
            rTable.setSorting(true);

            CompletionStatus completionStatus = CompletionStatus_started;

            dfg::qt::CsvTableView::forEachIndexInSelection(*pView, selection, dfg::qt::CsvTableView::ModelIndexTypeView, [&](const QModelIndex& mi, bool& rbContinue)
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
                sVal.replace(',', '.'); // Hack: to make comma-localized values such as "1,2" be interpreted as 1.2
                rTable[mi.column()][dfg::qt::CsvItemModel::internalRowIndexToVisible(mi.row())] = sVal.toDouble(); // Note that indexes are view indexes, not source model indexes (e.g. in case of filtered table, row indexes in filtered table)
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

    dfg::cont::ViewableSharedPtr<Table> m_spTable;
};

// TODO: consider moving to dfg/qt instead of being here in dfgQtTableEditor main.cpp
class CsvTableViewChartDataSource : public dfg::qt::GraphDataSource
{
public:
    CsvTableViewChartDataSource(dfg::qt::CsvTableView* view)
        : m_spView(view)
    {
        if (!m_spView)
            return;
        m_spSelectionAnalyzer = std::make_shared<SelectionAnalyzerForGraphing>();
        m_spSelectionAnalyzer->m_spTable.reset(std::make_shared<SelectionAnalyzerForGraphing::Table>());
        m_spDataViewer = m_spSelectionAnalyzer->m_spTable.createViewer();
        DFG_QT_VERIFY_CONNECT(connect(m_spSelectionAnalyzer.get(), &dfg::qt::CsvTableViewSelectionAnalyzer::sigAnalyzeCompleted, this, &dfg::qt::GraphDataSource::sigChanged));
        m_spView->addSelectionAnalyzer(m_spSelectionAnalyzer);
    }

    QObject* underlyingSource() override
    {
        return m_spView;
    }

    void forEachElement_fromTableSelection(std::function<void (DataSourceIndex, DataSourceIndex, QVariant)> handler) override
    {
        if (!handler || !m_spView)
            return;

        auto spTableView = (m_spDataViewer) ? m_spDataViewer->view() : nullptr;
        if (!spTableView) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
            return;
        const auto& rTable = *spTableView;

        for(auto iterCol = rTable.begin(); iterCol != rTable.end(); ++iterCol)
        {
            for (auto iterRow = iterCol->second.begin(); iterRow != iterCol->second.end(); ++iterRow)
            {
                handler(static_cast<DataSourceIndex>(iterRow->first), static_cast<DataSourceIndex>(iterCol->first), iterRow->second);
            }
        }
    }

    DataSourceIndex columnCount() const override
    {
        auto spTableView = privGetTableView();
        return (spTableView) ? spTableView->size() : 0;
    }

    DataSourceIndex columnIndexByName(const dfg::StringViewUtf8 sv) const override
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

    SingleColumnDoubleValuesOptional singleColumnDoubleValues_byOffsetFromFirst(const DataSourceIndex offsetFromFirst) override
    {
        auto spTableView = privGetTableView();
        if (!spTableView) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
            return SingleColumnDoubleValuesOptional();
        const auto& rTable = *spTableView;
        if (rTable.empty())
            return SingleColumnDoubleValuesOptional();
        const DataSourceIndex nFirstCol = rTable.frontKey();
        const auto nTargetCol = nFirstCol + offsetFromFirst;
        return singleColumnDoubleValues_byColumnIndex(nTargetCol);
    }

    SingleColumnDoubleValuesOptional singleColumnDoubleValues_byColumnIndex(const DataSourceIndex nColIndex) override
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
        return rv;
    }

    void enable(const bool b) override
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

    std::shared_ptr<const SelectionAnalyzerForGraphing::Table> privGetTableView() const
    {
        return (m_spDataViewer) ? m_spDataViewer->view() : nullptr;
    }

    QPointer<dfg::qt::CsvTableView> m_spView;
    std::shared_ptr<dfg::cont::ViewableSharedPtrViewer<SelectionAnalyzerForGraphing::Table>> m_spDataViewer;
    std::shared_ptr<SelectionAnalyzerForGraphing> m_spSelectionAnalyzer;
};

} } // Module namespace
