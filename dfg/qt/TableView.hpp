#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "../Span.hpp"
#include <initializer_list>
#include "../cont/TrivialPair.hpp"
#include "../rangeIterator.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(cont) {

    template <class T>
    class IntervalSet;
} } // dfg::cont


DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QTableView>
#include <QKeyEvent>
#include <QEvent>
#include <QItemSelection>
#include <QPointer>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    namespace DFG_DETAIL_NS
    {
        // Basically an alternative to QItemSelectionRange using integer indexes instead of QModelIndexes
        class ItemSelectionRect
        {
        public:
            using Index = int;
            using IndexPair = ::DFG_MODULE_NS(cont)::TrivialPair<int, int>;

            ItemSelectionRect() = default;
            ItemSelectionRect(const QItemSelectionRange& selectionRange);

            static ItemSelectionRect fromContiguousRowRange(Index col, Index rowTop, Index rowBottom);

            template <class Func_T>
            void forEachRowColPair(Func_T&& func) const
            {
                for (Index c = this->m_left; c <= this->m_right; ++c)
                {
                    for (Index r = this->m_top; r <= this->m_bottom; ++r)
                    {
                        func(r, c);
                    }
                }
            }

            template <class Func_T>
            void forEachModelIndex(const QAbstractItemModel& model, Func_T&& func) const
            {
                forEachRowColPair([&](const Index r, const Index c)
                    {
                        func(model.index(r, c));
                    });
            }

            QItemSelectionRange toQItemSelectionRange(const QAbstractItemModel& model) const;

            Index m_top = -1;
            Index m_left = -1;
            Index m_bottom = -1;
            Index m_right = -1;
        };

        using ItemSelectionRangeList = std::vector<ItemSelectionRect>;

        // Extended and convenienced QItemSelection, i.e. a class for describing selected items
        // Differences to QItemSelection:
        //      -QItemSelection "is basically a list of selection ranges", while ItemSelection abstracts storage details further
        class ItemSelection
        {
        public:
            using Index = ItemSelectionRect::Index;
            using IndexPair = ItemSelectionRect::IndexPair;
            ItemSelection() = default;
            // Constructs from existing QItemSelection
            //      @param pModel If given, rowCol pairs after possible mapping are consired to be from this model. If null, model is deducted from itemSelection
            //      @param indexMapper If given, rowCol pairs in itemSelection are mapped through this and should match rowCol pairs in 'pModel'
            ItemSelection(const QItemSelection& itemSelection, const QAbstractItemModel* pModel = nullptr, std::function<IndexPair(Index, Index)> indexMapper = nullptr);

            template <class Map_T>
            static ItemSelection fromColumnBasedIntervalSetMap(const Map_T& mapColToIntervalSet, const QAbstractItemModel* pModel)
            {
                ItemSelection selection;
                for (const auto& item : mapColToIntervalSet)
                {
                    selection.privAddRowRangeFromIntervalSet(item.first, item.second);
                }
                selection.m_spModel = pModel;
                return selection;
            }

            template <class Func_T>
            void forEachRowColPair(Func_T&& func) const
            {
                for (const auto& range : this->m_selectionRanges)
                {
                    range.forEachRowColPair(func);
                }
            }

            template <class Func_T>
            void forEachModelIndex(const QAbstractItemModel& model, Func_T&& func) const
            {
                for (const auto& range : this->m_selectionRanges)
                {
                    range.forEachModelIndex(model, func);
                }
            }

            const QAbstractItemModel* model() const { return m_spModel; }

            void privAddRowRangeFromIntervalSet(const Index nCol, const ::DFG_MODULE_NS(cont)::IntervalSet<Index>& indexSet);

            QItemSelection toQItemSelection() const;
            QItemSelection toQItemSelection(const QAbstractItemModel& model) const;

            ItemSelectionRangeList m_selectionRanges;
            QPointer<const QAbstractItemModel> m_spModel;
        }; // class ItemSelection
    }

    class TableView : public QTableView
    {
    public:
        typedef TableView ThisClass;
        typedef QTableView BaseClass;
        using ItemSelection = DFG_DETAIL_NS::ItemSelection;
        using Index = ItemSelection::Index;
        using IndexPair = ItemSelection::IndexPair;

        TableView(QWidget* pParent) : BaseClass(pParent)
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

        // Convenience method: clears existing selection and selects cell at (r,c). If index (r, c) does not exist, does nothing.
        bool makeSingleCellSelection(int r, int c);

        // Convenience method: calls makeSingleCellSelection() and scrolls to selection like selectRow()
        void selectCell(int r, int c);

        // Convenience method: clears existing selection and selects given indexes. If given indexes are not from this->model(),
        // indexMapper should be provided that maps indexes to that model (for example if this->model() is proxy model while given indexes are from underlying source model)
        // @note This should not be used for big index lists: for example with list of size 50000,
        //       call has been seen to last over a minute in dfgQtTableEditor debug build (2023-06).
        //       Typically big selections can be represented as a small number of rectangular selections
        //       in case which overload taking ItemSelection can provide massively better performance.
        void setSelectedIndexes(const QModelIndexList& indexes, std::function<QModelIndex(const QModelIndex&)> indexMapper);

        // Sets selection from ItemSelection. Purpose of indexMapper is the same as in other overload, but works with
        // integer indexes instead of QModelIndex'es.
        void setSelectedIndexes(const ItemSelection& selection, std::function<IndexPair(Index, Index)> indexMapper);

        // Convenience overload for setting selected indexes with index pairs instead of QModelIndexes
        void setSelectedIndexes(const Span<const IndexPair> indexes);
        // Convenience overload for setting selected indexes with initalizer list, e.g. setSelectedIndexes({ {1, 2} })
        void setSelectedIndexes(const std::initializer_list<const IndexPair>& indexes);

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
    }; // TableView
}}

