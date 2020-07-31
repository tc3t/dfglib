#include "TableEditor.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"

#include "connectHelper.hpp"
#include "PropertyHelper.hpp"

#include "qtIncludeHelpers.hpp"
DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
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
#include <QMenu>
#include <QMessageBox>
#include <QMetaMethod>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QSpinBox>
#include <QToolBar>
#include <QToolButton>
#include <QSplitter>
#include <QTimer>
DFG_END_INCLUDE_QT_HEADERS

#define DFG_TABLEEDITOR_LOG_WARNING(x) // Placeholder for logging warning

namespace
{
    enum TableEditorPropertyId
    {
        // Add properties here in the beginning (or remember to update TableEditorPropertyId_last)
        TableEditorPropertyId_cellEditorFontPointSize,
        TableEditorPropertyId_cellEditorHeight,
        TableEditorPropertyId_last = TableEditorPropertyId_cellEditorHeight
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
			// Check if v is already of type WindowExtentProperty
			if (v.canConvert<WindowExtentProperty>())
			{
				*this = v.value<WindowExtentProperty>();
				return;
			}
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

    double defaultFontPointSizeForCellEditor()
    {
        auto pApp = qobject_cast<QApplication*>(QApplication::instance());
        DFG_ASSERT(pApp != nullptr); // Remove this if there's a valid case of having TableEditor without QApplication.
        return (pApp) ? pApp->font().pointSizeF() : 8.25;
    }

    // Properties
    DFG_QT_DEFINE_OBJECT_PROPERTY_CLASS(TableEditor)
    DFG_QT_DEFINE_OBJECT_PROPERTY_CUSTOM_TYPE("TableEditor_cellEditorHeight",
                                              TableEditor,
                                              TableEditorPropertyId_cellEditorHeight,
                                              WindowExtentProperty,
                                              [] { return WindowExtentProperty(50); },
                                              WindowExtentProperty);
    DFG_QT_DEFINE_OBJECT_PROPERTY("TableEditor_cellEditorFontPointSize",
                                              TableEditor,
                                              TableEditorPropertyId_cellEditorFontPointSize,
                                              double,
                                              defaultFontPointSizeForCellEditor);


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

    // TODO: move to CsvTableView.h?
    class FindPanelWidget : public QWidget
    {
    public:
        FindPanelWidget(const QString& label)
            : m_pTextEdit(nullptr)
            , m_pColumnSelector(nullptr)
            , m_pCaseSensitivityCheckBox(nullptr)
            , m_pMatchSyntaxCombobox(nullptr)
        {
            int nColumn = 0;

            auto l = new QGridLayout(this);
            l->addWidget(new QLabel(label, this), 0, nColumn++);

            m_pTextEdit = new HighlightTextEdit(this);
            l->addWidget(m_pTextEdit, 0, nColumn++);

            // Case-sensitivity control
            {
                m_pCaseSensitivityCheckBox = new QCheckBox(tr("Case sensitive"), this);
                m_pCaseSensitivityCheckBox->setToolTip(tr("Check to enable case sensitivity"));
                m_pCaseSensitivityCheckBox->setChecked(false);
                l->addWidget(m_pCaseSensitivityCheckBox, 0, nColumn++);
            }

            // Match syntax control
            {
                m_pMatchSyntaxCombobox = new QComboBox(this);

                m_pMatchSyntaxCombobox->insertItem(0, tr("Wildcard"),              QRegExp::Wildcard);
                m_pMatchSyntaxCombobox->insertItem(1, tr("Wildcard (Unix Shell)"), QRegExp::WildcardUnix);
                m_pMatchSyntaxCombobox->insertItem(2, tr("Simple string"),          QRegExp::FixedString);
                m_pMatchSyntaxCombobox->insertItem(3, tr("Regular expression"),     QRegExp::RegExp);
                m_pMatchSyntaxCombobox->insertItem(4, tr("Regular expression 2"),   QRegExp::RegExp2);
                m_pMatchSyntaxCombobox->insertItem(5, tr("Regular expression"
                                                         " (W3C XML Schema 1.1)"),  QRegExp::W3CXmlSchema11);
                m_pMatchSyntaxCombobox->setCurrentIndex(0);

                const char szRegularExpressionTooltip[] = "Regular expression\n\n"
                                                          ". Matches any single character\n"
                                                          ".* Matches zero or more characters\n"
                                                          "[...] Set of characters\n"
                                                          "| Match expression before or after\n"
                                                          "^ Beginning of line\n"
                                                          "$ End of line\n";

                // Set tooltips.
                m_pMatchSyntaxCombobox->setItemData(0, "? Matches single character, the same as . in regexp.\n"
                                                       "* Matches zero or more characters, the same as .* in regexp.\n"
                                                       "[...] Set of character similar to regexp.", Qt::ToolTipRole);
                m_pMatchSyntaxCombobox->setItemData(1, "Like Wildcard, but wildcard characters can be escaped with \\", Qt::ToolTipRole);
                m_pMatchSyntaxCombobox->setItemData(2, "Plain search string, no need to worry about special characters", Qt::ToolTipRole);
                m_pMatchSyntaxCombobox->setItemData(3, szRegularExpressionTooltip, Qt::ToolTipRole);
                m_pMatchSyntaxCombobox->setItemData(4, "Regular expression with greedy quantifiers", Qt::ToolTipRole);
                m_pMatchSyntaxCombobox->setItemData(5, "Regular expression (W3C XML Schema 1.1)", Qt::ToolTipRole);


                l->addWidget(m_pMatchSyntaxCombobox, 0, nColumn++);
            }

            // Column control
            {
                l->addWidget(new QLabel(tr("Column"), this), 0, nColumn++);
                m_pColumnSelector = new QSpinBox(this);
                m_pColumnSelector->setMinimum(-1);
                m_pColumnSelector->setValue(-1);
                l->addWidget(m_pColumnSelector, 0, nColumn++);
            }

            // TODO: match type (wildcard, regexp...)
            // TODO: highlighting details (color)
            // TODO: match count
        }

