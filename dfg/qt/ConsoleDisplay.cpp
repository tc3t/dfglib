#include "ConsoleDisplay.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QTime>
DFG_END_INCLUDE_QT_HEADERS

::DFG_MODULE_NS(qt)::ConsoleDisplay::ConsoleDisplay(QWidget* pParent)
    : BaseClass(pParent)
{
    setReadOnly(true);
}

::DFG_MODULE_NS(qt)::ConsoleDisplay::~ConsoleDisplay()
{
}

void ::DFG_MODULE_NS(qt)::ConsoleDisplay::addEntry(const QString& s)
{
    m_nLengthCounter += static_cast<size_t>(14 + s.length());
    appendPlainText(QString("%1: %2").arg(QTime::currentTime().toString("hh:mm:ss.zzz")).arg(s));

    // Checking length are limiting if necessary.
    if (m_nLengthCounter > lengthLimit())
    {
        auto sNew = this->toPlainText();
        const auto nRemoveCount = sNew.length() / 2;
        sNew.remove(0, nRemoveCount); // Removing first half of the current log.
        m_nLengthCounter = static_cast<size_t>(sNew.length());
        this->setPlainText(sNew);
    }
}
