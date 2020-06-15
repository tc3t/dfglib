#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QPlainTextEdit>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

// Provides UI for viewing and controlling list of (single line) JSON items.
class JsonListWidget : public QPlainTextEdit
{
public:
    using BaseClass = QPlainTextEdit;
    using BaseClass::BaseClass; // Inheriting constructor
}; // Class JsonListWidget

} } // module namespace
