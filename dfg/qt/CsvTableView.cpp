#include "../buildConfig.hpp" // To get rid of C4996 "Function call with parameters that may be unsafe" in MSVC.
#include "qtIncludeHelpers.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"
#include "CsvTableViewActions.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QMenu>
#include <QFileDialog>
#include <QUndoStack>
#include <QHeaderView>
#include <QFormLayout>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QInputDialog>
DFG_END_INCLUDE_QT_HEADERS

#include <set>
#include <unordered_set>
#include "../alg.hpp"
#include "../str/stringLiteralCharToValue.hpp"

using namespace DFG_MODULE_NS(qt);

DFG_CLASS_NAME(CsvTableView)::DFG_CLASS_NAME(CsvTableView)(QWidget* pParent) : BaseClass(pParent)
{
    auto pVertHdr = verticalHeader();
    if (pVertHdr)
        pVertHdr->setDefaultSectionSize(20); // Default row height seems to be 30, which looks somewhat bloated. Make it smaller.

    {
        auto pAction = new QAction(tr("Move first row to header"), this);
        //pAction->setShortcut(tr(""));
        connect(pAction, &QAction::triggered, this, &ThisClass::moveFirstRowToHeader);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Move header to first row"), this);
        //pAction->setShortcut(tr(""));
        connect(pAction, &QAction::triggered, this, &ThisClass::moveHeaderToFirstRow);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Open file..."), this);
        connect(pAction, &QAction::triggered, this, &ThisClass::openFromFile);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Save to file..."), this);
        connect(pAction, &QAction::triggered, this, &ThisClass::saveToFile);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Save to file with options..."), this);
        connect(pAction, &QAction::triggered, this, &ThisClass::saveToFileWithOptions);
        addAction(pAction);
    }

    // -------------------------------------------------
    {
        auto pAction = new QAction(this);
        pAction->setSeparator(true);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Clear selected cell(s)"), this);
        pAction->setShortcut(tr("Del"));
        connect(pAction, &QAction::triggered, this, &ThisClass::clearSelected);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Insert row here"), this);
        pAction->setShortcut(tr("Ins"));
        connect(pAction, &QAction::triggered, this, &ThisClass::insertRowHere);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Insert row after current"), this);
        pAction->setShortcut(tr("Shift+Ins"));
        connect(pAction, &QAction::triggered, this, &ThisClass::insertRowAfterCurrent);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Insert column"), this);
        pAction->setShortcut(tr("Ctrl+Alt+Ins"));
        connect(pAction, &QAction::triggered, this, &ThisClass::insertColumn);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Insert column after current"), this);
        pAction->setShortcut(tr("Ctrl+Alt+Shift+Ins"));
        connect(pAction, &QAction::triggered, this, &ThisClass::insertColumnAfterCurrent);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Delete selected row(s)"), this);
        pAction->setShortcut(tr("Shift+Del"));
        connect(pAction, &QAction::triggered, this, &ThisClass::deleteSelectedRow);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Delete current column"), this);
        pAction->setShortcut(tr("Ctrl+Del"));
        connect(pAction, SIGNAL(triggered()), this, SLOT(deleteCurrentColumn()));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Resize table"), this);
        //pAction->setShortcut(tr(""));
        connect(pAction, &QAction::triggered, this, &ThisClass::resizeTable);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(this);
        pAction->setSeparator(true);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Invert selection"), this);
        pAction->setShortcut(tr("Ctrl+I"));
        connect(pAction, &QAction::triggered, this, &ThisClass::invertSelection);
        addAction(pAction);
    }

    /* Not Implemented
    {
        auto pAction = new QAction(tr("Move row up"), this);
        pAction->setShortcut(tr("Alt+Up"));
        connect(pAction, &QAction::triggered, this, &ThisClass::moveRowUp);
        addAction(pAction);
    }

    {
    auto pAction = new QAction(tr("Move row down"), this);
    pAction->setShortcut(tr("Alt+Down"));
    connect(pAction, &QAction::triggered, this, &ThisClass::moveRowDown);
    addAction(pAction);
    }
    */

    // -------------------------------------------------
    {
        auto pAction = new QAction(this);
        pAction->setSeparator(true);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Cut"), this);
        pAction->setShortcut(tr("Ctrl+X"));
        connect(pAction, &QAction::triggered, this, &ThisClass::cut);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Copy"), this);
        pAction->setShortcut(tr("Ctrl+C"));
        connect(pAction, &QAction::triggered, this, &ThisClass::copy);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Paste"), this);
        pAction->setShortcut(tr("Ctrl+V"));
        connect(pAction, &QAction::triggered, this, &ThisClass::paste);
        addAction(pAction);
    }


    // -------------------------------------------------
    {
        auto pAction = new QAction(this);
        pAction->setSeparator(true);
        addAction(pAction);
    }
    privAddUndoRedoActions();
}

