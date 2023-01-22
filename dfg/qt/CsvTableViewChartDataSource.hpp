#pragma once

#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QObject>
#include <QPointer>
DFG_END_INCLUDE_QT_HEADERS

#include "graphTools.hpp"
#include "CsvTableView.hpp"
#include "../cont/MapVector.hpp"
#include "../cont/ViewableSharedPtr.hpp"
#include "../OpaquePtr.hpp"

#include <atomic>
#include <memory>


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   SelectionAnalyzerForGraphing
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Selection analyzer that gathers data from selection for graphs.
class SelectionAnalyzerForGraphing : public CsvTableViewSelectionAnalyzer
{
public:
    typedef ::DFG_MODULE_NS(cont)::MapVectorSoA<double, double> RowToValueMap;
    typedef ::DFG_MODULE_NS(cont)::MapVectorSoA<int, RowToValueMap> ColumnToValuesMap;

    class SelectionInfo : public QItemSelection
    {
    public:
        GraphDataSource::ColumnNameMap m_columnNames;
    };

    void analyzeImpl(QItemSelection selection) override;

    void setChartDefinitionViewer(ChartDefinitionViewer viewer);

    ::DFG_MODULE_NS(cont)::ViewableSharedPtr<SelectionInfo> m_spSelectionInfo;
    GraphDataSourceId m_sSourceId;
    ChartDefinitionViewer m_chartDefinitionViewer;
    std::atomic<ChartDefinitionViewer*> m_apChartDefinitionViewer;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   CsvTableViewChartDataSource
//
//   Implements data source for CsvTableView selection.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CsvTableViewChartDataSource : public GraphDataSource
{
public:
    using BaseClass = GraphDataSource;

    CsvTableViewChartDataSource(CsvTableView* view);
    ~CsvTableViewChartDataSource();

    QObject* underlyingSource() override;

    void forEachElement_byColumn(DataSourceIndex c, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler) override;
    void fetchColumnNumberData(GraphDataSourceDataPipe& pipe, const DataSourceIndex nColumn, const DataQueryDetails& queryDetails) override;

    IndexList columnIndexes() const override;

    DataSourceIndex columnCount() const override;

    DataSourceIndex columnIndexByName(const StringViewUtf8 sv) const override;

    bool enable(bool b) override;

    ColumnDataTypeMap columnDataTypes() const override;

    ColumnNameMap columnNames() const override;

    void setChartDefinitionViewer(ChartDefinitionViewer) override;

    std::shared_ptr<const SelectionAnalyzerForGraphing::SelectionInfo> privGetSelectionViewer() const;

    bool isCachingAllowed() const;
    void setCachingAllowed(bool bAllowed);

private:
    bool isSafeToQueryDataFromThreadImpl(const QThread* pThread) const override;

public slots:
    void onSelectionAnalysisCompleted();
    void resetCache();

public:
    QPointer<CsvTableView> m_spView;
    ::DFG_MODULE_NS(cont)::ViewableSharedPtrViewer<SelectionAnalyzerForGraphing::SelectionInfo> m_selectionViewer;
    std::shared_ptr<SelectionAnalyzerForGraphing> m_spSelectionAnalyzer;

    class DataSourceNumberCache;
    DFG_OPAQUE_PTR_DECLARE();
}; // class CsvTableViewChartDataSource

} } // Module namespace
