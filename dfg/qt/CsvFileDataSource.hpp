#pragma once

#include "../dfgDefs.hpp"
#include "graphTools.hpp"
#include "../cont/tableCsv.hpp"
#include "../cont/MapToStringViews.hpp"
#include "../str/string.hpp"
#include "containerUtils.hpp"
#include "FileDataSource.hpp"

class QFileSystemWatcher;

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) {

class CsvFileDataSource : public FileDataSource
{
public:
    using BaseClass = FileDataSource;
    CsvFileDataSource(const QString& sPath, QString sId);
    ~CsvFileDataSource() override;

// Begin: GraphDataSource interface overloads -->
    void forEachElement_byColumn(DataSourceIndex nCol, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler) override;
    auto underlyingSource() -> QObject* override;
private:
    void refreshAvailabilityImpl() override { privUpdateStatusAndAvailability(); }
// End GraphDataSource interface overloads <--


// Begin: FileDataSource overloads -->
private:
    void updateColumnInfoImpl() override;
// End: FileDataSource overloads -->

public slots:
    void onFileChanged();

public:

    CsvFormatDefinition m_format;
}; // class CsvFileDataSource

}} // module namespace
