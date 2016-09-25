#include <dfg/qt/qtIncludeHelpers.hpp>

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QApplication>
DFG_END_INCLUDE_QT_HEADERS

#include <dfg/qt/TableEditor.hpp>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    dfg::qt::TableEditor tableEditor;
    tableEditor.show();

    return a.exec();
}
