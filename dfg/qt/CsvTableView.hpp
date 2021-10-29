#pragma once

#include "../dfgDefs.hpp"
#include "TableView.hpp"
#include "../cont/TorRef.hpp"
#include "StringMatchDefinition.hpp"
#include <memory>
#include <atomic>
#include <functional>
#include <tuple>
#include "../OpaquePtr.hpp"

#include "qtIncludeHelpers.hpp"

#include "containerUtils.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QHeaderView>
    #include <QPointer>
DFG_END_INCLUDE_QT_HEADERS

class QUndoStack;
class QAbstractProxyModel;
class QCheckBox;
class QDate;
class QDateTime;
class QItemSelection;
class QItemSelectionRange;
class QMenu;
class QPoint;
class QProgressBar;
class QPushButton;
class QThread;
class QMutex;
class QReadWriteLock;
class QTime;
class QToolButton;


namespace DFG_ROOT_NS
{
    class CsvFormatDefinition;
}

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(cont) {
    class CsvConfig;
} }

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(math) {
    class FormulaParser;
} }

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    class CsvItemModel;
    class CsvTableView;

    class CsvTableViewBasicSelectionAnalyzerPanel;

    // Typed column index representing index of data model
    class ColumnIndex_data
    {
    public:
        explicit ColumnIndex_data(const int nCol = -1)
            : m_nCol(nCol)
        {}
        int value() const { return m_nCol; }
        int m_nCol;
    };

    // Typed column index representing index of view model
    class ColumnIndex_view
    {
    public:
        explicit ColumnIndex_view(const int nCol = -1)
            : m_nCol(nCol)
        {}
        int value() const { return m_nCol; }
        int m_nCol;
    };

    // Baseclass of selection detail collectors
    class SelectionDetailCollector
    {
    public:
        static const char s_propertyName_id[];
        static const char s_propertyName_type[];
        static const char s_propertyName_uiNameShort[];
        static const char s_propertyName_uiNameLong[];
        static const char s_propertyName_description[];

        SelectionDetailCollector(StringUtf8 sId);
        SelectionDetailCollector(const QString& sId);
        virtual ~SelectionDetailCollector();

        StringViewUtf8 id() const;

        void enable(bool b, bool bUpdateCheckBox = true);
        bool isEnabled() const;

        void setProperty(const QString& sKey, const QVariant value);
        QVariant getProperty(const QString& sKey, const QVariant defaultValue = QVariant()) const;

        QString getUiName_long() const;
        QString getUiName_short() const;

        QCheckBox* createCheckBox(QMenu* pParent);
        QCheckBox* getCheckBoxPtr();
        QAction* getCheckBoxAction();

        bool isBuiltIn() const;

        double value() const { return (m_bNeedsUpdate) ? valueImpl() : std::numeric_limits<double>::quiet_NaN(); }
        void update(const double val) { if (m_bNeedsUpdate) updateImpl(val); }

        // Resets collector clearing memory from previous collection around.
        void reset() { if (m_bNeedsUpdate) resetImpl(); }

        QString exportDefinitionToJson() const;

    private:
        virtual double valueImpl() const { return std::numeric_limits<double>::quiet_NaN(); }
        virtual void updateImpl(double val) { DFG_UNUSED(val); }
        virtual void resetImpl() {}

    protected:
        bool m_bNeedsUpdate = false;
        DFG_OPAQUE_PTR_DECLARE();
    }; // class SelectionDetailCollector

    // Selection detail collector using formula parser.
    class SelectionDetailCollector_formula : public SelectionDetailCollector
    {
    public:
        using BaseClass = SelectionDetailCollector;
        using FormulaParser = ::DFG_MODULE_NS(math)::FormulaParser;

        static const char s_propertyName_formula[];
        static const char s_propertyName_initialValue[];

        SelectionDetailCollector_formula(StringUtf8 sId, StringViewUtf8 svFormula, double initialValue);
        ~SelectionDetailCollector_formula();

    private:
        double valueImpl() const override;
        void updateImpl(double val) override;
        void resetImpl() override;

        DFG_OPAQUE_PTR_DECLARE();
    }; // Class SelectionDetailCollector_formula

    // Stores SelectionDetailCollectors
    class SelectionDetailCollectorContainer : public std::vector<std::shared_ptr<SelectionDetailCollector>>
    {
    public:
        using BaseClass = std::vector<std::shared_ptr<SelectionDetailCollector>>;
        using Container = BaseClass;

        auto find(const StringViewUtf8& id) -> SelectionDetailCollector*;
        auto find(const QString& id) -> SelectionDetailCollector*;

        bool erase(const StringViewUtf8& id);

    private:
        auto findIter(const StringViewUtf8& id) -> Container::iterator;
    }; // class SelectionDetailCollectorContainer

    // Analyzes item selection
    class CsvTableViewSelectionAnalyzer : public QObject
    {
        Q_OBJECT
    public:
        enum CompletionStatus
        {
            CompletionStatus_started,
            CompletionStatus_completed,
            CompletionStatus_terminatedByTimeLimit,
            CompletionStatus_terminatedByUserRequest,
            CompletionStatus_terminatedByNewSelection,
            CompletionStatus_terminatedByDisabling
        };

        CsvTableViewSelectionAnalyzer();

        virtual ~CsvTableViewSelectionAnalyzer();

        // Adds given selection to analyzation queue. Analyzer may decide how to handle queue: it can e.g. terminate current analysis and schelude a new one with given selection.
        // Note: this interface is thread safe.
        void addSelectionToQueue(QItemSelection selection) { if (m_abIsEnabled) addSelectionToQueueImpl(selection); }

        // Disables this analyzer (=i.e. it won't accept any new selections and disables any further work). Note though that existing work may still get completed.
        // Note: this interface is thread safe.
        void disable() { m_abIsEnabled = false; }

    signals:
        void sigAnalyzeCompleted();

    public slots:
        void onCheckAnalyzeQueue();

    public:       
        std::atomic<bool> m_abIsEnabled;
        std::unique_ptr<QMutex> m_spMutexAnalyzeQueue; // For protecting queue handling
        std::vector<QItemSelection> m_analyzeQueue;
        bool m_bPendingCheckQueue; // True when there is pending signal for onCheckAnalyzeQueue(). Used to prevent signal queue from forming.
        std::atomic<bool> m_abNewSelectionPending; // True when there's a new selection pending.
        QPointer<QAbstractItemView> m_spView;
    private:
        // Implementation must be thread safe and must not block calling thread e.g. by starting new analysis on the calling thread.
        virtual void addSelectionToQueueImpl(QItemSelection selection);

        virtual void analyzeImpl(QItemSelection selection) = 0;
    }; // CsvTableViewSelectionAnalyzer

    class CsvTableViewBasicSelectionAnalyzer : public CsvTableViewSelectionAnalyzer
    {
        Q_OBJECT
    public:
        typedef CsvTableViewBasicSelectionAnalyzerPanel PanelT;

        CsvTableViewBasicSelectionAnalyzer(PanelT* uiPanel);
        ~CsvTableViewBasicSelectionAnalyzer() DFG_OVERRIDE_DESTRUCTOR;

        QPointer<CsvTableViewBasicSelectionAnalyzerPanel> m_spUiPanel;
    private:
        void analyzeImpl(QItemSelection selection) override;
    }; // Class CsvTableViewBasicSelectionAnalyzer

    class CsvTableViewBasicSelectionAnalyzerPanel : public QWidget
    {
        Q_OBJECT
    public:
        typedef CsvTableViewBasicSelectionAnalyzerPanel ThisClass;
        typedef QWidget BaseClass;
        using CollectorContainerPtr = std::shared_ptr<SelectionDetailCollectorContainer>;

        CsvTableViewBasicSelectionAnalyzerPanel(QWidget *pParent = nullptr);
        virtual ~CsvTableViewBasicSelectionAnalyzerPanel();

        void setValueDisplayString(const QString& s);

        void onEvaluationStarting(bool bEnabled);
        void onEvaluationEnded(const double timeInSeconds, CsvTableViewSelectionAnalyzer::CompletionStatus completionStatus);

        double getMaxTimeInSeconds() const;
        bool isStopRequested() const;

        // Thread-safe
        void clearAllDetails();

        // Thread-safe
        bool addDetail(const QVariantMap& items);

        // Thread-safe
        bool deleteDetail(const StringViewUtf8& id);

        // Thread-safe
        void setDefaultDetails();

        // Thread-safe
        CollectorContainerPtr collectors() const;

    signals:
        void sigEvaluationStartingHandleRequest(bool bEnabled);
        void sigEvaluationEndedHandleRequest(const double timeInSeconds, int completionStatus);
        void sigSetValueDisplayString(const QString& s);

    private slots:
        void onEvaluationStarting_myThread(bool bEnabled);
        void onEvaluationEnded_myThread(const double timeInSeconds, int completionStatus);
        void setValueDisplayString_myThread(const QString& s);
        void onAddCustomCollector();
        void onEnableAllDetails();
        void onDisableAllDetails();

    private:
        void setEnableStatusForAll(bool b);

    private:
        QObjectStorage<QLineEdit>      m_spValueDisplay;
        QObjectStorage<QToolButton>    m_spDetailSelector;
        QPointer<QLineEdit>            m_spTimeLimitDisplay;
        QObjectStorage<QProgressBar>   m_spProgressBar;
        QObjectStorage<QPushButton>    m_spStopButton;

        DFG_OPAQUE_PTR_DECLARE();
    }; // class CsvTableViewBasicSelectionAnalyzerPanel

    class TableHeaderView : public QHeaderView
    {
        Q_OBJECT
    public:
        using BaseClass = QHeaderView;

        TableHeaderView(CsvTableView* pParent);

        CsvTableView*       tableView();
        const CsvTableView* tableView() const;

        int columnIndex_dataModel(const QPoint& pos) const;
        int columnIndex_viewModel(const QPoint& pos) const;

        // Base class overrides -->
        void contextMenuEvent(QContextMenuEvent* pEvent) override;
        // <-- Base class overrides

        int m_nLatestContextMenuEventColumn_dataModel = -1;
    };

    // View for showing CsvItemModel.
    class CsvTableView : public TableView
    {
        Q_OBJECT

    public:
        typedef TableView BaseClass;
        typedef CsvTableView ThisClass;
        typedef CsvItemModel CsvModel;
        typedef StringMatchDefinition StringMatchDef;
        typedef CsvTableViewSelectionAnalyzer SelectionAnalyzer;
        using PropertyFetcher = std::function<std::pair<QString, QString>()>;
        using CsvConfig = ::DFG_MODULE_NS(cont)::CsvConfig;

        enum class ViewType
        {
            allFeatures,        // View that has all features enabled
            fixedDimensionEdit  // View that has only features for editing underlying model; no ability to insert/removes rows or columns, no ability to open/save file.
        };

        enum ModelIndexType
        {
            ModelIndexTypeSource, // Refers to indexes in underlying data model.
            ModelIndexTypeView    // Refers to indexes in view model (which can be the same as source-model in case there is no proxy).
        };

        enum class ForEachOrder
        {
            any,
            inOrderFirstRows // Rows from top to bottom and then channel increment.
        };

        enum class ProxyModelInvalidation
        {
            no,
            ifNeeded
        };

        CsvTableView(std::shared_ptr<QReadWriteLock> spReadWriteLock, QWidget* pParent, ViewType viewType = ViewType::allFeatures);
        ~CsvTableView() DFG_OVERRIDE_DESTRUCTOR;

        // If already present, old undo stack will be destroyed.
        void createUndoStack();
        void clearUndoStack();
        void showUndoWindow();

        void setExternalUndoStack(QUndoStack* pUndoStack);

        void setModel(QAbstractItemModel* pModel) override;
        CsvModel* csvModel();
        const CsvModel* csvModel() const;

        // Returns the smallest row index in the current view selection,
        // row count if no selection exists.
        int getFirstSelectedViewRow() const;

        // Returns true iff current selection is non-empty, i.e. at least one cell is selected.
        bool isSelectionNonEmpty() const;

        // Returns list of row indexes of column @p nCol.
        // If proxy model is given, the returned indexes will correspond
        // to the indexes of the underlying model, otherwise they will be
        // {0, 1,..., rowCount()-1}
        std::vector<int> getRowsOfCol(const int nCol, const QAbstractProxyModel* pProxy) const;

        // Returned list is free of duplicates. If @p pProxy != nullptr,
        // the selected indexes will be mapped by the proxy.
        std::vector<int> getRowsOfSelectedItems(const QAbstractProxyModel* pProxy) const;

        // Returns list of selected indexes as model indexes of underlying data model.
        QModelIndexList getSelectedItemIndexes_dataModel() const;

        // Returns list of selected indexes as model indexes from model(). If there is no proxy model,
        // returns the same as getSelectedItemIndexes_dataModel().
        QModelIndexList getSelectedItemIndexes_viewModel() const;

        // Convenience method for returning row count of data model (i.e. the number of rows in the underlying table).
        int getRowCount_dataModel() const;
        // Convenience method for returning row count of visible model (i.e. the number of visible rows).
        int getRowCount_viewModel() const;

        QString getColumnName(ColumnIndex_data index) const;
        QString getColumnName(ColumnIndex_view index) const;

        std::vector<int> getDataModelRowsOfSelectedItems() const
        {
            return getRowsOfSelectedItems(getProxyModelPtr());
        }

        void invertSelection();

        bool isRowMode() const;

        QAbstractProxyModel* getProxyModelPtr();
        const QAbstractProxyModel* getProxyModelPtr() const;

        bool saveToFileImpl(const QString& path, const CsvFormatDefinition& formatDef);
        bool saveToFileImpl(const CsvFormatDefinition& formatDef);

        void privAddUndoRedoActions(QAction* pAddBefore = nullptr);

        bool generateContentImpl(const CsvModel& csvModel);

        bool getAllowApplicationSettingsUsage() const;
        void setAllowApplicationSettingsUsage(bool b);

        void finishEdits();

        int getFindColumnIndex() const;

        void onFind(const bool forward);

        void addSelectionAnalyzer(std::shared_ptr<SelectionAnalyzer> spAnalyzer);

        void removeSelectionAnalyzer(const SelectionAnalyzer* pAnalyzer);

        // Maps index to view model (i.e. the one returned by model()) assuming that 'index' is either from csvModel() or model().
        QModelIndex mapToViewModel(const QModelIndex& index) const;

        // Maps index to data model (i.e. the one returned by csvModel()) assuming that 'index' is either from csvModel() or model().
        QModelIndex mapToDataModel(const QModelIndex& index) const;

        // Forgets latest find position so that next begins from memoryless situation.
        void forgetLatestFindPosition();

        std::unique_ptr<QMenu> createResizeColumnsMenu();

        template <class Func_T>
        void forEachCsvModelIndexInSelection(Func_T func);

        template <class Func_T>
        void forEachCsvModelIndexInSelection(Func_T func) const;

        // Calls given func for every index at given QItemSelectionRange.
        // func gets two arguments: (const QModelIndex& index, bool& rbContinue)
        template <class Func_T>
        void forEachCsvModelIndexInSelectionRange(const QItemSelectionRange& sr, ForEachOrder order, Func_T func);
        template <class Func_T>
        void forEachCsvModelIndexInSelectionRange(const QItemSelectionRange& sr, Func_T func) { forEachCsvModelIndexInSelectionRange(sr, ForEachOrder::any, std::forward<Func_T>(func)); }

        // Const overload
        template <class Func_T>
        void forEachCsvModelIndexInSelectionRange(const QItemSelectionRange& sr, ForEachOrder order, Func_T func) const;
        template <class Func_T>
        void forEachCsvModelIndexInSelectionRange(const QItemSelectionRange& sr, Func_T func) const { forEachCsvModelIndexInSelectionRange(sr, ForEachOrder::any, std::forward<Func_T>(func)); }

        // Returns viewModel->index(r, c) mapped to source model, QModelIndex() if neither pModel or pProxy is available.
        static QModelIndex mapToSource(const QAbstractItemModel* pModel, const QAbstractProxyModel* pProxy, int r, int c);

        bool openFile(const QString& sPath);
        bool openFile(const QString& sPath, const CsvFormatDefinition& formatDef);

        // Shows QToolTip-like status info to user.
        void showStatusInfoTip(const QString& sMsg);

        QString privCreateActionBlockedDueToLockedContentMessage(const QString& actionname);
        void privShowExecutionBlockedNotification(const QString& actionname);

        QString makeClipboardStringForCopy(QChar cDelim = '\t');

        // Convenience function, effectively returns selectionModel()->selection();
        QItemSelection getSelection() const;

        LockReleaser tryLockForEdit(); // Note: edits include both content edit and view edits such as changing filter.
        LockReleaser tryLockForRead();

        TableHeaderView* horizontalTableHeader();

        // Adds a callback that is called to fetch additional properties for config file when saving
        void addConfigSavePropertyFetcher(PropertyFetcher fetcher);

        using SelectionRangeList = std::vector<std::tuple<int, int, int, int>>;

        // Stores current selection so that it can be restored later with restoreSelection()
        SelectionRangeList storeSelection() const;

        // Restores selection created by storeSelection()
        // Note: Only supported for case where table internals or geometry (row/column count, model object etc.) does not change between storeSelection() and restoreSelection()
        void restoreSelection(const SelectionRangeList& selection) const;

        bool isReadOnlyMode() const;

        bool isColumnVisible(ColumnIndex_data nCol) const;

        ColumnIndex_view columnIndexDataToView(ColumnIndex_data) const; // Returns view column index of given data index
        ColumnIndex_data columnIndexViewToData(ColumnIndex_view) const; // Returns data column index of given view index

        CsvConfig populateCsvConfig(const CsvItemModel& rCsvModel);

        QStringList weekDayNames() const;
        QString dateTimeToString(const QDateTime& dateTime, const QString& sFormat) const;
        QString dateTimeToString(const QDate& date, const QString& sFormat) const;
        QString dateTimeToString(const QTime& qtime, const QString& sFormat) const;

        QDate insertDate(); // Returns QDate that was used for creating the inserted string.
        QTime insertTime(); // Returns QTime that was used for creating the inserted string.
        QDateTime insertDateTime(); // Returns QDateTime that was used for creating the inserted string.

        QVariant getColumnPropertyByDataModelIndex(int nDataModelCol, const StringViewUtf8& svKey, QVariant defaultValue = QVariant()) const;

        void invalidateSortFilterProxyModel();

    private:
        template <class T, class Param0_T>
        bool executeAction(Param0_T&& p0);

        template <class T, class Param0_T, class Param1_T>
        bool executeAction(Param0_T&& p0, Param1_T&& p1);

        template <class T, class Param0_T, class Param1_T, class Param2_T>
        bool executeAction(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2);

        template <class T, class Param0_T>
        void pushToUndoStack(Param0_T&& p0);

        template <class T, class Param0_T, class Param1_T>
        void pushToUndoStack(Param0_T&& p0, Param1_T&& p1);

        template <class T, class Param0_T, class Param1_T, class Param2_T>
        void pushToUndoStack(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2);

    public:
        template <class This_T, class Func_T>
        static void forEachViewModelIndexInSelection(This_T& thisItem, Func_T func);

        template <class This_T, class Func_T>
        static void forEachCsvModelIndexInSelection(This_T& thisItem, Func_T func);

        template <class This_T, class Func_T>
        static void forEachIndexInSelection(This_T& thisItem, ModelIndexType indexType, Func_T&& func);

        template <class This_T, class Func_T>
        static void forEachIndexInSelection(This_T& thisItem, const QItemSelection &selection, ModelIndexType indexType, Func_T&& func);

        template <class This_T, class Func_T>
        static void forEachCsvModelIndexInSelectionRange(This_T& thisItem, const QItemSelectionRange& sr, ForEachOrder order, Func_T func);

        template <class This_T, class Func_T>
        static void forEachIndexInSelectionRange(This_T& thisItem, const QItemSelectionRange& sr, ForEachOrder order, ModelIndexType indexType, Func_T func);

    public slots:
        void createNewTable();
        bool createNewTableFromClipboard();
        bool openFromFile();
        bool openFromFileWithOptions();
        bool reloadFromFile();
        bool mergeFilesToCurrent();
        bool save();
        bool saveToFile();
        bool saveToFileWithOptions();
        bool openAppConfigFile();
        bool openConfigFile();
        bool saveConfigFile();
        bool saveConfigFileWithOptions();
        bool clearSelected();
        bool insertRowHere();
        bool insertRowAfterCurrent();
        bool insertColumn();
        bool insertColumnAfterCurrent();
        bool paste();
        bool copy();
        bool cut();
        void undo();
        void redo();
        size_t replace(const QVariantMap& params); // Returns the number of cells edited.

        bool deleteCurrentColumn();
        bool deleteCurrentColumn(const int nCol);
        bool deleteSelectedRow();

        bool moveFirstRowToHeader();
        bool moveHeaderToFirstRow();

        bool resizeTable();
        bool resizeTableNoUi(int r, int c);
        bool transpose();

        bool generateContent();

        void evaluateSelectionAsFormula();

        bool diffWithUnmodified();

        void onGoToCellTriggered();
        void onFindRequested();
        void onFindNext();
        void onFindPrevious();

        void onReplace();

        void onFilterRequested();

        void setCaseSensitiveSorting(bool bCaseSensitive);
        void resetSorting();

        void setFindText(const StringMatchDef matchDef, const int nCol);

        void onNewSourceOpened();

        void onSelectionModelChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void onViewModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

        // To be triggered when either set of selected cells or content in existing selection changes.
        void onSelectionModelOrContentChanged(const QItemSelection& selected, const QItemSelection& deselected, const QItemSelection& editedViewModelItems);

        void onColumnResizeAction_toViewEvenly();
        void onColumnResizeAction_toViewContentAware();
        void onColumnResizeAction_content();
        void onColumnResizeAction_fixedSize();

        void onRowResizeAction_content();
        void onRowResizeAction_fixedSize();

        void setRowMode(bool);
        void setUndoEnabled(bool);
        void setReadOnlyMode(bool);

        void insertGeneric(const QString& s);

        // Column header action handlers
        void setColumnNames();
        void setColumnVisibility(int nCol, bool bVisible, ProxyModelInvalidation = ProxyModelInvalidation::ifNeeded);
        void unhideAllColumns();
        void showSelectColumnVisibilityDialog();

        /*
        void pasteColumn();
        void pasteColumn(const int nCol);
        void copyColumn();
        void copyColumn(const int nCol);
        void moveColumnRight();
        void moveColumnLeft();
        void moveRowDown();
        void moveRowUp();
        void renameColumn();
        void renameColumn(const int nCol);
        
        void insertColumnAfterCurrent();
        void insertColumnAfterCurrent(const int nCol);
        void insertColumn(const int nCol);
        void insertAfterCurrent();
        void insert();
        void deleteSelected();
        */

    signals:
        void sigFindActivated();
        void sigFilterActivated();
        void sigSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected, const QItemSelection& editedItems);
        void sigOnAllowApplicationSettingsUsageChanged(bool);
        void sigReadOnlyModeChanged(bool);

    protected:
        void contextMenuEvent(QContextMenuEvent* pEvent) override;
        QModelIndexList selectedIndexes() const override;

    private:
        template <class Func_T>
        void forEachCompleterEnabledColumnIndex(Func_T func);

        template <class Func_T> void forEachSelectionAnalyzer(Func_T func);
        template <class Func_T> void forEachSelectionAnalyzerThread(Func_T func);

        bool getProceedConfirmationFromUserIfInModifiedState(const QString& sTranslatedActionDescription);

        void stopAnalyzerThreads();

        void addAllActions();
        void addOpenSaveActions();
        void addDimensionEditActions();
        void addFindAndSelectionActions();
        void addContentEditActions();
        void addSortActions();
        void addHeaderActions();
        void addMiscellaneousActions();

        void addSeparatorAction();
        void addSeparatorActionTo(QWidget* pTarget);

        template <class Str_T>
        void setReadOnlyModeFromProperty(const Str_T& s);

        QString askConfigFilePath(CsvItemModel& rModel);
        bool saveConfigFileTo(const CsvConfig& config, const QString& sPath);

    public:
        std::unique_ptr<DFG_MODULE_NS(cont)::TorRef<QUndoStack>> m_spUndoStack;
        QStringList m_tempFilePathsToRemoveOnExit;
        QModelIndex m_latestFoundIndex; // Index from underlying model. Invalid if doing first find.
        StringMatchDef m_matchDef;
        int m_nFindColumnIndex;
        std::vector<std::shared_ptr<SelectionAnalyzer>> m_selectionAnalyzers; // All items are guaranteed to be non-null.
        std::unique_ptr<QMenu> m_spResizeColumnsMenu;
        bool m_bUndoEnabled;
        std::vector<QObjectStorage<QThread>> m_analyzerThreads;
        std::shared_ptr<QReadWriteLock> m_spEditLock; // For controlling when table can be edited.
        DFG_OPAQUE_PTR_DECLARE();
    }; // class CsvTableView

    // Helper class providing CsvTableView readily usable in a dialog
    class CsvTableViewDlg
    {
    public:
        CsvTableViewDlg(std::shared_ptr<QReadWriteLock> spReadWriteLock, QWidget* pParent, CsvTableView::ViewType viewType = CsvTableView::ViewType::allFeatures);

        void setModel(QAbstractItemModel* pModel);

        CsvTableView& csvView();

        // Adds widget to layout.
        void addVerticalLayoutWidget(int nPos, QWidget* pWidget);

        QWidget& dialog();
        void resize(const int w, const int h);

        int exec();

        DFG_OPAQUE_PTR_DECLARE();
    }; // class CSvTableViewDlg

    template <class Func_T>
    void CsvTableView::forEachCsvModelIndexInSelection(Func_T func)
    {
        forEachCsvModelIndexInSelection(*this, std::forward<Func_T>(func));
    }

    template <class Func_T>
    void CsvTableView::forEachCsvModelIndexInSelection(Func_T func) const
    {
       forEachCsvModelIndexInSelection(*this, std::forward<Func_T>(func));
    }

    template <class Func_T>
    void CsvTableView::forEachCsvModelIndexInSelectionRange(const QItemSelectionRange& sr, ForEachOrder order, Func_T func)
    {
        forEachCsvModelIndexInSelectionRange(*this, sr, order, std::forward<Func_T>(func));
    }

    template <class Func_T>
    void CsvTableView::forEachCsvModelIndexInSelectionRange(const QItemSelectionRange& sr, ForEachOrder order, Func_T func) const
    {
        forEachCsvModelIndexInSelectionRange(*this, sr, order, std::forward<Func_T>(func));
    }

    template <class This_T, class Func_T>
    void CsvTableView::forEachCsvModelIndexInSelection(This_T& thisItem, Func_T func)
    {
        forEachIndexInSelection(thisItem, ModelIndexTypeSource, std::forward<Func_T>(func));
    }

    template <class This_T, class Func_T>
    void CsvTableView::forEachIndexInSelection(This_T& thisItem, const QItemSelection& selection, const ModelIndexType indexType, Func_T&& func)
    {
        for (auto iter = selection.cbegin(); iter != selection.cend(); ++iter)
        {
            forEachIndexInSelectionRange(thisItem, *iter, ForEachOrder::any, indexType, func);
        }
    }

    template <class This_T, class Func_T>
    void CsvTableView::forEachIndexInSelection(This_T& thisItem, const ModelIndexType indexType, Func_T&& func)
    {
        const auto sm = thisItem.selectionModel();
        const auto selection = (sm) ? sm->selection() : QItemSelection();
        forEachIndexInSelection(thisItem, selection, indexType, std::forward<Func_T>(func));
    }

    template <class This_T, class Func_T>
    void CsvTableView::forEachCsvModelIndexInSelectionRange(This_T& thisItem, const QItemSelectionRange& sr, ForEachOrder order, Func_T func)
    {
        forEachIndexInSelectionRange(thisItem, sr, order, ModelIndexTypeSource, std::forward<Func_T>(func));
    }

    template <class This_T, class Func_T>
    void CsvTableView::forEachIndexInSelectionRange(This_T& thisItem, const QItemSelectionRange& sr, const ForEachOrder order, const ModelIndexType indexType, Func_T func)
    {
        DFG_UNUSED(order);
        auto pProxy = (indexType == ModelIndexTypeSource) ? thisItem.getProxyModelPtr() : nullptr;
        auto pModel = thisItem.model();
        if (!pModel)
            return;
        // TODO: if not having proxy, iterate in the way that is optimal to underlying data structure
        const auto right = sr.right();
        const auto bottom = sr.bottom();
        bool bContinue = true;
        for (int c = sr.left(); c<=right && bContinue; ++c)
        {
            for (int r = sr.top(); r<=bottom && bContinue; ++r)
            {
                func(mapToSource(thisItem.model(), pProxy, r, c), bContinue);
            }
        }
    }

} } // module namespace
