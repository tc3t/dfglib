#pragma once

#include "dfgDefs.hpp"

DFG_ROOT_NS_BEGIN
{
    // For index-based container, tests whether 'index' can be used to access the container.
    // Example
    //	std::vector<char> v{'a','b','c'}
    //	isValidIndex(v, 0) == true
    //	isValidIndex(v, 1) == true
    //	isValidIndex(v, 2) == true
    //	isValidIndex(v, 3) == false
    template <class ContT, class IndexT> bool isValidIndex(const ContT& cont, const IndexT index)
    {
        return (index >= 0 && static_cast<size_t>(index) < static_cast<size_t>(cont.size()));
    }

    // Overload of IsValidIndex for array.
    template <class DataT, size_t N, class IndexT> bool isValidIndex(const DataT(&)[N], const IndexT index)
    {
        return (index >= 0 && index < N);
    }
} // namespace
