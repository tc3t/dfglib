#include "TableEditor.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"

#include "connectHelper.hpp"
#include "PropertyHelper.hpp"
#include "PatternMatcher.hpp"
#include "stringConversions.hpp"
#include "widgetHelpers.hpp"

#include "qtIncludeHelpers.hpp"
DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QAction>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QDockWidget>
#include <QGridLayout>
#include <QLineEdit>
#include <QStatusBar>
#include <QApplication>
#include <QGuiApplication>
#include <QCloseEvent>
#include <QScreen>
#include <QDesktopServices>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLabel>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QMetaMethod>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QToolBar>
#include <QToolButton>
#include <QSplitter>
#include <QTimer>
#include <QVector>
#include <QInputDialog>
DFG_END_INCLUDE_QT_HEADERS

#define DFG_TABLEEDITOR_LOG_DEBUG(x)   // Placeholder for logging debug-level message
#define DFG_TABLEEDITOR_LOG_WARNING(x) // Placeholder for logging warning

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {

namespace
{
    enum TableEditorPropertyId
    {
        // Add properties here in the beginning (or remember to update TableEditorPropertyId_last)
        TableEditorPropertyId_chartPanelWidth,
        TableEditorPropertyId_cellEditorFontPointSize,
        TableEditorPropertyId_cellEditorHeight,
        TableEditorPropertyId_selectionDetails,
        TableEditorPropertyId_selectionDetailsResultPrecision,
        TableEditorPropertyId_last = TableEditorPropertyId_selectionDetailsResultPrecision
    };

    // Class for window extent (=one dimension of window size) either as absolute value or as percentage of some widget size
    class WindowExtentProperty
    {
    public:
        WindowExtentProperty() :
            m_val(0)
        {
        }

        static WindowExtentProperty fromPercentage(int percentage)
        {
            WindowExtentProperty wep;
            DFG_ROOT_NS::limit(percentage, 0, 100);
            wep.m_val = -1 * percentage;
            return wep;
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
                auto strList = v.toStringList(); // comma-separated list can be interpreted as QStringList so checking that as well.
                if (!strList.isEmpty())
                    strVal = strList.first();
            }
            if (!strVal.isEmpty() && strVal[0] == '%')
            {
                strVal.remove(0, 1);
                auto pval = strVal.toInt(&ok);
                if (ok)
                    *this = fromPercentage(pval);
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

        int m_val; // Positive values are pixels, negative percentage.
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
                                              [] { return WindowExtentProperty(50); }, // Note: used also as default minimum cell editor size
                                              WindowExtentProperty);
    DFG_QT_DEFINE_OBJECT_PROPERTY("TableEditor_cellEditorFontPointSize",
                                              TableEditor,
                                              TableEditorPropertyId_cellEditorFontPointSize,
                                              double,
                                              defaultFontPointSizeForCellEditor);
    DFG_QT_DEFINE_OBJECT_PROPERTY_CUSTOM_TYPE("TableEditor_chartPanelWidth",
                                              TableEditor,
                                              TableEditorPropertyId_chartPanelWidth,
                                              WindowExtentProperty,
                                              [] { return WindowExtentProperty::fromPercentage(35); },
                                              WindowExtentProperty);
    DFG_QT_DEFINE_OBJECT_PROPERTY_QSTRING("TableEditorPropertyId_selectionDetails",
                                              TableEditor,
                                              TableEditorPropertyId_selectionDetails,
                                              QString,
                                              [] { return QString(); });
    DFG_QT_DEFINE_OBJECT_PROPERTY("TableEditorPropertyId_selectionDetailsResultPrecision",
                                              TableEditor,
                                              TableEditorPropertyId_selectionDetailsResultPrecision,
                                              int,
                                              [] { return -1; });


    template <TableEditorPropertyId ID>
    auto getTableEditorProperty(TableEditor* editor) -> typename DFG_QT_OBJECT_PROPERTY_CLASS_NAME(TableEditor)<ID>::PropertyType
    {
        return getProperty<DFG_QT_OBJECT_PROPERTY_CLASS_NAME(TableEditor)<ID>>(editor);
    }

    static int setChartPanelWidth(QSplitter* pSplitter, const int nParentWidth, const WindowExtentProperty& wep)
    {
        if (!pSplitter)
            return 0;
        QList<int> sizes;
        sizes.push_back(0);
        sizes.push_back(wep.toAbsolute(nParentWidth));
        sizes[0] = nParentWidth - sizes[1];
        pSplitter->setSizes(sizes);
        return sizes[1];
    }

    static void setCellEditorTitle(QDockWidget* pWidget, const QString& sAdditionalInfo = QString(), const bool bReadOnly = false)
    {
        if (!pWidget)
            return;
        auto sTitle = pWidget->tr("Cell edit%1").arg((bReadOnly) ? pWidget->tr(" (read-only)") : "");
        if (!sAdditionalInfo.isEmpty())
            sTitle += sAdditionalInfo;
        auto pTitleBarWidget = qobject_cast<TableEditor::CellEditorWidgetTitleBar*>(pWidget->titleBarWidget());
        if (pTitleBarWidget)
        {
            pTitleBarWidget->setTitleText(sTitle);
        }
        else
        {
            DFG_ASSERT_CORRECTNESS(false); // Not expected to reach this branch.
        }
    }

    const char celLEditorObjectProperty_shownModelIndex[] = "shown_model_index";

} // unnamed namespace

namespace DFG_DETAIL_NS {

    // TODO: move to CsvTableView.h?
    class FindPanelWidget : public QWidget
    {
    public:
        using HighlightTextEdit = LineEditWithValidityHighlighting;

        FindPanelWidget(const QString& label, const bool bAllowJsonSyntax = false)
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

            // Insert button
            {
                m_spJsonInsertButton.reset(new QToolButton());
                m_spJsonInsertButton->setHidden(true);
                m_spJsonInsertButton->setPopupMode(QToolButton::InstantPopup);
                m_spJsonInsertButton->setText(tr("Insert "));
                auto pMenu = new QMenu(this); // Deletion through parentship

                const auto addMenuItem = [&](const QString& sTitle, const QString& sInsertText)
                {
                    auto pAct = pMenu->addAction(sTitle);
                    DFG_QT_VERIFY_CONNECT(connect(pAct, &QAction::triggered, m_pTextEdit, [=]()
                    {
                        m_pTextEdit->setText(sInsertText);
                    }));
                };

                addMenuItem(tr("Minimal example"), TableStringMatchDefinition::jsonExampleMinimal());
                addMenuItem(tr("Full single object example"), TableStringMatchDefinition::jsonExampleFullSingle());
                addMenuItem(tr("Example with and/or logics"), R"({"text":"column 1 match", "apply_columns":"1", "and_group":"a"} {"text":"column 2 match", "apply_columns":"2", "and_group":"a"} {"text":"any column match"})");
                m_spJsonInsertButton->setMenu(pMenu); // Does not transfer ownership
                l->addWidget(m_spJsonInsertButton.get(), 0, nColumn++);
            }

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

                PatternMatcher::forEachPatternSyntax([&](int i, const PatternMatcher::PatternSyntax syntax)
                {
                    if (syntax != PatternMatcher::PatternSyntax::Json || bAllowJsonSyntax)
                    {
                        m_pMatchSyntaxCombobox->insertItem(i, tr(PatternMatcher::patternSyntaxName_untranslated(syntax)), syntax);
                        m_pMatchSyntaxCombobox->setItemData(i, tr(PatternMatcher::shortDescriptionForPatternSyntax_untranslated(syntax)), Qt::ToolTipRole);
                    }
                });
                m_pMatchSyntaxCombobox->setCurrentIndex(0);


                l->addWidget(m_pMatchSyntaxCombobox, 0, nColumn++);
            }

            // Column control
            {
                m_spColumnLabel.reset(new QLabel(tr("Column"), this));
                l->addWidget(m_spColumnLabel.get(), 0, nColumn++);
                m_pColumnSelector = new QSpinBox(this);
                const auto nMinValue = CsvItemModel::internalColumnToVisibleShift() - 1;
                m_pColumnSelector->setMinimum(nMinValue);
                m_pColumnSelector->setValue(nMinValue);
                m_pColumnSelector->setSpecialValueText(tr("Any")); // Sets text shown for minimum value.
                l->addWidget(m_pColumnSelector, 0, nColumn++);
            }

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

        PatternMatcher::PatternSyntax getPatternSyntax() const
        {
            const auto currentData = m_pMatchSyntaxCombobox->currentData();
            return static_cast<PatternMatcher::PatternSyntax>(currentData.toUInt());
        }

        void setPatternSyntax(const PatternMatcher::PatternSyntax syntax)
        {
            if (!m_pMatchSyntaxCombobox)
                return;
            const auto i = m_pMatchSyntaxCombobox->findData(static_cast<int>(syntax));
            if (i >= 0)
                m_pMatchSyntaxCombobox->setCurrentIndex(i);
        }

        void setSyntaxIndicator_good()
        {
            if (!m_pTextEdit)
                return;
            m_pTextEdit->setValidity(HighlightTextEdit::ValidityInfo(true));
        }

        void setSyntaxIndicator_bad(const QString& sErrorText)
        {
            if (!m_pTextEdit)
                return;
            m_pTextEdit->setValidity(HighlightTextEdit::ValidityInfo(false, sErrorText));
        }

        virtual void forEachConfigProperty(std::function<void(const CsvTableView::PropertyFetcher)>) const;

        virtual void setPropertiesFromLoadOptions(const CsvItemModel::LoadOptions& loadOptions);

        // Returned object is owned by 'this' and lives until the destruction of 'this'.
        QCheckBox* getCaseSensivitivyCheckBox() { return m_pCaseSensitivityCheckBox; }

        int getColumnIndex() const { return (m_pColumnSelector) ? m_pColumnSelector->value() : -1; }

        HighlightTextEdit* m_pTextEdit;
        QObjectStorage<QLabel> m_spColumnLabel;
        QSpinBox* m_pColumnSelector;
        QCheckBox* m_pCaseSensitivityCheckBox;
        QComboBox* m_pMatchSyntaxCombobox;
        QObjectStorage<QToolButton> m_spJsonInsertButton;
    }; // class FindPanelWidget

    void FindPanelWidget::forEachConfigProperty(std::function<void(CsvTableView::PropertyFetcher)> func) const
    {
        if (!func)
            return;
        using Pfp = CsvTableView::PropertyFetcherPair;
        QPointer<const FindPanelWidget> spThis = this;
#define DFG_TEMP_ADD_ITEM(PROP, ACCESS) func([spThis, this]() { return Pfp(QString("properties/%1").arg(PROP), (spThis) ? ACCESS : QString()); })
        DFG_TEMP_ADD_ITEM(CsvOptionProperty_findText,          this->getPattern());
        DFG_TEMP_ADD_ITEM(CsvOptionProperty_findCaseSensitive, QString::number(this->getCaseSensitivity()));
        DFG_TEMP_ADD_ITEM(CsvOptionProperty_findSyntaxType,    PatternMatcher::patternSyntaxName_untranslated(this->getPatternSyntax()));
        DFG_TEMP_ADD_ITEM(CsvOptionProperty_findColumn,        QString::number(this->getColumnIndex()));
#undef DFG_TEMP_ADD_ITEM
    }

    void FindPanelWidget::setPropertiesFromLoadOptions(const CsvItemModel::LoadOptions& loadOptions)
    {
        if (this->m_pTextEdit)
            this->m_pTextEdit->setText(QString::fromStdString(loadOptions.getProperty(CsvOptionProperty_findText, this->m_pTextEdit->text().toStdString())));
        if (this->m_pCaseSensitivityCheckBox)
            this->m_pCaseSensitivityCheckBox->setChecked(loadOptions.getPropertyThroughStrTo<bool>(CsvOptionProperty_findCaseSensitive, this->m_pCaseSensitivityCheckBox->isChecked()));
        if (this->m_pMatchSyntaxCombobox)
        {
            const auto existingSyntax = this->getPatternSyntax();
            const auto patternSyntaxAsString = PatternMatcher::patternSyntaxName_untranslated(existingSyntax);
            const auto syntax = PatternMatcher::patternSyntaxNameToEnum(loadOptions.getProperty(CsvOptionProperty_findSyntaxType, patternSyntaxAsString), existingSyntax);
            this->setPatternSyntax(syntax);
        }
        if (this->m_pColumnSelector)
            this->m_pColumnSelector->setValue(loadOptions.getPropertyThroughStrTo<bool>(CsvOptionProperty_findColumn, this->m_pColumnSelector->value()));
    }

    class FilterPanelWidget : public FindPanelWidget
    {
    public:
        typedef FindPanelWidget BaseClass;
        FilterPanelWidget()
            : BaseClass(tr("Filter"), true)
        {
            if (this->m_pMatchSyntaxCombobox)
                DFG_QT_VERIFY_CONNECT(connect(this->m_pMatchSyntaxCombobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterPanelWidget::onSyntaxChanged));
        }

        void onSyntaxChanged(const int index)
        {
            const auto itemData = (m_pMatchSyntaxCombobox) ? m_pMatchSyntaxCombobox->itemData(index) : QVariant();
            const bool bIsJson = (itemData.toInt() == PatternMatcher::Json);
            if (m_pColumnSelector)
                m_pColumnSelector->setHidden(bIsJson);
            if (m_pCaseSensitivityCheckBox)
                m_pCaseSensitivityCheckBox->setHidden(bIsJson);
            if (m_spColumnLabel)
                m_spColumnLabel->setHidden(bIsJson);
            if (m_spJsonInsertButton)
                m_spJsonInsertButton->setVisible(bIsJson);
        }

        void setPropertiesFromLoadOptions(const CsvItemModel::LoadOptions& loadOptions) override;
        void forEachConfigProperty(std::function<void(const CsvTableView::PropertyFetcher)>) const override;
    }; // class FilterPanelWidget

    void FilterPanelWidget::setPropertiesFromLoadOptions(const CsvItemModel::LoadOptions& loadOptions)
    {
        if (this->m_pCaseSensitivityCheckBox)
            this->m_pCaseSensitivityCheckBox->setChecked(loadOptions.getPropertyThroughStrTo<bool>(CsvOptionProperty_filterCaseSensitive, this->m_pCaseSensitivityCheckBox->isChecked()));
        if (this->m_pMatchSyntaxCombobox)
        {
            const auto existingSyntax = this->getPatternSyntax();
            const auto patternSyntaxAsString = PatternMatcher::patternSyntaxName_untranslated(existingSyntax);
            const auto syntax = PatternMatcher::patternSyntaxNameToEnum(loadOptions.getProperty(CsvOptionProperty_filterSyntaxType, patternSyntaxAsString), existingSyntax);
            this->setPatternSyntax(syntax);
        }
        if (this->m_pColumnSelector)
            this->m_pColumnSelector->setValue(loadOptions.getPropertyThroughStrTo<bool>(CsvOptionProperty_filterColumn, this->m_pColumnSelector->value()));
        if (this->m_pTextEdit)
            this->m_pTextEdit->setText(QString::fromStdString(loadOptions.getProperty(CsvOptionProperty_filterText, this->m_pTextEdit->text().toStdString())));
    }

    void FilterPanelWidget::forEachConfigProperty(std::function<void(const CsvTableView::PropertyFetcher)> func) const
    {
        if (!func)
            return;
        using Pfp = CsvTableView::PropertyFetcherPair;
        QPointer<const FilterPanelWidget> spThis = this;
#define DFG_TEMP_ADD_ITEM(PROP, ACCESS) func([spThis, this]() { return Pfp(QString("properties/%1").arg(PROP), (spThis) ? ACCESS : QString()); })
        DFG_TEMP_ADD_ITEM(CsvOptionProperty_filterText,           this->getPattern());
        DFG_TEMP_ADD_ITEM(CsvOptionProperty_filterSyntaxType,     PatternMatcher::patternSyntaxName_untranslated(this->getPatternSyntax()));
        DFG_TEMP_ADD_ITEM(CsvOptionProperty_filterCaseSensitive,  QString::number(this->getCaseSensitivity()));
        DFG_TEMP_ADD_ITEM(CsvOptionProperty_filterColumn,         QString::number(this->getColumnIndex()));
#undef DFG_TEMP_ADD_ITEM
    }

    class FileInfoToolButton : public QToolButton
    {
    public:
        using BaseClass = QToolButton;

        FileInfoToolButton(TableEditor* pParent)
            : BaseClass(pParent)
            , m_spTableEditor(pParent)
        {}

    protected:
        bool event(QEvent* pEvent) override
        {
            if (pEvent && pEvent->type() == QEvent::ToolTip)
            {
                // When getting tooltip event, updating tooltip text.
                if (m_spTableEditor)
                    m_spTableEditor->updateFileInfoToolTip();
            }
            return BaseClass::event(pEvent);
        }

        QPointer<TableEditor> m_spTableEditor;
    }; // class FileInfoToolButton

} // namespace DFG_DETAILS_NS (inside namespace dfg::qt)

DFG_OPAQUE_PTR_DEFINE(TableEditor)
{
    QMap<QModelIndex, QString> m_pendingEdits; // Stores edits done in cell editor which couldn't be applied immediately to be tried later.
    QPointer<QWidget> m_spResizeWindow;        // Defines widget to resize/move if document requests such.
    bool m_bIgnoreOnSelectionChanged = false;
    bool m_bIgnoreCellEditorTextChanged = false; // If true, onCellEditorTextChanged() will return without doing anything.
    QObjectStorage<QToolButton> m_spFileInfoButton;
};

void TableEditor::CellTextEditor::setFontPointSizeF(const qreal pointSize)
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

void TableEditor::CellTextEditor::setShownCellDetails(const QModelIndex& index)
{
    this->setProperty(celLEditorObjectProperty_shownModelIndex, index);
}

void DFG_DETAIL_NS::CellEditorWidgetTitleBarImpl::setTitleText(const QString& sText)
{
    auto pLabel = findChild<QLabel*>();
    if (pLabel)
        pLabel->setText(sText);
    else
    {
        DFG_ASSERT_IMPLEMENTED(false);
    }
}

TableEditor::TableEditor()
{
    // Model
    m_spTableModel.reset(new CsvItemModel);
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &CsvItemModel::sigSourcePathChanged, this, &ThisClass::onSourcePathChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &CsvItemModel::sigOnNewSourceOpened, this, &ThisClass::onNewSourceOpened));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &CsvItemModel::sigModifiedStatusChanged, this, &ThisClass::onModifiedStatusChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &CsvItemModel::sigOnSaveToFileCompleted, this, &ThisClass::onSaveCompleted));

    // View
    m_spTableView.reset(new ViewClass(m_spTableModel->getReadWriteLock(), this));
    // Proxy model
    {
        m_spProxyModel.reset(new ProxyModelClass(m_spTableView.get()));
        m_spProxyModel->setSourceModel(m_spTableModel.get());
        m_spProxyModel->setDynamicSortFilter(true);
    }
    m_spTableView->setModel(m_spProxyModel.get());
    std::unique_ptr<CsvTableViewBasicSelectionAnalyzerPanel> spAnalyzerPanel(new CsvTableViewBasicSelectionAnalyzerPanel(this));
    m_spTableView->addSelectionAnalyzer(std::make_shared<CsvTableViewBasicSelectionAnalyzer>(spAnalyzerPanel.get()));
    QPointer<CsvTableViewBasicSelectionAnalyzerPanel> spAnalyzerPanelCapture(spAnalyzerPanel.get());
    m_spSelectionAnalyzerPanel.reset(spAnalyzerPanel.release());
    m_spTableView->addConfigSavePropertyFetcher([=]() { return std::make_pair(QString("properties/selectionDetails"), spAnalyzerPanelCapture->detailConfigsToString()); });
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigSelectionChanged, this, &ThisClass::onSelectionChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigFindActivated, this, &ThisClass::onFindRequested));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigFilterActivated, this, &ThisClass::onFilterRequested));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigFilterJsonRequested, this, &ThisClass::setFilterJson));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigFilterToColumnRequested, this, &ThisClass::setFilterToColumn));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigReadOnlyModeChanged, this, &ThisClass::onViewReadOnlyModeChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigCellDelegateTextChanged, this, &ThisClass::onViewCellEditorTextEdited));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableView.get(), &ViewClass::sigCellEditorClosed, this, &ThisClass::onViewCellEditorClosed));

    // Setting default selection details from app settings.
    setSelectionDetailsFromIni(getTableEditorProperty<TableEditorPropertyId_selectionDetails>(this));
    m_spTableView->setAcceptDrops(true);

    // Source path line edit
    m_spLineEditSourcePath.reset(new QLineEdit());
    m_spLineEditSourcePath->setReadOnly(true);
    m_spLineEditSourcePath->setPlaceholderText(tr("<no file path available>"));

    // File info tool button
    {
        DFG_OPAQUE_REF().m_spFileInfoButton.reset(new DFG_DETAIL_NS::FileInfoToolButton(this));
        auto pButton = DFG_OPAQUE_REF().m_spFileInfoButton.get();

        pButton->setPopupMode(QToolButton::InstantPopup);
        pButton->setHidden(true);

        // Adding menu to button
        {
            auto pMenu = new QMenu(pButton); // Deletion through parentship
            const auto addMenuItem = [&](const QString& sTitle, const QString sIconPath, decltype(&TableEditor::onCopyFileInfoToClipboard) func)
            {
                auto pAct = pMenu->addAction(sTitle);
                if (pAct)
                {
                    DFG_QT_VERIFY_CONNECT(QObject::connect(pAct, &QAction::triggered, this, func));
                    if (!sIconPath.isEmpty())
                        pAct->setIcon(QIcon(sIconPath));
                }
            };

            addMenuItem(tr("Copy file info to clipboard"), QString(), &TableEditor::onCopyFileInfoToClipboard);
            addMenuItem(tr("Open directory in file manager"), QString(), &TableEditor::onOpenFileDirectoryInOsFileManager);
            pButton->setMenu(pMenu); // Does not transfer ownership
        }

        auto pStyle = QApplication::style();
        if (pStyle)
            pButton->setIcon(pStyle->standardIcon(QStyle::SP_MessageBoxInformation));

    }

    // Cell editor, its container and title bar widgets
    {
        m_spCellEditorDockWidget.reset(new QDockWidget(this));
        m_spCellEditorDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        m_spCellEditorDockWidget->setObjectName("Cell edit");

        m_spCellEditor.reset(new CellTextEditor(this));
        m_spCellEditor->setDisabled(true);
        m_spCellEditor->setFontPointSizeF(getTableEditorProperty<TableEditorPropertyId_cellEditorFontPointSize>(this));

        // Shortcut handling
        if (m_spTableView)
        {
            const auto addAction = [&](QWidget& rAddActionTargetAndParent,
                const QString& sShortCut,
                auto&& slot) -> QAction&
                {
                    auto pAction = new QAction(&rAddActionTargetAndParent);
                    if (!sShortCut.isEmpty())
                        pAction->setShortcut(sShortCut);
                    DFG_QT_VERIFY_CONNECT(QObject::connect(pAction, &QAction::triggered, &rAddActionTargetAndParent, slot));
                    pAction->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
                    rAddActionTargetAndParent.addAction(pAction); // Ownership of pAction is not transferred to rAddActionTargetAndParent.
                    return *pAction;
                };
                
            auto *pCe = m_spCellEditor.get();
#define DFG_TEMP_ADD_ACTION(SHORTCUT, DATA_FUNC) \
                addAction(*pCe, SHORTCUT, [pCe, this]() { if (m_spTableView) pCe->insertPlainText(m_spTableView->dateTimeToString(DATA_FUNC())); })

            // Note: Shortcuts should align with those defined in CsvTableView for the same insert actions.
            DFG_TEMP_ADD_ACTION(tr("Alt+Q"), QDate::currentDate);
            DFG_TEMP_ADD_ACTION(tr("Alt+W"), QTime::currentTime);
            DFG_TEMP_ADD_ACTION(tr("Shift+Alt+Q"), QDateTime::currentDateTime);
#undef DFG_TEMP_ADD_ACTION
        }

        // Creating custom title bar that has buttons.
        {
            auto pTitleBarWidget = new CellEditorWidgetTitleBar(this); // Deletion by parenthood
            auto pTitleBarLayout = new QBoxLayout(QBoxLayout::Direction::LeftToRight, pTitleBarWidget); // Deletion by parenthood
            pTitleBarLayout->setContentsMargins(0, 0, 0, 0);
            // Tweaking title bar colour a bit (getting colour of default title bar widget didn't seem to be too easy -> tweaking/hacking the colour manually)
            {
                auto titleWidgetPalette = pTitleBarWidget->palette();
                const auto defaultWindowColour = titleWidgetPalette.color(QPalette::Window);
                pTitleBarWidget->setStyleSheet(QString("background-color: %1;").arg(defaultWindowColour.darker(110).name()));
            }

            auto pTextLabel = new QLabel(tr("Cell edit"), this);
            pTextLabel->setContentsMargins(0, 0, 0, 0);

            // Layout in title bar (horizontal):
            //    text label, buttons
            pTitleBarLayout->addWidget(pTextLabel);
            //pTitleBarLayout->addStretch(); // This can be used to add spacer between label and buttons.

            QPointer<QBoxLayout> spTitleBarLayout = pTitleBarLayout;
            const auto addButton = [this, &spTitleBarLayout](const QString& sIconPathOrText, const QString sToolTip, auto clickHandler)
                {
                    if (!spTitleBarLayout)
                        return;
                    auto pButton = new QPushButton(m_spCellEditorDockWidget.get()); // Deletion by parenthood
                    DFG_QT_VERIFY_CONNECT(connect(pButton, &QPushButton::clicked, m_spCellEditorDockWidget.get(), clickHandler));
                    if (sIconPathOrText.startsWith(":/"))
                        pButton->setIcon(QIcon(sIconPathOrText));
                    else
                        pButton->setText(sIconPathOrText);
                    pButton->setContentsMargins(0, 0, 0, 0);
                    pButton->setToolTip(sToolTip);
                    pButton->setMaximumWidth(25);
                    spTitleBarLayout->addWidget(pButton);
                };
            // Default size -button
            addButton(":/resources/defaultSize.png", tr("Set default cell editor height"), [this]()
                {
                    DFG_TABLEEDITOR_LOG_DEBUG("CellEditor 'default size'-action triggered");
                    const auto defaultHeight = getTableEditorProperty<TableEditorPropertyId_cellEditorHeight>(this);
                    setWidgetMaximumHeight(m_spCellEditorDockWidget.get(), this->height(), defaultHeight);
                    this->update();
                });
            // Resize-button
            addButton("rz", tr("Set custom cell editor height"), [this]()
                {
                    DFG_TABLEEDITOR_LOG_DEBUG("CellEditor 'resize'-action triggered");
                    if (!m_spCellEditorDockWidget || !m_spTableView)
                        return;
                    const auto nCurrentHeight = m_spCellEditorDockWidget->height();
                    const auto nMinimumSize = getTableEditorProperty<TableEditorPropertyId_cellEditorHeight>(nullptr).toAbsolute(0);
                    const auto nTableAndEditorHeights = m_spTableView->height() + m_spCellEditorDockWidget->height();
                    // Note: if want to set height to more than half of tableview+cellEditor, probably need to go through layout.
                    const auto nMaximumSize = nTableAndEditorHeights / 2;
                    bool bOk = false;
                    const auto nNewSize = QInputDialog::getInt(this,
                        tr("Cell editor height"),
                        tr("New cell editor height, in range [%1, %2]").arg(nMinimumSize).arg(nMaximumSize),
                        nCurrentHeight,
                        nMinimumSize,
                        nMaximumSize,
                        1,
                        &bOk);
                    if (!bOk)
                        return;
                    setWidgetMaximumHeight(m_spCellEditorDockWidget.get(), 0, WindowExtentProperty(nNewSize));
                    this->update();
                });
            pTitleBarWidget->setContentsMargins(0, 0, 0, 0);
            pTitleBarWidget->setFixedHeight(21);
            m_spCellEditorDockWidget->setTitleBarWidget(pTitleBarWidget); // Does not take ownership.
        }

        m_spCellEditorDockWidget->setWidget(m_spCellEditor.get());

        DFG_QT_VERIFY_CONNECT(connect(m_spCellEditor.get(), &QPlainTextEdit::textChanged, this, &ThisClass::onCellEditorTextChanged));
    } // Cell editor widgets

    // Status bar
    {
        m_spStatusBar.reset(new TableEditorStatusBar);
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

            firstRowLayout->addWidget(DFG_OPAQUE_REF().m_spFileInfoButton.get());

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
        DFG_QT_VERIFY_CONNECT(connect(m_spFindPanel->m_pMatchSyntaxCombobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ThisClass::onFindColumnChanged));
        DFG_QT_VERIFY_CONNECT(connect(m_spFindPanel->getCaseSensivitivyCheckBox(), &QCheckBox::stateChanged, this, &ThisClass::onHighlightTextCaseSensitivityChanged));
        spLayout->addWidget(m_spFindPanel.get(), row++, 0);

        // Filter panel
        {
            m_spFilterPanel.reset(new DFG_DETAIL_NS::FilterPanelWidget);
        #if (DFG_CSVTABLEVIEW_FILTER_PROXY_AVAILABLE == 1)
            DFG_QT_VERIFY_CONNECT(connect(m_spFilterPanel->m_pTextEdit, &QLineEdit::textChanged, this, &ThisClass::onFilterTextChanged));
            DFG_QT_VERIFY_CONNECT(connect(m_spFilterPanel->m_pColumnSelector, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ThisClass::onFilterColumnChanged));
            DFG_QT_VERIFY_CONNECT(connect(m_spFilterPanel->m_pMatchSyntaxCombobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ThisClass::onFilterColumnChanged));
            DFG_QT_VERIFY_CONNECT(connect(m_spFilterPanel->getCaseSensivitivyCheckBox(), &QCheckBox::stateChanged, this, &ThisClass::onFilterCaseSensitivityChanged));
        #else
            m_spFilterPanel->m_pTextEdit->setText(tr("Not available; requires building with Qt >= 5.12"));
            m_spFilterPanel->m_pTextEdit->setEnabled(false);
            m_spFilterPanel->m_pCaseSensitivityCheckBox->setEnabled(false);
            m_spFilterPanel->m_pColumnSelector->setEnabled(false);
            m_spFilterPanel->m_pMatchSyntaxCombobox->setEnabled(false);
        #endif
            spLayout->addWidget(m_spFilterPanel.get(), row++, 0);
        }

        // Adding config property fetchers for find and filter panels
        {
            this->m_spFindPanel->forEachConfigProperty([&](const auto& propertyFetcher) { m_spTableView->addConfigSavePropertyFetcher(propertyFetcher); });
            this->m_spFilterPanel->forEachConfigProperty([&](const auto& propertyFetcher) { m_spTableView->addConfigSavePropertyFetcher(propertyFetcher); });
        }

        spLayout->addWidget(m_spSelectionAnalyzerPanel.get(), row++, 0);
        spLayout->addWidget(m_spStatusBar.get(), row++, 0);

        spLayout->setSpacing(0);
        auto pEditorWidget = new CellEditorWidget(this);
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
        auto pPrimaryScreen = QGuiApplication::primaryScreen();
        if (pPrimaryScreen)
        {
            const auto screenRect = pPrimaryScreen->geometry();
            resize(screenRect.size() * 0.75); // Fill 0.75 of the whole screen in both directions (arbitrary default value)
        }
    }

    // Resize widgets
    setWidgetMaximumHeight(m_spCellEditorDockWidget.get(), this->height(), getTableEditorProperty<TableEditorPropertyId_cellEditorHeight>(this));

    resizeColumnsToView();
}

