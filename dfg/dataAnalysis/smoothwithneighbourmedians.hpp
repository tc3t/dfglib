#pragma once

#include "../dfgDefs.hpp"
#include "../cont/arrayWrapper.hpp"
#include "../func/memFunc.hpp"
#include "../dfgBase.hpp"
#include "../cont/elementType.hpp"
#include "../cont/SortedSequence.hpp"
#include "../numeric/median.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(dataAnalysis) {

// Convert values in an container into a median values of local neighbour window.
// For example with value in the middle of container [k] -> median([k-nWindowSize], ..., [k], ..., [k+nWindowSize])
// On boundaries the median is calculated from as many elements as there are in the window. For example on first element
// window consists only of right side neighbours, on second there's only one left neighbour.
// TODO: Write using iterators so that the algorithm works also for non-indexed containers.
template <class Cont_T>
void smoothWithNeighbourMedians(Cont_T&& cont, const size_t nWindowRadiusRequest = 1)
{
    typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Cont_T>::type ValueT;

    const auto nSize = ::DFG_ROOT_NS::count(cont);

    if (nWindowRadiusRequest < 1 || nSize <= 1)
    {
        return;
    }

    const auto nWindowRadius = Min(nWindowRadiusRequest, nSize - 1);

    // If nWindowRadius is small, using this non-heap allocated array. TODO: use soo-vector
    ValueT originalValuesStatic[3] = { 0, 0, 0 };
    std::vector<ValueT> vecOriginalValues;
    if (nWindowRadius >= DFG_COUNTOF(originalValuesStatic))
		vecOriginalValues.resize(nWindowRadius + 1, 0);
    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ArrayWrapperT)<ValueT> originalVals((vecOriginalValues.empty()) ? originalValuesStatic : &vecOriginalValues[0], nWindowRadius + 1);

	DFG_MODULE_NS(cont)::DFG_CLASS_NAME(SortedSequence)<std::vector<ValueT>> window;

	const auto nMaxWindowSize = Min(nSize, 1 + 2 * nWindowRadius);
	window.reserve(nMaxWindowSize);

	size_t iWnd = 0;
	ValueT latestReplaced;
	for (size_t iCont = 0; iCont < nSize; ++iCont)
	{
		const auto lowerWindowItem = (iCont >= nWindowRadius) ? iCont - nWindowRadius : 0;
		const auto endWndItem = Min(iCont + nWindowRadius + 1, nSize);
		const auto nCurrentWindowSize = endWndItem - lowerWindowItem;

		for (; iWnd < endWndItem; ++iWnd)
		{
			if (window.size() < nMaxWindowSize)
				window.insert(cont[iWnd]);
			else
				window.replace(originalVals[iCont % originalVals.size()], cont[iWnd]);
		}
		if (window.size() > nCurrentWindowSize)
			window.erase(originalVals[(iCont - nWindowRadius - 1) % originalVals.size()]);
		originalVals[iCont % originalVals.size()] = cont[iCont];
		cont[iCont] = DFG_MODULE_NS(numeric)::medianInSorted(window);
	}
}

}} // module namespace
