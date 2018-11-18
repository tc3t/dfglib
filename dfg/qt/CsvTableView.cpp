#include "../buildConfig.hpp" // To get rid of C4996 "Function call with parameters that may be unsafe" in MSVC.
#include "qtIncludeHelpers.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"
#include "CsvTableViewActions.hpp"
#include "QtApplication.hpp"
#include "widgetHelpers.hpp"
#include "../os.hpp"
#include "../os/TemporaryFileStream.hpp"
#include "PropertyHelper.hpp"
#include "connectHelper.hpp"
#include "CsvTableViewCompleterDelegate.hpp"
#include "../time/timerCpu.hpp"
#include "../cont/valueArray.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QMenu>
#include <QFileDialog>
#include <QUndoStack>
#include <QHeaderView>
#include <QFormLayout>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QCompleter>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QMetaMethod>
#include <QProcess>
#include <QProgressBar>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QThread>
#include <QTimer>
#include <QToolTip>
#include <QUndoView>
DFG_END_INCLUDE_QT_HEADERS

#include <set>
#include "../alg.hpp"
#include "../cont/SortedSequence.hpp"
#include "../math.hpp"
#include "../str/stringLiteralCharToValue.hpp"

using namespace DFG_MODULE_NS(qt);

namespace
{
    class ProgressWidget : public QProgressDialog
    {
    public:
        typedef QProgressDialog BaseClass;
        ProgressWidget(const QString sLabelText, QWidget* pParent)
            : BaseClass(sLabelText, QString(), 0, 0, pParent)
        {
            removeContextHelpButtonFromDialog(this);
            removeCloseButtonFromDialog(this);
            setWindowModality(Qt::WindowModal);
            setCancelButton(nullptr); // Hide cancel button as there is no way to cancel the operation.
        }
    }; // Class ProgressWidget

    enum CsvTableViewPropertyId
    {
        CsvTableViewPropertyId_diffProgPath,
        CsvTableViewPropertyId_initialScrollPosition,
        CsvTableViewPropertyId_minimumVisibleColumnWidth
    };

    DFG_QT_DEFINE_OBJECT_PROPERTY_CLASS(CsvTableView)

    template <CsvTableViewPropertyId ID>
    auto getCsvTableViewProperty(DFG_CLASS_NAME(CsvTableView)* view) -> typename CsvTableViewPropertyDefinition<ID>::PropertyType
    {
        return DFG_MODULE_NS(qt)::getProperty<DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CsvTableView)<ID>>(view);
    }

    // Properties
    DFG_QT_DEFINE_OBJECT_PROPERTY("diffProgPath", CsvTableView, CsvTableViewPropertyId_diffProgPath, QString, PropertyType);
    DFG_QT_DEFINE_OBJECT_PROPERTY("CsvTableView_initialScrollPosition", CsvTableView, CsvTableViewPropertyId_initialScrollPosition, QString, PropertyType);
    DFG_QT_DEFINE_OBJECT_PROPERTY("CsvTableView_minimumVisibleColumnWidth", CsvTableView, CsvTableViewPropertyId_minimumVisibleColumnWidth, int, []() { return 5; });

    template <class T>
    QString floatToQString(const T val)
    {
        return QString::fromLatin1(DFG_MODULE_NS(str)::floatingPointToStr<DFG_ROOT_NS::DFG_CLASS_NAME(StringAscii)>(val).c_str().c_str());
    }

    const int gnDefaultRowHeight = 21; // Default row height seems to be 30, which looks somewhat wasteful so make it smaller.

    class UndoViewWidget : public QDialog
    {
    public:
        typedef QDialog BaseClass;
        UndoViewWidget(DFG_CLASS_NAME(CsvTableView)* pParent)
            : BaseClass(pParent)
        {
            setAttribute(Qt::WA_DeleteOnClose, true);
            removeContextHelpButtonFromDialog(this);
            if (!pParent || !pParent->m_spUndoStack)
                return;
            auto pLayout = new QHBoxLayout(this);
            pLayout->addWidget(new QUndoView(&pParent->m_spUndoStack.get()->item(), this));
        }

        ~UndoViewWidget()
        {
        }

    }; // Class UndoViewWidget

    static void doModalOperation(QWidget* pParent, const QString& sProgressDialogLabel, const QString& sThreadName, std::function<void ()> func)
    {
        QEventLoop eventLoop;

        auto pProgressDialog = new ProgressWidget(sProgressDialogLabel, pParent);
        auto pWorkerThread = new QThread();
        pWorkerThread->setObjectName(sThreadName); // Sets thread name visible to debugger.
        QObject::connect(pWorkerThread, &QThread::started, [&]()
                {
                    func();
                    pWorkerThread->quit();
                });
        // Connect thread finish to trigger event loop quit and closing of progress bar.
        QObject::connect(pWorkerThread, &QThread::finished, &eventLoop, &QEventLoop::quit);
        QObject::connect(pWorkerThread, &QThread::finished, pWorkerThread, &QObject::deleteLater);
        QObject::connect(pWorkerThread, &QObject::destroyed, pProgressDialog, &QObject::deleteLater);

        pWorkerThread->start();

        // Wait a while before showing the progress dialog; don't want to pop it up for tiny files.
        QTimer::singleShot(750, pProgressDialog, SLOT(show()));

        // Keep event loop running while operating.
        eventLoop.exec();
    }

    QString getOpenFileName(QWidget* pParent)
    {
        return QFileDialog::getOpenFileName(pParent,
                                            QApplication::tr("Open file"),
                                            QString()/*dir*/,
                                            QApplication::tr("CSV files (*.csv *.tsv);;All files (*.*)"),
                                            nullptr/*selected filter*/,
                                            0/*options*/);
    }

} // unnamed namespace

