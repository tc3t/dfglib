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
    const QString sSeparator(": ");
    m_nLengthCounter += static_cast<size_t>(sTimeStamp.length() + sSeparator.length()) + static_cast<size_t>(sMsg.length());
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

void ::DFG_MODULE_NS(qt)::ConsoleDisplay::clear()
{
    BaseClass::clear();
    m_nLengthCounter = 0;
}

void ::DFG_MODULE_NS(qt)::ConsoleDisplay::contextMenuEvent(QContextMenuEvent* pEvent)
{
    if (!pEvent)
        return;
    
    // Taking standard context menu...
    std::unique_ptr<QMenu> spMenu(BaseClass::createStandardContextMenu(pEvent->pos())); // Not sure if pos is given in correct units (global vs widget)

    // ...and adding custom entries.
    spMenu->addSeparator();

    // 'Clear'-item
    {
        auto pActClear = spMenu->addAction(tr("Clear"));
        if (pActClear)
            DFG_QT_VERIFY_CONNECT(connect(pActClear, &QAction::triggered, this, &ConsoleDisplay::clear));
    }

    // 'Size indicator'-item
    {
        auto pAct = spMenu->addAction(tr("Current size: %1 (%2 % of limit)").arg(m_nLengthCounter).arg(100.0 * static_cast<double>(m_nLengthCounter) / static_cast<double>(m_nLengthLimit), 0, 'f', 1));
        if (pAct)
            pAct->setEnabled(false);
    }

    // Adding additional actions (if any)
    const auto actions = this->actions();
    if (!actions.isEmpty())
    {
        spMenu->addSeparator();
        spMenu->addActions(actions);
    }

    spMenu->move(this->mapToGlobal(pEvent->pos()));
    spMenu->exec();
}
