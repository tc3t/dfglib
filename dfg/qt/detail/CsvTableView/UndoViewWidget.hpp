#pragma once

#include <QDialog>
#include <QUndoView>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

class CsvTableView;

namespace DFG_DETAIL_NS
{

class UndoViewWidget : public QDialog
{
public:
    typedef QDialog BaseClass;
    UndoViewWidget(CsvTableView* pParent);

    // Hack: Because QUndoView handles undo stack directly, it will bypass CsvTableView read/edit lock when used. To prevent this,
    //       using event filter to check if lock is available before allowing events that possibly trigger undo stack.
    //       While hacky, this should address at least most cases. Window will also be left in disabled state after operation ends,
    //       but can be re-enabled simply be clicking.
    bool eventFilter(QObject* pObj, QEvent* pEvent) override;

    ~UndoViewWidget();

    QPointer<CsvTableView> m_spTableView;
    QObjectStorage<QUndoView> m_spUndoView;
}; // Class UndoViewWidget

} } } // namespace dfg::qt::DFG_DETAIL_NS