TableEditor::~TableEditor()
{
    m_spCellEditorDockWidget.release();
}

auto TableEditor::tableView() -> ViewClass*
{
    return this->m_spTableView.get();
}

auto TableEditor::selectionDetailPanel() -> QWidget*
{
    return this->m_spSelectionAnalyzerPanel.get();
}

bool TableEditor::tryOpenFileFromPath(QString path)
{
    auto pModel = (m_spTableView) ? m_spTableView->csvModel() : nullptr;
    if (!pModel || pModel->isModified())
        return false;

    return m_spTableView->openFile(path);
}

void TableEditor::setWindowModified(bool bNewModifiedStatus)
{
    const auto bCurrentStatus = isWindowModified();
    if (bCurrentStatus != bNewModifiedStatus)
    {
        BaseClass::setWindowModified(bNewModifiedStatus);
        Q_EMIT sigModifiedStatusChanged(isWindowModified());
    }
}

void TableEditor::updateWindowTitle()
{
    // Note: [*] is a placeholder for modified-indicator (see QWidget::windowModified)
    QString sTitle = QString("%1 [*]").arg((m_spTableModel) ? m_spTableModel->getTableTitle(tr("Unsaved")) : QString());
    setWindowModified(m_spTableModel && m_spTableModel->isModified());
    setWindowTitle(sTitle);
}