DFG_CLASS_NAME(CsvTableView)::~DFG_CLASS_NAME(CsvTableView)()
{
}

void DFG_CLASS_NAME(CsvTableView)::createUndoStack()
{
    m_spUndoStack.reset(new DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TorRef)<QUndoStack>);
}

void DFG_CLASS_NAME(CsvTableView)::privAddUndoRedoActions(QAction* pAddBefore)
{
    if (!m_spUndoStack)
        createUndoStack();
    if (m_spUndoStack)
    {
        auto pActionUndo = m_spUndoStack->item().createUndoAction(this, tr("&Undo"));
        pActionUndo->setShortcuts(QKeySequence::Undo);
        insertAction(pAddBefore, pActionUndo);

        auto pActionRedo = m_spUndoStack->item().createRedoAction(this, tr("&Redo"));
        pActionRedo->setShortcuts(QKeySequence::Redo);
        insertAction(pAddBefore, pActionRedo);
    }
}

void DFG_CLASS_NAME(CsvTableView)::setExternalUndoStack(QUndoStack* pUndoStack)
{
    if (!m_spUndoStack)
        createUndoStack();
    m_spUndoStack->setRef(pUndoStack);

    // Find and remove undo&redo actions from action list since they use the old undostack...
    auto acts = actions();
    std::vector<QAction*> removeList;
    QAction* pAddNewBefore = nullptr;
    for (auto iter = acts.begin(); iter != acts.end(); ++iter)
    {
        auto pAction = *iter;
        if (pAction && pAction->shortcut() == QKeySequence::Undo)
            removeList.push_back(pAction);
        else if (pAction && pAction->shortcut() == QKeySequence::Redo)
        {
            removeList.push_back(pAction);
            if (iter + 1 != acts.end())
                pAddNewBefore = *(iter + 1);
        }
    }
    for (auto iter = removeList.begin(); iter != removeList.end(); ++iter)
        removeAction(*iter);

    // ... and add the new undo&redo actions that refer to the new undostack.
    privAddUndoRedoActions(pAddNewBefore);
}

