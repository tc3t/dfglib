#pragma once

#include "../dfgDefs.hpp"
#include <type_traits>
#include <iterator>
#include <array>
#include <vector>
#include <string>
#include <iterator> // For std::contiguous_iterator wtih c++20

DFG_ROOT_NS_BEGIN
{
    template <class Iter_T> struct IsStdArrayContainer{ static const bool value = false; };

    template <class Elem_T, size_t N> struct IsStdArrayContainer<std::array<Elem_T, N>>
    {
        static const bool value = true;
    };

    // Note: before C++20, does not find all contiguous memory iterators, for example iterators of std::array<int, N> are not detected as CMI.
    // For related standard proposal, see "N4132: Contiguous Iterators" (http://isocpp.org/files/papers/n4132.html)
    template <class Iter_T>
    struct IsContiguousMemoryIterator
    {
        // There didn't seem to be feature test macro for std::contiguous_iterator, so filtering by C++ standard
#if DFG_CPLUSPLUS >= DFG_CPLUSPLUS_20
        static constexpr bool value = std::contiguous_iterator<Iter_T>;
#else // Case: before c++20
        typedef typename std::iterator_traits<Iter_T>::value_type value_type;
        static constexpr bool value = std::is_pointer<Iter_T>::value == true ||
                                    std::is_same<Iter_T, typename std::vector<value_type>::iterator>::value == true ||
                                    std::is_same<Iter_T, typename std::vector<value_type>::const_iterator>::value == true ||
                                    std::is_same<Iter_T, typename std::string::iterator>::value == true ||
                                    std::is_same<Iter_T, typename std::string::const_iterator>::value == true ||
                                    std::is_same<Iter_T, typename std::wstring::iterator>::value == true ||
                                    std::is_same<Iter_T, typename std::wstring::const_iterator>::value == true;
#endif
    };

} // namespace
