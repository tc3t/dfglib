#pragma once

#include "../dfgDefs.hpp"
#include <iterator>
#include <utility>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(alg) {

    /*
     * Given range [begin, end) which is sorted if excluding item at 'i', sorts the range so that after this call the whole range is sorted according to 'comp'.
     *
     * Note: For convenience, if 'i' is equal to end(iterable) and iterable is non-empty, 'i' is interpreted as i - 1.
     * Time: O(n) (worst case).
     * Space: O(1)
     * Precondition: 'iterable' excluding the item at 'i' must be in sorted order with respect to predicate 'comp'.
     * Return value: iterator to new position of 'i'.
     * Example usage: 
     *      std::vector<int> a;
     *      // add some elements to a
     *      std::sort(a);
     *      a.push_back(some_int);
     *      sortSingleItem(a, a.end());
     * TODO: document sort stability.
     * TODO: test custom comp
     * TODO: revise and document comp.
    */
    template <class Iterable_T, class Iter_T, class PredicateT>
    typename std::remove_reference<Iter_T>::type sortSingleItem(Iterable_T&& iterable, Iter_T i, PredicateT comp)
    {
        const auto iBegin = std::begin(iterable);
        const auto iEnd = std::end(iterable);
        if (iBegin == iEnd) // Empty range?
            return iEnd;

        if (i == iEnd)
            --i;

        auto iNext = i;
        std::advance(iNext, 1);
        if (iNext != iEnd && comp(*iNext, *i)) // Next is less than current -> move item forward
        {
            for (; iNext != iEnd && comp(*iNext, *i); ++i, ++iNext)
            {
                std::swap(*iNext, *i);
            }
        }
        else if (i != iBegin) // Case: Item can't be moved forward, check backward.
        {
            auto iPrev = i;
            std::advance(iPrev, -1);
            for (; comp(*i, *iPrev); --i, --iPrev)
            {
                std::swap(*iPrev, *i);
                if (iPrev == iBegin)
                {
                    --i;
                    break;
                }
            }
        }
        return i;
    }

    template <class Iterable_T, class Iter_T>
    typename std::remove_reference<Iter_T>::type sortSingleItem(Iterable_T&& iterable, Iter_T&& i)
    {
        return sortSingleItem(std::forward<Iterable_T>(iterable), std::forward<Iter_T>(i), std::less<decltype(*i)>());
    }

} } // module namespace