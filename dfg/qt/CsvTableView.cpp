#include "../buildConfig.hpp" // To get rid of C4996 "Function call with parameters that may be unsafe" in MSVC.
#include "qtIncludeHelpers.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"
#include "CsvTableViewActions.hpp"
#include "QtApplication.hpp"
#include "../os.hpp"
#include "../os/TemporaryFileStream.hpp"
#include "PropertyHelper.hpp"

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
#include <QLabel>
#include <QMessageBox>
#include <QMetaMethod>
#include <QProcess>
#include <QSettings>
#include <QToolTip>
DFG_END_INCLUDE_QT_HEADERS

#include <set>
#include "../alg.hpp"
#include "../cont/SortedSequence.hpp"
#include "../str/stringLiteralCharToValue.hpp"

using namespace DFG_MODULE_NS(qt);

namespace
{
    enum CsvTableViewPropertyId
    {
        CsvTableViewPropertyId_diffProgPath
    };

    DFG_QT_DEFINE_OBJECT_PROPERTY_CLASS(CsvTableView)

    template <CsvTableViewPropertyId ID>
    auto getCsvTableViewProperty(DFG_CLASS_NAME(CsvTableView)* view) -> typename CsvTableViewPropertyDefinition<ID>::PropertyType
    {
        return DFG_MODULE_NS(qt)::getProperty<DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CsvTableView)<ID>>(view);
    }

    // Properties
    DFG_QT_DEFINE_OBJECT_PROPERTY("diffProgPath", CsvTableView, CsvTableViewPropertyId_diffProgPath, QString, PropertyType);

} // unnamed namespace

DFG_CLASS_NAME(CsvTableView)::DFG_CLASS_NAME(CsvTableView)(QWidget* pParent)
    : BaseClass(pParent)
    , m_nFindColumnIndex(0)
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
        auto pAction = new QAction(tr("Merge files to current..."), this);
        connect(pAction, &QAction::triggered, this, &ThisClass::mergeFilesToCurrent);
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Save"), this);
        pAction->setShortcut(tr("Ctrl+S"));
        connect(pAction, &QAction::triggered, this, &ThisClass::save);
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
        auto pAction = new QAction(tr("Generate content..."), this);
        //pAction->setShortcut(tr(""));
        connect(pAction, &QAction::triggered, this, &ThisClass::generateContent);
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

    // Find actions
    {
        {
            auto pAction = new QAction(tr("Find"), this);
            pAction->setShortcut(tr("Ctrl+F"));
            connect(pAction, &QAction::triggered, this, &ThisClass::onFindRequested);
            addAction(pAction);
        }
        {
            auto pAction = new QAction(tr("Find next"), this);
            pAction->setShortcut(tr("F3"));
            connect(pAction, &QAction::triggered, this, &ThisClass::onFindNext);
            addAction(pAction);
        }
        {
            auto pAction = new QAction(tr("Find previous"), this);
            pAction->setShortcut(tr("Shift+F3"));
            connect(pAction, &QAction::triggered, this, &ThisClass::onFindPrevious);
            addAction(pAction);
        }
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

    // -------------------------------------------------
    {
        // Add diff-action
        {
            auto pAction = new QAction(tr("Diff with unmodified"), this);
            pAction->setShortcut(tr("Alt+D"));
            connect(pAction, &QAction::triggered, this, &ThisClass::diffWithUnmodified);
            addAction(pAction);
        }
    }
}

DFG_CLASS_NAME(CsvTableView)::~DFG_CLASS_NAME(CsvTableView)()
{
    for(const auto& path : m_tempFilePathToRemoveOnExit)
    {
        QFile::remove(path);
    }
}

void DFG_CLASS_NAME(CsvTableView)::createUndoStack()
{
    m_spUndoStack.reset(new DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TorRef)<QUndoStack>);
}

void DFG_CLASS_NAME(CsvTableView)::clearUndoStack()
{
    if (m_spUndoStack)
        m_spUndoStack->item().clear();
}

namespace
{
    const char gszMenuText_clearUndoBuffer[] = "&Clear undo-buffer";
}

