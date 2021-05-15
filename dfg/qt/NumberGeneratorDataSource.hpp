#pragma once

#include "../dfgDefs.hpp"
#include "graphTools.hpp"
#include "../OpaquePtr.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) {

/*
 * Data source providing generated numbers without any external dependencies to files etc.
 * Note: forEachElement_byColumn() only provives data for query mask 'DataMaskNumerics', fetchColumnNumberData() provides also DataMaskRows.
 *
*/
class NumberGeneratorDataSource : public GraphDataSource
{
public:
    using BaseClass = GraphDataSource;

    NumberGeneratorDataSource(QString sId, DataSourceIndex nRowCount); // Creates object identical to createByCountFirstStep(sId, nRowCount, 0, 1)
    NumberGeneratorDataSource(QString sId, DataSourceIndex nRowCount, DataSourceIndex transferBlockSize);
private:
    NumberGeneratorDataSource(QString sId, DataSourceIndex nRowCount, double first, double step, DataSourceIndex transferBlockSize);
public:
    ~NumberGeneratorDataSource() override;

    static std::unique_ptr<NumberGeneratorDataSource> createByCountFirstStep(QString sId, DataSourceIndex nRowCount, double first, double step);
    static std::unique_ptr<NumberGeneratorDataSource> createByCountFirstLast(QString sId, DataSourceIndex nRowCount, double first, double step);
    // Returns generator for values {first, first + step, ..., first + N*step }, where N is such that first + N*step <= last and first + (N+1)*step > last.
    // Note: 'last' is not guaranteed to appear in the sequence: this can also happen due to rounding errors.
    static std::unique_ptr<NumberGeneratorDataSource> createByfirstLastStep(QString sId, double first, double last, double step);

// Begin: interface overloads -->
    auto columnCount() const -> DataSourceIndex override;
    auto columnIndexes() const -> IndexList override;
    auto columnNames() const -> ColumnNameMap override;
    bool enable(bool b) override;
    auto columnDataTypes() const -> ColumnDataTypeMap override;
    auto columnDataType(DataSourceIndex nCol) const -> ChartDataType override;
    void forEachElement_byColumn(DataSourceIndex, const DataQueryDetails&, ForEachElementByColumHandler) override;
    void fetchColumnNumberData(GraphDataSourceDataPipe&& pipe, const DataSourceIndex nColumn, const DataQueryDetails& queryDetails) override;
private:
    bool isSafeToQueryDataFromThreadImpl(const QThread*) const override { return true; }
// End interface overloads <--

public:
    DataSourceIndex rowCount() const;
    static DataSourceIndex maxRowCount();
    static DataSourceIndex defaultTransferBlockSize();

    DFG_OPAQUE_PTR_DECLARE();

}; // class NumberGeneratorDataSource

}} // module namespace