void DFG_CLASS_NAME(CsvTableView)::contextMenuEvent(QContextMenuEvent* pEvent)
{
    DFG_UNUSED(pEvent);

    QMenu menu;
    menu.addActions(actions());
    menu.exec(QCursor::pos());

    //BaseClass::contextMenuEvent(pEvent);
    /*
    QMenu menu;

    auto actionDelete_current_column = new QAction(this);
    actionDelete_current_column->setObjectName(QStringLiteral("actionDelete_current_column"));
    auto actionRename_column = new QAction(this);
    actionRename_column->setObjectName(QStringLiteral("actionRename_column"));
    auto actionMove_column_left = new QAction(this);
    actionMove_column_left->setObjectName(QStringLiteral("actionMove_column_left"));
    auto actionMove_column_right = new QAction(this);
    actionMove_column_right->setObjectName(QStringLiteral("actionMove_column_right"));
    auto actionCopy_column = new QAction(this);
    actionCopy_column->setObjectName(QStringLiteral("actionCopy_column"));
    auto actionPaste_column = new QAction(this);
    actionPaste_column->setObjectName(QStringLiteral("actionPaste_column"));
    auto actionDelete_selected_row_s = new QAction(this);
    actionDelete_selected_row_s->setObjectName(QStringLiteral("actionDelete_selected_row_s"));

    actionRename_column->setText(QApplication::translate("MainWindow", "Rename column", 0, QApplication::UnicodeUTF8));
    actionRename_column->setShortcut(QApplication::translate("MainWindow", "Ctrl+R", 0, QApplication::UnicodeUTF8));
    actionMove_column_left->setText(QApplication::translate("MainWindow", "Move column left", 0, QApplication::UnicodeUTF8));
    actionMove_column_left->setShortcut(QApplication::translate("MainWindow", "Alt+Left", 0, QApplication::UnicodeUTF8));
    actionMove_column_right->setText(QApplication::translate("MainWindow", "Move column right", 0, QApplication::UnicodeUTF8));
    actionMove_column_right->setShortcut(QApplication::translate("MainWindow", "Alt+Right", 0, QApplication::UnicodeUTF8));
    actionCopy_column->setText(QApplication::translate("MainWindow", "Copy column", 0, QApplication::UnicodeUTF8));
    actionCopy_column->setShortcut(QApplication::translate("MainWindow", "Ctrl+D", 0, QApplication::UnicodeUTF8));
    actionPaste_column->setText(QApplication::translate("MainWindow", "Paste column", 0, QApplication::UnicodeUTF8));
    actionPaste_column->setShortcut(QApplication::translate("MainWindow", "Ctrl+Shift+V", 0, QApplication::UnicodeUTF8));

    menu.addAction();

    menu.exec(QCursor::pos());
    */
}

void DFG_CLASS_NAME(CsvTableView)::setModel(QAbstractItemModel* pModel)
{
    BaseClass::setModel(pModel);
}

DFG_CLASS_NAME(CsvItemModel)* DFG_CLASS_NAME(CsvTableView)::csvModel()
{
    auto pModel = model();
    auto pCsvModel = dynamic_cast<CsvModel*>(pModel);
    if (pCsvModel)
        return pCsvModel;
    auto pProxyModel = dynamic_cast<QAbstractProxyModel*>(pModel);
    return (pProxyModel) ? dynamic_cast<CsvModel*>(pProxyModel->sourceModel()) : nullptr;
}

int DFG_CLASS_NAME(CsvTableView)::getFirstSelectedViewRow() const
{
    const auto& contItems = getRowsOfSelectedItems(nullptr);
    return (!contItems.empty()) ? *contItems.begin() : model()->rowCount();
}

std::vector<int> DFG_CLASS_NAME(CsvTableView)::getRowsOfCol(const int nCol, const QAbstractProxyModel* pProxy) const
{
    std::vector<int> vec(model()->rowCount());
    if (!pProxy)
    {
        DFG_MODULE_NS(alg)::generateAdjacent(vec, 0, 1);
    }
    else
    {
        for (int i = 0; i<int(vec.size()); ++i)
            vec[i] = pProxy->mapToSource(pProxy->index(i, nCol)).row();
    }
    return vec;
}

QModelIndexList DFG_CLASS_NAME(CsvTableView)::getSelectedItemIndexes(const QAbstractProxyModel* pProxy) const
{
    QModelIndexList listSelected = selectedIndexes();
    if (pProxy)
    {
        for (int i = 0; i<listSelected.size(); ++i)
            listSelected[i] = pProxy->mapToSource(listSelected[i]);
    }
    return listSelected;
}

