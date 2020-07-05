#pragma once

#include "dfgDefs.hpp"
#include "numericTypeTools.hpp"

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
        constexpr auto nWidestSize = (sizeof(IndexT) >= sizeof(decltype(cont.size()))) ? sizeof(IndexT) : sizeof(decltype(cont.size()));
        using WidestUnsigned = typename IntegerTypeBySizeAndSign<nWidestSize, false>::type;
        return (index >= 0 && static_cast<WidestUnsigned>(index) < static_cast<WidestUnsigned>(cont.size()));
    }

    // Overload of IsValidIndex for array.
    template <class DataT, size_t N, class IndexT> bool isValidIndex(const DataT(&)[N], const IndexT index)
    {
        constexpr auto nWidestSize = (sizeof(IndexT) >= sizeof(size_t)) ? sizeof(IndexT) : sizeof(size_t);
        using WidestUnsigned = typename IntegerTypeBySizeAndSign<nWidestSize, false>::type;
        return (index >= 0 && static_cast<WidestUnsigned>(index) < static_cast<WidestUnsigned>(N));
    }
} // namespace
