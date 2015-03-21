#pragma once

#include "dfgDefs.hpp"
#include <string>

DFG_ROOT_NS_BEGIN
{
    template <class Range_T> auto ptrToContiguousMemory(Range_T& t) -> decltype(t.data())
    {
        return t.data(); 
    }

    template <class T> T* ptrToContiguousMemory(T* p)
    {
        return p;
    }

    template <class T> T* ptrToContiguousMemory(std::basic_string<T>& s)
    {
        return (!s.empty()) ? &s[0] : nullptr;
    }

    // Defines type either as pointer type to the beginning of the contiguous range or if not available, begin iterator.
    template <class T>
    struct RangeToBeginPtrOrIter
    {
        //template <class U> auto func(U* p) -> decltype(p->data());
        template <class U> static auto func(U* p) -> decltype(DFG_ROOT_NS::ptrToContiguousMemory(*p));
        static auto func(...) -> decltype(std::begin(*(T*)nullptr));

        typedef decltype(func((T*)nullptr)) type;
    };

    template <class T, size_t N>
    struct RangeToBeginPtrOrIter<T[N]>
    {
        typedef T* type;
    };

    // Defines value as true if it is known at compile time for given range that
    // &*(std::begin(range) + i) == &*std::begin(range) + i
    // for all i in [0, count(range)[
    template <class Range_T>
    struct IsContiguousRange
    {
        typedef typename std::remove_reference<Range_T>::type RefRemovedRangeT;
        static const bool value = std::is_pointer<typename RangeToBeginPtrOrIter<RefRemovedRangeT>::type>::value;
    };

} // namespace
