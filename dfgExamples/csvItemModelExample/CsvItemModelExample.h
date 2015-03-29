#ifndef CSVITEMMODELEXAMPLE_H
#define CSVITEMMODELEXAMPLE_H

#include <dfg/qt/qtIncludeHelpers.hpp>

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QtWidgets/QMainWindow>
#include "ui_CsvItemModelExample.h"
DFG_END_INCLUDE_QT_HEADERS

class CsvItemModelExample : public QMainWindow
{
	Q_OBJECT

public:
	CsvItemModelExample(QWidget *parent = 0);
	~CsvItemModelExample();

private:
	Ui::CsvItemModelExampleClass ui;
};

#endif // CSVITEMMODELEXAMPLE_H
