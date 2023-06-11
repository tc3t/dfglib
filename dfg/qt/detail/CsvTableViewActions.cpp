#include "../CsvTableViewActions.hpp"
#include "../../str/format_fmt.hpp"
#include "../../str.hpp"

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

    bool SelectionForEachUndoCommand::VisitorParams::setCellString(const StringViewUtf8& svNewData)
    {
        auto& rDataModel = this->dataModel();
        const bool bEdited = rDataModel.setDataNoUndo(m_modelIndex, svNewData);
        this->m_nEditCount += uint32(bEdited);
        return bEdited;
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
        VisitorParams params = { QModelIndex(), *pModel, *pView, StringViewUtf8() };
        looper(params);
        if (pView && pSelectList)
            pView->setSelectedIndexed(*pSelectList, [&](const QModelIndex& index) { return pView->mapToViewModel(index); }); // Selecting the items that were edited by redo.
    }

    class VisitorErrorMessageHandler
    {
    public:
        using MessageGenerator = std::function<QString()>;

        // Handles error and if needed, calls messageGenerator to get needed error message text; messageGenerator is not stored so only needs to be valid during the call.
        // If messageGenerator is not given, prints cell contents as extra info
        void handleError(CsvTableView& rView, const StringViewC svCell, const QModelIndex& index, const size_t nMaxFailureMessageCount, MessageGenerator messageGenerator = MessageGenerator());

        void showErrorMessageIfAvailable(CsvTableView& rView, const size_t nMaxFailureMessageCount);

        size_t m_nFailureCount = 0;
        QString m_sFailureMsgs;

    }; // class VisitorErrorMessageHandler

    void VisitorErrorMessageHandler::handleError(CsvTableView& rView, const StringViewC svCell,const QModelIndex& index, const size_t nMaxFailureMessageCount, MessageGenerator messageGenerator)
    {
        auto& nFailureCount = this->m_nFailureCount;
        ++nFailureCount;
        if (nFailureCount <= nMaxFailureMessageCount)
        {
            auto& sFailureMsgs = this->m_sFailureMsgs;
            if (nFailureCount == 1)
                sFailureMsgs += rView.tr("Operation failed for cells:");
            sFailureMsgs.push_back(rView.tr("\n(%1, %2): \"%3\"")
                .arg(CsvItemModel::internalRowIndexToVisible(index.row()))
                .arg(CsvItemModel::internalColumnIndexToVisible(index.column()))
                .arg((messageGenerator) ? messageGenerator() : ((svCell.size() <= 16) ? untypedViewToQStringAsUtf8(svCell) : QString("%1...").arg(untypedViewToQStringAsUtf8(svCell.substr_startCount(0, 16))))));
        }
    }

    void VisitorErrorMessageHandler::showErrorMessageIfAvailable(CsvTableView& rView, const size_t nMaxFailureMessageCount)
    {
        const auto nFailureCount = this->m_nFailureCount;
        auto& sFailureMsgs = this->m_sFailureMsgs;
        if (nFailureCount > nMaxFailureMessageCount)
            sFailureMsgs += rView.tr("\n+ %1 failure(s)").arg(nFailureCount - nMaxFailureMessageCount);
        if (!sFailureMsgs.isEmpty())
            rView.showStatusInfoTip(sFailureMsgs);
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
    DFG_DETAIL_NS::VisitorErrorMessageHandler m_errorMessageHandler;
};

void CsvTableViewActionEvaluateSelectionAsFormula::FormulaVisitor::handleCell(VisitorParams& params)
{
    const auto svCell = params.stringView().asUntypedView();
    auto sv = svCell;
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
        DFG_OPAQUE_REF().m_errorMessageHandler.handleError(params.view(), svCell, index, maxFailureMessageCount());
}

void CsvTableViewActionEvaluateSelectionAsFormula::FormulaVisitor::onForEachLoopDone(VisitorParams& params)
{
    DFG_OPAQUE_REF().m_errorMessageHandler.showErrorMessageIfAvailable(params.view(), maxFailureMessageCount());
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
    DFG_DETAIL_NS::VisitorErrorMessageHandler m_errorMessageHandler;
    char szBuffer[96] = "";
    Params m_radixParams;
};

