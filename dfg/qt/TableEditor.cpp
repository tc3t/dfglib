#include "TableEditor.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"

#include "connectHelper.hpp"
#include "PropertyHelper.hpp"

#include "qtIncludeHelpers.hpp"
DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QAction>
#include <QDockWidget>
#include <QGridLayout>
#include <QLineEdit>
#include <QStatusBar>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>
DFG_END_INCLUDE_QT_HEADERS

#define DFG_TABLEEDITOR_LOG_WARNING(x) // Placeholder for logging warning

namespace
{
    enum TableEditorPropertyId
    {
        TableEditorPropertyId_cellEditorHeight
    };

    // Class for window extent (=one dimension of window size) either as absolute value or as percentage of some widget size
    class WindowExtentProperty
    {
    public:
        WindowExtentProperty() :
            m_val(0)
        {
        }

        WindowExtentProperty(const QVariant& v) :
            m_val(0)
        {
            bool ok = false;
            auto intVal = v.toInt(&ok);
            if (ok) // Plain int? If yes, interpret as absolute value.
            {
                DFG_ROOT_NS::limit(intVal, 0, QWIDGETSIZE_MAX);
                m_val = intVal;
                return;
            }
            // Ignore everything beyond first ','-character to leave room for future additions without breaking
            // old clients.
            auto strVal = v.toString().trimmed().split(',').first();
            if (strVal.isEmpty())
            {
                auto strList = v.toStringList(); // comma-separated list can be interpreted as QStringList so check that as well.
                if (!strList.isEmpty())
                    strVal = strList.first();
            }
            if (!strVal.isEmpty() && strVal[0] == '%')
            {
                strVal.remove(0, 1);
                auto pval = strVal.toInt(&ok);
                if (ok)
                {
                    DFG_ROOT_NS::limit(pval, 0, 100);
                    m_val = -1*pval;
                }
            }
        }

        int toAbsolute(const int refValue) const
        {
            if (m_val >= 0)
                return m_val;
            else
            {
                auto doubleVal = std::abs(m_val) * static_cast<double>(refValue) / 100.0;
                return (doubleVal >= 0 && doubleVal <= QWIDGETSIZE_MAX) ? static_cast<int>(doubleVal) : 0;
            }
        }

        int m_val;
    };

    void setWidgetMaximumHeight(QWidget* pWidget, const int refHeight, const WindowExtentProperty& extent)
    {
        if (!pWidget)
            return;
        pWidget->setMaximumHeight(extent.toAbsolute(refHeight));
    }

    // Properties
    DFG_QT_DEFINE_OBJECT_PROPERTY_CLASS(TableEditor)
    DFG_QT_DEFINE_OBJECT_PROPERTY_CUSTOM_TYPE("TableEditor_cellEditorHeight",
                                              TableEditor,
                                              TableEditorPropertyId_cellEditorHeight,
                                              WindowExtentProperty,
                                              [] { return WindowExtentProperty(40); },
                                              WindowExtentProperty);

    template <TableEditorPropertyId ID>
    auto getTableEditorProperty(DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)* editor) -> typename DFG_QT_OBJECT_PROPERTY_CLASS_NAME(TableEditor)<ID>::PropertyType
    {
        return DFG_MODULE_NS(qt)::getProperty<DFG_QT_OBJECT_PROPERTY_CLASS_NAME(TableEditor)<ID>>(editor);
    }

    class HighlightTextEdit : public QLineEdit
    {
    public:
        typedef QLineEdit BaseClass;
        HighlightTextEdit() {}
        HighlightTextEdit(QWidget* pParent) :
            BaseClass(pParent)
        {}
    };

} // unnamed namespace

