#include <dfg/qt/qtIncludeHelpers.hpp>
#include <dfg/qt/connectHelper.hpp>
#include "main.h"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QApplication>
#include <QDateTime>
#include <QDialog>
#include <QDesktopServices>
#include <QFileInfo>
#include <QMessageBox>
#include <QMenu>
#include <QTimer>
#include <QIcon>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QSettings>
#include <QStyle>
#include <QStyleHints>
#include <QToolButton>
#include <QVBoxLayout>
DFG_END_INCLUDE_QT_HEADERS

#include <dfg/qt/TableEditor.hpp>
#include <dfg/qt/QtApplication.hpp>
#include <dfg/qt/qtBasic.hpp>
#include <dfg/qt/graphTools.hpp>
#include <dfg/qt/CsvTableViewChartDataSource.hpp>
#include <dfg/qt/CsvItemModelChartDataSource.hpp>
#include <dfg/build/buildTimeDetails.hpp>
#include <dfg/debug/structuredExceptionHandling.h>
#include <dfg/qt/CsvTableView.hpp>
#include <dfg/qt/widgetHelpers.hpp>
#include <exception>


#include <QItemSelection>
#include <QItemSelectionRange>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include <dfg/str/fmtlib/format.cc>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

#include <dfg/qt/CsvItemModel.hpp>

static QWidget* gpMainWindow = nullptr;


void DiffUrlHandler::urlHandler(const QUrl& url)
{
    const auto sPath = url.path();
    if (sPath == "/qmake_time_working_tree.diff")
    {
        QDialog dlg;
        auto pLayout = new QVBoxLayout(&dlg);
        auto pTextDisplay = new QPlainTextEdit(&dlg);
        pTextDisplay->setLineWrapMode(QPlainTextEdit::NoWrap);
        pLayout->addWidget(pTextDisplay);
        QFile file(":/qmake_time_working_tree.diff");
        file.open(QFile::ReadOnly);
        const auto sDiff = QString::fromUtf8(file.readAll());
        pTextDisplay->setPlainText(sDiff);
        dlg.resize(800, 600);
        dlg.exec();
    }
    else
        QMessageBox::information(nullptr, tr("Unexpected url"), tr("Diff handler got unexpected URL %1").arg(url.toString()));

}

static DiffUrlHandler diffDisplay;


static void onShowAboutBox()
{
    using namespace DFG_ROOT_NS;

    QString s = QString("%1 version %2<br><br>"
                        "csv-oriented table editor based on dfglib.<br><br>")
                        .arg(QApplication::applicationName(), DFG_QT_TABLE_EDITOR_VERSION_STRING);

    const auto sSettingsPath = dfg::qt::QtApplication::getApplicationSettingsPath();
    const bool bSettingFileExists = QFileInfo::exists(sSettingsPath);

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
            s += QString("%1: %2<br>").arg(DFG_ROOT_NS::buildTimeDetailIdToStr(detailId), QString(psz).toHtmlEscaped());
    });

#ifdef DFGQTE_GIT_BRANCH
    s += QString("Build from git branch: %1<br>").arg(DFGQTE_GIT_BRANCH);
#endif
#ifdef DFGQTE_GIT_COMMIT
    s += QString("Build from git commit: %1").arg(QString("%1").arg(DFGQTE_GIT_COMMIT).mid(0, 8));
    #ifdef DFGQTE_GIT_WORKING_TREE_STATUS
        if (DFGQTE_GIT_WORKING_TREE_STATUS == QLatin1String("unmodified"))
            s += " (no local changes)";
        else
            s += " (with local changes, <a href=dfgdiff:/qmake_time_working_tree.diff>diff</a> is included)";
        s += "<br>";
    #endif // DFGQTE_GIT_WORKING_TREE_STATUS
#endif // DFGQTE_GIT_COMMIT


    s += QApplication::tr("<br>Source code: <a href=%1>%1</a>").arg("https://github.com/tc3t/dfglib");
    s += QApplication::tr("<br>3rd party libraries used in this application: ") +
                 "<a href=www.boost.org>Boost</a>"
                 ", <a href=https://github.com/fmtlib/fmt>fmt</a>"
                 ", Qt"
                 ", <a href=https://github.com/nemtrif/utfcpp>UTF8-CPP</a>"
                 ", <a href=https://github.com/beltoforion/muparser>muparser</a>"
             #if (defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1))
                 + QString(", <a href=https://www.qcustomplot.com>%1</a>").arg(::DFG_MODULE_NS(qt)::chartBackendImplementationIdStr().toHtmlEscaped())
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
public:
    void setDefaultGeometry(); // Moves and resizes mainwindow to default position and size.

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

void MainWindow::setDefaultGeometry()
{
    dfg::qt::centreAndResizeToScreen(this, 0.8, 0.8);
}

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
    tableEditor.setResizeWindow(&mainWindow);
    mainWindow.setWindowIcon(QIcon(":/mainWindowIcon.png"));

