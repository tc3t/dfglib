#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QPlainTextEdit>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

enum class ConsoleDisplayEntryType
{
    error,
    warning,
    generic,
};

// Widget for displaying console-like messages.
class ConsoleDisplay : public QPlainTextEdit
{
public:
    using BaseClass = QPlainTextEdit;

    ConsoleDisplay(QWidget* pParent = nullptr);
    ~ConsoleDisplay();

    // Current console display length in characters (codepoints)
    size_t lengthInCharacters() const { return m_nLengthCounter; }

    size_t lengthInCharactersLimit() const { return m_nLengthLimit; }
    void lengthInCharactersLimit(const size_t nNewLimit) { m_nLengthLimit = nNewLimit; }

    // Adds new entry. If after adding total content length is longer than lengthInCharactersLimit(), oldest content will be dropped.
    void addEntry(const QString& s);

    void addEntry(const QString& s, ConsoleDisplayEntryType entryType);

    void clear();

protected:
    void contextMenuEvent(QContextMenuEvent* pEvent) override;

public:
    size_t m_nLengthCounter = 0; // Counts in characters as given by QString::length()
    size_t m_nLengthLimit = 2000000; // 2 M characters
};

} } // Module namespace
