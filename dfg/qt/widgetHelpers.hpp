#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "connectHelper.hpp"
#include "../math.hpp"
#include "../cont/Vector.hpp"

#include <memory>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QDialog>
    #include <QDialogButtonBox>
    #include <QLabel>
    #include <QMenu>
    #include <QString>
    #include <QTimer>
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
        pMenu->addSeparator();
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

    // From widget list, returns the first object that is of type T, nullptr if not found.
    // Example:
    //      getWidgetByType<QMainWindow>(QApplication::topLevelWidgets());
    //          -Returns first QMainWindow from application's topLevelWidgets()
    template <class T>
    T* getWidgetByType(const QWidgetList& widgetList)
    {
        for (const auto& pWidget : widgetList)
        {
            auto p = qobject_cast<T*>(pWidget);
            if (p)
                return p;
        }
        return nullptr;
    }

    // Adjusts font of given QWidget-like object that has setFont() and font() members that behave like those of QWidget.
    // Note: currently only integer sizes are supported; argument is rounded to closest integer.
    template <class WidgetLike_T>
    inline void adjustWidgetFontProperties(WidgetLike_T* pWidget, const double newPointSize, const int nWeight = -1, const bool bItalic = false)
    {
        if (!pWidget || !::DFG_MODULE_NS(math)::isFinite(newPointSize) || newPointSize < 0.5 || newPointSize > 1000)
            return;
        pWidget->setFont(QFont(pWidget->font().family(), ::DFG_ROOT_NS::round<int>(newPointSize), nWeight, bItalic));
    }

    namespace DFG_DETAIL_NS
    {
        class InfoTipWidget : public QLabel
        {
        public:
            using BaseClass = QLabel;
            
            InfoTipWidget(QWidget* pParent)
                : BaseClass(pParent, Qt::ToolTip)
            {}

        protected:
            void mousePressEvent(QMouseEvent* pEvent) override
            {
                DFG_UNUSED(pEvent);
                this->deleteLater(); // Delete self if clicked.
            }
        };

    } // namespace DFG_DETAILS_NS

    // Informational widget intended to fill the gap between tooltip and messagebox:
    //      -Modal messagebox is often too heavy for simple information message and tooltip too volatile (uncertain how long it shows if it shows at all)
    inline void showInfoTip(const QString& sMsg, QWidget* pParent)
    {
        // Using tooltip-like QLabel as discussed here: https://forum.qt.io/topic/67240/constantly-updating-tooltip-text/6
        auto pToolTip = new DFG_DETAIL_NS::InfoTipWidget(pParent);
        pToolTip->setText(sMsg);
        adjustWidgetFontProperties(pToolTip, 12); // Increasing font size, default is a bit small.
        pToolTip->move(QCursor::pos());
        pToolTip->show();
        // Scheduling self-destruction after 5 seconds.
        QTimer::singleShot(5000, pToolTip, &QWidget::deleteLater);
    }

    // Adds Ok/Cancel button box to dialog and connects it to QDialog::accept/reject
    // Returns created button box, which is child of pDlg.
    inline QDialogButtonBox* addOkCancelButtonBoxToDialog(QDialog* pDlg, QLayout* pLayout)
    {
        if (!pDlg || !pLayout)
            return nullptr;
        auto pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, pDlg);
        DFG_QT_VERIFY_CONNECT(QObject::connect(pButtonBox, &QDialogButtonBox::accepted, pDlg, &QDialog::accept));
        DFG_QT_VERIFY_CONNECT(QObject::connect(pButtonBox, &QDialogButtonBox::rejected, pDlg, &QDialog::reject));
        return pButtonBox;
    }

}} // Module namespace
