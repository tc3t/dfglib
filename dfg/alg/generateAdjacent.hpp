#pragma once

#include "../dfgDefs.hpp"
#include <algorithm>


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(alg) {

    // Generates values {startVal, startVal + diff, startVal + 2*diff, ...}
    // For similar functionality, see std::iota (C++11), counting_iterator (boost).
    template <class Iteratable, class T, class Diff_T>
    void generateAdjacent(Iteratable&& iteratable, T startVal, const Diff_T& diff)
    {
        T counter = 0;
        std::generate(std::begin(iteratable), std::end(iteratable), [&]() -> T {return startVal + (counter++) * diff; });
    }

} } // module namespace
