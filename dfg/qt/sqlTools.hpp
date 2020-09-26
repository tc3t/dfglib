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
class QStringListModel;

class QLabel;
class QListView;
class QLineEdit;
class QVBoxLayout;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(sql) {

// Opens SQLite database from given path.
QSqlDatabase openSQLiteDatabase(const QString& sDbFilePath);

// Returns table names from given SQLite file, empty if file doesn't exist or if opening database fails.
QStringList getSQLiteFileTableNames(const QString& sDbFilePath, const QSql::TableType type = QSql::Tables);

// Returns column names from given SQLite file and it's table, empty if failed to determine names (e.g. if file or table doesn't exist)
QStringList getSQLiteFileTableColumnNames(const QString& sDbFilePath, const QString& sTableName);


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
