#pragma once

#include "../../../dfgDefs.hpp"
#include "../../CsvItemModel.hpp"
#include "../../graphTools.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) { namespace DFG_DETAIL_NS {

// Helper class for providing unified access to CsvItemModel double data for charts. 
class CsvItemModelChartDoubleAccesser
{
public:
    using CellIndex = CsvItemModel::Index;
    using ColumnDataTypeMap = GraphDataSource::ColumnDataTypeMap;

    CsvItemModelChartDoubleAccesser(CellIndex nRow, CellIndex nCol, const CsvItemModel& rModel, ColumnDataTypeMap& rColumnTypes, DataSourceIndex nGraphColumnIndex)
        : m_nRow(nRow)
        , m_nCol(nCol)
        , m_rModel(rModel)
        , m_rColumnTypes(rColumnTypes)
        , m_nGraphColumnIndex(nGraphColumnIndex)
    {}

    // This is the interface to call after creating the object.
    double operator()(const SzPtrUtf8R psz) const
    {
        return (psz) ? GraphDataSource::cellStringToDouble(psz, m_nGraphColumnIndex, &m_rColumnTypes, *this) : std::numeric_limits<double>::quiet_NaN();
    }

    // This is interface for GraphDataSource::cellStringToDouble()
    double operator()(const StringViewSzUtf8& sv, ChartDataType* pDataType, double defVal)
    {
        return m_rModel.cellDataAsDouble(sv, m_nRow, m_nCol, pDataType, defVal);
    }

    CellIndex m_nRow;
    CellIndex m_nCol;
    const CsvItemModel& m_rModel;
    ColumnDataTypeMap& m_rColumnTypes;
    DataSourceIndex m_nGraphColumnIndex;
}; // class CsvItemModelChartDoubleAccesser

}}} // Module dfg::qt::DFG_DETAIL_NS

