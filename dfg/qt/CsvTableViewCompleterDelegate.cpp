#include "CsvTableViewCompleterDelegate.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QKeyEvent>
    #include <QCompleter>
    #include <QLineEdit>
    #include <QPainter>
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

bool ::DFG_MODULE_NS(qt)::CsvTableViewDelegate::checkCellEditability(const QModelIndex& index) const
{
    if (!m_spTableView)
        return false;
    const auto dataIndex = m_spTableView->mapToDataModel(index);
    const auto cellEditability = m_spTableView->getCellEditability(RowIndex_data(dataIndex.row()), ColumnIndex_data(dataIndex.column()));
    return (cellEditability == CsvTableView::CellEditability::editable);
#if 0
        QString sMsg;
        switch (cellEditability)
        {
            case CsvTableView::CellEditability::blocked_tableReadOnly:  sMsg = tr("Unable to edit: table is read-only"); break;
            case CsvTableView::CellEditability::blocked_columnReadOnly: sMsg = tr("Unable to edit: column is read-only"); break;
            default:                                                    sMsg = tr("Unable to edit: cell is not editable"); break;
        }
        m_spTableView->showStatusInfoTip(sMsg);
#endif
}

QWidget* ::DFG_MODULE_NS(qt)::CsvTableViewDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    DFG_UNUSED(option);
    auto p = new LineEditCtrl(parent);
    if (!checkCellEditability(index))
        p->setReadOnly(true);
    return p;
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
    const auto doDefaultPaint = [&]()
    {
        auto pCsvModel = (m_spTableView) ? m_spTableView->csvModel() : nullptr;
        const bool bIsCellEditable = (pCsvModel) ? pCsvModel->isCellEditable(m_spTableView->mapToDataModel(index)) : nullptr;
        if (!bIsCellEditable)
            painter->fillRect(option.rect, m_spTableView->getReadOnlyBackgroundColour());
        BaseClass::paint(painter, option, index);
    };

    // If cell is not selected, default painting does what's needed
    if (!option.state.testFlag(QStyle::State_Selected))
    {
        doDefaultPaint();
        return;
    }

    // handling the case manually and adjusting color if needed (related to ticket #120 about selected items that match find-filter not showing clearly)
    const auto bgRole = index.data(Qt::BackgroundRole);
    if (bgRole.isNull())
        doDefaultPaint(); // Cell has no BackgroundRole -> default painting does what's needed.
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

DFG_MODULE_NS(qt)::CsvTableViewCompleterDelegate::~CsvTableViewCompleterDelegate() = default;

QWidget* DFG_MODULE_NS(qt)::CsvTableViewCompleterDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto pLineEdit = qobject_cast<QLineEdit*>(BaseClass::createEditor(parent, option, index));
    if (pLineEdit && !pLineEdit->isReadOnly())
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
    const auto bHasControlModifier = (e->modifiers() & Qt::ControlModifier);
    if (bHasControlModifier && e->key() == Qt::Key_Space)
    {
        // Trigger autocompletion on Ctrl+space (TODO: make customisable)
        QCompleter* pCompleter = completer();
        if (pCompleter)
        {
            pCompleter->setCompletionPrefix(text());
            pCompleter->complete();
        }
    }
    else if (bHasControlModifier || !this->isReadOnly()) // In read-only mode, not passing normal key events to baseclass since it by default triggers "jump to next cell starting with pressed key" which can cause unwanted UX.
        BaseClass::keyPressEvent(e);
}
