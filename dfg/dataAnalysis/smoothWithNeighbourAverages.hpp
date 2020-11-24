#pragma once

#include "../dfgDefs.hpp"
#include "../cont/arrayWrapper.hpp"
#include "../func/memFunc.hpp"
#include "../dfgBase.hpp"
#include "../cont/elementType.hpp"
#include "../math.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(dataAnalysis) {

// Convert values in an container into average values of it's neigbours.
// For example with value in the middle of container [k] -> average([k-nWindowSize], ..., [k], ..., [k+nWindowSize])
// On boundaries the average consists of as many elements as there are in the window. For example on first element
// window consists only of right side neighbours, on second there's only one left neighbour.
// 
//  -NaN handling: NaN's occupy space from smooth window, but value is ignored effectively making smooth window element count smaller.
//                 NaN are not replaced by smoothed values.
// TODO: Write using iterators so that the algorithm works also for non-indexed containers.
template <class Cont_T>
void smoothWithNeighbourAverages(Cont_T&& cont, const size_t nWindowRadiusRequest = 1)
{
    //typedef typename Cont_T::value_type ValueT;
    typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Cont_T>::type ValueT;
    if (nWindowRadiusRequest < 1 || cont.size() <= 1)
        return;

    using namespace ::DFG_MODULE_NS(math);

    const auto nWindowRadius = Min(nWindowRadiusRequest, cont.size() - 1);

    // If nWindowRadius is small, using this non-heap allocated array.
    ValueT previousValuesStatic[3] = { 0, 0, 0 };
    std::vector<ValueT> vecPreviousValues;
    if (nWindowRadius >= DFG_COUNTOF(previousValuesStatic))
        vecPreviousValues.resize(nWindowRadius + 1, 0);
    DFG_MODULE_NS(cont)::ArrayWrapperT<ValueT> previousVals((vecPreviousValues.empty()) ? previousValuesStatic : &vecPreviousValues[0], nWindowRadius + 1);

    DFG_MODULE_NS(func)::MemFuncAvg<ValueT> mfAvg;
    size_t nValueCountInWindow = 0; // Number of non-Nan's in window
    for (size_t i = 0; i <= nWindowRadius; ++i)
    {
        if (!isNan(cont[i]))
        {
            ++nValueCountInWindow;
            mfAvg.feedDataPoint(cont[i]);
        }
    }

    previousVals[nWindowRadius] = cont[0];
    size_t nPrevMemoryIndex = nWindowRadius;
    if (!isNan(cont[0]))
        cont[0] = mfAvg.average();
    size_t nPrevLeftRadius = 0;
    size_t nPrevElemsInWnd = 1 + nWindowRadius;
    size_t nPrevValueCountInWnd = nValueCountInWindow;
    
    ValueT prevAvg = mfAvg.average();

    for(size_t i = 1; i<cont.size(); ++i)
    {
        size_t nLeftRadius = Min(i, nWindowRadius);
        size_t nRightRadius = Min(nWindowRadius, cont.size() - i - 1);
        const auto nElemsInWnd = 1 + nLeftRadius + nRightRadius;

        const auto nNewMemoryIndex = (nPrevMemoryIndex + 1) % previousVals.size();

        ValueT newAvg;

        if (nPrevElemsInWnd < nElemsInWnd) // In start, window is growing.
        {
            const auto newWindowItemVal = cont[i + nRightRadius];
            if (!isNan(newWindowItemVal))
            {
                ++nValueCountInWindow;
                newAvg = (!isNan(prevAvg)) ? (ValueT(nPrevValueCountInWnd) / nValueCountInWindow) * (prevAvg + newWindowItemVal / nPrevValueCountInWnd) : newWindowItemVal;
                
            }
            else // If new value is NaN, no effect on average in the growing case.
                newAvg = prevAvg;
        }
        else if (nPrevElemsInWnd == nElemsInWnd)
        {
            if (nPrevLeftRadius == nLeftRadius) // Typical update: new item from right, dropping one from left.
            {
                const auto newWindowItemVal = cont[i + nRightRadius];
                const auto dropoutItem = previousVals[nNewMemoryIndex];
                const bool bPoppedValue = !isNan(dropoutItem);
                const bool bAddedValue = !isNan(newWindowItemVal);
                nValueCountInWindow += int(bAddedValue) - int(bPoppedValue);
                if (bAddedValue && bPoppedValue) // case: added new, popped old.
                    // avg_new = (sum_prev + new_item - popped_item) / nValueCountInWindow
                    //         = avg_prev + (new_item - popped_item) / nValueCountInWindow
                    newAvg = prevAvg + (newWindowItemVal - dropoutItem) / nValueCountInWindow;
                else if (bAddedValue) // case: added value, popped nan
                {
                    // avg_new = (sum_prev + new_item) / nValueCountInWindow
                    //         = (nPrevValueCountInWnd * sum_prev / nPrevValueCountInWnd + new_item) / nValueCountInWindow
                    //         = (nPrevValueCountInWnd * avg_prev + new_item) / nValueCountInWindow
                    newAvg = (nPrevValueCountInWnd * prevAvg + newWindowItemVal) / (static_cast<ValueT>(nValueCountInWindow));
                }
                else if (bPoppedValue) // case: added NaN, popped value
                {
                    // avg_new = (sum_prev - popped_item) / nValueCountInWindow
                    //         = (nPrevValueCountInWnd * sum_prev / nPrevValueCountInWnd - popped_item) / nValueCountInWindow
                    //         = (nPrevValueCountInWnd * avg_prev - popped_item) / nValueCountInWindow
                    newAvg = (nPrevValueCountInWnd * prevAvg - dropoutItem) / (static_cast<ValueT>(nValueCountInWindow));
                }
                else // case: added NaN, popped NaN
                {
                    newAvg = prevAvg;
                }
            }
            else // case: left radius grew while total size remained -> right radius got smaller. All items were and are in window
                newAvg = prevAvg;
        }
        else // nPrevElemsInWnd > nElemsInWnd. Right boundary, window getting smaller by dropping out left items.
        {
            const auto dropoutItem = previousVals[nNewMemoryIndex];
            if (!isNan(dropoutItem))
            {
                newAvg = (prevAvg - dropoutItem / nValueCountInWindow) * ValueT(nPrevValueCountInWnd) / (nValueCountInWindow - ValueT(1));
                --nValueCountInWindow;
            }
            else
                newAvg = prevAvg;
        }

        nPrevLeftRadius = nLeftRadius;
        nPrevElemsInWnd = nElemsInWnd;
        nPrevValueCountInWnd = nValueCountInWindow;
        
        previousVals[nNewMemoryIndex] = cont[i];
        nPrevMemoryIndex = nNewMemoryIndex;
        if (!isNan(cont[i]))
            cont[i] = newAvg;
        prevAvg = newAvg;
    }
}

}}