#if (defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1))
    dfg::qt::GraphControlAndDisplayWidget graphDisplay(true);

    // Setting chart guide
    {
        QFile file(":/chartGuide.html");
        file.open(QFile::ReadOnly);
        const auto sGuide = QString::fromUtf8(file.readAll());
        graphDisplay.setChartGuide(sGuide);
    }

    // Setting data sources to chart display.
    {
        std::unique_ptr<dfg::qt::CsvTableViewChartDataSource> selectionSource(new dfg::qt::CsvTableViewChartDataSource(tableEditor.m_spTableView.get()));
        const auto selectionSourceId = selectionSource->uniqueId();
        graphDisplay.addDataSource(std::move(selectionSource));
        // Setting selection as default source, i.e. when data_source is not specified, ChartObject will query data from selection.
        graphDisplay.setDefaultDataSourceId(selectionSourceId);

        // Adding 'whole table'-source
        {
            DFG_ASSERT(tableEditor.m_spTableView != nullptr);
            if (tableEditor.m_spTableView)
            {
                std::unique_ptr<dfg::qt::CsvItemModelChartDataSource> tableSource(new dfg::qt::CsvItemModelChartDataSource(tableEditor.m_spTableView->csvModel(), "table"));
                graphDisplay.addDataSource(std::move(tableSource));
            }
        }
    }
    if (tableEditor.setGraphDisplay(&graphDisplay) == 0)
    {
        // Getting here means that display is hidden -> disabling chart updates.
        graphDisplay.setChartingEnabledState(false);
    }
    if (tableEditor.m_spTableView)
    {
        tableEditor.m_spTableView->addConfigSavePropertyFetcher([&]() { return std::make_pair(QString("properties/chartControls"), graphDisplay.getChartDefinitionString()); });
        tableEditor.m_spTableView->addConfigSavePropertyFetcher([&]() { return std::make_pair(QString("properties/chartPanelWidth"), QString::number(graphDisplay.width())); });

        tableEditor.m_spTableView->addConfigSavePropertyFetcher([&]() { return std::make_pair(QString("properties/windowWidth"), QString::number(mainWindow.width())); });
        tableEditor.m_spTableView->addConfigSavePropertyFetcher([&]() { return std::make_pair(QString("properties/windowHeight"), QString::number(mainWindow.height())); });

        tableEditor.m_spTableView->addConfigSavePropertyFetcher([&]() { return std::make_pair(QString("properties/windowPosX"), QString::number(mainWindow.pos().x())); });
        tableEditor.m_spTableView->addConfigSavePropertyFetcher([&]() { return std::make_pair(QString("properties/windowPosY"), QString::number(mainWindow.pos().y())); });
    }
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

    // Adding toolbar buttons
    {
        tableEditor.addToolBarSeparator();
        // Adding operations-button to TableEditor toolbar
        {
            auto pButton = new QToolButton(&tableEditor);

            pButton->setToolTip(QApplication::tr("Operations"));
            pButton->setPopupMode(QToolButton::InstantPopup);

            auto pMenu = new QMenu(pButton); // Deletion through parentship
            const auto addMenuItem = [&](const QString& sTitle, const QString sIconPath, std::function<void()> func)
            {
                auto pAct = pMenu->addAction(sTitle);
                if (pAct)
                {
                    DFG_QT_VERIFY_CONNECT(QObject::connect(pAct, &QAction::triggered, &mainWindow, func));
                    if (!sIconPath.isEmpty())
                        pAct->setIcon(QIcon(sIconPath));
                }
            };

            addMenuItem(mainWindow.tr("Maximize window horizontally"),              ":/resources/maximizeHorizontally.png", [&]() { dfg::qt::maximizeHorizontally(&mainWindow); });
            addMenuItem(mainWindow.tr("Maximize window vertically"),                ":/resources/maximizeVertically.png"  , [&]() { dfg::qt::maximizeVertically(&mainWindow); });
            addMenuItem(mainWindow.tr("Reset window to default size and position"), ":/resources/defaultSize.png"         , [&]() { mainWindow.setDefaultGeometry(); });

            pButton->setMenu(pMenu); // Does not transfer ownership
            pButton->setIcon(QIcon(":/resources/actionsButton.png"));

            tableEditor.addToolBarWidget(pButton);
        }

        // Adding about-button to TableEditor toolbar.
        {
            QDesktopServices::setUrlHandler("dfgdiff", &diffDisplay, "urlHandler");
            auto pButton = new QToolButton(&tableEditor);
            pButton->setToolTip(QApplication::tr("Information about the application"));
            auto pStyle = a.style();
            if (pStyle)
                pButton->setIcon(pStyle->standardIcon(QStyle::SP_MessageBoxInformation));
            DFG_QT_VERIFY_CONNECT(QObject::connect(pButton, &QToolButton::clicked, &onShowAboutBox));
            tableEditor.addToolBarWidget(pButton);
        }
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

    mainWindow.setDefaultGeometry();

    while (true)
    {
        // Exceptions should not propagate this far, but better to at least catch them here instead of letting application crash 
        try
        {
            return a.exec();
        }
        catch (const std::exception& e)
        {
            QMessageBox::critical(nullptr, a.tr("Error"), a.tr("An internal error occurred; application will try to continue running, but it might be in inconsistent state so restarting is recommended.\nException details:\n'%1'").arg(e.what()));
        }
        catch (...)
        {
            QMessageBox::critical(nullptr, a.tr("Error"), a.tr("An internal error occurred; application will try to continue running, but it might be in inconsistent state so restarting is recommended."));
        }
    }
}