DFG_CLASS_NAME(CsvTableView)::DFG_CLASS_NAME(CsvTableView)(QWidget* pParent)
    : BaseClass(pParent)
    , m_matchDef(QString(), Qt::CaseInsensitive, QRegExp::Wildcard)
    , m_nFindColumnIndex(0)
    , m_bUndoEnabled(true)
{
    auto pVertHdr = verticalHeader();
    if (pVertHdr)
        pVertHdr->setDefaultSectionSize(gnDefaultRowHeight); // TODO: make customisable

    // TODO: make customisable.
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    const auto addSeparator = [&]()
        {
            auto pAction = new QAction(this);
            pAction->setSeparator(true);
            addAction(pAction);
        };

    {
        auto pAction = new QAction(tr("Move first row to header"), this);
        //pAction->setShortcut(tr(""));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::moveFirstRowToHeader));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Move header to first row"), this);
        //pAction->setShortcut(tr(""));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::moveHeaderToFirstRow));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Open file..."), this);
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::openFromFile));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Open file with options..."), this);
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::openFromFileWithOptions));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Merge files to current..."), this);
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::mergeFilesToCurrent));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Save"), this);
        pAction->setShortcut(tr("Ctrl+S"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::save));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Save to file..."), this);
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::saveToFile));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Save to file with options..."), this);
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::saveToFileWithOptions));
        addAction(pAction);
    }

    // -------------------------------------------------
    addSeparator();

    {
        auto pAction = new QAction(tr("Clear selected cell(s)"), this);
        pAction->setShortcut(tr("Del"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::clearSelected));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Insert row here"), this);
        pAction->setShortcut(tr("Ins"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::insertRowHere));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Insert row after current"), this);
        pAction->setShortcut(tr("Shift+Ins"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::insertRowAfterCurrent));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Insert column"), this);
        pAction->setShortcut(tr("Ctrl+Alt+Ins"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::insertColumn));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Insert column after current"), this);
        pAction->setShortcut(tr("Ctrl+Alt+Shift+Ins"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::insertColumnAfterCurrent));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Delete selected row(s)"), this);
        pAction->setShortcut(tr("Shift+Del"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::deleteSelectedRow));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Delete current column"), this);
        pAction->setShortcut(tr("Ctrl+Del"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, SIGNAL(triggered()), this, SLOT(deleteCurrentColumn())));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Resize table"), this);
        //pAction->setShortcut(tr(""));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::resizeTable));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Generate content..."), this);
        //pAction->setShortcut(tr(""));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::generateContent));
        addAction(pAction);
    }

    // -------------------------------------------------
    addSeparator();

    {
        auto pAction = new QAction(tr("Invert selection"), this);
        pAction->setShortcut(tr("Ctrl+I"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::invertSelection));
        addAction(pAction);
    }

    // Find and filter actions
    {
        {
            auto pAction = new QAction(tr("Find"), this);
            pAction->setShortcut(tr("Ctrl+F"));
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::onFindRequested));
            addAction(pAction);
        }
        {
            auto pAction = new QAction(tr("Find next"), this);
            pAction->setShortcut(tr("F3"));
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::onFindNext));
            addAction(pAction);
        }
        {
            auto pAction = new QAction(tr("Find previous"), this);
            pAction->setShortcut(tr("Shift+F3"));
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::onFindPrevious));
            addAction(pAction);
        }

        // Filter
        {
            auto pAction = new QAction(tr("Filter"), this);
            pAction->setShortcut(tr("Alt+F"));
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::onFilterRequested));
            addAction(pAction);
        }
    }

    /* Not Implemented
    {
        auto pAction = new QAction(tr("Move row up"), this);
        pAction->setShortcut(tr("Alt+Up"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::moveRowUp));
        addAction(pAction);
    }

    {
    auto pAction = new QAction(tr("Move row down"), this);
    pAction->setShortcut(tr("Alt+Down"));
    DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::moveRowDown));
    addAction(pAction);
    }
    */

    // -------------------------------------------------
    addSeparator();

    {
        auto pAction = new QAction(tr("Cut"), this);
        pAction->setShortcut(tr("Ctrl+X"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::cut));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Copy"), this);
        pAction->setShortcut(tr("Ctrl+C"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::copy));
        addAction(pAction);
    }

    {
        auto pAction = new QAction(tr("Paste"), this);
        pAction->setShortcut(tr("Ctrl+V"));
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::paste));
        addAction(pAction);
    }

    privAddUndoRedoActions();

    // -------------------------------------------------
    addSeparator();

    // Add 'sortable columns'-action
    {
        auto pAction = new QAction(tr("Sortable columns"), this);
        pAction->setCheckable(true);
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::toggled, this, &ThisClass::setSortingEnabled));
        addAction(pAction);
    }

    // Add 'Case sensitive sorting'-action
    {
        auto pAction = new QAction(tr("Case sensitive sorting"), this);
        pAction->setCheckable(true);
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::toggled, [&](const bool bCaseSensitive)
        {
            auto pProxy = qobject_cast<QSortFilterProxyModel*>(getProxyModelPtr());
            if (pProxy)
                pProxy->setSortCaseSensitivity((bCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive);
            else
                QToolTip::showText(QCursor::pos(), tr("Unable to toggle sort case senstitivity: no suitable proxy model found"));
        }));
        addAction(pAction);
    }

    // Add 'reset sorting'-action
    {
        auto pAction = new QAction(tr("Reset sorting"), this);
        DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, [&]()
        {
            auto pProxy = this->getProxyModelPtr();
            if (pProxy)
                pProxy->sort(-1);
            else
                QToolTip::showText(QCursor::pos(), tr("Unable to reset sorting: no proxy model found"));
        }));
        addAction(pAction);
    }

    // -------------------------------------------------
    addSeparator();

    {
        auto pAction = new QAction(tr("Resize"), this);
        m_spResizeColumnsMenu = createResizeColumnsMenu();
        pAction->setMenu(m_spResizeColumnsMenu.get());
        addAction(pAction);
    }

    // -------------------------------------------------
    addSeparator();

    {
        // Add diff-action
        {
            auto pAction = new QAction(tr("Diff with unmodified"), this);
            pAction->setShortcut(tr("Alt+D"));
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &ThisClass::diffWithUnmodified));
            addAction(pAction);
        }

        // Add row mode -control
        {
            auto pAction = new QAction(tr("Row mode"), this);
            pAction->setToolTip(tr("Selections by row instead of by cell (experimental)"));
            pAction->setCheckable(true);
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::toggled, this, &ThisClass::setRowMode));
            addAction(pAction);
        }
    }
}

DFG_CLASS_NAME(CsvTableView)::~DFG_CLASS_NAME(CsvTableView)()
{
    for (auto iter = m_tempFilePathsToRemoveOnExit.cbegin(), iterEnd = m_tempFilePathsToRemoveOnExit.cend(); iter != iterEnd; ++iter)
    {
        QFile::remove(*iter);
    }
}

