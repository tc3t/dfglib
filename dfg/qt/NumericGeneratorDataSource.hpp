#pragma once

#include "../dfgDefs.hpp"
#include "graphTools.hpp"
#include "../OpaquePtr.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) {

/*
 * Data source providing generated values without any external dependencies to files etc.
 * Only provives data for query mask 'DataMaskNumerics'.
*/
class NumericGeneratorDataSource : public GraphDataSource
{
public:
    using BaseClass = GraphDataSource;

    NumericGeneratorDataSource(QString sId, DataSourceIndex nRowCount);
    NumericGeneratorDataSource(QString sId, DataSourceIndex nRowCount, DataSourceIndex transferBlockSize);
    ~NumericGeneratorDataSource() override;

// Begin: interface overloads -->
    auto columnCount() const -> DataSourceIndex override;
    auto columnIndexes() const -> IndexList override;
    auto columnNames() const -> ColumnNameMap override;
    void enable(bool b) override;
    auto columnDataTypes() const -> ColumnDataTypeMap override;
    auto columnDataType(DataSourceIndex nCol) const -> ChartDataType override;
    void forEachElement_byColumn(DataSourceIndex, const DataQueryDetails&, ForEachElementByColumHandler) override;
private:
    bool isSafeToQueryDataFromThreadImpl(const QThread*) const override { return true; }
// End interface overloads <--

public:
    DataSourceIndex rowCount() const;
    static DataSourceIndex maxRowCount();
    static DataSourceIndex defaultTransferBlockSize();

    DFG_OPAQUE_PTR_DECLARE();

}; // class NumericGeneratorDataSource

}} // module namespace
