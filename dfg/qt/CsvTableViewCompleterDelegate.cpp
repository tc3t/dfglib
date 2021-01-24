#include "CsvTableViewCompleterDelegate.hpp"
#include "CsvTableView.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QKeyEvent>
    #include <QCompleter>
    #include <QLineEdit>
DFG_END_INCLUDE_QT_HEADERS


namespace
{
    class LineEditCtrl : public QLineEdit
    {
    public:
        typedef QLineEdit BaseClass;
        LineEditCtrl(QWidget* pParent) : QLineEdit(pParent) {}
        ~LineEditCtrl()
        {

        }
    protected:
        void keyPressEvent(QKeyEvent *e);
    };
}

::DFG_MODULE_NS(qt)::CsvTableViewDelegate::CsvTableViewDelegate(QWidget* pParent)
    : BaseClass(pParent)
    , m_spTableView(qobject_cast<CsvTableView*>(pParent))
{
}

bool ::DFG_MODULE_NS(qt)::CsvTableViewDelegate::editorToString(QWidget* pWidget, QString& sText) const
{
    auto pLineEdit = qobject_cast<QLineEdit*>(pWidget);
    DFG_STATIC_ASSERT((std::is_base_of<QLineEdit, LineEditCtrl>::value), "Casting below assumes that LineEditCtrl inherits QLineEdit");
    DFG_ASSERT_CORRECTNESS(pLineEdit != nullptr); // Assuming that pWidget is QLineEdit might be relying on Qt's implementation details.
    if (pLineEdit)
        sText = pLineEdit->text();
    return (pLineEdit != nullptr);
}

void ::DFG_MODULE_NS(qt)::CsvTableViewDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (m_spTableView)
    {
        auto lockReleaser = m_spTableView->tryLockForEdit();
        if (!lockReleaser.isLocked())
        {
            m_spTableView->privShowExecutionBlockedNotification("Cell edit");
            return;
        }
    }
    QString sNewText;
    if (editorToString(editor, sNewText))
        model->setData(index, sNewText, Qt::EditRole);
}

DFG_MODULE_NS(qt)::CsvTableViewCompleterDelegate::CsvTableViewCompleterDelegate(QWidget* pParent, QCompleter* pCompleter)
    : BaseClass(pParent)
    , m_spCompleter(pCompleter)
{
}

DFG_MODULE_NS(qt)::CsvTableViewCompleterDelegate::~CsvTableViewCompleterDelegate()
{

}

QWidget* DFG_MODULE_NS(qt)::CsvTableViewCompleterDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    LineEditCtrl* pLineEdit = new LineEditCtrl(parent);
    if (m_spCompleter)
       pLineEdit->setCompleter(m_spCompleter.data());
    return pLineEdit;
}

void DFG_MODULE_NS(qt)::CsvTableViewCompleterDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    LineEditCtrl* pLineEdit = static_cast<LineEditCtrl*>(editor);
    pLineEdit->setText(index.data().toString());
}

void DFG_MODULE_NS(qt)::CsvTableViewCompleterDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

void LineEditCtrl::keyPressEvent(QKeyEvent *e)
{
   if ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space)
   {
       // Trigger autocompletion on Ctrl+space (TODO: make customisable)
       QCompleter* pCompleter = completer();
       if (pCompleter)
       {
           pCompleter->setCompletionPrefix(text());
           pCompleter->complete();
       }
   }
   else
       BaseClass::keyPressEvent(e);
}
