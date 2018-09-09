#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QTableView>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    class DFG_CLASS_NAME(TableView) : public QTableView
    {
    public:
        typedef DFG_CLASS_NAME(TableView) ThisClass;
        typedef QTableView BaseClass;

        ThisClass(QWidget* pParent) : BaseClass(pParent)
        {

        }

        static void invertSelectionStatic(QTableView* pTable)
        {
            const auto pModel = (pTable) ? pTable->model() : nullptr;
            const auto pSelectionModel = (pTable) ? pTable->selectionModel() : nullptr;
            if (!pSelectionModel || !pModel || pModel->rowCount() == 0 || pModel->columnCount() == 0)
                 return;
            pSelectionModel->select(QItemSelection(pModel->index(0, 0), pModel->index(pModel->rowCount() - 1, pModel->columnCount() - 1)),
                                    QItemSelectionModel::Toggle);
        }

        void invertSelection()
        {
            invertSelectionStatic(this);
        }

        int getSelectedItemCount() const { return getSelectedItemCount(this); }

        static int getSelectedItemCount(const QTableView* view)
        {
            if (!view)
                return 0;
            const auto sm = view->selectionModel();
            const auto selection = (sm) ? sm->selection() : QItemSelection();
            int selectedCount = 0;
            for(auto iter = selection.cbegin(); iter != selection.cend(); ++iter)
                selectedCount += iter->height() * iter->width();
            return selectedCount;
        }
    }; // DFG_CLASS_NAME(TableView)
}}