std::vector<int> DFG_CLASS_NAME(CsvTableView)::getRowsOfSelectedItems(const QAbstractProxyModel* pProxy, const bool bSort) const
{
    QModelIndexList listSelected = getSelectedItemIndexes(pProxy);

    std::set<int> setRows;
    std::vector<int> vRows;
    for (QModelIndexList::const_iterator iter = listSelected.begin(); iter != listSelected.end(); ++iter)
    {
        if (iter->isValid())
        {
            if (setRows.find(iter->row()) != setRows.end())
                continue;
            setRows.insert(iter->row());
            vRows.push_back(iter->row());
        }
    }

    DFG_ASSERT(setRows.size() == vRows.size());
    if (bSort)
        std::copy(setRows.begin(), setRows.end(), vRows.begin());
    return vRows;
}

QModelIndex DFG_CLASS_NAME(CsvTableView)::getFirstSelectedItem(QAbstractProxyModel* pProxy) const
{
    QModelIndexList listSelected = selectedIndexes();
    if (listSelected.empty())
        return QModelIndex();
    if (pProxy)
        return pProxy->mapToSource(listSelected[0]);
    else
        return listSelected[0];

}

void DFG_CLASS_NAME(CsvTableView)::invertSelection()
{
    const auto pModel = model();
    if (pModel == nullptr)
        return;
    for (int r = 0; r<pModel->rowCount(); ++r)
    {
        for (int c = 0; c<pModel->columnCount(); ++c)
        {
            QModelIndex index = pModel->index(r, c);
            selectionModel()->select(index, QItemSelectionModel::Toggle);
        }
    }
}

bool DFG_CLASS_NAME(CsvTableView)::isRowMode() const
{
    return false;
}

bool DFG_CLASS_NAME(CsvTableView)::saveToFileImpl(const DFG_ROOT_NS::DFG_CLASS_NAME(CsvFormatDefinition)& formatDef)
{
    auto sPath = QFileDialog::getSaveFileName(this,
        tr("Open file"),
        QString()/*dir*/,
        tr("CSV files (*.csv);;All files (*.*)"),
        nullptr/*selected filter*/,
        0/*options*/);

    if (sPath.isEmpty())
        return false;

    auto pModel = csvModel();

    if (!pModel)
        return false;

    const auto bSuccess = pModel->saveToFile(sPath, formatDef);
    if (!bSuccess)
        QMessageBox::information(nullptr, tr("Save failed"), tr("Failed to save to path %1").arg(sPath));

    return bSuccess;
}

bool DFG_CLASS_NAME(CsvTableView)::saveToFile()
{
    return saveToFileImpl(DFG_CLASS_NAME(CsvItemModel)::SaveOptions());
}

class CsvFormatDefinitionDialog : public QDialog
{
public:
    typedef QDialog BaseClass;
    CsvFormatDefinitionDialog()
    {
        using namespace DFG_MODULE_NS(io);
        auto spLayout = std::unique_ptr<QFormLayout>(new QFormLayout);
        m_pSeparatorEdit = new QComboBox(this);
        m_pEnclosingEdit = new QComboBox(this);
        m_pEolEdit = new QComboBox(this);
        m_pEncodingEdit = new QComboBox(this);
        m_pSaveHeader = new QCheckBox(this);
        m_pWriteBOM = new QCheckBox(this);

        m_pSeparatorEdit->addItems(QStringList() << "," << "\\t" << ";");
        m_pSeparatorEdit->setEditable(true);
        m_pEnclosingEdit->addItem("\"");
        m_pEnclosingEdit->setEditable(true);
        m_pEolEdit->addItems(QStringList() << "\\n" << "\\r" << "\\r\\n");
        m_pEolEdit->setEditable(false);
        m_pEncodingEdit->addItems(QStringList() << encodingToStrId(encodingUTF8) << encodingToStrId(encodingLatin1));
        m_pEncodingEdit->setEditable(false);
        m_pSaveHeader->setChecked(true);
        m_pWriteBOM->setChecked(true);

        spLayout->addRow(tr("Separator char"), m_pSeparatorEdit);
        spLayout->addRow(tr("Enclosing char"), m_pEnclosingEdit);
        spLayout->addRow(tr("End-of-line"), m_pEolEdit);
        spLayout->addRow(tr("Save header"), m_pSaveHeader);
        spLayout->addRow(tr("Encoding"), m_pEncodingEdit);
        spLayout->addRow(tr("Write BOM"), m_pWriteBOM);

        auto& rButtonBox = *(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel));

