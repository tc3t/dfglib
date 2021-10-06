#pragma once

#include "qtIncludeHelpers.hpp"
#include "connectHelper.hpp"
#include "../cont/Vector.hpp"
#include "widgetHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QDialog>
    #include <QDialogButtonBox>
    #include <QInputDialog>
    #include <QLabel>
    #include <QListView>
    #include <QStringListModel>
    #include <QVBoxLayout>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt)
{

class InputDialog : public QInputDialog
{
public:
    static auto getItems(QWidget* pParent,
                         const QString& sTitle,
                         const QString& sLabel,
                         const QStringList& items/*,
                         int nCurrent = 0,
                         bool bEditable = false,
                         bool* pOk = nullptr,
                         Qt::WindowFlags flags = Qt::WindowFlags(),
                         Qt::InputMethodHints inputMethodHints = Qt::ImhNone*/) -> ::DFG_MODULE_NS(cont)::Vector<int>
    {
        ::DFG_MODULE_NS(cont)::Vector<int> rv;
        QDialog dlg(pParent);
        dlg.setWindowTitle(sTitle);
        auto pLayout = new QVBoxLayout(&dlg);

        pLayout->addWidget(new QLabel(sLabel, &dlg));

        QListView view(&dlg);
        QStringListModel model(items, &dlg);
        view.setModel(&model);
        view.setSelectionMode(QAbstractItemView::MultiSelection);
        pLayout->addWidget(&view);

        pLayout->addWidget(addOkCancelButtonBoxToDialog(&dlg, pLayout));
        removeContextHelpButtonFromDialog(&dlg);

        dlg.setLayout(pLayout);
        if (dlg.exec() == QDialog::Accepted)
        {
            auto pSm = view.selectionModel();
            if (pSm)
            {
                const auto rows = pSm->selectedRows();
                for (const auto& index : rows)
                {
                    rv.push_back(index.row());
                }

            }
        }
        return rv;
    }
}; // class InputDialog

}} // namespace dfg::qt
