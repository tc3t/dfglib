#include "CsvTableViewCompleterDelegate.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"
#include "widgetHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QAbstractButton>
    #include <QCompleter>
    #include <QKeyEvent>
    #include <QLineEdit>
    #include <QPainter>
    #include <QPlainTextEdit>
    #include <QVBoxLayout>
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
    }; // class LineEditCtrl

    class MultiLineEditor : public QDialog
    {
    public:
        //using BaseClass = QWidget;
        using BaseClass = QDialog;

        MultiLineEditor(QWidget* pParent);
        ~MultiLineEditor();

        QString toPlainText() const;
        void setPlainText(const QString& s);
        void setReadOnly(bool bReadOnly);

        ::DFG_MODULE_NS(qt)::QObjectStorage<QPlainTextEdit> m_spPlainTextEdit;
    }; // class MultiLineEditor
} // unnamed namespace

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

template <class T>
T* ::DFG_MODULE_NS(qt)::CsvTableViewDelegate::createEditorImpl(QWidget* pParent, const QModelIndex& index) const
{
    auto pEditor = new T(pParent);
    if (!checkCellEditability(index))
        pEditor->setReadOnly(true);
    return pEditor;
}

QWidget* ::DFG_MODULE_NS(qt)::CsvTableViewDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    DFG_UNUSED(option);
    const auto sText = index.data().toString();
    QWidget* pEditor = nullptr;
    // If cell has no newlines, using LineEditCtrl...
    if (sText.indexOf('\n') == -1 && sText.indexOf(QChar::SpecialCharacter::LineSeparator) == -1)
        pEditor = createEditorImpl<LineEditCtrl>(parent, index);
    else // ...otherwise MultiLineEditor without completer-functionality.
        pEditor = createEditorImpl<MultiLineEditor>(parent, index);
    return pEditor;
}

bool ::DFG_MODULE_NS(qt)::CsvTableViewDelegate::editorToString(QWidget* pWidget, QString& sText) const
{
    auto pLineEdit = qobject_cast<QLineEdit*>(pWidget);
    auto pMultiLineEditor = (pLineEdit) ? nullptr : dynamic_cast<MultiLineEditor*>(pWidget);
    if (pLineEdit)
        sText = pLineEdit->text();
    else if (pMultiLineEditor)
        sText = pMultiLineEditor->toPlainText();
    else
        DFG_ASSERT_IMPLEMENTED(false);
    return (pLineEdit || pMultiLineEditor);
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

void ::DFG_MODULE_NS(qt)::CsvTableViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& optionArg, const QModelIndex& index) const
{
    auto option = optionArg;
    option.index = index; // drawDisplay() didn't seem to receive index by default in option.index, adding it there just in case it will need it at some point.

    // Default alignment in QStyleOptionViewItem::displayAlignment is Qt::AlignLeft which can cause cell to show empty if it has trailing newlines (#135).
    // In default case adding Qt::AlignTop so that first line will always be shown.
    const auto displayAlignment = index.data(Qt::TextAlignmentRole);
    option.displayAlignment = (displayAlignment.isNull()) ? Qt::AlignLeft | Qt::AlignTop : Qt::Alignment(displayAlignment.toInt());
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

    const auto bgRole = index.data(Qt::BackgroundRole);
    if (bgRole.isNull())
        doDefaultPaint(); // Cell has no BackgroundRole -> default painting does what's needed.
    else
    {
        // Getting here means that index is selected and it has non-default background -> using BackgroundRole
        // with adjustments so that cell has different background color than what non-selected would have.
        // (related to ticket #120 about selected items that match find-filter not showing clearly)
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

void ::DFG_MODULE_NS(qt)::CsvTableViewDelegate::drawDisplay(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QString& text) const
{
    // Checking if adjustments are needed for better multiline visualization (#135)
    // If rect height is such that only first line is show, adding ellipsis to end of first line.
    // Not optimal in the sense that with non-default row height, multiline cell can show empty as the ellipsis handling might not get activated.
    // Also if there is content after last shown row, ellipsis are not shown.
    if (painter)
    {
        const auto nRectHeight = rect.height();
        const auto nFontHeight = painter->fontMetrics().height();
        /*
        // These lines could be used to access row height of the row that is being drawn.
        const auto nRow = option.index.row();
        const auto nRowHeight = m_spTableView->rowHeight(nRow);
        */
        if (nFontHeight < 10000000 && nRectHeight <= nFontHeight * 7 / 4) // Using factor 1.75 so that ellipsis are shown until second line is mostly shown. First condition is overflow guard.
        {
            const auto nEolPos = text.indexOf(QChar::LineSeparator); // Qt converts \n into QChar::LineSeparator in QAbstractItemDelegatePrivate::textForRole() called by QItemDelegate::paint() 
            if (nEolPos != -1)
            {
                // EOL was found, replacing EOL -> ...EOL
                QString sAdjusted = text;
                sAdjusted.insert(nEolPos, QChar(0x2026)); // 0x2026 is horizontal ellipsis (https://en.wikipedia.org/wiki/Ellipsis)
                BaseClass::drawDisplay(painter, option, rect, sAdjusted);
                return;
            }
        }
    }
    BaseClass::drawDisplay(painter, option, rect, text);
}

bool DFG_MODULE_NS(qt)::CsvTableViewDelegate::eventFilter(QObject* editor, QEvent* event)
{
    // Special handling for MultiLineEditor Ok-button: clicking OK-button didn't trigger the same handling as clicking mouse outside the editor
    // so handling it separately. Note that Hide-event from Cancel-button didn't seem to get here at all.
    if (event && event->type() == QEvent::Hide)
    {
        auto pMultiLineEditor = qobject_cast<QDialog*>(editor);
        if (pMultiLineEditor && pMultiLineEditor->result() == QDialog::Accepted)
        {
            Q_EMIT commitData(pMultiLineEditor);
            return true;
        }
    }    
    return BaseClass::eventFilter(editor, event);
}

void DFG_MODULE_NS(qt)::CsvTableViewDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto pMultiLineEditor = dynamic_cast<MultiLineEditor*>(editor);
    if (pMultiLineEditor)
    {
        const auto nHeightIncrease = pMultiLineEditor->fontMetrics().height() * 6;
        editor->setGeometry(option.rect.adjusted(0, 0, 0, nHeightIncrease));
    }
    else
        BaseClass::updateEditorGeometry(editor, option, index);
}

void DFG_MODULE_NS(qt)::CsvTableViewDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto pLineEdit = dynamic_cast<LineEditCtrl*>(editor);
    auto pMultiLineEditor = (pLineEdit) ? nullptr : dynamic_cast<MultiLineEditor*>(editor);
    const QString sText = index.data().toString();
    if (pLineEdit)
        pLineEdit->setText(sText);
    else if (pMultiLineEditor)
        pMultiLineEditor->setPlainText(sText);
    else
        DFG_ASSERT_IMPLEMENTED(false);
}