void DFG_CLASS_NAME(CsvTableView)::privAddUndoRedoActions(QAction* pAddBefore)
{
    if (!m_spUndoStack)
        createUndoStack();
    if (m_spUndoStack)
    {
        // Add undo-action
        auto pActionUndo = m_spUndoStack->item().createUndoAction(this, tr("&Undo"));
        pActionUndo->setShortcuts(QKeySequence::Undo);
        insertAction(pAddBefore, pActionUndo);

        // Add redo-action
        auto pActionRedo = m_spUndoStack->item().createRedoAction(this, tr("&Redo"));
        pActionRedo->setShortcuts(QKeySequence::Redo);
        insertAction(pAddBefore, pActionRedo);

        // Add Clear undo-buffer -action
        auto pActionClearUndoBuffer = new QAction(tr(gszMenuText_clearUndoBuffer), this);
        connect(pActionClearUndoBuffer, &QAction::triggered, this, &ThisClass::clearUndoStack);
        addAction(pActionClearUndoBuffer);
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
        if (!pAction)
            continue;
        if (pAction->shortcut() == QKeySequence::Undo)
            removeList.push_back(pAction);
        else if (pAction->shortcut() == QKeySequence::Redo)
            removeList.push_back(pAction);
        else if (pAction->text() == tr(gszMenuText_clearUndoBuffer))
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
    if (m_spUndoStack)
    {
        auto pCsvModel = dynamic_cast<CsvModel*>(pModel);
        if (pCsvModel)
            pCsvModel->setUndoStack(&m_spUndoStack->item());
    }
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
    return saveToFileImpl(sPath, formatDef);
}

bool DFG_CLASS_NAME(CsvTableView)::saveToFileImpl(const QString& path, const DFG_ROOT_NS::DFG_CLASS_NAME(CsvFormatDefinition)& formatDef)
{
    auto pModel = csvModel();

    if (!pModel)
        return false;

    // TODO: Don't block caller thread.
    // TODO: add signals 'savingFile()' and 'savingFinished(time)' or similar so that owner GUI can show the info to user.
    // TODO: allow user to cancel saving (e.g. if it takes too long)
    const auto bSuccess = pModel->saveToFile(path, formatDef);
    if (!bSuccess)
        QMessageBox::information(nullptr, tr("Save failed"), tr("Failed to save to path %1").arg(path));

    return bSuccess;
}


bool DFG_CLASS_NAME(CsvTableView)::save()
{
    auto model = csvModel();
    const auto& path = (model) ? model->getFilePath() : QString();
    if (!path.isEmpty())
        return saveToFileImpl(path, DFG_CLASS_NAME(CsvItemModel)::SaveOptions());
    else
        return saveToFile();
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
            QMessageBox::information(nullptr, "", tr("Failed to open file '%1'").arg(sPath));
        return bSuccess;
    }
    else
        return false;
}

