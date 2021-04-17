#pragma once

#include "../../dfgDefs.hpp"
#include "../../isValidIndex.hpp"
#include "../TrivialPair.hpp"
#include <memory>
#include <vector>
#include <type_traits>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(cont) { namespace DFG_DETAIL_NS {

class MapBlockIndexCommonBase
{
public:
    // TODO: terminology with index/key is a bit messy.
    using IndexT    = uint32;
    using KeyT      = IndexT;
    using size_type = uint32;
};

template <size_t BlockSize_T>
class MapBlockIndexCompileTimeBase : public MapBlockIndexCommonBase
{
public:
    using BaseClass = MapBlockIndexCommonBase;
    using IndexT    = BaseClass::IndexT;
    using KeyT      = BaseClass::KeyT;
    using size_type = BaseClass::size_type;

    static constexpr IndexT blockSize() { return BlockSize_T; }

    static constexpr bool isBlockSizeCompileTimeDefined() { return true; }

};

class MapBlockIndexDynamicBase : public MapBlockIndexCommonBase
{
public:
    using BaseClass = MapBlockIndexCommonBase;
    using IndexT    = BaseClass::IndexT;
    using KeyT      = BaseClass::KeyT;
    using size_type = BaseClass::size_type;

    MapBlockIndexDynamicBase(const IndexT blockSize = 4096)
        : m_nBlockSize(Max(IndexT(1), blockSize))
    {}

    IndexT blockSize() const { return m_nBlockSize; }

    static constexpr bool isBlockSizeCompileTimeDefined() { return false; }
    
    IndexT m_nBlockSize;
};

// A map class specifically crafted for needs of TableSz
// -Semantically represents a map from index to data.
// -Instead of storing (index, T) pairs, index is stored implicitly by the size of the map
// -Compared to such implementation with std::vector, this divides indexes into blocks of given size
//      -In plain vector implementation, adding one key with value N would need a vector of size N+1.
//      -With MapBlockIndex, this needs N / blockSize() blocks and one allocated block.
//      -Block sizes are fixed so indexing is O(1).
// -In blocks, items with default value are considered non-existing.
template <class T, size_t BlockSize_T>
class MapBlockIndex : public std::conditional<BlockSize_T != 0, MapBlockIndexCompileTimeBase<BlockSize_T>, MapBlockIndexDynamicBase>::type
{
public:
    using BaseClass = typename std::conditional<BlockSize_T != 0, MapBlockIndexCompileTimeBase<BlockSize_T>, MapBlockIndexDynamicBase>::type;
    using IndexT    = typename BaseClass::IndexT;
    using KeyT      = typename BaseClass::KeyT;
    using size_type = typename BaseClass::size_type;
    

    static T defaultValue() { return T(); }

    // Single index block section.
    // While from MapBlockIndex's perspective IndexBlock is always of fixed size, IndexBlock itself uses more detailed
    // accounting in order to avoid allocating before any mapping appear and to avoid the need to always go through the whole block.
    //      -Block's allocated capacity is always either 0 or BlockSize_T.
    //      -Only items in range [0, blockSize()[ are guaranteed to be initialized
    class IndexBlock
    {
    public:      
        static bool isExistingMapping(const T& t)
        {
            return t != defaultValue();
        }

        // Returns true iff this block has no mappings.
        bool empty() const
        {
            return (!m_spStorage || std::all_of(cbegin(), cend(), [](const T& t) { return !isExistingMapping(t); }));
        }

        T*       begin()        { return m_spStorage.get(); }
        const T* begin()  const { return m_spStorage.get(); }
        const T* cbegin() const { return begin(); }
        T*       end()          { return begin() + blockSize(); }
        const T* end()    const { return begin() + blockSize(); }
        const T* cend()   const { return end(); }

        // Returns the number of mappings.
        size_type mappingCount() const
        {
            return (m_spStorage) ? static_cast<size_type>(std::count_if(begin(), begin() + m_nEffectiveBlockSize, [](const T& t) { return isExistingMapping(t); })) : 0;
        }

        // Returns the effective block size.
        size_type blockSize() const
        {
            return m_nEffectiveBlockSize;
        }

        // Returns value at given subindex.
        // Precondition: i >= 0 && i < BlockSize_T
        T value(const IndexT i, const IndexT nBlockSize) const
        {
            DFG_ASSERT_UB(i >= 0 && i < nBlockSize);
            DFG_UNUSED(nBlockSize); // To avoid "unreferenced formal parameter" warning in release builds
            // Note: even if m_spStorage is allocated, can't return m_spStorage[i] directly as values >= m_nEffectiveBlockSize may be uninitialized.
            return (isValidIndex(i)) ? m_spStorage[i] : defaultValue();
        }