        connect(&rButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(&rButtonBox, SIGNAL(rejected()), this, SLOT(reject()));


        spLayout->addRow(QString(), &rButtonBox);
        setLayout(spLayout.release());
    }

    void accept() override
    {
        using namespace DFG_MODULE_NS(io);
        if (!m_pSeparatorEdit || !m_pEnclosingEdit || !m_pEolEdit || !m_pSaveHeader || !m_pWriteBOM || !m_pEncodingEdit)
        {
            QMessageBox::information(this, tr("CSV saving"), tr("Internal error occurred; saving failed."));
            return;
        }
        auto sSep = m_pSeparatorEdit->currentText().trimmed();
        auto sEnc = m_pEnclosingEdit->currentText().trimmed();
        auto sEol = m_pEolEdit->currentText().trimmed();

        DFG_MODULE_NS(io)::EndOfLineType eolType = DFG_MODULE_NS(io)::EndOfLineTypeNative;

        const auto sep = DFG_MODULE_NS(str)::stringLiteralCharToValue<wchar_t>(sSep.toStdWString());
        const auto enc = DFG_MODULE_NS(str)::stringLiteralCharToValue<wchar_t>(sEnc.toStdWString());

        if (sEol == "\\n")
            eolType = DFG_MODULE_NS(io)::EndOfLineTypeN;
        else if (sEol == "\\r")
            eolType = DFG_MODULE_NS(io)::EndOfLineTypeR;
        else if (sEol == "\\r\\n")
            eolType = DFG_MODULE_NS(io)::EndOfLineTypeRN;

        // TODO: check for identical values (e.g. require that sep != enc)
        if (!sep.first || !enc.first || eolType == DFG_MODULE_NS(io)::EndOfLineTypeNative)
        {
            // TODO: more informative message for the user.
            QMessageBox::information(this, tr("CSV saving"), tr("Chosen settings can't be used. Please revise the selections."));
            return;
        }

        m_formatDef.m_cEnc = enc.second;
        m_formatDef.m_cSep = sep.second;
        m_formatDef.m_eolType = eolType;
        m_formatDef.headerWriting(m_pSaveHeader->isChecked());
        m_formatDef.bomWriting(m_pWriteBOM->isChecked());
        m_formatDef.textEncoding(strIdToEncoding(m_pEncodingEdit->currentText().toLatin1().data()));

        BaseClass::accept();
    }

    DFG_CLASS_NAME(CsvItemModel)::SaveOptions m_formatDef;
    QComboBox* m_pSeparatorEdit;
    QComboBox* m_pEnclosingEdit;
    QComboBox* m_pEolEdit;
    QComboBox* m_pEncodingEdit;
    QCheckBox* m_pSaveHeader;
    QCheckBox* m_pWriteBOM;
};

bool DFG_CLASS_NAME(CsvTableView)::saveToFileWithOptions()
{
    CsvFormatDefinitionDialog dlg;
    if (dlg.exec() != QDialog::Accepted)
        return false;
    return saveToFileImpl(dlg.m_formatDef);
    
}

