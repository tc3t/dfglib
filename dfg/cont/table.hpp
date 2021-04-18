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
#include <memory>
#include <numeric>
#include "Vector.hpp"
#include "TrivialPair.hpp"
#include "../build/languageFeatureInfo.hpp"
#include "../build/inlineTools.hpp"
#include "MapVector.hpp"
#include "detail/MapBlockIndex.hpp"

#ifdef _MSC_VER
    #define DFG_TABLESZ_INLINING    DFG_FORCEINLINE
#else
    #define DFG_TABLESZ_INLINING
#endif

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
            using StringViewT = StringView<Char_T, StringT>;
        };

        template <> struct TableInterfaceTypes<char, DFG_MODULE_NS(io)::encodingUTF8>
        {
            typedef SzPtrUtf8W SzPtrW;
            typedef SzPtrUtf8R SzPtrR;
            typedef DFG_CLASS_NAME(StringTyped)<CharPtrTypeUtf8> StringT;
            using  StringViewT = StringViewUtf8;
        };

        // Row storage based on AoS map (row, pointer-to-content)
        // -Memory usage per cell (32-bit index, 64-bit ptr): 16 bytes
        // -Memory usage if setting one cell at row N (32-bit index, 64-bit ptr): ~16 bytes
        // -Content access by row: O(log N)
        template <class Index_T, class Char_T>
        class RowToContentMapAoS : public MapVectorAoS<Index_T, const Char_T*>
        {
        public:
            using BaseClass = MapVectorAoS<Index_T, const Char_T*>;

            // Precondition: p != nullptr
            // Precondition: empty() || i > backKey()
            void push_back(const Index_T i, const Char_T* p)
            {
                DFG_ASSERT_UB(p != nullptr);
                DFG_ASSERT_UB(this->empty() || i > this->backKey());
#ifdef _MSC_VER // Manually implement growth factor of 2 on MSVC to get more consistent performance characteristics between compilers.
                if (this->size() == this->capacity())
                    this->reserve(2 * this->capacity());
#endif

                this->m_storage.push_back(TrivialPair<Index_T, const Char_T*>(i, p));
            }

            // Precondition: p != nullptr
            // Precondition: There's no existing key 'i'
            void insertNonExisting(const typename BaseClass::iterator iterInsertPos, Index_T i, const Char_T* p)
            {
                DFG_ASSERT_UB(p != nullptr);
                this->insertNonExistingTo(std::move(i), std::move(p), iterInsertPos);
            }

            template <class This_T>
            static auto lowerBoundImpl(This_T& rThis, const Index_T nRow) -> decltype(rThis.begin())
            {
                using ValueT = typename BaseClass::value_type;
                const ValueT searchItem(nRow, nullptr);
                const auto pred = [](const ValueT& a, const ValueT& b) {return a.first < b.first; };
                return std::lower_bound(rThis.begin(), rThis.end(), searchItem, pred);
            }

            const Char_T* content(const Index_T nRow, Dummy) const
            {
                auto iter = this->find(nRow);
                return (iter != this->end()) ? iter->second : nullptr;
            }

            void privShiftRowIndexesInRowGreaterOrEqual(Index_T nRow, const Index_T nShift, const bool bPositiveShift)
            {
                for (auto iter = this->lowerBound(nRow), iterEnd = this->end(); iter != iterEnd; ++iter)
                    iter->first = (bPositiveShift) ? iter->first + nShift : iter->first - nShift;
            }

            void removeRows(const Index_T nRow, const Index_T nRemoveCount)
            {
                auto iterFirst = this->lowerBound(nRow);
                auto iterEnd = this->lowerBound(nRow + nRemoveCount);
                this->erase(iterFirst, iterEnd);
                privShiftRowIndexesInRowGreaterOrEqual(nRow + nRemoveCount, nRemoveCount, false);
            }

            void insertRowsAt(const Index_T nRow, const Index_T nInsertCount)
            {
                privShiftRowIndexesInRowGreaterOrEqual(nRow, nInsertCount, true);
            }

            // Precondition: iter is dereferencable
            Index_T iteratorToRow(const typename BaseClass::const_iterator iter) const
            {
                return iter->first;
            }

            // Precondition: iter is dereferencable
            void setContent(typename BaseClass::iterator iter, const Index_T nRow, const Char_T* p)
            {
                DFG_UNUSED(nRow);
                iter->second = p;
            }

            void setContent(const Index_T nRow, const Char_T* pData)
            {
                // If row is non-existing because it is > than any existing row, simply push_back() and return.
                if (this->empty() || nRow > this->backKey())
                {
                    this->push_back(nRow, pData);
                    return;
                }

                // Check if the given row already exists.
                auto iterGreaterOrEqualToRow = this->findOrInsertHint(nRow);
                if (iterGreaterOrEqualToRow != this->end() && this->iteratorToRow(iterGreaterOrEqualToRow) == nRow)
                {
                    // Already have given row; overwrite the pointer.
                    setContent(iterGreaterOrEqualToRow, nRow, pData);
                    return;
                }

                // Didn't have given row -> inserting (if there's data)
                if (pData)
                    this->insertNonExisting(iterGreaterOrEqualToRow, nRow, pData);
            }

            void swapRowContents(typename BaseClass::iterator iterA, typename BaseClass::iterator iterB)
            {
                std::swap(iterA->second, iterB->second);
            }

            static constexpr Index_T maxRowCount()
            {
                return maxValueOfType<Index_T>();
            }

            // Precondition: iter must be dereferencable.
            bool isExistingRow(const typename BaseClass::const_iterator) const { return true; }

            typename BaseClass::iterator findOrInsertHint(const Index_T nRow)
            {
                return lowerBound(nRow);
            }

            auto lowerBound(const Index_T nRow)       -> typename BaseClass::iterator       { return lowerBoundImpl(*this, nRow); }
            auto lowerBound(const Index_T nRow) const -> typename BaseClass::const_iterator { return lowerBoundImpl(*this, nRow); }
        }; // class RowToContentMapAoS

        // Row storage based on vector of pointers-to-content. Indexes are embedded in size. i.e. size of the vector is defined by max index (size() == max index + 1)
        // Non-existing mappings are distinguished by nullptr value.
        // -Memory usage per cell (64-bit build): 8 bytes
        // -Memory usage if setting one cell at row N (64-bit build): 8 * N
        // -Content access by row: O(1)
        template <class Index_T, class Char_T>
        class RowToContentMapVector : public Vector<const Char_T*>
        {
        public:
            using BaseClass = Vector<const Char_T*>;

            // Precondition: p != nullptr
            // Precondition: empty() || (i >= size() && i + 1 <= maxValueOfType<Index_T>()
            void push_back(const Index_T i, const Char_T* p)
            {
                DFG_ASSERT_UB(p != nullptr);
                DFG_ASSERT_UB(this->empty() || i >= this->sizeAsIndexT());
                DFG_ASSERT_UB(i <= maxValueOfType<Index_T>() - 1);

#ifdef _MSC_VER // Manually implement growth factor of 2 on MSVC to get more consistent performance characteristics between compilers.
                if (this->size() == this->capacity())
                    this->reserve(2 * this->capacity());
#endif

                if (i == this->size())
                    BaseClass::push_back(p);
                else
                {
                    this->resize(i + 1, nullptr);
                    this->back() = p;
                }
            }

            void setContent(const Index_T nRow, const Char_T* pData)
            {
                if (nRow >= this->sizeAsIndexT())
                    this->push_back(nRow, pData);
                else
                    (*this)[nRow] = pData;
            }

            // Precondition: !empty()
            Index_T backKey() const { return sizeAsIndexT() - 1; }

            Index_T sizeAsIndexT() const { return static_cast<Index_T>(this->size()); }

            const Char_T* content(const Index_T nRow, Dummy) const
            {
                return (isValidIndex(*this, nRow)) ? (*this)[nRow] : nullptr;
            }

            void insertNonExisting(const typename BaseClass::iterator iterInsertPos, Index_T i, const Char_T* p)
            {
                DFG_UNUSED(i);
                this->insert(iterInsertPos, p);
            }

            template <class This_T>
            static auto lowerBoundImpl(This_T& rThis, const Index_T nRow) -> decltype(rThis.begin())
            {
                return rThis.begin() + Min(nRow, rThis.sizeAsIndexT());
            }

            Index_T iteratorToRow(const typename BaseClass::const_iterator iter) const
            {
                return static_cast<Index_T>(iter - this->begin());
            }

            // Precondition: iter can be written to.
            void setContent(typename BaseClass::iterator iter, const Index_T nRow, const Char_T* p)
            {
                DFG_UNUNSED(nRow);
                *iter = p;
            }

            // Precondition: nInsertCount is valid count.
            void insertRowsAt(const Index_T nRow, const Index_T nInsertCount)
            {
                auto iter = lowerBound(nRow);
                static_cast<std::vector<const Char_T*>&>(*this).insert(iter, nInsertCount, static_cast<const Char_T*>(nullptr));
            }

            // Precondition: nRemoveCount is valid count.
            void removeRows(const Index_T nRow, const Index_T nRemoveCount)
            {
                auto iter = lowerBound(nRow);
                this->erase(iter, iter + Min(nRemoveCount, static_cast<Index_T>(this->size() - (iter - this->begin()))));
            }

            void swapRowContents(typename BaseClass::iterator iterA, typename BaseClass::iterator iterB)
            {
                std::swap(*iterA, *iterB);
            }

            static constexpr Index_T maxRowCount()
            {
                return static_cast<Index_T>(Min(saturateCast<size_t>(maxValueOfType<Index_T>()), size_t(100000000))); // Arbitrary limit.
            }

            // Precondition: iter must be dereferencable.
            bool isExistingRow(const typename BaseClass::const_iterator iter) const
            {
                return *iter != nullptr;
            }

            typename BaseClass::iterator find(const Index_T nRow)
            {
                return (isValidIndex(*this, nRow)) ? this->begin() + nRow : this->end();
            }

            typename BaseClass::iterator findOrInsertHint(const Index_T nRow)
            {
                return this->find(nRow);
            }

            auto lowerBound(const Index_T nRow)       -> typename BaseClass::iterator       { return lowerBoundImpl(*this, nRow); }
            auto lowerBound(const Index_T nRow) const -> typename BaseClass::const_iterator { return lowerBoundImpl(*this, nRow); }
        }; // class RowToContentMapVector

        // Row storage based on MapBlockIndex.
        // -Memory usage per cell (64-bit build): ~8 bytes
        // -Memory usage if setting one cell at row N (64-bit build): (N/4096)*16 + 8 * 4096
        // -Content access by row: O(1)
        template <class Index_T, class Char_T, size_t BlockSize_T>
        class RowToContentMapBlockIndex : public DFG_DETAIL_NS::MapBlockIndex<const char*, BlockSize_T>
        {
        public:
            using BaseClass = DFG_DETAIL_NS::MapBlockIndex<const char*, BlockSize_T>;
            using IndexT = typename BaseClass::IndexT;

            // Precondition: p != nullptr
            // Precondition: empty() || (i >= size() && i + 1 <= maxValueOfType<Index_T>()
            void push_back(const Index_T i, const Char_T* p)
            {
                DFG_ASSERT_UB(p != nullptr);
                DFG_ASSERT_UB(this->empty() || i >= this->sizeAsIndexT());
                DFG_ASSERT_UB(i <= maxValueOfType<Index_T>() - 1);
                this->set(i, p);
            }

            // Note: this may be relatively expensive operation.
            Index_T sizeAsIndexT() const { return static_cast<Index_T>(this->size()); }

            const Char_T* content(const Index_T nRow, Dummy) const
            {
                return this->value(static_cast<IndexT>(nRow));
            }

            // Precondition: iter is dereferencable
            Index_T iteratorToRow(const typename BaseClass::const_iterator iter) const
            {
                DFG_ASSERT_UB(iter->first != invalidKey());
                return iter->first;
            }

            void setContent(const Index_T nRow, const Char_T* p)
            {
                this->set(static_cast<IndexT>(nRow), p);
            }

            // Precondition: nInsertCount is valid count.
            void insertRowsAt(const Index_T nRow, const Index_T nInsertCount)
            {
                this->insertKeysByPositionAndCount(nRow, nInsertCount);
            }

            // Precondition: nRemoveCount is valid count.
            void removeRows(const Index_T nRow, const Index_T nRemoveCount)
            {
                this->removeKeysByPositionAndCount(nRow, nRemoveCount);
            }

            void swapRowContents(typename BaseClass::iterator iterA, typename BaseClass::iterator iterB)
            {
                const auto aContent = iterA->second;
                this->set(iterA->first, iterB->second);
                this->set(iterB->first, aContent);
            }

            static Index_T maxRowCount()
            {
                return saturateCast<Index_T>(BaseClass::maxKey() + 1);
            }

            // Precondition: iter must be dereferencable.
            bool isExistingRow(const typename BaseClass::const_iterator iter) const
            {
                return iter->second != nullptr;
            }

            typename BaseClass::const_iterator findOrInsertHint(const Index_T nRow)
            {
                return this->find(nRow);
            }
        }; // class RowToContentMapBlockIndex

    } // namespace DFG_DETAIL_NS

    // Class for efficiently storing big table of small strings with no embedded nulls.
    template <class Char_T, 
              class Index_T = uint32,
              DFG_MODULE_NS(io)::TextEncoding InternalEncoding_T = DFG_MODULE_NS(io)::encodingUnknown,
              class InterfaceTypes_T = DFG_DETAIL_NS::TableInterfaceTypes<Char_T, InternalEncoding_T>>
    class DFG_CLASS_NAME(TableSz)
    {
    public:
        // Stores char content. A simple structure that allocates storage on construction and appends new items to storage.
        // Does not modify already written bytes and does not reallocate -> references to content are valid as long as the objects is alive.
        // Reason for using this instead of std::vector<Char_T> is performance (see benchmarks for details).
        class CharStorageItem
        {
        public:
            CharStorageItem(const size_t nCapacity) :
                m_nSize(0),
                m_nCapacity(nCapacity)
            {
                m_spStorage.reset(new Char_T[m_nCapacity]);
            }

            CharStorageItem(CharStorageItem&& other) DFG_NOEXCEPT_TRUE
            {
                assignImpl(std::move(other));
            }

            CharStorageItem& operator=(CharStorageItem&& other) DFG_NOEXCEPT_TRUE
            {
                if (this != &other)
                    assignImpl(std::move(other));
                return *this;
            }

            size_t capacity() const
            {
                return m_nCapacity;
            }

            size_t size() const
            {
                return m_nSize;
            }

            const Char_T& operator[](const size_t n) const
            {
                DFG_ASSERT_UB(n < m_nSize);
                return m_spStorage[n];
            }

            // Appends characters to storage
            // Precondition: pEnd - pBegin <= m_nCapacity - m_nSize (i.e. there must be enough capacity)
            void append_unchecked(const Char_T* pBegin, const Char_T* pEnd)
            {
                append_unchecked(pBegin, static_cast<size_t>(pEnd - pBegin));
            }

            // Appends characters to storage
            // Precondition: nCount <= m_nCapacity - m_nSize (i.e. there must be enough capacity)
            void append_unchecked(const Char_T* pBegin, size_t nCount)
            {
                DFG_ASSERT_UB(nCount <= m_nCapacity - m_nSize);
#if defined(_MSC_VER)
                auto pDest = &m_spStorage[m_nSize];
                m_nSize += nCount;
                while (nCount--)
                    *pDest++ = *pBegin++;
#else
                memcpy(&m_spStorage[m_nSize], pBegin, nCount * sizeof(Char_T));
                m_nSize += nCount;
#endif
            }

            // Precondition: m_nSize < m_nCapacity (i.e. there must be enough capacity.)
            void append_unchecked(const Char_T c)
            {
                DFG_ASSERT_UB(m_nSize < m_nCapacity);
                m_spStorage[m_nSize++] = c;
            }

            bool hasCapacityFor(const size_t nCount) const
            {
                return nCount <= m_nCapacity - m_nSize;
            }

        private:
            void assignImpl(CharStorageItem&& other) DFG_NOEXCEPT_TRUE
            {
                // Note: if *this has capacity, it is effectively lost
                // (note though that can't just swap() capacity as *this is uninitialized if called from move constructor).
                m_nSize = other.m_nSize;
                m_nCapacity = other.m_nCapacity;
                m_spStorage.swap(other.m_spStorage);
                other.m_nSize = 0;
                other.m_nCapacity = 0;
            }

        private:
            size_t m_nSize;
            size_t m_nCapacity;
            std::unique_ptr<Char_T[]> m_spStorage;
        }; // Class CharStorageItem

        using IndexT = Index_T;
        //typedef DFG_DETAIL_NS::RowToContentMapAoS<Index_T, Char_T> RowToContentMap; // Map-based, good for sparse content.
        //typedef DFG_DETAIL_NS::RowToContentMapVector<Index_T, Char_T> RowToContentMap; // Vector-based, good for dense.
        using MapBlockIndexT = DFG_DETAIL_NS::RowToContentMapBlockIndex<Index_T, Char_T, 2048>;
        using RowToContentMap = MapBlockIndexT; // "Block-vector", dedicated structure made for TableSz

        typedef std::vector<RowToContentMap> TableIndexContainer;
        typedef std::vector<CharStorageItem> CharStorage;
        typedef std::vector<CharStorage> CharStorageContainer;
        typedef typename InterfaceTypes_T::SzPtrW SzPtrW;
        typedef typename InterfaceTypes_T::SzPtrR SzPtrR;
        typedef typename InterfaceTypes_T::StringT StringT;
        using StringViewT = typename InterfaceTypes_T::StringViewT;

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

        template <class Str_T>
        DFG_TABLESZ_INLINING bool setElement(size_t nRow, size_t nCol, const Str_T& sSrc)
        {
            return addString(sSrc, static_cast<Index_T>(nRow), static_cast<Index_T>(nCol));
        }

        // Sets element to (nRow, nCol). This does not invalidate any previous pointers returned by operator()(), but after this call all pointers returned for (nRow, nCol)
        // will point to the previous element, not current.
        // If element at (nRow, nCol) already exists, it is overwritten.
        // If either index is negative, string is not added.
        // Return: true if string was added, false otherwise.
        // Note: Even in case of overwrite, previous item is not cleared from string storage (this is implementation detail that is not part of the interface, i.e. it is not to be relied on).
        bool addString(const StringViewT sv, const Index_T nRow, const Index_T nCol)
        {
            // Checking (row, column) index validity.
            if (nRow < 0 || nCol < 0 || nRow > maxRowIndex() || nCol > maxColumnIndex())
                return false;
            if (!isValidIndex(m_colToRows, nCol))
            {
                DFG_ASSERT_UB(m_colToRows.size() == m_charBuffers.size());
                if (nCol >= NumericTraits<Index_T>::maxValue) // Guard for nCol + 1 overflow.
                    return false;
                m_colToRows.resize(nCol + 1);
                m_charBuffers.resize(nCol + 1);
            }

            const auto nLength = sv.length();

            // Optimization: use shared null for empty items.
            if (nLength == 0)
            {
                m_colToRows[nCol].setContent(nRow, &m_emptyString);
                return true;
            }

            auto& bufferCont = m_charBuffers[nCol];

            // Check whether there's enough space in buffer and allocate new block if not.
            // Note that reallocation is not allowed because it would invalidate existing data pointers.
            if (bufferCont.empty() || !bufferCont.back().hasCapacityFor(nLength + 1))
            {
                const size_t nNewBlockSize = (m_bAllowStringsLongerThanBlockSize) ? Max(m_nBlockSize, nLength + 1) : m_nBlockSize;
                // If content has length greater than block size and block size is not allowed to be exceeded, return false as item can't be added.
                if (nLength >= nNewBlockSize)
                    return false;
                bufferCont.push_back(CharStorageItem(nNewBlockSize));
            }

            auto& currentBuffer = bufferCont.back();
            DFG_ASSERT_UB(currentBuffer.capacity() - currentBuffer.size() > nLength);

            const auto nBeginIndex = currentBuffer.size();

            currentBuffer.append_unchecked(toCharPtr_raw(sv.begin()), nLength);
            currentBuffer.append_unchecked('\0');

            const Char_T* const pData = &currentBuffer[nBeginIndex];

            m_colToRows[nCol].setContent(nRow, pData);

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

        const Char_T* privRowIteratorToRawContent(const Index_T nCol, typename RowToContentMap::const_iterator iter, const DFG_DETAIL_NS::RowToContentMapAoS<Index_T, Char_T>*) const
        {
            DFG_UNUSED(nCol);
            return iter->second;
        }

        const Char_T* privRowIteratorToRawContent(const Index_T nCol, typename RowToContentMap::const_iterator iter, const DFG_DETAIL_NS::RowToContentMapVector<Index_T, Char_T>*) const
        {
            DFG_UNUSED(nCol);
            return *iter;
        }

        const Char_T* privRowIteratorToRawContent(const Index_T nCol, typename RowToContentMap::const_iterator iter, const MapBlockIndexT*) const
        {
            DFG_UNUSED(nCol);
            return iter->second;
        }

        // Precondition: iter must be dereferencable.
        const Char_T* privRowIteratorToRawContent(const Index_T nCol, typename RowToContentMap::const_iterator iter) const
        {
            return privRowIteratorToRawContent(nCol, iter, static_cast<const RowToContentMap*>(nullptr));
        }

        // Precondition: iter must be dereferencable.
        SzPtrR privRowIteratorToContentView(const Index_T nCol, typename RowToContentMap::const_iterator iter) const
        {
            return SzPtrR(privRowIteratorToRawContent(nCol, iter));
        }

        // Precondition: iter is dereferencable.
        Index_T privRowIteratorToRowNumber(const Index_T nCol, typename RowToContentMap::const_iterator iter, const DFG_DETAIL_NS::RowToContentMapAoS<Index_T, Char_T>*) const
        {
            DFG_UNUSED(nCol);
            return iter->first;
        }

        Index_T privRowIteratorToRowNumber(const Index_T nCol, typename RowToContentMap::const_iterator iter, const DFG_DETAIL_NS::RowToContentMapVector<Index_T, Char_T>*) const
        {
            return m_colToRows[nCol].iteratorToRow(iter);
        }

        Index_T privRowIteratorToRowNumber(const Index_T nCol, typename RowToContentMap::const_iterator iter, const MapBlockIndexT*) const
        {
            DFG_UNUSED(nCol);
            return iter->first;
        }

        // Precondition: iter is dereferencable.
        // Precondition: nCol is valid.
        Index_T privRowIteratorToRowNumber(const Index_T nCol, typename RowToContentMap::const_iterator iter) const
        {
            return privRowIteratorToRowNumber(nCol, iter, static_cast<const RowToContentMap*>(nullptr));
        }

        // const-overload.
        template <class Func_T>
        void forEachFwdRowInColumn(const Index_T nCol, Func_T&& func) const
        {
            forEachFwdRowInColumnWhile(nCol, [](Dummy) { return true; }, std::forward<Func_T>(func));
        }

        // Like forEachFwdRowInColumn, but goes through column only as long as whileFunc returns true.
        // whileFunc is given one parameter (row index) and it should return bool.
        // TODO: test
        template <class Func_T, class WhileFunc_T>
        void forEachFwdRowInColumnWhile(const Index_T nCol, WhileFunc_T&& whileFunc, Func_T&& func) const
        {
            if (!isValidIndex(m_colToRows, nCol))
                return;

            const auto& rowContent = m_colToRows[nCol];
            for (auto iter = rowContent.begin(), iterEnd = rowContent.end(); iter != iterEnd && whileFunc(rowContent.iteratorToRow(iter)); ++iter)
            {
                if (rowContent.isExistingRow(iter))
                    func(rowContent.iteratorToRow(iter), privRowIteratorToContentView(nCol, iter));
            }
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
                    nRowCount = Max(nRowCount, static_cast<IndexT>(m_colToRows[i].backKey() + 1));
            }
            return nRowCount;
        }

        Index_T colCountByMaxColIndex() const
        {
            DFG_ASSERT_CORRECTNESS(m_colToRows.size() == m_charBuffers.size());
            return static_cast<Index_T>(m_colToRows.size());
        }

        Index_T maxRowCount() const { return RowToContentMap::maxRowCount(); }
        Index_T maxRowIndex() const { return maxRowCount() - IndexT(1); }

        Index_T maxColumnCount() const { return Min(maxValueOfType<IndexT>(), saturateCast<IndexT>(m_colToRows.max_size()), saturateCast<IndexT>(m_charBuffers.max_size())); }
        Index_T maxColumnIndex() const { return maxColumnCount() - IndexT(1); }

        // TODO: test
        // Note: appending rows at end will actually do nothing at the moment.
        void insertRowsAt(Index_T nRow, Index_T nInsertCount)
        {
            if (nInsertCount <= 0)
                return;
            const auto nCurrentCount = rowCountByMaxRowIndex();
            if (nRow < 0 || nRow > nCurrentCount)
                nRow = nCurrentCount;
            nInsertCount = Min(nInsertCount, static_cast<IndexT>(maxRowCount() - nCurrentCount));
            forEachFwdColumnIndex([&](const Index_T nCol)
            {
                this->m_colToRows[nCol].insertRowsAt(nRow, nInsertCount);
            });
        }

        void removeRows(Index_T nRow, Index_T nRemoveCount)
        {
            if (nRow < 0 || nRemoveCount <= 0 || nRow > maxRowIndex())
                return;
            limitMax(nRemoveCount, static_cast<IndexT>(maxRowCount() - nRow));
            forEachFwdColumnIndex([&](const Index_T nCol)
            {
                this->m_colToRows[nCol].removeRows(nRow, nRemoveCount);
            });
        }

        void insertColumnsAt(Index_T nCol, Index_T nInsertCount)
        {
            if (nInsertCount <= 0)
                return;
            DFG_ASSERT_UB(m_colToRows.size() == m_charBuffers.size());
            const Index_T nColCount = static_cast<Index_T>(m_colToRows.size());
            if (nCol < 0 || nCol > nColCount)
                nCol = nColCount;
            nInsertCount = Min(nInsertCount, maxColumnCount() - nColCount);

            // Inserting new columns to m_colToRows. Since insert() doesn't work in case of move-only m_colToRows-items, doing it
            // with resize() & rotate()
            //m_colToRows.insert(m_colToRows.begin() + nCol, nInsertCount, RowToContentMap());
            m_colToRows.resize(m_colToRows.size() + static_cast<size_t>(nInsertCount));
            std::rotate(m_colToRows.begin() + nCol, m_colToRows.end() - nInsertCount, m_colToRows.end());

            // Insert new columns. Note that can't use insert(iterStart, iterEnd, val) because CharStorage() is not copy-assignable.
            m_charBuffers.reserve(m_charBuffers.size() + nInsertCount);
            for (Index_T n = 0; n < nInsertCount; ++n)
                m_charBuffers.insert(m_charBuffers.begin() + nCol, CharStorage());

            DFG_ASSERT_UB(m_colToRows.size() == m_charBuffers.size());
        }

        void eraseColumnsByPosAndCount(Index_T nCol, Index_T nRemoveCount)
        {
            DFG_ASSERT_UB(m_colToRows.size() == m_charBuffers.size());
            const Index_T nColCount = static_cast<Index_T>(m_colToRows.size());
            if (nCol < 0 || nCol > nColCount)
                nCol = nColCount;
            nRemoveCount = Min(nRemoveCount, nColCount - nCol);
            m_colToRows.erase(m_colToRows.begin() + nCol, m_colToRows.begin() + nCol + nRemoveCount);
            m_charBuffers.erase(m_charBuffers.begin() + nCol, m_charBuffers.begin() + nCol + nRemoveCount);
        }

        // Returns either pointer to null terminated string or nullptr, if no element exists.
        // Note: Returned pointer remains valid even if adding new strings. For behaviour in case of 
        //       overwriting item at (row, col), see documentation for addString.
        SzPtrR operator()(Index_T row, Index_T col) const
        {
            if (!isValidIndex(m_colToRows, col))
                return SzPtrR(nullptr);
            const auto& colContent = m_colToRows[col];
            return SzPtrR(colContent.content(row, m_charBuffers));
        }

        // Returns the number of non-empty cells in the table.
        // Note: if number does not fit to IndexT, return value is unspecified.
        Index_T cellCountNonEmpty() const
        {
            typename std::make_unsigned<IndexT>::type nCount = 0;
            forEachFwdColumnIndex([&](Index_T col)
            {
                forEachFwdRowInColumn(col, [&](const Index_T, const SzPtrR psz)
                {
                    if (!DFG_MODULE_NS(str)::isEmptyStr(psz))
                        nCount++;
                });
            });
            return saturateCast<IndexT>(nCount);
        }

        // Returns content storage size in bytes. Note that returned value includes nulls and possibly content from removed cells.
        size_t contentStorageSizeInBytes() const
        {
            size_t nByteCount = 0;
            for (auto iter = m_charBuffers.cbegin(), iterEnd = m_charBuffers.cend(); iter != iterEnd; ++iter)
            {
                for (auto iter2 = iter->cbegin(), iter2End = iter->cend(); iter2 != iter2End; ++iter2)
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

        void swapCellContentInColumn(const Index_T nCol, RowToContentMap& colItems, const Index_T r0, const Index_T r1)
        {
            auto iterA = colItems.find(r0);
            auto iterB = colItems.find(r1);
            const bool br0Match = (iterA != colItems.end());
            const bool br1Match = (iterB != colItems.end());
            if (!br0Match && !br1Match) // Checking if  neither cell has content, swapping not needed in that case.
                return;
            if (!br0Match)
            {
                colItems.setContent(r0, privRowIteratorToContentView(nCol, iterB));
                colItems.setContent(r1, nullptr);
            }
            else if (!br1Match)
            {
                colItems.setContent(r1, privRowIteratorToContentView(nCol, iterA));
                colItems.setContent(r0, nullptr);
            }
            else
                colItems.swapRowContents(iterA, iterB);
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
                auto iterA = colItems.find(static_cast<Index_T>(a));
                auto iterB = colItems.find(static_cast<Index_T>(b));
                auto pA = (iterA != colItems.end()) ? privRowIteratorToContentView(nCol, iterA) : nullptr;
                auto pB = (iterB != colItems.end()) ? privRowIteratorToContentView(nCol, iterB) : nullptr;
                return pred(pA, pB);
            });
            forEachFwdColumnIndex([&](const Index_T nCol)
            {
                typedef Index_T IndexTypedefWorkAroundForVC2010;
                auto& rowAtCol = m_colToRows[nCol];
                DFG_MODULE_NS(alg)::DFG_DETAIL_NS::sortByIndexArray_tN_sN_WithSwapImpl(indexes, [&](size_t a, size_t b)
                {
                    swapCellContentInColumn(nCol, rowAtCol, static_cast<IndexTypedefWorkAroundForVC2010>(a), static_cast<IndexTypedefWorkAroundForVC2010>(b));
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
            -m_charBuffers is a map column -> CharStorage,  where CharStorage's store null terminated strings in blocks of contiguous memory for each column.
            -m_colToRows[nCol] gives list of (row,psz) pairs ordered by row in column nCol.
            If table has cell at (row,col), it can be accessed by finding row from m_colToRows[nCol].
            Since m_colToRows[nCol] is ordered by row, it can be searched with binary search.
        */
        const Char_T m_emptyString; // Shared empty item.
        CharStorageContainer m_charBuffers;
        TableIndexContainer m_colToRows;
        size_t m_nBlockSize;
        bool m_bAllowStringsLongerThanBlockSize; // If false, strings longer than m_nBlockSize can't be added to table.
    }; // Class TableSz
}} // module cont