bool DFG_CLASS_NAME(CsvTableView)::mergeFilesToCurrent()
{
    auto sPaths = QFileDialog::getOpenFileNames(this,
        tr("Select files to merge"),
        QString()/*dir*/,
        tr("CSV files (*.csv *.tsv);;All files (*.*)"),
        nullptr/*selected filter*/,
        0/*options*/);
    if (sPaths.isEmpty())
        return false;

    auto pModel = csvModel();
    if (pModel)
    {
        const auto bSuccess = pModel->importFiles(sPaths);
        if (!bSuccess)
            QMessageBox::information(nullptr, "", tr("Failed to merge files"));
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
            CsvModel::IndexSet setIgnoreCols;

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

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QDialog>
    #include <QComboBox>
    #include <QStyledItemDelegate>
DFG_END_INCLUDE_QT_HEADERS

#include "connectHelper.hpp"

namespace
{
    // Note: ID's should match values in arrPropDefs-array.
    enum PropertyId
    {
        PropertyIdInvalid = -1,
        PropertyIdTarget,
        PropertyIdGenerator,
        LastNonParamPropertyId = PropertyIdGenerator,
        PropertyIdMinValueInt,
        PropertyIdMaxValueInt,
        PropertyIdMinValueDouble,
        PropertyIdMaxValueDouble,
    };

    enum GeneratorType
    {
        GeneratorTypeUnknown,
        GeneratorTypeRandomIntegers,
        GeneratorTypeRandomDoubles,
        GeneratorTypeFill,
        GeneratorType_last = GeneratorTypeFill
    };

    enum TargetType
    {
        TargetTypeUnknown,
        TargetTypeSelection,
        TargetTypeWholeTable,
        TargetType_last = TargetTypeWholeTable
    };

    enum ValueType
    {
        ValueTypeKeyList,
        ValueTypeInteger,
        ValueTypeUInteger = ValueTypeInteger,
        ValueTypeDouble,
        ValueTypeString,
    };

    struct PropertyDefinition
    {
        const char* m_pszName;
        int m_nType;
        const char* m_keyList;
        const char* m_pszDefault;
    };

    // Note: this is a POD-table (for notes about initialization of PODs, see
    //    -http://stackoverflow.com/questions/2960307/pod-global-object-initialization
    //    -http://stackoverflow.com/questions/15212261/default-initialization-of-pod-types-in-c
    const PropertyDefinition arrPropDefs[] =
    {
        // Key name       Value type         Value items (if key type is list)           Default value
        //                                   In syntax |x;y;z... items x,y,z define
        //                                   the indexes in this table that are 
        //                                   parameters for given item.
        { "Target"              , ValueTypeKeyList  , "Selection,Whole table"                               , "Selection"       },
        { "Generator"           , ValueTypeKeyList  , "Random integers|2;3,Random doubles|4;5;6;7,Fill|8"   , "Random integers" },
        { "Min value"           , ValueTypeInteger  , ""                                                    , "0"               },
        { "Max value"           , ValueTypeInteger  , ""                                                    , "32767"           },
        { "Min value"           , ValueTypeDouble   , ""                                                    , "0.0"             },
        { "Max value"           , ValueTypeDouble   , ""                                                    , "1.0"             },
        { "Format type"         , ValueTypeString   , ""                                                    , "g"               }, 
        { "Format precision"    , ValueTypeUInteger , ""                                                    , "6"               }, // Note: empty value must be supported as well.
        { "Fill string"         , ValueTypeString   , ""                                                    , ""                }
    };

    PropertyId rowToPropertyId(const int r)
    {
        if (r == 0)
            return PropertyIdTarget;
        else if (r == 1)
            return PropertyIdGenerator;
        else
            return PropertyIdInvalid;
    }

    QStringList valueListFromProperty(const PropertyId id)
    {
        if (!::DFG_ROOT_NS::isValidIndex(arrPropDefs, id))
            return QStringList();
        QStringList items = QString(arrPropDefs[id].m_keyList).split(',');
        for (auto iter = items.begin(); iter != items.end(); ++iter)
        {
            const auto n = iter->indexOf('|');
            if (n < 0)
                continue;
            iter->remove(n, iter->length() - n);
        }
        return items;
    }

    class ContentGeneratorDialog;

    class ComboBoxDelegate : public QStyledItemDelegate
    {
        typedef QStyledItemDelegate BaseClass;

    public:
        ComboBoxDelegate(ContentGeneratorDialog* parent);

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        void setEditorData(QWidget *editor, const QModelIndex &index) const override;
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

        void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

        ContentGeneratorDialog* m_pParentDialog;
    };

    class ContentGeneratorDialog : public QDialog
    {
    public:
        typedef QDialog BaseClass;
        ContentGeneratorDialog(QWidget* pParent) : 
            BaseClass(pParent),
            m_pLayout(nullptr),
            m_pGeneratorControlsLayout(nullptr),
            m_nLatestComboBoxItemIndex(-1)
        {
            m_spSettingsTable.reset(new DFG_CLASS_NAME(CsvTableView(this)));
            m_spSettingsModel.reset(new DFG_CLASS_NAME(CsvItemModel));
            m_spSettingsTable->setModel(m_spSettingsModel.get());
            m_spSettingsTable->setItemDelegate(new ComboBoxDelegate(this));


            m_spSettingsModel->insertColumns(0, 2);

            m_spSettingsModel->setColumnName(0, tr("Parameter"));
            m_spSettingsModel->setColumnName(1, tr("Value"));

            // Set streching for the last column
            {
                auto pHeader = m_spSettingsTable->horizontalHeader();
                if (pHeader)
                    pHeader->setStretchLastSection(true);
            }
            // 
            {
                auto pHeader = m_spSettingsTable->verticalHeader();
                if (pHeader)
                    pHeader->setDefaultSectionSize(30);
            }
            m_pLayout = new QVBoxLayout(this);


            m_pLayout->addWidget(m_spSettingsTable.get());

            for (size_t i = 0; i <= LastNonParamPropertyId; ++i)
            {
                const auto nRow = m_spSettingsModel->rowCount();
                m_spSettingsModel->insertRows(nRow, 1);
                m_spSettingsModel->setData(m_spSettingsModel->index(nRow, 0), tr(arrPropDefs[i].m_pszName));
                m_spSettingsModel->setData(m_spSettingsModel->index(nRow, 1), arrPropDefs[i].m_pszDefault);
            }

            m_pLayout->addWidget(new QLabel(tr("Note: undo is not yet available for content generation"), this));

            auto& rButtonBox = *(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel));

            connect(&rButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
            connect(&rButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

            m_pLayout->addWidget(&rButtonBox);

            createPropertyParams(PropertyIdGenerator, 0);

            DFG_QT_VERIFY_CONNECT(connect(m_spSettingsModel.get(), &QAbstractItemModel::dataChanged, this, &ContentGeneratorDialog::onDataChanged));
        }

        PropertyId rowToPropertyId(const int i) const
        {
            if (i == PropertyIdGenerator)
                return PropertyIdGenerator;
            else
                return PropertyIdInvalid;
        }

        int propertyIdToRow(const PropertyId propId) const
        {
            if (propId == PropertyIdGenerator)
                return 1;
            else
                return -1;
        }

        std::vector<std::reference_wrapper<const PropertyDefinition>> generatorParameters(const int itemIndex)
        {
            std::vector<std::reference_wrapper<const PropertyDefinition>> rv;
            QString sKeyList = arrPropDefs[PropertyIdGenerator].m_keyList;
            const auto keys = sKeyList.split(',');
            if (!DFG_ROOT_NS::isValidIndex(keys, itemIndex))
                return rv;
            QString sName = keys[itemIndex];
            const auto n = sName.indexOf('|');
            if (n < 0)
                return rv;
            sName.remove(0, n + 1);
            const auto paramIndexes = sName.split(';');
            for (int i = 0, nCount = paramIndexes.size(); i < nCount; ++i)
            {
                bool bOk = false;
                const auto index = paramIndexes[i].toInt(&bOk);
                if (bOk && DFG_ROOT_NS::isValidIndex(arrPropDefs, index))
                    rv.push_back(arrPropDefs[index]);
                else
                {
                    DFG_ASSERT(false);
                }
            }
            return rv;
        }

        void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>&/* roles*/)
        {
            const auto isCellInIndexRect = [](int r, int c, const QModelIndex& tl, const QModelIndex&  br)
                                            {
                                                return (r >= tl.row() && r <= br.row() && c >= tl.column() && c <= br.column());
                                            };
            if (isCellInIndexRect(1, 1, topLeft, bottomRight))
                createPropertyParams(rowToPropertyId(1), m_nLatestComboBoxItemIndex);
        }

        void createPropertyParams(const PropertyId prop, const int itemIndex)
        {
            if (!m_spSettingsModel)
            {
                DFG_ASSERT(false);
                return;
            }
            if (prop == PropertyIdGenerator)
            {
                const auto& params = generatorParameters(itemIndex);
                const auto nParamCount = static_cast<int>(params.size());
                auto nBaseRow = propertyIdToRow(PropertyIdGenerator);
                if (nBaseRow < 0)
                {
                    DFG_ASSERT(false);
                    return;
                }
                ++nBaseRow;
                if (m_spSettingsModel->rowCount() < nBaseRow + nParamCount) // Need to add rows?
                    m_spSettingsModel->insertRows(nBaseRow, nBaseRow + nParamCount - m_spSettingsModel->rowCount());
                if (m_spSettingsModel->rowCount() > nBaseRow + nParamCount) // Need to remove rows?
                    m_spSettingsModel->removeRows(nBaseRow, m_spSettingsModel->rowCount() - (nBaseRow + nParamCount));
                for (int i = 0; i < nParamCount; ++i)
                {
                    m_spSettingsModel->setData(m_spSettingsModel->index(nBaseRow + i, 0), params[i].get().m_pszName);
                    m_spSettingsModel->setData(m_spSettingsModel->index(nBaseRow + i, 1), params[i].get().m_pszDefault);
                }
            }
            else
            {
                DFG_ASSERT_IMPLEMENTED(false);
            }
        }

        void setLatestComboBoxItemIndex(int index)
        {
            m_nLatestComboBoxItemIndex = index;
        }

        QVBoxLayout* m_pLayout;
        QGridLayout* m_pGeneratorControlsLayout;
        std::unique_ptr<DFG_CLASS_NAME(CsvTableView)> m_spSettingsTable;
        std::unique_ptr<DFG_CLASS_NAME(CsvItemModel)> m_spSettingsModel;
        int m_nLatestComboBoxItemIndex;
    }; // Class ContentGeneratorDialog

    ComboBoxDelegate::ComboBoxDelegate(ContentGeneratorDialog* parent) :
        m_pParentDialog(parent)
    {
    }

    QWidget* ComboBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        if (!index.isValid())
            return nullptr;
        const auto nRow = index.row();
        const auto nCol = index.column();
        if (nCol != 1) // Only second column is editable.
            return nullptr;

        else if (nRow < 2)
        {
            auto pComboBox = new QComboBox(parent);
            DFG_QT_VERIFY_CONNECT(connect(pComboBox, SIGNAL(currentIndexChanged(int)), pComboBox, SLOT(close()))); // TODO: check this, Qt's star delegate example connects differently here.
            return pComboBox;
        }
        else
            return BaseClass::createEditor(parent, option, index);
    }

    void ComboBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
    {
        auto pModel = index.model();
        if (!pModel)
            return;
        auto pComboBoxDelegate = dynamic_cast<QComboBox*>(editor);
        if (pComboBoxDelegate == nullptr)
        {
            BaseClass::setEditorData(editor, index);
            return;
        }
        const auto keyVal = pModel->data(pModel->index(index.row(), index.column() - 1));
        const auto value = index.data(Qt::EditRole).toString();

        const auto& values = valueListFromProperty(rowToPropertyId(index.row()));

        pComboBoxDelegate->addItems(values);
        pComboBoxDelegate->setCurrentText(value);
    }

    void ComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        if (!model)
            return;
        auto pComboBoxDelegate = dynamic_cast<QComboBox*>(editor);
        if (pComboBoxDelegate)
        {
            const auto& value = pComboBoxDelegate->currentText();
            const auto selectionIndex = pComboBoxDelegate->currentIndex();
            if (m_pParentDialog)
                m_pParentDialog->setLatestComboBoxItemIndex(selectionIndex);
            model->setData(index, value, Qt::EditRole);
        }
        else
            BaseClass::setModelData(editor, model, index);

    }

    void ComboBoxDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        DFG_UNUSED(index);
        if (editor)
            editor->setGeometry(option.rect);
    }

    TargetType targetType(const DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)& csvModel)
    {
        // TODO: use more reliable detection (string comparison does not work with tr())
        DFG_STATIC_ASSERT(TargetType_last == 2, "This implementation handles only two target types");
        const auto& sTarget = csvModel.data(csvModel.index(0, 1)).toString();
        if (sTarget == "Selection")
            return TargetTypeSelection;
        else if (sTarget == "Whole table")
            return TargetTypeWholeTable;
        else
        {
            DFG_ASSERT_IMPLEMENTED(false);
            return TargetTypeUnknown;
        }
    }

    GeneratorType generatorType(const DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)& csvModel)
    {
        // TODO: use more reliable detection (string comparison does not work with tr())
        DFG_STATIC_ASSERT(GeneratorType_last == 3, "This implementation handles only two generator types");
        const auto& sGenerator = csvModel.data(csvModel.index(1, 1)).toString();
        if (sGenerator == "Random integers")
            return GeneratorTypeRandomIntegers;
        else if (sGenerator == "Random doubles")
            return GeneratorTypeRandomDoubles;
        else if (sGenerator == "Fill")
            return GeneratorTypeFill;
        else
        {
            DFG_ASSERT_IMPLEMENTED(false);
            return GeneratorTypeUnknown;
        }
    }

} // unnamed namespace