std::unique_ptr<QMenu> DFG_CLASS_NAME(CsvTableView)::createResizeColumnsMenu()
{
    std::unique_ptr<QMenu> spMenu(new QMenu);

    // Note: not using addSection() (i.e. a separator with text) as they are not shown in Windows in default style;
    // can be made visible e.g. by using "fusion" style (for more information see setStyle(), QStyleFactory::create)

    // Column actions
    {
        auto pActViewEvenly = spMenu->addAction(tr("Col: Resize to view evenly"));
        DFG_QT_VERIFY_CONNECT(connect(pActViewEvenly, &QAction::triggered, this, &ThisClass::onColumnResizeAction_toViewEvenly));
        DFG_ASSERT_CORRECTNESS(pActViewEvenly->parent() == spMenu.get()); // Expecting action to have the menu as parent for automatic destruction.

        auto pActViewContent = spMenu->addAction(tr("Col: Resize to view content aware"));
        DFG_QT_VERIFY_CONNECT(connect(pActViewContent, &QAction::triggered, this, &ThisClass::onColumnResizeAction_toViewContentAware));

        auto pActContent = spMenu->addAction(tr("Col: Resize all to content"));
        DFG_QT_VERIFY_CONNECT(connect(pActContent, &QAction::triggered, this, &ThisClass::onColumnResizeAction_content));

        auto pActFixedSize = spMenu->addAction(tr("Col: Set fixed size..."));
        DFG_QT_VERIFY_CONNECT(connect(pActFixedSize, &QAction::triggered, this, &ThisClass::onColumnResizeAction_fixedSize));
    }

    spMenu->addSeparator();

    // Row actions
    {
        auto pActContent = spMenu->addAction(tr("Row: Resize all to content"));
        DFG_QT_VERIFY_CONNECT(connect(pActContent, &QAction::triggered, this, &ThisClass::onRowResizeAction_content));

        auto pActFixedSize = spMenu->addAction(tr("Row: Set fixed size..."));
        DFG_QT_VERIFY_CONNECT(connect(pActFixedSize, &QAction::triggered, this, &ThisClass::onRowResizeAction_fixedSize));
    }

    return spMenu;
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

void DFG_CLASS_NAME(CsvTableView)::showUndoWindow()
{
    if (!m_spUndoStack)
        return;
    auto pUndoWidget = new UndoViewWidget(this);
    pUndoWidget->show();
}

namespace
{
    const char gszMenuText_enableUndo[] = "Enable undo";
    const char gszMenuText_clearUndoBuffer[] = "&Clear undo buffer";
    const char gszMenuText_showUndoWindow[] = "Show undo buffer";
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

        // Undo menu
        {
            auto pAction = new QAction(tr("Undo details"), this);

            auto pMenu = new QMenu();
            // Schedule destruction of menu with the parent action.
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QObject::destroyed, pMenu, [=]() { delete pMenu; }));

            // Add 'enable undo' -action
            {
                auto pActionUndoEnableDisable = new QAction(tr(gszMenuText_enableUndo), this);
                pActionUndoEnableDisable->setCheckable(true);
                pActionUndoEnableDisable->setChecked(m_bUndoEnabled);
                DFG_QT_VERIFY_CONNECT(connect(pActionUndoEnableDisable, &QAction::toggled, this, &ThisClass::setUndoEnabled));
                pMenu->addAction(pActionUndoEnableDisable);
            }

            // Add Clear undo buffer -action
            {
                auto pActionClearUndoBuffer = new QAction(tr(gszMenuText_clearUndoBuffer), this);
                connect(pActionClearUndoBuffer, &QAction::triggered, this, &ThisClass::clearUndoStack);
                pMenu->addAction(pActionClearUndoBuffer);
            }

            // Add show undo window -action
            {
                auto pActionUndoWindow = new QAction(tr(gszMenuText_showUndoWindow), this);
                connect(pActionUndoWindow, &QAction::triggered, this, &ThisClass::showUndoWindow);
                pMenu->addAction(pActionUndoWindow);
            }

            pAction->setMenu(pMenu); // Does not transfer ownership.
            addAction(pAction);
        }
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
    auto pCsvModel = csvModel();
    if (m_spUndoStack && pCsvModel)
        pCsvModel->setUndoStack(&m_spUndoStack->item());
    if (pCsvModel)
        DFG_QT_VERIFY_CONNECT(connect(pCsvModel, &CsvModel::sigOnNewSourceOpened, this, &ThisClass::onNewSourceOpened));
    DFG_QT_VERIFY_CONNECT(connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &ThisClass::onSelectionChanged));
}

namespace
{
    template <class CsvModel_T, class ProxyModel_T, class Model_T>
    CsvModel_T* csvModelImpl(Model_T* pModel)
    {
        auto pCsvModel = qobject_cast<CsvModel_T*>(pModel);
        if (pCsvModel)
            return pCsvModel;
        auto pProxyModel = qobject_cast<ProxyModel_T*>(pModel);
        return (pProxyModel) ? qobject_cast<CsvModel_T*>(pProxyModel->sourceModel()) : nullptr;
    }
}

auto DFG_CLASS_NAME(CsvTableView)::csvModel() -> CsvModel*
{
    return csvModelImpl<CsvModel, QAbstractProxyModel>(model());
}

