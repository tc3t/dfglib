#include "../CsvTableViewActions.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QTimer>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) {

CsvTableViewActionFill::CsvTableViewActionFill(CsvTableView* pView, const QString& sFill, const QString& sOperationUiName)
    : m_spView(pView)
    , m_sFill(qStringToStringUtf8(sFill))
    , m_sOperationUiName(sOperationUiName)
{
    auto pModel = (pView) ? pView->csvModel() : nullptr;
    if (!pModel)
        return;
    pView->forEachIndexInSelection(*pView, CsvTableView::ModelIndexTypeSource, [&](const QModelIndex& index, bool& bContinue)
    {
        DFG_UNUSED(bContinue);
        m_cellMemoryRedo.setElement(index.row(), index.column(), DFG_UTF8(""));
        m_cellMemoryUndo.setElement(index.row(), index.column(), pModel->rawStringViewAt(index.row(), index.column()));
        ++m_nCellCount;
    });
    setText(m_sOperationUiName + pView->tr(", %1 cell(s)").arg(m_nCellCount));
}

void CsvTableViewActionFill::undo()
{
    auto pModel = (m_spView) ? m_spView->csvModel() : nullptr;
    if (!pModel)
        return;
    if (m_nCellCount == 1)
        DFG_DETAIL_NS::restoreCells(m_cellMemoryUndo, *pModel);
    else
    {
        bool bIsCancelled = false;
        m_spView->doModalOperation(m_spView->tr("Undo '%1' in progress...").arg(m_sOperationUiName), ProgressWidget::IsCancellable::yes, "undo_action_fill", [&](ProgressWidget* pWidget)
            {
                DFG_DETAIL_NS::restoreCells(m_cellMemoryUndo, *pModel, SzPtrUtf8R(nullptr), [&]()
                    {
                        if (pWidget)
                            bIsCancelled = pWidget->isCancelled();
                        return bIsCancelled;
                    });
            });
        m_spView->invalidateSortFilterProxyModel(); // Without this call fill result might not show in UI until triggering manual draw e.g. by scrolling.
        if (bIsCancelled)
        {
            // redoing so that won't leave half-way result, i.e. that cancelled undo is as if not having undoed at all.
            QTimer::singleShot(0, m_spView.data(), &CsvTableView::redo);
        }
    }
}

void CsvTableViewActionFill::redo()
{
    auto pModel = (m_spView) ? m_spView->csvModel() : nullptr;
    if (!pModel)
        return;
    if (m_nCellCount == 1) // When having only one selected cell, setting cell directly to avoid fiddling with worker thread. Limit is coarse, there's no need to use worker thread for 2 cells either etc.
        pModel->setDataNoUndo(m_cellMemoryRedo.rowCountByMaxRowIndex() - 1, m_cellMemoryRedo.colCountByMaxColIndex() - 1, m_sFill.c_str());
    else // case: inserting to more than one cell, using batch edit
    {
        bool bIsCancelled = false;
        m_spView->doModalOperation(m_spView->tr("'%1' in progress...").arg(m_sOperationUiName), ProgressWidget::IsCancellable::yes, "redo_action_fill", [&](ProgressWidget* pWidget)
            {
                pModel->setDataByBatch_noUndo(m_cellMemoryRedo, m_sFill.c_str(), [&]()
                    {
                        if (pWidget)
                            bIsCancelled = pWidget->isCancelled();
                        return bIsCancelled;
                    });
            });
        m_spView->invalidateSortFilterProxyModel(); // Without this call fill result might not show in UI until triggering manual draw e.g. by scrolling.
        if (bIsCancelled)
        {
            // Undoing so that won't leave half-way result, i.e. that cancelled redo is as if not having redoed at all. If undo is disabled, this does nothing, i.e. cells has been filled until terminated.
            // Note that using singleShot() instead of calling m_spView->undo() directly as it seemed to mess up undo stack.
            QTimer::singleShot(0, m_spView.data(), &CsvTableView::undo);
        }
    }
}


CsvTableViewActionResizeTable::CsvTableViewActionResizeTable(CsvTableView* pView, const int nNewRowCount, const int nNewColCount)
    : m_spView(pView)
{
    auto pModel = (m_spView) ? m_spView->csvModel() : nullptr;
    if (!pModel)
        return;

    m_nOldRowCount = pModel->rowCount();
    m_nOldColCount = pModel->columnCount();
    m_nNewRowCount = nNewRowCount;
    m_nNewColCount = nNewColCount;

    // If some content got removed due to resize, storing it to memory so that undo can restore them
    {
        for (int r = m_nNewRowCount; r < m_nOldRowCount; ++r)
        {
            for (int c = 0; c < m_nOldColCount; ++c)
                this->m_cellMemory.setElement(r, c, pModel->rawStringViewAt(r, c));
        }
        for (int c = m_nNewColCount; c < m_nOldColCount; ++c)
        {
            for (int r = 0; r < Min(m_nNewRowCount, m_nOldRowCount); ++r) // Min() is used to avoid bottom right corner to be handled twice when both row and column count gets smaller.
                this->m_cellMemory.setElement(r, c, pModel->rawStringViewAt(r, c));
        }
    }

    QString sDesc = m_spView->tr("Resize to (%1, %2)").arg(m_nNewRowCount).arg(m_nNewColCount);
    setText(sDesc);
}

void CsvTableViewActionResizeTable::impl(const int nTargetRowCount, const int nTargetColCount)
{
    auto pModel = (m_spView) ? m_spView->csvModel() : nullptr;
    if (!pModel)
        return;

    const int nCurrentRowCount = pModel->rowCount();
    const int nCurrentColCount = pModel->columnCount();

    // Change row count
    if (nCurrentRowCount >= 0 && nTargetRowCount >= 0)
    {
        const auto nPositiveCount = (nCurrentRowCount < nTargetRowCount) ? nTargetRowCount - nCurrentRowCount : nCurrentRowCount - nTargetRowCount;
        if (nCurrentRowCount < nTargetRowCount)
            pModel->insertRows(nCurrentRowCount, nPositiveCount);
        else
            pModel->removeRows(nCurrentRowCount - nPositiveCount, nPositiveCount);
    }

    // Change column count
    if (nCurrentColCount >= 0 && nTargetColCount >= 0)
    {
        const auto nPositiveCount = (nCurrentColCount < nTargetColCount) ? nTargetColCount - nCurrentColCount : nCurrentColCount - nTargetColCount;
        if (nCurrentColCount < nTargetColCount)
            pModel->insertColumns(nCurrentColCount, nPositiveCount);
        else
            pModel->removeColumns(nCurrentColCount - nPositiveCount, nPositiveCount);
    }
}

void CsvTableViewActionResizeTable::undo()
{
    auto pModel = (m_spView) ? m_spView->csvModel() : nullptr;
    if (!pModel)
        return;
    impl(m_nOldRowCount, m_nOldColCount);
    // Restoring cells if needed
    if (m_nNewRowCount < m_nOldRowCount || m_nNewColCount < m_nOldColCount)
    {
        m_spView->doModalOperation(m_spView->tr("Undoing resize in progress..."), ProgressWidget::IsCancellable::no, "undo_resize", [&](ProgressWidget* /*pWidget*/)
            {
                pModel->setDataByBatch_noUndo(m_cellMemory);
            });
    }
}

void CsvTableViewActionResizeTable::redo()
{
    impl(m_nNewRowCount, m_nNewColCount);
}

}} // namespace dfg::qt
