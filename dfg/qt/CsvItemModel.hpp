#pragma once

#include "../dfgDefs.hpp"
#include "../buildConfig.hpp" // To get rid of C4996 "Function call with parameters that may be unsafe" in MSVC.
#include "qtIncludeHelpers.hpp"
#include "qtBasic.hpp"
#include "../cont/tableCsv.hpp"
#include "../io/textEncodingTypes.hpp"
#include "StringMatchDefinition.hpp"
#include "../build/languageFeatureInfo.hpp"
#include "../cont/arrayWrapper.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QAbstractTableModel>
#include <QBrush>
#include <QTextStream>
DFG_END_INCLUDE_QT_HEADERS

#include "../io/DelimitedTextWriter.hpp"
#include "../io/DelimitedTextReader.hpp"
#include <memory>
#include <vector>

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

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

#ifndef DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS
    #define DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS  0
#endif

    const char CsvOptionProperty_completerColumns[]          = "completerColumns";
    const char CsvOptionProperty_completerEnabledSizeLimit[] = "completerEnabledSizeLimit";
    const char CsvOptionProperty_includeRows[]               = "includeRows"; // To define rows to read from file, as IntervalSet syntax. ASCII, 0-based index
    const char CsvOptionProperty_includeColumns[]            = "includeColumns"; // To define columns to read from file, as IntervalSet syntax. ASCII, 0-based index
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

    namespace DFG_DETAIL_NS
    {
        class HighlightDefinition
        {
        public:
            HighlightDefinition(const QString id,
                                const int col,
                                DFG_CLASS_NAME(StringMatchDefinition) matcher,
                                QBrush highlightBrush = QBrush(Qt::green, Qt::SolidPattern)
                                )
                : m_id(id)
                , m_nColumn(col)
                , m_matcher(matcher)
                , m_highlightBrush(highlightBrush)
            {}

            QVariant data(const QAbstractItemModel& model, const QModelIndex& index, const int role) const;

            const DFG_CLASS_NAME(StringMatchDefinition)& matcher() const { return m_matcher; }

            QString m_id;
            int m_nColumn;
            DFG_CLASS_NAME(StringMatchDefinition) m_matcher;
            QBrush m_highlightBrush;
        }; // class HighlightDefinition

    } // detail-namespace

    // TODO: test
    class DFG_CLASS_NAME(CsvItemModel) : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        static const QString s_sEmpty;
        typedef QAbstractTableModel BaseClass;
        typedef int64 LinearIndex; // LinearIndex guarantees that LinearIndex(rowCount()) * LinearIndex(columnCount()) does not overflow.
        typedef std::ostream StreamT;
        typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TableSz)<char, int, DFG_MODULE_NS(io)::encodingUTF8> RawDataTable;
        typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TableCsv)<char, int, DFG_MODULE_NS(io)::encodingUTF8> DataTable;
        typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(SortedSequence)<std::vector<int>> IndexSet;
        typedef DFG_DETAIL_NS::HighlightDefinition HighlightDefinition;
        typedef DFG_MODULE_NS(qt)::DFG_CLASS_NAME(StringMatchDefinition) StringMatchDefinition;
        
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

        struct ColInfo
        {
            struct CompleterDeleter
            {
                void operator()(QCompleter* ptr) const;
            };

            ColInfo(QString sName = "", ColType type = ColTypeText, CompleterType complType = CompleterTypeNone) :
                m_name(sName), m_type(type), m_completerType(complType) {}

#if (DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT == 0)
            ColInfo(ColInfo&& other);
            ColInfo& operator=(ColInfo&& other);
#endif

            bool hasCompleter() const { return m_spCompleter.get() != nullptr; }

            QString m_name;
            ColType m_type;
            CompleterType m_completerType;
            std::unique_ptr<QCompleter, CompleterDeleter> m_spCompleter;
        };

        class SaveOptions : public DFG_CLASS_NAME(CsvFormatDefinition)
        {
        public:
            DFG_BASE_CONSTRUCTOR_DELEGATE_1(SaveOptions, DFG_CLASS_NAME(CsvFormatDefinition)) {}
            SaveOptions(DFG_CLASS_NAME(CsvItemModel)* itemModel);
            SaveOptions(const DFG_CLASS_NAME(CsvItemModel)* itemModel);

        private:
            void initFromItemModelPtr(const DFG_CLASS_NAME(CsvItemModel)* pItemModel);
        }; // class SaveOptions

        class LoadOptions : public DFG_CLASS_NAME(CsvFormatDefinition)
        {
        public:
            DFG_BASE_CONSTRUCTOR_DELEGATE_1(LoadOptions, DFG_CLASS_NAME(CsvFormatDefinition)) {}
            LoadOptions() : DFG_CLASS_NAME(CsvFormatDefinition)(::DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect,
                                                                '"',
                                                                DFG_MODULE_NS(io)::EndOfLineTypeN, 
                                                                DFG_MODULE_NS(io)::encodingUnknown)
            {}
        }; // class LoadOptions

        // Maps valid internal row index [0, rowCount[ to user seen indexing, usually 1-based indexing.
        static int internalRowIndexToVisible(const int nRow) { return nRow + 1; }
        static int visibleRowIndexToInternal(const int nRow) { return nRow - 1; }

        DFG_CLASS_NAME(CsvItemModel)();
        ~DFG_CLASS_NAME(CsvItemModel)();

        // Calls saveToFile(m_sFilePath)
        bool saveToFile();

        // Tries to save csv-data to given path. If successful, sets path and resets modified flag.
        // [return] : Returns true iff. save is successful.
        bool saveToFile(const QString& sPath);
        bool saveToFile(const QString& sPath, const SaveOptions& options);

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
        bool readData(const LoadOptions& options, std::function<void()> tableFiller);

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

        const QString& getHeaderName(const int nCol) const { return (isValidColumn(nCol)) ? m_vecColInfo[nCol].m_name : s_sEmpty; }

        ColType getColType(const int nCol) const { return (isValidColumn(nCol) ? m_vecColInfo[nCol].m_type : ColTypeText); }
        CompleterType getColCompleterType(const int nCol) const { return (isValidColumn(nCol) ? m_vecColInfo[nCol].m_completerType : CompleterTypeNone); }

        ColInfo* getColInfo(const int nCol) { return isValidColumn(nCol) ? &m_vecColInfo[nCol] : nullptr; }

        SaveOptions getSaveOptions() const;

        // Appends data model row to string.
        void rowToString(const int nRow, QString& str, const QChar cDelim, const IndexSet* pSetIgnoreColumns = nullptr) const;

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

        // Sets cell strings in column @p nCol to those given in @p vecStrings.
        void setColumnCells(const int nCol, const std::vector<QString>& vecStrings);

        void setColumnType(const int nCol, const ColType colType);

        // Tokenizes properly formatted text line and sets the data
        // as cells of given row @p nRow.
        void setRow(const int nRow, QString sLine);

        // Returns true if item was set, false otherwise.
        bool setItem(const int nRow, const int nCol, const QString str);
        bool setItem(const int nRow, const int nCol, SzPtrUtf8R psz);

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

        // Model Overloads
        int rowCount(const QModelIndex & parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
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
    }; // class CsvItemModel

    template <class Func_T> void DFG_CLASS_NAME(CsvItemModel)::batchEditNoUndo(Func_T func)
    {
        beginResetModel(); // This might be a bit coarse for smaller edits.
        m_bResetting = true;
        func(m_table);
        endResetModel();
        m_bResetting = false;
        setModifiedStatus(true);
    }

} } // module namespace