bool DFG_CLASS_NAME(CsvTableView)::generateContent()
{
    auto pModel = model();
    if (!pModel)
        return false;

    // TODO: store settings and use them on next dialog open.
    ContentGeneratorDialog dlg(this);
    dlg.resize(350, 300);
    bool bStop = false;
    while (!bStop) // Show dialog until values are accepted or cancel is selected.
    {
        const auto rv = dlg.exec();
        if (rv == QDialog::Accepted && dlg.m_spSettingsModel)
        {
            bStop = generateContentImpl(*dlg.m_spSettingsModel);
            if (bStop)
                return true;
        }
        else
            return false;
    }
    return false;
}

#include "../rand.hpp"

template <class Generator_T>
void generateForEachInTarget(const TargetType targetType, const DFG_CLASS_NAME(CsvTableView)& view, DFG_CLASS_NAME(CsvItemModel)& rModel, Generator_T generator)
{
    DFG_STATIC_ASSERT(TargetType_last == 2, "This implementation handles only two target types");

    if (targetType == TargetTypeWholeTable)
    {
        const auto nRows = rModel.rowCount();
        const auto nCols = rModel.columnCount();
        if (nRows < 1 || nCols < 1) // Nothing to add?
            return;
        size_t nCounter = 0;
        rModel.batchEditNoUndo([&](DFG_CLASS_NAME(CsvItemModel)::DataTable& table)
        {
            for (int c = 0; c < nCols; ++c)
            {
                for (int r = 0; r < nRows; ++r, ++nCounter)
                {
                    generator(table, r, c, nCounter);
                }
            }
        });
    }
    else if (targetType == TargetTypeSelection)
    {
        auto pSelectionModel = view.selectionModel();
        auto pModel = view.model();
        if (pSelectionModel && pModel)
        {
            const auto& selected = pSelectionModel->selectedIndexes();
            size_t nCounter = 0;
            rModel.batchEditNoUndo([&](DFG_CLASS_NAME(CsvItemModel)::DataTable& table)
            {
                for (auto iter = selected.begin(); iter != selected.end(); ++iter, ++nCounter)
                {
                    generator(table, iter->row(), iter->column(), nCounter);
                }
            });
        }
    }
    else
    {
        DFG_ASSERT_IMPLEMENTED(false);
    }
}

