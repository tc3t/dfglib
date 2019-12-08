#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QTableView>
#include <QKeyEvent>
#include <QEvent>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    class DFG_CLASS_NAME(TableView) : public QTableView
    {
    public:
        typedef DFG_CLASS_NAME(TableView) ThisClass;
        typedef QTableView BaseClass;

        DFG_CLASS_NAME(TableView)(QWidget* pParent) : BaseClass(pParent)
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

        // Implements ctrl+arrow key moving similar to spreadsheet applications. Behaviour:
        //  -Current cell is non-empty and next cell is empty:
        //      -ctrl only : advances to next non-empty (or end).
        //      -ctrl+shift: does nothing
        //  -Current cell is non-empty and next cell is non-empty:
        //      -ctrl only : advances to next empty (or end). Note: this differs e.g. from LibreOffice behaviour, where in this case selection goes to last non-empty.
        //      -ctrl+shift: selects all non-empty cells from start to last non-empty in move direction.
        //  -Current cell is empty:
        //      -ctrl-only : advances to next non-empty (or end).
        //      -ctrl+shift: selects all empty cells from start to last empty in move direction.
        template <class ToModelIndex_T>
        QModelIndex targetCellOnCtrlArrow(decltype(&QAbstractItemModel::rowCount) mfpTraverseDimCount,
                                          decltype(&QModelIndex::row) mfpTraverseDimAccess,
                                          decltype(&QModelIndex::row) mfpFixDimAccess,
                                          ToModelIndex_T toModelIndex,
                                          const bool bPositiveStep,
                                          const QAbstractItemView::CursorAction cursorAction,
                                          const Qt::KeyboardModifiers modifiers)
        {
            const auto startIndex = this->currentIndex();
            auto pModel = model();
            if (pModel && pModel->rowCount() > 0 && pModel->columnCount() > 0 && startIndex.isValid() && (modifiers & Qt::KeyboardModifier::ControlModifier))
            {
                const auto isEmptyCell = [&](const QModelIndex& index)
                            {
                                return !pModel->data(index).isValid();
                            };
                const auto nFixed = (startIndex.*mfpFixDimAccess)();
                const auto nTraverseLimit = (bPositiveStep) ? (pModel->*mfpTraverseDimCount)(QModelIndex()) : -1;
                auto bStartEmpty = isEmptyCell(startIndex);
                const auto nStep = (bPositiveStep) ? 1 : -1;
                auto nStartIndex = (startIndex.*mfpTraverseDimAccess)();
                const auto bHasShiftModifier = (modifiers & Qt::KeyboardModifier::ShiftModifier) != 0;
                // If next is empty and shift is not pressed, traverse to next non-empty by starting from the following empty cell.
                // Rationale of having shift modifier adjustment is that when selecting, one probably usually wants to select the set of non-empty (or empty) items while in the case of just moving cursor,
                // often might be seeking the next cell to edit.
                if (!bStartEmpty && !bHasShiftModifier && nStartIndex + nStep != nTraverseLimit && isEmptyCell(toModelIndex(*pModel, nStartIndex + nStep, nFixed)))
                {
                    bStartEmpty = true;
                    nStartIndex += nStep;
                }
                // TODO: traversing could be implemented much more efficiently for CsvTableView: every call to isEmptyCell() check involves virtual function call that does binary search.
                //       For example when traversing a column (i.e. up/down arrow) on a table with N rows with no empty cells, ctrl+down on first row is O(N*log(N)), but if searched directly
                //       from TableCsv data structure, it would be O(N).
                for (auto nTraverse = nStartIndex; nTraverse != nTraverseLimit; nTraverse += nStep)
                {
                    if (bStartEmpty != isEmptyCell(toModelIndex(*pModel, nTraverse, nFixed)))
                    {
                        // When having shift modifier, don't extent selection to trailing cell (see rationale above)
                        return (bHasShiftModifier && nTraverse != nStartIndex) ? toModelIndex(*pModel, nTraverse - nStep, nFixed) : toModelIndex(*pModel, nTraverse, nFixed);
                    }
                }
                // Ending up here means that limit (i.e. start or end) has been reached.
                return toModelIndex(*pModel, (bPositiveStep) ? nTraverseLimit - 1 : 0, nFixed);
            }
            else
                return BaseClass::moveCursor(cursorAction, modifiers);
        }

#define DFG_TEMP_IMPL_CTRL_MOVE(MOVE_DIRECTION, TRAVERSE_COUNT, TRAVERSE_INDEX, FIXED_INDEX, STEP_DIR, ROW_ITEM, COLUMN_ITEM) \
    return targetCellOnCtrlArrow(&QAbstractItemModel::TRAVERSE_COUNT, \
                                 &QModelIndex::TRAVERSE_INDEX, \
                                 &QModelIndex::FIXED_INDEX, \
                                 [](QAbstractItemModel& model, const int nTraverse, const int nFix) { return model.index(ROW_ITEM, COLUMN_ITEM); }, \
                                 STEP_DIR, \
                                 QAbstractItemView::MOVE_DIRECTION, \
                                 modifiers)

        // Returns next focussed model index when user presses ctrl+down arrow
        QModelIndex targetCellOnCtrlArrowDown(const Qt::KeyboardModifiers modifiers)
        {
            DFG_TEMP_IMPL_CTRL_MOVE(MoveDown, rowCount, row, column, true, nTraverse, nFix);
        }

        // Returns next focussed model index when user presses ctrl+right arrow
        QModelIndex targetCellOnCtrlArrowRight(const Qt::KeyboardModifiers modifiers)
        {
            DFG_TEMP_IMPL_CTRL_MOVE(MoveRight, columnCount, column, row, true, nFix, nTraverse);
        }

        // Returns next focussed model index when user presses ctrl+up arrow
        QModelIndex targetCellOnCtrlArrowUp(const Qt::KeyboardModifiers modifiers)
        {
            DFG_TEMP_IMPL_CTRL_MOVE(MoveUp, rowCount, row, column, false, nTraverse, nFix);
        }

        // Returns next focussed model index when user presses ctrl+left arrow
        QModelIndex targetCellOnCtrlArrowLeft(const Qt::KeyboardModifiers modifiers)
        {
            DFG_TEMP_IMPL_CTRL_MOVE(MoveLeft, columnCount, column, row, false, nFix, nTraverse);
        }

#undef DFG_TEMP_IMPL_CTRL_MOVE

    protected:
        QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override
        {
            if (cursorAction == QAbstractItemView::MoveDown)
                return targetCellOnCtrlArrowDown(modifiers);
            else if (cursorAction == QAbstractItemView::MoveRight)
                return targetCellOnCtrlArrowRight(modifiers);
            else if (cursorAction == QAbstractItemView::MoveUp)
                return targetCellOnCtrlArrowUp(modifiers);
            else if (cursorAction == QAbstractItemView::MoveLeft)
                return targetCellOnCtrlArrowLeft(modifiers);
            else
                return BaseClass::moveCursor(cursorAction, modifiers);
        }

        QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex& index, const QEvent* event) const override
        {
            // Default behaviour for QTableView with ctrl+arrow key is to move focus but keep selection. Since a different behaviour is wanted, manually
            // customizing for that case.
            if (event && event->type() == QEvent::KeyPress)
            {
                auto pKeyEvent = static_cast<const QKeyEvent*>(event);
                if (pKeyEvent)
                {
                    const auto modifiers = pKeyEvent->modifiers();
                    const auto bHasCtrl = (modifiers & Qt::ControlModifier);
                    if (bHasCtrl)
                    {
                        const auto bHasShift = (modifiers & Qt::ShiftModifier);
                        return (bHasShift) ? QItemSelectionModel::SelectCurrent : QItemSelectionModel::ClearAndSelect;
                    }
                }
            }
            return BaseClass::selectionCommand(index, event);
        }
    }; // DFG_CLASS_NAME(TableView)
}}