auto DFG_CLASS_NAME(CsvTableView)::csvModel() const -> const CsvModel*
{
   return csvModelImpl<const CsvModel, const QAbstractProxyModel>(model());
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

QModelIndexList DFG_CLASS_NAME(CsvTableView)::selectedIndexes() const
{
    DFG_ASSERT_WITH_MSG(false, "Avoid using selectedIndexes() as it's behaviour is unclear when using proxies: selected indexes of proxy or underlying model?");
    return BaseClass::selectedIndexes();
}

QModelIndexList DFG_CLASS_NAME(CsvTableView)::getSelectedItemIndexes_dataModel() const
{
    auto pSelectionModel = selectionModel();
    if (!pSelectionModel)
        return QModelIndexList();
    auto selected = pSelectionModel->selectedIndexes();
    if (selected.isEmpty())
        return QModelIndexList();
    auto pProxy = getProxyModelPtr();
    // Map indexes to underlying model. For unknown reason the indexes returned by selection model
    // seem to be sometimes from proxy and sometimes from underlying.
    if (pProxy && selected.front().model() == pProxy)
        std::transform(selected.begin(), selected.end(), selected.begin(), [=](const QModelIndex& index) { return pProxy->mapToSource(index); });
    return selected;
}

QModelIndexList DFG_CLASS_NAME(CsvTableView)::getSelectedItemIndexes_viewModel() const
{
    auto indexes = getSelectedItemIndexes_dataModel();
    auto pProxy = getProxyModelPtr();
    if (pProxy)
        std::transform(indexes.begin(), indexes.end(), indexes.begin(), [=](const QModelIndex& index) { return pProxy->mapFromSource(index); });
    return indexes;
}

std::vector<int> DFG_CLASS_NAME(CsvTableView)::getRowsOfSelectedItems(const QAbstractProxyModel* pProxy, const bool bSort) const
{
    QModelIndexList listSelected = (!pProxy) ? getSelectedItemIndexes_viewModel() : getSelectedItemIndexes_dataModel();

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
    const QModelIndexList listSelected = getSelectedItemIndexes_dataModel();
    if (listSelected.empty())
        return QModelIndex();
    if (pProxy)
        return pProxy->mapToSource(listSelected[0]);
    else
        return listSelected[0];

}

void DFG_CLASS_NAME(CsvTableView)::invertSelection()
{
    BaseClass::invertSelection();
}

bool DFG_CLASS_NAME(CsvTableView)::isRowMode() const
{
    return (selectionBehavior() == QAbstractItemView::SelectRows);
}

void DFG_CLASS_NAME(CsvTableView)::setRowMode(const bool b)
{
    setSelectionBehavior((b) ? QAbstractItemView::SelectRows : QAbstractItemView::SelectItems);
}

void DFG_CLASS_NAME(CsvTableView)::setUndoEnabled(const bool bEnable)
{
    auto pCsvModel = csvModel();
    clearUndoStack();
    m_bUndoEnabled = bEnable;
    if (!pCsvModel)
        return;
    if (bEnable)
    {
        if (m_spUndoStack)
            pCsvModel->setUndoStack(&m_spUndoStack->item());
    }
    else
    {
        pCsvModel->setUndoStack(nullptr);
    }
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

    bool bSuccess = false;
    doModalOperation(this, tr("Saving to file\n%1").arg(path), "CsvTableViewFileWriter", [&]()
        {
            // TODO: allow user to cancel saving (e.g. if it takes too long)
            bSuccess = pModel->saveToFile(path, formatDef);
        });

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
    enum DialogType { DialogTypeSave, DialogTypeLoad };

    typedef DFG_CLASS_NAME(CsvItemModel)::LoadOptions LoadOptions;
    typedef DFG_CLASS_NAME(CsvItemModel)::SaveOptions SaveOptions;

    CsvFormatDefinitionDialog(const DialogType dialogType)
        : m_dialogType(dialogType)
    {
        using namespace DFG_MODULE_NS(io);
        removeContextHelpButtonFromDialog(this);
        auto spLayout = std::unique_ptr<QFormLayout>(new QFormLayout);
        m_spSeparatorEdit.reset(new QComboBox(this));
        m_spEnclosingEdit.reset(new QComboBox(this));
        m_spEolEdit.reset(new QComboBox(this));
        if (isSaveDialog())
        {
            m_spEncodingEdit.reset(new QComboBox(this));
            m_spSaveHeader.reset(new QCheckBox(this));
            m_spWriteBOM.reset(new QCheckBox(this));
        }
        else
        {
            m_spCompleterColumns.reset(new QLineEdit(this));
        }

        m_spSeparatorEdit->addItems(QStringList() << "," << "\\t" << ";");
        m_spSeparatorEdit->setEditable(true);
        m_spEnclosingEdit->addItems(QStringList() << "\"" << "");
        m_spEnclosingEdit->setEditable(true);
        m_spEolEdit->addItems(QStringList() << "\\n" << "\\r" << "\\r\\n");
        m_spEolEdit->setEditable(false);
        if (isSaveDialog())
        {
            m_spEncodingEdit->addItems(QStringList() << encodingToStrId(encodingUTF8) << encodingToStrId(encodingLatin1));
            m_spEncodingEdit->setEditable(false);
            m_spSaveHeader->setChecked(true);
            m_spWriteBOM->setChecked(true);
        }

        spLayout->addRow(tr("Separator char"), m_spSeparatorEdit.get());
        spLayout->addRow(tr("Enclosing char"), m_spEnclosingEdit.get());
        spLayout->addRow(tr("End-of-line"), m_spEolEdit.get());
        if (isSaveDialog())
        {
            spLayout->addRow(tr("Encoding"), m_spEncodingEdit.get());
            spLayout->addRow(tr("Save header"), m_spSaveHeader.get());
            spLayout->addRow(tr("Write BOM"), m_spWriteBOM.get());
        }
        else
        {
            spLayout->addRow(tr("Completer columns"), m_spCompleterColumns.get());
            m_spCompleterColumns->setText("*"); // TODO: might want some logics here; not reasonable to have this enabled by default for huge files.
            m_spCompleterColumns->setToolTip(tr("Column indexes (starting from 0) where completion is available, use * to enable on all."));
        }
        //spLayout->addRow(new QLabel(tr("Note: "), this));

        auto& rButtonBox = *(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel));

        connect(&rButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(&rButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

        spLayout->addRow(QString(), &rButtonBox);
        setLayout(spLayout.release());
    }

    bool isSaveDialog() const
    {
        return m_dialogType == DialogTypeSave;
    }

    bool isLoadDialog() const
    {
        return !isSaveDialog();
    }

    void accept() override
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);
        if (isSaveDialog() && (!m_spSeparatorEdit || !m_spEnclosingEdit || !m_spEolEdit || !m_spSaveHeader || !m_spWriteBOM || !m_spEncodingEdit))
        {
            QMessageBox::information(this, tr("CSV saving"), tr("Internal error occurred; saving failed."));
            return;
        }
        if (isLoadDialog() && (!m_spSeparatorEdit || !m_spEnclosingEdit || !m_spEolEdit || !m_spCompleterColumns))
        {
            QMessageBox::information(this, tr("CSV loading"), tr("Internal error occurred; loading failed."));
            return;
        }
        auto sSep = m_spSeparatorEdit->currentText().trimmed();
        auto sEnc = m_spEnclosingEdit->currentText().trimmed();
        auto sEol = m_spEolEdit->currentText().trimmed();

        DFG_MODULE_NS(io)::EndOfLineType eolType = DFG_MODULE_NS(io)::EndOfLineTypeNative;

        const auto sep = DFG_MODULE_NS(str)::stringLiteralCharToValue<int32>(sSep.toStdWString());
        const auto enc = DFG_MODULE_NS(str)::stringLiteralCharToValue<int32>(sEnc.toStdWString());

        if (sEol == "\\n")
            eolType = DFG_MODULE_NS(io)::EndOfLineTypeN;
        else if (sEol == "\\r")
            eolType = DFG_MODULE_NS(io)::EndOfLineTypeR;
        else if (sEol == "\\r\\n")
            eolType = DFG_MODULE_NS(io)::EndOfLineTypeRN;

        // TODO: check for identical values (e.g. require that sep != enc)
        if (!sep.first || (!sEnc.isEmpty() && !enc.first) || eolType == DFG_MODULE_NS(io)::EndOfLineTypeNative)
        {
            // TODO: more informative message for the user.
            QMessageBox::information(this, tr("CSV saving"), tr("Chosen settings can't be used. Please revise the selections."));
            return;
        }

        if (isLoadDialog())
        {
            m_loadOptions.m_cEnc = (!sEnc.isEmpty()) ? enc.second : ::DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone;
            m_loadOptions.m_cSep = sep.second;
            m_loadOptions.m_eolType = eolType;
            m_loadOptions.setProperty("completerColumns", m_spCompleterColumns->text().toStdString());
        }
        else // case: save dialog
        {
            m_saveOptions.m_cEnc = enc.second;
            m_saveOptions.m_cSep = sep.second;
            m_saveOptions.m_eolType = eolType;
            m_saveOptions.headerWriting(m_spSaveHeader->isChecked());
            m_saveOptions.bomWriting(m_spWriteBOM->isChecked());
            m_saveOptions.textEncoding(strIdToEncoding(m_spEncodingEdit->currentText().toLatin1().data()));
        }

        BaseClass::accept();
    }

    LoadOptions getLoadOptions() const
    {
        return m_loadOptions;
    }

    SaveOptions getSaveOptions() const
    {
        return m_saveOptions;
    }


    DialogType m_dialogType;
    LoadOptions m_loadOptions;
    SaveOptions m_saveOptions;
    std::unique_ptr<QComboBox> m_spSeparatorEdit;
    std::unique_ptr<QComboBox> m_spEnclosingEdit;
    std::unique_ptr<QComboBox> m_spEolEdit;
    std::unique_ptr<QComboBox> m_spEncodingEdit;
    std::unique_ptr<QCheckBox> m_spSaveHeader;
    std::unique_ptr<QCheckBox> m_spWriteBOM;
    // Load-only properties
    std::unique_ptr<QLineEdit> m_spCompleterColumns;
}; // Class CsvFormatDefinitionDialog

