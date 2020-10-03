#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "containerUtils.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QSql>
    #include <QMap>
    #include <QString>
    #include <QStringList>

    // UI includes
    #include <QDialog>
DFG_END_INCLUDE_QT_HEADERS

class QSqlDatabase;
class QSqlQuery;
class QSqlRecord;
class QStringListModel;

class QLabel;
class QListView;
class QLineEdit;
class QVBoxLayout;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(sql) {


// RAII-wrapper for QSqlDatabase: with QSqlDatabase opening a database and destroying QSqlDatabase-object doesn't seem to close connection meaning
// e.g. that the database file is in use by the application even if nothing actually references it. Using this class guarantees the connection
// to database gets closed when database object is destroyed.
// Note: by default database is opened in read-only mode.
class SQLiteDatabase
{
public:
    SQLiteDatabase(const QString& sFilePath) : SQLiteDatabase(sFilePath, "QSQLITE_OPEN_READONLY") {}
    SQLiteDatabase(const QString& sFilePath, const QString& sConnectOptions);
    ~SQLiteDatabase();

    // Returns name of tables in the database.
    QStringList tableNames(const QSql::TableType type = QSql::Tables) const;

    bool isOpen() const;

    QSqlRecord record(const QString& sTableName) const;
    
    QSqlQuery createQuery();

    // Returns true iff query looks like a select.
    // Note: does only simple, non-robust checking.
    static bool isSelectQuery(const QString& sQuery);

    static bool isSQLiteFileExtension(const QString& sExtensionWithoutDot);

    // Returns true iff file at given path looks like SQLite file. Note that returns false if unable to check bytes.
    // By default checks beginning bytes, optionally only checks extension.
    static bool isSQLiteFile(const QString& sPath, bool bCheckExtensionOnly = false);

    // Opens SQLite database from given path.
    static QSqlDatabase openSQLiteDatabase(const QString& sDbFilePath, const QString& sConnectOptions = QString());

    // Returns table names from given SQLite file, empty if file doesn't exist or if opening database fails.
    static QStringList getSQLiteFileTableNames(const QString& sDbFilePath, const QSql::TableType type = QSql::Tables);

    // Returns column names from given SQLite file and it's table, empty if failed to determine names (e.g. if file or table doesn't exist)
    static QStringList getSQLiteFileTableColumnNames(const QString& sDbFilePath, const QString& sTableName);

    std::unique_ptr<QSqlDatabase> m_spDatabase;
};

// Dialog for opening SQLite file
class SQLiteFileOpenDialog : public QDialog
{
public:
    using BaseClass = QDialog;
    template <class T> using QObjectStorage = ::DFG_MODULE_NS(qt)::QObjectStorage<T>;

    SQLiteFileOpenDialog(const QString& sFilePath, QWidget* pParent);
    ~SQLiteFileOpenDialog();

    void accept() override;

    QString query() const;

    void tableItemChanged();
    void columnSelectionChanged();

    QString currentTable() const;

    void updateQuery(const QString& sTable, const QStringList& columns);

    QString m_sFilePath;
    QStringList m_tableNames;
    QObjectStorage<QVBoxLayout> m_spLayout;
    QObjectStorage<QLineEdit> m_spQueryLineEdit;
    QObjectStorage<QLabel> m_spErrorLine;
    QObjectStorage<QListView> m_spTableView;
    QObjectStorage<QListView> m_spColumnView;
    QObjectStorage<QStringListModel> m_spTableNameModel;
    QObjectStorage<QStringListModel> m_spColumnNameModel;
    QObjectStorage<QLabel> m_spColumnListLabel;
    QMap<QString, QStringList> m_mapTableToColumnNames;
};

} } // module namespace
