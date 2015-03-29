#include "CsvItemModelExample.h"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QApplication>
DFG_BEGIN_INCLUDE_QT_HEADERS

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	CsvItemModelExample w;
	w.show();
	return a.exec();
}
