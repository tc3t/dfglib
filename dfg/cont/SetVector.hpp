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
#include "../alg/sortMultiple.hpp"
#include "../alg/sortSingleItem.hpp"
#include <algorithm>
#include <utility>
#include <vector>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    template <class Cont_T, class Iter_T, class Swaper_T, class Eraser_T>
    Iter_T endSwapErase(Cont_T& cont, Iter_T iterRangeFirst, const Iter_T& iterRangeEnd, Swaper_T swap, Eraser_T erase)
    {
        if (iterRangeFirst == iterRangeEnd)
            return iterRangeFirst;
        const auto nFirst = iterRangeFirst - cont.begin();
        const auto nRemoveCount = iterRangeEnd - iterRangeFirst;
        const auto nSwapRoom = cont.end() - iterRangeEnd;
        auto iterSwapTo = cont.end();
        ptrdiff_t nSwappedCount = 0;
        for (; iterRangeFirst != iterRangeEnd && nSwappedCount < nSwapRoom; ++iterRangeFirst, ++nSwappedCount)
        {
            --iterSwapTo;
            swap(iterRangeFirst, iterSwapTo);
        }
        if (nSwappedCount < nRemoveCount)
            iterSwapTo = iterSwapTo + (nSwappedCount - nRemoveCount);
        erase(iterSwapTo, cont.end());
        return cont.begin() + nFirst;
    }

    template <class Key_T>
    class DFG_CLASS_NAME(SetVector)
    {
    public:
        typedef std::vector<Key_T>                  ContainerT;
        typedef typename ContainerT::iterator       iterator;
        typedef typename ContainerT::const_iterator const_iterator;
        typedef Key_T                               key_type;
        typedef Key_T                               value_type;
        typedef size_t                              size_type;

        DFG_CLASS_NAME(SetVector)() :
            m_bSorted(true)
        {}

        bool                empty() const       { return m_storage.empty(); }
        size_t              size() const        { return m_storage.size(); }

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

        iterator    erase(iterator iterDeleteFrom) { return (iterDeleteFrom != end()) ? erase(iterDeleteFrom, iterDeleteFrom + 1) : end(); }
        template <class T>
        size_type   erase(const T& key) { auto rv = erase(find(key)); return (rv != end()) ? 1 : 0; } // Returns the number of elements removed.
        iterator    erase(iterator iterRangeFirst, const iterator iterRangeEnd)
        {
            if (m_bSorted)
                return m_storage.erase(iterRangeFirst, iterRangeEnd);
            else // With unsorted, swap to-be-removed items to end and erase items from end -> O(1) for single element removal.
            {
                return endSwapErase(*this, iterRangeFirst, iterRangeEnd,
                                    [&](const iterator& i0, const iterator& i1) { std::iter_swap(i0, i1); },
                                    [&](const iterator& i0, const iterator& i1) { m_storage.erase(i0, i1); });
            }
        }

        template <class This_T, class T>
        static auto findImpl(This_T& rThis, const T& key) -> decltype(rThis.begin())
        {
            const auto iterBegin = rThis.begin();
            const auto iterEnd = rThis.end();
            if (rThis.m_bSorted)
            {
                auto iter = std::lower_bound(iterBegin, iterEnd, key, [&](const value_type& keyItem, const T& searchKey)
                {
                    return keyItem < searchKey;
                });
                return (iter != iterEnd && *iter == key) ? iter : rThis.end();
            }
            else // case: not sorted.
            {
                auto iter = std::find_if(iterBegin, iterEnd, [&](const value_type& keyItem)
                {
                    return keyItem == key;
                });
                return iter;
            }
        }

        template <class T> iterator         find(const T& key)          { return findImpl(*this, key); }
        template <class T> const_iterator   find(const T& key) const    { return findImpl(*this, key); }

        template <class T> bool hasKey(const T& key) const              { return find(key) != end(); }

        iterator insertNonExisting(key_type&& value)
        { 
            m_storage.push_back(std::move(value));
            if (m_bSorted)
            {
                auto iter = ::DFG_MODULE_NS(alg)::sortSingleItem(m_storage, backIter());
                return iter;
            }
            else
                return backIter();
        }

        template <class T>
        std::pair<iterator, bool> insert(const T& newVal)
        {
            auto iter = find(newVal);
            if (iter != end())
                return std::pair<iterator, bool>(iter, false);
            else
            {
                iter = insertNonExisting(key_type(newVal));
                return std::pair<iterator, bool>(iter, true);
            }
        }

        template <class T>
        std::pair<iterator, bool> insert(key_type&& newVal)
        {
            auto iter = find(newVal);
            if (iter != end())
                return std::pair<iterator, bool>(iter, false);
            else
            {
                iter = insertNonExisting(std::move(newVal));
                return std::pair<iterator, bool>(iter, true);
            }
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
            if (bSort == m_bSorted)
                return;
            if (bSort)
                sort();
            m_bSorted = bSort;
        }

        bool isSorted() const { return m_bSorted; }

        bool m_bSorted;
        ContainerT m_storage;
    }; // class SetVector

} } // Module namespace