        // Returns largest subindex.
        // Precondition: !empty()
        IndexT backIndex() const
        {
            DFG_ASSERT_UB(!empty());
            DFG_ASSERT(m_spStorage[m_nEffectiveBlockSize - 1] != defaultValue());
            return m_nEffectiveBlockSize - 1;
        }

        // Returns value at backIndex()
        // Precondition: !empty()
        T back() const
        {
            DFG_ASSERT_UB(!empty());
            return m_spStorage[m_nEffectiveBlockSize - 1];
        }

        // Returns true iff m_spStorage can be accessed with m_spStorage[nIndex].
        bool isValidIndex(const IndexT nIndex) const
        {
            DFG_ASSERT(nIndex >= m_nEffectiveBlockSize || m_spStorage != nullptr);
            return nIndex < m_nEffectiveBlockSize;
        }

        // Clears mapping at given subindex.
        // Note: does not deallocate even if there remains no mappings in the block.
        void clearMapping(const IndexT nIndex)
        {
            if (!isValidIndex(nIndex))
                return;
            m_spStorage[nIndex] = defaultValue();
            if (nIndex + 1 == m_nEffectiveBlockSize) // If last was removed, adjusting block size.
            {
                while (m_nEffectiveBlockSize > 0 && !isExistingMapping(m_spStorage[m_nEffectiveBlockSize - 1]))
                    --m_nEffectiveBlockSize;
            }
        }

        // Sets mapping at given subinex.
        // Precondition: nIndex >= 0 && nindex < BlockSize_T
        // Precondition: val != defaultValue()
        void setMapping(const IndexT nIndex, const T& val, const IndexT nBlockSize)
        {
            DFG_ASSERT_CORRECTNESS(val != defaultValue());
            DFG_ASSERT_UB(nIndex >= 0 && nIndex < nBlockSize);
            if (!m_spStorage)
                m_spStorage.reset(new T[nBlockSize]);
            if (nIndex > m_nEffectiveBlockSize) // If new index "jumps ahead", initializing value between old backIndex() and new backIndex()
                std::fill(begin() + m_nEffectiveBlockSize, begin() + nIndex, defaultValue());
            m_nEffectiveBlockSize = Max(m_nEffectiveBlockSize, nIndex + 1);
            m_spStorage[nIndex] = val;
        }

        // Visits each subindex [nLowest, backIndex()] in order from last.
        template <class Func_T>
        void forEachKeyUntil_reverse(const IndexT nLowest, Func_T&& func)
        {
            if (m_nEffectiveBlockSize == 0 || nLowest >= m_nEffectiveBlockSize)
                return;
            for (auto i = m_nEffectiveBlockSize; i > nLowest; --i)
            {
                const auto nIndex = i - 1;
                if (isExistingMapping(m_spStorage[nIndex]))
                    func(nIndex, m_spStorage[nIndex]); // Note: func() might change value of m_nEffectiveBlockSize
            }
        }

        // Returns first index if available, else nBlockSize
        IndexT firstIndex(const IndexT nBlockSize) const
        {
            auto iter = std::find_if(cbegin(), cend(), [](const T& t) { return isExistingMapping(t); });
            return (iter != cend()) ? static_cast<IndexT>(iter - begin()) : nBlockSize;
        }

        // Returns next mapped index from given index. If such does not exist, returns BlockSize_T
        IndexT nextIndex(IndexT nIndex, const IndexT nBlockSize) const
        {
            DFG_ASSERT_CORRECTNESS(nIndex < nBlockSize);
            ++nIndex; // Jump to next
            if (nIndex >= m_nEffectiveBlockSize || !m_spStorage)
                return nBlockSize;
            const auto iterEnd = m_spStorage.get() + m_nEffectiveBlockSize;
            auto iter = std::find_if(&m_spStorage[nIndex], iterEnd, [](const T t) { return isExistingMapping(t); });
            return (iter != iterEnd) ? static_cast<IndexT>(iter - m_spStorage.get()) : nBlockSize;
        }

        std::unique_ptr<T[]> m_spStorage;
        IndexT m_nEffectiveBlockSize = 0; // Size by maximum mapped index. If m_nEffectiveBlockSize is > 0, it must imply isExistingMapping(m_spStorage[m_nEffectiveBlockSize - 1]) == true
    }; // IndexBlock

