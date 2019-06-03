#pragma once

#include "../dfgDefs.hpp"
#include "../dfgAssert.hpp"
#include "../dfgBase.hpp"
#include "../str.hpp"
#include "../alg/sortMultiple.hpp"
#include "../io/textEncodingTypes.hpp"
#include "../numericTypeTools.hpp"
#include <algorithm>
#include <vector>
#include <numeric>
#include <map>
#include <deque>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(cont) {

    // Container for table which may have non equal row sizes.
    template <class Elem_T, class Index_T = size_t>
    class DFG_CLASS_NAME(Table)
    {
    public:
        static const auto s_nInvalidIndex = NumericTraits<Index_T>::maxValue;

        // Note: This was std::deque before. Clean up of deque-table constructed from ~50 MB file took tens-of-seconds/minutes
        //       while the same table with std::vector implementation cleaned up almost instantly.
        typedef std::vector<Elem_T> ElementContainerT;

        typedef typename ElementContainerT::iterator iterator;
        typedef typename ElementContainerT::const_iterator const_iterator;
        typedef Elem_T value_type;

        iterator begin() { return m_contElems.begin(); }
        const_iterator begin() const { return m_contElems.begin(); }
        const_iterator cbegin() const { return begin(); }

        iterator end() { return m_contElems.end(); }
        const_iterator end() const { return m_contElems.end(); }
        const_iterator cend() const { return end(); }

        void pushBackOnRow(Index_T nRow, Elem_T elem)
        {
            if (nRow == NumericTraits<Index_T>::maxValue) // Make sure that nRow + 1 won't overflow.
                return;

            if (!isValidIndex(m_rowSizes, nRow))
            {
                m_rowSizes.resize(nRow + 1, 0);
            }
            DFG_ASSERT_UB(isValidIndex(m_rowSizes, nRow));
            if (nRow + 1 == m_rowSizes.size()) // Adding element as the last item on the last row.
            {
                m_contElems.push_back(std::move(elem));
                m_rowSizes.back()++;
            }
            else
            {
                const auto nPos = privateGetLinearPosition(nRow + 1, 0);
                auto iterNewPos = m_contElems.begin() + nPos;
                m_contElems.insert(iterNewPos, std::move(elem));
                m_rowSizes[nRow]++;
            }
        }

        void setElement(Index_T nRow, Index_T nCol, Elem_T elem)
        {
            if (nRow == NumericTraits<Index_T>::maxValue || nCol == NumericTraits<Index_T>::maxValue) // Make sure that nRow + 1 nor nCol + 1 won't overflow.
                return;
            if (!isValidIndex(m_rowSizes, nRow))
                m_rowSizes.resize(nRow + 1, 0);
            if (nRow == m_rowSizes.size() - 1 && nCol == m_rowSizes[nRow]) // If new item is next on row, do pushBackOnRow.
                pushBackOnRow(nRow, std::move(elem));
            else // Case: not next item on row.
            {
                if (m_rowSizes[nRow] <= nCol)
                {
                    const auto nOldRowSize = m_rowSizes[nRow];
                    const auto nPos = privateGetLinearPositionVirtual(nRow, nOldRowSize);
                    const auto iterAddPos = m_contElems.begin() + nPos;
                    const auto nSizeInc = nCol - nOldRowSize + 1;
                    m_rowSizes[nRow] = nCol + 1;
                    m_contElems.insert(iterAddPos, nSizeInc, Elem_T());
                    m_contElems[nPos + nSizeInc - 1] = std::move(elem);
                }
                else // case: item already exists, replace.
                {
                    const auto nPos = privateGetLinearPosition(nRow, nCol);
                    m_contElems[nPos] = std::move(elem);
                }
            }
        }

        // If no element at (nRow, nCol) does not exist, returns s_nInvalidIndex.
        Index_T privateGetLinearPosition(Index_T nRow, Index_T nCol) const
        {
            if (!isValidTableIndex(nRow, nCol))
                return s_nInvalidIndex;
            const auto nPos = nCol + std::accumulate(m_rowSizes.cbegin(), m_rowSizes.cbegin() + nRow, Index_T(0));
            return nPos;
        }

        // Like privateGetLinearPosition, but if element at (nRow, nCol) does not exist, returns the index that given element would have if added to table.
        Index_T privateGetLinearPositionVirtual(Index_T nRow, Index_T nCol) const
        {
            const auto nPos = nCol + std::accumulate(m_rowSizes.cbegin(), m_rowSizes.cbegin() + nRow, Index_T(0));
            return nPos;
        }

        bool isValidTableIndex(Index_T row, Index_T col) const
        {
            return isValidIndex(m_rowSizes, row) && (col >= 0 && col < m_rowSizes[row]);
        }

        // requires
        //        - isValidTableIndex(row, col) == true
        const Elem_T& operator()(Index_T row, Index_T col) const
        {
            DFG_ASSERT_UB(isValidTableIndex(row, col));
            return m_contElems[privateGetLinearPosition(row, col)];
        }

        Index_T getColumnCountOnRow(Index_T nRow) const
        {
            return isValidIndex(m_rowSizes, nRow) ? m_rowSizes[nRow] : 0;
        }

        // Returns true iff. this is empty or all rows have equal size.
        bool hasSingleColumnCount() const
        {
            const auto nFirstRowSize = (!m_rowSizes.empty()) ? m_rowSizes[0] : 0;
            return m_rowSizes.empty() || std::all_of(m_rowSizes.cbegin(), m_rowSizes.cend(), [=](const Index_T nRowSize) {return nRowSize == nFirstRowSize; });
        }

        Index_T getCellCount() const
        {
            return m_contElems.size();
        }

        Index_T getRowCount() const
        {
            return m_rowSizes.size();
        }

        bool empty() const
        {
            return getCellCount() == 0;
        }

        void clear()
        {
            m_contElems.clear();
            m_rowSizes.clear();
        }

        // TODO: test
        // If there not enough elements to make the last row full, it will have size less than rowLength.
        template <class Iterable_T>
        void setElements(const Iterable_T& data, const Index_T rowLength)
        {
            if (rowLength <= 0)
            {
                clear();
                return;
            }
            m_contElems.assign(data.cbegin(), data.cend());
            const auto countOfFullRows = m_contElems.size() / rowLength;
            m_rowSizes.assign(countOfFullRows, rowLength);
            const auto excessElems = m_contElems.size() % rowLength;
            if (excessElems > 0)
                m_rowSizes.push_back(excessElems);
            DFG_ASSERT(std::accumulate(m_rowSizes.cbegin(), m_rowSizes.cend(), size_t(0)) == m_contElems.size());
        }

        ElementContainerT m_contElems;
        std::vector<Index_T> m_rowSizes;
    };

    namespace DFG_DETAIL_NS
    {
        template <class Char_T, DFG_MODULE_NS(io)::TextEncoding> class TableInterfaceTypes
        {
        public:
            typedef Char_T* SzPtrW;
            typedef const Char_T* SzPtrR;
            typedef std::basic_string<Char_T> StringT;
        };

        template <> struct TableInterfaceTypes<char, DFG_MODULE_NS(io)::encodingUTF8>
        {
            typedef SzPtrUtf8W SzPtrW;
            typedef SzPtrUtf8R SzPtrR;
            typedef DFG_CLASS_NAME(StringTyped) < CharPtrTypeUtf8 > StringT;
        };
    } // namespace DFG_DETAIL_NS

    // Class for efficiently storing big table of small strings with no embedded nulls.
    template <class Char_T, 
              class Index_T = uint32,
              DFG_MODULE_NS(io)::TextEncoding InternalEncoding_T = DFG_MODULE_NS(io)::encodingUnknown,
              class InterfaceTypes_T = DFG_DETAIL_NS::TableInterfaceTypes<Char_T, InternalEncoding_T>>
    class DFG_CLASS_NAME(TableSz)
    {
    public:
        typedef std::pair<Index_T, const Char_T*> IndexPtrPair;
        typedef std::vector<IndexPtrPair> ColumnIndexPairContainer;
        typedef std::vector<ColumnIndexPairContainer> TableIndexPairContainer;
        typedef std::map<Index_T, std::deque<std::vector<Char_T>>> CharBufferContainer;
        typedef typename InterfaceTypes_T::SzPtrW SzPtrW;
        typedef typename InterfaceTypes_T::SzPtrR SzPtrR;
        typedef typename InterfaceTypes_T::StringT StringT;

        DFG_CLASS_NAME(TableSz)() : 
            m_emptyString('\0'),
            m_nBlockSize(2048),
            m_bAllowStringsLongerThanBlockSize(true)

        {
        }

        void setBlockSize(size_t nBlockSize)
        {
            m_nBlockSize = nBlockSize;
        }
        size_t blockSize() const { return m_nBlockSize; }

        void setAllowBlockSizeExceptions(bool b)
        {
            m_bAllowStringsLongerThanBlockSize = b;
        }
        bool isBlockSizeFixed() const { return m_bAllowStringsLongerThanBlockSize; }

        void privSetRowContent(ColumnIndexPairContainer& rowsInCol, Index_T nRow, const Char_T* pData)
        {
            // If row is non-existing because it is > than any existing row, simply push_back() and return.
            if (rowsInCol.empty() || nRow > rowsInCol.back().first)
            {
                rowsInCol.push_back(IndexPtrPair(nRow, pData));
                return;
            }

            // Check if the given row already exists.
            auto iterGreaterOrEqualToRow = privLowerBoundInColumnNonConst(rowsInCol, nRow);
            if (iterGreaterOrEqualToRow != rowsInCol.end() && iterGreaterOrEqualToRow->first == nRow)
            {
                // Already have given row; overwrite the pointer.
                iterGreaterOrEqualToRow->second = pData;
                return;
            }

            // Didn't have, insert it.
            rowsInCol.insert(iterGreaterOrEqualToRow, IndexPtrPair(nRow, pData));
        }

        template <class Str_T>
        bool setElement(size_t nRow, size_t nCol, const Str_T& sSrc)
        {
            return addString(sSrc, static_cast<Index_T>(nRow), static_cast<Index_T>(nCol));
        }

        // Sets element to (nRow, nCol). This does not invalidate any previous pointers return by operator()(), but after this call all pointers returned for (nRow, nCol)
        // will point to the previous element, not current.
        // If element at (nRow, nCol) already exists, it is overwritten.
        // Return: true if string was added, false otherwise.
        // Note: Even in case of overwrite, previous item is not cleared from string storage (this is implementation detail that is not part of the interface, i.e. it is not to be relied on).
        bool addString(const DFG_CLASS_NAME(StringView)<Char_T, StringT>& sv, const Index_T nRow, const Index_T nCol)
        {
            if (nCol >= NumericTraits<Index_T>::maxValue) // Guard for nCol + 1 overflow.
                return false;

            if (!isValidIndex(m_colToRows, nCol))
                m_colToRows.resize(nCol + 1);

            const auto nLength = sv.length();

            // Optimization: use shared null for empty items.
            if (nLength == 0)
            {
                privSetRowContent(m_colToRows[nCol], nRow, &m_emptyString);
                return true;
            }

            auto& bufferCont = m_charBuffers[nCol];
            if (bufferCont.empty() || bufferCont.back().capacity() - bufferCont.back().size() < nLength + 1)
            {
                bufferCont.push_back(std::vector<Char_T>());
                const size_t nNewBlockSize = (m_bAllowStringsLongerThanBlockSize) ? Max(m_nBlockSize, nLength + 1) : m_nBlockSize;
                bufferCont.back().reserve(nNewBlockSize);
            }

            auto& currentBuffer = bufferCont.back();
            const auto nBeginIndex = currentBuffer.size();

            const auto nFreeSpace = currentBuffer.capacity() - currentBuffer.size();
            // Make sure that there's enough space in the buffer.
            // This should trigger only if nLength >= m_nBlockSize and strings longer than block length is not allowed.
            // Note that reallocation is not allowed because it would invalidate existing data pointers.
            if (nLength >= nFreeSpace)
                return false;
            currentBuffer.insert(currentBuffer.end(), toCharPtr_raw(sv.begin()), toCharPtr_raw(sv.end()));
            currentBuffer.push_back('\0');
            const Char_T* const pData = &currentBuffer[nBeginIndex];

            privSetRowContent(m_colToRows[nCol], nRow, pData);

            return true;
        }

        // Shared implementation for const/non-const cases.
        template <class This_T, class Func_T>
        static void privForEachFwdColumnIndexImpl(This_T& rThis, Func_T&& func)
        {
            Index_T i = 0;
            for (auto iter = rThis.m_colToRows.begin(), iterEnd = rThis.m_colToRows.end(); iter != iterEnd; ++iter, ++i)
            {
                func(i);
            }
        }

        template <class Func_T>
        void forEachFwdColumnIndex(Func_T&& func)
        {
            privForEachFwdColumnIndexImpl(*this, std::forward<Func_T>(func));
        }

        template <class Func_T>
        void forEachFwdColumnIndex(Func_T&& func) const
        {
            privForEachFwdColumnIndexImpl(*this, std::forward<Func_T>(func));
        }

        // Functor is given two parameters: row index and null terminated string for cell in (row, nCol).
        template <class Func_T>
        void forEachFwdRowInColumn(const Index_T nCol, Func_T&& func)
        {
            if (!isValidIndex(m_colToRows, nCol))
                return;

            for (auto iter = m_colToRows[nCol].begin(), iterEnd = m_colToRows[nCol].end(); iter != iterEnd; ++iter)
                func(iter->first, SzPtrR(iter->second));
        }

        // const-overload.
        template <class Func_T>
        void forEachFwdRowInColumn(const Index_T nCol, Func_T&& func) const
        {
            if (!isValidIndex(m_colToRows, nCol))
                return;

            for (auto iter = m_colToRows[nCol].begin(), iterEnd = m_colToRows[nCol].end(); iter != iterEnd; ++iter)
                func(iter->first, SzPtrR(iter->second));
        }

        // Visits all cells that have non-null ptr in unspecified order.
        template <class Func_T>
        void forEachNonNullCell(Func_T&& func) const
        {
            forEachFwdColumnIndex([&](const Index_T nCol)
            {
                forEachFwdRowInColumn(nCol, [&](const Index_T nRow, SzPtrR tpsz)
                {
                    if (tpsz) // Not sure is this test needed.
                        func(nRow, nCol, tpsz);
                });
            });
        }

        // Returns row count defining it by maximum row index in all columns.
        // Note: there's no simple rowCount() function because it's not clearly definable: for example if the table has only one cell and it's located at row 4, 
        //       rowCount could be interpreted as 1 or as this function returns, 5.
        // TODO: test
        Index_T rowCountByMaxRowIndex() const
        {
            Index_T nRowCount = 0;
            const auto nCount = m_colToRows.size();
            for (size_t i = 0; i < nCount; ++i)
            {
                if (!m_colToRows[i].empty())
                    nRowCount = Max(nRowCount, m_colToRows[i].back().first + 1);
            }
            return nRowCount;
        }

        Index_T colCountByMaxColIndex() const
        {
            return (!m_colToRows.empty()) ? static_cast<Index_T>(m_colToRows.size()) : 0;
        }

        void privShiftRowIndexesInRowGreaterOrEqual(Index_T nRow, const Index_T nShift, const bool bPositiveShift)
        {
            for(Index_T i = 0, nCount = static_cast<Index_T>(m_colToRows.size()); i < nCount; ++i)
            {
                auto& colToRows = m_colToRows[i];
                auto iter = privLowerBoundInColumn<typename ColumnIndexPairContainer::iterator>(colToRows, nRow);
                for(; iter != colToRows.end(); ++iter)
                    iter->first = (bPositiveShift) ? iter->first + nShift : iter->first - nShift;
            }
        }

        // TODO: test
        // Note: appending rows at end will actually do nothing at the moment.
        void insertRowsAt(Index_T nRow, Index_T nInsertCount)
        {
            privShiftRowIndexesInRowGreaterOrEqual(nRow, nInsertCount, true);
        }

        void removeRows(Index_T nRow, Index_T nRemoveCount)
        {
            for(auto iterRowCont = m_colToRows.begin(); iterRowCont != m_colToRows.end(); ++iterRowCont)
            {
                auto iterFirst = privLowerBoundInColumn<typename ColumnIndexPairContainer::iterator>(*iterRowCont, nRow);
                auto iterEnd = privLowerBoundInColumn<typename ColumnIndexPairContainer::iterator>(*iterRowCont, nRow + nRemoveCount);
                iterRowCont->erase(iterFirst, iterEnd);
            }
            privShiftRowIndexesInRowGreaterOrEqual(nRow + nRemoveCount, nRemoveCount, false);
        }

        // TODO: test
        void insertColumnsAt(Index_T nCol, Index_T nInsertCount)
        {
            const Index_T nColCount = static_cast<Index_T>(m_colToRows.size());
            if (nCol < 0 || nCol > nColCount)
                nCol = nColCount;
            m_colToRows.insert(m_colToRows.begin() + nCol, nInsertCount, ColumnIndexPairContainer());

            // Increment indexes of existing char buffer columns whose index is out-of-date after insertion. Implementation uses temporary map.
            CharBufferContainer tempBuffers;
            for(auto iter = m_charBuffers.begin(); iter != m_charBuffers.end(); ++iter)
            {
                const auto n = iter->first;
                if (n < nCol)
                    tempBuffers[n].swap(m_charBuffers[n]);
                else
                    tempBuffers[n + nInsertCount].swap(m_charBuffers[n]);
            }
            m_charBuffers.swap(tempBuffers);
        }

        // TODO: test
        void eraseColumnsByPosAndCount(Index_T nCol, Index_T nRemoveCount)
        {
            const Index_T nColCount = static_cast<Index_T>(m_colToRows.size());
            if (nCol < 0 || nCol > nColCount)
                nCol = nColCount;
            nRemoveCount = Min(nRemoveCount, nColCount - nCol);
            m_colToRows.erase(m_colToRows.begin() + nCol, m_colToRows.begin() + nCol + nRemoveCount);
            // Move char arrays whose index is more than nCol + nRemoveCount to match with new index, i.e. nCol -> nCol - nRemoveCount.
            auto iter = m_charBuffers.lower_bound(nCol + nRemoveCount);
            for(; iter != m_charBuffers.end(); ++iter)
            {
                const auto nThisIndex = iter->first;
                const auto nTargetIndex = nThisIndex - nRemoveCount;
                m_charBuffers[nTargetIndex].swap(m_charBuffers[nThisIndex]);
            }
            // Remove unused char buffers.
            while(!m_charBuffers.empty() && m_charBuffers.rbegin()->first >= static_cast<Index_T>(m_colToRows.size()))
            {
                auto iterLast = m_charBuffers.end();
                m_charBuffers.erase(--iterLast);
            }
        }

        // Erases cell at (row, col) so that after this operator()(row, col) returns nullptr.
        // TODO: test
        void eraseCell(const Index_T row, const Index_T col)
        {
            auto iter = privIteratorToIndexPair(row, col);
            if (iter.first)
                m_colToRows[col].erase(iter.second);
        }

        // Returns either pointer to null terminated string or nullptr, if no element exists.
        // Note: Returned pointer remains valid even if adding new strings. For behaviour in case of 
        //       overwriting item at (row, col), see documentation for AddString.
        SzPtrR operator()(Index_T row, Index_T col) const
        {
            auto iter = privIteratorToIndexPair(row, col);
            return (iter.first && iter.second->first == row) ? SzPtrR(iter.second->second) : SzPtrR(nullptr);
        }

        template <class Iter_T, class ColumnIndexPairContainer_T>
        static Iter_T privLowerBoundInColumn(ColumnIndexPairContainer_T& cont, const Index_T nRow)
        {
            const IndexPtrPair searchItem(nRow, nullptr);
            const auto pred = [](const IndexPtrPair& a, const IndexPtrPair& b) {return a.first < b.first; };
            auto iter = std::lower_bound(cont.begin(), cont.end(), searchItem, pred);
            return iter;
        }

        typename ColumnIndexPairContainer::iterator privLowerBoundInColumnNonConst(ColumnIndexPairContainer& cont, const Index_T nRow)
        {
            return privLowerBoundInColumn<typename ColumnIndexPairContainer::iterator>(cont, nRow);
        }

        typename ColumnIndexPairContainer::const_iterator privLowerBoundInColumnConst(ColumnIndexPairContainer& cont, const Index_T nRow)
        {
            return privLowerBoundInColumn<typename ColumnIndexPairContainer::const_iterator>(static_cast<const ColumnIndexPairContainer&>(cont), nRow);
        }

        template <class Iterator_T, class ThisClass>
        static std::pair<bool, Iterator_T> privIteratorToIndexPairImpl(ThisClass& rThis, const Index_T row, const Index_T col)
        {
            if (!isValidIndex(rThis.m_colToRows, col))
                return std::pair<bool, Iterator_T>(false, ColumnIndexPairContainer().begin());
            auto& colToRowCont = rThis.m_colToRows[col];
            auto iter = privLowerBoundInColumn<Iterator_T>(colToRowCont, row);
            return std::pair<bool, Iterator_T>(iter != colToRowCont.end(), iter);
        }

        std::pair<bool, typename ColumnIndexPairContainer::iterator> privIteratorToIndexPair(const Index_T row, const Index_T col)
        {
            return privIteratorToIndexPairImpl<typename ColumnIndexPairContainer::iterator>(*this, row, col);
        }

        std::pair<bool, typename ColumnIndexPairContainer::const_iterator> privIteratorToIndexPair(const Index_T row, const Index_T col) const
        {
            return privIteratorToIndexPairImpl<typename ColumnIndexPairContainer::const_iterator>(*this, row, col);
        }

        Index_T cellCountNonEmpty() const
        {
            Index_T nCount = 0;
            forEachFwdColumnIndex([&](Index_T col)
            {
                forEachFwdRowInColumn(col, [&](const Index_T, const SzPtrR psz)
                {
                    if (!DFG_MODULE_NS(str)::isEmptyStr(psz))
                        nCount++;
                });
            });
            return nCount;
        }

        // Returns content storage size in bytes. Note that returned value includes nulls and possibly content from removed cells.
        size_t contentStorageSizeInBytes() const
        {
            size_t nByteCount = 0;
            for (auto iter = m_charBuffers.cbegin(), iterEnd = m_charBuffers.cend(); iter != iterEnd; ++iter)
            {
                for (auto iter2 = iter->second.cbegin(), iter2End = iter->second.cend(); iter2 != iter2End; ++iter2)
                {
                    nByteCount += iter2->size();
                }
            }
            return nByteCount;
        }

        void clear()
        {
            m_charBuffers.clear();
            m_colToRows.clear();
        }

        void swapCellContentInColumn(ColumnIndexPairContainer& colItems, const Index_T r0, const Index_T r1)
        {
            auto iterA = privLowerBoundInColumn<typename ColumnIndexPairContainer::iterator>(colItems, r0);
            auto iterB = privLowerBoundInColumn<typename ColumnIndexPairContainer::iterator>(colItems, r1);
            const bool br0Match = (iterA != colItems.end() && iterA->first == r0);
            const bool br1Match = (iterB != colItems.end() && iterB->first == r1);
            if (!br0Match && !br1Match) // Check whether of neither cell has content, no swapping is needed in that case.
                return;
            if (!br0Match)
            {
                privSetRowContent(colItems, r0, iterB->second); // Note: this may invalidate iters.
                privSetRowContent(colItems, r1, nullptr);
            }
            else if (!br1Match)
            {
                privSetRowContent(colItems, r1, iterA->second); // Note: this may invalidate iters.
                privSetRowContent(colItems, r0, nullptr);
            }
            else
                std::swap(iterA->second, iterB->second);
        }

        // TODO: stable sorting.
        template <class Pred>
        void sortByColumn(Index_T nCol, Pred&& pred)
        {
            if (!DFG_ROOT_NS::isValidIndex(m_colToRows, nCol))
                return;
            const auto nCount = rowCountByMaxRowIndex();
            auto& colItems = m_colToRows[nCol];
            auto indexes = DFG_MODULE_NS(alg)::computeSortIndexesBySizeAndPred(nCount, [&](const size_t a, const size_t b) -> bool
            {
                auto iterA = privLowerBoundInColumnConst(colItems, static_cast<Index_T>(a));
                auto iterB = privLowerBoundInColumnConst(colItems, static_cast<Index_T>(b));
                auto pA = (iterA != colItems.end()) ? iterA->second : nullptr;
                auto pB = (iterB != colItems.end()) ? iterB->second : nullptr;
                return pred(pA, pB);
            });
            forEachFwdColumnIndex([&](const Index_T nCol)
            {
                typedef Index_T IndexTypedefWorkAroundForVC2010;
                auto& rowAtCol = m_colToRows[nCol];
                DFG_MODULE_NS(alg)::DFG_DETAIL_NS::sortByIndexArray_tN_sN_WithSwapImpl(indexes, [&](size_t a, size_t b)
                {
                    swapCellContentInColumn(rowAtCol, static_cast<IndexTypedefWorkAroundForVC2010>(a), static_cast<IndexTypedefWorkAroundForVC2010>(b));
                });
            });
        }

        void sortByColumn(Index_T nCol)
        {
            sortByColumn(nCol, [](const SzPtrR psz0, const SzPtrR psz1) -> bool
            {
                if (psz0 == nullptr && psz1 != nullptr)
                    return true;
                else if (psz0 != nullptr && psz1 != nullptr)
                    return (DFG_MODULE_NS(str)::strCmp(psz0, psz1) < 0);
                else 
                    return false;
            });
        }

        // TODO: Implement copying and moving. Currently hidden because default copy causes the pointers in m_colToRows in the new
        //       object to refer to the old table strings.
        DFG_HIDE_COPY_CONSTRUCTOR_AND_COPY_ASSIGNMENT(DFG_CLASS_NAME(TableSz));

        public:
        /*
        Storage implementation:
            -m_charBuffers stores null terminated strings in blocks of contiguous memory for each column.
            -m_colToRows[nCol] gives list of (row,psz) pairs ordered by row in column nCol.
            If table has cell at (row,col), it can be accessed by finding row from m_colToRows[nCol].
            Since m_colToRows[nCol] is ordered by row, it can be searched with binary search.
        */
        const Char_T m_emptyString; // Shared empty item.
        CharBufferContainer m_charBuffers;
        TableIndexPairContainer m_colToRows;
        size_t m_nBlockSize;
        bool m_bAllowStringsLongerThanBlockSize; // If false, strings longer than m_nBlockSize can't be added to table.
    };
}} // module cont