void TableEditor::onSourcePathChanged()
{
    if (!m_spTableModel || !m_spLineEditSourcePath)
        return;
    m_spLineEditSourcePath->setText(m_spTableModel->getFilePath());

    updateWindowTitle();
}

void TableEditor::setResizeWindow(QWidget* pWindow)
{
    DFG_OPAQUE_REF().m_spResizeWindow = pWindow;
}

bool TableEditor::toggleCellEditorVisibility()
{
    if (!m_spCellEditorDockWidget)
        return false;
    m_spCellEditorDockWidget->setVisible(!m_spCellEditorDockWidget->isVisible());
    return m_spCellEditorDockWidget->isVisible();
}

namespace
{
    void setCellEditorToNoSelectionState(QPlainTextEdit* pEditor, QDockWidget* pDockWidget, const bool bReadOnly)
    {
        if (pEditor)
        {
            pEditor->setPlainText(QString());
            pEditor->setEnabled(false);
        }
        setCellEditorTitle(pDockWidget, QString(), bReadOnly);
    }
}

void TableEditor::setSelectionDetails(const StringViewC& sv, const int nResultPrecision)
{
    auto pDetailPanel = qobject_cast<CsvTableViewBasicSelectionAnalyzerPanel*>(m_spSelectionAnalyzerPanel.get());
    if (pDetailPanel)
    {
        if (!sv.empty())
        {
            pDetailPanel->clearAllDetails();
            using DelimitedTextReader = ::DFG_MODULE_NS(io)::DelimitedTextReader;
            const auto metaNone = DelimitedTextReader::s_nMetaCharNone;
            ::DFG_MODULE_NS(io)::BasicImStream istrm(sv.data(), sv.size());
            DelimitedTextReader::readRow<char>(istrm, '\n', metaNone, metaNone, [&](Dummy, const char* psz, const size_t nCount)
            {
                auto doc = QJsonDocument::fromJson(QByteArray::fromRawData(psz, saturateCast<int>(nCount)));
                auto obj = doc.object();
                pDetailPanel->addDetail(obj.toVariantMap());
            });
        }
        const auto nDefaultPrecision = (nResultPrecision == -2) ? getTableEditorProperty<TableEditorPropertyId_selectionDetailsResultPrecision>(this) : nResultPrecision;
        pDetailPanel->setDefaultNumericPrecision(nDefaultPrecision);
    }
}