namespace
{
    // TODO: test
    bool isValidFloatPrefix(const QStringRef& sPrefix)
    {
        return (sPrefix.isEmpty() ||
            sPrefix == "l" ||
            sPrefix == "L");
    }

    // TODO: test
    bool isValidIntegerPrefix(const QStringRef& sPrefix)
    {
        return (sPrefix.isEmpty() ||
            sPrefix == "I" ||
            sPrefix == "l" ||
            sPrefix == "L" ||
            sPrefix == "ll" ||
            sPrefix == "LL" ||
            sPrefix == "I32" ||
            sPrefix == "I64");
    }

    const char gszIntegerTypes[] = "diouxX";
    const char gszFloatTypes[] = "gGeEfaA";

    // TODO: test
    bool isValidFormatType(const QString& s)
    {
        if (s.isEmpty())
            return false;
        const auto cLast = s[s.size() - 1].toLatin1();
        
        const bool bIsFloatType = (std::strchr(gszFloatTypes, cLast) != nullptr);
        const bool bIsIntergerType = (!bIsFloatType && std::strchr(gszIntegerTypes, cLast) != nullptr);
        if (!bIsFloatType && !bIsIntergerType)
            return false;
        const auto sPrefix = s.midRef(0, s.size() - 1);
        return ((bIsFloatType && isValidFloatPrefix(sPrefix)) ||
                (bIsIntergerType && isValidIntegerPrefix(sPrefix)));
    }
}

