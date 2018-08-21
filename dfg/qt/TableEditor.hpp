#pragma once

#include "../dfgDefs.hpp"
#include "../baseConstructorDelegate.hpp"
#include <memory>

#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QWidget>
    #include <QStatusBar>
    #include <QPlainTextEdit>
DFG_END_INCLUDE_QT_HEADERS

class QDockWidget;
class QItemSelection;
class QLineEdit;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    class DFG_CLASS_NAME(CsvTableView);
    class DFG_CLASS_NAME(CsvItemModel);

    class DFG_CLASS_NAME(TableEditorStatusBar) : public QStatusBar
    {
    public:
        typedef QStatusBar BaseClass;

        DFG_CLASS_NAME(TableEditorStatusBar)() {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_1(DFG_CLASS_NAME(TableEditorStatusBar), QStatusBar) {}
    }; // class TableEditorStatusBar

    class DFG_CLASS_NAME(TableEditor) : public QWidget
    {
    public:
        typedef QWidget BaseClass;
        typedef DFG_CLASS_NAME(TableEditor) ThisClass;

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
        };

        DFG_CLASS_NAME(TableEditor)();
        ~DFG_CLASS_NAME(TableEditor)();

        /** Returns true if opened, false otherwise. Opening will fail if TableEditor already has a file opened and it has been modified. */
        bool tryOpenFileFromPath(QString path);

        /** Resizes column widths. */
        void resizeColumnsToView(ColumnResizeStyle style = ColumnResizeStyle_auto);

        void setAllowApplicationSettingsUsage(bool b);

        void updateWindowTitle();

    protected:
        void closeEvent(QCloseEvent* event) override;

    public slots:
        void onSourcePathChanged();
        void onNewSourceOpened();
        void onModifiedStatusChanged(bool);
        void onSaveCompleted(bool, double);
        void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void onCellEditorTextChanged();
        void onModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>());

    public:
        std::unique_ptr<DFG_CLASS_NAME(CsvTableView)> m_spTableView;
        std::unique_ptr<DFG_CLASS_NAME(CsvItemModel)> m_spTableModel;
        std::unique_ptr<QLineEdit> m_spLineEditSourcePath;
        std::unique_ptr<DFG_CLASS_NAME(TableEditorStatusBar)> m_spStatusBar;
        std::unique_ptr<CellEditor> m_spCellEditor;
        std::unique_ptr<QDockWidget> m_spCellEditorDockWidget;
        bool m_bHandlingOnCellEditorTextChanged;
    };

} } // module namespace
