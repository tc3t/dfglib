#include <dfg/qt/qtIncludeHelpers.hpp>
#include <dfg/qt/connectHelper.hpp>

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QStyle>
#include <QToolButton>
DFG_END_INCLUDE_QT_HEADERS

#include <dfg/qt/TableEditor.hpp>
#include <dfg/qt/QtApplication.hpp>

static void onShowAboutBox()
{
    QMessageBox::information(nullptr,
                             QApplication::tr("About dfgQtTableEditor"),
                             QApplication::tr("TODO"));
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    dfg::qt::QtApplication::m_sSettingsPath = a.applicationFilePath() + ".ini";

    dfg::qt::TableEditor tableEditor;
    tableEditor.setAllowApplicationSettingsUsage(true);
    tableEditor.show();

    auto args = a.arguments();

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

    return a.exec();
}
