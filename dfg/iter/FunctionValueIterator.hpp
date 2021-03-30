#pragma once

#include "../dfgDefs.hpp"
#include <iterator>
#include <type_traits>

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
        , m_func(func)
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
auto makeFunctionValueIterator(Index_T i, Func_T func) -> FunctionValueIterator<Func_T, decltype(func(Index_T())), Index_T>
{
    return FunctionValueIterator<Func_T, decltype(func(Index_T())), Index_T>(i, func);
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
