#pragma once

#include "../dfgDefs.hpp"
#include "../dfgBase.hpp"
#include "../cont/elementType.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(numeric) {

// Returns index i such that [i / count(Cont_T), (i+1) / count(Cont_T)] contains 'percentile'.
// If 'percentile' is in two such ranges, returns index of the greater element.
// TODO: revise rounding correctness.
template <class Cont_T>
auto percentileInSorted_enclosingElemIndex(const Cont_T& contSorted, double percentile) -> size_t
{
    const auto nCount = count(contSorted);

    if (nCount <= 0)
        return 0;

    limit(percentile, 0, 100);

    auto nIndex = Min(size_t(nCount - 1), floorToInteger<size_t>(nCount * percentile / 100.0));

    return nIndex;
}

// Returns element corresponding to index returned by percentileInSorted_enclosingElemIndex().
// Remarks: Returned value is guaranteed to be an element of 'cont' (given it is non-empty).
//          For empty range, with floating point type return value is quiet_NaN, non-floats return value is unspecified.
template <class Cont_T>
auto percentileInSorted_enclosingElem(const Cont_T& contSorted, const double percentile) -> typename DFG_MODULE_NS(cont)::ElementType<Cont_T>::type
{
    typedef typename DFG_MODULE_NS(cont)::ElementType<Cont_T>::type ElemT;
    if (isEmpty(contSorted))
        return (std::numeric_limits<ElemT>::has_quiet_NaN) ? std::numeric_limits<ElemT>::quiet_NaN() : ElemT();
    return contSorted[percentileInSorted_enclosingElemIndex(contSorted, percentile)];
}

} } // module namespace
