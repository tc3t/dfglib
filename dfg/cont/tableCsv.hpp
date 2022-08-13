#pragma once

#include "table.hpp"
#include "../io/textEncodingTypes.hpp"
#include "../io/EndOfLineTypes.hpp"
#include "../io/DelimitedTextReader.hpp"
#include "../io/DelimitedTextWriter.hpp"
#include "../io/fileToByteContainer.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../io/IfStream.hpp"
#include "../io.hpp"
#include "../io/ImStreamWithEncoding.hpp"
#include "../utf.hpp"
#include "MapVector.hpp"
#include <unordered_map>
#include "CsvConfig.hpp"
#include "../str/stringLiteralCharToValue.hpp"
#include "../io/IfmmStream.hpp"
#include "IntervalSet.hpp"
#include "MapToStringViews.hpp"
#include "../numericTypeTools.hpp"
#include "vectorSso.hpp"
#include "detail/tableCsvHelpers.hpp"
#include "../time/timerCpu.hpp"

#include "../concurrency/ConditionCounter.hpp"
#include "../concurrency/ThreadList.hpp"

#include "../str/format_fmt.hpp"

#include <mutex>

DFG_ROOT_NS_BEGIN{ 
    namespace DFG_DETAIL_NS
    {
        template <class Strm_T>
        inline CsvFormatDefinition peekCsvFormat(Strm_T&& rawStrm, const size_t nPeekLimitAsBaseChars = 512)
        {
            // Determines csv-format by peeking at first nPeekLimit base chars from stream.
            using namespace ::DFG_MODULE_NS(io);
            using namespace ::DFG_MODULE_NS(cont);
            std::vector<char> rawPeekBuffer; // Stores raw bytes from stream.
            const auto encoding = checkBOM(rawStrm);
            const auto encodingCharSize = baseCharacterSize(encoding);
            rawPeekBuffer.resize(nPeekLimitAsBaseChars * encodingCharSize);
            const auto nRead = rawStrm.readBytes(rawPeekBuffer.data(), rawPeekBuffer.size());
            ImStreamWithEncoding istrmEncoding(rawPeekBuffer.data(), nRead, encoding);

            VectorSso<char, 512> charArray; // Stores plain chars where non-ascii-value have been saturateCasted (=assuming format chars to be ascii)
            // Reading code points from encoding stream to charArray.
            for (int i = istrmEncoding.get(); i != istrmEncoding.eofValue(); i = istrmEncoding.get())
                charArray.push_back(saturateCast<int8>(i));

            // Now parsing the charArray.
            BasicImStream basicImStream(charArray.data(), charArray.size());
            DelimitedTextReader::CellData<char> cellData(DelimitedTextReader::s_nMetaCharAutoDetect, '"', '\n');
            auto reader = DelimitedTextReader::createReader(basicImStream, cellData);

            DelimitedTextReader::readRow(reader, [&](size_t, decltype(cellData)& cd)
            {
                // Terminating reading after first cell since separator detection is ready.
                cd.setReadStatus(DelimitedTextReader::cellHrvTerminateRead);
            });

            // Determining eol simply by looking for \r from the charArray. Note that this fails if enclosed cells have eol's different from csv-eol,
            // e.g. "a\r\nb"\n"c" will be detected wrongly as \r\n instead of correct \n.
            EndOfLineType eolType = EndOfLineTypeN;
            auto iterCr = std::find(charArray.begin(), charArray.end(), '\r');
            // If \r is found, setting eol-type either to \r\n or \r depending on whether \r is followed by \n
            if (iterCr != charArray.end())
                eolType = (iterCr + 1 != charArray.end() && *(iterCr + 1) == '\n') ? EndOfLineTypeRN : EndOfLineTypeR;
            const auto formatDefInfo = reader.getFormatDefInfo();
            const auto rawSep = formatDefInfo.getSep();
            const auto nEffectiveSep = (rawSep > 0 && rawSep < 128) ? static_cast<char>(rawSep) : ',';
            return CsvFormatDefinition(nEffectiveSep, '"', eolType, encoding);
        }
    } // namespace DFG_DETAIL_NS

    // Peeks at given file to determine its CSV format.
    // Currently supports detecting:
    //      -Any of the auto-detected separators
    //      -Encoding
    //      -End of line type
    // The following are NOT detected:
    //      -enclosing character
    // Note that separator/eol needs to be within peek limit in order to get detected.
    inline CsvFormatDefinition peekCsvFormatFromFile(const StringViewSzC& sPath, const size_t nPeekLimitAsBaseChars = 512)
    {
        return DFG_DETAIL_NS::peekCsvFormat(::DFG_MODULE_NS(io)::createInputStreamBinaryFile(ReadOnlySzParamC(sPath.c_str())), nPeekLimitAsBaseChars);
    }
    
    DFG_SUB_NS(cont)
    {
        namespace DFG_DETAIL_NS
        {
            template <class Strm_T> inline StringViewAscii streamStatName()                                 { return DFG_ASCII("custom"); }
            template <> inline StringViewAscii streamStatName<::DFG_MODULE_NS(io)::BasicImStream>()         { return DFG_ASCII("memory_basic"); }
            template <> inline StringViewAscii streamStatName<::DFG_MODULE_NS(io)::ImStreamWithEncoding>()  { return DFG_ASCII("memory_encoding"); }
            template <> inline StringViewAscii streamStatName<::DFG_MODULE_NS(io)::IfStreamWithEncoding>()  { return DFG_ASCII("file_encoding"); }

            template <class Appender_T> inline StringViewAscii appenderStatName() { return DFG_ASCII("custom"); }
            template <> inline StringViewAscii appenderStatName<::DFG_MODULE_NS(io)::DelimitedTextReader::CharAppenderUtf<::DFG_MODULE_NS(io)::DelimitedTextReader::CharBuffer<char>>>() { return DFG_ASCII("utf_charbufc"); }
            template <> inline StringViewAscii appenderStatName<::DFG_MODULE_NS(io)::DelimitedTextReader::CharAppenderStringViewCBuffer>() { return DFG_ASCII("viewc_basic"); }
            template <> inline StringViewAscii appenderStatName<::DFG_MODULE_NS(io)::DelimitedTextReader::CharAppenderStringViewCBufferWithEnclosedCellSupport>() { return DFG_ASCII("viewc_enclosed"); }

            template <class Index_T>
            constexpr Index_T anyColumn()
            {
                return NumericTraits<Index_T>::maxValue;
            }

            class RowContentDummyFilter
            {
            public:
                constexpr bool operator()(Dummy, Dummy, Dummy, Dummy, Dummy, Dummy, Dummy) const
                {
                    return true;
                }
                void onReadDone(Dummy)
                {
                }
            };

            using ConcurrencySafeCellHandlerNo  = std::integral_constant<int, 0>;
            using ConcurrencySafeCellHandlerYes = std::integral_constant<int, 1>;

            template <class Index_T> using RowContentFilterBuffer = MapToStringViews<Index_T, StringUtf8, StringStorageType::sizeOnly>;

            template <class Table_T, class StringMatcher_T>
            class RowContentFilter
            {
            public:
                using IndexT = typename Table_T::IndexT;
                using RowBuffer = RowContentFilterBuffer<IndexT>;
                using StringViewT = StringViewUtf8;

                enum class RowFilterStatus
                {
                    unresolved, // Filter status not yet known, so content is to be stored to temporary buffer.
                    include,    // Row content has matched filter -> content can be stored directly to destination.
                    ignore      // Row is filtered out so nothing to be done for the content.
                };

                RowContentFilter(StringMatcher_T matcher, IndexT nMatchCol = anyColumn<IndexT>())
                    : m_stringMatcher(matcher)
                    , m_nFilterCol(nMatchCol)
                {}

                bool isAnyColumnFilter() const
                {
                    return m_nFilterCol == anyColumn<IndexT>();
                }

                void updateFilterStatus(Table_T& rTable, const IndexT nInputRow, const IndexT nInputCol, const IndexT nTargetRow, const IndexT nTargetCol, const char* pData, const size_t nCount)
                {
                    DFG_UNUSED(nTargetRow);
                    DFG_UNUSED(nTargetCol);
                    if (nInputCol != m_nFilterCol)
                        return;
                    if (m_stringMatcher.isMatch(nInputRow, toView(pData, nCount)))
                    {
                        // When having single column matching and match found from target column, flushing buffer to destination and setting status
                        // so that remaining items on the row gets written directly to destination.
                        writeToDestination(rTable, m_rowBuffer);
                        m_rowFilterStatus = RowFilterStatus::include;
                    }
                    else
                        m_rowFilterStatus = RowFilterStatus::ignore;
                }

                void writeToDestination(Table_T& table, const IndexT nCol, const StringViewT& sv)
                {
                    table.setElement(m_nBufferTargetRow - m_nFilteredRowCount, nCol, sv);
                }

                void writeToDestination(Table_T& table, RowBuffer& rowBuffer)
                {
                    for (const auto& item : rowBuffer)
                    {
                        DFG_ASSERT_CORRECTNESS(m_mapInputColumnToTargetColumn.hasKey(item.first));
                        writeToDestination(table, m_mapInputColumnToTargetColumn[item.first], item.second(rowBuffer));
                    }
                    rowBuffer.clear_noDealloc();
                    m_mapInputColumnToTargetColumn.clear();
                }

                StringViewT toView(const char* pData, const size_t nCount)
                {
                    return StringViewT(TypedCharPtrUtf8R(pData), nCount);
                }

                void finishRow(Table_T& rTable, const IndexT nNewInputRow, const IndexT nNewTargetRow)
                {
                    if (!m_rowBuffer.empty())
                    {
                        // Note: ending up here with single column filter means that filter column was not encountered (e.g. filter column at 3 and only 2 columns at current row)
                        //       In this case passing the whole buffer to matcher and letting it decide how to interpret such case.
                        if (m_rowFilterStatus != RowFilterStatus::ignore && m_stringMatcher.isMatch(m_nBufferInputRow, m_rowBuffer))
                            writeToDestination(rTable, m_rowBuffer);
                        else
                            m_nFilteredRowCount++;
                        m_rowBuffer.clear_noDealloc();
                    }
                    m_nBufferInputRow = nNewInputRow;
                    m_nBufferTargetRow = nNewTargetRow;
                    m_rowFilterStatus = RowFilterStatus::unresolved;
                }

                // Return true if caller should add data to table, false otherwise. With this implementation 
                // always returning false since all writing gets handling by 'this'.
                bool operator()(Table_T& rTable, const IndexT nInputRow, const IndexT nInputCol, const IndexT nTargetRow, const IndexT nTargetCol, const char* pData, const size_t nCount)
                {
                    if (nTargetCol == 0) // On columnm 0 (=new row), checking if previous row needs to be added to destination table.
                        finishRow(rTable, nInputRow, nTargetRow);
                    m_mapInputColumnToTargetColumn[nInputCol] = nTargetCol;
                    updateFilterStatus(rTable, nInputRow, nInputCol, nTargetRow, nTargetCol, pData, nCount);
                    if (m_rowFilterStatus == RowFilterStatus::unresolved)
                        m_rowBuffer.insert(nInputCol, StringViewT(TypedCharPtrUtf8R(pData), TypedCharPtrUtf8R(pData + nCount)));
                    else if (m_rowFilterStatus == RowFilterStatus::include)
                        writeToDestination(rTable, nTargetCol, toView(pData, nCount));
                    return false;
                }

                void onReadDone(Table_T& rTable)
                {
                    finishRow(rTable, m_nBufferInputRow + 1, m_nBufferTargetRow + 1);
                }

                StringMatcher_T m_stringMatcher;
                IndexT m_nFilterCol = anyColumn<IndexT>();
                RowFilterStatus m_rowFilterStatus = RowFilterStatus::unresolved;
                RowBuffer m_rowBuffer;
                MapVectorSoA<IndexT, IndexT> m_mapInputColumnToTargetColumn;
                IndexT m_nBufferInputRow = 0; // Stores input index of the row that m_rowBuffer stores.
                IndexT m_nBufferTargetRow = 0; // Stores target index of the row that m_rowBuffer stores.
                IndexT m_nFilteredRowCount = 0; // Stores the number of rows filter because of content filter. Note: row include filter not included here.
            }; // RowContentFilter


            template <class Table_T, class RowContentFilter_T>
            class FilterCellHandler
            {
            public:
                using IndexT = typename Table_T::IndexT;
                using IntervalSet = ::DFG_MODULE_NS(cont)::IntervalSet<IndexT>;

                // At least row index handling would probably need to be revamped for concurrency-safe reading.
                static constexpr ConcurrencySafeCellHandlerNo isConcurrencySafeT() { return ConcurrencySafeCellHandlerNo(); }

                FilterCellHandler(Table_T& rTable, RowContentFilter_T&& rowContentFilter)
                    : m_rTable(rTable)
                    , m_rowContentFilter(rowContentFilter)
                {}

                // Note: Indexes starts from 0. If file has header, first data row is 1.
                void setIncludeRows(IntervalSet is)
                {
                    m_includeRows = std::move(is);
                }

                // Note: Indexes starts from 0.
                void setIncludeColumns(IntervalSet is)
                {
                    m_includeColumns = std::move(is);
                }

                void operator()(const size_t nRowArg, const size_t nColArg, const char* pData, const size_t nCount)
                {
                    if (!isValWithinLimitsOfType<IndexT>(nRowArg) || !isValWithinLimitsOfType<IndexT>(nColArg))
                        return;
                    const auto nInputRow = static_cast<IndexT>(nRowArg);
                    const auto nInputCol = static_cast<IndexT>(nColArg);
                    if (!m_includeRows.hasValue(nInputRow))
                    {
                        if (nInputCol == 0)
                            m_nFilteredRowCount++;
                        return;
                    }
                    if (!m_includeColumns.hasValue(nInputCol))
                        return;
                    const bool bAllColumns = m_includeColumns.isSingleInterval(0, maxValueOfType<IndexT>());
                    auto nTargetCol = nInputCol;
                    if (!bAllColumns)
                    {
                        auto iter = m_mapInputColumnToTargetColumn.find(nInputCol);
                        if (iter == m_mapInputColumnToTargetColumn.end())
                            iter = m_mapInputColumnToTargetColumn.insert(nInputCol, static_cast<IndexT>(m_includeColumns.countOfElementsInRange(0, nInputCol) - 1)).first;
                        nTargetCol = iter->second;
                    }
                    const auto nTargetRow = nInputRow - m_nFilteredRowCount;
                    if (m_rowContentFilter(m_rTable, nInputRow, nInputCol, nTargetRow, nTargetCol, pData, nCount))
                        m_rTable.setElement(nTargetRow, nTargetCol, StringViewUtf8(TypedCharPtrUtf8R(pData), nCount));
                };

                void onReadDone()
                {
                    m_rowContentFilter.onReadDone(m_rTable);
                }

                Table_T& m_rTable;
                IntervalSet m_includeRows;
                IntervalSet m_includeColumns;
                MapVectorSoA<IndexT, IndexT> m_mapInputColumnToTargetColumn;
                IndexT m_nFilteredRowCount = 0;
                RowContentFilter_T m_rowContentFilter;
            }; // Class FilterCellHandler
        } // namespace DFG_DETAIL_NS


        template <class Char_T, class Index_T, DFG_MODULE_NS(io)::TextEncoding InternalEncoding_T = DFG_MODULE_NS(io)::encodingUTF8>
        class TableCsv : public TableSz<Char_T, Index_T, InternalEncoding_T>
        {
        public:
            using ThisClass = TableCsv;
            using BaseClass = TableSz<Char_T, Index_T, InternalEncoding_T>;
            using IndexT = typename BaseClass::IndexT;
            typedef DFG_ROOT_NS::CsvFormatDefinition CsvFormatDefinition;
            typedef typename TableSz<Char_T, Index_T>::RowToContentMap RowToContentMap;
            typedef DFG_MODULE_NS(io)::DelimitedTextReader::CharBuffer<char> DelimitedTextReaderBufferTypeC;
            typedef DFG_MODULE_NS(io)::DelimitedTextReader::CharBuffer<Char_T> DelimitedTextReaderBufferTypeT;
            using DelimitedTextReader = ::DFG_MODULE_NS(io)::DelimitedTextReader;
            template <class StringMatcher_T> using RowContentFilter = DFG_DETAIL_NS::RowContentFilter<ThisClass, StringMatcher_T>;

            // Class-"variables" for concurrency safety. When CellHander returns ConcurrencySafeCellHandlerNo from isConcurrencySafeT(), then TableCsv will not try to
            // do multithreaded read. When it returns yes, CellHandler class must implement makeConcurrencyClone() that returns a concurrency-safe handler that is used for reading
            // other block.
            using ConcurrencySafeCellHandlerNo  = DFG_DETAIL_NS::ConcurrencySafeCellHandlerNo;
            using ConcurrencySafeCellHandlerYes = DFG_DETAIL_NS::ConcurrencySafeCellHandlerYes;

            class OperationCancelledException {};

            // RowContentFilterBuffer is a map-like structure that can be iterated with regular range-based for like described in iterating through MapToStringViews.
            using RowContentFilterBuffer = DFG_DETAIL_NS::RowContentFilterBuffer<IndexT>;

            class CellHandlerBase
            {
            public:
                void onReadDone()
                {
                }

                // Using 'not-safe' as default, derived class can override this.
                static constexpr ConcurrencySafeCellHandlerNo isConcurrencySafeT() { return ConcurrencySafeCellHandlerNo(); }
            };

            class DefaultCellHandler : public CellHandlerBase
            {
            public:
                DefaultCellHandler(TableCsv& rTable)
                    : m_rTable(rTable)
                {}

                // Default handler has no state other than reference to TableCsv and since concurrent reading is done to separate TableCsv-objects,
                // this handler is concurrency-safe.
                static constexpr ConcurrencySafeCellHandlerYes isConcurrencySafeT() { return ConcurrencySafeCellHandlerYes(); }

                // Returns a concurrency-safe clone of this that is used for reading to given TableCsv.
                template <class Derived_T>
                Derived_T makeConcurrencyClone(TableCsv& rTable) { return Derived_T(rTable); }

                void operator()(const size_t nRow, const size_t nCol, const Char_T* pData, const size_t nCount)
                {
                    DFG_STATIC_ASSERT(InternalEncoding_T == DFG_MODULE_NS(io)::encodingUTF8, "Implimentation exists only for UTF8-encoding");
                    // TODO: this effectively assumes that user given input is valid UTF8.
                    m_rTable.setElement(nRow, nCol, StringViewUtf8(TypedCharPtrUtf8R(pData), nCount));
                };

                TableCsv& m_rTable;
            };

        public:

            TableCsv()
                : m_readFormat(',', '"', DFG_MODULE_NS(io)::EndOfLineTypeN, DFG_MODULE_NS(io)::encodingUTF8)
                , m_saveFormat(m_readFormat)
            {}

            DefaultCellHandler defaultCellHandler()
            {
                return DefaultCellHandler(*this);
            }

            // Creates filter cell handler that can used to do filtered reading.
            // Note: lifetime of returned object is bound to lifetime of 'this'
            // Returned filter has all rows excluded and all columns included
            auto createFilterCellHandler() -> DFG_DETAIL_NS::FilterCellHandler<ThisClass, DFG_DETAIL_NS::RowContentDummyFilter>
            {
                auto rv = DFG_DETAIL_NS::FilterCellHandler<ThisClass, DFG_DETAIL_NS::RowContentDummyFilter>(*this, DFG_DETAIL_NS::RowContentDummyFilter());
                rv.setIncludeColumns(IntervalSet<IndexT>::makeSingleInterval(0, maxValueOfType<IndexT>()));
                return rv;
            }

            template <class StringMatcher_T>
            auto createFilterCellHandler(StringMatcher_T matcher, const IndexT nMatchCol = DFG_DETAIL_NS::anyColumn<IndexT>()) -> DFG_DETAIL_NS::FilterCellHandler<ThisClass, RowContentFilter<StringMatcher_T>>
            {
                auto rv = DFG_DETAIL_NS::FilterCellHandler<ThisClass, RowContentFilter<StringMatcher_T>>(*this, RowContentFilter<StringMatcher_T>(matcher, nMatchCol));
                // When using string matcher filtering, by default enabling all rows and columns.
                rv.setIncludeRows(IntervalSet<IndexT>::makeSingleInterval(0, maxValueOfType<IndexT>()));
                rv.setIncludeColumns(IntervalSet<IndexT>::makeSingleInterval(0, maxValueOfType<IndexT>()));
                return rv;
            }

            auto defaultAppender() const -> DelimitedTextReader::CharAppenderUtf<DelimitedTextReaderBufferTypeC>
            {
                return DelimitedTextReader::CharAppenderUtf<DelimitedTextReaderBufferTypeC>();
            }

            // TODO: test
            bool isContentAndSizesIdenticalWith(const TableCsv& other) const
            {
                const auto nRows = this->rowCountByMaxRowIndex();
                const auto nCols = this->colCountByMaxColIndex();
                if (nRows != other.rowCountByMaxRowIndex() || nCols != other.colCountByMaxColIndex())
                    return false;

                // TODO: optimize, this is quickly done, inefficient implementation.
                for (Index_T r = 0; r < nRows; ++r)
                {
                    for (Index_T c = 0; c < nCols; ++c)
                    {
                        auto p0 = (*this)(r, c);
                        auto p1 = other(r, c);
                        // TODO: revise logics: implementation below treats null and empty cells as different.
                        if ((!p0 && p1) || (p0 && !p1) || (std::strcmp(toCharPtr_raw(p0), toCharPtr_raw(p1)) != 0)) // TODO: Create comparison function instead of using strcmp().
                            return false;
                    }
                }
                return true;
            }

            void readFromFile(const ReadOnlySzParamC& sPath) { readFromFileImpl(sPath, defaultReadFormat()); }
            void readFromFile(const ReadOnlySzParamW& sPath) { readFromFileImpl(sPath, defaultReadFormat()); }
            void readFromFile(const ReadOnlySzParamC& sPath, const CsvFormatDefinition& formatDef) { readFromFileImpl(sPath, formatDef); }
            void readFromFile(const ReadOnlySzParamW& sPath, const CsvFormatDefinition& formatDef) { readFromFileImpl(sPath, formatDef); }

            template <class Reader_T>
            void readFromFile(const ReadOnlySzParamC& sPath, const CsvFormatDefinition& formatDef, Reader_T&& reader) { readFromFileImpl(sPath, formatDef, std::forward<Reader_T>(reader)); }

            template <class Char_T1>
            void readFromFileImpl(const ReadOnlySzParam<Char_T1>& sPath, const CsvFormatDefinition& formatDef)
            {
                readFromFileImpl(sPath, formatDef, defaultCellHandler());
            }

            template <class Char_T1, class Reader_T>
            void readFromFileImpl(const ReadOnlySzParam<Char_T1>& sPath, const CsvFormatDefinition& formatDef, Reader_T&& reader)
            {
                bool bRead = false;
                try
                {
                    auto memMappedFile = DFG_MODULE_NS(io)::FileMemoryMapped(sPath);
                    readFromMemory(memMappedFile.data(), memMappedFile.size(), formatDef, std::forward<Reader_T>(reader));
                    bRead = true;
                }
                catch (...)
                {
                }

                if (!bRead)
                {
                    DFG_MODULE_NS(io)::IfStreamWithEncoding istrm;
                    istrm.open(StringViewSz<Char_T1>(sPath.c_str()));
                    read(istrm, formatDef, defaultAppender(), std::forward<Reader_T>(reader));
                    m_readFormat.textEncoding(istrm.encoding());
                    m_saveFormat = m_readFormat;
                }
            }

            // Given byte range and block count, this function returns start positions of read blocks excluding first block that starts at 0.
            // Start positions are start of line positions.
            // Overall logic is:
            //      1. Calculate average block size given byte count and block count.
            //      2. Set read position to 0 
            //      3. Increment read position by block size
            //      4. Advance from start position until finding eol-characters. If eol not found, stop.
            //      5. Set next block start to next char from eol, i.e. start of next line
            //      6. Set read position to new block start and goto step 3.
            static std::vector<size_t> determineBlockStartPos(RangeIterator_T<const char*> bytes, const size_t nBlockCount, const int32 cEol)
            {
                if (nBlockCount == 0)
                    return std::vector<size_t>();
                const auto nBlockSizeAvg = bytes.size() / nBlockCount;
                std::vector<size_t> blockStarts;
                size_t nPos = nBlockSizeAvg;
                for (; nPos < bytes.size() && blockStarts.size() < nBlockCount;)
                {
                    for (; nPos < bytes.size(); ++nPos)
                    {
                        if (bytes[nPos] == cEol)
                        {
                            blockStarts.push_back(nPos + 1);
                            nPos = saturateAdd<size_t>(nPos, nBlockSizeAvg);
                            break;
                        }
                    }
                    if (blockStarts.size() + 1 == nBlockCount)
                        break; // Already found all block starts
                }
                DFG_ASSERT_CORRECTNESS(blockStarts.size() < nBlockCount);
                return blockStarts;
            }

            CsvFormatDefinition defaultReadFormat() const
            {
                return CsvFormatDefinition(DelimitedTextReader::s_nMetaCharAutoDetect,
                    '"',
                    DFG_MODULE_NS(io)::EndOfLineTypeN,
                    DFG_MODULE_NS(io)::encodingUnknown);
            }

            TableCsvReadWriteOptions readFormat() const
            {
                return m_readFormat;
            }

            TableCsvReadWriteOptions saveFormat() const
            {
                return m_saveFormat;
            }

            void saveFormat(const TableCsvReadWriteOptions& newFormat)
            {
                m_saveFormat = newFormat;
            }

            void readFromMemory(const char* const pData, const size_t nSize)
            {
                readFromMemory(pData, nSize, this->defaultReadFormat());
            }

            void readFromMemory(const char* const pData, const size_t nSize, const CsvFormatDefinition& formatDef)
            {
                readFromMemory(pData, nSize, formatDef, defaultCellHandler());
            }

            void readFromString(const StringViewC& sv)
            {
                readFromString(sv, this->defaultReadFormat());
            }

            void readFromString(const StringViewC& sv, const CsvFormatDefinition& formatDef)
            {
                readFromMemory(sv.data(), sv.size(), formatDef, this->defaultCellHandler());
            }

            // If already checked that multithreaded read is ok from input's format perspective, this function determines how many threads to actually consider using.
            static uint32 privDetermineReadThreadCountRequest(::DFG_MODULE_NS(io)::BasicImStream& strm, const CsvFormatDefinition& formatDef)
            {
                auto nThreadCountRequest = TableCsvReadWriteOptions::getPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadCount>(formatDef, 1);
                if (nThreadCountRequest == 1)
                    return 1;
                else if (nThreadCountRequest == 0) // Case: autodetermine, using hardware_concurrency.
                    nThreadCountRequest = Max<unsigned int>(1, std::thread::hardware_concurrency()); // hardware_concurrency() may return 0 so Max() is there to make sure that have at least value 1.
                const auto nInputSize = strm.countOfRemainingElems();
                // For now simply using coarse hardcoded default value 10 MB, should be runtime context dependent
                const auto nDefaultBlockSize = 10000000;
                const auto nThreadBlockSize = TableCsvReadWriteOptions::getPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>(formatDef, nDefaultBlockSize);
                if (nInputSize < nThreadBlockSize / 2)
                    return 1; // Whole input is less than 2 * thread block size -> not threading. Factor 2 is there because if block size is, say, 10 MB, need to have at least 20 MB input to have 10 MB for both threads.
                else if (nInputSize / nThreadCountRequest >= nThreadBlockSize)
                    return nThreadCountRequest; // There is enough work for all available threads.
                else // case: thread request should be something between ]1, nThreadCountRequest[
                {
                    // Simple linear search, pick first thread count that gives enough work for every thread.
                    // Note that blocking algorithm might still cause uneven input sizes to different threads or even decide to use less threads.
                    for (auto nThreadCount = nThreadCountRequest - 1; nThreadCount > 1; --nThreadCount)
                    {
                        const auto nBlockSize = nInputSize / nThreadCount;
                        if (nBlockSize >= nThreadBlockSize)
                            return nThreadCount;
                    }
                    return 1;
                }
            }

            template <class Reader_T>
            void privReadFromMemory_singleThreaded(const char* const pData, const size_t nSize, const ::DFG_MODULE_NS(io)::TextEncoding encoding, const CsvFormatDefinition& formatDef, Reader_T&& reader)
            {
                const auto encodingUtf8 = ::DFG_MODULE_NS(io)::encodingUTF8;
                if (encoding == encodingUtf8)
                {
                    ::DFG_MODULE_NS(io)::BasicImStream strm(pData, nSize);
                    if (formatDef.enclosingChar() == DelimitedTextReader::s_nMetaCharNone)
                        read(strm, formatDef, DelimitedTextReader::CharAppenderStringViewCBuffer(), std::forward<Reader_T>(reader));
                    else 
                        read(strm, formatDef, DelimitedTextReader::CharAppenderStringViewCBufferWithEnclosedCellSupport(), std::forward<Reader_T>(reader));
                }
                else if (encoding == ::DFG_MODULE_NS(io)::encodingUnknown)
                {
                    // Encoding of source bytes is unknown -> reading as Latin-1.
                    ::DFG_MODULE_NS(io)::BasicImStream strm(pData, nSize);
                    read(strm, formatDef, defaultAppender(), std::forward<Reader_T>(reader));
                }
                else // Case: encoding known and other than UTF8, read using encoding istream.
                {
                    ::DFG_MODULE_NS(io)::ImStreamWithEncoding strm(pData, nSize, encoding);
                    read(strm, formatDef, defaultAppender(), std::forward<Reader_T>(reader));
                }
            }

            template <class Appender_T, class CellHandler_T, class ReadStreamCreator_T>
            void privReadFromMemory_maybeMultithreaded(ConcurrencySafeCellHandlerYes, const char* const pData, const size_t nSize, const ::DFG_MODULE_NS(io)::TextEncoding encoding, const CsvFormatDefinition& formatDef, Appender_T&& appender, CellHandler_T&& cellHandler, ReadStreamCreator_T&& streamCreator)
            {
                using MemStream = ::DFG_MODULE_NS(io)::BasicImStream;
                MemStream strm(pData, nSize);
                const auto nThreadCountRequest = privDetermineReadThreadCountRequest(strm, formatDef);
                if (nThreadCountRequest > 1)
                    privReadFromMemoryMultiThreaded(MemStream(pData, nSize), formatDef, nThreadCountRequest, std::forward<Appender_T>(appender), std::forward<CellHandler_T>(cellHandler), std::forward<ReadStreamCreator_T>(streamCreator));
                else
                    privReadFromMemory_singleThreaded(pData, nSize, encoding, formatDef, std::forward<CellHandler_T>(cellHandler));
                
            }

            template <class Appender_T, class CellHandler_T, class ReadStreamCreator_T>
            void privReadFromMemory_maybeMultithreaded(ConcurrencySafeCellHandlerNo, const char* const pData, const size_t nSize, const ::DFG_MODULE_NS(io)::TextEncoding encoding, const CsvFormatDefinition& formatDef, Appender_T&& appender, CellHandler_T&& cellHandler, ReadStreamCreator_T&& streamCreator)
            {
                DFG_UNUSED(appender);
                DFG_UNUSED(streamCreator);
                privReadFromMemory_singleThreaded(pData, nSize, encoding, formatDef, std::forward<CellHandler_T>(cellHandler));
            }

            template <class Reader_T>
            void readFromMemory(const char* pData, size_t nSize, const CsvFormatDefinition& formatDef, Reader_T&& reader)
            {
                ::DFG_MODULE_NS(time)::TimerCpu timer;
                const auto streamBom = DFG_MODULE_NS(io)::checkBOM(pData, nSize);
                const auto encoding = (formatDef.textEncoding() == DFG_MODULE_NS(io)::encodingUnknown) ? streamBom : formatDef.textEncoding();
                const auto encodingUtf8 = ::DFG_MODULE_NS(io)::encodingUTF8;
                const auto encodingLatin1 = ::DFG_MODULE_NS(io)::encodingLatin1;
                const auto encodingUnknown = ::DFG_MODULE_NS(io)::encodingUnknown;

                const auto nBomSkip = (encoding == encodingUtf8 && streamBom == encodingUtf8) ? ::DFG_MODULE_NS(utf)::bomSizeInBytes(encodingUtf8) : 0;
                pData += nBomSkip;
                nSize -= nBomSkip;

                // Note: this is more of a implementation limitation. e.g. UTF16 input could be divided into read blocks, but not implemented.
                const auto bIsEncodingMultithreadCompatible = (encoding == ::DFG_MODULE_NS(io)::encodingUnknown || ::DFG_MODULE_NS(io)::areAsciiBytesValidContentInEncoding(encoding));

                if (bIsEncodingMultithreadCompatible && (formatDef.enclosingChar() == DelimitedTextReader::s_nMetaCharNone))
                {
                    using readerConcurrencySafety = decltype(std::remove_reference<Reader_T>::type::isConcurrencySafeT());
                    const auto readStreamCreatorBasic    = [](const char* pData, const size_t nSize) { return ::DFG_MODULE_NS(io)::BasicImStream(pData, nSize); };
                    const auto readStreamCreatorEncoding = [=](const char* pData, const size_t nSize) { return ::DFG_MODULE_NS(io)::ImStreamWithEncoding(pData, nSize, encoding); };
                    if (encoding == encodingUtf8)
                        privReadFromMemory_maybeMultithreaded(readerConcurrencySafety(), pData, nSize, encoding, formatDef, DelimitedTextReader::CharAppenderStringViewCBuffer(), std::forward<Reader_T>(reader), readStreamCreatorBasic);
                    else if (encoding == encodingLatin1 || encoding == encodingUnknown)
                        privReadFromMemory_maybeMultithreaded(readerConcurrencySafety(), pData, nSize, encoding, formatDef, defaultAppender(), std::forward<Reader_T>(reader), readStreamCreatorBasic);
                    else // If input is not UTF-8 nor Latin1, using encoding-aware stream when reading
                        privReadFromMemory_maybeMultithreaded(readerConcurrencySafety(), pData, nSize, encoding, formatDef, defaultAppender(), std::forward<Reader_T>(reader), readStreamCreatorEncoding);
                }
                else // Case: single-thread reading
                {
                    privReadFromMemory_singleThreaded(pData, nSize, encoding, formatDef, std::forward<Reader_T>(reader));
                }

                m_readFormat.textEncoding(encoding);
                const auto elapsed = timer.elapsedWallSeconds();
                if (isReadStatsEnabled())
                    m_readFormat.setReadStat<TableCsvReadStat::timeTotal>(elapsed);
                m_saveFormat = m_readFormat;
            }

            bool isReadStatsEnabled() const
            {
                return true;
            }

            // Returns true on success, false on failure. In case of failure, extended error details may be available through TableCsvReadStat::errorInfo
            template <class Strm_T, class CharAppender_T, class Reader_T>
            bool read(Strm_T& strm, const CsvFormatDefinition& formatDef, CharAppender_T, Reader_T&& cellHandler)
            {
                using namespace DFG_MODULE_NS(io);
                this->clear();

                if (isReadStatsEnabled())
                {
                    m_readFormat.setReadStat<TableCsvReadStat::appenderType>(::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::appenderStatName<CharAppender_T>());
                    m_readFormat.setReadStat<TableCsvReadStat::streamType>(::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::streamStatName<Strm_T>());
                }

                bool exceptionCaught = false;
                typedef DelimitedTextReader::ParsingDefinition<char, CharAppender_T> ParseDef;
                try
                {
                    const auto& readFormat = DelimitedTextReader::readEx(ParseDef(), strm, formatDef.separatorChar(), formatDef.enclosingChar(), formatDef.eolCharFromEndOfLineType(), std::forward<Reader_T>(cellHandler));
                    cellHandler.onReadDone();

                    m_readFormat.separatorChar(readFormat.getSep());
                    m_readFormat.enclosingChar(readFormat.getEnc());
                    if (formatDef.eolType() == DFG_MODULE_NS(io)::EndOfLineTypeRN)
                        m_readFormat.eolType(DFG_MODULE_NS(io)::EndOfLineTypeRN);
                    else if (formatDef.eolType() == DFG_MODULE_NS(io)::EndOfLineTypeR)
                        m_readFormat.eolType(DFG_MODULE_NS(io)::EndOfLineTypeR);
                    else
                        m_readFormat.eolType(DFG_MODULE_NS(io)::EndOfLineTypeN);
                    //m_readFormat.endOfLineChar(readFormat.getEol());
                    //m_readFormat.textEncoding(strm.encoding()); // This is set 
                    //m_readFormat.headerWriting(); //
                    //m_readFormat.bomWriting(); // TODO: This is should be enquiried from the stream whether the stream had BOM.
                    m_saveFormat = m_readFormat;
                }
                catch (const std::exception& e)
                {
                    if (isReadStatsEnabled())
                    {
                        CsvConfig errorInfo;
                        errorInfo.setKeyValue_fromUntyped(TableCsvErrorInfoFields::errorMsg.c_str(), format_fmt("Caught exception: '{0}'", e.what()));
                        m_readFormat.setReadStat<TableCsvReadStat::errorInfo>(errorInfo);
                    }
                    exceptionCaught = true;
                }
                catch (...)
                {
                    if (isReadStatsEnabled())
                    {
                        CsvConfig errorInfo;
                        errorInfo.setKeyValue(TableCsvErrorInfoFields::errorMsg, DFG_UTF8("Caught unknown exception type"));
                        m_readFormat.setReadStat<TableCsvReadStat::errorInfo>(errorInfo);
                    }
                    exceptionCaught = true;
                }
                return !exceptionCaught;
            }

            // Reads table from memory possibly in multiple threads. Overall logics:
            //      -Divides input into blocks
            //      -Every block is read as regular TableCsv in their own read-threads
            //      -When all blocks have been read, merges all TableCsv's into 'this'
            template <class CharAppender_T, class CellHandler_T, class ReadStreamCreator_T>
            void privReadFromMemoryMultiThreaded(::DFG_MODULE_NS(io)::BasicImStream strm, const CsvFormatDefinition& formatDef, const uint32 nThreadCountRequest, CharAppender_T, CellHandler_T&& cellHandler, ReadStreamCreator_T&& streamCreator)
            {
                ::DFG_MODULE_NS(time)::TimerCpu timerBlockReads;
                const auto nBlockCount = Max<size_t>(1, nThreadCountRequest);
                const auto additionalBlockStarts = determineBlockStartPos(makeRange(strm.beginPtr(), strm.endPtr()), nBlockCount, formatDef.eolCharFromEndOfLineType());
                std::vector<TableCsv> tables(additionalBlockStarts.size());
                std::vector<BaseClass*> tablePtrs(tables.size());
                std::transform(tables.begin(), tables.end(), tablePtrs.begin(), [](TableCsv& rTable) { return &rTable; });

                // Creating handler objects for additional blocks
                using CellHandlerT = typename std::remove_reference<CellHandler_T>::type;
                std::vector<CellHandlerT> additionalBlockCellHanders;
                DFG_STATIC_ASSERT((std::is_same<decltype(CellHandlerT::isConcurrencySafeT()), ConcurrencySafeCellHandlerYes>::value), "readFromMemoryMultiThreaded() may be called only for handler that are concurrency-safe");
                for (size_t i = 0; i < additionalBlockStarts.size(); ++i)
                    additionalBlockCellHanders.push_back(cellHandler.template makeConcurrencyClone<CellHandlerT>(tables[i]));

                const auto blockSize = [&](const size_t i) { return (i + 1 < additionalBlockStarts.size()) ? additionalBlockStarts[i + 1] - additionalBlockStarts[i] : strm.sizeInCharacters() - additionalBlockStarts[i]; };

                std::mutex mutexMergeFlags;
                typename std::make_unsigned<IndexT>::type nNextColumnToMerge = 0; // Accesses must be done while holding mutexMergeFlags
                IndexT nSeedTableOriginalRowCount = 0;
                IndexT nColumnCount = 0;

                // Defining column merge function. It merges columns as long as there are unprocessed columns available using variable nNextColumnToMerge.
                const auto mergeColumns = [&]()
                {
                    while (true)
                    {
                        IndexT nColToMerge = 0;
                        {
                            std::lock_guard<decltype(mutexMergeFlags)> lock(mutexMergeFlags);
                            if (static_cast<IndexT>(nNextColumnToMerge) >= nColumnCount)
                                return;
                            nColToMerge = static_cast<IndexT>(nNextColumnToMerge);
                            ++nNextColumnToMerge;
                        }
                        this->appendColumnWithMoveImpl(nColToMerge, nSeedTableOriginalRowCount, tablePtrs);
                    }
                };

                ::DFG_MODULE_NS(concurrency)::ThreadList threads;
                ::DFG_MODULE_NS(concurrency)::ConditionCounter conditionReadsDone(additionalBlockStarts.size());
                ::DFG_MODULE_NS(concurrency)::ConditionCounter conditionCanStartMerge(1);

                // Launching read-thread for every additional block handler
                for (size_t i = 0; i < additionalBlockStarts.size(); ++i)
                {
                    threads.push_back(std::thread([i, this, &tables, &strm, &additionalBlockStarts, &conditionReadsDone, &tablePtrs,
                                                   &mergeColumns, &conditionCanStartMerge,
                                                   &nSeedTableOriginalRowCount, blockSize, &formatDef, &additionalBlockCellHanders, streamCreator]()
                        {
                            auto& table = tables[i];
                            auto strmBlock = streamCreator(strm.beginPtr() + additionalBlockStarts[i], blockSize(i));
                            // Reading block
                            table.read(strmBlock, formatDef, CharAppender_T(), additionalBlockCellHanders[i]);
                            // Signaling read completed on this block
                            conditionReadsDone.decrementCounter();
                            // Waiting for 'ok to merge columns'-condition
                            conditionCanStartMerge.waitCounter();
                            // Starting column merge.
                            mergeColumns();
                        }));
                }
                const auto nTotalThreadCount = saturateCast<uint32>(threads.size() + 1u);

                // Reading first block from current thread.
                auto strmFirstBlock = streamCreator(strm.beginPtr(), (!additionalBlockStarts.empty()) ? additionalBlockStarts[0] : strm.sizeInCharacters());
                read(strmFirstBlock, formatDef, CharAppender_T(), std::forward<CellHandler_T>(cellHandler));
                nSeedTableOriginalRowCount = this->rowCountByMaxRowIndex();

                // Wait until other read threads complete
                conditionReadsDone.waitCounter();

                if (isReadStatsEnabled())
                {
                    const auto blockReadTime = timerBlockReads.elapsedWallSeconds();
                    m_readFormat.setReadStat<TableCsvReadStat::timeBlockReads>(blockReadTime);
                }

                ::DFG_MODULE_NS(time)::TimerCpu timerMerge;

                // Computing final column count
                nColumnCount = this->colCountByMaxColIndex();
                for (const auto& table : tables)
                    nColumnCount = Max(nColumnCount, table.colCountByMaxColIndex());

                // Appending blocks into 'this' concurrently per column
                this->appendTablesWithMoveImpl(tablePtrs, [&](BaseClass& rTable)
                    {
                        // Now that all threads have completed reading and column vectors have been resized properly by appendTablesWithMoveImpl(), notify threads to proceed to column merging.
                        DFG_ASSERT_UB(rTable.colCountByMaxColIndex() == nColumnCount);
                        conditionCanStartMerge.decrementCounter();
                        mergeColumns();
                        // Waiting other block handlers to complete.
                        threads.joinAllAndClear();
                    });

                if (isReadStatsEnabled())
                {
                    const auto mergeTime = timerMerge.elapsedWallSeconds();
                    m_readFormat.setReadStat<TableCsvReadStat::threadCount>(nTotalThreadCount);
                    m_readFormat.setReadStat<TableCsvReadStat::timeBlockMerge>(mergeTime);
                }

                // Checking for read errors
                if (isReadStatsEnabled())
                {
                    // Copying all error items from different TableCsv-objects into errorInfo of this
                    // using uri prefix "threads/thread_{0}/", where {0} is thread index (0 for thread of 'this').
                    CsvConfig csvConfig;
                    const auto storeErrors = [&](const TableCsv& table, const size_t nIndex)
                    {
                        const auto myError = table.readFormat().template getReadStat<TableCsvReadStat::errorInfo>();
                        if (!myError.empty())
                        {
                            myError.forEachKeyValue([&](CsvConfig::StringViewT svKey, CsvConfig::StringViewT svValue)
                                {
                                    auto sKey = format_fmt("threads/thread_{0}/{1}", nIndex, svKey.asUntypedView().toString());
                                    csvConfig.setKeyValue_fromUntyped(std::move(sKey), svValue);
                                });
                        }

                    };
                    storeErrors(*this, 0);
                    for (size_t i = 0; i < tables.size(); ++i)
                    {
                        storeErrors(*this, i + 1u);
                    }
                    if (!csvConfig.empty())
                    {
                        this->m_readFormat.setReadStat<TableCsvReadStat::errorInfo>(csvConfig);
                    }
                }
            }

            template <class Stream_T>
            class WritePolicySimple
            {
            public:
                WritePolicySimple(const CsvFormatDefinition& csvFormat) :
                    m_format(csvFormat)
                {
                    const auto cSep = m_format.separatorChar();
                    const auto svEol = ::DFG_MODULE_NS(io)::eolStrFromEndOfLineType(m_format.eolType());
                    const auto encoding = m_format.textEncoding();
                    DFG_MODULE_NS(utf)::cpToEncoded(cSep, std::back_inserter(m_bytes), encoding);
                    m_nEncodedSepSizeInBytes = static_cast<decltype(m_nEncodedSepSizeInBytes)>(m_bytes.size());
                    for (auto iter = svEol.cbegin(), iterEnd = svEol.cend(); iter != iterEnd; ++iter)
                        DFG_MODULE_NS(utf)::cpToEncoded(*iter, std::back_inserter(m_bytes), encoding);
                    m_nEncodedEolSizeInBytes = static_cast<decltype(m_nEncodedEolSizeInBytes)>(m_bytes.size()) - m_nEncodedSepSizeInBytes;
                    // Note: set the pointers after all bytes have been written to m_bytes to make sure that 
                    //       there will be no pointer invalidating reallocation.
                    m_pEncodedSep = ptrToContiguousMemory(m_bytes);
                    m_pEncodedEol = m_pEncodedSep + m_nEncodedSepSizeInBytes;
                }

                void writeItemFunc(Stream_T& strm, int c)
                {
                    m_workBytes.clear();
                    DFG_MODULE_NS(utf)::cpToEncoded(c, std::back_inserter(m_workBytes), m_format.textEncoding());
                    DFG_MODULE_NS(io)::writeBinary(strm, ptrToContiguousMemory(m_workBytes), m_workBytes.size());
                }

                void write(Stream_T& strm, const char* pData, const Index_T/*nRow*/, const Index_T/*nCol*/)
                {
                    using namespace DFG_MODULE_NS(io);
                    if (pData == nullptr)
                        return;
                    utf8::unchecked::iterator<const char*> inputIter(pData);
                    utf8::unchecked::iterator<const char*> inputIterEnd(pData + std::strlen(pData));
                    DelimitedTextCellWriter::writeCellFromStrIter(strm,
                                                                                 makeRange(inputIter, inputIterEnd),
                                                                                 uint32(m_format.separatorChar()),
                                                                                 uint32(m_format.enclosingChar()),
                                                                                 uint32(eolCharFromEndOfLineType(m_format.eolType())),
                                                                                 m_format.enclosementBehaviour(),
                                                                                 [&](Stream_T& strm, int c) {this->writeItemFunc(strm, c); });
                }

                void writeSeparator(Stream_T& strm, const Index_T /*nRow*/, const Index_T /*nCol*/)
                {
                    DFG_MODULE_NS(io)::writeBinary(strm, m_pEncodedSep, m_nEncodedSepSizeInBytes);
                }

                void writeEol(Stream_T& strm)
                {
                    DFG_MODULE_NS(io)::writeBinary(strm, m_pEncodedEol, m_nEncodedEolSizeInBytes);
                }

                void writeBom(Stream_T& strm)
                {
                    if (m_format.bomWriting())
                    {
                        const auto bomBytes = DFG_MODULE_NS(utf)::encodingToBom(m_format.textEncoding());
                        strm.write(bomBytes.data(), bomBytes.size());
                    }
                }

                CsvFormatDefinition m_format;
                std::string m_bytes;
                std::string m_workBytes;
                const char* m_pEncodedSep;
                const char* m_pEncodedEol;
                uint32 m_nEncodedSepSizeInBytes;
                uint32 m_nEncodedEolSizeInBytes;
            }; // class WritePolicySimple

            template <class Stream_T>
            auto createWritePolicy() const -> WritePolicySimple<Stream_T>
            {
                return WritePolicySimple<Stream_T>(m_saveFormat);
            }

            template <class Stream_T>
            static auto createWritePolicy(CsvFormatDefinition format) -> WritePolicySimple<Stream_T>
            {
                return WritePolicySimple<Stream_T>(format);
            }
            
            // Strm must have write()-method which writes bytes untranslated.
            //  TODO: test writing non-square data.
            template <class Strm_T, class Policy_T>
            void writeToStream(Strm_T& strm, Policy_T& policy) const
            {
                policy.writeBom(strm);

                if (this->m_colToRows.empty())
                    return;
                // nextColItemRowIters[i] is the valid iterator to the next row entry in column i.
                std::unordered_map<Index_T, typename RowToContentMap::const_iterator> nextColItemRowIters;
                this->forEachFwdColumnIndex([&](const Index_T nCol)
                {
                    if (isValidIndex(this->m_colToRows, nCol) && !this->m_colToRows[nCol].empty())
                        nextColItemRowIters[nCol] = this->m_colToRows[nCol].cbegin();
                });
                const auto nMaxColCount = this->colCountByMaxColIndex();
                for (Index_T nRow = 0; !nextColItemRowIters.empty(); ++nRow)
                {
                    for (Index_T nCol = 0; nCol < nMaxColCount; ++nCol)
                    {
                        auto iter = nextColItemRowIters.find(nCol);
                        if (iter != nextColItemRowIters.end() && this->privRowIteratorToRowNumber(nCol, iter->second) == nRow) // Case: (row, col) has item
                        {
                            auto& rowEntryIter = iter->second;
                            const auto pData = this->privRowIteratorToRawContent(nCol, rowEntryIter);
                            policy.write(strm, pData, this->privRowIteratorToRowNumber(nCol, rowEntryIter), nCol);
                            ++rowEntryIter;
                            if (rowEntryIter == this->m_colToRows[nCol].cend())
                                nextColItemRowIters.erase(iter);
                        }
                        if (nCol + 1 < nMaxColCount) // Write separator for all but the last column.
                            policy.writeSeparator(strm, nRow, nCol);
                    }
                    policy.writeEol(strm);
                }
            }

            // Convenience overload, see implementation version for comments.
            template <class Strm_T>
            void writeToStream(Strm_T& strm) const
            {
                auto policy = createWritePolicy<Strm_T>();
                writeToStream(strm, policy);
            }

            TableCsvReadWriteOptions m_readFormat; // Stores the format of previously read input. If no read is done, stores to default output format.
                                                   // TODO: specify content in case of interrupted read.
            TableCsvReadWriteOptions m_saveFormat; // Format to be used when saving
        }; // class TableCsv

} } // module namespace
