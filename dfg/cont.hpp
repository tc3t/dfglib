#ifndef DFG_CONT_WJEQXMXY
#define DFG_CONT_WJEQXMXY

#include "dfgBase.hpp"
#include "math/interpolationLinear.hpp"
#include <limits>
#include <vector>
#include <array>
#include "cont/elementType.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(cont) {

template <class ContT, class T> void pushBack(ContT& cont, T&& a0)
{
    cont.reserve(cont.size() + 1);
    cont.push_back(std::forward<T>(a0));
}

template <class ContT, class T> void pushBack(ContT& cont, T&& a0, T&& a1)
{
    cont.reserve(cont.size() + 2);
    cont.push_back(std::forward<T>(a0));
    cont.push_back(std::forward<T>(a1));
}

#if DFG_MSVC_VER == DFG_MSVC_VER_2010

    template <class T> std::vector<T> makeVector(T a0)
    {
        std::vector<T> vec;
        pushBack(vec, std::move(a0));
        return vec;
    }

    template <class T> std::vector<T> makeVector(T a0, T a1)
    {
        std::vector<T> vec;
        pushBack(vec, std::move(a0), std::move(a1));
        return vec;
    }

    template <class T> std::vector<T> makeVector(T a0, T a1, T a2)
    {
        std::vector<T> vec;
        pushBack(vec, std::move(a0), std::move(a1));
        pushBack(vec, std::move(a2));
        return vec;
    }

    template <class T> std::vector<T> makeVector(T a0, T a1, T a2, T a3)
    {
        std::vector<T> vec;
        pushBack(vec, std::move(a0), std::move(a1));
        pushBack(vec, std::move(a2), std::move(a3));
        return vec;
    }

    template <class T> std::vector<T> makeVector(T a0, T a1, T a2, T a3, T a4)
    {
        std::vector<T> vec;
        pushBack(vec, std::move(a0), std::move(a1));
        pushBack(vec, std::move(a2), std::move(a3));
        pushBack(vec, std::move(a4));
        return vec;
    }

    template <class T> std::vector<T> makeVector(T a0, T a1, T a2, T a3, T a4, T a5)
    {
        std::vector<T> vec;
        pushBack(vec, std::move(a0), std::move(a1));
        pushBack(vec, std::move(a2), std::move(a3));
        pushBack(vec, std::move(a4), std::move(a5));
        return vec;
    }

#else // Compiler other than VC2010. In VC2010 code such as makeVector<std::string>("1", "2") wouldn't compile.

    template <class T> auto makeVector(T&& a0) -> std::vector<typename std::remove_reference<T>::type>
    {
        typedef typename std::remove_reference<T>::type ElemT;
        std::vector<ElemT> vec;
        pushBack(vec, std::forward<T>(a0));
        return vec;
    }

    template <class T> auto makeVector(T&& a0, T&& a1) -> std::vector<typename std::remove_reference<T>::type>
    {
        typedef typename std::remove_reference<T>::type ElemT;
        std::vector<ElemT> vec;
        pushBack(vec, std::forward<T>(a0), std::forward<T>(a1));
        return vec;
    }

    template <class T> auto makeVector(T&& a0, T&& a1, T&& a2) -> std::vector<typename std::remove_reference<T>::type>
    {
        typedef typename std::remove_reference<T>::type ElemT;
        std::vector<ElemT> vec;
        pushBack(vec, std::forward<T>(a0), std::forward<T>(a1));
        pushBack(vec, std::forward<T>(a2));
        return vec;
    }

    template <class T> auto makeVector(T&& a0, T&& a1, T&& a2, T&& a3) -> std::vector<typename std::remove_reference<T>::type>
    {
        typedef typename std::remove_reference<T>::type ElemT;
        std::vector<ElemT> vec;
        pushBack(vec, std::forward<T>(a0), std::forward<T>(a1));
        pushBack(vec, std::forward<T>(a2), std::forward<T>(a3));
        return vec;
    }

    template <class T> auto makeVector(T&& a0, T&& a1, T&& a2, T&& a3, T&& a4) -> std::vector<typename std::remove_reference<T>::type>
    {
        typedef typename std::remove_reference<T>::type ElemT;
        std::vector<ElemT> vec;
        pushBack(vec, std::forward<T>(a0), std::forward<T>(a1));
        pushBack(vec, std::forward<T>(a2), std::forward<T>(a3));
        pushBack(vec, std::forward<T>(a4));
        return vec;
    }

    template <class T> auto makeVector(T&& a0, T&& a1, T&& a2, T&& a3, T&& a4, T&& a5) -> std::vector<typename std::remove_reference<T>::type>
    {
        typedef typename std::remove_reference<T>::type ElemT;
        std::vector<ElemT> vec;
        pushBack(vec, std::forward<T>(a0), std::forward<T>(a1));
        pushBack(vec, std::forward<T>(a2), std::forward<T>(a3));
        pushBack(vec, std::forward<T>(a4), std::forward<T>(a5));
        return vec;
    }

#endif

// TODO: test
template <class T> std::array<T, 1> makeArray(T&& a0) { std::array<T, 1> a = { std::move(a0) }; return a; }
template <class T> std::array<T, 2> makeArray(T&& a0, T&& a1) { std::array<T, 2> a = { std::move(a0), std::move(a1) }; return a; }
template <class T> std::array<T, 3> makeArray(T&& a0, T&& a1, T&& a2) { std::array<T, 3> a = { std::move(a0), std::move(a1), std::move(a2) }; return a; }
template <class T> std::array<T, 4> makeArray(T&& a0, T&& a1, T&& a2, T&& a3) { std::array<T, 4> a = { std::move(a0), std::move(a1), std::move(a2), std::move(a3) }; return a; }
template <class T> std::array<T, 5> makeArray(T&& a0, T&& a1, T&& a2, T&& a3, T&& a4) { std::array<T, 5> a = { std::move(a0), std::move(a1), std::move(a2), std::move(a3), std::move(a4) }; return a; }
template <class T> std::array<T, 6> makeArray(T&& a0, T&& a1, T&& a2, T&& a3, T&& a4, T&& a5) { std::array<T, 6> a = { std::move(a0), std::move(a1), std::move(a2), std::move(a3), std::move(a4), std::move(a5) }; return a; }

// Interpolated access to indexed value list, i.e. instead of using integer-indexes to get values from a
// container, one can use value like 1.35 to get value between [1] and [2] using linear interpolation.
// Note: No extropolation is done: if index is not with [0, lastIndex], NaN will be returned.
// TODO: test
// TODO: extend to non-floating point types (e.g. values with units).
template <class Cont_T>
auto getInterpolatedValue(const Cont_T& cont, const double& index) -> typename std::remove_reference<decltype(*cont.begin())>::type
{
    typedef typename std::remove_reference<decltype(*cont.begin())>::type ValueType;
    static_assert(std::is_floating_point<ValueType>::value == true, "This function is currently implemented only for floating points.");
    if (cont.empty() || index < 0 || index > cont.size() - 1)
        return std::numeric_limits<ValueType>::quiet_NaN();
    const auto floorBin = round<size_t>(std::floor(index));
    const auto ceilBin = Min(floorBin + 1, cont.size() - 1);
    return DFG_SUB_NS_NAME(math)::interpolationLinear_X_X0Y0_X1Y1(index, ValueType(floorBin), ValueType(cont[floorBin]), ValueType(ceilBin), ValueType(cont[ceilBin]));
}


}} // module cont



#endif // Include guard
