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
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<double, double> RowToValueMap;
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<int, RowToValueMap> ColumnToValuesMap;
    class Table : public ColumnToValuesMap
    {
    public:
        GraphDataSource::ColumnDataTypeMap m_columnTypes;
        GraphDataSource::ColumnNameMap m_columnNames;
    };

    inline void analyzeImpl(QItemSelection selection) override;

    ::DFG_MODULE_NS(cont)::ViewableSharedPtr<Table> m_spTable;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   CsvTableViewChartDataSource
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CsvTableViewChartDataSource : public GraphDataSource
{
public:
    using BaseClass = GraphDataSource;

    CsvTableViewChartDataSource(CsvTableView* view);

    QObject* underlyingSource() override;

    void forEachElement_byColumn(DataSourceIndex c, ForEachElementByColumHandler handler) override;

    IndexList columnIndexes() const override;

    DataSourceIndex columnCount() const override;

    DataSourceIndex columnIndexByName(const StringViewUtf8 sv) const override;

    SingleColumnDoubleValuesOptional singleColumnDoubleValues_byOffsetFromFirst(DataSourceIndex offsetFromFirst) override;

    SingleColumnDoubleValuesOptional singleColumnDoubleValues_byColumnIndex(DataSourceIndex nColIndex) override;

    void enable(bool b) override;

    ColumnDataTypeMap columnDataTypes() const override;

    ColumnNameMap columnNames() const override;

    std::shared_ptr<const SelectionAnalyzerForGraphing::Table> privGetTableView() const;

    QPointer<CsvTableView> m_spView;
    std::shared_ptr<::DFG_MODULE_NS(cont)::ViewableSharedPtrViewer<SelectionAnalyzerForGraphing::Table>> m_spDataViewer;
    std::shared_ptr<SelectionAnalyzerForGraphing> m_spSelectionAnalyzer;
}; // class CsvTableViewChartDataSource

} } // Module namespace
