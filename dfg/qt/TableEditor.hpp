#pragma once

#include "../dfgDefs.hpp"
#include "../baseConstructorDelegate.hpp"
#include "../build/languageFeatureInfo.hpp"
#include <memory>
#include "../OpaquePtr.hpp"

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
class QSortFilterProxyModel;
class QToolBar;
class QSplitter;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    class DFG_CLASS_NAME(CsvTableView);
    class DFG_CLASS_NAME(CsvItemModel);

    namespace DFG_DETAIL_NS
    {
        class FindPanelWidget;
        class FilterPanelWidget;
    }

    class DFG_CLASS_NAME(TableEditorStatusBar) : public QStatusBar
    {
    public:
        typedef QStatusBar BaseClass;

        DFG_CLASS_NAME(TableEditorStatusBar)() {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_1(DFG_CLASS_NAME(TableEditorStatusBar), QStatusBar) {}
    }; // class TableEditorStatusBar

    class DFG_CLASS_NAME(TableEditor) : public QWidget
    {
        Q_OBJECT
    public:
        typedef QWidget BaseClass;
        typedef DFG_CLASS_NAME(TableEditor) ThisClass;
        typedef DFG_CLASS_NAME(CsvItemModel) ModelClass;
        typedef QSortFilterProxyModel ProxyModelClass;
        typedef DFG_CLASS_NAME(CsvTableView) ViewClass;

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

        DFG_CLASS_NAME(TableEditor)();
        ~DFG_CLASS_NAME(TableEditor)() DFG_OVERRIDE_DESTRUCTOR;

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

        void setGraphDisplay(QWidget* pGraphDisplay);

    protected:
        void closeEvent(QCloseEvent* event) override;

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
        void onFindColumnChanged(int newCol);
        void onFilterColumnChanged(int nNewCol);
        void onFindRequested();
        void onFilterRequested();
        void onHighlightTextCaseSensitivityChanged(bool);
        void onFilterCaseSensitivityChanged(bool);

    public:
        QObjectStorage<ViewClass> m_spTableView;
        QObjectStorage<ModelClass> m_spTableModel;
        QObjectStorage<ProxyModelClass> m_spProxyModel;
        QObjectStorage<QLineEdit> m_spLineEditSourcePath;
        QObjectStorage<DFG_CLASS_NAME(TableEditorStatusBar)> m_spStatusBar;
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
        bool m_bHandlingOnCellEditorTextChanged;

        DFG_OPAQUE_PTR_DECLARE();
    };

} } // module namespace
