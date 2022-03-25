#pragma once

#include "qtIncludeHelpers.hpp"
#include "connectHelper.hpp"
#include "../cont/Vector.hpp"
#include "widgetHelpers.hpp"

#include <functional>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QDialog>
    #include <QDialogButtonBox>
    #include <QInputDialog>
    #include <QJsonDocument>
    #include <QLabel>
    #include <QListView>
    #include <QPlainTextEdit>
    #include <QStringListModel>
    #include <QVBoxLayout>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt)
{

class InputDialog : public QInputDialog
{
public:
    using IsSelectedPred = std::function<bool(int, const QString&)>;

    static auto getItems(QWidget* pParent,
                         const QString& sTitle,
                         const QString& sLabel,
                         const QStringList& items,
                         IsSelectedPred isSelectedPred = IsSelectedPred(),
                         bool* pOk = nullptr/*,
                         int nCurrent = 0,
                         bool bEditable = false,
                         bool* pOk = nullptr,
                         Qt::WindowFlags flags = Qt::WindowFlags(),
                         Qt::InputMethodHints inputMethodHints = Qt::ImhNone*/) -> ::DFG_MODULE_NS(cont)::Vector<int>
    {
        if (pOk)
            *pOk = false;

        ::DFG_MODULE_NS(cont)::Vector<int> rv;
        QDialog dlg(pParent);
        dlg.setWindowTitle(sTitle);
        auto pLayout = new QVBoxLayout(&dlg);

        pLayout->addWidget(new QLabel(sLabel, &dlg));

        QListView view(&dlg);
        QStringListModel model(items, &dlg);
        view.setModel(&model);
        view.setSelectionMode(QAbstractItemView::MultiSelection);

        // Setting selected items if callback was given
        if (isSelectedPred)
        {
            auto pSelectionModel = view.selectionModel();
            if (pSelectionModel)
            {
                for (int r = 0; r < model.rowCount(); ++r)
                {
                    if (isSelectedPred(r, items[r]))
                        pSelectionModel->select(model.index(r), QItemSelectionModel::Select);
                }
            }
        }

        pLayout->addWidget(&view);

        pLayout->addWidget(addOkCancelButtonBoxToDialog(&dlg, pLayout));
        removeContextHelpButtonFromDialog(&dlg);

        dlg.setLayout(pLayout);
        if (dlg.exec() == QDialog::Accepted)
        {
            auto pSm = view.selectionModel();
            if (pSm)
            {
                const auto rows = pSm->selectedRows();
                for (const auto& index : rows)
                {
                    rv.push_back(index.row());
                }

            }
            if (pOk)
                *pOk = true;
        }
        return rv;
    }

    static QString getJsonAsText(
        QWidget* pParent,
        const QString& sTitle,
        const QString& sLabel,
        const QVariantMap& initialItems,
        const QString sSelectedValueKey = QString(),
        bool* pOk = nullptr);

    static QVariantMap getJsonAsVariantMap(
        QWidget* pParent,
        const QString& sTitle,
        const QString& sLabel,
        const QVariantMap& initialItems,
        const QString sSelectedValueKey = QString(),
        bool* pOk = nullptr);

}; // class InputDialog


QString InputDialog::getJsonAsText(
    QWidget* pParent,
    const QString& sTitle,
    const QString& sLabel,
    const QVariantMap& initialItems,
    const QString sSelectedValueKey,
    bool* pOk)
{
    if (pOk)
        *pOk = false;
    auto sJsonDoc = QString::fromUtf8(QJsonDocument::fromVariant(initialItems).toJson());
    if (sJsonDoc.isEmpty())
        return QString();
    QDialog dlg;
    dlg.setWindowTitle(sTitle);
    removeContextHelpButtonFromDialog(&dlg);
    QVBoxLayout layout(&dlg);
    auto pTextEdit = new QPlainTextEdit(sJsonDoc, &dlg); // Deletion by parenthood
    layout.addWidget(new QLabel(sLabel, &dlg)); // Deletion by parenthood
    layout.addWidget(pTextEdit);
    layout.addWidget(addOkCancelButtonBoxToDialog(&dlg, &layout));

    // If sSelectedValueKey is non-empty, selecting value of that key for convenient editing (i.e. don't need to find and select field to edit)
    if (!sSelectedValueKey.isEmpty())
    {
        if (pTextEdit->find(QString("\"%1\": ").arg(sSelectedValueKey)))
        {
            auto textCursor = pTextEdit->textCursor();
            const auto nSelectionEnd = textCursor.selectionEnd();
            textCursor.setPosition(nSelectionEnd);
            textCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            pTextEdit->setTextCursor(textCursor);
        }
    }

    const auto dlgRv = dlg.exec();
    const bool bOk = (dlgRv == QDialog::Accepted);
    if (pOk)
        *pOk = bOk;
    return (bOk) ? pTextEdit->toPlainText() : QString();
}

QVariantMap InputDialog::getJsonAsVariantMap(
    QWidget* pParent,
    const QString& sTitle,
    const QString& sLabel,
    const QVariantMap& initialItems,
    const QString sSelectedValueKey,
    bool* pOk)
{
    bool bOk;
    bool* pEffectiveOk = (pOk) ? pOk : &bOk;
    const auto sJson = getJsonAsText(pParent, sTitle, sLabel, initialItems, sSelectedValueKey, pEffectiveOk);
    if (*pEffectiveOk)
        return QJsonDocument::fromJson(sJson.toUtf8()).toVariant().toMap();
    else
        return QVariantMap();
}

}} // namespace dfg::qt
