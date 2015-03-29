#include "stdafx.h"
#include "spectrumColourExample.h"
#include <dfg/dfgDefs.hpp>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
#include <QApplication>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	spectrumColourExample w;
	w.show();
	return a.exec();
}
