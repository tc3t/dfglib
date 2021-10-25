#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "qtBasic.hpp"
#include "../cont/tableCsv.hpp"
#include "../io/textEncodingTypes.hpp"
#include "StringMatchDefinition.hpp"
#include "../build/languageFeatureInfo.hpp"
#include "../cont/arrayWrapper.hpp"
#include "../OpaquePtr.hpp"
#include "containerUtils.hpp"
#include "../numericTypeTools.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QAbstractTableModel>
#include <QBrush>
#include <QTextStream>
DFG_END_INCLUDE_QT_HEADERS

#include "../io/DelimitedTextWriter.hpp"
#include "../io/DelimitedTextReader.hpp"
#include <memory>
#include <vector>
#include <atomic>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont)
{
    template <class Cont_T>
    class DFG_CLASS_NAME(SortedSequence);

    class DFG_CLASS_NAME(CsvConfig);
} }

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io)
{
    class DFG_CLASS_NAME(OfStreamWithEncoding);
} }

class QUndoStack;
class QCompleter;
class QFile;
class QTextStream;
class QReadWriteLock;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

#ifndef DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS
    #define DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS  0
#endif

    const char CsvOptionProperty_completerColumns[]          = "completerColumns";
    const char CsvOptionProperty_completerEnabledSizeLimit[] = "completerEnabledSizeLimit";
    const char CsvOptionProperty_includeRows[]               = "includeRows"; // To define rows to read from file, as IntervalSet syntax. ASCII, 0-based index
    const char CsvOptionProperty_includeColumns[]            = "includeColumns"; // To define columns to read from file, as IntervalSet syntax. ASCII, 1-based index
    const char CsvOptionProperty_readFilters[]               = "readFilters"; // Newline (\n) separated list of extended StringMatchDefinitions, UTF-8
                                                                              // Read filters can be both OR'ed and AND'ed; all AND-groups are OR'ed, i.e. if any AND-group
                                                                              // evaluates true, filter is considered matching. By default all filters are AND'ed, see and_group -property.
                                                                              //
                                                                              // In addition to properties defined for StringMatchDefinitions, the following one can be used:
                                                                              //     -apply_rows: defines rows on which filter is applied; on non-applied rows filter is interpreted matching.
                                                                              //                  Note that by default row 0, which is typically header, is not included in the filter.
                                                                              //     -apply_columns: Like apply_rows, but for colums.
                                                                              //     -and_group: defines ID for filter AND-group
                                                                              //                  For example if there are 4 filters A, B, C, D and desired logics is
                                                                              //                  (A && B) || (C && D)
                                                                              //                  Filters A and B should be in the same and_group that is different from and_group of C and D.
                                                                              //                  When and_group is not given for any filter, they belong to the same AND-group so the
                                                                              //                  logics is A && B && C && D.
    const char CsvOptionProperty_chartControls[]            = "chartControls";   // Defines chartControls to use.
    const char CsvOptionProperty_chartPanelWidth[]          = "chartPanelWidth"; // Chart panel width to use with the associated document; see TableEditor_chartPanelWidth for format documentation.
    const char CsvOptionProperty_windowHeight[]             = "windowHeight";    // Window height to request for use with the associated document; see TableEditor_chartPanelWidth for format documentation.
    const char CsvOptionProperty_windowWidth[]              = "windowWidth";     // Window width to request for use with the associated document; see TableEditor_chartPanelWidth for format documentation.
    const char CsvOptionProperty_windowPosX[]               = "windowPosX";      // Window x position to request for use with the associated document.
    const char CsvOptionProperty_windowPosY[]               = "windowPosY";      // Window y position to request for use with the associated document.
    const char CsvOptionProperty_sqlQuery[]                 = "sqlQuery";        // Defines SQL-query to use when opening a SQLite file.
    const char CsvOptionProperty_editMode[]                 = "editMode";        // Defines edit mode (e.g. read-only) for document.
    const char CsvOptionProperty_selectionDetails[]         = "selectionDetails";// Defines list of selection details to show in TableEditor
    const char CsvOptionProperty_weekDayNames[]             = "weekDayNames";    // Defines weekday names starting from Monday as comma-separated list.
    const char CsvOptionProperty_timeFormat[]               = "timeFormat";      // Defines default time format.
    const char CsvOptionProperty_dateFormat[]               = "dateFormat";      // Defines default date format.
    const char CsvOptionProperty_dateTimeFormat[]           = "dateTimeFormat";  // Defines default dateTime format.

    namespace DFG_DETAIL_NS
    {
        class HighlightDefinition
        {
        public:
            HighlightDefinition(const QString id,
                                const int col,
                                StringMatchDefinition matcher,
                                QBrush highlightBrush = QBrush(Qt::green, Qt::SolidPattern)
                                )
                : m_id(id)
                , m_nColumn(col)
                , m_matcher(matcher)
                , m_highlightBrush(highlightBrush)
            {}

            bool hasMatchString() const
            {
                return m_matcher.hasMatchString();
            }

            QVariant data(const QAbstractItemModel& model, const QModelIndex& index, const int role) const;

            const StringMatchDefinition& matcher() const { return m_matcher; }

            QString m_id;
            int m_nColumn;
            StringMatchDefinition m_matcher;
            QBrush m_highlightBrush;
        }; // class HighlightDefinition

    } // detail-namespace

    class CsvItemModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        static constexpr ::DFG_MODULE_NS(io)::TextEncoding internalEncoding() { return ::DFG_MODULE_NS(io)::encodingUTF8; }

        static const QString s_sEmpty;
        typedef QAbstractTableModel BaseClass;
        typedef int32 Index;
        typedef int64 LinearIndex; // LinearIndex guarantees that LinearIndex(rowCount()) * LinearIndex(columnCount()) does not overflow.
        typedef std::ostream StreamT;
        typedef DFG_MODULE_NS(cont)::TableSz<char, Index, DFG_MODULE_NS(io)::encodingUTF8> RawDataTable;
        typedef DFG_MODULE_NS(cont)::TableCsv<char, Index, DFG_MODULE_NS(io)::encodingUTF8> DataTable;
        typedef DFG_MODULE_NS(cont)::SortedSequence<std::vector<Index>> IndexSet;
        typedef DFG_DETAIL_NS::HighlightDefinition HighlightDefinition;
        typedef DFG_MODULE_NS(qt)::StringMatchDefinition StringMatchDefinition;

        enum ColType
        {
            ColTypeText,
            ColTypeNumber,
            ColTypeDate
        };
        enum CompleterType
        {
            CompleterTypeNone,
            CompleterTypeTexts,
            CompleterTypeTextsListItems
        };

        enum FindDirection
        {
            FindDirectionForward,
            FindDirectionBackward
        };

        enum FindAdvanceStyle
        {
            FindAdvanceStyleLinear,
            FindAdvanceStyleRowIncrement
        };

        class ColInfo
        {
        public:
            struct CompleterDeleter
            {
                void operator()(QCompleter* ptr) const;
            };

            ColInfo(CsvItemModel* pModel, QString sName = QString(), ColType type = ColTypeText, CompleterType complType = CompleterTypeNone);

            bool hasCompleter() const { return m_spCompleter.get() != nullptr; }

            Index index() const;

            QString name() const;

            QVariant getProperty(const uintptr_t& contextId, const StringViewUtf8& svPropertyId, const QVariant& defaultVal = QVariant()) const;

            // Returns true iff property didn't exist or it's value was changed.
            bool setProperty(const uintptr_t& contextId, const StringViewUtf8& svPropertyId, const QVariant& value);

            QPointer<CsvItemModel> m_spModel;
            QString m_name;
            ColType m_type;
            CompleterType m_completerType;
            std::unique_ptr<QCompleter, CompleterDeleter> m_spCompleter;

            DFG_OPAQUE_PTR_DECLARE();
        }; // class ColInfo

        class IoOperationProgressController
        {
        public:
            using ProgressCallback = std::function<bool(uint64)>; // Callback shall return true to continue reading, false to terminate.

            class CopyableAtomicBool : public std::atomic_bool
            {
            public:
                using BaseClass = std::atomic_bool;
                CopyableAtomicBool() : BaseClass(false) {}
                CopyableAtomicBool(const CopyableAtomicBool& other)
                    : BaseClass(false)
                {
                    *this = other;
                }
                CopyableAtomicBool& operator=(const CopyableAtomicBool& other)
                {
                    this->store(other.load());
                    return *this;
                }
            };

            IoOperationProgressController() = default;
            IoOperationProgressController(ProgressCallback callback)
                : m_callback(std::move(callback))
            {}

            bool operator()(const uint64 nProcessed)
            {
                return (m_callback) ? m_callback(nProcessed) : true;
            }

            operator bool() const
            {
                return m_callback.operator bool();
            }

            // Thread-safe
            bool isCancelled() const
            {
                return m_cancelled.load(std::memory_order_relaxed);
            }

            // Thread-safe
            void setCancelled(const bool bCancelled)
            {
                m_cancelled.store(bCancelled);
            }

            bool isTimeToUpdateProgress(const size_t nRow, const size_t nCol) const
            {
                DFG_UNUSED(nCol);
#if defined(_MSC_VER) && defined(_DEBUG)
                return (nRow % 1024) == 0; // Some adjustment for debug builds on MSVC
#else
                return (nRow % 8192) == 0; // This is a pretty coarse condition: compile-time constant not taking things like column count into account.
#endif
            }

            ProgressCallback m_callback;
            CopyableAtomicBool m_cancelled;
        }; // IoOperationProgressController

        class CommonOptionsBase : public CsvFormatDefinition
        {
        public:
            using BaseClass = CsvFormatDefinition;
            using BaseClass::BaseClass; // Inheriting constructor
            using ProgressController = IoOperationProgressController;
            CommonOptionsBase(const CsvFormatDefinition& cfd) : BaseClass(cfd) {}
            CommonOptionsBase(CsvFormatDefinition&& cfd) : BaseClass(std::move(cfd)) {}

            void setProgressController(ProgressController controller)
            {
                m_progressController = std::move(controller);
            }

            ProgressController m_progressController;

        }; // Class CommonOptionsBase

        class SaveOptions : public CommonOptionsBase
        {
        public:
            using BaseClass = CommonOptionsBase;
            DFG_BASE_CONSTRUCTOR_DELEGATE_1(SaveOptions, BaseClass) {}
            SaveOptions(CsvItemModel* itemModel);
            SaveOptions(const CsvItemModel* itemModel);

        private:
            void initFromItemModelPtr(const CsvItemModel* pItemModel);
        }; // class SaveOptions

        class LoadOptions : public CommonOptionsBase
        {
        public:
            using BaseClass = CommonOptionsBase;
            DFG_BASE_CONSTRUCTOR_DELEGATE_1(LoadOptions, BaseClass) {}
            LoadOptions() : CommonOptionsBase(::DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharAutoDetect,
                                                                '"',
                                                                DFG_MODULE_NS(io)::EndOfLineTypeN, 
                                                                DFG_MODULE_NS(io)::encodingUnknown)
            {}

            bool isFilteredRead() const
            {
                return isFilteredRead(getProperty(CsvOptionProperty_includeRows, ""),
                                      getProperty(CsvOptionProperty_includeColumns, ""),
                                      getProperty(CsvOptionProperty_readFilters, ""));
            }

            bool isFilteredRead(const std::string& sIncludeRows, const std::string& sIncludeColumns, const std::string& sFilterItems) const
            {
                return !sIncludeRows.empty() || !sIncludeColumns.empty() || !sFilterItems.empty();
            }

        }; // class LoadOptions

        // Maps valid internal row index [0, rowCount[ to user seen indexing, usually 1-based indexing.
        static constexpr int internalRowToVisibleShift()        { return 1; }
        static constexpr int internalColumnToVisibleShift()     { return 1; }
        static int internalRowIndexToVisible(const int nRow)    { return saturateAdd<int>(nRow,  internalRowToVisibleShift()); }
        static int visibleRowIndexToInternal(const int nRow)    { return saturateAdd<int>(nRow, -internalRowToVisibleShift()); }
        static int internalColumnIndexToVisible(const int nCol) { return saturateAdd<int>(nCol,  internalColumnToVisibleShift()); }
        static int visibleColumnIndexToInternal(const int nCol) { return saturateAdd<int>(nCol, -internalColumnToVisibleShift()); }

        DFG_CLASS_NAME(CsvItemModel)();
        ~DFG_CLASS_NAME(CsvItemModel)();

        // Calls saveToFile(m_sFilePath)
        bool saveToFile();

        // Tries to save csv-data to given path. If successful, sets path and resets modified flag.
        // [return] : Returns true iff. save is successful.
        bool saveToFile(const QString& sPath);
        bool saveToFile(const QString& sPath, const SaveOptions& options);

        bool exportAsSQLiteFile(const QString& sPath);
        bool exportAsSQLiteFile(const QString& sPath, const SaveOptions& options);

        // Saves data to stream using default SaveOptions (in typical case means UTF-8 encoding with BOM and the same control chars as what was used in read except for EOL-char).
        // Note: Stream's write()-member must write bytes untranslated (i.e. no encoding nor eol translation)
        // TODO: Make encoding to be user definable through options.
        bool save(StreamT& strm);
        bool save(StreamT& strm, const SaveOptions& options);

        void setUndoStack(QUndoStack* pStack);

    public:

        bool openNewTable();
        bool mergeAnotherTableToThis(const DFG_CLASS_NAME(CsvItemModel)& other);
        bool openFile(const QString& sDbFilePath);
        bool openFromSqlite(const QString& sDbFilePath, const QString& sQuery);
        bool openFromSqlite(const QString& sDbFilePath, const QString& sQuery, LoadOptions& loadOptions);
        bool openFile(QString sDbFilePath, LoadOptions loadOptions);
        bool importFiles(const QStringList& paths);
        bool openStream(QTextStream& strm);
        bool openStream(QTextStream& strm, const LoadOptions& loadOptions);
        bool openString(const QString& str);
        bool openString(const QString& str, const LoadOptions& loadOptions);
        bool openFromMemory(const char* data, const size_t nSize, LoadOptions loadOptions);

        // Implementation level function.
        // 1. Clears existing data and prepares model for table changes.
        // 2. Calls actual table filling implementation.
        // 3. Handles model-related finalization.
        bool readData(const LoadOptions& options, std::function<bool()> tableFiller);

        bool isModified() const { return m_bModified; }
        void setModifiedStatus(const bool bMod = true);

        // Returns index of column with name 'sHeaderName' or 'returnValueIfNotFound'.
        int findColumnIndexByName(const QString& sHeaderName, const int returnValueIfNotFound) const;

        int getColumnCount() const { return int(m_vecColInfo.size()); }
        int getRowCount() const { return m_nRowCount; }
        bool isValidRow(int r) const { return r >= 0 && r < getRowCount(); }
        bool isValidColumn(int c) const { return c >= 0 && c < getColumnCount(); }

        int getRowCountUpperBound() const; // Returns upper bound for row count (note that effective maximum can be much less)
        int getColumnCountUpperBound() const; // Returns upper bound for column count (note that effective maximum can be much less)

        QStringList getColumnNames() const;
        const QString& getHeaderName(const int nCol) const { return (isValidColumn(nCol)) ? m_vecColInfo[nCol].m_name : s_sEmpty; }

        ColType getColType(const int nCol) const { return (isValidColumn(nCol) ? m_vecColInfo[nCol].m_type : ColTypeText); }
        CompleterType getColCompleterType(const int nCol) const { return (isValidColumn(nCol) ? m_vecColInfo[nCol].m_completerType : CompleterTypeNone); }

        ColInfo*       getColInfo(const int nCol)       { return isValidColumn(nCol) ? &m_vecColInfo[nCol] : nullptr; }
        const ColInfo* getColInfo(const int nCol) const { return isValidColumn(nCol) ? &m_vecColInfo[nCol] : nullptr; }
        void forEachColInfoWhile(std::function<bool(ColInfo&)>);
        void forEachColInfoWhile(std::function<bool(const ColInfo&)>) const;

        SaveOptions getSaveOptions() const;

        // Appends data model row to string.
        void rowToString(const int nRow, QString& str, const QChar cDelim, const IndexSet* pSetIgnoreColumns = nullptr) const;

        std::shared_ptr<QReadWriteLock> getReadWriteLock();
        LockReleaser tryLockForEdit();
        LockReleaser tryLockForRead() const;

        QString rowToString(const int nRow, const QChar cDelim = '\t') const
        {
            QString str;
            rowToString(nRow, str, cDelim);
            return str;
        }

        // Populates @p vecStrings with strings from given column. If nCol is not valid,
        // @p vecStrings will be empty.
        void columnToStrings(const int nCol, std::vector<QString>& vecStrings);

        SzPtrUtf8R RawStringPtrAt(const int nRow, const int nCol) const;
        StringViewUtf8 rawStringViewAt(const int nRow, const int nCol) const;
        StringViewUtf8 rawStringViewAt(const QModelIndex& index) const;

        // Sets cell strings in column @p nCol to those given in @p vecStrings.
        void setColumnCells(const int nCol, const std::vector<QString>& vecStrings);

        void setColumnType(const int nCol, const ColType colType);

        // Tokenizes properly formatted text line and sets the data
        // as cells of given row @p nRow.
        void setRow(const int nRow, QString sLine);

        // Returns true if item was set, false otherwise.
        bool setItem(const int nRow, const int nCol, const QString& str);
        bool setItem(const int nRow, const int nCol, const StringViewUtf8& sv);

        // Sets data of given index without triggering undo.
        // Note: Given model index must be valid; this function does not check it.
        void setDataNoUndo(const QModelIndex& index, const QString& str);
        void setDataNoUndo(const QModelIndex& index, SzPtrUtf8R pszU8);
        void setDataNoUndo(const int nRow, const int nCol, const QString& str);
        void setDataNoUndo(const int nRow, const int nCol, SzPtrUtf8R pszU8);

        // Sets data to table as follows:
        //      -Checks if target indexes are valid, returns false if not.
        //      -Checks if existing item is identical and returns false if identical.
        //      -Does NOT handle setting modified.
        //      -Does NOT signal dataChanged()
        // Note: Caller must handle dataChanged signaling and setting modified.
        // Return: true if value was set, false otherwise.
        bool privSetDataToTable(int nRow, int nCol, SzPtrUtf8R pszU8);

        void setColumnName(const int nCol, const QString& sName);

        template <class Cont_T>
        void removeRows(const Cont_T& indexesAscSorted) { removeRows(::DFG_MODULE_NS(cont)::ArrayWrapper::createArrayWrapper(indexesAscSorted)); }

        void removeRows(const ::DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ArrayWrapperT)<const int>& indexesAscSorted);

        bool transpose();

        static QString& dataCellToString(const QString& sSrc, QString& sDst, const QChar cDelim);

        const QString& getFilePath() const
        { 
            return m_sFilePath; 
        }

        QString getTableTitle(const QString& sDefault = QString()) const;

        void setFilePathWithoutSignalEmit(QString);
        void setFilePathWithSignalEmit(QString);

        void initCompletionFeature();

        float latestReadTimeInSeconds()  const { return m_readTimeInSeconds; }
        float latestWriteTimeInSeconds() const { return m_writeTimeInSeconds; }

        static LoadOptions getLoadOptionsForFile(const QString& sFilePath);
        LoadOptions getLoadOptionsFromConfFile() const; // Shortcut for CsvItemModel::getLoadOptionsForFile(this->getFilePath())
        LoadOptions getOpenTimeLoadOptions() const { return m_loadOptionsInOpen; } // Returns LoadOptions that were used when opened from file or memory.

        void populateConfig(DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig)& config) const;

        // Gives internal table to given function object for arbitrary edits and handles model specific tasks such as setting modified.
        // Note: Does not check whether the table has actually changed and always sets the model modified.
        // Note: This resets model (for details, see QAbstractItemModel::beginResetModel()). This means that e.g. view selection is lost when calling this function.
        //       If caller wishes to have the same selection before and after this call, it must be handled manually.
        template <class Func_T> void batchEditNoUndo(Func_T func);

        // Copies all elements from 'table' to this by (row, column) indexes and bypasses undo.
        // If pFill is given, data written to each (row, column) will be pFill instead of table(row, column)
        void setDataByBatch_noUndo(const RawDataTable& table, const SzPtrUtf8R pFill = SzPtrUtf8R(nullptr));

        void setHighlighter(HighlightDefinition hld);

        // Finds next match.
        QModelIndex findNextHighlighterMatch(QModelIndex seedIndex, // Seed which is advanced before doing first actual match.
                                             FindDirection direction = FindDirectionForward);

        QModelIndex nextCellByFinderAdvance(const QModelIndex& seedIndex, const FindDirection direction, const FindAdvanceStyle advanceStyle) const;
        void nextCellByFinderAdvance(int& r, int& c, const FindDirection direction, const FindAdvanceStyle advanceStyle) const;

        LinearIndex wrappedDistance(const QModelIndex& from, const QModelIndex& to, const FindDirection direction) const;

        // Returns estimate for resulting file size if content is written to file.
        uint64 getOutputFileSizeEstimate() const;

        bool isSupportedEncodingForSaving(DFG_MODULE_NS(io)::TextEncoding encoding) const;

        static void setCompleterHandlingFromInputSize(LoadOptions& loadOptions, const uint64 nSizeInBytes, const CsvItemModel* pModel);
        void setCompleterHandlingFromInputSize(LoadOptions& loadOptions, const uint64 nSizeInBytes) const { setCompleterHandlingFromInputSize(loadOptions, nSizeInBytes, this); }

        // Convenience function for setting table size. If count is negative, corresponding dimension is not changed.
        // Returns true iff size was changed.
        bool setSize(Index nRowCount, Index nColCount);

        // Model Overloads
        int rowCount(const QModelIndex & parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        // Note: for direct access to data, use rawStringViewAt().
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        // Note: for setting data without undo, use setDataNoUndo()
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        bool insertRows(int position, int rows, const QModelIndex& parent = QModelIndex()) override;
        bool removeRows(int position, int rows, const QModelIndex& parent = QModelIndex()) override;
        bool insertColumns(int position, int columns, const QModelIndex& parent = QModelIndex()) override;
        bool removeColumns(int position, int columns, const QModelIndex& parent = QModelIndex()) override;
        QModelIndexList match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const override;
#if DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS
        QStringList mimeTypes() const;
        QMimeData* mimeData(const QModelIndexList& indexes) const;
        bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
#endif

              DataTable& table()       { return m_table; }
        const DataTable& table() const { return m_table; }

    signals:
        void sigModifiedStatusChanged(bool bNewStatus);
        void sigOnNewSourceOpened();
        void sigSourcePathChanged();
        void sigOnSaveToFileCompleted(bool, double);

    protected:
        // Clears internal data. Caller should make sure this call
        // is done within appropriate resetmodel-calls.
        void clear();

        void insertColumnsImpl(int position, int count);

    private:
        template <class Stream_T>
        bool saveImpl(Stream_T& strm, const SaveOptions& options);

        template <class OutFile_T, class Stream_T>
        bool saveToFileImpl(const QString& sPath, OutFile_T& outFile, Stream_T& strm, const SaveOptions& options);

        bool readDataFromSqlite(const QString& sDbFilePath, const QString& sQuery, LoadOptions& loadOptions);

        template <class This_T, class Func_T>
        static void forEachColInfoWhileImpl(This_T& rThis, Func_T&& func);

    public:
        QUndoStack* m_pUndoStack;
        DataTable m_table;

    //private:

        std::vector<ColInfo> m_vecColInfo;
        int m_nRowCount;
        QString m_sFilePath;
        QString m_sTitle;
        bool m_bModified;
        bool m_bResetting;
        float m_readTimeInSeconds;
        float m_writeTimeInSeconds;
        std::vector<HighlightDefinition> m_highlighters;
        LoadOptions m_loadOptionsInOpen; // Stores file options that were used when opening.
        QStringList m_messagesFromLatestOpen;
        QStringList m_messagesFromLatestSave;
        DFG_OPAQUE_PTR_DECLARE();
    }; // class CsvItemModel

    template <class Func_T> void CsvItemModel::batchEditNoUndo(Func_T func)
    {
        beginResetModel(); // This might be a bit coarse for smaller edits.
        m_bResetting = true;
        func(table());
        endResetModel();
        m_bResetting = false;
        setModifiedStatus(true);
    }

    // Extends TableStringMatchDefinition providing CsvItemModel-specific table row/column index shifts.
    class CsvItemModelStringMatcher : public TableStringMatchDefinition
    {
    public:
        using BaseClass = TableStringMatchDefinition;

        CsvItemModelStringMatcher(BaseClass&& base) :
            BaseClass(std::move(base))
        {}

        static std::pair<CsvItemModelStringMatcher, std::string> fromJson(const StringViewUtf8& sv)
        {
            return BaseClass::fromJson(sv, 0, -CsvItemModel::internalColumnToVisibleShift());
        }

        bool isApplyColumn(const int nCol) const
        {
            return m_columns.hasValue(nCol);
        }
    }; // CsvItemModelStringMatcher

} } // module namespace
