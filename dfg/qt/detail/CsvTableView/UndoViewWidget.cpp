#include "UndoViewWidget.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

namespace DFG_DETAIL_NS
{

UndoViewWidget::UndoViewWidget(CsvTableView* pParent)
    : BaseClass(pParent)
    , m_spTableView(pParent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    removeContextHelpButtonFromDialog(this);
    if (!pParent || !pParent->m_spUndoStack)
        return;
    auto pLayout = new QHBoxLayout(this);
    m_spUndoView.reset(new QUndoView(&pParent->m_spUndoStack.get()->item(), this));
    const auto children = m_spUndoView->children();
    m_spUndoView->installEventFilter(this);
    for (auto pChild : children) if (pChild)
        pChild->installEventFilter(this);
    pLayout->addWidget(m_spUndoView.get());
}

UndoViewWidget::~UndoViewWidget() = default;

bool UndoViewWidget::eventFilter(QObject* pObj, QEvent* pEvent)
{
    const auto eventType = (pEvent) ? pEvent->type() : QEvent::None;
    if ((eventType == QEvent::KeyPress || eventType == QEvent::MouseButtonPress) && (m_spTableView && m_spUndoView))
    {
        auto lockReleaser = m_spTableView->tryLockForEdit();
        const auto bEnable = lockReleaser.isLocked();
        m_spUndoView->setEnabled(bEnable);
        if (!bEnable)
            return true; // Returning true means that child doesn't get the event. 
    }
    return BaseClass::eventFilter(pObj, pEvent);
}

} } } // namespace dfg::qt::DFG_DETAIL_NS
