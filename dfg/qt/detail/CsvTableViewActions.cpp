#include "../CsvTableViewActions.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QMap>
    #include <QTimer>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) {

////////////////////////////////////////////////////////////////////////////
///
/// SelectionForEachUndoCommand
///
////////////////////////////////////////////////////////////////////////////

namespace DFG_DETAIL_NS
{

    auto SelectionForEachUndoCommand::VisitorParams::setIndex(const QModelIndex& index) -> SelectionForEachUndoCommand::VisitorParams&
    {
        m_modelIndex = index;
        this->m_svData = dataModel().rawStringViewAt(index);
        return *this;
    }

    SelectionForEachUndoCommand::SelectionForEachUndoCommand(QString sCommandTitleTemplate, CsvTableView* pView)
        : m_spView(pView)
    {
        auto pModel = (m_spView) ? m_spView->csvModel() : nullptr;
        if (!pModel)
            return;

        auto& rView = *m_spView;
        auto& rModel = *pModel;

        size_t nCellCount = 0;
        rView.forEachCsvModelIndexInSelection([&](const QModelIndex& index, bool& /*rbContinue*/)
            {
                m_cellMemoryUndo.setElement(index.row(), index.column(), rModel.rawStringPtrAt(index));
                ++nCellCount;
                m_initialSelection.push_back(index);
            });

        setText(sCommandTitleTemplate.arg(nCellCount));
    }

    void SelectionForEachUndoCommand::undo()
    {
        auto pModel = (m_spView) ? m_spView->csvModel() : nullptr;
        if (pModel)
            restoreCells(m_cellMemoryUndo, *pModel);
    }

    void SelectionForEachUndoCommand::redo()
    {
        auto spGenerator = this->createVisitor();
        if (!spGenerator)
            return;
        privDirectRedoImpl(this->m_spView, &this->m_initialSelection, [&](VisitorParams& params)
            {
                for (const auto& index : qAsConst(this->m_initialSelection))
                    spGenerator->handleCell(params.setIndex(index));
                spGenerator->onForEachLoopDone(params.setIndex(QModelIndex()));
            });
    }

    void SelectionForEachUndoCommand::privDirectRedoImpl(CsvTableView* pView, const QModelIndexList* pSelectList, std::function<void(VisitorParams&)> looper)
    {
        auto pModel = (pView) ? pView->csvModel() : nullptr;
        if (!pModel)
            return;
        VisitorParams params = { QModelIndex(), *pModel, *pView };
        looper(params);
        if (pView && pSelectList)
            pView->setSelectedIndexed(*pSelectList, [&](const QModelIndex& index) { return pView->mapToViewModel(index); }); // Selecting the items that were edited by redo.
    }

} // namespace DFG_DETAIL_NS


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

////////////////////////////////////////////////////////////////////////////
///
/// CsvTableViewActionEvaluateSelectionAsFormula
///
////////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(CsvTableViewActionEvaluateSelectionAsFormula::FormulaVisitor)
{
    ::DFG_MODULE_NS(math)::FormulaParser parser;
    char szBuffer[32] = "";
    size_t m_nFailureCount = 0;
    QString m_sFailureMsgs;
};