bool DFG_CLASS_NAME(CsvTableView)::saveToFileWithOptions()
{
    CsvFormatDefinitionDialog dlg(CsvFormatDefinitionDialog::DialogTypeSave);
    if (dlg.exec() != QDialog::Accepted)
        return false;
    return saveToFileImpl(dlg.getSaveOptions());
}

bool DFG_CLASS_NAME(CsvTableView)::openFile(const QString& sPath)
{
    return openFile(sPath, CsvItemModel::LoadOptions());
}

bool DFG_CLASS_NAME(CsvTableView)::openFile(const QString& sPath, const DFG_ROOT_NS::DFG_CLASS_NAME(CsvFormatDefinition)& formatDef)
{
    if (sPath.isEmpty())
        return false;
    auto pModel = csvModel();
    if (!pModel)
        return false;

    // Reset models to prevent event loop from updating stuff while model is being read in another thread
    auto pViewModel = model();
    auto pProxyModel = getProxyModelPtr();
    if (pProxyModel && pProxyModel->sourceModel() == pModel)
        pProxyModel->setSourceModel(nullptr);
    setModel(nullptr);

    bool bSuccess = false;
    doModalOperation(this, tr("Reading file\n%1").arg(sPath), "CsvTableViewFileLoader", [&]()
        {
            bSuccess = pModel->openFile(sPath, formatDef);
        });

    if (pProxyModel)
        pProxyModel->setSourceModel(pModel);
    setModel(pViewModel);

    const auto scrollPos = getCsvTableViewProperty<CsvTableViewPropertyId_initialScrollPosition>(this);
    if (scrollPos == "bottom")
        scrollToBottom();

    onColumnResizeAction_toViewEvenly();

    if (bSuccess)
        onNewSourceOpened();

    return bSuccess;
}

bool DFG_CLASS_NAME(CsvTableView)::openFromFile()
{
    return openFile(getOpenFileName(this));
}

bool DFG_CLASS_NAME(CsvTableView)::openFromFileWithOptions()
{
    const auto sPath = getOpenFileName(this);
    if (sPath.isEmpty())
        return false;
    CsvFormatDefinitionDialog dlg(CsvFormatDefinitionDialog::DialogTypeLoad);
    if (dlg.exec() != QDialog::Accepted)
        return false;
    return openFile(sPath, dlg.getLoadOptions());
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
    if (m_spUndoStack && m_bUndoEnabled)
        pushToUndoStack<T>(std::forward<Param0_T>(p0));
    else
        DFG_CLASS_NAME(UndoCommand)::directRedo<T>(std::forward<Param0_T>(p0));

    return true;
}

template <class T, class Param0_T, class Param1_T>
bool DFG_CLASS_NAME(CsvTableView)::executeAction(Param0_T&& p0, Param1_T&& p1)
{
    if (m_spUndoStack && m_bUndoEnabled)
        pushToUndoStack<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));
    else
        DFG_CLASS_NAME(UndoCommand)::directRedo<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));

    return true;
}

template <class T, class Param0_T, class Param1_T, class Param2_T>
bool DFG_CLASS_NAME(CsvTableView)::executeAction(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2)
{
    if (m_spUndoStack && m_bUndoEnabled)
        pushToUndoStack<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2));
    else
        DFG_CLASS_NAME(UndoCommand)::directRedo<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2));

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
        auto pComboBoxDelegate = qobject_cast<QComboBox*>(editor);
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
        auto pComboBoxDelegate = qobject_cast<QComboBox*>(editor);
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
        const auto& selected = view.getSelectedItemIndexes_dataModel();
        size_t nCounter = 0;
        rModel.batchEditNoUndo([&](DFG_CLASS_NAME(CsvItemModel)::DataTable& table)
        {
            for (auto iter = selected.begin(); iter != selected.end(); ++iter, ++nCounter)
            {
                generator(table, iter->row(), iter->column(), nCounter);
            }
        });
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
    return qobject_cast<QAbstractProxyModel*>(model());
}

const QAbstractProxyModel* DFG_CLASS_NAME(CsvTableView)::getProxyModelPtr() const
{
    return qobject_cast<const QAbstractProxyModel*>(model());
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
        m_tempFilePathsToRemoveOnExit.push_back(QString::fromUtf8(toCharPtr_raw(strmTemp.pathU8())));
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

void DFG_CLASS_NAME(CsvTableView)::onFilterRequested()
{
    const QMetaMethod findActivatedSignal = QMetaMethod::fromSignal(&ThisClass::sigFilterActivated);
    if (isSignalConnected(findActivatedSignal))
        Q_EMIT sigFilterActivated();
    else
        QToolTip::showText(QCursor::pos(), tr("Sorry, standalone filter is not implemented."));
}

void DFG_CLASS_NAME(CsvTableView)::onFindRequested()
{
    const QMetaMethod findActivatedSignal = QMetaMethod::fromSignal(&ThisClass::sigFindActivated);
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
        {
            m_matchDef.m_matchString = std::move(s);
            setFindText(m_matchDef, m_nFindColumnIndex);
        }
    }
}

void DFG_CLASS_NAME(CsvTableView)::onFind(const bool forward)
{
    if (m_matchDef.m_matchString.isEmpty())
        return;

    auto pBaseModel = csvModel();
    if (!pBaseModel)
        return;

    // This to prevent setting invalid find column from resetting view (e.g. scroll to top).
    if (getFindColumnIndex() >= pBaseModel->getColumnCount())
    {
        QToolTip::showText(QCursor::pos(), tr("Find column index is invalid"));
        return;
    }

    const auto findSeed = [&]() -> QModelIndex
        {
            if (m_latestFoundIndex.isValid())
                return m_latestFoundIndex;
            else if (getFindColumnIndex() >= 0)
            {
                const auto current = currentIndex();
                if (current.isValid())
                    return pBaseModel->index(current.row(), getFindColumnIndex());
                else
                    return pBaseModel->index(0, getFindColumnIndex()); // Might need to improve this e.g. to return cell from top left corner of visible rect.
            }
            else
                return currentIndex();
        }();

    // TODO: this doesn't work correctly with proxy filtering: finds cells also from filter-hidden cells.
    const auto found = pBaseModel->findNextHighlighterMatch(findSeed, (forward) ? CsvModel::FindDirectionForward : CsvModel::FindDirectionBackward);

    if (found.isValid())
    {
        m_latestFoundIndex = found;
        scrollTo(mapToViewModel(m_latestFoundIndex));
        setCurrentIndex(mapToViewModel(m_latestFoundIndex));
    }
    else
        forgetLatestFindPosition();
}

void DFG_CLASS_NAME(CsvTableView)::onFindNext()
{
    onFind(true);
}

void DFG_CLASS_NAME(CsvTableView)::onFindPrevious()
{
    onFind(false);
}

