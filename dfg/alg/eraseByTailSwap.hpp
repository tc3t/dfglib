#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(alg)
{
    
    // Erases iterator range from given container by swapping erased items with tail items and erasing tail.
    // This can be used to do O(1) single element erasing from middle of vector if it's ok to change to existing order.
    // Returns iterator to the same index position as iterRangeFirst (i.e. iterRangeFirst - begin() == returnIterator - begin())
    template <class Cont_T, class Iter_T, class SwapFunc_T, class EraseFunc_T>
    Iter_T eraseByTailSwap(Cont_T& cont, Iter_T iterRangeFirst, const Iter_T& iterRangeEnd, SwapFunc_T&& swapFunc, EraseFunc_T&& eraseFunc)
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
            swapFunc(iterRangeFirst, iterSwapTo);
        }
        if (nSwappedCount < nRemoveCount)
            iterSwapTo = iterSwapTo + (nSwappedCount - nRemoveCount);
        eraseFunc(iterSwapTo, cont.end());
        return cont.begin() + nFirst;
    }

} } // module namespace