bool DFG_CLASS_NAME(CsvTableView)::generateContentImpl(const DFG_CLASS_NAME(CsvItemModel)& settingsModel)
{
    if (settingsModel.rowCount() < 2)
    {
        DFG_ASSERT(false);
        return false;
    }
    const auto generator = generatorType(settingsModel);
    const auto target = targetType(settingsModel);
    auto pModel = csvModel();
    if (!pModel)
        return false;
    auto& rModel = *pModel;

    DFG_STATIC_ASSERT(GeneratorType_last == 3, "This implementation handles only two generator types");
    if (generator == GeneratorTypeRandomIntegers)
    {
        if (settingsModel.rowCount() < 4) // Not enough parameters
            return false;
        const auto minVal = settingsModel.data(settingsModel.index(2, 1)).toString().toLongLong();
        const auto maxVal = settingsModel.data(settingsModel.index(3, 1)).toString().toLongLong();
        auto randEng = ::DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();
        char szBuffer[32];
        const auto generator = [&](DFG_CLASS_NAME(CsvItemModel)::DataTable& table, int r, int c, size_t)
                                {
                                    const auto val = ::DFG_MODULE_NS(rand)::rand(randEng, minVal, maxVal);
                                    DFG_MODULE_NS(str)::toStr(val, szBuffer);
                                    table.setElement(r, c, SzPtrUtf8R(szBuffer));
                                };
        generateForEachInTarget(target, *this, rModel, generator);
        return true;
    }
    else if (generator == GeneratorTypeRandomDoubles)
    {
        if (settingsModel.rowCount() < 6) // Not enough parameters
            return false;
        const auto minVal      = settingsModel.data(settingsModel.index(2, 1)).toString().toDouble();
        const auto maxVal      = settingsModel.data(settingsModel.index(3, 1)).toString().toDouble();
        if (minVal > maxVal)
        {
            QMessageBox::information(nullptr, tr("Invalid parameter"), tr("Minimum value is greater than maximum value, no content generation is done"));
            return false;
        }
        const auto sFormatType = settingsModel.data(settingsModel.index(4, 1)).toString().trimmed();
        auto sPrecision   = settingsModel.data(settingsModel.index(5, 1)).toString();
        bool bPrecisionIsUint;
        if (!sPrecision.isEmpty() && (sPrecision.toUInt(&bPrecisionIsUint) > 1000 || !bPrecisionIsUint)) // Not sure does this need to be limited; just cut it somewhere.
        {
            QMessageBox::information(nullptr, tr("Invalid parameter"), tr("Precision-parameter has invalid value; no content generation is done"));
            return false;
        }
        if (!isValidFormatType(sFormatType))
        {
            QMessageBox::information(nullptr, tr("Invalid parameter"), tr("Format type parameter is not accepted. Note: only a subset of printf-valid items can be used."));
            return false;
        }
        if (!sPrecision.isEmpty())
            sPrecision.prepend('.');
        std::string sFormat(("%" + sPrecision + sFormatType).toLatin1());
        const auto pszFormat = sFormat.c_str();

        auto randEng = ::DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();
        char szBuffer[64];
        const auto generator = [&](DFG_CLASS_NAME(CsvItemModel)::DataTable& table, int r, int c, size_t)
                                {
                                    const auto val = ::DFG_MODULE_NS(rand)::rand(randEng, minVal, maxVal);
                                    DFG_MODULE_NS(str)::toStr(val, szBuffer, pszFormat);
                                    table.setElement(r, c, SzPtrUtf8R(szBuffer));
                                };
        generateForEachInTarget(target, *this, rModel, generator);
        return true;
    }
    else if (generator == GeneratorTypeFill)
    {
        const auto sFill = settingsModel.data(settingsModel.index(LastNonParamPropertyId + 1, 1)).toString().toUtf8();
        const auto pszFillU8 = SzPtrUtf8(sFill.data());
        const auto generator = [&](DFG_CLASS_NAME(CsvItemModel)::DataTable& table, int r, int c, size_t)
        {
            table.setElement(r, c, pszFillU8);
        };
        generateForEachInTarget(target, *this, rModel, generator);
        return true;
    }
    else
    {
        DFG_ASSERT_IMPLEMENTED(false);
    }
    return false;
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

bool DFG_CLASS_NAME(CsvTableView)::diffWithUnmodified()
{
    const char szTempFileNameTemplate[] = "dfgqtCTV"; // static part for temporary filenames.
    auto dataModelPtr = csvModel();
    if (!dataModelPtr)
        return false;
    if (!DFG_MODULE_NS(os)::isPathFileAvailable(dataModelPtr->getFilePath().toStdWString(), DFG_MODULE_NS(os)::FileModeRead))
        return false;

    auto sDiffPath = getCsvTableViewProperty<CsvTableViewPropertyId_diffProgPath>(this);

    if (sDiffPath.isEmpty())
    {
        const auto rv = QMessageBox::question(this,
                                              tr("Unable to locate diff viewer"),
                                              tr("Diff viewer path was not found; locate it manually?")
                                              );
        if (rv != QMessageBox::Yes)
            return false;
        const auto manuallyLocatedDiffer = QFileDialog::getOpenFileName(this, tr("Locate diff viewer"));
        if (manuallyLocatedDiffer.isEmpty())
            return false;
        // TODO: store this to in-memory property set so it doesn't need to be queried again for this run.
        sDiffPath = manuallyLocatedDiffer;
    }

    typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStreamWithEncoding) StreamT;
    DFG_MODULE_NS(os)::DFG_CLASS_NAME(TemporaryFileStreamT)<StreamT> strmTemp(nullptr, // nullptr = use default temp path
                                                                              szTempFileNameTemplate,
                                                                              nullptr, // nullptr = no suffix
                                                                              ".csv" // extension
                                                                              );
    strmTemp.setAutoRemove(false); // To not remove the file while it's being used by diff viewer.
    strmTemp.stream().m_streamBuffer.m_encodingBuffer.setEncoding(DFG_MODULE_NS(io)::encodingUnknown);

    // TODO: saving can take time -> don't freeze GUI while it is being done.
    if (!dataModelPtr->save(strmTemp.stream()))
    { // Saving file unsuccessfull, can't diff.
        QMessageBox::information(this,
                                 tr("Unable to diff"),
                                 tr("Saving current temporary document for diffing failed -> unable to diff"));
        return false;
    }

    const QString sEditedFileTempPath = QString::fromUtf8(strmTemp.pathU8().c_str());

    strmTemp.close();

    const bool bStarted = QProcess::startDetached(sDiffPath,
                                                  QStringList() << dataModelPtr->getFilePath() << sEditedFileTempPath);
    if (!bStarted)
    {
        QMessageBox::information(this, tr("Unable to diff"), tr("Couldn't start diff application from path '%1'").arg(sDiffPath));
        strmTemp.setAutoRemove(true);
        return false;
    }
    else
    {
        m_tempFilePathToRemoveOnExit.push_back(QString::fromUtf8(toCharPtr_raw(strmTemp.pathU8())));
        return true;
    }
}

