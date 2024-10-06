#pragma once

#include "../dfgDefs.hpp"
#include "../Span.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(cont) {

template <class T> using ArrayWrapperT = Span<T>;

class ArrayWrapper
{
public:
    template <class T>
    static ArrayWrapperT<T> createArrayWrapper(T* pData, const size_t nSize)
    {
        return ArrayWrapperT<T>(pData, nSize);
    }

    template <class T, size_t N>
    static ArrayWrapperT<T> createArrayWrapper(T (&arrData)[N])
    {
        return createArrayWrapper(arrData, N);
    }

    template <class Cont_T>
    static ArrayWrapperT<typename Cont_T::value_type> createArrayWrapper(Cont_T& cont)
    {
        return ArrayWrapperT<typename Cont_T::value_type>(cont.data(), cont.size());
    }

    template <class Cont_T>
    static ArrayWrapperT<const typename Cont_T::value_type> createArrayWrapper(const Cont_T& cont)
    {
        return ArrayWrapperT<const typename Cont_T::value_type>(cont.data(), cont.size());
    }
};

}} // module cont
