#include "SQLiteFileDataSource.hpp"
#include "qtBasic.hpp"
#include "qtIncludeHelpers.hpp"
#include "connectHelper.hpp"
#include "sqlTools.hpp"
#include "../alg.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QSqlQuery>
    #include <QSqlRecord>
DFG_END_INCLUDE_QT_HEADERS


::DFG_MODULE_NS(qt)::SQLiteFileDataSource::SQLiteFileDataSource(const QString& sQuery, const QString& sPath, QString sId)
    : BaseClass(sPath, sId)
    , m_sQuery(sQuery)
{
    if (!privUpdateStatusAndAvailability())
        return;
}

::DFG_MODULE_NS(qt)::SQLiteFileDataSource::~SQLiteFileDataSource() = default;

void ::DFG_MODULE_NS(qt)::SQLiteFileDataSource::updateColumnInfoImpl()
{
    using DataBase = ::DFG_MODULE_NS(sql)::SQLiteDatabase;
    DataBase db(QString::fromLocal8Bit(this->m_sPath.c_str()));
    m_columnDataTypes.clear();
    m_columnIndexToColumnName.clear_noDealloc();
    auto query = db.createQuery();
    if (!query.prepare(m_sQuery))
        return;
    if (!query.exec())
        return;
    auto columns = DataBase::getSQLiteFileTableColumnNames(query.record());
    ::DFG_MODULE_NS(alg)::forEachFwdWithIndexT<DataSourceIndex>(columns, [&](const QString& s, const DataSourceIndex i)
    {
        m_columnIndexToColumnName.insert(i, qStringToStringUtf8(s));
    });
}

QObject* ::DFG_MODULE_NS(qt)::SQLiteFileDataSource::underlyingSource()
{
    return nullptr; // Don't have an underlying source as QObject
}

void ::DFG_MODULE_NS(qt)::SQLiteFileDataSource::forEachElement_byColumn(const DataSourceIndex nCol, const DataQueryDetails& queryDetails, ForEachElementByColumHandler handler)
{
    if (!handler)
        return;
    using DataBase = ::DFG_MODULE_NS(sql)::SQLiteDatabase;
    DataBase db(fileApi8BitToQString(this->m_sPath));
    if (!db.isOpen())
        return;

    auto iterCol = this->m_columnIndexToColumnName.find(nCol);
    if (iterCol == this->m_columnIndexToColumnName.end())
        return;
    const auto svColName = iterCol->second(this->m_columnIndexToColumnName);
    auto query = db.createQuery();

    // In common cases where m_sQuery is simple "SELECT * FROM table" or "SELECT <list of columns> FROM table"
    // querying data for single column could be optimized to "SELECT <column name> FROM table"
    // But since table is constructed by arbitrary user supplied query, adjusting it to include only single column
    // seems tricky at least so simply using the full query and getting only the requested column values.
    if (!query.prepare(m_sQuery))
        return;
    if (!query.exec())
        return;

    ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::SourceSpanBuffer m_sourceSpanBuffer(nCol, queryDetails, &this->m_columnDataTypes, handler);

    // Reading values and filling table.
    const auto nColInt = saturateCast<int>(nCol);
    for (DataSourceIndex nRow = 1; query.next(); ++nRow)
        m_sourceSpanBuffer.storeToBuffer(nRow, query.value(nColInt));
    m_sourceSpanBuffer.submitData();
}
