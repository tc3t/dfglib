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
    addEntry(s, ConsoleDisplayEntryType::generic);
}

void ::DFG_MODULE_NS(qt)::ConsoleDisplay::addEntry(const QString& sMsg, const ConsoleDisplayEntryType entryType)
{
    const QString sTimeStamp = QTime::currentTime().toString("hh:mm:ss.zzz");
    const QString sSeparator = QLatin1Literal(": ");
    m_nLengthCounter += static_cast<size_t>(sTimeStamp.length() + sSeparator.length() + sMsg.length());
    if (entryType == ConsoleDisplayEntryType::error)
        appendHtml(QString(R"(<span style="background: #ffaaaa">%1</span>%2%3)").arg(sTimeStamp.toHtmlEscaped(), sSeparator.toHtmlEscaped(), sMsg.toHtmlEscaped()));
    else if (entryType == ConsoleDisplayEntryType::warning)
        appendHtml(QString(R"(<span style="background: #ffff00">%1</span>%2%3)").arg(sTimeStamp.toHtmlEscaped(), sSeparator.toHtmlEscaped(), sMsg.toHtmlEscaped()));
    else
        appendPlainText(sTimeStamp + sSeparator + sMsg);

    // Checking length are limiting if necessary.
    if (lengthInCharacters() > lengthInCharactersLimit())
    {
        auto sNew = this->toPlainText();
        const auto nRemoveCount = sNew.length() / 2;
        sNew.remove(0, nRemoveCount); // Removing first half of the current log.
        m_nLengthCounter = static_cast<size_t>(sNew.length());
        this->setPlainText(sNew);
    }
}
