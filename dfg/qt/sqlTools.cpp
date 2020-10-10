#include "sqlTools.hpp"
#include "connectHelper.hpp"
#include "../isValidIndex.hpp"
#include "../io/fileToByteContainer.hpp"
#include "qtBasic.hpp"

// UI includes
#include "widgetHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QFileInfo>
    #include <QSqlDatabase>
    #include <QSqlError>
    #include <QSqlQuery>
    #include <QSqlRecord>
    #include <QStringListModel>

    // UI includes
    #include <QDialogButtonBox>
    #include <QLabel>
    #include <QLineEdit>
    #include <QListView>
    #include <QVBoxLayout>
    
DFG_END_INCLUDE_QT_HEADERS



::DFG_MODULE_NS(sql)::SQLiteDatabase::SQLiteDatabase(const QString& sFilePath, const QString& sConnectOptions)
{
    m_spDatabase.reset(new QSqlDatabase);
    *m_spDatabase = openSQLiteDatabase(sFilePath, sConnectOptions);
}

::DFG_MODULE_NS(sql)::SQLiteDatabase::~SQLiteDatabase()
{
    commit();
    close();
}

void ::DFG_MODULE_NS(sql)::SQLiteDatabase::close()
{
    if (m_spDatabase)
        m_spDatabase->close();
}

bool ::DFG_MODULE_NS(sql)::SQLiteDatabase::commit()
{
    return (m_spDatabase) ? m_spDatabase->commit() : false;
}

bool ::DFG_MODULE_NS(sql)::SQLiteDatabase::transaction()
{
    return (m_spDatabase) ? m_spDatabase->transaction() : false;
}

auto ::DFG_MODULE_NS(sql)::SQLiteDatabase::lastError() const -> QSqlError
{
    return (m_spDatabase) ? m_spDatabase->lastError() : QString();
}

auto ::DFG_MODULE_NS(sql)::SQLiteDatabase::lastErrorText() const -> QString
{
    return lastError().text();
}

auto ::DFG_MODULE_NS(sql)::SQLiteDatabase::tableNames(const QSql::TableType type) const -> QStringList
{
    return (m_spDatabase) ? m_spDatabase->tables(type) : QStringList();
}

bool ::DFG_MODULE_NS(sql)::SQLiteDatabase::isOpen() const
{
    return (m_spDatabase) ? m_spDatabase->isOpen() : false;
}

auto ::DFG_MODULE_NS(sql)::SQLiteDatabase::record(const QString& sTableName) const -> QSqlRecord
{
    return (m_spDatabase) ? m_spDatabase->record(sTableName) : QSqlRecord();
}

auto ::DFG_MODULE_NS(sql)::SQLiteDatabase::createQuery() -> QSqlQuery
{
    return (m_spDatabase) ? QSqlQuery(*m_spDatabase) : QSqlQuery();
}

auto ::DFG_MODULE_NS(sql)::SQLiteDatabase::openSQLiteDatabase(const QString& sDbFilePath, const QString& sConnectOptions) -> QSqlDatabase
{
    // Note: QSQLITE means "SQLite version 3 or above" at least as of Qt 5.13
    auto database = QSqlDatabase::addDatabase("QSQLITE", sDbFilePath); // Seconds arg is connectionName
    database.setDatabaseName(sDbFilePath);
    database.setConnectOptions(sConnectOptions);
    return (database.open()) ? database : QSqlDatabase();
}

auto ::DFG_MODULE_NS(sql)::SQLiteDatabase::getSQLiteFileTableNames(const QString& sDbFilePath, const QSql::TableType type) -> QStringList
{
    return SQLiteDatabase(sDbFilePath).tableNames(type);
}

auto ::DFG_MODULE_NS(sql)::SQLiteDatabase::getSQLiteFileTableColumnNames(const QString& sDbFilePath, const QString& sTableName) -> QStringList
{
    SQLiteDatabase database(sDbFilePath);
    if (!database.isOpen())
        return QStringList();
    auto record = database.record(sTableName);
    QStringList rv;
    for (int i = 0, nCount = record.count(); i < nCount; ++i)
        rv.push_back(record.fieldName(i));
    return rv;
}

bool ::DFG_MODULE_NS(sql)::SQLiteDatabase::isSQLiteFileExtension(const QString& sExtensionWithoutDot)
{
    return (sExtensionWithoutDot == QLatin1String("sqlite3") || sExtensionWithoutDot == QLatin1String("sqlite") || sExtensionWithoutDot == QLatin1String("db"));
}

bool ::DFG_MODULE_NS(sql)::SQLiteDatabase::isSQLiteFile(const QString& sPath, const bool bCheckExtensionOnly)
{
    if (bCheckExtensionOnly)
        return isSQLiteFileExtension(QFileInfo(sPath).suffix());
    else
    {
        // https://sqlite.org/fileformat.html
        const size_t nCompareCount = 16; // strlen("SQLite format 3") + null (=15+1).
        const char szExpected[nCompareCount] = "SQLite format 3";
        // Reading first 16 bytes from file.
        const auto bytes = ::DFG_MODULE_NS(io)::fileToVector(::DFG_MODULE_NS(qt)::qStringToFileApi8Bit(sPath), 0, nCompareCount);
        return bytes.size() == nCompareCount && std::equal(bytes.begin(), bytes.end(), std::begin(szExpected));
    }
}

