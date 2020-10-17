#pragma once

#include "../dfgDefs.hpp"
#include "graphTools.hpp"
#include "../str/string.hpp"
#include "containerUtils.hpp"
#include "../cont/MapToStringViews.hpp"

class QFileSystemWatcher;

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) {

class FileDataSource : public GraphDataSource
{
public:
    using BaseClass = GraphDataSource;

    FileDataSource(const QString& sPath, QString sId);
    ~FileDataSource() override;

// Begin: interface overloads -->
    auto columnCount() const -> DataSourceIndex override;
    auto columnIndexByName(const StringViewUtf8 sv) const -> DataSourceIndex override;
    auto columnIndexes() const -> IndexList override;
    auto columnNames() const -> ColumnNameMap override;
    void enable(bool b) override;
    auto columnDataTypes() const -> ColumnDataTypeMap override;
    auto columnDataType(DataSourceIndex nCol) const -> ChartDataType override;
private:
    String statusDescriptionImpl() const override;
    void refreshAvailabilityImpl() override { privUpdateStatusAndAvailability(); }
// End interface overloads <--

public slots:
    void onFileChanged();

public:
    void updateColumnInfo() { updateColumnInfoImpl(); }

private:
    virtual void updateColumnInfoImpl() = 0;

public:

// Internals
    // Updates status and availability and return true iff file is readable.
    bool privUpdateStatusAndAvailability();

    std::string m_sPath; // File path in native 8-bit encoding.
    String m_sStatusDescription;
    QObjectStorage<QFileSystemWatcher> m_spFileWatcher;
    ColumnDataTypeMap m_columnDataTypes;
    ::DFG_MODULE_NS(cont)::MapToStringViews<DataSourceIndex, StringUtf8> m_columnIndexToColumnName;
}; // class FileDataSource

}} // module namespace
