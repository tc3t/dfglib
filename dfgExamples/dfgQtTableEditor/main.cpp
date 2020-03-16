#include <dfg/qt/qtIncludeHelpers.hpp>
#include <dfg/qt/connectHelper.hpp>

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>
#include <QIcon>
#include <QMainWindow>
#include <QSettings>
#include <QStyle>
#include <QStyleHints>
#include <QToolButton>
DFG_END_INCLUDE_QT_HEADERS

#include <dfg/qt/TableEditor.hpp>
#include <dfg/qt/QtApplication.hpp>
#include <dfg/qt/qtBasic.hpp>
#include <dfg/qt/graphTools.hpp>
#include <dfg/build/buildTimeDetails.hpp>
#include <dfg/debug/structuredExceptionHandling.h>
#include <dfg/qt/CsvTableView.hpp>

#include <dfg/cont/ViewableSharedPtr.hpp>
#include <dfg/alg.hpp>
#include <dfg/cont/MapVector.hpp>
#include <dfg/cont/SetVector.hpp>

#include <QItemSelection>
#include <QItemSelectionRange>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include <dfg/str/fmtlib/format.cc>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

#include <dfg/qt/CsvItemModel.hpp>

static QWidget* gpMainWindow = nullptr;


static void onShowAboutBox()
{
    using namespace DFG_ROOT_NS;

    QString s = QString("%1 version %2<br><br>"
                        "Example application demonstrating csv handling features in dfglib.<br><br>")
                        .arg(QApplication::applicationName())
                        .arg(DFG_QT_TABLE_EDITOR_VERSION_STRING);

    const auto sSettingsPath = dfg::qt::QtApplication::getApplicationSettingsPath();
    const bool bSettingFileExists = QFileInfo(sSettingsPath).exists();

    s += QString("<b>Application path</b>: <a href=%1>%2</a><br>"
         "<b>Settings file path</b>: %3<br>"
         "<b>Application PID</b>: %4<br>"
         "<b>Qt runtime version</b>: %5<br>")
            .arg(QApplication::applicationDirPath())
            .arg(QApplication::applicationFilePath())
            .arg((bSettingFileExists) ? QString("<a href=%1>%1</a>").arg(sSettingsPath) : QString("(file doesn't exist)"))
            .arg(QCoreApplication::applicationPid())
            .arg(qVersion());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    s += QString("<b>OS</b>: %1<br><br>").arg(QSysInfo::prettyProductName());
#else
    s += "<br>";
#endif

    s += "Build details:<br><br>";

    getBuildTimeDetailStrs([&](const BuildTimeDetail detailId, const char* psz)
    {
        if (psz && psz[0] != '\0')
            s += QString("%1: %2<br>").arg(DFG_ROOT_NS::buildTimeDetailIdToStr(detailId)).arg(psz);
    });

    s += QApplication::tr("<br>Source code: <a href=%1>%1</a>").arg("https://github.com/tc3t/dfglib");
    s += QApplication::tr("<br>3rd party libraries used in this application: ") +
                 "<a href=www.boost.org>Boost</a>"
                 ", <a href=https://github.com/fmtlib/fmt>fmt</a>"
                 ", Qt"
                 ", <a href=https://github.com/nemtrif/utfcpp>UTF8-CPP</a>"
             #if (defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1))
                 ", Qt Charts"
             #endif
             #if (defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1))
                 ", <a href=https://www.qcustomplot.com>QCustomPlot</a>"
             #endif
            ;

#if defined(DFGQTE_GPL_INFECTED) && DFGQTE_GPL_INFECTED == 1
    s += QApplication::tr("<br><br>Note: This build is infected by GPL due to license requirements in optional 3rd party code.");
#endif

    QMessageBox::about(gpMainWindow,
                             QApplication::tr("About"),
                             s);
    QMessageBox::aboutQt(gpMainWindow);
}

class MainWindow : public QMainWindow
{
    // QWidget interface
protected:
    void closeEvent(QCloseEvent* event) override;
};

