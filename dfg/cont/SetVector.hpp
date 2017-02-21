#pragma once

/*
MapSet.hpp

Implements contiguous storage set with the following properties:
-Guarantees contiguous storage.
-Sorting is controlled by a per-object runtime flag and thus searching is either linear or logarithmic.
-Invalidation of iterators is dependent on runtime properties, especially capacity and sorting.
    -Insert to set with size() == capacity() will invalidate all iterators.
    -Insert to unsorted set with size() < capacity() will invalidate only end() iterator.
    -Insert to sorted set with size() < capacity() invalidates iterators only for those items greater in order than new value.
    -Single deletion from unsorted set invalidates only iterators of deleted item, last item and end(). TODO: revise this.
    -Deletion from sorted set invalidates only those items whose key is after deleted key. // TODO: verify behaviour of vector erase.
-Provides reserve() for preallocation.
-Allows values to be modified (programmer responsibility to use it correctly).
-Values can be searched without contructing value_type (for example std::string items can be searched with const char* without constructing std::string)

Related reading and implementations:
-boost::container::flat_set
*/

#include "../dfgDefs.hpp"
#include "../alg/eraseByTailSwap.hpp"
#include "../alg/sortMultiple.hpp"
#include "../build/languageFeatureInfo.hpp"
#include "detail/keyContainerUtils.hpp"
#include <algorithm>
#include <utility>
#include <vector>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    namespace DFG_DETAIL_NS
    {
        template <class T> struct SetVectorTraits
        { 
            typedef T                                   key_type;
            typedef std::vector<key_type>               container; // TODO: use DFG_CLASS_NAME(Vector).
            typedef typename container::iterator        iterator;
            typedef typename container::const_iterator  const_iterator;
            typedef iterator                            key_iterator;
            typedef const_iterator                      const_key_iterator;
        };
    }

    template <class Key_T>
    class DFG_CLASS_NAME(SetVector) : public DFG_DETAIL_NS::KeyContainerBase<DFG_DETAIL_NS::SetVectorTraits<Key_T>>
    {
    public:
        typedef DFG_DETAIL_NS::KeyContainerBase<DFG_DETAIL_NS::SetVectorTraits<Key_T>> BaseClass;
        typedef typename DFG_DETAIL_NS::SetVectorTraits<Key_T>::container   ContainerT;
        typedef typename BaseClass::iterator                                iterator;
        typedef typename BaseClass::const_iterator                          const_iterator;
        typedef typename ContainerT::const_reference                        const_reference;
        typedef typename BaseClass::key_type                                key_type;
        typedef typename BaseClass::key_iterator                            key_iterator;
        typedef typename BaseClass::const_key_iterator                      const_key_iterator;
        typedef key_type                                                    value_type;
        typedef size_t                                                      size_type;

        DFG_CLASS_NAME(SetVector)()
        {}

#if !DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT
        DFG_CLASS_NAME(SetVector)(DFG_CLASS_NAME(SetVector)&& other)
        {
            operator=(std::move(other));
        }

        DFG_CLASS_NAME(SetVector)(const DFG_CLASS_NAME(SetVector)& other)
        {
            operator=(other);
        }

        DFG_CLASS_NAME(SetVector)& operator=(const DFG_CLASS_NAME(SetVector)& other)
        {
            this->m_bSorted = other.m_bSorted;
            m_storage = other.m_storage;
            return *this;
        }

        DFG_CLASS_NAME(SetVector)& operator=(DFG_CLASS_NAME(SetVector)&& other)
        {
            this->m_bSorted = other.m_bSorted;
            m_storage = std::move(other.m_storage);
            return *this;
        }
#endif // DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT

        bool                empty() const       { return m_storage.empty(); }
        size_t              size() const        { return m_storage.size(); }
        void                clear()             { m_storage.clear(); }

        iterator            begin()             { return m_storage.begin(); }
        const_iterator      begin() const       { return m_storage.begin(); }
        const_iterator      cbegin() const      { return m_storage.cbegin(); }
        iterator            end()               { return m_storage.end(); }
        const_iterator      end() const         { return m_storage.end(); }
        const_iterator      cend() const        { return m_storage.cend(); }

        iterator            backIter()          { return end() - 1; }
        const_iterator      backIter() const    { return end() - 1; }

        value_type&         front()             { return *begin(); }
        const value_type&   front() const       { return *begin(); }
        value_type&         back()              { return *backIter(); }
        const value_type&   back() const        { return *backIter(); }

        // For interface compatibility with MapVector.
        iterator            makeIterator(const size_t i)        { return begin() + i; }
        const_iterator      makeIterator(const size_t i) const  { return begin() + i; }

        // For interface compatibility with MapVector.
        key_type&           keyIterValueToKeyValue(key_type& keyVal)               { return keyVal; }
        const key_type&     keyIterValueToKeyValue(const key_type& keyVal) const   { return keyVal; }

        // For interface compatibility with MapVector.
        key_iterator        beginKey()          { return begin(); }
        const_key_iterator  beginKey() const    { return begin(); }
        key_iterator        endKey()            { return end(); }
        const_key_iterator  endKey() const      { return end(); }

        iterator    erase(iterator iterDeleteFrom) { return (iterDeleteFrom != end()) ? erase(iterDeleteFrom, iterDeleteFrom + 1) : end(); }
        template <class T>
        size_type   erase(const T& key) { auto rv = erase(find(key)); return (rv != end()) ? 1 : 0; } // Returns the number of elements removed.
        iterator    erase(iterator iterRangeFirst, const iterator iterRangeEnd)
        {
            if (this->m_bSorted)
                return m_storage.erase(iterRangeFirst, iterRangeEnd);
            else // With unsorted, swap to-be-removed items to end and erase items from end -> O(1) for single element removal.
            {
                return DFG_MODULE_NS(alg)::eraseByTailSwap(*this, iterRangeFirst, iterRangeEnd,
                                                            [&](const iterator& i0, const iterator& i1) { std::iter_swap(i0, i1); },
                                                            [&](const iterator& i0, const iterator& i1) { m_storage.erase(i0, i1); });
            }
        }

        template <class T0, class T1>
        static bool isKeyMatch(const T0& a, const T1& b)
        {
            return DFG_MODULE_NS(alg)::isKeyMatch(a, b);
        }

        template <class This_T, class T>
        static auto findInsertPos(This_T& rThis, const T& key) -> decltype(rThis.begin())
        {
            const auto iterBegin = rThis.begin();
            const auto iterEnd = rThis.end();
            auto iter = DFG_MODULE_NS(alg)::findInsertPosLinearOrBinary(rThis.isSorted(), iterBegin, iterEnd, key);
            return iter;
        }

        template <class T> iterator         find(const T& key)          { return this->findImpl(*this, key); }
        template <class T> const_iterator   find(const T& key) const    { return this->findImpl(*this, key); }

        template <class T> bool hasKey(const T& key) const              { return find(key) != end(); }

        iterator insertNonExistingTo(key_type&& value)
        {
            return insertNonExistingTo(std::move(value), findInsertPos(*this, value));
        }

        iterator insertNonExistingTo(key_type&& value, iterator insertPos)
        { 
            return m_storage.insert(insertPos, std::move(value));
        }

        template <class T>
        std::pair<iterator, bool> insert(const T& newVal)
        {
            auto iter = findInsertPos(*this, newVal);
            if (iter != end() && isKeyMatch(*iter, newVal))
                return std::pair<iterator, bool>(iter, false);
            else
            {
                iter = insertNonExistingTo(this->keyParamToInsertable(newVal), iter);
                return std::pair<iterator, bool>(iter, true);
            }
        }

        std::pair<iterator, bool> insert(key_type&& newVal)
        {
            auto iter = findInsertPos(*this, newVal);
            if (iter != end() && isKeyMatch(*iter, newVal))
                return std::pair<iterator, bool>(iter, false);
            else
            {
                iter = insertNonExistingTo(std::move(newVal), iter);
                return std::pair<iterator, bool>(iter, true);
            }
        }

        iterator insert(const_iterator hint, key_type&& newVal)
        {
            DFG_UNUSED(hint);
            // TODO: implement (i.e. use hint)
            return insert(std::move(newVal)).first;
        }

        template <class T>
        iterator insert(const_iterator hint, const T& newVal)
        {
            DFG_UNUSED(hint);
            // TODO: implement (i.e. use hint)
            return insert(newVal).first;
        }

        void reserve(const size_t nReserve) { m_storage.reserve(nReserve); }

        size_t capacity() const             { return m_storage.capacity(); }

        void sort()
        {
            std::sort(m_storage.begin(), m_storage.end(), [](const value_type& left, const value_type& right)
            {
                return left < right;
            });
        }

        void setSorting(const bool bSort)
        {
            if (bSort == this->m_bSorted)
                return;
            if (bSort)
                sort();
            this->m_bSorted = bSort;
        }

        bool operator==(const DFG_CLASS_NAME(SetVector)& other) const
        {
            if (size() != other.size())
                return false;
            if (this->isSorted() && other.isSorted())
                return std::equal(begin(), end(), other.begin());
            auto pPossiblySorted = (this->isSorted()) ? this : &other;
            auto pNonSorted = (pPossiblySorted == this) ? &other : this;
            const auto iterEnd = pNonSorted->m_storage.end();
            for (auto iter = pNonSorted->m_storage.begin(); iter != iterEnd; ++iter)
            {
                if (!pPossiblySorted->hasKey(*iter))
                    return false;
            }
            return true;
        }

        bool operator!=(const DFG_CLASS_NAME(SetVector)& other) const
        {
            return !(*this == other);
        }

        ContainerT m_storage;
    }; // class SetVector

} } // Module namespace
