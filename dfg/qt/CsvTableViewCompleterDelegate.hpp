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

class DFG_CLASS_NAME(CsvTableViewCompleterDelegate) : public QItemDelegate
{
    Q_OBJECT

    typedef DFG_CLASS_NAME(CsvTableViewCompleterDelegate) ThisClass;
    typedef QItemDelegate BaseClass;

public:
    DFG_CLASS_NAME(CsvTableViewCompleterDelegate)(QObject* parent = nullptr, QCompleter* pCompleter = nullptr);
    ~DFG_CLASS_NAME(CsvTableViewCompleterDelegate)();

    QWidget *createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                           const QModelIndex &index) const;

    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                       const QModelIndex& index) const;

    void updateEditorGeometry(QWidget* editor,
    const QStyleOptionViewItem& option, const QModelIndex& index) const;

    QPointer<QCompleter> m_spCompleter;
};

}} // module namespace
