#pragma once

#include "../dfgDefs.hpp"
#include <iterator>
#include <tuple>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(iter) {

// Iterator that provides custom access for existing iterator, for example iterator of int elements in std::vector<std::pair<int, double>>
template <class MainIterator_T, class Access_T>
class CustomAccessIterator
{
public:
    using iterator_category = typename std::iterator_traits<MainIterator_T>::iterator_category;
    using difference_type = typename std::iterator_traits<MainIterator_T>::difference_type;
    using pointer = decltype(Access_T()(MainIterator_T()));
    using value_type = typename std::remove_pointer<pointer>::type;
    using reference = typename std::add_lvalue_reference<typename std::remove_pointer<pointer>::type>::type;

    CustomAccessIterator(const MainIterator_T& iter)
        : m_iter(iter)
    {}

    // Returns underlying iterator which this iterator uses.
    MainIterator_T underlyingIterator() const
    {
        return m_iter;
    }

    auto operator->() const -> pointer
    {
        return Access_T()(m_iter);
    }

    auto operator*() const -> reference
    {
        return *operator->();
    }

    bool operator<(const CustomAccessIterator& other) const
    {
        return this->m_iter < other.m_iter;
    }

    bool operator==(const CustomAccessIterator& other) const
    {
        return this->m_iter == other.m_iter;
    }

    bool operator!=(const CustomAccessIterator& other) const
    {
        return !(*this == other);
    }

    CustomAccessIterator operator-(const difference_type nDiff) const
    {
        return CustomAccessIterator(m_iter - nDiff);
    }

    difference_type operator-(const CustomAccessIterator& other) const
    {
        return this->m_iter - other.m_iter;
    }

    CustomAccessIterator operator+(const difference_type nDiff) const
    {
        return CustomAccessIterator(m_iter + nDiff);
    }

    CustomAccessIterator& operator++()
    {
        ++this->m_iter;
        return *this;
    }

    CustomAccessIterator operator++(int)
    {
        auto iter = *this;
        ++*this;
        return iter;
    }

    CustomAccessIterator& operator--()
    {
        --this->m_iter;
        return *this;
    }

    CustomAccessIterator operator--(int)
    {
        auto iter = *this;
        --*this;
        return iter;
    }

    CustomAccessIterator& operator+=(difference_type nDiff)
    {
        this->m_iter += nDiff;
        return *this;
    }

    CustomAccessIterator& operator-=(difference_type nDiff)
    {
        this->m_iter -= nDiff;
        return *this;
    }

    MainIterator_T m_iter;
}; // class CustomAccessIterator

namespace DFG_DETAIL_NS
{
    template <size_t Index_T, class Iter_T>
    class TupleAccessFunc
    {
    public:
        using TupleType = typename std::remove_reference<decltype(*Iter_T())>::type;
        using ReturnValueType = typename std::tuple_element<Index_T, TupleType>::type;
        ReturnValueType* operator()(const Iter_T& iter) const { using namespace std; return &get<Index_T>(*iter); }
    };
}

// Returns custom access iterator that accesses tuple element in container of tuples.
// Example:
//      std::vector<std::pair<std::string, double>> cont;
//      auto stringIterator = makeTupleElementAccessIterator<0>(cont.begin());
//      auto doubleIterator = makeTupleElementAccessIterator<1>(cont.begin());
template <size_t Index_T, class Iter_T>
auto makeTupleElementAccessIterator(const Iter_T& iter) -> CustomAccessIterator<Iter_T, DFG_DETAIL_NS::TupleAccessFunc<Index_T, Iter_T>>
{
    return CustomAccessIterator<Iter_T, DFG_DETAIL_NS::TupleAccessFunc<Index_T, Iter_T>>(iter);
}

}} // module namespace
