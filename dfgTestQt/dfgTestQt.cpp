//#include <stdafx.h>
#include <dfg/qt/CsvItemModel.hpp>
#include <dfg/io.hpp>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
#include <gtest/gtest.h>
#include <QFileInfo>
#include <QDialog>
#include <QSpinBox>
#include <QGridLayout>
#include <dfg/qt/qxt/gui/qxtspanslider.h>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

TEST(dfgQt, CsvItemModel)
{

	for (size_t i = 0;; ++i)
	{
		const QString sInputPath = QString("testfiles/example%1.csv").arg(i);
		const QString sOutputPath = QString("testfiles/generated/example%1.testOutput.csv").arg(i);
		if (!QFileInfo(sInputPath).exists())
			break;

		DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel) model;

		DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::LoadOptions loadOptions;
		if (i == 3) // Example 1 needs non-native eol-settings.
			loadOptions.separatorChar('\t');

		const auto bOpenSuccess = model.openFile(sInputPath, loadOptions);
		EXPECT_EQ(true, bOpenSuccess);

		DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::SaveOptions saveOptions;
		if (i == 1) // Example 1 needs non-native eol-settings.
			saveOptions.eolType(DFG_MODULE_NS(io)::EndOfLineTypeN);
		else if (i == 3)
			saveOptions = loadOptions;

		// Note: EOL-setting does not affect in-cell EOL-items, i.e. saving of eol-chars inside cell content
		// is not affected by eol-setting.
		const auto bSaveSuccess = model.saveToFile(sOutputPath, saveOptions);
		EXPECT_EQ(true, bSaveSuccess);

		const auto inputBytes = DFG_MODULE_NS(io)::fileToVector(sInputPath.toLatin1().data());
		const auto outputBytes = DFG_MODULE_NS(io)::fileToVector(sOutputPath.toLatin1().data());
		EXPECT_EQ(inputBytes, outputBytes);

	}
	
}

TEST(dfgQt, SpanSlider)
{
	// Simply test that this compiles and links.
	auto pSpanSlider = std::unique_ptr<QxtSpanSlider>(new QxtSpanSlider(Qt::Horizontal));
#if 0 // Enable to test in a GUI.
	auto pLayOut = new QGridLayout();
	auto pEditMinVal = new QSpinBox;
	auto pEditMaxVal = new QSpinBox;

	QObject::connect(pSpanSlider.get(), SIGNAL(lowerValueChanged(int)), pEditMinVal, SLOT(setValue(int)));
	QObject::connect(pSpanSlider.get(), SIGNAL(upperValueChanged(int)), pEditMaxVal, SLOT(setValue(int)));

	pLayOut->addWidget(pSpanSlider.release(), 0, 0, 1, 2); // Takes ownership
	pLayOut->addWidget(pEditMinVal, 1, 0, 1, 1); // Takes ownership
	pLayOut->addWidget(pEditMaxVal, 1, 1, 1, 1); // Takes ownership
	QDialog w;
	w.setLayout(pLayOut); // Takes ownership
	w.exec();
#endif
}