    // Iterates (key, value) pairs like typical map-iterator
    // Includes only minimal functionality and is read-only.
    class Iterator
    {
    public:
        class PairT : public TrivialPair<IndexT, T>
        {
        public:
            using BaseClass = TrivialPair<IndexT, T>;
            using BaseClass::BaseClass; // Inheriting constructor
            const BaseClass* operator->() const { return this; }
        };

        Iterator() = default;

        Iterator(const MapBlockIndex& rCont, const IndexT key)
            : m_pCont(&rCont)
            , m_key(key)
        {}

        bool operator==(const Iterator& other) const
        {
            DFG_ASSERT_CORRECTNESS(this->m_pCont == other.m_pCont);
            return this->m_key == other.m_key;
        }

        bool operator!=(const Iterator& other) const
        {
            return !((*this) == other);
        }

        void operator++()
        {
            DFG_ASSERT_UB(m_pCont != nullptr);
            m_key = m_pCont->nextKey(m_key);
        }

        const PairT operator*() const
        {
            return operator->();
        }

        const PairT operator->() const
        {
            DFG_ASSERT_UB(this->m_pCont != nullptr);
            return PairT(m_key, m_pCont->value(m_key));
        }

        const MapBlockIndex* m_pCont = nullptr;
        IndexT m_key = invalidKey();
    }; // Class Iterator

    using iterator = Iterator;
    using const_iterator = Iterator;

    // For now providing only const_iterators.
    const_iterator begin()  const { return cbegin(); }
    const_iterator end()    const { return cend(); }
    const_iterator cbegin() const { return Iterator(*this, frontKey()); }
    const_iterator cend()   const { return Iterator(*this, invalidKey()); }

    using BaseClass::BaseClass; // Inheriting constructor

    bool empty() const
    {
        return m_blocks.empty() || std::all_of(m_blocks.begin(), m_blocks.end(), [](const IndexBlock& ib) { return ib.empty(); });
    }

    // Returns the number of mappings.
    size_type size() const
    {
        size_type i = 0;
        for (const auto& block : m_blocks)
            i += block.mappingCount();
        return i;
    }

    static IndexT maxKey()     { return maxValueOfType<IndexT>() - 1; }
    static IndexT invalidKey() { return maxKey() + 1; }

    // Sets mapping key -> value, overrides existing if key is already mapped.
    // If value is defaultValue(), clears mapping.
    void set(const KeyT key, T value)
    {
        if (key > maxKey())
            return;
        if (value == defaultValue())
        {
            clearMapping(key);
            return;
        }
        const auto nBlockIndex = blockIndex(key);
        if (!isValidIndex(m_blocks, nBlockIndex))
            m_blocks.resize(nBlockIndex + 1);
        m_blocks[nBlockIndex].setMapping(subIndex(key), value, this->blockSize());
    }

    // Returns value at given key, defaultValue() if mapping at key does not exist.
    T value(const KeyT key) const
    {
        const auto nBlockIndex = blockIndex(key);
        return (isValidIndex(m_blocks, nBlockIndex)) ? m_blocks[nBlockIndex].value(subIndex(key), this->blockSize()) : defaultValue();
    }

    // Returns first key if available, otherwise invalidKey()
    KeyT frontKey() const
    {
        for (const auto& block : m_blocks)
        {
            if (!block.empty())
                return privBlockIndexToLinearIndex(&block - &m_blocks[0]) + block.firstIndex(this->blockSize());
        }
        return invalidKey();
    }

    // Returns last key if available, otherwise invalidKey()
    KeyT backKey() const
    {
        for (auto iter = m_blocks.crbegin(), iterRend = m_blocks.crend(); iter != iterRend; ++iter)
        {
            if (!iter->empty())
                return privBlockIndexToLinearIndex(&*iter - &m_blocks[0]) + iter->backIndex();
        }
        return invalidKey();
    }

    // Clears mapping for key.
    void clearMapping(const KeyT key)
    {
        const auto nBlockIndex = blockIndex(key);
        if (nBlockIndex >= blockCount())
            return;
        m_blocks[nBlockIndex].clearMapping(subIndex(key));
    }

