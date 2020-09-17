#include "../dfgDefs.hpp"
#include "graphTools.hpp"
#include "../cont/tableCsv.hpp"
#include "../cont/MapToStringViews.hpp"
#include "../str/string.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) {

class CsvFileDataSource : public GraphDataSource
{
public:
    CsvFileDataSource(const QString& sPath, QString sId);

// Begin: interface overloads -->
    auto columnCount() const -> DataSourceIndex override;
    auto columnDataTypes() const -> ColumnDataTypeMap override;
    auto columnIndexByName(const StringViewUtf8 sv) const -> DataSourceIndex override;
    auto columnIndexes() const -> IndexList override;
    auto columnNames() const -> ColumnNameMap override;
    void enable(bool b) override;
    void forEachElement_byColumn(DataSourceIndex nCol, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler) override;
    auto underlyingSource() -> QObject* override;
    // End interface overloads <--

    std::string m_sPath; // File path in native 8-bit encoding.
    ::DFG_MODULE_NS(cont)::MapToStringViews<DataSourceIndex, StringUtf8> m_columnIndexToColumnName;
    ColumnDataTypeMap m_columnDataTypes;
    CsvFormatDefinition m_format;
}; // class CsvFileDataSource

}} // module namespace
