#pragma once

#include "../dfgDefs.hpp"
#include "FileDataSource.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) {

class SQLiteFileDataSource : public FileDataSource
{
public:
    using BaseClass = FileDataSource;
    SQLiteFileDataSource(const QString& sQuery, const QString& sPath, QString sId);
    ~SQLiteFileDataSource() override;

// Begin: interface overloads -->
    void forEachElement_byColumn(DataSourceIndex nCol, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler) override;
    auto underlyingSource() -> QObject* override;
private:
    void refreshAvailabilityImpl() override { privUpdateStatusAndAvailability(); }
// End interface overloads <--

// Begin: FileDataSource overloads -->
private:
    void updateColumnInfoImpl() override;
// End: FileDataSource overloads -->

public:
    QString m_sQuery;
}; // class SQLiteFileDataSource

}} // module namespace
