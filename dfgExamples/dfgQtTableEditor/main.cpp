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
#include <dfg/build/buildTimeDetails.hpp>
#include <dfg/debug/structuredExceptionHandling.h>

#ifdef _WIN32
    DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
        #include <dfg/str/fmtlib/format.cc>
    DFG_END_INCLUDE_WITH_DISABLED_WARNINGS
#endif // _WIN32

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

#ifdef _WIN32
    #define FMT_ABOUT_ENTRY ", <a href=https://github.com/fmtlib/fmt>fmt</a>"
#else
    #define FMT_ABOUT_ENTRY ""
#endif
    s += QString("<br>Source code: <a href=%1>%1</a>").arg("https://github.com/tc3t/dfglib");
    s += QString("<br>3rd party libraries used in this application: <a href=www.boost.org>Boost</a>" FMT_ABOUT_ENTRY ", Qt, <a href=https://github.com/nemtrif/utfcpp>UTF8-CPP</a>");

    QMessageBox::about(gpMainWindow,
                             QApplication::tr("About dfgQtTableEditor"),
                             s);
    QMessageBox::aboutQt(gpMainWindow);
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

    QMainWindow mainWindow;
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

    mainWindow.setCentralWidget(&tableEditor);
    mainWindow.resize(tableEditor.size());
    DFG_QT_VERIFY_CONNECT(QObject::connect(&tableEditor, &QWidget::windowTitleChanged, &mainWindow, &QWidget::setWindowTitle));
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
