#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QItemDelegate>
    #include <QPointer>
DFG_END_INCLUDE_QT_HEADERS

class QCompleter;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

class CsvTableView;

// Item delegate (=cell editor handler) for CsvTableView.
// This takes CsvTableView's edit lock into account; i.e. setModelData() will fail if unable to acquire lock for editing.
class CsvTableViewDelegate : public QItemDelegate
{
public:
    using BaseClass = QItemDelegate;

    CsvTableViewDelegate(QWidget* pParent);

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private:
    virtual bool editorToString(QWidget* pWidget, QString& sText) const; // To return true iff text should be set to model.

public:
    QPointer<CsvTableView> m_spTableView;
}; // CsvTableViewDelegate


// Completer delegate for CsvTableView, i.e. provides cell editing with completer.
class CsvTableViewCompleterDelegate : public CsvTableViewDelegate
{
    Q_OBJECT

    typedef CsvTableViewCompleterDelegate ThisClass;
    using BaseClass = CsvTableViewDelegate;

public:
    // Does not take ownership of pCompleter.
    CsvTableViewCompleterDelegate(QWidget* pParent = nullptr, QCompleter* pCompleter = nullptr);
    ~CsvTableViewCompleterDelegate();

    
    QWidget *createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex &index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;

    void updateEditorGeometry(QWidget* editor,
    const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QPointer<QCompleter> m_spCompleter;
}; // CsvTableViewCompleterDelegate

}} // module namespace