void CsvTableViewActionEvaluateSelectionAsFormula::FormulaVisitor::handleCell(VisitorParams& params)
{
    auto sv = params.stringView().asUntypedView();
    if (sv.empty())
        return; // Skipping empty strings.
    if (!sv.empty() && sv.front() == '=')
        sv.pop_front(); // Skipping leading = if present

    auto& parser = DFG_OPAQUE_REF().parser;
    ::DFG_MODULE_NS(math)::FormulaParser::ReturnStatus evalStatus;

    const auto index = params.index();
    const auto val = (parser.setFormula(sv)) ? parser.evaluateFormulaAsDouble(&evalStatus) : std::numeric_limits<double>::quiet_NaN();
    if (evalStatus)
    {
        auto& rModel = params.dataModel();
        auto& szBuffer = DFG_OPAQUE_REF().szBuffer;
        rModel.setDataNoUndo(index, SzPtrUtf8(::DFG_MODULE_NS(str)::toStr(val, szBuffer)));
    }
    else
    {
        auto& nFailureCount = DFG_OPAQUE_REF().m_nFailureCount;
        const size_t nMaxFailureMessageCount = maxFailureMessageCount();
        ++nFailureCount;
        if (nFailureCount <= nMaxFailureMessageCount)
        {
            auto& rView = params.view();
            auto& sFailureMsgs = DFG_OPAQUE_REF().m_sFailureMsgs;
            if (nFailureCount == 1)
                sFailureMsgs += rView.tr("Failed evaluations:");
            sFailureMsgs.push_back(rView.tr("\ncell(%1, %2): '%3'")
                .arg(CsvItemModel::internalRowIndexToVisible(index.row()))
                .arg(CsvItemModel::internalColumnIndexToVisible(index.column()))
                .arg(untypedViewToQStringAsUtf8(evalStatus.errorString())));
        }
    }
}

void CsvTableViewActionEvaluateSelectionAsFormula::FormulaVisitor::onForEachLoopDone(VisitorParams& params)
{
    // If generator encountered errors, showing a status info widget
    auto& rView = params.view();
    const auto nFailureCount = DFG_OPAQUE_REF().m_nFailureCount;
    const auto nMaxFailureMessageCount = maxFailureMessageCount();
    auto& sFailureMsgs = DFG_OPAQUE_REF().m_sFailureMsgs;
    if (nFailureCount > nMaxFailureMessageCount)
        sFailureMsgs += rView.tr("\n+ %1 failure(s)").arg(nFailureCount - nMaxFailureMessageCount);
    if (!sFailureMsgs.isEmpty())
        rView.showStatusInfoTip(sFailureMsgs);
}

CsvTableViewActionEvaluateSelectionAsFormula::CsvTableViewActionEvaluateSelectionAsFormula(CsvTableView* pView)
    : BaseClass((pView) ? pView->tr("Evaluate %1 cell(s) as formula") : QString("bug"), pView)
{
}

auto CsvTableViewActionEvaluateSelectionAsFormula::createVisitorStatic() -> std::unique_ptr<FormulaVisitor>
{
    return std::unique_ptr<FormulaVisitor>(new FormulaVisitor);
}

auto CsvTableViewActionEvaluateSelectionAsFormula::createVisitor() -> std::unique_ptr<Visitor>
{
    return createVisitorStatic();
}

////////////////////////////////////////////////////////////////////////////
///
/// CsvTableViewActionResizeTable
///
////////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(CsvTableViewActionResizeTable)
{
    QMap<int, QString> m_columnNames;
};

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
            DFG_OPAQUE_REF().m_columnNames[c] = pModel->getHeaderName(c);
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

        for (auto iter = DFG_OPAQUE_REF().m_columnNames.begin(), iterEnd = DFG_OPAQUE_REF().m_columnNames.end(); iter != iterEnd; ++iter)
        {
            if (iter.key() >= nTargetColCount)
                break;
            pModel->setColumnName(iter.key(), iter.value());
        }
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

////////////////////////////////////////////////////////////////////////////
///
/// CsvTableViewActionChangeRadix
///
////////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(CsvTableViewActionChangeRadix)
{
    Params m_params;
};

DFG_OPAQUE_PTR_DEFINE(CsvTableViewActionChangeRadix::RadixChangeVisitor)
{
    char szBuffer[96] = "";
    size_t m_nFailureCount = 0;
    QString m_sFailureMsgs;
    Params m_radixParams;
};

CsvTableViewActionChangeRadix::RadixChangeVisitor::RadixChangeVisitor(Params params)
{
    DFG_OPAQUE_REF().m_radixParams = params;
}

