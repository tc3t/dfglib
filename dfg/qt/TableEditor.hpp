#pragma once

#include "../dfgDefs.hpp"
#include "../baseConstructorDelegate.hpp"
#include "../build/languageFeatureInfo.hpp"
#include <memory>
#include "../OpaquePtr.hpp"
#include "../ReadOnlySzParam.hpp"

#include "qtIncludeHelpers.hpp"
#include "containerUtils.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QModelIndex>
    #include <QWidget>
    #include <QStatusBar>
    #include <QPlainTextEdit>
DFG_END_INCLUDE_QT_HEADERS

class QDockWidget;
class QItemSelection;
class QLabel;
class QLineEdit;
class QMenu;
class QToolBar;
class QSplitter;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    class CsvTableView;
    class CsvItemModel;
    class CsvTableViewSortFilterProxyModel;

    namespace DFG_DETAIL_NS
    {
        class FindPanelWidget;
        class FilterPanelWidget;
    }

    class TableEditorStatusBar : public QStatusBar
    {
    public:
        typedef QStatusBar BaseClass;

        TableEditorStatusBar() {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_1(TableEditorStatusBar, QStatusBar) {}
    }; // class TableEditorStatusBar

    class TableEditor : public QWidget
    {
        Q_OBJECT
    public:
        typedef QWidget BaseClass;
        typedef TableEditor ThisClass;
        typedef CsvItemModel ModelClass;
        typedef CsvTableViewSortFilterProxyModel ProxyModelClass;
        typedef CsvTableView ViewClass;
        using Index = int; // Should be identical to CsvItemModel::Index

        enum ColumnResizeStyle
        {
            ColumnResizeStyle_evenlyDistributed,                            // Distributes available width evenly to all columns regardless of their content width.
            ColumnResizeStyle_auto = ColumnResizeStyle_evenlyDistributed    // Lets TableEditor choose style (note: implementation may change).
        };

        class CellEditor : public QPlainTextEdit
        {
        public:
           typedef QPlainTextEdit BaseClass;
           CellEditor(QWidget* parent) :
               BaseClass(parent)
           {
           }

           void setFontPointSizeF(qreal pointSize);
        };

        TableEditor();
        ~TableEditor() DFG_OVERRIDE_DESTRUCTOR;

        ViewClass* tableView();

        QWidget* selectionDetailPanel();

        /** Returns true if opened, false otherwise. Opening will fail if TableEditor already has a file opened and it has been modified. */
        bool tryOpenFileFromPath(QString path);

        /** Resizes column widths. */
        void resizeColumnsToView(ColumnResizeStyle style = ColumnResizeStyle_auto);

        void setAllowApplicationSettingsUsage(bool b);

        void updateWindowTitle();

        void addToolBarSeparator();
        void addToolBarWidget(QWidget* pWidget);

        bool handleExitConfirmationAndReturnTrueIfCanExit();

        void setWindowModified(bool bNewModifiedStatus);

        // Returns size of the chart display in the (possibly) resizable direction, value 0 can be used to determine that display is hidden.
        int setGraphDisplay(QWidget* pGraphDisplayDisplay);

        // Sets window to resize/move if document requests to use specific window size/position. If not set, requests are ignored.
        void setResizeWindow(QWidget* pWindow);

        // Toggles visibility of cell editor widget, returns true iff cell editor is visible after the call.
        bool toggleCellEditorVisibility();

        /** Returns fields used by file info tooltip.
         *  @note Fields are user visible, possibly translated texts and are not guaranteed to be stable.
         */
        QVector<QPair<QString, QString>> getToolTipFileInfoFields() const;
        void updateFileInfoToolTip();

        // Returns variant map of find panel settings where keys are strings defined by CsvOptionProperty_find* -entries.
        QVariantMap getFindPanelSettings() const;

        // Returns variant map of filter panel settings where keys are strings defined by CsvOptionProperty_filter* -entries.
        QVariantMap getFilterPanelSettings() const;

    protected:
        void closeEvent(QCloseEvent* event) override;

    private:
        void setSelectionDetails(const StringViewC& sv, int nResultPrecision = -2);
        void setSelectionDetailsFromIni(const QString& sv);
        QVariantMap getFindFilterPanelSettingsImpl(const DFG_DETAIL_NS::FindPanelWidget& rWidget) const;

    signals:
        void sigModifiedStatusChanged(bool);

    public slots:
        void onSourcePathChanged();
        void onNewSourceOpened();
        void onModifiedStatusChanged(bool);
        void onSaveCompleted(bool, double);
        void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected, const QItemSelection& edited);
        void onCellEditorTextChanged();
        void handlePendingEdits();
        void onHighlightTextChanged(const QString& text);
        void onFilterTextChanged(const QString& text);
        void onFindColumnChanged(::DFG_MODULE_NS(qt)::TableEditor::Index newCol);
        void onFilterColumnChanged(::DFG_MODULE_NS(qt)::TableEditor::Index nNewCol);
        void onFindRequested();
        void onFilterRequested();
        void setFilterJson(const QString& sJson);
        void setFilterToColumn(::DFG_MODULE_NS(qt)::TableEditor::Index nDataCol, const QVariantMap& filterDef);
        void onHighlightTextCaseSensitivityChanged(bool);
        void onFilterCaseSensitivityChanged(bool);
        void onViewReadOnlyModeChanged(bool);
        void onCopyFileInfoToClipboard();
        void onOpenFileDirectoryInOsFileManager();

    public:
        QObjectStorage<ViewClass> m_spTableView;
        QObjectStorage<ModelClass> m_spTableModel;
        QObjectStorage<ProxyModelClass> m_spProxyModel;
        QObjectStorage<QLineEdit> m_spLineEditSourcePath;
        QObjectStorage<TableEditorStatusBar> m_spStatusBar;
        QObjectStorage<QLabel> m_spSelectionStatusInfo;
        QObjectStorage<CellEditor> m_spCellEditor;
        QObjectStorage<QDockWidget> m_spCellEditorDockWidget;
        QObjectStorage<DFG_DETAIL_NS::FindPanelWidget> m_spFindPanel;
        QObjectStorage<DFG_DETAIL_NS::FilterPanelWidget> m_spFilterPanel;
        QObjectStorage<QWidget> m_spSelectionAnalyzerPanel;
        QObjectStorage<QMenu> m_spResizeColumnsMenu;
        QObjectStorage<QToolBar> m_spToolBar;
        QObjectStorage<QSplitter> m_spMainSplitter;
        QPointer<QWidget> m_spChartDisplay;

        DFG_OPAQUE_PTR_DECLARE();
    };

} } // module namespace
