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

void ::DFG_MODULE_NS(qt)::CsvTableViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!option.state.testFlag(QStyle::State_Selected))
    {
        // If cell is not selected, default painting does what's needed
        BaseClass::paint(painter, option, index);
        return;
    }
    // Getting here means that cell is selected and since selected cells don't use index.data(Qt::BackgroundRole) in background drawing,
    // handling the case manually and adjusting color if needed (related to ticket #120 about selected items that match find-filter not showing clearly)
    const auto bgRole = index.data(Qt::BackgroundRole);
    if (bgRole.isNull())
        BaseClass::paint(painter, option, index); // Cell has no BackgroundRole -> default painting does what's needed.
    else
    {
        // Getting here means that index is selected and it has non-default background -> using BackgroundRole
        // with adjustments so that cell has different background color than what non-selected would have.
        const auto bIsActive = option.state.testFlag(QStyle::State_Active);
        const auto bgBrush = bgRole.value<QBrush>();
        const auto bgColor = bgBrush.color();
        const auto makeBgColor = [&]()
        {
            // Arbitrary adjustment to bgColor.
            const auto factor = (bIsActive) ? 0.5 : 0.75;
            return QColor(limited(static_cast<int>(bgColor.red() * factor), 0, 255),
                          limited(static_cast<int>(bgColor.green() * factor), 0, 255),
                          limited(static_cast<int>(bgColor.blue() * factor), 0, 255));
        };
        auto newPalette = option.palette;
        const auto colorGroup = bIsActive ? QPalette::Active : QPalette::Inactive;
        newPalette.setColor(colorGroup, QPalette::Highlight, makeBgColor());
        auto newOption = option;
        newOption.palette = newPalette;
        BaseClass::paint(painter, newOption, index);
    }
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
