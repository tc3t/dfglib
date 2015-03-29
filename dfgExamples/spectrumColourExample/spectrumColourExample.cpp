#include "stdafx.h"
#include "spectrumColourExample.h"
#include <dfg/colour.hpp>
#include <dfg/physics.hpp>
#include <dfg/colour/specRendJw.cpp>

spectrumColourExample::spectrumColourExample(QWidget *parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	const int minBlackbodyT = 500;
	const int maxBlackbodyT = 10000;
	ui.spinBoxBlackBodyTemperatureDisplay->setRange(minBlackbodyT, maxBlackbodyT);
	ui.sliderBlackBodyTemperature->setRange(minBlackbodyT, maxBlackbodyT);

	connect(ui.spinBoxCurrentWavelength, SIGNAL(valueChanged(int)), this, SLOT(onCurrentWavelengthSelectionChanged(int)));
	connect(ui.checkBoxGammaCorrection, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));

	onCurrentWavelengthSelectionChanged(380);
}

spectrumColourExample::~spectrumColourExample()
{

}

QString CreateRgbString(double r, double g, double b)
{
	return QString("RGB: %1 %2 %3").arg(r, 0, 'g', 3).arg(g, 0, 'g', 3).arg(b, 0, 'g', 3);
}

void spectrumColourExample::paintSpectrum(const QWidget& widget, DFG_MODULE_NS(colour)::ColourSystem cs, const bool bGradient)
{
	using namespace DFG_MODULE_NS(colour);
	QPainter painter(this);
	auto rect = widget.rect();
	rect.moveTo(widget.pos());
	const bool bGammaCorrect = ui.checkBoxGammaCorrection->isChecked();
	
	DFG_ASSERT(rect.width() == 401);
	for (int i = 0; i < rect.width(); ++i)
	{
		const auto rgbD = (bGradient) ? DFG_CLASS_NAME(SpectrumColour)::wavelengthInNmToRgbDSimpleGradient_Experimental(380 + i)
							: DFG_CLASS_NAME(SpectrumColour)::wavelengthInNmToRgbD(380 + i, cs, bGammaCorrect);
		painter.setPen(QColor::fromRgbF(rgbD.r, rgbD.g, rgbD.b));
		// TODO: y-position seems to be off by toolbar height, fix.
		painter.drawLine(rect.left() + i, rect.top(), rect.left() + i, rect.bottom());
	}
}

void spectrumColourExample::paintBlackBodySpectrum(const QWidget& widget, DFG_MODULE_NS(colour)::ColourSystem cs)
{
	using namespace DFG_MODULE_NS(colour);

	QPainter painter(this);
	auto rect = widget.rect();
	rect.moveTo(widget.pos());
	const bool bGammaCorrect = ui.checkBoxGammaCorrection->isChecked();
	const auto sliderValueRange = ui.sliderBlackBodyTemperature->maximum() - ui.sliderBlackBodyTemperature->minimum();
	for (int i = 0; i < rect.width(); ++i)
	{
		const auto temperatureInK = 500 + i * (sliderValueRange) / (rect.width() - 1);
		const auto spectrumFunc = [&](const double wlInNm)
		{
		
			return DFG_MODULE_NS(physics)::planckBlackBodySpectralEnergyDensityByWaveLength_RawSi(temperatureInK, wlInNm*1e-9);
		};
		const auto rgbD = DFG_CLASS_NAME(SpectrumColour)::spectrumToRgbDSimpleNm(spectrumFunc, ColourSystemSMPTEsystemSrjw, bGammaCorrect);
		painter.setPen(QColor::fromRgbF(rgbD.r, rgbD.g, rgbD.b));
		painter.drawLine(rect.left() + i, rect.top(), rect.left() + i, rect.bottom());
	}
}

void spectrumColourExample::paintEvent(QPaintEvent* pEvent)
{
	using namespace DFG_MODULE_NS(colour);
	BaseClass::paintEvent(pEvent);
	
	paintSpectrum(*ui.spectrumNTSC, ColourSystemNTSCsystemSrjw);
	paintSpectrum(*ui.spectrumEBU, ColourSystemEBUsystemSrjw);
	paintSpectrum(*ui.spectrumSMPTE, ColourSystemSMPTEsystemSrjw);
	paintSpectrum(*ui.spectrumHDTV, ColourSystemHDTVsystemSrjw);
	paintSpectrum(*ui.spectrumCIE, ColourSystemCIEsystemSrjw);
	paintSpectrum(*ui.spectrumRec709, ColourSystemRec709systemSrjw);
	paintSpectrum(*ui.spectrumGradient, ColourSystemRec709systemSrjw, true);

	paintBlackBodySpectrum(*ui.colourDisplay, ColourSystemNTSCsystemSrjw);
}