void MainWindow::closeEvent(QCloseEvent* event)
{
    auto pTableEditor = findChild<dfg::qt::TableEditor*>();
    DFG_ASSERT_CORRECTNESS(pTableEditor != nullptr);
    if (!pTableEditor || pTableEditor->handleExitConfirmationAndReturnTrueIfCanExit())
        event->accept();
    else
        event->ignore();
}

// Selection analyzer that gathers data from selection for graphs.
class SelectionAnalyzerForGraphing : public dfg::qt::CsvTableViewSelectionAnalyzer
{
public:
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<double, double> RowToValueMap;
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<int, RowToValueMap> ColumnToValuesMap;
    typedef ColumnToValuesMap Table;

    void analyzeImpl(const QItemSelection selection) override
    {
        auto pView = qobject_cast<dfg::qt::CsvTableView*>(this->m_spView.data());
        if (!pView || !m_spTable)
            return;

        m_spTable.edit([&](Table& rTable, const Table* pOld)
        {
            // TODO: this may be storing way too much data: e.g. if all items are selected, graphing may not use any or perhaps only two columns.
            //       -> filter according to actual needs. Note that in order to be able to do so, would need to know something about graph definitions here.
            // TODO: which indexes to use e.g. in case of filtered table: source row indexes or view rows?
            //      -e.g. if table of 1000 rows is filtered so that only rows 100, 600 and 700 are shown, should single column graph use
            //       values 1, 2, 3 or 100, 600, 700 as x-coordinate value? This might be something to define in graph definition -> another reason why
            //       graph definitions should be somehow known at this point.

            DFG_UNUSED(pOld);

            dfg::cont::SetVector<int> presentColumnIndexes;
            presentColumnIndexes.setSorting(true); // Sorting is needed for std::set_difference

            // Clearing all values from every column so that those would actually contain current values, not remnants from previous update.
            // Note that removing unused columns is done after going through the selection.
            dfg::alg::forEachFwd(rTable.valueRange(), [](RowToValueMap& columnValues) { columnValues.clear(); });

            // Sorting is needed for std::set_difference
            rTable.setSorting(true);

            CompletionStatus completionStatus = CompletionStatus_started;

            dfg::qt::CsvTableView::forEachIndexInSelection(*pView, selection, dfg::qt::CsvTableView::ModelIndexTypeView, [&](const QModelIndex& mi, bool& rbContinue)
            {
                if (m_abNewSelectionPending)
                {
                    completionStatus = ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus_terminatedByNewSelection;
                    rbContinue = false;
                    return;
                }
                else if (!m_abIsEnabled.load(std::memory_order_relaxed))
                {
                    completionStatus = ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus_terminatedByDisabling;
                    rbContinue = false;
                    return;
                }

                if (!mi.isValid())
                    return;
                auto sVal = mi.data().toString();
                presentColumnIndexes.insert(mi.column());
                sVal.replace(',', '.'); // Hack: to make comma-localized values such as "1,2" be interpreted as 1.2
                rTable[mi.column()][mi.row()] = sVal.toDouble(); // Note that indexes are view indexes, not source model indexes (e.g. in case of filtered table, row indexes in filtered table)
            });

            // Removing unused columns from rTable.
            std::vector<int> colsToRemove;
            const auto colsInTable = rTable.keyRange();
            std::set_difference(colsInTable.cbegin(), colsInTable.cend(), presentColumnIndexes.cbegin(), presentColumnIndexes.cend(), std::back_inserter(colsToRemove));
            for (auto iter = colsToRemove.cbegin(), iterEnd = colsToRemove.cend(); iter != iterEnd; ++iter)
            {
                rTable.erase(*iter);
            }
        });

        Q_EMIT sigAnalyzeCompleted();
    }

    dfg::cont::ViewableSharedPtr<Table> m_spTable;
};

