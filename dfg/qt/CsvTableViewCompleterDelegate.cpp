#include "CsvTableViewCompleterDelegate.hpp"

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

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewCompleterDelegate)::DFG_CLASS_NAME(CsvTableViewCompleterDelegate)(QObject *parent, QCompleter* pCompleter)
    : QItemDelegate(parent), m_spCompleter(pCompleter)
{
}

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewCompleterDelegate)::~DFG_CLASS_NAME(CsvTableViewCompleterDelegate)()
{

}

QWidget* DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewCompleterDelegate)::createEditor(QWidget *parent,
    const QStyleOptionViewItem &/* option */,
    const QModelIndex &/* index */) const
{
    LineEditCtrl* pLineEdit = new LineEditCtrl(parent);
    if (m_spCompleter)
       pLineEdit->setCompleter(m_spCompleter.data());
    return pLineEdit;
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewCompleterDelegate)::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    LineEditCtrl* pLineEdit = static_cast<LineEditCtrl*>(editor);
    pLineEdit->setText(index.data().toString());

}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewCompleterDelegate)::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    LineEditCtrl* pLineEdit = static_cast<LineEditCtrl*>(editor);
    model->setData(index, pLineEdit->text(), Qt::EditRole);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewCompleterDelegate)::updateEditorGeometry(QWidget *editor,
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
