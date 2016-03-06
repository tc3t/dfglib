#pragma once

#include "../dfgDefs.hpp"
#include "percentileRange.hpp"
#include <algorithm>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(numeric) {

template <class Cont_T>
void trimToPercentileRangeIISorted(Cont_T& vals, const double lower, const double upper)
{
    auto rv = percentileRangeInSortedII(vals, lower, upper);
    const auto nCount = std::distance(rv.first, rv.second);
    std::rotate(std::begin(vals), rv.first, rv.second); // Move range [rv.first, rv.second) to the beginning of vals.
    vals.erase(std::begin(vals) + nCount, std::end(vals)); // Erase the values [rv.second, vals.end)
}

template <class Cont_T>
void trimToPercentileRangeII(Cont_T& vals, const double lower, const double upper)
{
    std::sort(std::begin(vals), std::end(vals));
    trimToPercentileRangeIISorted(vals, lower, upper);
}

} } // module namespace