namespace
{
    static void convertSpaceSeparatedJsonListToNewLineSeparated(QByteArray& utf8, std::function<void(const QByteArray&, const QJsonParseError&, int)> parseErrorhandler)
    {
        // Parsing all json items present in the text field; different json-objects are detected with QJsonError::GarbageAtEnd
        int nStart = 0;
        while (nStart < utf8.size())
        {
            QJsonParseError parseError;
            QJsonDocument::fromJson(utf8.mid(nStart), &parseError);
            if (parseError.error == QJsonParseError::GarbageAtEnd && parseError.offset != 0)
            {
                nStart += parseError.offset;
                if (utf8[nStart - 1] != '}')
                    utf8[nStart - 1] = '\n'; // Separating different json-objects by new-line, which is the syntax that proxy filter expects.
            }
            else if (parseError.error != QJsonParseError::NoError)
            {
                const auto nUtf8Offset = nStart + parseError.offset;
                if (parseErrorhandler)
                    parseErrorhandler(utf8, parseError, nUtf8Offset);
                return;
            }
            else
                nStart = ::DFG_ROOT_NS::saturateCast<decltype(nStart)>(utf8.size());
        }
    }
}

void TableEditor::setSelectionDetailsFromIni(const QString& s)
{
    // Ini-property is expected to have space-separated json objects
    if (!s.isEmpty())
    {
        auto utf8 = s.toUtf8();
        convertSpaceSeparatedJsonListToNewLineSeparated(utf8, nullptr);
        setSelectionDetails(StringViewC(utf8.data(), static_cast<size_t>(utf8.size())));
    }
}

