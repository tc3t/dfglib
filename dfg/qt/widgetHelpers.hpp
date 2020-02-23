#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

#include <memory>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QLabel>
    #include <QMenu>
    #include <QString>
    #include <QWidgetAction>
    #include <QWidget>
    #if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        #include <QLocale> // For QLocale::formattedDataSize
    #endif

DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    // At least on Windows default dialogs have ?-icon next to close button. This function removes it.
    inline void removeContextHelpButtonFromDialog(QWidget* pWidget)
    {
        if (!pWidget)
            return;
        pWidget->setWindowFlags(pWidget->windowFlags() & (~Qt::WindowContextHelpButtonHint));
    }

    // Currently at least on Windows this doesn't remove the button, just disables it.
    inline void removeCloseButtonFromDialog(QWidget* pWidget)
    {
        if (!pWidget)
            return;
        pWidget->setWindowFlags(pWidget->windowFlags() & (~Qt::WindowCloseButtonHint));
    }

    // Adds title-entry to a menu.
    inline void addTitleEntryToMenu(QMenu* pMenu, const QString& sTitle)
    {
        if (!pMenu)
            return;
        auto pAction = new QWidgetAction(pMenu); // Parent owned.
        std::unique_ptr<QLabel> spLabel(new QLabel(sTitle));
        auto font = spLabel->font();
        font.setBold(true);
        spLabel->setFont(font);
        spLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        pAction->setDefaultWidget(spLabel.release()); // Owned by pAction
        pMenu->addAction(pAction);
    }

    // Adds section entry to menu. Note that QMenu::addSection() doesn't seem to add any named entry at least in Windows.
    inline void addSectionEntryToMenu(QMenu* pMenu, const QString sName)
    {
        if (!pMenu)
            return;
        pMenu->addSeparator();
        auto pAction = pMenu->addAction(QString("-- %1 --").arg(sName));
        if (pAction)
            pAction->setEnabled(false);
    }

    inline QString formattedDataSize(const qint64 nSizeInBytes, const int nPrecision = 2)
    {
        #if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
            return QLocale(QLocale::English).formattedDataSize(nSizeInBytes, nPrecision, QLocale::DataSizeIecFormat);
        #else
            // Fallback for older Qt's; not a pretty output.
            return QString("%1 B").arg(QString::number(static_cast<double>(nSizeInBytes), 'g', nPrecision));
        #endif
    }
    

}} // Module namespace
