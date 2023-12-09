#pragma once

#include "../dfgDefs.hpp"
#include "../dfgAssert.hpp"
#include "../numericTypeTools.hpp"
#include <iterator>
#include <type_traits>
#include <functional>
#include <cmath>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(iter) {

namespace DFG_DETAIL_NS
{
    template <class Func_T>
    constexpr bool isFuncStorageNeeded()
    {
        // Note: captureless lambdas are default constructible only since C++20 (https://en.cppreference.com/w/cpp/language/lambda)
        return !std::is_class_v<Func_T> || !std::is_empty_v<Func_T> || !std::is_default_constructible_v<Func_T>;
    }

    // Helper class for implementing empty base optimization (https://en.cppreference.com/w/cpp/language/ebo)
    template <class Func_T, class Result_T, class Index_T>
    class FunctionValueIteratorEmptyBase
    {
    protected:
        FunctionValueIteratorEmptyBase(Func_T)
        {
        }

        Result_T deRef(const Index_T nIndex) const
        {
            return Func_T()(nIndex);
        }
    }; // class FunctionValueIteratorEmptyBase

    template <class Func_T, class Result_T, class Index_T>
    class FunctionValueIteratorFuncStorageBase
    {
    protected:
        FunctionValueIteratorFuncStorageBase(Func_T func)
            : m_func(std::move(func))
        {}

        Result_T deRef(const Index_T nIndex) const
        {
            return this->m_func(nIndex);
        }

        Func_T m_func;
    }; // class FunctionValueIteratorFuncStorageBase
} // namespace DFG_DETAIL_NS

/*
Iterator for iterating values of a function that takes Index_T as argument.
Index_T can also be floating point type, but values must be integers in suitable value range, otherwise behaviour is undefined.
*/
template <class Func_T, class Result_T, class Index_T = size_t>
class FunctionValueIterator : std::conditional_t<DFG_DETAIL_NS::isFuncStorageNeeded<Func_T>(),
                                                 DFG_DETAIL_NS::FunctionValueIteratorFuncStorageBase<Func_T, Result_T, Index_T>,
                                                 DFG_DETAIL_NS::FunctionValueIteratorEmptyBase<Func_T, Result_T, Index_T>>
{
public:
    using BaseClass = std::conditional_t<DFG_DETAIL_NS::isFuncStorageNeeded<Func_T>(),
                                         DFG_DETAIL_NS::FunctionValueIteratorFuncStorageBase<Func_T, Result_T, Index_T>,
                                         DFG_DETAIL_NS::FunctionValueIteratorEmptyBase<Func_T, Result_T, Index_T>>;
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = Result_T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = typename std::add_pointer<value_type>::type;
    using reference         = typename std::add_lvalue_reference<value_type>::type;

    FunctionValueIterator(const Index_T nPos, Func_T func)
        : BaseClass(std::move(func))
        , m_nPos(nPos)
    {
        if constexpr (!std::is_integral_v<Index_T>)
        {
            DFG_ASSERT(::DFG_MODULE_NS(math)::template isFloatConvertibleTo<difference_type>(m_nPos));
        }
    }

    auto operator*() const -> value_type
    {
        return this->deRef(m_nPos);
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
        auto iterNew = *this;
        iterNew -= nDiff;
        DFG_ASSERT(*this - iterNew == nDiff);
        return iterNew;
    }

    difference_type operator-(const FunctionValueIterator& other) const
    {
        if constexpr (std::is_integral_v<Index_T>)
        {
            const auto nDistance = ::DFG_MODULE_NS(math)::numericDistance(this->m_nPos, other.m_nPos);
            if (this->m_nPos >= other.m_nPos)
            {
                DFG_ASSERT(isValWithinLimitsOfType<difference_type>(nDistance));
                return static_cast<difference_type>(nDistance);
            }
            else // Return value is negative
            {
                constexpr auto nDiffTypeMin = (std::numeric_limits<difference_type>::min)();
                const auto nDiffTypeMinAbs = absAsUnsigned(nDiffTypeMin);
                DFG_ASSERT(nDistance <= nDiffTypeMinAbs);
                DFG_ASSERT(isValWithinLimitsOfType<difference_type>(nDiffTypeMinAbs - nDistance));
                return nDiffTypeMin + static_cast<difference_type>(nDiffTypeMinAbs - nDistance);
            }
        }
        else // Index_T is not integer
        {
            const auto diff = this->m_nPos - other.m_nPos;
            DFG_ASSERT(isValWithinLimitsOfType<difference_type>(diff));
            return static_cast<difference_type>(diff);
        }
    }

    FunctionValueIterator operator+(const difference_type nDiff) const
    {
        auto iterNew = *this;
        iterNew += nDiff;
        DFG_ASSERT(iterNew - *this == nDiff);
        return iterNew;
    }

    FunctionValueIterator& operator++()
    {
        if constexpr (std::is_floating_point_v<Index_T>)
        {
            DFG_ASSERT(std::nextafter(this->m_nPos, std::numeric_limits<Index_T>::infinity()) - this->m_nPos <= 1);
        }
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
        if constexpr (std::is_floating_point_v<Index_T>)
        {
            DFG_ASSERT(this->m_nPos - std::nextafter(this->m_nPos, -std::numeric_limits<Index_T>::infinity()) <= 1);
        }
        --this->m_nPos;
        return *this;
    }

    FunctionValueIterator operator--(int)
    {
        auto iter = *this;
        --*this;
        return iter;
    }

    FunctionValueIterator& operator+=(const difference_type nDiff)
    {
        const auto nResult = this->m_nPos + nDiff;
        DFG_ASSERT(isValWithinLimitsOfType<Index_T>(nResult));
        this->m_nPos = static_cast<Index_T>(nResult);
        return *this;
    }

    FunctionValueIterator& operator-=(const difference_type nDiff)
    {
        const auto nResult = this->m_nPos - nDiff;
        DFG_ASSERT(isValWithinLimitsOfType<Index_T>(nResult));
        this->m_nPos = static_cast<Index_T>(nResult);
        return *this;
    }

    Index_T index() const
    {
        return m_nPos;
    }

private:

    Index_T m_nPos;
}; // class FunctionValueIterator