void spectrumColourExample::setColourDisplay(double r, double g, double b)
{
	QColor colour;
	colour.setRgbF(r, g, b);

	QPalette palette(colour);
	ui.displayCurrentBlackbodyColour->setBackgroundRole(QPalette::Window);
	ui.displayCurrentBlackbodyColour->setPalette(palette);

	ui.labelRgb->setText(CreateRgbString(r,b,g));
}

void spectrumColourExample::on_sliderBlackBodyTemperature_valueChanged(int value)
{
	using namespace DFG_MODULE_NS(colour);

	const auto spectrumFunc = [&](const double wlInNm)
	{
		return DFG_MODULE_NS(physics)::planckBlackBodySpectralEnergyDensityByWaveLength_RawSi(value, wlInNm*1e-9);
	};

	const auto rgb = DFG_CLASS_NAME(SpectrumColour)::spectrumToRgbDSimpleNm(spectrumFunc, ColourSystemSMPTEsystemSrjw);

	setColourDisplay(rgb.r, rgb.g, rgb.b);

	ui.spinBoxBlackBodyTemperatureDisplay->setValue(value);
}

namespace
{
	void updateCurrentColorDisplay(QWidget& widget, QLabel& rgbLabel, const DFG_MODULE_NS(colour)::ColourSystem cs, const double wl, const bool bGammaCorrect, const bool bGradient = false)
	{
		const auto rgbD = (bGradient) ? 
							DFG_MODULE_NS(colour)::DFG_CLASS_NAME(SpectrumColour)::wavelengthInNmToRgbDSimpleGradient_Experimental(wl)
							: DFG_MODULE_NS(colour)::DFG_CLASS_NAME(SpectrumColour)::wavelengthInNmToRgbD(wl, cs, bGammaCorrect);
		QColor colour;
		colour.setRgbF(rgbD.r, rgbD.g, rgbD.b);

		QPalette palette(colour);
		widget.setAutoFillBackground(true);
		widget.setBackgroundRole(QPalette::Window);
		widget.setPalette(palette);

		rgbLabel.setText(CreateRgbString(rgbD.r, rgbD.g, rgbD.b));

	}
}

void spectrumColourExample::onCurrentWavelengthSelectionChanged(int wl)
{
	using namespace DFG_MODULE_NS(colour);

	const bool bGammaCorrect = ui.checkBoxGammaCorrection->isChecked();
	updateCurrentColorDisplay(*ui.displayCurrentWlColourCIE, *ui.rgbCIE, ColourSystemCIEsystemSrjw, wl, bGammaCorrect);
	updateCurrentColorDisplay(*ui.displayCurrentWlColourEBU, *ui.rgbEBU, ColourSystemEBUsystemSrjw, wl, bGammaCorrect);
	updateCurrentColorDisplay(*ui.displayCurrentWlColourHDTV, *ui.rgbHDTV, ColourSystemHDTVsystemSrjw, wl, bGammaCorrect);
	updateCurrentColorDisplay(*ui.displayCurrentWlColourNTSC, *ui.rgbNTSC, ColourSystemNTSCsystemSrjw, wl, bGammaCorrect);
	updateCurrentColorDisplay(*ui.displayCurrentWlColourRec709, *ui.rgbRec709, ColourSystemRec709systemSrjw, wl, bGammaCorrect);
	updateCurrentColorDisplay(*ui.displayCurrentWlColourSMPTE, *ui.rgbSMPTE, ColourSystemSMPTEsystemSrjw, wl, bGammaCorrect);
	updateCurrentColorDisplay(*ui.displayCurrentWlColourGradient, *ui.rgbGradient, ColourSystemSMPTEsystemSrjw, wl, bGammaCorrect, true);
}

void spectrumColourExample::updateDisplay()
{
	on_sliderBlackBodyTemperature_valueChanged(ui.sliderBlackBodyTemperature->value());
	onCurrentWavelengthSelectionChanged(ui.sliderCurrentWavelength->value());
	update();
}
