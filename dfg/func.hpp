#ifndef DFG_FUNC_CTFRZQMN
#define DFG_FUNC_CTFRZQMN

#include "dfgBase.hpp"
#include <tuple>
#include <type_traits>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(func) {

template <class T> T& identity(T& t) {return t;}
template <class T> const T& identity(const T& t) {return t;}

// On first call of operator() returns start value.
// On subsequent calls returns diff + previously_returned_value.
// So effectively n'th call to operator() returns startvalue + n*diff. (starting n from zero).
template <class T> struct Adds
{
    Adds(const T& n, const T& diff) : m_n(n), m_Diff(diff) {}
    T operator()() {T temp = m_n; m_n += m_Diff; return temp;}

    T m_n;
    T m_Diff;
};

// Like Adds, but returns startvalue - n*diff.
// Note that this can't be implemented with Adds in case of unsigned types.
template <class T> struct AddsNeg
{
    AddsNeg(const T& n, const T& diff) : m_n(n), m_Diff(diff) {}
    T operator()() {T temp = m_n; m_n -= m_Diff; return temp;}

    T m_n;
    T m_Diff;
};

namespace DFG_DETAIL_NS
{

// Calls given function for each element.
// Note: Rudimentary implementation: expect all elements to be similar enough to be passed to a function of single parameter type.
template <class Tuple_T>
class TupleVisitor
{
public:
    static const size_t s_nTupleSize = std::tuple_size<Tuple_T>::value;

    template <size_t N, class Func_T>
    static void forEachInTupleImpl(Tuple_T& tuple, Func_T&& func, const std::integral_constant<size_t, N>)
    {
        func(std::get<N>(tuple));
        forEachInTupleImpl(tuple, func, std::integral_constant<size_t, N + 1>());
    }

    template <class Func_T>
    static void forEachInTupleImpl(Tuple_T&, Func_T&&, const std::integral_constant<size_t, s_nTupleSize>)
    {
    }
}; // class TupleVisitor

template <class FuncTuple_T, class T>
class FuncTupleCaller
{
public:
    static const size_t s_nLastTupleIndex = std::tuple_size<FuncTuple_T>::value - 1;

    template <size_t N>
    static void doCall(FuncTuple_T& tuple, T& data)
    {
        doCallImpl(tuple, data, std::integral_constant<size_t, N>());
    }

    template <size_t N>
    static void doCallImpl(FuncTuple_T& tuple, T& data, const std::integral_constant<size_t, N>/* Nval*/)
    {
        std::get<N>(tuple)(data);
        doCallImpl(tuple, data, std::integral_constant<size_t, N + 1>());
    }

    static void doCallImpl(FuncTuple_T& tuple, T& data, const std::integral_constant<size_t, s_nLastTupleIndex>/*Nval*/)
    {
        std::get<s_nLastTupleIndex>(tuple)(data);
    }
}; // class FuncTupleCaller

} // namespace DFG_DETAIL_NS

// MultiUnaryCaller can be used to call every unary function in a tuple with given argument.
template <class FuncTuple_T, class Arg_T>
struct DFG_CLASS_NAME(MultiUnaryCaller)
{
public:
    DFG_CLASS_NAME(MultiUnaryCaller)() {}
    void operator()(Arg_T& data)
    {
        // Recursively calls the functions in ascending order.
        DFG_DETAIL_NS::FuncTupleCaller<FuncTuple_T, Arg_T>::template doCall<0>(m_funcTuple, data);
    }

    template <size_t N>
    typename std::tuple_element<N, FuncTuple_T>::type& get() { return std::get<N>(m_funcTuple); }

    FuncTuple_T& getTuple() {return m_funcTuple;}

    FuncTuple_T m_funcTuple;
};

template <class Tuple_T, class Func_T>
void forEachInTuple(Tuple_T& tuple, Func_T&& func)
{
    DFG_DETAIL_NS::TupleVisitor<Tuple_T>::forEachInTupleImpl(tuple, func, std::integral_constant<size_t, 0>());
}

// Functor which returns given parameter of type Src_T static_casted to Dest_T.
// Rationale: For example copying wchar_t array to char array with std::copy
//			  can cause compiler warning as wchar_t is converted to char.
//			  An alternative is to use std::transform and giving it castfunction as parameter.
template <class Src_T, class Dest_T> struct CastStatic : public std::unary_function<Src_T, Dest_T>
{
    Dest_T operator()(const Src_T& src) const {return static_cast<Dest_T>(src);}
};

}} // module func

#include "func/memFunc.hpp"

#endif // include guard
