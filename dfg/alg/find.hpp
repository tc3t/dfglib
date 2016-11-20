#pragma once

#include "../dfgDefs.hpp"
#include <algorithm>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(alg)
{
    template <class Iter_T, class Key_T, class IterValueTypeToComparable_T>
    Iter_T findLinear(const Iter_T& iterBegin, const Iter_T& iterEnd, const Key_T& key, IterValueTypeToComparable_T valToComparable)
    {
        auto iter = std::find_if(iterBegin, iterEnd, [&](decltype(*iterBegin)& keyItem)
        {
            return valToComparable(keyItem) == key;
        });
        return iter;
    }

    template <class Iter_T, class Key_T, class IterValueTypeToComparable_T>
    Iter_T findBinary(const Iter_T& iterBegin, const Iter_T& iterEnd, const Key_T& key, IterValueTypeToComparable_T valToComparable)
    {
        auto iter = std::lower_bound(iterBegin, iterEnd, key, [&](decltype(*iterBegin)& iterVal, const Key_T& searchKey)
        {
            return valToComparable(iterVal) < searchKey;
        });
        return (iter != iterEnd && valToComparable(*iter) == key) ? iter : iterEnd; // TODO: don't use == if type doesn't have it.
    }

    template <class Iter_T, class Key_T, class IterValueTypeToComparable_T>
    Iter_T findLinearOrBinary(const bool bIsSorted, const Iter_T& iterBegin, const Iter_T& iterEnd, const Key_T& key, IterValueTypeToComparable_T valToComparable)
    {
        if (bIsSorted)
            return findBinary(iterBegin, iterEnd, key, valToComparable);
        else
            return findLinear(iterBegin, iterEnd, key, valToComparable);
    }

    template <class Iter_T, class Key_T>
    Iter_T findLinearOrBinary(const bool bIsSorted, const Iter_T& iterBegin, const Iter_T& iterEnd, const Key_T& key)
    {
        return findLinearOrBinary(bIsSorted, iterBegin, iterEnd, key, [](decltype(*iterBegin)& val) -> decltype(*iterBegin)& { return val; });
    }

} } // module namespace
