#pragma once

#include "../dfgDefs.hpp"
#include "../baseConstructorDelegate.hpp"
#include <memory>

#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QWidget>
    #include <QStatusBar>
DFG_END_INCLUDE_QT_HEADERS

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
        DFG_CLASS_NAME(TableEditor)();
        ~DFG_CLASS_NAME(TableEditor)();

    public slots:
        void onSourcePathChanged();
        void onNewSourceOpened();

    public:
        std::unique_ptr<DFG_CLASS_NAME(CsvTableView)> m_spTableView;
        std::unique_ptr<DFG_CLASS_NAME(CsvItemModel)> m_spTableModel;
        std::unique_ptr<QLineEdit> m_spLineEditSourcePath;
        std::unique_ptr<TableEditorStatusBar> m_spStatusBar;
    };

} } // module namespace
