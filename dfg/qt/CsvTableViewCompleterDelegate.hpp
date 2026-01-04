#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

#define DFG_CSVTABLEVIEWDELEGATE_USE_STYLEDDELEGATE (QT_VERSION >= QT_VERSION_CHECK(6, 7, 0))

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QPointer>
#if (DFG_CSVTABLEVIEWDELEGATE_USE_STYLEDDELEGATE == 1)
    #include <QStyledItemDelegate>
#else
    #include <QItemDelegate>
#endif

DFG_END_INCLUDE_QT_HEADERS

class QCompleter;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

namespace DFG_DETAIL_NS
{
#if (DFG_CSVTABLEVIEWDELEGATE_USE_STYLEDDELEGATE == 1)
    // On Windows, style 'windows11' is the default since Qt 6.7 and there were quite many visual issues
    // when using QItemDelegate with that style, so using QStyledItemDelegate on newer Qt versions.
    using CsvTableViewDelegateBaseClass = QStyledItemDelegate;
#else
    using CsvTableViewDelegateBaseClass = QItemDelegate;
#endif
} // namespace DFG_DETAIL_NS

class CsvTableView;

// Item delegate (=cell editor handler) for CsvTableView.
// This takes CsvTableView's edit lock into account; i.e. setModelData() will fail if unable to acquire lock for editing.
class CsvTableViewDelegate : public DFG_DETAIL_NS::CsvTableViewDelegateBaseClass
{
public:
    using BaseClass = DFG_DETAIL_NS::CsvTableViewDelegateBaseClass;

    CsvTableViewDelegate(QWidget* pParent);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    static constexpr bool isWordWrapSupported()
    {
        // Text elide doesn't work optimally with QStyledItemDelegate if tableview has wordWrap set to true
        // Related links
        // https://qt-project.atlassian.net/browse/QTBUG-78956, Created October 2019, open as of 2025-12
        // https://qt-project.atlassian.net/browse/QTBUG-87178, Created October 2020, open as of 2025-12
        // https://stackoverflow.com/questions/64198197/how-to-prevent-too-aggressive-text-elide-in-qtableview
        // https://stackoverflow.com/questions/73322362/strange-behavior-of-qtableview-with-text-containing-slashes
        return (DFG_CSVTABLEVIEWDELEGATE_USE_STYLEDDELEGATE != 1);
    }

private:
    virtual bool editorToString(QWidget* pWidget, QString& sText) const; // To return true iff text should be set to model.

    template <class T>
    T* createEditorImpl(QWidget* pParent, const QString& sCurrentText, const QModelIndex& index) const;

protected:
    bool checkCellEditability(const QModelIndex& index) const;
#if (DFG_CSVTABLEVIEWDELEGATE_USE_STYLEDDELEGATE != 1)
    void drawDisplay(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QString& text) const override;
#endif
    bool eventFilter(QObject* editor, QEvent* event) override;

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

    QPointer<QCompleter> m_spCompleter;
}; // CsvTableViewCompleterDelegate

}} // module namespace