namespace DFG_DETAIL_NS
{
    using FuncCatStateless   = std::integral_constant<int, 0>;
    using FuncCatFuncPtr     = std::integral_constant<int, 1>;
    using FuncCatStdFunction = std::integral_constant<int, 2>;

    template <class FuncRaw_T>
    struct FuncCatImpl
    {
        using Func_T = std::decay_t<FuncRaw_T>;
        using type = std::conditional_t<
                std::is_pointer_v<Func_T>,
                FuncCatFuncPtr,
                std::conditional_t<std::is_empty_v<Func_T> && std::is_copy_assignable_v<Func_T>, FuncCatStateless, FuncCatStdFunction> // Stateless lambdas are copy assignable since C++20
                >;
    };

    template <class Func_T> using FuncCat = typename FuncCatImpl<Func_T>::type;

    template <class Func_T, class Index_T>
    auto makeFunctionValueIteratorImpl(Index_T i, Func_T&& func, FuncCatStateless) -> FunctionValueIterator<std::remove_reference_t<Func_T>, decltype(func(Index_T())), Index_T>
    {
        using ReturnT = decltype(func(Index_T()));
        return FunctionValueIterator<std::remove_reference_t<Func_T>, ReturnT, Index_T>(i, func);
    }

    template <class Func_T, class Index_T>
    auto makeFunctionValueIteratorImpl(Index_T i, Func_T* pFunc, FuncCatFuncPtr) -> FunctionValueIterator<Func_T*, decltype(pFunc(Index_T())), Index_T>
    {
        using ReturnT = decltype(pFunc(Index_T()));
        using FuncPtrT = ReturnT(*) (Index_T);
        DFG_ASSERT_UB(pFunc != nullptr);
        return FunctionValueIterator<FuncPtrT, ReturnT, Index_T>(i, pFunc);
    }

    template <class Func_T, class Index_T>
    auto makeFunctionValueIteratorImpl(Index_T i, Func_T&& func, FuncCatStdFunction) -> FunctionValueIterator<std::function<decltype(func(Index_T())) (Index_T)>, decltype(func(Index_T())), Index_T>
    {
        using ReturnT = decltype(func(Index_T()));
        using Wrapper = std::function<ReturnT (Index_T)>;
        return FunctionValueIterator<Wrapper, ReturnT, Index_T>(i, Wrapper(func));
    }
} // namespace DFG_DETAIL_NS

template <class Func_T, class Index_T>
auto makeFunctionValueIterator(Index_T i, Func_T&& func) -> 
    decltype(::DFG_MODULE_NS(iter)::DFG_DETAIL_NS::makeFunctionValueIteratorImpl(i, std::forward<Func_T>(func), ::DFG_MODULE_NS(iter)::DFG_DETAIL_NS::FuncCat<Func_T>()))
{
    const auto funcCat = ::DFG_MODULE_NS(iter)::DFG_DETAIL_NS::FuncCat<Func_T>();
    return ::DFG_MODULE_NS(iter)::DFG_DETAIL_NS::makeFunctionValueIteratorImpl(i, std::forward<Func_T>(func), funcCat);
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