void DFG_CLASS_NAME(CsvTableView)::setFindText(const StringMatchDef matchDef, const int nCol)
{
    m_matchDef = matchDef;
    m_nFindColumnIndex = nCol;
    auto pBaseModel = csvModel();
    if (!pBaseModel)
        return;

    CsvModel::HighlightDefinition hld("te0", getFindColumnIndex(), m_matchDef);
    pBaseModel->setHighlighter(std::move(hld));

    forgetLatestFindPosition();
}

template <class Func_T>
void DFG_CLASS_NAME(CsvTableView)::forEachCompleterEnabledColumnIndex(Func_T func)
{
    auto pModel = csvModel();
    if (pModel)
    {
        const auto nColCount = pModel->getColumnCount();
        for (int i = 0; i < nColCount; ++i)
        {
            auto pColInfo = pModel->getColInfo(i);
            if (pColInfo->hasCompleter())
                func(i, pColInfo);
        }
    }
}

void DFG_CLASS_NAME(CsvTableView)::onNewSourceOpened()
{
    forEachCompleterEnabledColumnIndex([&](const int nCol, CsvModel::ColInfo* pColInfo)
    {
        typedef DFG_CLASS_NAME(CsvTableViewCompleterDelegate) DelegateClass;
        if (pColInfo)
        {
            auto existingColumnDelegate = qobject_cast<DelegateClass*>(this->itemDelegateForColumn(nCol));
            // Note: delegates live in 'this', but actual completers live in model. These custom delegates will be
            // used on all columns for which completion has been enabled on any of the models opened, but actual completer
            // objects will be available only from current model; in other columns the weak reference to completer object
            // will be null and the delegate fallbacks to behaviour without completer.
            if (!existingColumnDelegate)
            {
                auto* pDelegate = new DFG_CLASS_NAME(CsvTableViewCompleterDelegate)(this, pColInfo->m_spCompleter.get());
                setItemDelegateForColumn(nCol, pDelegate); // Does not transfer ownership, delegate is parent owned by 'this'.
            }
            else // If there already is an delegate, update the completer.
                existingColumnDelegate->m_spCompleter = pColInfo->m_spCompleter.get();
        }
    });

    onSelectionContentChanged();
}

namespace
{
class WidgetPair : public QWidget
{
public:
    typedef QWidget BaseClass;

protected:
    WidgetPair(QWidget* pParent);

public:
    ~WidgetPair();

    static std::unique_ptr<WidgetPair> createHorizontalPair(QWidget* pParent,
                                                            QWidget* pFirst,
                                                            QWidget* pSecond,
                                                            const bool reparentWidgets = true);

    static std::unique_ptr<WidgetPair>createHorizontalLabelLineEditPair(QWidget* pParent,
                                                                         QString sLabel,
                                                                         const QString& sLineEditPlaceholder = QString());

    QHBoxLayout* m_pLayout;
    QWidget* m_pFirst;
    QWidget* m_pSecond;
}; // class WidgetPair

WidgetPair::WidgetPair(QWidget* pParent) :
    BaseClass(pParent)
{
}

WidgetPair::~WidgetPair()
{

}

std::unique_ptr<WidgetPair> WidgetPair::createHorizontalPair(QWidget* pParent,
                                                        QWidget* pFirst,
                                                        QWidget* pSecond,
                                                        const bool reparentWidgets)
{
    std::unique_ptr<WidgetPair> spWp(new WidgetPair(pParent));
    spWp->m_pLayout = new QHBoxLayout(spWp.get());
    if (reparentWidgets)
    {
        if (pFirst)
            pFirst->setParent(spWp.get());
        if (pSecond)
            pSecond->setParent(spWp.get());

    }
    spWp->m_pFirst = pFirst;
    spWp->m_pSecond = pSecond;

    spWp->m_pLayout->addWidget(spWp->m_pFirst);
    spWp->m_pLayout->addWidget(spWp->m_pSecond);
    return spWp;
}

std::unique_ptr<WidgetPair> WidgetPair::createHorizontalLabelLineEditPair(QWidget* pParent,
                                                                     QString sLabel,
                                                                     const QString& sLineEditPlaceholder)
{
    auto pLabel = new QLabel(sLabel);
    auto pLineEdit = new QLineEdit(sLineEditPlaceholder);
    return createHorizontalPair(pParent, pLabel, pLineEdit);
}

} // unnamed namespace

DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)::DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)(QWidget *pParent) :
    BaseClass(pParent)
{
    auto layout = new QGridLayout(this);
    layout->setMargin(0);

    int column = 0;

    // Title-Label
    layout->addWidget(new QLabel(tr("Selection"), this), 0, column++);

    // Value display
    m_spValueDisplay.reset(new QLineEdit(this));
    m_spValueDisplay->setReadOnly(true);
    layout->addWidget(m_spValueDisplay.get(), 0, column++);

    // Progress bar
    m_spProgressBar.reset(new QProgressBar(this));
    m_spProgressBar->setMaximumWidth(75);
    layout->addWidget(m_spProgressBar.get(), 0, column++);

    // Stop button
    m_spStopButton.reset(new QPushButton(tr("Stop"), this));
    m_spStopButton->setEnabled(false); // To be enabled when evaluation is ongoing.
    m_spStopButton->setCheckable(true);
    layout->addWidget(m_spStopButton.get(), 0, column++);

    // 'Maximum time'-control
    auto maxTimeControl = WidgetPair::createHorizontalLabelLineEditPair(this, tr("Limit (s)"));
    maxTimeControl->setMaximumWidth(100);
    if (maxTimeControl->m_pFirst)
        maxTimeControl->m_pFirst->setMaximumWidth(50);
    if (maxTimeControl->m_pSecond)
    {
        m_spTimeLimitDisplay = qobject_cast<QLineEdit*>(maxTimeControl->m_pSecond);
        if (m_spTimeLimitDisplay)
        {
            m_spTimeLimitDisplay->setText("1");
            m_spTimeLimitDisplay->setMaximumWidth(50);
        }
    }
    layout->addWidget(maxTimeControl.release(), 0, column++);

    DFG_QT_VERIFY_CONNECT(connect(this, &ThisClass::sigEvaluationStartingHandleRequest, this, &ThisClass::onEvaluationStarting_myThread));
    DFG_QT_VERIFY_CONNECT(connect(this, &ThisClass::sigEvaluationEndedHandleRequest, this, &ThisClass::onEvaluationEnded_myThread));
    DFG_QT_VERIFY_CONNECT(connect(this, &ThisClass::sigSetValueDisplayString, this, &ThisClass::setValueDisplayString_myThread));
}

DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)::~DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)()
{

}

void DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)::setValueDisplayString(const QString& s)
{
    Q_EMIT sigSetValueDisplayString(s);
}

void DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)::setValueDisplayString_myThread(const QString& s)
{
    if (m_spValueDisplay)
        m_spValueDisplay->setText(s);
}

void DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)::onEvaluationStarting(const bool bEnabled)
{
    Q_EMIT sigEvaluationStartingHandleRequest(bEnabled);
}

void DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)::onEvaluationStarting_myThread(const bool bEnabled)
{
    if (bEnabled)
    {
        setValueDisplayString(tr("Working..."));
        if (m_spProgressBar)
        {
            m_spProgressBar->setVisible(true);

            // Sets text to be shown inside the progress bar, the actual text-align value seems to have no effect.
            m_spProgressBar->setStyleSheet("QProgressBar::chunk { text-align: left; }");
            m_spProgressBar->setRange(0, 0); // Activates generic 'something is happening' indicator.
            m_spProgressBar->setValue(-1);
        }
        if (m_spStopButton)
        {
            m_spStopButton->setEnabled(true);
            m_spStopButton->setChecked(false);
        }
    }
    else
    {
        setValueDisplayString(tr("Disabled"));
        if (m_spProgressBar)
            m_spProgressBar->setVisible(false);
    }
}

void DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)::onEvaluationEnded(const double timeInSeconds, const DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus completionStatus)
{
    Q_EMIT sigEvaluationEndedHandleRequest(timeInSeconds, static_cast<int>(completionStatus));
}

void DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)::onEvaluationEnded_myThread(const double timeInSeconds, const int completionStatus)
{
    if (m_spProgressBar)
    {
        m_spProgressBar->setRange(0, 1); // Deactivates generic 'something is happening' indicator.
        m_spProgressBar->setValue(m_spProgressBar->maximum());
        m_spProgressBar->setFormat(QString("%1 s").arg(timeInSeconds));
        if (completionStatus != static_cast<int>(DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus_completed))
        {
            // When work gets interrupted, set color to red.
            m_spProgressBar->setStyleSheet("QProgressBar::chunk { background-color: #FF0000; text-align: left; }");
        }
    }
    if (m_spStopButton)
        m_spStopButton->setEnabled(false);
}

double DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)::getMaxTimeInSeconds() const
{
    bool bOk = false;
    double val = 0;
    if (m_spTimeLimitDisplay)
        val = m_spTimeLimitDisplay->text().toDouble(&bOk);
    return (bOk) ? val : -1.0;
}

bool DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)::isStopRequested() const
{
    return (m_spStopButton && m_spStopButton->isChecked());
}

void DFG_CLASS_NAME(CsvTableView)::onSelectionContentChanged()
{
    onSelectionChanged(QItemSelection(), QItemSelection());
}

void DFG_CLASS_NAME(CsvTableView)::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_EMIT sigSelectionChanged(selected, deselected);

    const auto sm = selectionModel();
    const auto selection = (sm) ? sm->selection() : QItemSelection();

    QEventLoop eventLoop;
    QThread workerThread;
    workerThread.setObjectName("selectionAnalyzer"); // Sets thread name visible to debugger.
    connect(&workerThread, &QThread::started, [&]()
            {
                // TODO: add canRunInParallel-flag to analyzers, split analyzers to groups based on that and run the other
                //       group in parallel.
                for (auto iter = m_selectionAnalyzers.cbegin(); iter != m_selectionAnalyzers.cend(); ++iter)
                {
                    if (!*iter)
                        continue;
                    (*iter)->analyze(this, selection);
                }
                workerThread.quit();
            });
    connect(&workerThread, &QThread::finished, &eventLoop, &QEventLoop::quit);
    workerThread.start();
    eventLoop.exec();
}

void DFG_CLASS_NAME(CsvTableView)::addSelectionAnalyzer(std::shared_ptr<DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)> spAnalyzer)
{
    if (!spAnalyzer)
        return;
    m_selectionAnalyzers.push_back(std::move(spAnalyzer));
}

QModelIndex DFG_CLASS_NAME(CsvTableView)::mapToViewModel(const QModelIndex& index) const
{
    const auto pIndexModel = index.model();
    if (pIndexModel == model())
        return index;
    else if (pIndexModel == csvModel() && getProxyModelPtr())
        return getProxyModelPtr()->mapFromSource(index);
    else
        return QModelIndex();
}

QModelIndex DFG_CLASS_NAME(CsvTableView)::mapToDataModel(const QModelIndex& index) const
{
    const auto pIndexModel = index.model();
    if (pIndexModel == csvModel())
        return index;
    else if (pIndexModel == model() && getProxyModelPtr())
        return getProxyModelPtr()->mapToSource(index);
    else
        return QModelIndex();
}

void DFG_CLASS_NAME(CsvTableView)::forgetLatestFindPosition()
{
    m_latestFoundIndex = QModelIndex();
}

void DFG_CLASS_NAME(CsvTableView)::onColumnResizeAction_toViewEvenly()
{
    auto pViewPort = viewport();
    auto pHorizontalHeader = horizontalHeader();
    const auto nColCount = pHorizontalHeader ? pHorizontalHeader->count() : 0;
    if (!pViewPort || nColCount < 1)
        return;
    const auto w = pViewPort->width();
    const int sectionSize = Max(w / nColCount, getCsvTableViewProperty<CsvTableViewPropertyId_minimumVisibleColumnWidth>(this));
    pHorizontalHeader->setDefaultSectionSize(sectionSize);
}

void DFG_CLASS_NAME(CsvTableView)::onColumnResizeAction_toViewContentAware()
{
    auto pHeader = horizontalHeader();
    auto pViewPort = viewport();
    const auto numHeader = (pHeader) ? pHeader->count() : 0;
    if (!pHeader || !pViewPort || numHeader < 1)
        return;

    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ValueVector)<int> sizes;
    sizes.reserve(static_cast<size_t>(numHeader));
    for (int i = 0; i < numHeader; ++i)
    {
        // Note: using only content hints, to take column header width into account, use pHeader->sectionSizeHint(i);
        sizes.push_back(sizeHintForColumn(i));
        //sizes.push_back(pHeader->sectionSizeHint(i));
    }
    // Using int64 in accumulate to avoid possibility for integer overflow.
    const int64 nHintTotalWidth = DFG_MODULE_NS(numeric)::accumulate(sizes, int64(0));
    const auto nAvailableTotalWidth = pViewPort->width();

    if (nHintTotalWidth < nAvailableTotalWidth)
    {
        // Case: content requirements are less than available window width.
        // Action: increase width for every column.
        auto nFreePixels = static_cast<int>(nAvailableTotalWidth - nHintTotalWidth);
        const auto nIncrement = nFreePixels / numHeader;
        for (int i = 0; i < numHeader; ++i)
        {
            pHeader->resizeSection(i, sizes[i] + nIncrement);
            nFreePixels -= nIncrement;
        }
        if (nFreePixels > 0) // If division wasn't even, add remainder to last column
            pHeader->resizeSection(numHeader - 1, pHeader->sectionSize(numHeader - 1) + nFreePixels);
        return;
    }
    else // case: content width >= available space.
    {
        /* Rough description of logics: resize all 'less than average'-width columns to content, for others
           distribute remaining space evenly.

         1. Calculate truncation limit as average width available for remaining columns.
         2. Go through columns and if there is one whose needed width is less than truncation limit,
            resize it, mark it handled and goto 1. Note that this operation may free width for remaining columns
            so on next round the truncation limit may be greater.
         3. If all remaining columns have size greater than truncation limit, distribute available width evenly for those.
         */

        int numUnhandled = numHeader;
        int nAvailableWidth = nAvailableTotalWidth;
        while (numUnhandled > 0)
        {
            // 1.
            const int truncationLimit = nAvailableWidth / numUnhandled;
            bool bHandledOne = false;
            // 2.
            for (int i = 0; i < numHeader; ++i)
            {
                if (sizes[i] < 0)
                    continue; // Column already handled.
                if (sizes[i] <= truncationLimit)
                {
                    pHeader->resizeSection(i, sizes[i]);
                    numUnhandled--;
                    nAvailableWidth -= sizes[i];
                    sizes[i] = -1; // Using -1 as "already handled"-indicator.
                    bHandledOne = true;
                    break;
                }
            }
            if (bHandledOne)
                continue;
            // 3.
            // All headers were wider than truncation limit -> distribute space evenly.
            for (int i = 0; i < numHeader; ++i)
            {
                if (sizes[i] > truncationLimit)
                {
                    numUnhandled--;
                    nAvailableWidth -= truncationLimit;
                    if (nAvailableWidth >= truncationLimit)
                        pHeader->resizeSection(i, truncationLimit);
                    else // Final column, add the remainder width there.
                    {
                        pHeader->resizeSection(i, truncationLimit + nAvailableWidth);
                        break;
                    }
                }
            }
            DFG_ASSERT_CORRECTNESS(numUnhandled == 0);
            numUnhandled = 0; // Set to zero as if there's a bug it's better to have malfunctioning size behaviour than
                              // infinite loop here.
        }
    } // 'Needs truncation'-case
}