namespace
{
    enum class FileInfoFormatType
    {
        clipboard,
        tooltip
    };

    static QVector<QPair<QString, QString>> getFileInfoFields(const QObject& trObj, const ::DFG_MODULE_NS(qt)::TableEditor::ModelClass& model)
    {
        const auto sFilePath = model.getFilePath();
        const auto dateTimeToString = [](const QDateTime& dateTime) { return dateTime.toString("yyyy-MM-dd hh:mm:ss"); };
        if (sFilePath.isEmpty())
            return QVector<QPair<QString, QString>>();
        const QFileInfo fi(sFilePath);
        const auto fileEncoding = model.getFileEncoding();
        const auto yesNoText = [&](const bool b) { return (b) ? trObj.tr("yes") : trObj.tr("no"); };
        const QString sEncoding = (fileEncoding != ::DFG_MODULE_NS(io)::encodingUnknown) ? ::DFG_MODULE_NS(io)::encodingToStrId(fileEncoding) : trObj.tr("unknown (note: detection is based only on UTF BOM)");
        const QFileInfo targetFileInfo = (fi.isSymLink()) ? QFileInfo(fi.symLinkTarget()) : fi;

        QVector<QPair<QString, QString>> fields;
        const auto addField = [&](const QString& sKey, const QString& sValue)
            {
                fields.push_back(qMakePair(sKey, sValue));
            };
        if (fi.isSymLink())
        {
            fields.push_back(qMakePair(trObj.tr("Symlink path"), fi.absoluteFilePath()));
            fields.push_back(qMakePair(trObj.tr("Target file path"), targetFileInfo.absoluteFilePath()));
        }
        else
        {
            fields.push_back(qMakePair(trObj.tr("File path"), targetFileInfo.absoluteFilePath()));
        }

        fields.push_back(qMakePair(trObj.tr("File size"), trObj.tr("%1 (%2 bytes)").arg(formattedDataSize(targetFileInfo.size(), 2), QString::number(targetFileInfo.size()))));
        fields.push_back(qMakePair(trObj.tr("Created"),
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
            dateTimeToString(targetFileInfo.birthTime())));
#else
            dateTimeToString(targetFileInfo.created())));
#endif
        fields.push_back(qMakePair(trObj.tr("Last modified"), dateTimeToString(targetFileInfo.lastModified())));
        fields.push_back(qMakePair(trObj.tr("Hidden"), yesNoText(targetFileInfo.isHidden())));
        fields.push_back(qMakePair(trObj.tr("Writable"), yesNoText(targetFileInfo.isWritable())));
        fields.push_back(qMakePair(trObj.tr("File encoding"), sEncoding));

        // Fields describing save format
        {
            const auto saveOptions = model.getSaveOptions();
            addField(trObj.tr("Save separator char"), QString::fromStdString(saveOptions.separatorCharAsString()));
            const auto enclosementBehaviour = saveOptions.enclosementBehaviour();
            //using enum ::DFG_MODULE_NS(io)::EnclosementBehaviour; // Requires >= C++20
            QString enclosementStr;
            switch (enclosementBehaviour)
            {
                case ::DFG_MODULE_NS(io)::EnclosementBehaviour::EbEnclose:
                    enclosementStr = trObj.tr("Always");       break;
                case ::DFG_MODULE_NS(io)::EnclosementBehaviour::EbNoEnclose:
                    enclosementStr = trObj.tr("Disabled (note: if table content is not compatible with this setting, saving may produce malformed file)"); break;
                case ::DFG_MODULE_NS(io)::EnclosementBehaviour::EbEncloseIfNeeded:
                    enclosementStr = trObj.tr("If needed");    break;
                case ::DFG_MODULE_NS(io)::EnclosementBehaviour::EbEncloseIfNonEmpty:
                    enclosementStr = trObj.tr("If non-empty"); break;
                default:
                    enclosementStr = trObj.tr("Unknown"); DFG_ASSERT_IMPLEMENTED(false); break;
            }
            addField(trObj.tr("Save enclosing behaviour"), enclosementStr);
            if (enclosementBehaviour != ::DFG_MODULE_NS(io)::EnclosementBehaviour::EbNoEnclose)
                addField(trObj.tr("Save enclosing char"), QString::fromStdString(saveOptions.enclosingCharAsString()));
            addField(trObj.tr("Save EOL"), untypedViewToQStringAsUtf8(saveOptions.eolTypeAsString()));
            addField(trObj.tr("Save encoding"), untypedViewToQStringAsUtf8(saveOptions.textEncodingAsString()));
        }
        return fields;
    }

    static QString createFileInfoText(const QObject* pTrObj, const ::DFG_MODULE_NS(qt)::TableEditor::ModelClass* pModel, const FileInfoFormatType formatType)
    {
        if (!pTrObj || !pModel)
            return QString();
        QString sInfoText;
        const QObject& trObj = *pTrObj;
        auto& model = *pModel;
        const auto sFilePath = model.getFilePath();

        if (!sFilePath.isEmpty())
        {
            const QFileInfo fi(sFilePath);
            if (fi.exists())
            {
                const auto fields = getFileInfoFields(trObj, model);

                if (formatType == FileInfoFormatType::tooltip)
                {
                    sInfoText += "<table>";
                    for (const auto& item : qAsConst(fields))
                    {
                        sInfoText.append(QString("<tr><td>%1</td><td>%2</td></tr>").arg(item.first, item.second));
                    }
                    sInfoText += "</table>";
                }
                else if (formatType == FileInfoFormatType::clipboard)
                {
                    // For clipboard formatting as CSV
                    for (const auto& item : qAsConst(fields))
                    {
                        sInfoText.append(QString("%1\t%2\n").arg(item.first, item.second));
                    }
                }
                else
                {
                    DFG_ASSERT_IMPLEMENTED(false);
                }
            }
            else // case: File does not exist
                sInfoText = trObj.tr("No file exists in path %1").arg(sFilePath);
        }
        return sInfoText;
    }

    static void updateFileInfoToolTipImpl(QToolButton* pButton, const ::DFG_MODULE_NS(qt)::TableEditor::ModelClass* pModel)
    {
        if (!pButton)
            return;

        const auto sToolTipText = createFileInfoText(pButton, pModel, FileInfoFormatType::tooltip);
        pButton->setVisible(!sToolTipText.isEmpty());
        pButton->setToolTip(sToolTipText);
    }
} // unnamed namespace

QVector<QPair<QString, QString>> TableEditor::getToolTipFileInfoFields() const
{
    auto pModel = this->m_spTableModel.get();
    if (!pModel)
        return {};
    return getFileInfoFields(*this, *pModel);
}

