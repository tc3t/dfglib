#pragma once

#include "../dfgDefs.hpp"
#include "../build/languageFeatureInfo.hpp"
#include "../preprocessor/compilerInfoMsvc.hpp"
#include <type_traits>
#include <tuple>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    // A pair with the following properties
    //  -is_trivially_copyable == true if both template types have the same property.
    //  -Default constructor does default initialization for members (e.g. TrivialPair<int, int> a; Both a.first and a.second have indeterminate value)
    //      -NOTE: if compiler does not support '= default' or value initialization is otherwise broken (possibly VC2013), default constructor does value initialization (notably MSVC before VC2015)
    template <class T0, class T1>
    class DFG_CLASS_NAME(TrivialPair)
    {
    public:
        typedef T0 first_type;
        typedef T1 second_type;

#if DFG_LANGFEAT_HAS_DEFAULTED_AND_DELETED_FUNCTIONS && (!defined(_MSC_VER) || _MSC_VER >= DFG_MSVC_VER_2015)
        DFG_CLASS_NAME(TrivialPair)() = default;
#else // Case: compiler has no defaulting, just always value initialize.
        DFG_CLASS_NAME(TrivialPair)() DFG_NOEXCEPT(std::is_nothrow_default_constructible<first_type>::value && std::is_nothrow_default_constructible<second_type>::value) :
            first(),
            second()
        {}
#endif
        DFG_CLASS_NAME(TrivialPair)(const T0& t0, const T1& t1) DFG_NOEXCEPT(std::is_nothrow_copy_constructible<first_type>::value && std::is_nothrow_copy_constructible<second_type>::value) :
            first(t0),
            second(t1)
        {}

        DFG_CLASS_NAME(TrivialPair)(T0&& t0, T1&& t1) DFG_NOEXCEPT(std::is_nothrow_move_constructible<first_type>::value && std::is_nothrow_move_constructible<second_type>::value) :
            first(std::move(t0)),
            second(std::move(t1))
        {}

        bool operator==(const DFG_CLASS_NAME(TrivialPair)&) const;

#if !DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT
        DFG_CLASS_NAME(TrivialPair)& operator=(const DFG_CLASS_NAME(TrivialPair)& other);
        DFG_CLASS_NAME(TrivialPair)& operator=(DFG_CLASS_NAME(TrivialPair)&& other);
#endif // DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT

        T0 first;
        T1 second;
    }; // Class TrivialPair

    template <class T0, class T1>
    bool DFG_CLASS_NAME(TrivialPair)<T0, T1>::operator==(const DFG_CLASS_NAME(TrivialPair)& other) const
    {
        return first == other.first && second == other.second;
    }

#if !DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT
    template <class T0, class T1>
    auto DFG_CLASS_NAME(TrivialPair)<T0, T1>::operator=(const DFG_CLASS_NAME(TrivialPair)& other) -> DFG_CLASS_NAME(TrivialPair)&
    {
        first = other.first;
        second = other.second;
        return *this;
    }

    template <class T0, class T1>
    auto DFG_CLASS_NAME(TrivialPair)<T0, T1>::operator=(DFG_CLASS_NAME(TrivialPair)&& other) -> DFG_CLASS_NAME(TrivialPair)&
    {
        first = std::move(other.first);
        second = std::move(other.second);
        return *this;
    }
#endif // DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT

namespace DFG_DETAIL_NS
{
    template <size_t Index_T> struct TrivialPairAccessHelper;
    template <> struct TrivialPairAccessHelper<0> { template <class Return_T, class TrivialPair_T> static auto get(TrivialPair_T&& tp) -> Return_T { return tp.first; } };
    template <> struct TrivialPairAccessHelper<1> { template <class Return_T, class TrivialPair_T> static auto get(TrivialPair_T&& tp) -> Return_T { return tp.second; } };
};

template <size_t Index_T, class T0, class T1>
constexpr auto get(TrivialPair<T0, T1>& tp) noexcept -> typename std::conditional<Index_T == 0, T0&, T1&>::type
{
    DFG_STATIC_ASSERT(Index_T < 2, "Bad index for get() with TrivialPair, must be either 0 (=first) or 1 (=second)");
    using ReturnT = typename std::conditional<Index_T == 0, T0&, T1&>::type;
    return DFG_DETAIL_NS::TrivialPairAccessHelper<Index_T>::template get<ReturnT>(tp);
}

// const-ref overload for get()
template <size_t Index_T, class T0, class T1>
constexpr auto get(const TrivialPair<T0, T1>& tp) noexcept -> typename std::conditional<Index_T == 0, const T0&, const T1&>::type
{
    DFG_STATIC_ASSERT(Index_T < 2, "Bad index for get() with TrivialPair, must be either 0 (=first) or 1 (=second)");
    using ReturnT = typename std::conditional<Index_T == 0, const T0&, const T1&>::type;
    return DFG_DETAIL_NS::TrivialPairAccessHelper<Index_T>::template get<ReturnT>(tp);
}

// rvalue overload for get()
template <size_t Index_T, class T0, class T1>
constexpr auto get(TrivialPair<T0, T1>&& tp) noexcept -> typename std::conditional<Index_T == 0, T0&&, T1&&>::type
{
    return std::move(get<Index_T>(tp));
}

} } // module namespace


// Adding specialization of std::tuple_element for TrivialPair. https://stackoverflow.com/questions/40584368/c-can-stdtuple-size-tuple-element-be-specialized
// Note that at least Clang 6.0 triggers "warning: 'tuple_element' defined as a struct template here but previously declared as a class template [-Wmismatched-tags]"
// Not workarounded as this is expected to disappear on newer versions:
//      -"Bug 41331 - std::tuple_element should be a struct ": https://bugs.llvm.org/show_bug.cgi?id=41331
//      -ISO C++ Standard - std discussion: "struct vs. class when specialising tuple_element / tuple_size" https://groups.google.com/a/isocpp.org/forum/#!topic/std-discussion/QC-AMb5oO1w
template<class T0, class T1>
struct std::tuple_element<0, ::DFG_MODULE_NS(cont)::TrivialPair<T0, T1>>
{
    using type = T0;
};

template<class T0, class T1>
struct std::tuple_element<1, ::DFG_MODULE_NS(cont)::TrivialPair<T0, T1>>
{
    using type = T1;
};