        Qt::CaseSensitivity getCaseSensitivity() const
        {
            return (m_pCaseSensitivityCheckBox->isChecked()) ? Qt::CaseSensitive : Qt::CaseInsensitive;
        }

        QString getPattern() const
        {
            return (m_pTextEdit) ? m_pTextEdit->text() : QString();
        }

        QRegExp::PatternSyntax getPatternSyntax() const
        {
            const auto currentData = m_pMatchSyntaxCombobox->currentData();
            return static_cast<QRegExp::PatternSyntax>(currentData.toUInt());
        }

        // Returned object is owned by 'this' and lives until the destruction of 'this'.
        QCheckBox* getCaseSensivitivyCheckBox() { return m_pCaseSensitivityCheckBox; }

        HighlightTextEdit* m_pTextEdit;
        QSpinBox* m_pColumnSelector;
        QCheckBox* m_pCaseSensitivityCheckBox;
        QComboBox* m_pMatchSyntaxCombobox;
    };

    class FilterPanelWidget : public FindPanelWidget
    {
    public:
        typedef FindPanelWidget BaseClass;
        FilterPanelWidget()
            : BaseClass(tr("Filter"))
        {
        }
    };
}}} // dfg::qt::DFG_DETAILS_NS -namespace

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::CellEditor::setFontPointSizeF(const qreal pointSize)
{
    if (pointSize <= 0)
    {
        DFG_ASSERT_INVALID_ARGUMENT(false, "Point size value should be > 0");
        return;
    }
    auto font = this->font();
    font.setPointSizeF(pointSize);
    setFont(font);
}


DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::DFG_CLASS_NAME(TableEditor)() :
    m_bHandlingOnCellEditorTextChanged(false)
{
    // Model
    m_spTableModel.reset(new DFG_CLASS_NAME(CsvItemModel));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigSourcePathChanged, this, &ThisClass::onSourcePathChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigOnNewSourceOpened, this, &ThisClass::onNewSourceOpened));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigModifiedStatusChanged, this, &ThisClass::onModifiedStatusChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigOnSaveToFileCompleted, this, &ThisClass::onSaveCompleted));

    // Proxy model
    m_spProxyModel.reset(new ProxyModelClass(this));
    m_spProxyModel->setSourceModel(m_spTableModel.get());
    m_spProxyModel->setDynamicSortFilter(true);

    // View
    m_spTableView.reset(new ViewClass(this));
    m_spTableView->setModel(m_spProxyModel.get());
    std::unique_ptr<DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)> spAnalyzerPanel(new DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)(this));
    m_spTableView->addSelectionAnalyzer(std::make_shared<DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzer)>(spAnalyzerPanel.get()));
    m_spSelectionAnalyzerPanel.reset(spAnalyzerPanel.release());
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigSelectionChanged, this, &ThisClass::onSelectionChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigFindActivated, this, &ThisClass::onFindRequested));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigFilterActivated, this, &ThisClass::onFilterRequested));

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
        m_spCellEditor->setFontPointSizeF(getTableEditorProperty<TableEditorPropertyId_cellEditorFontPointSize>(this));

        m_spCellEditorDockWidget->setWidget(m_spCellEditor.get());

        DFG_QT_VERIFY_CONNECT(connect(m_spCellEditor.get(), &QPlainTextEdit::textChanged, this, &ThisClass::onCellEditorTextChanged));
    }

    // Status bar
    {
        m_spStatusBar.reset(new DFG_CLASS_NAME(TableEditorStatusBar));
    }

    m_spMainSplitter.reset(new QSplitter(Qt::Horizontal, this));

    // Layout
    {
        std::unique_ptr<QGridLayout> spLayout(new QGridLayout);
        int row = 0;

        // First row
        {
            auto firstRowLayout = new QHBoxLayout; // Deletion through layout parentship (see addLayout).
            firstRowLayout->addWidget(m_spLineEditSourcePath.get());

            m_spToolBar.reset(new QToolBar(this));
            firstRowLayout->addWidget(m_spToolBar.get());

            spLayout->addLayout(firstRowLayout, row++, 0); // From docs: "layout becomes a child of the grid layout"
        } // First row

        spLayout->addWidget(m_spTableView.get(), row++, 0);
        spLayout->addWidget(m_spCellEditorDockWidget.get(), row++, 0);

        // Miscellaneous controls
        {
            std::unique_ptr<QHBoxLayout> pHl(new QHBoxLayout(nullptr));
            pHl->setSpacing(20); // Space between items

            // Resize controls
            {
                //auto pResizeButton = new QPushButton(tr("Resize..."), this); // Deletion through parentship.
                //pResizeButton->setMaximumWidth(100);
                m_spResizeColumnsMenu = m_spTableView->createResizeColumnsMenu();

                // Create resize buttons to toolbar.
                if (m_spResizeColumnsMenu && m_spToolBar)
                {
                    m_spToolBar->addSeparator();
                    m_spToolBar->addWidget(new QLabel(tr("Header resize: "), this)); // Deletion through parentship.
                    auto actions = m_spResizeColumnsMenu->actions();
                    for (int i = 0; i < actions.size(); ++i)
                    {
                        if (!actions[i])
                            continue;
                        auto pButton = new QToolButton(m_spToolBar.get()); // Deletion through parentship.
                        auto actionText = actions[i]->text();
                        int nDest = 1;

                        // Contructs string label from beginning characters of words
                        // TODO: Use icons instead of incomprehensible abbreviations.
                        {
                            for (int j = 1; j < actionText.size(); ++j)
                            {
                                if (actionText[j-1] == ' ' || actionText[j] == ':')
                                    actionText[nDest++] = actionText[j];
                            }
                            actionText.resize(nDest);
                        }

                        pButton->setDefaultAction(actions[i]);
                        pButton->setText(actionText);
                        m_spToolBar->addWidget(pButton);
                    }
                }
                //pResizeButton->setMenu(m_spResizeColumnsMenu.get());
                //pHl->addWidget(pResizeButton);
            }

            pHl->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding)); // pHl seems to take ownership.
            spLayout->addLayout(pHl.release(), row++, 0); // pHl becomes child of spLayout.
        } // 'Miscellenous control' -row

        // Find panel
        m_spFindPanel.reset(new DFG_DETAIL_NS::FindPanelWidget(tr("Find")));
        DFG_QT_VERIFY_CONNECT(connect(m_spFindPanel->m_pTextEdit, &QLineEdit::textChanged, this, &ThisClass::onHighlightTextChanged));
        DFG_QT_VERIFY_CONNECT(connect(m_spFindPanel->m_pColumnSelector, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ThisClass::onFindColumnChanged));
        DFG_QT_VERIFY_CONNECT(connect(m_spFindPanel->getCaseSensivitivyCheckBox(), &QCheckBox::stateChanged, this, &ThisClass::onHighlightTextCaseSensitivityChanged));
        spLayout->addWidget(m_spFindPanel.get(), row++, 0);

        // Filter panel
        {
            m_spFilterPanel.reset(new DFG_DETAIL_NS::FilterPanelWidget);
            DFG_QT_VERIFY_CONNECT(connect(m_spFilterPanel->m_pTextEdit, &QLineEdit::textChanged, this, &ThisClass::onFilterTextChanged));
            DFG_QT_VERIFY_CONNECT(connect(m_spFilterPanel->m_pColumnSelector, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ThisClass::onFilterColumnChanged));
            DFG_QT_VERIFY_CONNECT(connect(m_spFilterPanel->getCaseSensivitivyCheckBox(), &QCheckBox::stateChanged, this, &ThisClass::onFilterCaseSensitivityChanged));
            spLayout->addWidget(m_spFilterPanel.get(), row++, 0);
        }

        spLayout->addWidget(m_spSelectionAnalyzerPanel.get(), row++, 0);
        spLayout->addWidget(m_spStatusBar.get(), row++, 0);

        spLayout->setSpacing(0);
        auto pEditorWidget = new QWidget(this);
        delete pEditorWidget->layout();
        pEditorWidget->setLayout(spLayout.release());
        m_spMainSplitter->addWidget(pEditorWidget);

        delete layout();
        auto mainLayout = new QGridLayout(this);
        mainLayout->addWidget(m_spMainSplitter.get());
        setLayout(mainLayout);
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
    auto pModel = (m_spTableView) ? m_spTableView->csvModel() : nullptr;
    if (!pModel || pModel->isModified())
        return false;

    return m_spTableView->openFile(path);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::setWindowModified(bool bNewModifiedStatus)
{
    const auto bCurrentStatus = isWindowModified();
    if (bCurrentStatus != bNewModifiedStatus)
    {
        BaseClass::setWindowModified(bNewModifiedStatus);
        Q_EMIT sigModifiedStatusChanged(isWindowModified());
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::updateWindowTitle()
{
    // Note: [*] is a placeholder for modified-indicator (see QWidget::windowModified)
    QString sTitle = QString("%1 [*]").arg((m_spTableModel) ? m_spTableModel->getTableTitle(tr("Unsaved")) : QString());
    setWindowModified(m_spTableModel && m_spTableModel->isModified());
    setWindowTitle(sTitle);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onSourcePathChanged()
{
    if (!m_spTableModel || !m_spLineEditSourcePath)
        return;
    m_spLineEditSourcePath->setText(m_spTableModel->getFilePath());

    updateWindowTitle();
}

namespace
{
    void setCellEditorToNoSelectionState(QPlainTextEdit* pEditor, QDockWidget* pDockWidget)
    {
        if (pEditor)
        {
            pEditor->setPlainText("");
            pEditor->setEnabled(false);
        }
        if (pDockWidget)
            pDockWidget->setWindowTitle(pDockWidget->tr("Cell edit"));
    }
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
    setCellEditorToNoSelectionState(m_spCellEditor.get(), m_spCellEditorDockWidget.get());
    resizeColumnsToView();
    updateWindowTitle();

    if (m_spChartDisplay)
    {
        const auto loadOptions = model.getOpenTimeLoadOptions();
        const auto chartControls = loadOptions.getProperty(CsvOptionProperty_chartControls, "none");
        if (chartControls != "none")
            DFG_VERIFY(QMetaObject::invokeMethod(m_spChartDisplay.data(), "setChartControls", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(QString, QString::fromUtf8(chartControls.c_str()))));
    }
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

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected, const QItemSelection& edited)
{
    DFG_UNUSED(selected);
    DFG_UNUSED(deselected);
    DFG_UNUSED(edited);
    int selectedCount = -1;

    if (m_bHandlingOnCellEditorTextChanged) // To prevent cell editor change signal triggering reloading cell editor data from model. In practice this could cause e.g. cursor jumping to beginning after entering a letter to end.
        return;

    if (m_spCellEditor && m_spTableView && m_spTableModel && (selectedCount = m_spTableView->getSelectedItemCount()) == 1)
    {
        const QModelIndexList& indexes = m_spTableView->getSelectedItemIndexes_dataModel();
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
            // Getting here shouldn't happen, but has happened if table view has been in a corrupted state
            // where view row count and table row count has not been the same.
            DFG_ASSERT_CORRECTNESS(false);
            setCellEditorToNoSelectionState(m_spCellEditor.get(), m_spCellEditorDockWidget.get());
        }
    }
    else
    {
        setCellEditorToNoSelectionState(m_spCellEditor.get(), m_spCellEditorDockWidget.get());
    }

    // Update status bar
    if (m_spStatusBar)
    {
        if (selectedCount < 0)
            selectedCount =  m_spTableView->getSelectedItemCount();
        if (!m_spSelectionStatusInfo)
        {
            m_spSelectionStatusInfo.reset(new QLabel(this));
            m_spStatusBar->addPermanentWidget(m_spSelectionStatusInfo.get());
        }

        // Trailing spaces are on purpose to put some margin
        m_spSelectionStatusInfo->setText(tr("Selected count: %1  ").arg(selectedCount));
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
    {
        DFG_TABLEEDITOR_LOG_WARNING(QString("Requested resizeColumnsToView with unknown style '%1'").arg(static_cast<int>(style)));
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::setAllowApplicationSettingsUsage(const bool b)
{
    setProperty("dfglib_allow_app_settings_usage", b);

    // Re-apply properties that might have changed with change of this setting.
    {
        DFG_STATIC_ASSERT(TableEditorPropertyId_last == 1, "Check whether new properties should be handled here.");

        // cellEditorHeight
        setWidgetMaximumHeight(m_spCellEditorDockWidget.get(), this->height(), getTableEditorProperty<TableEditorPropertyId_cellEditorHeight>(this));

        // cellEditorFontPointSize
        if (m_spCellEditor)
            m_spCellEditor->setFontPointSizeF(getTableEditorProperty<TableEditorPropertyId_cellEditorFontPointSize>(this));
    }

    if (m_spTableModel)
        m_spTableModel->setProperty("dfglib_allow_app_settings_usage", b);
    if (m_spTableView)
        m_spTableView->setAllowApplicationSettingsUsage(b);
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::handleExitConfirmationAndReturnTrueIfCanExit()
{
    auto pView = m_spTableView.get();
    auto model = (pView) ? pView->csvModel() : nullptr;
    if (!model)
        return true;

    // Finish edits before checking modified as otherwise ongoing edit whose editor
    // has not been closed would not be detected.
    // In practice: user opens file, edits a cell and presses X while cell content editor is active,
    // without line below:
    // -> this function would be called before model's setData()-handler
    // -> Since no setData()-calls have been made, this function gets 'model->isModified() == false'
    //    and editor closes without confirmation.
    pView->finishEdits();

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
            pView->save();
    }

    return true;
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::closeEvent(QCloseEvent* event)
{
    if (!event)
    {
        BaseClass::closeEvent(event);
        return;
    }

    if (handleExitConfirmationAndReturnTrueIfCanExit())
        event->accept();
    else
        event->ignore();
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onCellEditorTextChanged()
{
    if (!m_spTableView)
        return;
    const auto& indexes = (m_spTableView->getSelectedItemCount() == 1) ? m_spTableView->getSelectedItemIndexes_dataModel() : QModelIndexList();
    if (m_spTableModel && m_spCellEditor && indexes.size() == 1)
    {
        const auto oldFlag = m_bHandlingOnCellEditorTextChanged;
        auto flagHandler = makeScopedCaller([&] { m_bHandlingOnCellEditorTextChanged = true ;}, [=] { m_bHandlingOnCellEditorTextChanged = oldFlag; });
        this->m_spTableModel->setData(indexes.front(), m_spCellEditor->toPlainText());
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onHighlightTextChanged(const QString& text)
{
    if (!m_spTableView || !m_spFindPanel)
        return;

    DFG_CLASS_NAME(StringMatchDefinition) matchDef(text, m_spFindPanel->getCaseSensitivity(), m_spFindPanel->getPatternSyntax());
    m_spTableView->setFindText(matchDef, m_spFindPanel->m_pColumnSelector->value());
    m_spTableView->onFindNext();
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onFilterTextChanged(const QString& text)
{
    if (!m_spFilterPanel)
        return;

    // Setting edit readonly as setFilterRegExp() may change selection which, at the time of writing, launched
    // event loop and if user edited the filter text while at it, this function would get called in re-entrant manner
    // causing a crash in setFilterRegExp().
    auto readOnlyGuard = makeScopedCaller( [&]() { m_spFilterPanel->m_pTextEdit->setReadOnly(true); },
                                           [&]() { m_spFilterPanel->m_pTextEdit->setReadOnly(false); });


    auto pProxy = (m_spTableView) ? qobject_cast<ProxyModelClass*>(m_spTableView->getProxyModelPtr()): nullptr;
    if (!pProxy || !m_spFilterPanel)
        return;

    auto lockReleaser = m_spTableView->tryLockForEdit();
    if (!lockReleaser.isLocked())
    {
        // Couldn't acquire lock. Scheduling a new try in 200 ms.
        QPointer<TableEditor> thisPtr = this;
        QTimer::singleShot(200, [=]() { if (thisPtr) thisPtr->onFilterTextChanged(thisPtr->m_spFilterPanel->getPattern()); });
        return;
    }
    QRegExp regExp(text, m_spFilterPanel->getCaseSensitivity(), m_spFilterPanel->getPatternSyntax());
    pProxy->setFilterRegExp(regExp);
    pProxy->setFilterKeyColumn(m_spFilterPanel->m_pColumnSelector->value());
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onFindColumnChanged(const int newCol)
{
    DFG_UNUSED(newCol);
	if (m_spFindPanel)
    	onHighlightTextChanged(m_spFindPanel->m_pTextEdit->text());
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onFilterColumnChanged(const int nNewCol)
{
    DFG_UNUSED(nNewCol);
    if (m_spFilterPanel)
        onFilterTextChanged(m_spFilterPanel->getPattern());
}

namespace
{
    static void activateFindTextEdit(QLineEdit* pEdit)
    {
        if (!pEdit)
            return;
        pEdit->setFocus(Qt::OtherFocusReason);
        pEdit->selectAll();
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onFindRequested()
{
    if (m_spFindPanel)
        activateFindTextEdit(m_spFindPanel->m_pTextEdit);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onFilterRequested()
{
    if (m_spFilterPanel)
        activateFindTextEdit(m_spFilterPanel->m_pTextEdit);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onHighlightTextCaseSensitivityChanged(const bool bCaseSensitive)
{
    DFG_UNUSED(bCaseSensitive);
    if (m_spFindPanel)
        onHighlightTextChanged(m_spFindPanel->m_pTextEdit->text());
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onFilterCaseSensitivityChanged(const bool bCaseSensitive)
{
    DFG_UNUSED(bCaseSensitive);
    if (m_spFilterPanel)
        onFilterTextChanged(m_spFilterPanel->getPattern());
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::addToolBarSeparator()
{
    if (m_spToolBar)
        m_spToolBar->addSeparator();
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::addToolBarWidget(QWidget* pWidget)
{
    if (m_spToolBar)
        m_spToolBar->addWidget(pWidget);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::setGraphDisplay(QWidget* pGraphDisplay)
{
    DFG_ASSERT_CORRECTNESS(m_spMainSplitter != nullptr);
    if (!pGraphDisplay || !m_spMainSplitter)
        return;
    m_spChartDisplay = pGraphDisplay;
    m_spMainSplitter->addWidget(pGraphDisplay);
}
