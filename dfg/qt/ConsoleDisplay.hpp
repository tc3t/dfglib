#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QPlainTextEdit>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

// Widget for displaying console-like messages.
class ConsoleDisplay : public QPlainTextEdit
{
public:
    using BaseClass = QPlainTextEdit;

    ConsoleDisplay(QWidget* pParent = nullptr);
    ~ConsoleDisplay();

    size_t lengthLimit() const { return m_nLengthLimit; }
    void lengthLimit(const size_t nNewLimit) { m_nLengthLimit = nNewLimit; }

    // Addes new entry. If after insert total content length is longer than lengthLimit(), cleanup will run.
    void addEntry(const QString& s);

    size_t m_nLengthCounter = 0;
    size_t m_nLengthLimit = 2000000; // 2 MB
};

} } // Module namespace