DFG_ROOT_NS_BEGIN
{
    namespace
    {
        void onHeaderResizeAction_content(QHeaderView* pHeader)
        {
            if (!pHeader)
                return;
            const auto waitCursor = makeScopedCaller([]() { QApplication::setOverrideCursor(QCursor(Qt::WaitCursor)); },
                                                     []() { QApplication::restoreOverrideCursor(); });
            pHeader->resizeSections(QHeaderView::ResizeToContents);
        }

        void onHeaderResizeAction_fixedSize(QWidget* pParent, QHeaderView* pHeader)
        {
            if (!pHeader)
                return;
            bool bOk = false;
            const auto nCurrentSetting = pHeader->defaultSectionSize();
            const auto nNewSize = QInputDialog::getInt(pParent,
                                                       QApplication::tr("Header size"),
                                                       QApplication::tr("New header size"),
                                                       nCurrentSetting,
                                                       1,
                                                       1048575, // Maximum section size at least in Qt 5.2
                                                       1,
                                                       &bOk);
            if (bOk && nNewSize > 0)
            {
                pHeader->setDefaultSectionSize(nNewSize);
            }
        }
    }
}

void DFG_CLASS_NAME(CsvTableView)::onColumnResizeAction_content()
{
    onHeaderResizeAction_content(horizontalHeader());
}

void DFG_CLASS_NAME(CsvTableView)::onRowResizeAction_content()
{
    onHeaderResizeAction_content(verticalHeader());
}

void DFG_CLASS_NAME(CsvTableView)::onColumnResizeAction_fixedSize()
{
    onHeaderResizeAction_fixedSize(this, horizontalHeader());
}

void DFG_CLASS_NAME(CsvTableView)::onRowResizeAction_fixedSize()
{
    onHeaderResizeAction_fixedSize(this, verticalHeader());
}

QModelIndex DFG_CLASS_NAME(CsvTableView)::mapToSource(const QAbstractItemModel* pModel, const QAbstractProxyModel* pProxy, const int r, const int c)
{
    if (pProxy)
        return pProxy->mapToSource(pProxy->index(r, c));
    else if (pModel)
        return pModel->index(r, c);
    else
        return QModelIndex();
}

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzer)::DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzer)(PanelT* uiPanel)
   : m_spUiPanel(uiPanel)
{
}

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzer)::~DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzer)()
{
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzer)::analyzeImpl(QAbstractItemView* pView, const QItemSelection& selection)
{
    auto uiPanel = m_spUiPanel.data();
    if (!uiPanel)
        return;

    auto pCtvView = qobject_cast<DFG_CLASS_NAME(const CsvTableView)*>(pView);
    auto pModel = (pCtvView) ? pCtvView->csvModel() : nullptr;
    if (!pModel)
    {
        uiPanel->setValueDisplayString(QString());
        return;
    }

    const auto maxTime = uiPanel->getMaxTimeInSeconds();
    const auto enabled = (!DFG_MODULE_NS(math)::isNan(maxTime) &&  maxTime > 0);
    DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncMinMax)<double> minMaxMf;
    DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncAvg)<double> avgMf;
    int nExcluded = 0;
    CompletionStatus completionStatus = CompletionStatus_started;
    DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) operationTimer;
    uiPanel->onEvaluationStarting(enabled);
    if (enabled)
    {
        for(auto iter = selection.cbegin(); iter != selection.cend(); ++iter)
        {
            pCtvView->forEachCsvModelIndexInSelectionRange(*iter, [&](const QModelIndex& index, bool& rbContinue)
            {
                const auto bHasMaxTimePassed = operationTimer.elapsedWallSeconds() >= maxTime;
                if (bHasMaxTimePassed || uiPanel->isStopRequested())
                {
                    if (bHasMaxTimePassed)
                        completionStatus = ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus_terminatedByTimeLimit;
                    else
                        completionStatus = ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus_terminatedByUserRequest;
                    rbContinue = false;
                    return;
                }
                QString str = pModel->data(index).toString();
                str.replace(',', '.'); // Hack: to make comma-localized values such as "1,2" be interpreted as 1.2
                bool bOk;
                const double val = str.toDouble(&bOk);
                if (bOk)
                {
                    avgMf(val);
                    minMaxMf(val);
                }
                else
                    nExcluded++;
            });
        }
        const auto elapsedTime = operationTimer.elapsedWallSeconds();
        if (completionStatus == CompletionStatus_started)
            completionStatus = CompletionStatus_completed;
        uiPanel->onEvaluationEnded(elapsedTime, completionStatus);

        QString sMessage;
        if (completionStatus == CompletionStatus_completed)
            sMessage = uiPanel->tr("Included: %1, Excluded: %2, Sum: %3, Avg: %4, Min: %5, Max: %6")
                                                                         .arg(avgMf.callCount())
                                                                         .arg(nExcluded)
                                                                         .arg(floatToQString(avgMf.sum()))
                                                                         .arg(floatToQString(avgMf.average()))
                                                                         .arg(floatToQString(minMaxMf.minValue()))
                                                                         .arg(floatToQString(minMaxMf.maxValue()));
        else if (completionStatus == CompletionStatus_terminatedByTimeLimit)
            sMessage = uiPanel->tr("Interrupted (time limit exceeded)");
        else if (completionStatus == CompletionStatus_terminatedByUserRequest)
            sMessage = uiPanel->tr("Stopped");
        else
            sMessage = uiPanel->tr("Interrupted (unknown reason)");

        uiPanel->setValueDisplayString(sMessage);
    }
}