    // Removes keys having two effects:
    //  -All mappings [key, key + nCount[ are removed
    //  -All mappings [key + nCount] will have their key reduced by nCount.
    void removeKeysByPositionAndCount(const KeyT key, IndexT nCount)
    {
        if (nCount <= 0 || empty())
            return;
        const auto origBackKey = backKey();
        if (key > origBackKey)
            return;
        // Overflow guard for nCount
        nCount = Min(nCount, maxKey() - key + 1); // key + nCount - 1 <= maxKey() -> nCount <= maxKey() - key + 1
        this->clearMapping(key);
        const auto nLastToRemove = key + nCount - 1;
        auto newKey = nextKey(key);
        // First clearing old mappings.
        for (; newKey <= nLastToRemove; newKey = nextKey(newKey))
        {
            this->clearMapping(newKey);
        }
        // Now shifting mappings >= key + nCount by -nCount.
        for (; newKey <= origBackKey; newKey = nextKey(newKey))
        {
            this->set(newKey - nCount, this->value(newKey));
            this->clearMapping(newKey);
        }
    }

    // Returns next key after given key. If such does not exist, returns invalidKey()
    KeyT nextKey(const KeyT key) const
    {
        auto nBlockIndex = blockIndex(key);
        const auto nBlockCount = blockCount();
        if (nBlockIndex >= nBlockCount)
            return invalidKey();
        // First checking if the new index in the same block
        auto newSubIndex = m_blocks[nBlockIndex].nextIndex(subIndex(key), this->blockSize());
        if (newSubIndex != this->blockSize())
            return privBlockIndexToLinearIndex(nBlockIndex) + newSubIndex;
        // New index was not in the same block; checking following blocks.
        for (++nBlockIndex; nBlockIndex < nBlockCount; ++nBlockIndex)
        {
            newSubIndex = m_blocks[nBlockIndex].firstIndex(this->blockSize());
            if (newSubIndex != this->blockSize())
                return privBlockIndexToLinearIndex(nBlockIndex) + newSubIndex;
        }
        return invalidKey();
    }

    // Returns the number of blocks; this includes also non-allocated blocks.
    IndexT blockCount() const
    {
        return static_cast<IndexT>(this->m_blocks.size());
    }

    // Inserts indexes at given position. In practice this means that keys in range [startKey, backKey()] are shifted up by nInsertCount.
    // If nInsertCount is more than what fits in to the map, inserts as many as possible so that none of the existing mapping are lost (i.e. old backKey() would be maxKey()).
    void insertKeysByPositionAndCount(const KeyT startKey, IndexT nInsertCount)
    {
        if (empty() || startKey > backKey())
            return;
        // Insert count overflow guard
        nInsertCount = Min(nInsertCount, maxKey() - backKey()); // backKey() + nInsertCount <= maxKey() -> nInsertCount <= maxKey() - backKey()
        const auto nNewBackKey = backKey() + nInsertCount;
        const auto nNewBlockCount = blockIndex(nNewBackKey) + 1;
        m_blocks.resize(nNewBlockCount); // Note: this is important: without this, this->set() below might reallocate and wipe out the items being iterated through.
        forEachKeyUntil_reverse(startKey, [&](const IndexT nIndex, T& val)
        {
            this->set(nIndex + nInsertCount, std::move(val));
            this->clearMapping(nIndex);
        });
    }

    const_iterator find(const KeyT key) const
    {
        auto v = value(key);
        return (v != defaultValue()) ? Iterator(*this, key) : end();
    }

    void clear()
    {
        m_blocks.clear();
    }

 private:
    IndexT blockIndex(const IndexT i) const { return i / this->blockSize(); }
    IndexT subIndex(const IndexT i)   const { return i % this->blockSize(); }

    IndexT privBlockIndexToLinearIndex(const ptrdiff_t n) const
    {
        return static_cast<IndexT>(n * this->blockSize());
    }

    // Visits each key [key, backKey()] in order from last.
    template <class Func_T>
    void forEachKeyUntil_reverse(const KeyT key, Func_T&& func)
    {
        if (empty())
            return;
        const auto nLowestBlock = blockIndex(key);
        const auto nHighestBlock = blockIndex(backKey());
        for (auto i = nHighestBlock + 1; i > nLowestBlock; --i)
        {
            const auto nBlock = i - 1;
            const auto nBaseIndex = privBlockIndexToLinearIndex(nBlock);
            const auto nSubIndexStart = (nBlock != nLowestBlock) ? 0 : subIndex(key);
            m_blocks[nBlock].forEachKeyUntil_reverse(nSubIndexStart, [&](const IndexT nSubIndex, T& val) { func(nBaseIndex + nSubIndex, val); });
        }
    }

    std::vector<IndexBlock> m_blocks;
}; // class MapBlockIndex

} } } // module namespace
