#pragma once

#include "../dfgDefs.hpp"
#include <iterator>
#include <type_traits>
#include <functional>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(iter) {

/*
Iterator for iterating values of a function that takes Index_T as argument.
*/
template <class Func_T, class Result_T, class Index_T = size_t>
class FunctionValueIterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = Result_T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = typename std::add_pointer<value_type>::type;
    using reference         = typename std::add_lvalue_reference<value_type>::type;

    FunctionValueIterator(const Index_T nPos, Func_T func)
        : m_nPos(nPos)
        , m_func(std::move(func))
    {}

    auto operator*() const -> value_type
    {
        return this->m_func(m_nPos);
    }

    bool operator<(const FunctionValueIterator& other) const
    {
        return this->m_nPos < other.m_nPos;
    }

    bool operator==(const FunctionValueIterator& other) const
    {
        return this->m_nPos == other.m_nPos;
    }

    bool operator!=(const FunctionValueIterator& other) const
    {
        return !(*this == other);
    }

    FunctionValueIterator operator-(const difference_type nDiff) const
    {
        return FunctionValueIterator(this->m_nPos - nDiff, this->m_func);
    }

    difference_type operator-(const FunctionValueIterator& other) const
    {
        return this->m_nPos - other.m_nPos;
    }

    FunctionValueIterator operator+(const difference_type nDiff) const
    {
        return FunctionValueIterator(this->m_nPos + nDiff, this->m_func);
    }

    FunctionValueIterator& operator++()
    {
        ++this->m_nPos;
        return *this;
    }

    FunctionValueIterator operator++(int)
    {
        auto iter = *this;
        ++*this;
        return iter;
    }

    FunctionValueIterator& operator--()
    {
        --this->m_nPos;
        return *this;
    }

    FunctionValueIterator operator--(int)
    {
        auto iter = *this;
        --*this;
        return iter;
    }

    FunctionValueIterator& operator+=(difference_type nDiff)
    {
        this->m_nPos += nDiff;
        return *this;
    }

    FunctionValueIterator& operator-=(difference_type nDiff)
    {
        this->m_nPos -= nDiff;
        return *this;
    }

    Index_T m_nPos;
    Func_T m_func;
}; // class FunctionValueIterator

template <class Func_T, class Index_T>
auto makeFunctionValueIterator(Index_T i, Func_T func) -> FunctionValueIterator<decltype(func(Index_T())) (*) (Index_T), decltype(func(Index_T())), Index_T>
{
    using ReturnT = decltype(func(Index_T()));
    using FuncPtrT = ReturnT (*) (Index_T);
    DFG_STATIC_ASSERT((std::is_convertible<Func_T, FuncPtrT>::value), "Only stateless lambdas are accepted by this overload. See std::function overload of this function as alternative.");
    FuncPtrT pFunc = func;
    return FunctionValueIterator<FuncPtrT, decltype(func(Index_T())), Index_T>(i, pFunc);
}

// Note: when returned iterator is used in algorithms, it may get copied in arbitrary ways so given function should be compatible with such use.
template <class Func_T, class Index_T>
auto makeFunctionValueIterator(Index_T i, std::function<Func_T> func) -> FunctionValueIterator<std::function<Func_T>, decltype(func(Index_T())), Index_T>
{
    using ReturnT = decltype(func(Index_T()));
    using Wrapper = std::function<Func_T>;
    return FunctionValueIterator<Wrapper, ReturnT, Index_T>(i, std::move(func));
}

namespace DFG_DETAIL_NS
{
    template <class T> struct IdentityHelper { T operator()(T t) const { return t; } };
}

template <class Index_T>
auto makeIndexIterator(Index_T i) -> FunctionValueIterator<DFG_DETAIL_NS::IdentityHelper<Index_T>, Index_T, Index_T>
{
    return FunctionValueIterator<DFG_DETAIL_NS::IdentityHelper<Index_T>, Index_T, Index_T>(i, DFG_DETAIL_NS::IdentityHelper<Index_T>());
}


}} // module namespace
