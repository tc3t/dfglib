#include <dfg/qt/qtIncludeHelpers.hpp>

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QApplication>
#include <QTimer>
DFG_END_INCLUDE_QT_HEADERS

#include <dfg/qt/TableEditor.hpp>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    dfg::qt::TableEditor tableEditor;
    tableEditor.show();
    auto args = a.arguments();

    if (args.size() >= 2)
        QTimer::singleShot(1, &tableEditor, [&]() { tableEditor.tryOpenFileFromPath(args[1]); });
    return a.exec();
}
