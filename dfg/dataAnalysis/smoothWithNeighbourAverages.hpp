#pragma once

#include "../dfgDefs.hpp"
#include "../cont/arrayWrapper.hpp"
#include "../func/memFunc.hpp"
#include "../dfgBase.hpp"
#include "../cont/elementType.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(dataAnalysis) {

// Convert values in an container into average values of it's neigbours.
// For example with value in the middle of container [k] -> average([k-nWindowSize], ..., [k], ..., [k+nWindowSize])
// On boundaries the average consists of as many elements as there are in the window. For example on first element
// window consists only of right side neighbours, on second there's only one left neighbour.
// TODO: Write using iterators so that the algorithm works also for non-indexed containers.
template <class Cont_T>
void smoothWithNeighbourAverages(Cont_T&& cont, const size_t nWindowRadiusRequest = 1)
{
    //typedef typename Cont_T::value_type ValueT;
    typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Cont_T>::type ValueT;
    if (nWindowRadiusRequest < 1 || cont.size() <= 1)
    {
        return;
    }

    const auto nWindowRadius = Min(nWindowRadiusRequest, cont.size() - 1);

    // If nWindowRadius is small, using this non-heap allocated array. TODO: use soo-vector
    ValueT previousValuesStatic[3] = { 0, 0, 0 };
    std::vector<ValueT> vecPreviousValues;
    if (nWindowRadius >= DFG_COUNTOF(previousValuesStatic))
        vecPreviousValues.resize(nWindowRadius + 1, 0);
    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ArrayWrapperT)<ValueT> previousVals((vecPreviousValues.empty()) ? previousValuesStatic : &vecPreviousValues[0], nWindowRadius + 1);

    DFG_SUB_NS_NAME(func)::DFG_CLASS_NAME(MemFuncAvg)<ValueT> mfAvg;
    for (size_t i = 0; i <= nWindowRadius; ++i)
        mfAvg.feedDataPoint(cont[i]);

    previousVals[nWindowRadius] = cont[0];
    size_t nPrevMemoryIndex = nWindowRadius;
    cont[0] = mfAvg.average();
    size_t nPrevLeftRadius = 0;
    size_t nPrevElemsInWnd = 1 + nWindowRadius;
    
    ValueT prevAvg = cont[0];

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
            newAvg = (ValueT(nPrevElemsInWnd) / nElemsInWnd) * (prevAvg + newWindowItemVal / nPrevElemsInWnd);
        }
        else if (nPrevElemsInWnd == nElemsInWnd)
        {
            if (nPrevLeftRadius == nLeftRadius)
            {
                const auto newWindowItemVal = cont[i + nRightRadius];
                const auto dropoutItem = previousVals[nNewMemoryIndex];
                newAvg = prevAvg + (newWindowItemVal - dropoutItem) / nElemsInWnd;
            }
            else // case: left radius grew while total size remained -> right radius got smaller. All items are in window
                newAvg = prevAvg;

        }
        else // nPrevElemsInWnd > nElemsInWnd
        {
            const auto dropoutItem = previousVals[nNewMemoryIndex];
            newAvg = (prevAvg - dropoutItem / nPrevElemsInWnd) * ValueT(nPrevElemsInWnd) / nElemsInWnd;
        }

        nPrevLeftRadius = nLeftRadius;
        nPrevElemsInWnd = nElemsInWnd;
        
        previousVals[nNewMemoryIndex] = cont[i];
        nPrevMemoryIndex = nNewMemoryIndex;
        cont[i] = newAvg;
        prevAvg = newAvg;
    }
}

}}