bool DFG_CLASS_NAME(CsvTableView)::openFromFile()
{
    auto sPath = QFileDialog::getOpenFileName(this,
        tr("Open file"),
        QString()/*dir*/,
        tr("CSV files (*.csv *.tsv);;All files (*.*)"),
                                                nullptr/*selected filter*/,
                                                0/*options*/);
    if (sPath.isEmpty())
        return false;

    auto pModel = csvModel();
    if (pModel)
    {
        const auto bSuccess = pModel->openFile(sPath);
        if (!bSuccess)
            QMessageBox::information(nullptr, "", QString("Failed to open file '%1'").arg(sPath));
        return bSuccess;
    }
    else
        return false;
}

template <class T, class Param0_T>
bool DFG_CLASS_NAME(CsvTableView)::executeAction(Param0_T&& p0)
{
    if (m_spUndoStack)
        pushToUndoStack<T>(std::forward<Param0_T>(p0));
    else
        T(std::forward<Param0_T>(p0)).redo();

    return true;
}

template <class T, class Param0_T, class Param1_T>
bool DFG_CLASS_NAME(CsvTableView)::executeAction(Param0_T&& p0, Param1_T&& p1)
{
    if (m_spUndoStack)
        pushToUndoStack<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));
    else
        T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1)).redo();

    return true;
}

template <class T, class Param0_T, class Param1_T, class Param2_T>
bool DFG_CLASS_NAME(CsvTableView)::executeAction(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2)
{
    if (m_spUndoStack)
        pushToUndoStack<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2));
    else
        T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2)).redo();

    return true;
}

template <class T, class Param0_T>
void DFG_CLASS_NAME(CsvTableView)::pushToUndoStack(Param0_T&& p0)
{
    if (!m_spUndoStack)
        createUndoStack();
    QUndoCommand* command = new T(std::forward<Param0_T>(p0));
    (*m_spUndoStack)->push(command); // Stack takes ownership of command.
}

template <class T, class Param0_T, class Param1_T>
void DFG_CLASS_NAME(CsvTableView)::pushToUndoStack(Param0_T&& p0, Param1_T&& p1)
{
    if (!m_spUndoStack)
        createUndoStack();
    QUndoCommand* command = new T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));
    (*m_spUndoStack)->push(command); // Stack takes ownership of command.
}

template <class T, class Param0_T, class Param1_T, class Param2_T>
void DFG_CLASS_NAME(CsvTableView)::pushToUndoStack(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2)
{
    if (!m_spUndoStack)
        createUndoStack();
    QUndoCommand* command = new T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2));
    (*m_spUndoStack)->push(command); // Stack takes ownership of command.
}

bool DFG_CLASS_NAME(CsvTableView)::clearSelected()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionDelete)>(*this, getProxyModelPtr(), false /*false = not row mode*/);
}

bool DFG_CLASS_NAME(CsvTableView)::insertRowHere()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionInsertRow)>(this, DFG_SUB_NS_NAME(undoCommands)::InsertRowTypeBefore);
}

bool DFG_CLASS_NAME(CsvTableView)::insertRowAfterCurrent()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionInsertRow)>(this, DFG_SUB_NS_NAME(undoCommands)::InsertRowTypeAfter);
}

bool DFG_CLASS_NAME(CsvTableView)::insertColumn()
{
    const auto nCol = currentIndex().column();
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionInsertColumn)>(csvModel(), nCol);
}

bool DFG_CLASS_NAME(CsvTableView)::insertColumnAfterCurrent()
{
    const auto nCol = currentIndex().column();
    if (nCol >= 0)
        return executeAction<DFG_CLASS_NAME(CsvTableViewActionInsertColumn)>(csvModel(), nCol + 1);
    else
        return false;
}

bool DFG_CLASS_NAME(CsvTableView)::cut()
{
    copy();
    clearSelected();
    return true;
}

