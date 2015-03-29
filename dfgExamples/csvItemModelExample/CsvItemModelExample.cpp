#include <dfg/buildConfig.hpp> // To get rid of C4996 "Function call with parameters that may be unsafe" in MSVC.
#include "CsvItemModelExample.h"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QGridLayout>
#include <QTableView>
DFG_END_INCLUDE_QT_HEADERS

#include <memory>
#include <dfg/qt/CsvItemModel.hpp>
#include <dfg/qt/CsvTableView.hpp>

CsvItemModelExample::CsvItemModelExample(QWidget *parent)
	: QMainWindow(parent)
{	ui.setupUi(this);

	delete ui.centralWidget->layout(); // Delete existing layout.
	auto pLayout = new QGridLayout(ui.centralWidget); // Set layout automatically through parenthood and is owned by parent.
	auto spTableView = std::unique_ptr<DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableView)>(new DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableView)(this));
	auto spTableModel = std::unique_ptr<DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)>(new DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)());
	spTableModel->openFile("example0.csv");
	spTableView->setModel(spTableModel.release());
	pLayout->addWidget(spTableView.release());
}

CsvItemModelExample::~CsvItemModelExample()
{

}