CsvTableViewActionChangeRadix::RadixChangeVisitor::RadixChangeVisitor(Params params)
{
    DFG_OPAQUE_REF().m_radixParams = params;
}

void CsvTableViewActionChangeRadix::RadixChangeVisitor::handleCell(VisitorParams& params)
{
    const auto svCell = params.stringView().asUntypedView();
    if (svCell.empty())
        return; // Skipping empty strings.

    auto sv = svCell;
    using namespace ::DFG_MODULE_NS(str);
    auto& rOpaq = DFG_OPAQUE_REF();
    const auto radixParams = rOpaq.m_radixParams;
    const auto index = params.index();

    if (!radixParams.isValid())
    {
        rOpaq.m_errorMessageHandler.handleError(params.view(), svCell, index, maxFailureMessageCount(), [&]()
            {
                return params.view().tr("Invalid radix params: %1 -> %2").arg(radixParams.fromRadix).arg(radixParams.toRadix);
            });
        return;
    }

    // Skipping prefix if defined
    if (!radixParams.ignorePrefix.empty())
    {
        if (beginsWith(sv, StringViewC(radixParams.ignorePrefix.rawStorage())))
            sv = sv.substr_start(radixParams.ignorePrefix.size());
    }
    // Skipping suffix if defined
    if (!radixParams.ignoreSuffix.empty())
    {
        if (endsWith(sv, StringViewC(radixParams.ignoreSuffix.rawStorage())))
            sv.cutTail_byCutCount(radixParams.ignoreSuffix.size());
    }

    const auto nFromRadix = radixParams.fromRadix;
    bool bOk;
#if DFG_STRTO_RADIX_SUPPORT == 1
    const auto nSrcVal = strTo<int64>(sv, { NumberRadix(nFromRadix), &bOk });
#else // Case: in older compilers fallbacking to conversion using QString
    const auto nSrcVal = untypedViewToQStringAsUtf8(sv).toLongLong(&bOk, nFromRadix);
#endif

    auto& szBuffer = rOpaq.szBuffer;
    DFG_DETAIL_NS::VisitorErrorMessageHandler::MessageGenerator messageGenerator;
    if (bOk)
    {
        const auto nToRadix = radixParams.toRadix;
        if (nToRadix != radixParams.resultDigits.size())
            toStr(nSrcVal, szBuffer, nToRadix);
        else // Case: itoa with non-default digits.
        {
            QChar chars[128];
            const auto rv = intToRadixRepresentation(nSrcVal, nToRadix, [&](const size_t i) { return radixParams.resultDigits[saturateCast<unsigned int>(i)]; }, '-', std::begin(chars), std::end(chars));
            if (rv > 0 && static_cast<size_t>(rv) < DFG_COUNTOF(szBuffer))
            {
                QString s(chars, rv);
                strCpyAllThatFit(szBuffer, s.toUtf8().data());
            }
            else
            {
                szBuffer[0] = '\0';
                bOk = false;
                messageGenerator = [&]() { return params.view().tr("Failed: intToRadixRepresentation() returned %1, buffer size = %2").arg(rv).arg(DFG_COUNTOF(szBuffer)); };
            }
        }
    }
    else
        szBuffer[0] = '\0'; // Clearing output if conversion failed.
    auto& rModel = params.dataModel();
    if (radixParams.hasResultAdjustments())
        rModel.setDataNoUndo(index, SzPtrUtf8(format_fmt("{}{}{}", radixParams.resultPrefix.rawStorage(), szBuffer, radixParams.resultSuffix.rawStorage()).c_str()));
    else
        rModel.setDataNoUndo(index, SzPtrUtf8(szBuffer));
    if (!bOk)
        rOpaq.m_errorMessageHandler.handleError(params.view(), svCell, index, maxFailureMessageCount(), messageGenerator);
}