void CsvTableViewActionChangeRadix::RadixChangeVisitor::handleCell(VisitorParams& params)
{
    auto sv = params.stringView().asUntypedView();
    if (sv.empty())
        return; // Skipping empty strings.

    using namespace ::DFG_MODULE_NS(str);
    const auto nFromRadix = DFG_OPAQUE_REF().m_radixParams.fromRadix;
    bool bOk;
#if DFG_STRTO_RADIX_SUPPORT == 1
    const auto nSrcVal = strTo<int64>(sv, { NumberRadix(nFromRadix), &bOk });
#else
    const auto nSrcVal = untypedViewToQStringAsUtf8(sv).toLongLong(&bOk, nFromRadix);
#endif
    auto& szBuffer = DFG_OPAQUE_REF().szBuffer;
    if (bOk)
    {
        const auto nToRadix = DFG_OPAQUE_REF().m_radixParams.toRadix;
        toStr(nSrcVal, szBuffer, nToRadix);
    }
    else
        szBuffer[0] = '\0'; // Clearing output if conversion failed.
    auto& rModel = params.dataModel();
    const auto index = params.index();
    rModel.setDataNoUndo(index, SzPtrUtf8(szBuffer));
    if (!bOk)
    {
        auto& nFailureCount = DFG_OPAQUE_REF().m_nFailureCount;
        ++nFailureCount;
        const size_t nMaxFailureMessageCount = maxFailureMessageCount();
        if (nFailureCount <= nMaxFailureMessageCount)
        {
            auto& sFailureMsgs = DFG_OPAQUE_REF().m_sFailureMsgs;
            auto& rView = params.view();
            if (nFailureCount == 1)
                sFailureMsgs += rView.tr("Failed conversions:");
            sFailureMsgs.push_back(rView.tr("\ncell(%1, %2): %3")
                .arg(CsvItemModel::internalRowIndexToVisible(index.row()))
                .arg(CsvItemModel::internalColumnIndexToVisible(index.column()))
                .arg((sv.size() <= 16) ? untypedViewToQStringAsUtf8(sv) : QString("%1...").arg(untypedViewToQStringAsUtf8(sv.substr_startCount(0, 16)))));
        }
    }
}

void CsvTableViewActionChangeRadix::RadixChangeVisitor::onForEachLoopDone(VisitorParams& params)
{
    auto& rView = params.view();
    const auto nFailureCount = DFG_OPAQUE_REF().m_nFailureCount;
    const auto nMaxFailureMessageCount = maxFailureMessageCount();
    auto& sFailureMsgs = DFG_OPAQUE_REF().m_sFailureMsgs;
    if (nFailureCount > nMaxFailureMessageCount)
        sFailureMsgs += rView.tr("\n+ %1 failure(s)").arg(nFailureCount - nMaxFailureMessageCount);
    if (!sFailureMsgs.isEmpty())
        rView.showStatusInfoTip(sFailureMsgs);
}

CsvTableViewActionChangeRadix::CsvTableViewActionChangeRadix(CsvTableView* pView, Params params)
    : BaseClass((pView) ? pView->tr("Change radix for %1 cell(s)") : QString("bug"), pView)
{
    DFG_OPAQUE_REF().m_params = params;
}

auto CsvTableViewActionChangeRadix::createVisitorStatic(Params params) -> std::unique_ptr<RadixChangeVisitor>
{
    return (params.isValid()) ? std::unique_ptr<RadixChangeVisitor>(new RadixChangeVisitor(params)) : nullptr;
}

auto CsvTableViewActionChangeRadix::createVisitor() -> std::unique_ptr<Visitor>
{
    return createVisitorStatic(DFG_OPAQUE_REF().m_params);
}

bool CsvTableViewActionChangeRadix::Params::isValid() const
{
    const auto isValidRadix = [](const int n) { return n >= 2 && n <= 36; };
    return isValidRadix(this->fromRadix) && isValidRadix(this->toRadix);
}

}} // namespace dfg::qt