// TODO: consider moving to dfg/qt instead of being here in dfgQtTableEditor main.cpp
class CsvTableViewDataSource : public dfg::qt::GraphDataSource
{
public:
    CsvTableViewDataSource(dfg::qt::CsvTableView* view)
        : m_spView(view)
    {
        if (!m_spView)
            return;
        m_spSelectionAnalyzer = std::make_shared<SelectionAnalyzerForGraphing>();
        m_spSelectionAnalyzer->m_spTable.reset(std::make_shared<SelectionAnalyzerForGraphing::Table>());
        m_spDataViewer = m_spSelectionAnalyzer->m_spTable.createViewer();
        DFG_QT_VERIFY_CONNECT(connect(m_spSelectionAnalyzer.get(), &dfg::qt::CsvTableViewSelectionAnalyzer::sigAnalyzeCompleted, this, &dfg::qt::GraphDataSource::sigChanged));
        m_spView->addSelectionAnalyzer(m_spSelectionAnalyzer);
    }

    QObject* underlyingSource() override
    {
        return m_spView;
    }

    void forEachElement_fromTableSelection(std::function<void (DataSourceIndex, DataSourceIndex, QVariant)> handler) override
    {
        if (!handler || !m_spView)
            return;

        auto spTableView = (m_spDataViewer) ? m_spDataViewer->view() : nullptr;
        if (!spTableView) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
            return;
        const auto& rTable = *spTableView;

        for(auto iterCol = rTable.begin(); iterCol != rTable.end(); ++iterCol)
        {
            for (auto iterRow = iterCol->second.begin(); iterRow != iterCol->second.end(); ++iterRow)
            {
                handler(static_cast<DataSourceIndex>(iterRow->first), static_cast<DataSourceIndex>(iterCol->first), iterRow->second);
            }
        }
    }

    DataSourceIndex columnCount() const override
    {
        auto spTableView = (m_spDataViewer) ? m_spDataViewer->view() : nullptr;
        return (spTableView) ? spTableView->size() : 0;
    }

    DataSourceIndex columnIndexByName(const dfg::StringViewUtf8 sv) const override
    {
        auto pModel = (m_spView) ? m_spView->csvModel() : nullptr;
        if (!pModel)
            return invalidIndex();
        auto nIndex = pModel->findColumnIndexByName(QString::fromUtf8(sv.beginRaw(), static_cast<int>(sv.size())), -1);
        return (nIndex >= 0) ? static_cast<DataSourceIndex>(nIndex) : invalidIndex();
    }

    SingleColumnDoubleValuesOptional singleColumnDoubleValues_byOffsetFromFirst(const DataSourceIndex offsetFromFirst) override
    {
        auto spTableView = (m_spDataViewer) ? m_spDataViewer->view() : nullptr;
        if (!spTableView) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
            return SingleColumnDoubleValuesOptional();
        const auto& rTable = *spTableView;
        if (rTable.empty())
            return SingleColumnDoubleValuesOptional();
        const DataSourceIndex nFirstCol = rTable.frontKey();
        const auto nTargetCol = nFirstCol + offsetFromFirst;
        auto iter = rTable.find(nTargetCol);
        if (iter == rTable.end())
            return SingleColumnDoubleValuesOptional();

        const auto& values = iter->second.valueRange();
        auto rv = std::make_shared<DoubleValueVector>();
        rv->resize(values.size());
        std::copy(values.cbegin(), values.cend(), rv->begin());
        return rv;
    }

    void enable(const bool b) override
    {
        if (!m_spView)
            return;
        if (b)
        {
            m_spView->addSelectionAnalyzer(m_spSelectionAnalyzer);
            m_spSelectionAnalyzer->addSelectionToQueue(m_spView->getSelection());
        }
        else
            m_spView->removeSelectionAnalyzer(m_spSelectionAnalyzer.get());
    }

    QPointer<dfg::qt::CsvTableView> m_spView;
    std::shared_ptr<dfg::cont::ViewableSharedPtrViewer<SelectionAnalyzerForGraphing::Table>> m_spDataViewer;
    std::shared_ptr<SelectionAnalyzerForGraphing> m_spSelectionAnalyzer;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    dfg::qt::QtApplication::m_sSettingsPath = a.applicationFilePath() + ".ini";
    a.setApplicationVersion(DFG_QT_TABLE_EDITOR_VERSION_STRING);

#ifdef _WIN32
    if (dfg::qt::QtApplication::getApplicationSettings()->value("dfglib/enableAutoDumps", false).toBool())
    {
        // Note: crash dump time stamp is for now taken here, around app start time, rather than from crash time.
        const QString sCrashDumpPath = a.applicationDirPath() + QString("/crashdump_%1.dmp").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmsszzz"));
        dfg::debug::structuredExceptionHandling::enableAutoDumps(dfg::qt::qStringToFileApi8Bit(sCrashDumpPath).c_str());
    }
#endif // _WIN32, structuredExceptionHandling

