#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QPointer>
DFG_END_INCLUDE_QT_HEADERS

class QVariant;

#include "graphTools.hpp"
#include "CsvItemModel.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {

namespace DFG_DETAIL_NS
{
} // namespace DFG_DETAIL_NS


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   CsvItemModelDataSource
//
//   DataSource for reading from CsvItemModel.
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class CsvItemModelChartDataSource : public GraphDataSource
{
public:
    using BaseClass = GraphDataSource;

    // TODO: life-time requirements
    CsvItemModelChartDataSource(CsvItemModel* pModel, QString sId);
    ~CsvItemModelChartDataSource() override;

    // Begin: interface overloads -->
    QObject* underlyingSource() override;
    void forEachElement_byColumn(DataSourceIndex c, ForEachElementByColumHandler handler) override;
    DataSourceIndex columnCount() const override;
    IndexList       columnIndexes() const override;
    DataSourceIndex columnIndexByName(const StringViewUtf8 sv) const override;
    SingleColumnDoubleValuesOptional singleColumnDoubleValues_byOffsetFromFirst(DataSourceIndex offsetFromFirst) override;
    SingleColumnDoubleValuesOptional singleColumnDoubleValues_byColumnIndex(DataSourceIndex nColIndex) override;
    void enable(bool b) override;
    ColumnDataTypeMap columnDataTypes() const override;
    ColumnNameMap columnNames() const override;
    // End interface overloads <--

    const CsvItemModel* privGetCsvModel() const;
    const CsvItemModel::DataTable* privGetDataTable() const;

    QPointer<const CsvItemModel> m_spModel;
    ColumnDataTypeMap m_columnTypes;
}; // class CsvItemModelChartDataSource


} } // Module namespace