void CsvTableViewActionChangeRadix::RadixChangeVisitor::onForEachLoopDone(VisitorParams& params)
{
    DFG_OPAQUE_REF().m_errorMessageHandler.showErrorMessageIfAvailable(params.view(), maxFailureMessageCount());
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

CsvTableViewActionChangeRadixParams::CsvTableViewActionChangeRadixParams(const QVariantMap& params)
{
    const auto getVar = [&](const ParamId id) { return params[paramStringId(id)]; };
    this->fromRadix    = getVar(ParamId::fromRadix).toInt();
    this->toRadix      = getVar(ParamId::toRadix).toInt();
    this->ignorePrefix = qStringToStringUtf8(getVar(ParamId::ignorePrefix).toString());
    this->ignoreSuffix = qStringToStringUtf8(getVar(ParamId::ignoreSuffix).toString());
    this->resultPrefix = qStringToStringUtf8(getVar(ParamId::resultPrefix).toString());
    this->resultSuffix = qStringToStringUtf8(getVar(ParamId::resultSuffix).toString());
    this->resultDigits = getVar(ParamId::resultDigits).toString();
    if (this->toRadix == 0 && !this->resultDigits.isEmpty())
        this->toRadix = saturateCast<int>(this->resultDigits.size());
    if (this->toRadix > 36  && this->toRadix <= 62 && this->resultDigits.isEmpty())
        this->resultDigits = QString::fromUtf8("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", this->toRadix);
}

QString CsvTableViewActionChangeRadixParams::paramStringId(const ParamId id)
{
    switch (id)
    {
        case ParamId::fromRadix   : return "from_radix";
        case ParamId::toRadix     : return "to_radix";
        case ParamId::ignorePrefix: return "ignore_prefix";
        case ParamId::ignoreSuffix: return "ignore_suffix";
        case ParamId::resultPrefix: return "result_prefix";
        case ParamId::resultSuffix: return "result_suffix";
        case ParamId::resultDigits: return "result_digits";
        default: return "BUG";
    }
}

bool CsvTableViewActionChangeRadixParams::isValid() const
{
    const auto isValidFromRadix = [](const int n) { return n >= 2 && n <= 36; };
    return isValidFromRadix(this->fromRadix) && this->toRadix >= 2 && ((this->resultDigits.isEmpty() && this->toRadix <= 62) || (this->toRadix == saturateCast<int>(this->resultDigits.size())));
}

bool CsvTableViewActionChangeRadixParams::hasResultAdjustments() const
{
    return !this->resultPrefix.empty() || !this->resultSuffix.empty();
}

////////////////////////////////////////////////////////////////////////////
///
/// CsvTableViewActionTrimCells
///
////////////////////////////////////////////////////////////////////////////

CsvTableViewActionTrimCells::CsvTableViewActionTrimCells(CsvTableView* pView)
    : BaseClass((pView) ? pView->tr("Trim cells for %1 cell(s)") : QString("bug"), pView)
{

}

auto CsvTableViewActionTrimCells::createVisitorStatic() -> std::unique_ptr<TrimCellVisitor>
{
    return std::unique_ptr<TrimCellVisitor>(new TrimCellVisitor);
}

auto CsvTableViewActionTrimCells::createVisitor() -> std::unique_ptr<Visitor>
{
    return createVisitorStatic();
}

void CsvTableViewActionTrimCells::TrimCellVisitor::handleCell(VisitorParams& params)
{
    const auto svCell = params.stringView();
    if (svCell.empty())
        return; // Skipping empty strings.

    const auto nOrigSize = svCell.size();
    const StringViewC trimChars(" \t"); // Note: trimming multi base chars codepoints such as non-breaking spaces (https://en.wikipedia.org/wiki/Non-breaking_space) in UTF8 is not currently supported.
    auto sv = svCell.trimmed_tail(trimChars);
    sv.trimFront(trimChars);
    if (nOrigSize == sv.size())
        return; // Nothing was trimmed.
    params.setCellString(sv);
}

void CsvTableViewActionTrimCells::TrimCellVisitor::onForEachLoopDone(VisitorParams& params)
{
    const auto nEditCount = params.editCount();
    DFG_LOG_FMT(params.view().getLogger(), LoggingLevel::info, "Trim cells edited {} cell(s)", nEditCount);
}

}} // namespace dfg::qt
