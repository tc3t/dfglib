#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "qtBasic.hpp"
#include "../io/textEncodingTypes.hpp"
#include "StringMatchDefinition.hpp"
#include "../build/languageFeatureInfo.hpp"
#include "../cont/arrayWrapper.hpp"
#include "../OpaquePtr.hpp"
#include "containerUtils.hpp"
#include "../numericTypeTools.hpp"
#include "../CsvFormatDefinition.hpp"
#include "../cont/detail/tableCsvHelpers.hpp"
#include "../cont/SortedSequence.hpp"
#include "../Span.hpp"

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
    class CsvConfig;
} }

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io)
{
    class OfStreamWithEncoding;
} }

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(charts)
{
    class ChartDataType;
} } // module charts

class QUndoStack;
class QCompleter;
class QFile;
class QTextStream;
class QReadWriteLock;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

#ifndef DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS
    // Note: item-level drag & drop probably requires changes also to CsvTableView, #131 introduced file drag & drop solely in CsvTableView.
    #define DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS  0
#endif

    const char CsvOptionProperty_completerColumns[]          = "completerColumns";
    const char CsvOptionProperty_completerEnabledSizeLimit[] = "completerEnabledSizeLimit";
    const char CsvOptionProperty_includeRows[]               = "includeRows"; // To define rows to read from file, as IntervalSet syntax. ASCII, 0-based index
    const char CsvOptionProperty_includeColumns[]            = "includeColumns"; // To define columns to read from file, as IntervalSet syntax. ASCII, 1-based index
    const char CsvOptionProperty_initialScrollPosition[]     = "initialScrollPosition"; // Defines initial scroll position for opened document.
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
    const char CsvOptionProperty_readThreadBlockSizeMinimum[] = "readThreadBlockSizeMinimum"; // Sets TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum
    const char CsvOptionProperty_readThreadCountMaximum[]   = "readThreadCountMaximum"; // Sets TableCsvReadWriteOptions::PropertyId::readOpt_threadCount
    const char CsvOptionProperty_windowHeight[]             = "windowHeight";    // Window height to request for use with the associated document, ignored if windowMaximized is true; see TableEditor_chartPanelWidth for format documentation.
    const char CsvOptionProperty_windowWidth[]              = "windowWidth";     // Window width to request for use with the associated document, ignored if windowMaximized is true; see TableEditor_chartPanelWidth for format documentation.
    const char CsvOptionProperty_windowPosX[]               = "windowPosX";      // Window x position to request for use with the associated document, ignored if windowMaximized is true.
    const char CsvOptionProperty_windowPosY[]               = "windowPosY";      // Window y position to request for use with the associated document, ignored if windowMaximized is true.
    const char CsvOptionProperty_windowMaximized[]          = "windowMaximized"; // Defines whether window should be maximized.
    const char CsvOptionProperty_sqlQuery[]                 = "sqlQuery";        // Defines SQL-query to use when opening a SQLite file.
    const char CsvOptionProperty_editMode[]                 = "editMode";        // Defines edit mode (e.g. read-only) for document.
    const char CsvOptionProperty_selectionDetails[]         = "selectionDetails";// Defines list of selection details to show in TableEditor
    const char CsvOptionProperty_selectionDetailsPrecision[]= "selectionDetailsResultPrecision"; // Defines numerical precision of selection detail results shown in TableEditor
    const char CsvOptionProperty_weekDayNames[]             = "weekDayNames";    // Defines weekday names starting from Monday as comma-separated list.
    const char CsvOptionProperty_timeFormat[]               = "timeFormat";      // Defines default time format.
    const char CsvOptionProperty_dateFormat[]               = "dateFormat";      // Defines default date format.
    const char CsvOptionProperty_dateTimeFormat[]           = "dateTimeFormat";  // Defines default dateTime format.
    const char CsvOptionProperty_columnSortingEnabled[]       = "columnSortingEnabled";       // Defines whether columns can be sorted from UI (0/1, default is 0)
    const char CsvOptionProperty_columnSortingCaseSensitive[] = "columnSortingCaseSensitive"; // Defines whether column sorting is case-sensitive (0/1, default is 0)
    const char CsvOptionProperty_columnSortingColumnIndex[]   = "columnSortingColumnIndex";   // Defines which column is sorted (1-based column index)
    const char CsvOptionProperty_columnSortingOrder[]         = "columnSortingOrder";         // Defines sort order for column defined by columnSortingColumnIndex (A or D, default is A)

    const char CsvOptionProperty_findText[]          = "findText";          // Defines find-text for find panel
    const char CsvOptionProperty_findCaseSensitive[] = "findCaseSensitive"; // Defines whether find is case-sensitive
    const char CsvOptionProperty_findSyntaxType[]    = "findSyntaxType";    // Defines find syntax type, for recognized values, see PatternMatcher::patternSyntaxName_untranslated
    const char CsvOptionProperty_findColumn[]        = "findColumn";        // Defines find-column index (1-based column index, value 0 is interpreted as "any")

    const char CsvOptionProperty_filterText[]          = "filterText";          // Defines filter-text for filter panel
    const char CsvOptionProperty_filterCaseSensitive[] = "filterCaseSensitive"; // Defines whether filter is case-sensitive
    const char CsvOptionProperty_filterSyntaxType[]    = "filterSyntaxType";    // Defines filter syntax type, for recognized values, see PatternMatcher::patternSyntaxName_untranslated
    const char CsvOptionProperty_filterColumn[]        = "filterColumn";        // Defines filter-column index (1-based column index, value 0 is interpreted as "any")

    namespace DFG_DETAIL_NS
    {
        class HighlightDefinition
        {
        public:
            HighlightDefinition(const QString id,
                                const int row,
                                const int col,
                                StringMatchDefinition matcher,
                                QBrush highlightBrush = QBrush(Qt::green, Qt::SolidPattern)
                                )
                : m_id(id)
                , m_nRow(row)
                , m_nColumn(col)
                , m_matcher(matcher)
                , m_highlightBrush(highlightBrush)
            {}

            HighlightDefinition(const QString id,
                const int col,
                StringMatchDefinition matcher,
                QBrush highlightBrush = QBrush(Qt::green, Qt::SolidPattern))
                : HighlightDefinition(id, -1, col, matcher, highlightBrush)
            {}

            bool hasMatchString() const
            {
                return m_matcher.hasMatchString();
            }

            QVariant data(const QAbstractItemModel& model, const QModelIndex& index, const int role) const;

            const StringMatchDefinition& matcher() const { return m_matcher; }

            QString m_id;
            int m_nRow;
            int m_nColumn;
            StringMatchDefinition m_matcher;
            QBrush m_highlightBrush;
        }; // class HighlightDefinition

        // Provides public interface for internal table implementation.
        class CsvItemModelTable
        {
        public:
            using Index = int32;
            using LinearIndex = uint64;

            CsvItemModelTable();
            ~CsvItemModelTable();

            SzPtrUtf8R operator()(Index nRow, Index nCol) const;
            StringViewUtf8 viewAt(Index nRow, Index nCol) const;
            void setElement(Index nRow, Index nCol, StringViewUtf8);

            void forEachFwdRowInColumnWhile(Index nCol, std::function<bool (Index)> whileFunc, std::function<void (Index, SzPtrUtf8R)> func) const;

            void forEachFwdRowInColumn(Index nCol, std::function<void (Index, SzPtrUtf8R)> func) const;

            Index rowCountByMaxRowIndex() const;

            Index colCountByMaxColIndex() const;

            LinearIndex cellCountNonEmpty() const;

            class TableRef;

                  TableRef& impl();
            const TableRef& impl() const;

            DFG_OPAQUE_PTR_DECLARE();
        }; // class CsvItemModelTable

    } // detail-namespace

    enum class CsvItemModelColumnProperty
    {
        readOnly // Whether cells in column or column name can be edited. Note that this does not make column completely immutable: for example can remove rows or even completely remove the column.
    };

    enum class CsvItemModelColumnType
    {
                   //    Sorting                      Double conversion
                   //    -------------------------- | -----------------
        text,      //    Text comparison            | Built-in or custom
        number,    //    Double comparison          | Built-in or custom
        datetime   //    Double comparison          | Built-in or custom (typically QDateTime parser with user-provided format)
    }; // enum class CsvItemModelColumnType

    class CsvItemModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        static constexpr ::DFG_MODULE_NS(io)::TextEncoding internalEncoding() { return ::DFG_MODULE_NS(io)::encodingUTF8; }

        static const QString s_sEmpty;
        typedef QAbstractTableModel BaseClass;
        typedef int32 Index;       // Index type for both rows and columns
        typedef int64 LinearIndex; // LinearIndex guarantees that LinearIndex(rowCount()) * LinearIndex(columnCount()) does not overflow.
        typedef uint64 IndexPairInteger; // Integer type that can hold (row, column) pair as integer.
        typedef std::ostream StreamT;
        typedef DFG_DETAIL_NS::CsvItemModelTable RawDataTable;
        typedef RawDataTable DataTable;
        typedef DFG_MODULE_NS(cont)::SortedSequence<std::vector<Index>> IndexSet;
        typedef DFG_DETAIL_NS::HighlightDefinition HighlightDefinition;
        typedef DFG_MODULE_NS(qt)::StringMatchDefinition StringMatchDefinition;
        class OpaqueTypeDefs;

        using ColType = CsvItemModelColumnType;
        // For compatibility, don't use these in new code.
        static constexpr auto ColTypeText = ColType::text;
        static constexpr auto ColTypeNumber = ColType::number;

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

        using DataTableRef      =       DFG_DETAIL_NS::CsvItemModelTable::TableRef&;
        using DataTableConstRef = const DFG_DETAIL_NS::CsvItemModelTable::TableRef&;

        class ColInfo
        {
        public:
            class StringToDoubleParserParam
            {
            public:
                using ChartDataType = ::DFG_MODULE_NS(charts)::ChartDataType;
                StringToDoubleParserParam(const StringViewSzUtf8& sv, ChartDataType* pInterpretedChartType, const double returnValueOnConversionError)
                    : m_sv(sv)
                    , m_pInterpretedChartType(pInterpretedChartType)
                    , m_returnValueOnConversionError(returnValueOnConversionError)
                {}

                StringViewSzUtf8 view() const { return m_sv; }
                // Returns true iff chart data type was set.
                bool setInterpretedChartType(const ChartDataType& dt);

                double conversionErrorReturnValue() const { return m_returnValueOnConversionError; }

            private:
                StringViewSzUtf8 m_sv;
                ::DFG_MODULE_NS(charts)::ChartDataType* m_pInterpretedChartType = nullptr;
                double m_returnValueOnConversionError = std::numeric_limits<double>::quiet_NaN();
            }; // class StringToDoubleParserParam

            class StringToDoubleParser
            {
            public:
                using ParserParam = ColInfo::StringToDoubleParserParam;
                using ParserFunc = std::function<double(ParserParam&, const QString&)>;

                StringToDoubleParser() = default;
                StringToDoubleParser(std::nullptr_t) {}
                StringToDoubleParser(QString sDefitinionString, ParserFunc parserFunc)
                    : m_parserFunc(parserFunc)
                    , m_sDefString(std::move(sDefitinionString))
                {}

                static StringToDoubleParser createQDateTimeParser(QString sFormat)
                {
                    return StringToDoubleParser(std::move(sFormat), &StringToDoubleParser::qDateTimeConverter);
                }

                explicit operator bool() const
                {
                    return m_parserFunc.operator bool();
                }

                QString getDefinitionString() const { return m_sDefString; }

                double operator()(ParserParam param) const
                {
                    return (m_parserFunc) ? m_parserFunc(param, m_sDefString) : std::numeric_limits<double>::quiet_NaN();
                }

                static double qDateTimeConverter(ParserParam& param, const QString& sDefString);

            private:
                ParserFunc m_parserFunc;
                QString m_sDefString;
            }; // class StringToDoubleParser

            struct CompleterDeleter
            {
                void operator()(QCompleter* ptr) const;
            };

            ColInfo(CsvItemModel* pModel, QString sName = QString(), ColType type = ColType::text, CompleterType complType = CompleterTypeNone);

            bool hasCompleter() const { return m_spCompleter.get() != nullptr; }

            StringViewUtf8 columnTypeAsString() const;
            static StringViewUtf8 columnTypeAsString(ColType colType);

            Index index() const;

            QString name() const;

            // If this column has custom string-to-double parser, returns functor that does the conversion, otherwise empty functor.
            StringToDoubleParser getCustomStringToDoubleParser() const;

            // Returns a column property.
            QVariant getProperty(const CsvItemModelColumnProperty propertyId, const QVariant& defaultVal = QVariant()) const;

            // Returns a property in given context. For example if the same CsvItemModel is used in different views, properties such as column visibility
            // are not properties of the column itself, but properties of views: column could be shown in one, and be hidden in the other.
            // ColInfo provides way to store these context-dependent properties to ColInfo object so that views don't need to implement storage.
            QVariant getProperty(const uintptr_t& contextId, const StringViewUtf8& svPropertyId, const QVariant& defaultVal = QVariant()) const;

            // Returns true iff property didn't exist or it's value was changed.
            bool setProperty(const CsvItemModelColumnProperty& columnProperty, const QVariant& value);
            bool setProperty(const uintptr_t& contextId, const StringViewUtf8& svPropertyId, const QVariant& value);

        private:
            template <class Cont_T, class Key_T, class ToStorageKey_T>
            bool setPropertyImpl(Cont_T& cont, const Key_T& key, const QVariant& value, ToStorageKey_T);

            template <class Cont_T, class Key_T>
            bool setPropertyImpl(Cont_T& cont, const Key_T& key, const QVariant& value);

        public:
            QPointer<CsvItemModel> m_spModel;
            QString m_name;
            ColType m_type;
            CompleterType m_completerType;
            StringToDoubleParser m_customStringToDoubleParser;
            std::unique_ptr<QCompleter, CompleterDeleter> m_spCompleter;

            DFG_OPAQUE_PTR_DECLARE();
        }; // class ColInfo

        class IoOperationProgressController
        {
        public:
            using ProcessCounterT = uint64;

            using ProgressCallbackReturnT = bool;

            class ProgressCallbackParamT
            {
            public:
                ProgressCallbackParamT(ProcessCounterT nProcessed, const uint32 nThreadCount = 1)
                    : m_nCounter(nProcessed)
                    , m_nThreadCount(nThreadCount)
                {}

                ProcessCounterT counter() const { return m_nCounter; }
                uint32 threadCount() const      { return m_nThreadCount; }

                ProcessCounterT m_nCounter = 0;
                uint32 m_nThreadCount = 1;
            }; // class ProgressCallbackParam

            using ProgressCallback = std::function<ProgressCallbackReturnT (ProgressCallbackParamT)>; // Callback shall return true to continue reading, false to terminate.

            template <class T>
            class CopyableAtomicT : public std::atomic<T>
            {
            public:
                using BaseClass = std::atomic<T>;
                CopyableAtomicT(const T a = T()) : BaseClass(a) {}
                CopyableAtomicT(const CopyableAtomicT& other)
                    : BaseClass(false)
                {
                    *this = other;
                }
                CopyableAtomicT& operator=(const CopyableAtomicT& other)
                {
                    this->store(other.load());
                    return *this;
                }
            };

            IoOperationProgressController() = default;
            IoOperationProgressController(ProgressCallback callback)
                : m_callback(std::move(callback))
            {}

            ProgressCallbackReturnT operator()(const ProgressCallbackParamT param)
            {
                return (m_callback) ? m_callback(param) : true;
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
#if defined(_MSC_VER) && defined(DFG_BUILD_TYPE_DEBUG)
                return (nRow % 1024) == 0; // Some adjustment for debug builds on MSVC
#else
                return (nRow % 8192) == 0; // This is a pretty coarse condition: compile-time constant not taking things like column count into account.
#endif
            }

            // Increments thread count being used in this operation, returns old value + 1.
            // Thread-safe
            uint32 incrementOperationThreadCount()
            {
                auto nOldValue = m_anOperationThreadCount.fetch_add(1u);
                return nOldValue + 1u;
            }

            uint32 getCurrentOperationThreadCount() const
            {
                return m_anOperationThreadCount.load(std::memory_order_relaxed);
            }

            ProgressCallback m_callback;
            CopyableAtomicT<bool> m_cancelled{false};
            CopyableAtomicT<uint32> m_anOperationThreadCount{ 1 };
        }; // IoOperationProgressController

        class CommonOptionsBase : public ::DFG_MODULE_NS(cont)::TableCsvReadWriteOptions
        {
        public:
            using BaseClass = ::DFG_MODULE_NS(cont)::TableCsvReadWriteOptions;
            using BaseClass::BaseClass; // Inheriting constructor
            using ProgressController = IoOperationProgressController;
            using ProgressControllerParamT = IoOperationProgressController::ProgressCallbackParamT;
            using ProgressCallbackReturnT  = IoOperationProgressController::ProgressCallbackReturnT;
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
            LoadOptions(const CsvFormatDefinition& cfd) : BaseClass(cfd) {}
            LoadOptions(CsvFormatDefinition&& cfd) : BaseClass(std::move(cfd)) {}
            LoadOptions(const CsvItemModel* pCsvItemModel = nullptr); // If pCsvItemModel is given, some default properties are queried from it's settings

            static LoadOptions constructFromConfig(const CsvConfig& config, const CsvItemModel* pCsvItemModel);

            bool isFilteredRead() const;

            bool isFilteredRead(const std::string& sIncludeRows, const std::string& sIncludeColumns, const std::string& sFilterItems) const;

            void setDefaultProperties(const CsvItemModel* pCsvItemModel);

        }; // class LoadOptions

        // Maps valid internal row index [0, rowCount[ to user seen indexing, usually 1-based indexing.
        static constexpr int internalRowToVisibleShift()        { return 1; }
        static constexpr int internalColumnToVisibleShift()     { return 1; }
        static int internalRowIndexToVisible(const int nRow)    { return saturateAdd<int>(nRow,  internalRowToVisibleShift()); }
        static int visibleRowIndexToInternal(const int nRow)    { return saturateAdd<int>(nRow, -internalRowToVisibleShift()); }
        static int internalColumnIndexToVisible(const int nCol) { return saturateAdd<int>(nCol,  internalColumnToVisibleShift()); }
        static int visibleColumnIndexToInternal(const int nCol) { return saturateAdd<int>(nCol, -internalColumnToVisibleShift()); }

        CsvItemModel();
        ~CsvItemModel();

        // Calls saveToFile(m_sFilePath)
        bool saveToFile();

        // Tries to save csv-data to given path. If successful, sets path and resets modified flag.
        // [return] : Returns true iff. save is successful.
        bool saveToFile(const QString& sPath);
        bool saveToFile(const QString& sPath, const SaveOptions& options);

        // Returns csv-data as bytes, encoding depends on save options.
        std::string saveToByteString(const SaveOptions& options);
        std::string saveToByteString(); // Convenience overload: calls saveToByteString() with default save options as argument

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
        bool mergeAnotherTableToThis(const CsvItemModel& other);
        bool openFile(const QString& sDbFilePath);
        bool openFromSqlite(const QString& sDbFilePath, const QString& sQuery);
        bool openFromSqlite(const QString& sDbFilePath, const QString& sQuery, LoadOptions& loadOptions);
        bool openFile(QString sDbFilePath, LoadOptions loadOptions);
        bool importFiles(const QStringList& paths);
        bool openStream(QTextStream& strm);
        bool openStream(QTextStream& strm, const LoadOptions& loadOptions);
        bool openString(const QString& str);
        bool openString(const QString& str, const LoadOptions& loadOptions);
        bool openFromMemory(const Span<const char> bytes, LoadOptions loadOptions);
        bool openFromMemory(const char* data, const size_t nSize, LoadOptions loadOptions);

        // Implementation level function.
        // 1. Clears existing data and prepares model for table changes.
        // 2. Calls actual table filling implementation.
        // 3. Handles model-related finalization.
        bool readData(const LoadOptions& options, std::function<bool()> tableFiller);

        bool isModified() const { return m_bModified; }
        void setModifiedStatus(const bool bMod = true);

        // Returns index of column with name 'sHeaderName' or 'returnValueIfNotFound'.
        Index findColumnIndexByName(const QString& sHeaderName, const int returnValueIfNotFound) const;

        Index getColumnCount() const { return int(m_vecColInfo.size()); }
        Index getRowCount() const { return m_nRowCount; }
        bool isValidRow(Index r) const { return r >= 0 && r < getRowCount(); }
        bool isValidColumn(Index c) const { return c >= 0 && c < getColumnCount(); }
        bool isReadOnlyColumn(Index c) const;
        bool isCellEditable(Index r, Index c) const; // Returns true iff cell can edited, might be uneditable e.g. due to being on read-only column 
        bool isCellEditable(const QModelIndex& index); // Convenience overload equivalent to isCellEditable(index.row(), index.colum());
        
        int getRowCountUpperBound() const; // Returns upper bound for row count (note that effective maximum can be much less)
        int getColumnCountUpperBound() const; // Returns upper bound for column count (note that effective maximum can be much less)

        QStringList getColumnNames() const;
        const QString& getHeaderName(const int nCol) const { return (isValidColumn(nCol)) ? m_vecColInfo[nCol].m_name : s_sEmpty; }

        ColType getColType(const Index nCol) const { return (isValidColumn(nCol) ? m_vecColInfo[nCol].m_type : ColType::text); }
        StringViewUtf8 getColTypeAsString(const Index nCol) const { return (isValidColumn(nCol) ? m_vecColInfo[nCol].columnTypeAsString() : StringViewUtf8()); }
        CompleterType getColCompleterType(const Index nCol) const { return (isValidColumn(nCol) ? m_vecColInfo[nCol].m_completerType : CompleterTypeNone); }

        ColInfo*       getColInfo(const int nCol)       { return isValidColumn(nCol) ? &m_vecColInfo[nCol] : nullptr; }
        const ColInfo* getColInfo(const int nCol) const { return isValidColumn(nCol) ? &m_vecColInfo[nCol] : nullptr; }

        // Calls given function for each ColInfo. Function should return true to keep iterating.
        // Locking must be handled by caller and lockReleaser is taken as argument to make the requirement explicit:
        // if lockReleaser.isLocked() returns false, columns won't be iterated.
        // @return true if columns were iterated (or could have been had there been any), false otherwise.
        bool forEachColInfoWhile(const LockReleaser& lockReleaser, std::function<bool(ColInfo&)>);
        // const-overload.
        bool forEachColInfoWhile(const LockReleaser& lockReleaser, std::function<bool(const ColInfo&)>) const;

        SaveOptions getSaveOptions() const;

        // Appends data model row to string.
        void rowToString(const int nRow, QString& str, const QChar cDelim, const IndexSet* pSetIgnoreColumns = nullptr) const;

        std::shared_ptr<QReadWriteLock> getReadWriteLock();
        DFG_NODISCARD LockReleaser tryLockForEdit();
        DFG_NODISCARD LockReleaser tryLockForRead() const;

        QString rowToString(const int nRow, const QChar cDelim = '\t') const
        {
            QString str;
            rowToString(nRow, str, cDelim);
            return str;
        }

        // Populates @p vecStrings with strings from given column. If nCol is not valid,
        // @p vecStrings will be empty.
        void columnToStrings(const int nCol, std::vector<QString>& vecStrings);

        // Returns typed string pointer. Prefer to use rawStringViewAt() as first choice as returning null-terminated string pointers need assumption about underlying implementation.
        SzPtrUtf8R rawStringPtrAt(const int nRow, const int nCol) const;
        SzPtrUtf8R rawStringPtrAt(const QModelIndex& index) const;
        StringViewUtf8 rawStringViewAt(const int nRow, const int nCol) const;
        StringViewUtf8 rawStringViewAt(const QModelIndex& index) const;

        double cellDataAsDouble(const QModelIndex& modelIndex, ::DFG_MODULE_NS(charts)::ChartDataType* pInterpretedInputDataType = nullptr, double returnValueOnConversionFailure = std::numeric_limits<double>::quiet_NaN()) const;
        double cellDataAsDouble(Index nRow, Index nCol, ::DFG_MODULE_NS(charts)::ChartDataType* pInterpretedInputDataType = nullptr, double returnValueOnConversionFailure = std::numeric_limits<double>::quiet_NaN()) const;
        // Overload for case where string has already been fetched. Precondition: rawStringViewAt(nRow, nCol) == sv
        double cellDataAsDouble(StringViewSzUtf8 sv, Index nRow, Index nCol, ::DFG_MODULE_NS(charts)::ChartDataType* pInterpretedInputDataType = nullptr, double returnValueOnConversionFailure = std::numeric_limits<double>::quiet_NaN()) const;

        // Sets cell strings in column @p nCol to those given in @p vecStrings.
        void setColumnCells(const int nCol, const std::vector<QString>& vecStrings);

        void setColumnType(Index nCol, ColType colType); // Sets column type for given column. @note If column has custom string-to-double parser, it will be cleared.
        void setColumnType(Index nCol, StringViewC sColType); // Converts string to ColType and calls setColumnType() with ColType-data, sColType must one of: <empty> (=type not changed), "text", "number", "date".
        void setColumnStringToDoubleParser(Index nCol, ColInfo::StringToDoubleParser parser); // Sets custom string-to-double parser for given column.
        QString getColumnStringToDoubleParserDefinition(Index nCol) const; // Returns definition string of current custom string-to-double parser if such is defined, empty otherwise.
        QVariant getColumnProperty(Index nCol, CsvItemModelColumnProperty propertyId, QVariant defaultValue = QVariant());
        void setColumnProperty(Index nCol, CsvItemModelColumnProperty propertyId, QVariant value);

        // Sets read-only status for single (row, column) address.
        // Note: operations such as row insert do not adjust the read-only indexes:
        //       for example if setting read-only to (0, 0) and inserting row to beginning,
        //       read-only cell remains at (0, 0). Thus this functionality might only be 
        //       useful in limited use cases where data table structure is restricted by other means.
        void setCellReadOnlyStatus(Index nRow, Index nCol, bool bReadOnly);

        // Tokenizes properly formatted text line and sets the data
        // as cells of given row @p nRow.
        void setRow(const int nRow, QString sLine);

        // Returns true if item was set, false otherwise.
        bool setItem(const int nRow, const int nCol, const QString& str);
        bool setItem(const int nRow, const int nCol, const StringViewUtf8& sv);

        // Sets data of given index without triggering undo, returns return value of privSetDataToTable()
        // Note: Given model index must be valid; this function does not check it.
        bool setDataNoUndo(const QModelIndex& index, const QString& str);
        bool setDataNoUndo(const QModelIndex& index, StringViewUtf8 sv);
        bool setDataNoUndo(Index nRow, Index nCol, const QString& str);
        bool setDataNoUndo(Index nRow, Index nCol, StringViewUtf8 sv);

        // Sets data to table as follows:
        //      -Checks if target indexes are valid, returns false if not.
        //      -Checks if existing item is identical and returns false if identical.
        //      -Does NOT handle setting modified.
        //      -Does NOT signal dataChanged()
        // Note: Caller must handle dataChanged signaling and setting modified.
        // Return: true if value was set and was different from existing, false otherwise.
        bool privSetDataToTable(Index nRow, Index nCol, StringViewUtf8 sv);

        void setColumnName(const int nCol, const QString& sName);

        template <class Cont_T>
        void removeRows(const Cont_T& indexesAscSorted) { removeRows(::DFG_MODULE_NS(cont)::ArrayWrapper::createArrayWrapper(indexesAscSorted)); }

        void removeRows(const ::DFG_MODULE_NS(cont)::ArrayWrapperT<const int>& indexesAscSorted);

        bool transpose();

        static QString& dataCellToString(const QString& sSrc, QString& sDst, const QChar cDelim);

        const QString& getFilePath() const
        { 
            return m_sFilePath; 
        }

        QString getTableTitle(const QString& sDefault = QString()) const;

        // Returns file encoding from given file path. Note: based solely on BOM-markers, so won't detect e.g. BOM-less UTF8
        static ::DFG_MODULE_NS(io)::TextEncoding getFileEncoding(const QString& filePath);

        // Convenience function returning getFileEncoding(this->getFilePath())
        ::DFG_MODULE_NS(io)::TextEncoding getFileEncoding() const;

        void setFilePathWithoutSignalEmit(QString);
        void setFilePathWithSignalEmit(QString);

        void initCompletionFeature();

        float latestReadTimeInSeconds()  const { return m_readTimeInSeconds; }
        float latestWriteTimeInSeconds() const { return m_writeTimeInSeconds; }

        LoadOptions getLoadOptionsForFile(const QString& sFilePath) const;
        static LoadOptions getLoadOptionsForFile(const QString& sFilePath, const CsvItemModel* pModel);
        LoadOptions getLoadOptionsFromConfFile() const; // Shortcut for CsvItemModel::getLoadOptionsForFile(this->getFilePath())
        LoadOptions getOpenTimeLoadOptions() const { return m_loadOptionsInOpen; } // Returns LoadOptions that were used when opened from file or memory.

        void populateConfig(::DFG_MODULE_NS(cont)::CsvConfig& config) const;

        ::DFG_MODULE_NS(cont)::CsvConfig getConfig() const;
        static ::DFG_MODULE_NS(cont)::CsvConfig getConfig(const QString& sConfFilePath);

        // Gives internal table to given function object for arbitrary edits and handles model specific tasks such as setting modified.
        // Note: this is low level access function requiring care when used; for example it's caller responsibility to check active read-only flags in CsvItemModel.
        // Note: Does not check whether the table has actually changed and always sets the model modified.
        // Note: This resets model (for details, see QAbstractItemModel::beginResetModel()). This means that e.g. view selection is lost when calling this function.
        //       If caller wishes to have the same selection before and after this call, it must be handled manually.
        template <class Func_T> void batchEditNoUndo(Func_T func);

        // Copies all elements from 'table' to this by (row, column) indexes and bypasses undo.
        // If pFill is given, data written to each (row, column) will be pFill instead of table(row, column)
        // If isCancelledFunc is given, implementation polls the function if batch edit should be terminated, should return true to stop editing, false to continue.
        void setDataByBatch_noUndo(const RawDataTable& table, const SzPtrUtf8R pFill = SzPtrUtf8R(nullptr), std::function<bool()> isCancelledFunc = std::function<bool()>());

        void setHighlighter(HighlightDefinition hld);

        // Finds next match.
        QModelIndex findNextHighlighterMatch(QModelIndex seedIndex, // Seed which is advanced before doing first actual match.
                                             FindDirection direction = FindDirectionForward);

        QModelIndex nextCellByFinderAdvance(const QModelIndex& seedIndex, const FindDirection direction, const FindAdvanceStyle advanceStyle) const;
        void nextCellByFinderAdvance(int& r, int& c, const FindDirection direction, const FindAdvanceStyle advanceStyle) const;

        LinearIndex cellRowColumnPairToLinearIndex(Index r, Index c) const;
        static IndexPairInteger cellRowColumnPairToIndexPairInteger(Index r, Index c);
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
        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList& indexes) const override;
        bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
        bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
#endif

        // Returns internal datatable type defined only in CsvItemModel.cpp
        DataTableRef&      table();
        DataTableConstRef& table() const;

        static CsvFormatDefinition peekCsvFormatFromFile(const QString& sPath, const size_t nPeekLimitAsBaseChars = 512);

        using ColumnNumberDataInterpretationChangedParam = int; // Placeholder

    signals:
        void sigModifiedStatusChanged(bool bNewStatus);
        void sigOnNewSourceOpened();
        void sigSourcePathChanged();
        void sigOnSaveToFileCompleted(bool, double);
        // Emitted when the way how content is interpreted as numbers has changed (e.g. column type or custom string-to-double parser has changed)
        // Parameter is currently not used.
        void sigColumnNumberDataInterpretationChanged(::DFG_MODULE_NS(qt)::CsvItemModel::Index nCol, ::DFG_MODULE_NS(qt)::CsvItemModel::ColumnNumberDataInterpretationChangedParam);

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
        static bool forEachColInfoWhileImpl(This_T& rThis, const LockReleaser& lockReleaser, Func_T&& func);

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
        func(m_table);
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
