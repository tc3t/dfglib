#pragma once

#include "../dfgDefs.hpp"
#include <type_traits>
#include <iterator>
#include <array>
#include <vector>
#include <string>

DFG_ROOT_NS_BEGIN
{
    template <class Iter_T> struct IsStdArrayContainer{ static const bool value = false; };

    template <class Elem_T, size_t N> struct IsStdArrayContainer<std::array<Elem_T, N>>
    {
        static const bool value = true;
    };

    // Note: Does not find all contiguous memory iterators, for example iterators of std::array<int, N> are not detected as CMI.
    // TODO: Make check more reliable and implement at least the std::array.
    // For related standard proposal, see "N4132: Contiguous Iterators" (http://isocpp.org/files/papers/n4132.html)
    template <class Iter_T>
    struct IsContiguousMemoryIterator
    {
        typedef typename std::iterator_traits<Iter_T>::value_type value_type;
        static const bool value = std::is_pointer<Iter_T>::value == true ||
                                    std::is_same<Iter_T, typename std::vector<value_type>::iterator>::value == true ||
                                    std::is_same<Iter_T, typename std::vector<value_type>::const_iterator>::value == true ||
                                    std::is_same<Iter_T, typename std::string::iterator>::value == true ||
                                    std::is_same<Iter_T, typename std::string::const_iterator>::value == true ||
                                    std::is_same<Iter_T, typename std::wstring::iterator>::value == true ||
                                    std::is_same<Iter_T, typename std::wstring::const_iterator>::value == true;
    };

} // namespace