void TableEditor::updateFileInfoToolTip()
{
    updateFileInfoToolTipImpl(DFG_OPAQUE_REF().m_spFileInfoButton.get(), this->m_spTableModel.get());
}

QVariantMap TableEditor::getFindFilterPanelSettingsImpl(const DFG_DETAIL_NS::FindPanelWidget& rWidget) const
{
    QVariantMap items;
    rWidget.forEachConfigProperty([&](const auto& fetcher)
        {
            auto kv = fetcher();
            DFG_ASSERT_CORRECTNESS(kv.first.startsWith("properties/"));
            items.insert(kv.first.mid(11), kv.second);
        });
    return items;
}

QVariantMap TableEditor::getFindPanelSettings() const
{
    auto pFindPanel = this->m_spFindPanel.get();
    return (pFindPanel) ? getFindFilterPanelSettingsImpl(*pFindPanel) : QVariantMap();
}

QVariantMap TableEditor::getFilterPanelSettings() const
{
    auto pFilterPanel = this->m_spFilterPanel.get();
    return (pFilterPanel) ? getFindFilterPanelSettingsImpl(*pFilterPanel) : QVariantMap();
}

void TableEditor::onCopyFileInfoToClipboard()
{
    const auto sInfoText = createFileInfoText(DFG_OPAQUE_REF().m_spFileInfoButton.get(), m_spTableModel.get(), FileInfoFormatType::clipboard);
    auto pClipboard = QApplication::clipboard();
    if (pClipboard)
        pClipboard->setText(sInfoText);
}

void TableEditor::onOpenFileDirectoryInOsFileManager()
{
    auto pModel = m_spTableModel.get();
    if (!pModel)
        return;
    auto& model = *pModel;
    const auto sFilePath = model.getFilePath();
    if (sFilePath.isEmpty())
        return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(sFilePath).absolutePath()));
}

void TableEditor::onNewSourceOpened()
{
    if (!m_spTableModel)
        return;
    auto& model = *m_spTableModel;
    if (m_spLineEditSourcePath)
        m_spLineEditSourcePath->setText(model.getFilePath());
    if (m_spStatusBar)
        m_spStatusBar->showMessage(tr("Reading lasted %1 s").arg(model.latestReadTimeInSeconds(), 0, 'g', 4));
    setCellEditorToNoSelectionState(m_spCellEditor.get(), m_spCellEditorDockWidget.get(), (m_spTableView) ? m_spTableView->isReadOnlyMode() : true);

    const auto loadOptions = model.getOpenTimeLoadOptions();

    // Checking if window resize is allowed and requested; if yes, maximizing or resizing & repositioning window.
    auto pResizeWindow = DFG_OPAQUE_REF().m_spResizeWindow.data();
    if (pResizeWindow)
    {
        auto pPrimaryScreen = QGuiApplication::primaryScreen();
        if (pPrimaryScreen)
        {
            const bool bMaximize = loadOptions.getPropertyThroughStrTo(CsvOptionProperty_windowMaximized, false);

            if (bMaximize)
                pResizeWindow->showMaximized();
            else
            {
                const auto screenRect = pPrimaryScreen->geometry();
                const WindowExtentProperty heightRequest(QString::fromStdString(loadOptions.getProperty(CsvOptionProperty_windowHeight, "")));
                const WindowExtentProperty widthRequest(QString::fromStdString(loadOptions.getProperty(CsvOptionProperty_windowWidth, "")));
                const auto nMaxHeight = screenRect.height();
                const auto nMaxWidth = screenRect.width();
                auto nNewHeight = Min(heightRequest.toAbsolute(nMaxHeight), nMaxHeight);
                auto nNewWidth = Min(widthRequest.toAbsolute(nMaxWidth), nMaxWidth);

                if (nNewWidth > 0 || nNewHeight > 0)
                {
                    const auto posFunc = [&](const char* pszId, const int nNewExtent, const int nWindowExtent, const int pos)
                    {
                        const auto sPos = loadOptions.getProperty(pszId, "");
                        if (sPos.empty())
                            return (nNewExtent > 0) ? (nWindowExtent - nNewExtent) / 2 : pos;
                        else
                            return Min(nWindowExtent, Max(0, ::DFG_MODULE_NS(str)::strTo<int>(sPos)));
                    };

                    const auto xPos = posFunc(CsvOptionProperty_windowPosX, nNewWidth, screenRect.width(), pResizeWindow->pos().x());
                    const auto yPos = posFunc(CsvOptionProperty_windowPosY, nNewHeight, screenRect.height(), pResizeWindow->pos().y());

                    if (nNewWidth == 0)
                        nNewWidth = pResizeWindow->width();
                    if (nNewHeight == 0)
                        nNewHeight = pResizeWindow->height();
                    pResizeWindow->resize(nNewWidth, nNewHeight);
                    pResizeWindow->move(xPos, yPos);
                }
            }
        }
    }

    resizeColumnsToView();
    updateWindowTitle();
    updateFileInfoToolTip();

    if (m_spChartDisplay)
    {
        // Resizing chart panel if needed
        if (m_spChartDisplay)
        {
            const auto sChartPanelWidth = loadOptions.getProperty(CsvOptionProperty_chartPanelWidth, "");
            if (!sChartPanelWidth.empty())
            {
                const int nWidth = setChartPanelWidth(m_spMainSplitter.get(), this->width(), WindowExtentProperty(sChartPanelWidth.c_str()));
                // Enabling charting if width is non-zero, disabling if zero.
                DFG_VERIFY(QMetaObject::invokeMethod(m_spChartDisplay.data(), "setChartingEnabledState", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(bool, nWidth != 0)));
            }
        }

        const auto chartControls = loadOptions.getProperty(CsvOptionProperty_chartControls, "none");
        if (chartControls != "none")
            DFG_VERIFY(QMetaObject::invokeMethod(m_spChartDisplay.data(), "setChartControls", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(QString, QString::fromUtf8(chartControls.c_str()))));
    }

    // Setting selection details
    setSelectionDetails(loadOptions.getProperty(CsvOptionProperty_selectionDetails, ""), loadOptions.getPropertyThroughStrTo<int>(CsvOptionProperty_selectionDetailsPrecision, -2));

    // Setting find and filter options. using QTimer to let CsvTableView initialize itself properly.
    {
        if (this->m_spFindPanel)
            this->m_spFindPanel->setPropertiesFromLoadOptions(loadOptions);
        if (this->m_spFilterPanel)
            this->m_spFilterPanel->setPropertiesFromLoadOptions(loadOptions);
        auto pView = this->tableView();
        // Hack: if proxy model is null, view might not have initialized itself properly yet and filter might not be effective,
        //       so triggering a manual update a bit later.
        if (pView && pView->getProxyModelPtr() == nullptr)
            QTimer::singleShot(50, this, [&]() { if (this->m_spFilterPanel) this->onFilterTextChanged(this->m_spFilterPanel->getPattern()); });
    }
}

void TableEditor::onSaveCompleted(const bool success, const double saveTimeInSeconds)
{
    if (!m_spStatusBar)
        return;
    if (success)
    {
        m_spStatusBar->showMessage(tr("Saving done in %1 s").arg(saveTimeInSeconds, 0, 'g', 4));
        updateFileInfoToolTip();
    }
    else
        m_spStatusBar->showMessage(tr("Saving failed lasting %1 s").arg(saveTimeInSeconds, 0, 'g', 4));
}