inline bool ::DFG_MODULE_NS(qt)::TableView::makeSingleCellSelection(const int r, const int c)
{
    auto pModel = model();
    if (!pModel)
        return false;
    QModelIndexList indexes{ pModel->index(r, c)};
    if (!indexes[0].isValid())
        return false;

    setSelectedIndexes(indexes, nullptr);
    return true;
}

inline void ::DFG_MODULE_NS(qt)::TableView::selectCell(const int r, const int c)
{
    auto pModel = model();
    if (pModel && makeSingleCellSelection(r, c))
        scrollTo(pModel->index(r, c));
}

inline void ::DFG_MODULE_NS(qt)::TableView::setSelectedIndexes(const QModelIndexList& indexes, std::function<QModelIndex (const QModelIndex&)> indexMapper)
{
    QItemSelection selection;
    auto pModel = model();
    if (!pModel)
        return;
    for (const auto& index : indexes)
    {
        if (indexMapper)
        {
            auto mappedIndex = indexMapper(index);
            selection.push_back(QItemSelectionRange(mappedIndex, mappedIndex)); // This is probably redundantly heavy if selection is big and mostly contiguous.
        }
        else
            selection.push_back(QItemSelectionRange(index, index)); // This is probably redundantly heavy if selection is big and mostly contiguous.
            
    }
    auto pSelectionModel = selectionModel();
    if (pSelectionModel)
        pSelectionModel->select(selection, QItemSelectionModel::SelectCurrent); // This can be massively slow if list is big
}

inline void ::DFG_MODULE_NS(qt)::TableView::setSelectedIndexes(const ItemSelection& selection, std::function<IndexPair(Index, Index)> indexMapper)
{
    auto pSelectionModel = selectionModel();
    auto pViewModel = model();
    const auto pSourceModel = selection.model();
    if (!pSelectionModel || !pViewModel || !pSourceModel)
        return;
    if (indexMapper)
    {
        const ItemSelection mappedSelection(selection.toQItemSelection(), pViewModel, indexMapper);
        pSelectionModel->select(mappedSelection.toQItemSelection(), QItemSelectionModel::SelectCurrent);
    }
    else
        pSelectionModel->select(selection.toQItemSelection(), QItemSelectionModel::SelectCurrent);
}

inline void ::DFG_MODULE_NS(qt)::TableView::setSelectedIndexes(const Span<const IndexPair> indexes)
{
    auto pModel = this->model();
    if (!pModel)
        return;
    QModelIndexList modelIndexes;
    modelIndexes.reserve(saturateCast<int>(indexes.size()));
    for (const auto item : indexes)
    {
        modelIndexes.push_back(pModel->index(item.first, item.second));
    }
    setSelectedIndexes(modelIndexes, nullptr);
}

inline void ::DFG_MODULE_NS(qt)::TableView::setSelectedIndexes(const std::initializer_list<const IndexPair>& indexes)
{
    setSelectedIndexes(makeRange(indexes.begin(), indexes.end()));
}
