#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QWidget>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    // At least on Windows default dialogs have ?-icon next to close button. This function removes it.
    void removeContextHelpButtonFromDialog(QWidget* pWidget)
    {
        if (!pWidget)
            return;
        pWidget->setWindowFlags(pWidget->windowFlags() & (~Qt::WindowContextHelpButtonHint));
    }

    // Currently at least on Windows this doesn't remove the button, just disables it.
    void removeCloseButtonFromDialog(QWidget* pWidget)
    {
        if (!pWidget)
            return;
        pWidget->setWindowFlags(pWidget->windowFlags() & (~Qt::WindowCloseButtonHint));
    }

}} // Module namespace