bool ::DFG_MODULE_NS(sql)::SQLiteDatabase::isSelectQuery(const QString& sQuery)
{
    return sQuery.mid(0, 7).toLower() == QLatin1String("select ");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SQLiteFileOpenDialog
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

::DFG_MODULE_NS(sql)::SQLiteFileOpenDialog::SQLiteFileOpenDialog(const QString& sFilePath, QWidget* pParent)
    : BaseClass(pParent)
    , m_sFilePath(sFilePath)
{
    QString sQuery;
    m_spLayout.reset(new QVBoxLayout(this));
    auto& layout = *m_spLayout;
    m_spTableView.reset(new QListView(this));
    QListView& tableList = *m_spTableView;
    m_spColumnView.reset(new QListView(this));
    m_spColumnListLabel.reset(new QLabel(this));

    // Getting table names from file.
    m_tableNames = SQLiteDatabase::getSQLiteFileTableNames(m_sFilePath);

    m_spTableNameModel.reset(new QStringListModel(m_tableNames, this));
    tableList.setModel(m_spTableNameModel.get());
    tableList.setEditTriggers(QListView::NoEditTriggers);

    m_spColumnView->setEditTriggers(QListView::NoEditTriggers);
    m_spColumnView->setSelectionMode(QAbstractItemView::MultiSelection);

    DFG_QT_VERIFY_CONNECT(connect(tableList.selectionModel(), &QItemSelectionModel::selectionChanged, this, &SQLiteFileOpenDialog::tableItemChanged));

    m_spQueryLineEdit.reset(new QLineEdit(this));

    // Constructing dialog
    {
        this->setWindowTitle(tr("Opening SQLite database"));
        ::DFG_MODULE_NS(qt)::removeContextHelpButtonFromDialog(this);

        layout.addWidget(new QLabel(tr("Database\n'%1'\nhas the following tables:").arg(m_sFilePath), this));

        layout.addWidget(&tableList);

        layout.addWidget(m_spColumnListLabel.get());
        layout.addWidget(m_spColumnView.get());

        layout.addWidget(new QLabel(tr("Select query whose result is populated to table:"), this));
        layout.addWidget(m_spQueryLineEdit.get());

        auto& rButtonBox = *(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this));
        DFG_QT_VERIFY_CONNECT(QObject::connect(&rButtonBox, SIGNAL(accepted()), this, SLOT(accept())));
        DFG_QT_VERIFY_CONNECT(QObject::connect(&rButtonBox, SIGNAL(rejected()), this, SLOT(reject())));
        layout.addWidget(&rButtonBox);
    }
    // Selecting first table name
    m_spTableView->setCurrentIndex(m_spTableView->model()->index(0, 0));
    tableItemChanged();
}

::DFG_MODULE_NS(sql)::SQLiteFileOpenDialog::~SQLiteFileOpenDialog() = default;

void ::DFG_MODULE_NS(sql)::SQLiteFileOpenDialog::accept()
{
    const QString sQuery = query();
    const bool bIsSelectQuery = SQLiteDatabase::isSelectQuery(sQuery);
    if (!bIsSelectQuery)
    {
        if (!m_spErrorLine)
        {
            m_spErrorLine.reset(new QLabel);
            m_spErrorLine->setText(tr("<font color=\"#ff0000\">Query is required to start with 'select ' (case insensitive)</font>"));
            if (m_spLayout)
                m_spLayout->insertWidget(m_spLayout->count() - 1, m_spErrorLine.get());
        }
        return;
    }
    BaseClass::accept();
}

auto ::DFG_MODULE_NS(sql)::SQLiteFileOpenDialog::currentTable() const -> QString
{
    const auto nIndex = (m_spTableView) ? m_spTableView->currentIndex().row() : -1;
    return (isValidIndex(m_tableNames, nIndex)) ? m_tableNames[nIndex] : QString();
}

void ::DFG_MODULE_NS(sql)::SQLiteFileOpenDialog::tableItemChanged()
{
    const auto sTable = currentTable();
    if (!m_mapTableToColumnNames.contains(sTable))
        m_mapTableToColumnNames[sTable] = SQLiteDatabase::getSQLiteFileTableColumnNames(m_sFilePath, sTable);

    auto columnNames = m_mapTableToColumnNames.value(sTable);
    if (!m_spColumnNameModel)
    {
        m_spColumnNameModel.reset(new QStringListModel);
        if (m_spColumnView)
        {
            m_spColumnView->setModel(m_spColumnNameModel.get());
            DFG_QT_VERIFY_CONNECT(connect(m_spColumnView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SQLiteFileOpenDialog::columnSelectionChanged));
        }
    }
    m_spColumnNameModel->setStringList(columnNames);
    if (m_spColumnListLabel)
    {
        m_spColumnListLabel->setText(tr("Columns in table %1:").arg(sTable));
        m_spColumnView->selectAll();
    }
}

void ::DFG_MODULE_NS(sql)::SQLiteFileOpenDialog::columnSelectionChanged()
{
    if (!m_spColumnView || !m_spColumnNameModel)
        return;
    const auto rows = m_spColumnView->selectionModel()->selectedRows();
    QStringList columns;
    if (rows.size() != m_spColumnNameModel->rowCount())
    {
        for (const auto mi : rows)
        {
            columns.push_back(mi.data().toString());
        }
    }
    else
        columns << "*";
    updateQuery(currentTable(), columns);
}

void ::DFG_MODULE_NS(sql)::SQLiteFileOpenDialog::updateQuery(const QString& sTable, const QStringList& columns)
{
    if (!m_spQueryLineEdit)
        return;
    const auto sQuery = (!columns.isEmpty()) ? QString("SELECT %1 FROM %2;").arg(columns.join(", "), sTable) : tr("<No columns selected>");
    m_spQueryLineEdit->setText(sQuery);
}

auto ::DFG_MODULE_NS(sql)::SQLiteFileOpenDialog::query() const -> QString
{
    return (m_spQueryLineEdit) ? m_spQueryLineEdit->text() : QString();
}
