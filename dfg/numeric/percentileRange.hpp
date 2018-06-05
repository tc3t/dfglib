#pragma once

#include "../dfgDefs.hpp"
#include "../dfgBase.hpp"
#include <utility>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(numeric) {

    // Returns [first, end) from sorted range (based on caller promise), where all values in the range are within percentile range [lowerBound, upperBound].
    // Element i is included if percentile range is within it's exclude-include range, where
    // exclude-include range is [i / count(cont), (i+1) / count(cont)]
    template <class Cont_T>
    auto percentileRangeInSortedII(Cont_T& cont, double lowerBound, double upperBound) -> std::pair<decltype(std::begin(cont)), decltype(std::begin(cont))>
    {
        if (upperBound < lowerBound)
            return std::make_pair(std::end(cont), std::end(cont));
        limit(lowerBound, 0.0, 100.0);
        lowerBound /= 100.0;
        limit(upperBound, lowerBound, 100.0);
        upperBound /= 100.0;
        const auto nCount = count(cont);

        auto nFirstIndex = floorToInteger<size_t>(nCount * lowerBound);
        // Check if lowerBound exactly on exc-inc upper boundary of previous element? Revise: Not sure how reasonable this is given the
        // nature of floating point values.
        if (nFirstIndex > 0 && static_cast<double>(nFirstIndex) == lowerBound * static_cast<double>(nCount))
            nFirstIndex--; // In that case include the previous one.

        auto nEndIndex = ceilToInteger<size_t>(nCount * upperBound);

        if (nEndIndex < nCount && static_cast<double>(nEndIndex) == static_cast<double>(nCount) * upperBound) // Is upperBoundary exactly on exc-inc lower boundary of next element?
            ++nEndIndex; // In that case include the next one.
        auto rv = std::make_pair(std::begin(cont), std::begin(cont));
        std::advance(rv.first, nFirstIndex);
        std::advance(rv.second, nEndIndex);
        return rv;
    }

} } // module namespace