Q_DECLARE_METATYPE(WindowExtentProperty); // Note: placing this in unnamed namespace generated a "class template specialization of 'QMetaTypeId' must occure at global scope"-message

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) { namespace DFG_DETAIL_NS {

    class FindPanelWidget : public QWidget
    {
    public:
        FindPanelWidget()
        {
            auto l = new QGridLayout(this);
            l->addWidget(new QLabel("Find", this), 0, 0);
            m_pTextEdit = new HighlightTextEdit(this);
            l->addWidget(m_pTextEdit, 0, 1);

            l->addWidget(new QLabel("Column", this), 0, 2);
            m_pColumnSelector = new QSpinBox(this);
            m_pColumnSelector->setMinimum(-1);
            m_pColumnSelector->setValue(-1);
            l->addWidget(m_pColumnSelector, 0, 3);

            // TODO: match type (wildcard, regexp...)
            // TODO: highlighting details (color)
            // TODO: match count
        }

        HighlightTextEdit* m_pTextEdit;
        QSpinBox* m_pColumnSelector;
    };
}}} // dfg::qt::DFG_DETAILS_NS -namespace


DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::DFG_CLASS_NAME(TableEditor)() :
    m_bHandlingOnCellEditorTextChanged(false)
{
    setProperty("dfglib_allow_app_settings_usage", true);
    // Model
    m_spTableModel.reset(new DFG_CLASS_NAME(CsvItemModel));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigSourcePathChanged, this, &ThisClass::onSourcePathChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigOnNewSourceOpened, this, &ThisClass::onNewSourceOpened));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigModifiedStatusChanged, this, &ThisClass::onModifiedStatusChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigOnSaveToFileCompleted, this, &ThisClass::onSaveCompleted));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::dataChanged, this, &ThisClass::onModelDataChanged));

    // View
    m_spTableView.reset(new DFG_CLASS_NAME(CsvTableView)(this));
    m_spTableView->setModel(m_spTableModel.get());
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ThisClass::onSelectionChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigFindActivated, this, &ThisClass::onFindRequested));

    // Source path line edit
    m_spLineEditSourcePath.reset(new QLineEdit());
    m_spLineEditSourcePath->setReadOnly(true);
    m_spLineEditSourcePath->setPlaceholderText(tr("<no file path available>"));

    // Cell Editor and it's container widget
    {
        m_spCellEditorDockWidget.reset(new QDockWidget(this));
        m_spCellEditorDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        m_spCellEditorDockWidget->setObjectName("Cell edit");

        m_spCellEditor.reset(new CellEditor(this));
        m_spCellEditor->setDisabled(true);

        m_spCellEditorDockWidget->setWidget(m_spCellEditor.get());

        DFG_QT_VERIFY_CONNECT(connect(m_spCellEditor.get(), &QPlainTextEdit::textChanged, this, &ThisClass::onCellEditorTextChanged));
    }

    // Status bar
    {
        m_spStatusBar.reset(new DFG_CLASS_NAME(TableEditorStatusBar));
    }

    // Layout
    {
        std::unique_ptr<QGridLayout> spLayout(new QGridLayout);
        spLayout->addWidget(m_spLineEditSourcePath.get(), 0, 0);
        spLayout->addWidget(m_spTableView.get(), 1, 0);
        const auto cellEditorMaxHeight = m_spCellEditor->maximumHeight();
        spLayout->addWidget(m_spCellEditorDockWidget.get(), 2, 0);
        m_spFindPanel.reset(new DFG_DETAIL_NS::FindPanelWidget);
        DFG_QT_VERIFY_CONNECT(connect(m_spFindPanel->m_pTextEdit, &QLineEdit::textChanged, this, &ThisClass::onHighlightTextChanged));
        DFG_QT_VERIFY_CONNECT(connect(m_spFindPanel->m_pColumnSelector, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ThisClass::onFindColumnChanged));
        spLayout->addWidget(m_spFindPanel.get(), 3, 0);
        spLayout->addWidget(m_spStatusBar.get(), 4, 0);
        delete layout();
        setLayout(spLayout.release());
    }

    // Set default window size.
    {
        auto desktop = QApplication::desktop();
        if (desktop)
        {
            const auto screenRect = desktop->screenGeometry(this); // Returns screen geometry of the screen that contains 'this' widget.
            resize(screenRect.size() * 0.75); // Fill 0.75 of the whole screen in both directions (arbitrary default value)
        }
    }

    // Resize widgets
    setWidgetMaximumHeight(m_spCellEditorDockWidget.get(), this->height(), getTableEditorProperty<TableEditorPropertyId_cellEditorHeight>(this));

    resizeColumnsToView();
}

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::~DFG_CLASS_NAME(TableEditor)()
{
    m_spCellEditorDockWidget.release();
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::tryOpenFileFromPath(QString path)
{
    if (!m_spTableModel || (m_spTableModel && m_spTableModel->isModified()))
        return false;

    return m_spTableModel->openFile(path);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::updateWindowTitle()
{
    const auto filePath = (m_spTableModel) ? m_spTableModel->getFilePath() : QString();
    // Note: [*] is a placeholder for modified-indicator (see QWidget::windowModified)
    QString prename = (!filePath.isEmpty()) ? QFileInfo(filePath).fileName() + "[*] - " : QString();
    QString title(prename + QCoreApplication::applicationName());
    setWindowModified(m_spTableModel && m_spTableModel->isModified());
    setWindowTitle(title);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onSourcePathChanged()
{
    if (!m_spTableModel || !m_spLineEditSourcePath)
        return;
    m_spLineEditSourcePath->setText(m_spTableModel->getFilePath());

    updateWindowTitle();
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onNewSourceOpened()
{
    if (!m_spTableModel)
        return;
    auto& model = *m_spTableModel;
    if (m_spLineEditSourcePath)
        m_spLineEditSourcePath->setText(model.getFilePath());
    if (m_spStatusBar)
        m_spStatusBar->showMessage(tr("Reading lasted %1 s").arg(model.latestReadTimeInSeconds(), 0, 'g', 4));
    resizeColumnsToView();
    updateWindowTitle();
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onSaveCompleted(const bool success, const double saveTimeInSeconds)
{
    if (!m_spStatusBar)
        return;
    if (success)
        m_spStatusBar->showMessage(tr("Saving done in %1 s").arg(saveTimeInSeconds, 0, 'g', 4));
    else
        m_spStatusBar->showMessage(tr("Saving failed lasting %1 s").arg(saveTimeInSeconds, 0, 'g', 4));
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    DFG_UNUSED(selected);
    DFG_UNUSED(deselected);
    if (m_spCellEditor && m_spTableView && m_spTableModel)
    {
        const QModelIndexList& indexes = m_spTableView->getSelectedItemIndexes(m_spTableView->getProxyModelPtr());
        // Block signals from edit to prevent "text edited" signals from non-user edits.
        auto sc = DFG_ROOT_NS::makeScopedCaller([this](){m_spCellEditor->blockSignals(true);}, [this](){m_spCellEditor->blockSignals(false);});
        if (indexes.size() == 1)
        {
            const auto& index = indexes.front();
            m_spCellEditor->setEnabled(true);

            auto& model = *m_spTableModel;
            m_spCellEditor->setPlainText(model.data(index).toString());

            auto colDescription = model.headerData(index.column(), Qt::Horizontal).toString();
            if (colDescription.isEmpty())
                colDescription = QString::number(index.column());
            const QString sAddInfo = QString(" (%1, %2)")
                                .arg(model.headerData(index.row(), Qt::Vertical).toString())
                                .arg(colDescription);
            m_spCellEditorDockWidget->setWindowTitle(tr("Cell edit") + sAddInfo);
        }
        else
        {
            m_spCellEditor->setPlainText("");
            m_spCellEditor->setEnabled(false);
            m_spCellEditorDockWidget->setWindowTitle(tr("Cell edit"));
        }
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onModifiedStatusChanged(const bool b)
{
    setWindowModified(b);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::resizeColumnsToView(ColumnResizeStyle style)
{
    auto viewport = (m_spTableView) ? m_spTableView->viewport() : nullptr;
    if (!viewport || !m_spTableModel || m_spTableModel->getColumnCount() == 0)
        return;
    if (style == ColumnResizeStyle_evenlyDistributed)
    {
        const auto width = m_spTableView->viewport()->width();
        const int sectionSize = width / m_spTableModel->getColumnCount();
        m_spTableView->horizontalHeader()->setDefaultSectionSize(sectionSize);
    }
    else
        DFG_TABLEEDITOR_LOG_WARNING(QString("Requested resizeColumnsToView with unknown style '%1'").arg(static_cast<int>(style)));
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::setAllowApplicationSettingsUsage(const bool b)
{
    if (m_spTableView)
        m_spTableView->setAllowApplicationSettingsUsage(b);
}

namespace
{
    static bool handleExitConfirmationAndReturnTrueIfCanExit(DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableView)* view)
    {
        auto model = (view) ? view->csvModel() : nullptr;
        if (!model)
            return true;

        // Finish edits before checking modified as otherwise ongoing edit whose editor
        // has not been closed would not be detected.
        // In practice: user opens file, edits a cell and presses X while cell content editor is active,
        // without line below:
        // -> this function would be called before model's setData()-handler
        // -> Since no setData()-calls have been made, this function gets 'model->isModified() == false'
        //    and editor closes without confirmation.
        view->finishEdits();

        if (model->isModified())
        {
            const auto existingPath = model->getFilePath();
            const QString uiPath = (!existingPath.isEmpty()) ?
                                QCoreApplication::tr("(path: %1)").arg(existingPath)
                                : QCoreApplication::tr("(no path exists, it will be asked if saving is chosen)");
            const auto rv = QMessageBox::question(nullptr,
                                                      QCoreApplication::tr("Save to file?"),
                                                      QCoreApplication::tr("Content has been edited, save to file?\n%1").arg(uiPath),
                                                      QMessageBox::Yes,
                                                      QMessageBox::No,
                                                      QMessageBox::Cancel);
            if (rv == QMessageBox::Cancel)
                return false;

            if (rv == QMessageBox::Yes)
                view->save();
        }

        return true;
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::closeEvent(QCloseEvent* event)
{
    if (!event)
    {
        BaseClass::closeEvent(event);
        return;
    }

    if (handleExitConfirmationAndReturnTrueIfCanExit(m_spTableView.get()))
        event->accept();
    else
        event->ignore();
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onCellEditorTextChanged()
{
    auto selectionModel = (m_spTableView) ? m_spTableView->selectionModel() : nullptr;
    const QModelIndexList& indexes = (selectionModel) ? selectionModel->selectedIndexes() : QModelIndexList();
    if (m_spTableModel && m_spCellEditor && indexes.size() == 1)
    {
        const auto oldFlag = m_bHandlingOnCellEditorTextChanged;
        auto flagHandler = makeScopedCaller([&] { m_bHandlingOnCellEditorTextChanged = true ;}, [=] { m_bHandlingOnCellEditorTextChanged = oldFlag; });
        this->m_spTableModel->setData(indexes.front(), m_spCellEditor->toPlainText());
    }
}

namespace
{
    static bool isModelIndexWithinSelectionRectangle(const QModelIndex& target, const QModelIndex& topLeft, const QModelIndex& bottomRight)
    {
        return (target.row() >= topLeft.row() && target.row() <= bottomRight.row()
                &&
                target.column() >= topLeft.column() && target.column() <= bottomRight.column());
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    DFG_UNUSED(roles);
    if (!m_spCellEditor)
        return;
    if (m_bHandlingOnCellEditorTextChanged) // To prevent cell editor change signal triggering reloading cell editor data from model.
        return;
    auto selectionModel = (m_spTableView) ? m_spTableView->selectionModel() : nullptr;
    const QModelIndexList& indexes = (selectionModel) ? selectionModel->selectedIndexes() : QModelIndexList();
    if (indexes.size() == 1 && isModelIndexWithinSelectionRectangle(indexes[0], topLeft, bottomRight))
            onSelectionChanged(QItemSelection(indexes[0], indexes[0]), QItemSelection());
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onHighlightTextChanged(const QString& text)
{
    if (!m_spTableView || !m_spFindPanel)
        return;

    m_spTableView->setFindText(text, m_spFindPanel->m_pColumnSelector->value());
    m_spTableView->onFindNext();
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onFindColumnChanged(const int newCol)
{
    DFG_UNUSED(newCol);
	if (m_spFindPanel)
    	onHighlightTextChanged(m_spFindPanel->m_pTextEdit->text());
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onFindRequested()
{
    if (m_spFindPanel)
        m_spFindPanel->m_pTextEdit->setFocus(Qt::OtherFocusReason);
}