void DFG_CLASS_NAME(CsvTableView)::setAllowApplicationSettingsUsage(bool b)
{
    setProperty("dfglib_allow_app_settings_usage", b);
}

void DFG_CLASS_NAME(CsvTableView)::finishEdits()
{
    const auto viewState = state();
    if (viewState == EditingState)
    {
        // For now approximate that focus widget is our editor.
        auto focusWidget = QApplication::focusWidget();
        if (focusWidget)
            focusWidget->clearFocus();
    }
}

int DFG_CLASS_NAME(CsvTableView)::getFindColumnIndex() const
{
    return m_nFindColumnIndex;
}

void DFG_CLASS_NAME(CsvTableView)::onFindRequested()
{
    static const QMetaMethod findActivatedSignal = QMetaMethod::fromSignal(&ThisClass::sigFindActivated);
    if (isSignalConnected(findActivatedSignal))
    {
        Q_EMIT sigFindActivated();
    }
    else
    {
        // TODO: use find panel here.
        bool bOk;
        auto s = QInputDialog::getText(this, tr("Find"), tr("Set text to search for"), QLineEdit::Normal, QString(), &bOk);
        if (bOk)
            setFindText(s, m_nFindColumnIndex);
    }
}

void DFG_CLASS_NAME(CsvTableView)::onFindNext()
{
    if (m_findText.isEmpty())
        return;
    auto pBaseModel = csvModel();
    if (!pBaseModel)
        return;

    // Note: this does 'column only'-search
    m_currentFindIndex = pBaseModel->index(1 + m_currentFindIndex.row(), getFindColumnIndex());
    const auto indexList = pBaseModel->match(m_currentFindIndex,
                                                 Qt::DisplayRole,
                                                 m_findText,
                                                 1, // Search for one match only.
                                                 Qt::MatchContains | Qt::MatchWrap);

    if (!indexList.isEmpty())
    {
        scrollTo(indexList.front());
        m_currentFindIndex = indexList.front();
        setCurrentIndex(m_currentFindIndex);
    }
    else
        m_currentFindIndex = QModelIndex();
}

void DFG_CLASS_NAME(CsvTableView)::onFindPrevious()
{
    QToolTip::showText(QCursor::pos(), tr("Backward find is on TODO-list, sorry"));
}

void DFG_CLASS_NAME(CsvTableView)::setFindText(QString s, const int col)
{
    m_findText = std::move(s);
    m_nFindColumnIndex = col;
    auto pBaseModel = csvModel();
    if (!pBaseModel)
        return;

    CsvModel::HighlightDefinition hld("te0", getFindColumnIndex(), CsvModel::StringMatchDefinition(m_findText, Qt::CaseInsensitive));
    pBaseModel->setHighlighter(std::move(hld));

    const auto currentIndex = this->currentIndex();
    m_currentFindIndex = pBaseModel->index((currentIndex.isValid()) ? currentIndex.row() : 0,
                                                getFindColumnIndex());
}