/////////////////////////////////////////////////////////////////////////////////////
// 
// CsvTableViewCompleterDelegate
//
/////////////////////////////////////////////////////////////////////////////////////

DFG_MODULE_NS(qt)::CsvTableViewCompleterDelegate::CsvTableViewCompleterDelegate(QWidget* pParent, QCompleter* pCompleter)
    : BaseClass(pParent)
    , m_spCompleter(pCompleter)
{
}

DFG_MODULE_NS(qt)::CsvTableViewCompleterDelegate::~CsvTableViewCompleterDelegate() = default;

QWidget* DFG_MODULE_NS(qt)::CsvTableViewCompleterDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto pEditor = BaseClass::createEditor(parent, option, index);
    auto pLineEdit = qobject_cast<QLineEdit*>(pEditor);
    if (pLineEdit && !pLineEdit->isReadOnly())
       pLineEdit->setCompleter(m_spCompleter.data());
    return pEditor;
}

/////////////////////////////////////////////////////////////////////////////////////
// 
// LineEditCtrl
//
/////////////////////////////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////////////////////////////
// 
// MultiLineEditor
//
/////////////////////////////////////////////////////////////////////////////////////

MultiLineEditor::MultiLineEditor(QWidget* pParent)
    : BaseClass(pParent)
{
    setWindowFlags(Qt::SubWindow); // This removes dialog frames and makes sizegrip resize this dialog instead of parent window.
    m_spPlainTextEdit.reset(new QPlainTextEdit(this));
    auto pLayout = new QVBoxLayout(this);
    pLayout->setMargin(0);
    pLayout->addWidget(m_spPlainTextEdit.get());
    auto pButtons = ::DFG_MODULE_NS(qt)::addOkCancelButtonBoxToDialog(this);
    pLayout->addWidget(pButtons);
    pLayout->addSpacing(15); // To make sure that size grip won't overlap with buttons

    this->setSizeGripEnabled(true);

    DFG_QT_VERIFY_CONNECT(QObject::connect(this, &QDialog::rejected, this, &QObject::deleteLater));
    DFG_QT_VERIFY_CONNECT(QObject::connect(this, &QDialog::accepted, this, &QObject::deleteLater));
    setResult(QDialog::Accepted);
}

MultiLineEditor::~MultiLineEditor()
{
}

QString MultiLineEditor::toPlainText() const
{
    return (m_spPlainTextEdit) ? m_spPlainTextEdit->toPlainText() : QString();
}

void MultiLineEditor::setPlainText(const QString& s)
{
    if (m_spPlainTextEdit)
        m_spPlainTextEdit->setPlainText(s);
}

void MultiLineEditor::setReadOnly(const bool bReadOnly)
{
    if (m_spPlainTextEdit)
        m_spPlainTextEdit->setReadOnly(bReadOnly);
}
