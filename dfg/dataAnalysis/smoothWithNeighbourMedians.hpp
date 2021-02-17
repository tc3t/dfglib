#pragma once

#include "../dfgDefs.hpp"
#include "../cont/arrayWrapper.hpp"
#include "../dfgBase.hpp"
#include "../cont/elementType.hpp"
#include "../cont/SortedSequence.hpp"
#include "../numeric/median.hpp"
#include "../math.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(dataAnalysis) {

// Convert values in an container into a median values of local neighbour window.
// For example with value in the middle of container [k] -> median([k-nWindowSize], ..., [k], ..., [k+nWindowSize])
// On boundaries the median is calculated from as many elements as there are in the window. For example on first element
// window consists only of right side neighbours, on second there's only one left neighbour.
// 
//  -NaN handling: NaN's occupy space from smooth window, but value is ignored effectively making smooth window element count smaller.
//                 NaN are not replaced by smoothed values.
// TODO: Write using iterators so that the algorithm works also for non-indexed containers.
template <class Cont_T>
void smoothWithNeighbourMedians(Cont_T&& cont, const size_t nWindowRadiusRequest = 1)
{
    using namespace ::DFG_MODULE_NS(cont);
    using namespace ::DFG_MODULE_NS(math);
    typedef typename ElementType<Cont_T>::type ValueT;

    const auto nSize = ::DFG_ROOT_NS::count(cont);

    if (nWindowRadiusRequest < 1 || nSize <= 1)
    {
        return;
    }

    const auto nWindowRadius = Min(nWindowRadiusRequest, nSize - 1);

    // If nWindowRadius is small, using this non-heap allocated array.
    ValueT originalValuesStatic[3] = { 0, 0, 0 };
    std::vector<ValueT> vecOriginalValues;
    if (nWindowRadius >= DFG_COUNTOF(originalValuesStatic))
        vecOriginalValues.resize(nWindowRadius + 1, 0);
    ArrayWrapperT<ValueT> originalVals((vecOriginalValues.empty()) ? originalValuesStatic : &vecOriginalValues[0], nWindowRadius + 1);

    SortedSequence<std::vector<ValueT>> window;

    const auto nMaxWindowSize = Min(nSize, 1 + 2 * nWindowRadius);
    window.reserve(nMaxWindowSize);

    // Populating all put last in the window
    for (size_t i = 0; i < nWindowRadius; ++i)
    {
        if (!isNan(cont[i]))
            window.insert(cont[i]);
    }
    size_t nFirstInWindow = 0;
    size_t nLastInWindow = nWindowRadius - 1;
    const size_t nLastIndex = cont.size() - 1;

    for (size_t iCont = 0; iCont < nSize; ++iCont)
    {
        const auto lowerWindowItem = (iCont >= nWindowRadius) ? iCont - nWindowRadius : 0;

        if (nLastInWindow < nLastIndex) // There's new element from right?
        {
            const auto newRightValue = cont[nLastInWindow + 1];
            if (nFirstInWindow < lowerWindowItem) // Popping one from left?
            {
                const auto popItem = originalVals[iCont % originalVals.size()];
                if (!isNan(newRightValue))
                {
                    if (!isNan(popItem))
                        window.replace(originalVals[iCont % originalVals.size()], newRightValue);
                    else // case: old leftmost item was NaN, there nothing to pop from window so just adding new.
                        window.insert(newRightValue);
                }
                else if (!isNan(popItem))
                    window.erase(originalVals[iCont % originalVals.size()]);
                ++nFirstInWindow;
            }
            else // case: adding one from right, but no need to pop from left
            {
                if (!isNan(newRightValue))
                    window.insert(newRightValue);
            }
            ++nLastInWindow;
        }
        else // case: right window edge has reached end, checking if need to pop from left.
        {
            if (nFirstInWindow < lowerWindowItem)
            {
                const auto popItem = originalVals[iCont % originalVals.size()];
                if (!isNan(popItem))
                    window.erase(originalVals[iCont % originalVals.size()]);
                nFirstInWindow++;
            }
        }
        originalVals[iCont % originalVals.size()] = cont[iCont];
        if (!isNan(cont[iCont]))
            cont[iCont] = DFG_MODULE_NS(numeric)::medianInSorted(window);
    }
}

}} // module namespace
