#ifndef SPECTRUMCOLOUREXAMPLE_H
#define SPECTRUMCOLOUREXAMPLE_H

#include <QMainWindow>
#include "ui_spectrumColourExample.h"
#include <dfg/colour/defs.hpp>

class spectrumColourExample : public QMainWindow
{
    Q_OBJECT

public:
    typedef QMainWindow BaseClass;
    spectrumColourExample(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~spectrumColourExample();

    void setColourDisplay(double r, double g, double b);

protected:
    void paintEvent(QPaintEvent* pEvent) override;

    void paintSpectrum(const QWidget& widget, DFG_MODULE_NS(colour)::ColourSystem cs, const bool bGradient = false);
    void paintBlackBodySpectrum(const QWidget& widget, DFG_MODULE_NS(colour)::ColourSystem cs);

public slots:
    void onCurrentWavelengthSelectionChanged(int wl);
    void updateDisplay();

private slots:
    void on_sliderBlackBodyTemperature_valueChanged(int value);

private:
    Ui::spectrumColourExampleClass ui;
};

#endif // SPECTRUMCOLOUREXAMPLE_H