bool DFG_CLASS_NAME(CsvTableView)::copy()
{
    auto vViewRows = getRowsOfSelectedItems(nullptr, false);
    auto vRows = getRowsOfSelectedItems(getProxyModelPtr(), false);
    auto pModel = csvModel();
    if (vRows.empty() || !pModel)
        return false;
    QString str;
    // Not sorting because it's probably more intuitive to get
    // items in that order in which they were shown.
    //std::sort(vRows.begin(), vRows.end());
    //std::sort(vViewRows.begin(), vViewRows.end());
    DFG_ASSERT(vViewRows.size() == vRows.size());
    QItemSelection selection;
    const bool bRowMode = isRowMode();
    for (size_t i = 0; i<vRows.size(); ++i)
    {
        QAbstractItemModel* pEffectiveModel = getProxyModelPtr();
        if (pEffectiveModel == nullptr)
            pEffectiveModel = pModel;

        if (!bRowMode)
        {
            std::unordered_set<int> setIgnoreCols;

            for (int col = 0; col < pModel->columnCount(); ++col)
            {
                if (!selectionModel()->isSelected(pEffectiveModel->index(vViewRows[i], col)))
                    setIgnoreCols.insert(col);
                else
                    selection.select(pEffectiveModel->index(vViewRows[i], col), pEffectiveModel->index(vViewRows[i], col));
            }
            pModel->rowToString(vRows[i], str, '\t', &setIgnoreCols);
        }
        else
        {
            pModel->rowToString(vRows[i], str, '\t');
            selection.select(pEffectiveModel->index(vViewRows[i], 0), pEffectiveModel->index(vViewRows[i], 0));
        }
        str.push_back('\n');

    }
    if (bRowMode)
        selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    else
        selectionModel()->select(selection, QItemSelectionModel::Select);

    QApplication::clipboard()->setText(str);

    return true;
}

bool DFG_CLASS_NAME(CsvTableView)::paste()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionPaste)>(this);
}

bool DFG_CLASS_NAME(CsvTableView)::deleteCurrentColumn()
{
    const auto curIndex = currentIndex();
    const int nRow = curIndex.row();
    const int nCol = curIndex.column();
    const auto rv = deleteCurrentColumn(nCol);
    selectionModel()->setCurrentIndex(model()->index(nRow, nCol), QItemSelectionModel::NoUpdate);
    return rv;
}

bool DFG_CLASS_NAME(CsvTableView)::deleteCurrentColumn(const int nCol)
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionDeleteColumn)>(this, nCol);
}

bool DFG_CLASS_NAME(CsvTableView)::deleteSelectedRow()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionDelete)>(*this, getProxyModelPtr(), true /*row mode*/);
}

bool DFG_CLASS_NAME(CsvTableView)::resizeTable()
{
    auto pModel = model();
    if (!pModel)
        return false;
    // TODO: ask new dimensions with a single dialog rather than with two separate.
    bool bOk = false;
    const int nRows = QInputDialog::getInt(this, tr("Table resizing"), tr("New row count"), pModel->rowCount(), 0, NumericTraits<int>::maxValue, 1, &bOk);
    if (!bOk)
        return false;
    const int nCols = QInputDialog::getInt(this, tr("Table resizing"), tr("New column count"), pModel->columnCount(), 0, NumericTraits<int>::maxValue, 1, &bOk);
    if (!bOk || nRows < 0 || nCols < 0)
        return false;
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionResizeTable)>(this, nRows, nCols);
}

bool DFG_CLASS_NAME(CsvTableView)::moveFirstRowToHeader()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionMoveFirstRowToHeader)>(this);
}

bool DFG_CLASS_NAME(CsvTableView)::moveHeaderToFirstRow()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionMoveFirstRowToHeader)>(this, true);
}

QAbstractProxyModel* DFG_CLASS_NAME(CsvTableView)::getProxyModelPtr()
{
    return dynamic_cast<QAbstractProxyModel*>(model());
}

const QAbstractProxyModel* DFG_CLASS_NAME(CsvTableView)::getProxyModelPtr() const
{
    return dynamic_cast<const QAbstractProxyModel*>(model());
}