    MainWindow mainWindow;
    gpMainWindow = &mainWindow;

    // Stubs for creating menu
#if 0
    {
        auto pMenuAbout = mainWindow.menuBar()->addMenu("&About");
        auto pAction = pMenuAbout->addAction("Test entry");
        auto pStyle = QApplication::style();
        if (pStyle)
            pAction->setIcon(pStyle->standardIcon(QStyle::SP_MessageBoxInformation));
    }
#endif

    dfg::qt::TableEditor tableEditor;
    tableEditor.setAllowApplicationSettingsUsage(true);
    mainWindow.setWindowIcon(QIcon(":/mainWindowIcon.png"));

#if (defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1)) || (defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1))
    dfg::qt::GraphControlAndDisplayWidget graphDisplay;
    std::unique_ptr<CsvTableViewDataSource> selectionSource(new CsvTableViewDataSource(tableEditor.m_spTableView.get()));
    graphDisplay.addDataSource(std::move(selectionSource));
    tableEditor.setGraphDisplay(&graphDisplay);
#endif

    mainWindow.setCentralWidget(&tableEditor);
    mainWindow.resize(tableEditor.size());
    DFG_QT_VERIFY_CONNECT(QObject::connect(&tableEditor, &QWidget::windowTitleChanged, &mainWindow, &QWidget::setWindowTitle));
    DFG_QT_VERIFY_CONNECT(QObject::connect(&tableEditor, &dfg::qt::TableEditor::sigModifiedStatusChanged, &mainWindow, &QWidget::setWindowModified));

    // Setting application name that gets shown in window titles (for details, see documentation of windowTitle)
    {
        const auto displayName = QString("%1 %2").arg(a.applicationName(), a.applicationVersion());
        a.setApplicationDisplayName(displayName);
    }

    mainWindow.show();

    auto args = a.arguments();

    // Example how a stylesheet could be set
#if 0
    const auto bytes = dfg::io::fileToVector(TODO_path_to_stylesheet_file);
    a.setStyleSheet(QByteArray(bytes.data(), static_cast<int>(bytes.size())));
#endif

    if (args.size() >= 2)
    {
        #if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
            QTimer::singleShot(1, &tableEditor, [&]() { tableEditor.tryOpenFileFromPath(args[1]); });
        #else
            tableEditor.tryOpenFileFromPath(args[1]);
        #endif
    }

    // Add about-button to TableEditor toolbar.
    {
        auto button = new QToolButton(&tableEditor);
        button->setToolTip(QApplication::tr("Information about the application"));
        auto pStyle = a.style();
        if (pStyle)
            button->setIcon(pStyle->standardIcon(QStyle::SP_MessageBoxInformation));
        DFG_QT_VERIFY_CONNECT(QObject::connect(button, &QToolButton::clicked, &onShowAboutBox));
        tableEditor.addToolBarSeparator();
        tableEditor.addToolBarWidget(button);
    }

    // Set shortcuts visible in context menus in >= 5.13. This is bit of a mess (these notes might be Windows-specific):
    //      -In versions < 5.10 they were shown, no way to control the behaviour.
    //      -In versions 5.10 - 5.12.3 they were not shown and there was no way to use the old behaviour.
    //      -In 5.12.4 they were shown like in versions prior to 5.10 (but still no way to control the behaviour).
    //      -In 5.13.0 they were not shown by default, but a way to enable them was provided.
    // Related issues:
    //      -https://bugreports.qt.io/browse/QTBUG-61181
    //      -https://bugreports.qt.io/browse/QTBUG-71471
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    auto pStyleHints = QGuiApplication::styleHints();
    if (pStyleHints)
    {
        pStyleHints->setShowShortcutsInContextMenus(true);
    }
#endif

    return a.exec();
}