void TableEditor::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected, const QItemSelection& edited)
{
    DFG_UNUSED(selected);
    DFG_UNUSED(deselected);
    DFG_UNUSED(edited);
    int selectedCount = -1;

    if (DFG_OPAQUE_REF().m_bIgnoreOnSelectionChanged) // To prevent cell editor change signal triggering reloading cell editor data from model. In practice this could cause e.g. cursor jumping to beginning after entering a letter to end.
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
            m_spCellEditor->setPlainText(model.data(index, Qt::EditRole).toString());

            auto colDescription = model.headerData(index.column(), Qt::Horizontal).toString();
            if (colDescription.isEmpty())
                colDescription = QString::number(ModelClass::internalColumnIndexToVisible(index.column()));
            const QString sAddInfo = QString(" (%1, %2)")
                                .arg(model.headerData(index.row(), Qt::Vertical).toString(), colDescription);
            const auto bReadOnly = m_spTableView->isReadOnlyMode() || !model.isCellEditable(index);
            setCellEditorTitle(m_spCellEditorDockWidget.get(), sAddInfo, bReadOnly);
            m_spCellEditor->setReadOnly(bReadOnly);
            m_spCellEditor->setShownCellDetails(index);
        }
        else
        {
            // Getting here shouldn't happen, but has happened if table view has been in a corrupted state
            // where view row count and table row count has not been the same.
            DFG_ASSERT_CORRECTNESS(false);
            setCellEditorToNoSelectionState(m_spCellEditor.get(), m_spCellEditorDockWidget.get(), m_spTableView->isReadOnlyMode());
            m_spCellEditor->setShownCellDetails(QModelIndex());
        }
    }
    else
    {
        setCellEditorToNoSelectionState(m_spCellEditor.get(), m_spCellEditorDockWidget.get(), (m_spTableView) ? m_spTableView->isReadOnlyMode() : true);
        m_spCellEditor->setShownCellDetails(QModelIndex());
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

void TableEditor::onModifiedStatusChanged(const bool b)
{
    setWindowModified(b);
}

void TableEditor::resizeColumnsToView(ColumnResizeStyle style)
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

void TableEditor::setAllowApplicationSettingsUsage(const bool b)
{
    setProperty(gPropertyIdAllowAppSettingsUsage, b);

    // Re-apply properties that might have changed with change of this setting.
    {
        DFG_STATIC_ASSERT(TableEditorPropertyId_last == 4, "Check whether new properties should be handled here.");

        // cellEditorHeight
        setWidgetMaximumHeight(m_spCellEditorDockWidget.get(), this->height(), getTableEditorProperty<TableEditorPropertyId_cellEditorHeight>(this));

        // cellEditorFontPointSize
        if (m_spCellEditor)
            m_spCellEditor->setFontPointSizeF(getTableEditorProperty<TableEditorPropertyId_cellEditorFontPointSize>(this));

        // Setting default selection details from app settings.
        setSelectionDetailsFromIni(getTableEditorProperty<TableEditorPropertyId_selectionDetails>(this));
    }

    if (m_spTableModel)
        m_spTableModel->setProperty(gPropertyIdAllowAppSettingsUsage, b);
    if (m_spTableView)
        m_spTableView->setAllowApplicationSettingsUsage(b);
}

bool TableEditor::handleExitConfirmationAndReturnTrueIfCanExit()
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
                                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (rv == QMessageBox::Cancel)
            return false;

        if (rv == QMessageBox::Yes)
            pView->save();
    }

    return true;
}

void TableEditor::closeEvent(QCloseEvent* event)
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

void TableEditor::handlePendingEdits()
{
    if (!m_spTableView || !this->m_spTableModel)
        return;
    auto lockReleaser = m_spTableView->tryLockForEdit();
    if (!lockReleaser.isLocked())
    {
        // Couldn't acquire lock. Scheduling a new try in 100 ms.
        QPointer<TableEditor> thisPtr = this;
        QTimer::singleShot(100, this, [=]() { if (thisPtr) thisPtr->handlePendingEdits(); });
        return;
    }
    auto& edits = DFG_OPAQUE_REF().m_pendingEdits;
    while (!edits.empty())
    {
        // Note: trusting that there has been no structural edits (remove rows/columns) since the original cell edit; those could make the row/column index mapping go wrong.
        //       i.e. setData() below could edit a different cell than what was originally intended.
        auto index = edits.firstKey();
        auto sText = edits.take(index);
        const auto oldFlag = DFG_OPAQUE_REF().m_bIgnoreOnSelectionChanged;
        auto flagHandler = makeScopedCaller([&] { DFG_OPAQUE_REF().m_bIgnoreOnSelectionChanged = true; }, [=] { DFG_OPAQUE_REF().m_bIgnoreOnSelectionChanged = oldFlag; });
        this->m_spTableModel->setData(index, sText);
    }
}

void TableEditor::onCellEditorTextChanged()
{
    if (!m_spTableView || DFG_OPAQUE_REF().m_bIgnoreCellEditorTextChanged)
        return;
    const auto& indexes = (m_spTableView->getSelectedItemCount() == 1) ? m_spTableView->getSelectedItemIndexes_dataModel() : QModelIndexList();
    if (m_spTableModel && m_spCellEditor && indexes.size() == 1)
    {
        auto lockReleaser = m_spTableView->tryLockForEdit();
        QString sNewText = m_spCellEditor->toPlainText();
        if (!lockReleaser.isLocked())
        {
            // Couldn't acquire lock. Scheduling a new try in 100 ms.
            QPointer<TableEditor> thisPtr = this;
            DFG_OPAQUE_REF().m_pendingEdits[indexes.front()] = std::move(sNewText);
            QTimer::singleShot(100, this, [=]() { if (thisPtr) thisPtr->handlePendingEdits(); });
            return;
        }

        const auto oldFlag = DFG_OPAQUE_REF().m_bIgnoreOnSelectionChanged;
        auto flagHandler = makeScopedCaller([&] { DFG_OPAQUE_REF().m_bIgnoreOnSelectionChanged = true ;}, [=] { DFG_OPAQUE_REF().m_bIgnoreOnSelectionChanged = oldFlag; });
        DFG_OPAQUE_REF().m_pendingEdits.remove(indexes.front()); // Removing any pending edits for this index if any.
        this->m_spTableModel->setData(indexes.front(), sNewText);
    }
}

void TableEditor::onHighlightTextChanged(const QString& text)
{
    if (!m_spTableView || !m_spFindPanel || !m_spFindPanel->m_pColumnSelector)
        return;

    StringMatchDefinition matchDef(text, m_spFindPanel->getCaseSensitivity(), m_spFindPanel->getPatternSyntax());
    m_spTableView->setFindText(matchDef, CsvItemModel::visibleColumnIndexToInternal(m_spFindPanel->m_pColumnSelector->value()));
    m_spTableView->onFindNext();
}

void TableEditor::setFilterJson(const QString& sJson)
{
    if (!m_spFilterPanel || !m_spFilterPanel->m_pTextEdit || !m_spFilterPanel->m_pMatchSyntaxCombobox)
        return;
    const auto nJsonIndex = m_spFilterPanel->m_pMatchSyntaxCombobox->findData(PatternMatcher::Json);
    DFG_ASSERT_CORRECTNESS(nJsonIndex != -1);
    m_spFilterPanel->m_pMatchSyntaxCombobox->setCurrentIndex(nJsonIndex);
    m_spFilterPanel->m_pTextEdit->setText(sJson);
}

void TableEditor::setFilterToColumn(const Index nDataCol, const QVariantMap& filterDef)
{
    if (!m_spFilterPanel)
        return;
    const auto sPattern = m_spFilterPanel->getPattern().trimmed();
    const auto syntaxType = m_spFilterPanel->getPatternSyntax();
    const auto sNewItem = QJsonDocument::fromVariant(filterDef).toJson(QJsonDocument::Compact);
    auto sNewText = sNewItem;
    if (syntaxType == PatternMatcher::PatternSyntax::Json)
    {
        // When already have json-typed filter, need to check if there is existing filter for this column
        // so that setting new filter doesn't create a duplicate entry.
        // Also removing all non-column filters here.
        auto jsonBytes = sPattern.toUtf8();
        convertSpaceSeparatedJsonListToNewLineSeparated(jsonBytes, nullptr);
        auto jsonItems = jsonBytes.split('\n');
        const auto sVisibleColumnIndex = QString::number(CsvItemModel::internalColumnIndexToVisible(nDataCol));
        const QString sAndGroup = TableStringMatchDefinition::makeColumnFilterAndGroupId();
        auto nExistingItem = jsonItems.size();
        std::vector<bool> keepFlags(static_cast<size_t>(jsonItems.size()) + 1u, true);
        for (decltype(nExistingItem) i = 0; i < jsonItems.size(); ++i)
        {
            QJsonParseError parseError;
            const auto jsonObj = QJsonDocument::fromJson(jsonItems[i], &parseError).object();
            if (parseError.error != QJsonParseError::NoError)
            {
                keepFlags[i] = false; // Marking invalid items for removal
                continue;
            }
            if (jsonObj.value(StringMatchDefinitionField_andGroup).toString() == sAndGroup)
            {
                if (jsonObj.value(StringMatchDefinitionField_applyColumns).toString() == sVisibleColumnIndex)
                    nExistingItem = i;
            }
            else
                keepFlags[i] = false; // Marking items that are not column filters for removal
        }
        const bool bEmptyPattern = filterDef.value(StringMatchDefinitionField_text).toString().isEmpty();
        if (isValidIndex(jsonItems, nExistingItem))
        {
            if (!bEmptyPattern)
                jsonItems[nExistingItem] = sNewItem;
            else // Removing existing item if new item has empty pattern text
                keepFlags[nExistingItem] = false;
        }
        else if (!bEmptyPattern) // Adding new item only if it has non-empty pattern text.
            jsonItems.push_back(sNewItem);

        ::DFG_MODULE_NS(alg)::keepByFlags(jsonItems, keepFlags);

        sNewText = jsonItems.join(' ');
    }

    setFilterJson(QString::fromUtf8(sNewText));
}

void TableEditor::onFilterTextChanged(const QString& text)
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
        QTimer::singleShot(200, this, [=]() { if (thisPtr) thisPtr->onFilterTextChanged(thisPtr->m_spFilterPanel->getPattern()); });
        return;
    }

    QModelIndex lastVisibleRowDataIndex;
    if (pProxy->rowCount() > 0)
    {
        const auto lasViewIndex = pProxy->index(pProxy->rowCount() - 1, 0);
        lastVisibleRowDataIndex = pProxy->mapToSource(lasViewIndex);
    }

    m_spFilterPanel->setSyntaxIndicator_good();
    if (m_spFilterPanel->getPatternSyntax() == PatternMatcher::Json)
    {
        auto utf8 = text.toUtf8();
        convertSpaceSeparatedJsonListToNewLineSeparated(utf8, [&](const QByteArray& utf8Data, const QJsonParseError& parseError, const int nErrorOffset)
        {
            const auto errContext = QString::fromUtf8(utf8Data.mid(Max(0, nErrorOffset - 10), 21)); // May malfunction if there is non-ascii and context cuts between multibyte codepoints.
            m_spFilterPanel->setSyntaxIndicator_bad(tr("json error: '%1'\noffset = %2 (context ... %3 ...)")
                .arg(parseError.errorString())
                .arg(nErrorOffset)
                .arg(errContext));
        });
        pProxy->setFilterFromNewLineSeparatedJsonList(utf8);
    }
    else if (m_spFilterPanel->m_pColumnSelector)
    {
        pProxy->setFilterFromNewLineSeparatedJsonList(QByteArray());
        if (!PatternMatcher(text, m_spFilterPanel->getCaseSensitivity(), m_spFilterPanel->getPatternSyntax()).setToProxyModel(pProxy))
        {
            DFG_ASSERT(false); // TODO: show information to user.
        }
        pProxy->setFilterKeyColumn(CsvItemModel::visibleColumnIndexToInternal(m_spFilterPanel->m_pColumnSelector->value()));
    }

    // Forcing visual update of column headers since filter change may affect those.
    // Hack: Since it would take bit of work to figure out which columns were affected (if any), for now simply using coarse-grained
    //       implementation by signaling header data change on all columns.
    Q_EMIT pProxy->headerDataChanged(Qt::Horizontal, 0, pProxy->columnCount() - 1);

    // If there was at least one item before applying filter, trying to keep that visible after filter change.
    // This tries to make especially filter clearing be more user friendly: view resetting always to top/bottom after filter clear is often a bit inconvenient.
    const auto scrollToViewIndex = (lastVisibleRowDataIndex.isValid()) ? pProxy->mapFromSource(lastVisibleRowDataIndex) : QModelIndex();
    if (scrollToViewIndex.isValid())
    {
        m_spTableView->repaint(); // scrollTo() didn't seem to do anything without repaint(), e.g. update() wouldn't work either. Probably something less coarse-grained would be sufficient, though.
        m_spTableView->scrollTo(scrollToViewIndex, QAbstractItemView::PositionAtBottom);
    }
    else // case: view was empty before filter change or last visible got filtered out, scroll to default scroll position
    {
        m_spTableView->scrollToDefaultPosition();
    }
}

namespace
{
    static void setColumnSelectorToolTip(const CsvTableView& rView, QSpinBox* pSpinBox, const CsvItemModel* pModel, const int nIndexShiftedDataCol)
    {
        if (!pSpinBox || !pModel)
            return;
        const auto nModelCol = pModel->visibleColumnIndexToInternal(nIndexShiftedDataCol);
        QString sColumnName;
        if (pModel->isValidColumn(nModelCol))
        {
            sColumnName = QObject::tr("Column '%1'").arg(pModel->getHeaderName(nModelCol).mid(0, 32));
            if (!rView.isColumnVisible(ColumnIndex_data(nModelCol)))
                sColumnName += QObject::tr("\n(column is hidden)");
        }
        else if (nModelCol >= CsvItemModel::internalColumnIndexToVisible(0))
            sColumnName = QObject::tr("Index is beyond column count %1").arg(pModel->columnCount());
        pSpinBox->setToolTip(sColumnName);
    }
}

void TableEditor::onFindColumnChanged(const Index newCol)
{
    // Note: changing syntax type also calls this so if planning to take nNewCol into use, introduce separate handler for that.
    DFG_UNUSED(newCol);

    if (!m_spFindPanel)
        return;
    if (m_spFindPanel->m_pColumnSelector && m_spTableView)
        setColumnSelectorToolTip(*m_spTableView, m_spFindPanel->m_pColumnSelector, m_spTableModel.get(), m_spFindPanel->m_pColumnSelector->value());
    onHighlightTextChanged(m_spFindPanel->m_pTextEdit->text());
}

void TableEditor::onFilterColumnChanged(const Index nNewCol)
{
    // Note: changing syntax type also calls this so if planning to take nNewCol into use, introduce separate handler for that.
    DFG_UNUSED(nNewCol);
    if (!m_spFilterPanel)
        return;
    if (m_spFilterPanel->m_pColumnSelector && m_spTableView)
        setColumnSelectorToolTip(*m_spTableView, m_spFilterPanel->m_pColumnSelector, m_spTableModel.get(), m_spFilterPanel->m_pColumnSelector->value());
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

void TableEditor::onFindRequested()
{
    if (m_spFindPanel)
        activateFindTextEdit(m_spFindPanel->m_pTextEdit);
}

void TableEditor::onFilterRequested()
{
    if (m_spFilterPanel)
        activateFindTextEdit(m_spFilterPanel->m_pTextEdit);
}

void TableEditor::onHighlightTextCaseSensitivityChanged(const bool bCaseSensitive)
{
    DFG_UNUSED(bCaseSensitive);
    if (m_spFindPanel)
        onHighlightTextChanged(m_spFindPanel->m_pTextEdit->text());
}

void TableEditor::onFilterCaseSensitivityChanged(const bool bCaseSensitive)
{
    DFG_UNUSED(bCaseSensitive);
    if (m_spFilterPanel)
        onFilterTextChanged(m_spFilterPanel->getPattern());
}

void TableEditor::addToolBarSeparator()
{
    if (m_spToolBar)
        m_spToolBar->addSeparator();
}

void TableEditor::addToolBarWidget(QWidget* pWidget)
{
    if (m_spToolBar)
        m_spToolBar->addWidget(pWidget);
}

int TableEditor::setGraphDisplay(QWidget* pGraphDisplay)
{
    DFG_ASSERT_CORRECTNESS(m_spMainSplitter != nullptr);
    if (!pGraphDisplay || !m_spMainSplitter)
        return 0;
    m_spChartDisplay = pGraphDisplay;
    m_spMainSplitter->addWidget(pGraphDisplay);

    // Setting panel width.
    return setChartPanelWidth(m_spMainSplitter.get(), this->width(), getTableEditorProperty<TableEditorPropertyId_chartPanelWidth>(this));
}

void TableEditor::onViewReadOnlyModeChanged(const bool bReadOnly)
{
    setCellEditorTitle(m_spCellEditorDockWidget.get(), QString(), bReadOnly);
    if (m_spCellEditor)
        m_spCellEditor->setReadOnly(bReadOnly);
}

void TableEditor::onViewCellEditorTextEdited(const CsvTableViewCellEditorTextChangedDetails& details)
{
    if (m_spCellEditorDockWidget && m_spCellEditorDockWidget->isVisible() &&
        m_spCellEditor && m_spCellEditor->property(celLEditorObjectProperty_shownModelIndex).toModelIndex() == details["modelIndex"].toModelIndex())
    {
        // Note: without setting  m_bIgnoreCellEditorTextChanged, onCellEditorTextChanged() would set cell content when text was edited directly in table cell.
        //       This would break the behaviour of esc-key reverting changes.
        const auto oldFlag = DFG_OPAQUE_REF().m_bIgnoreCellEditorTextChanged;
        auto flagHandler = makeScopedCaller([&] { DFG_OPAQUE_REF().m_bIgnoreCellEditorTextChanged = true; }, [=] { DFG_OPAQUE_REF().m_bIgnoreCellEditorTextChanged = oldFlag; });
        m_spCellEditor->setPlainText(details["newText"].toString());
    }
}

void TableEditor::onViewCellEditorClosed()
{
    // Reloading text to CellEditor widget to make sure it's up-to-date
    // In particular if editing was cancelled, CellEditor text should be reverted back to what is was before mirroring
    // live changes from CsvTableView editor delegate.
    if (m_spCellEditor)
        onSelectionChanged(QItemSelection(), QItemSelection(), QItemSelection());
}

} } // namespace dfg::qt
/////////////////////////////////

Q_DECLARE_METATYPE(::DFG_MODULE_NS(qt)::WindowExtentProperty); // Note: placing this in namespace generated a "class template specialization of 'QMetaTypeId' must occure at global scope"-message
