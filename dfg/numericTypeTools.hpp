#pragma once

#include "dfgDefs.hpp"
#include "dfgBaseTypedefs.hpp"
#include <limits>
#include <type_traits>
#include <algorithm> // for std::max
#include <cmath>     // for std::fmax/fmin
#include "dfgAssert.hpp"

DFG_ROOT_NS_BEGIN
{
    template <class T> struct NumericTraits {};

#ifdef _WIN32
    #define DFG_TEMP_NUMERIC_TRAITS_CONST_TYPE  const
#else
    #define DFG_TEMP_NUMERIC_TRAITS_CONST_TYPE  constexpr
#endif

#define DFG_TEMP_DEFINE_NUMERICTRAITS_SPECIALIZATION(TYPE) \
        template <> struct NumericTraits<TYPE> { static DFG_TEMP_NUMERIC_TRAITS_CONST_TYPE TYPE maxValue = TYPE##_max; static DFG_TEMP_NUMERIC_TRAITS_CONST_TYPE TYPE minValue = TYPE##_min; };

    DFG_TEMP_DEFINE_NUMERICTRAITS_SPECIALIZATION(int8);
    DFG_TEMP_DEFINE_NUMERICTRAITS_SPECIALIZATION(int16);
    DFG_TEMP_DEFINE_NUMERICTRAITS_SPECIALIZATION(int32);
    DFG_TEMP_DEFINE_NUMERICTRAITS_SPECIALIZATION(int64);

    DFG_TEMP_DEFINE_NUMERICTRAITS_SPECIALIZATION(uint8);
    DFG_TEMP_DEFINE_NUMERICTRAITS_SPECIALIZATION(uint16);
    DFG_TEMP_DEFINE_NUMERICTRAITS_SPECIALIZATION(uint32);
    DFG_TEMP_DEFINE_NUMERICTRAITS_SPECIALIZATION(uint64);

#undef DFG_TEMP_DEFINE_NUMERICTRAITS_SPECIALIZATION

#if !(DFG_IS_QT_AVAILABLE) // TODO: Add robust check. This is used because at least some versions of Qt seems to have ushort == wchar_t
    template <> struct NumericTraits<wchar_t> { static DFG_TEMP_NUMERIC_TRAITS_CONST_TYPE wchar_t maxValue = WCHAR_MAX; static DFG_TEMP_NUMERIC_TRAITS_CONST_TYPE wchar_t minValue = WCHAR_MIN; };
#endif

#undef DFG_TEMP_NUMERIC_TRAITS_CONST_TYPE

    template <size_t IntSize_T, bool IsSigned_T> struct IntegerTypeBySizeAndSign;
    template <> struct IntegerTypeBySizeAndSign<1, true>    { typedef int8   type; };
    template <> struct IntegerTypeBySizeAndSign<1, false>   { typedef uint8  type; };
    template <> struct IntegerTypeBySizeAndSign<2, true>    { typedef int16  type; };
    template <> struct IntegerTypeBySizeAndSign<2, false>   { typedef uint16 type; };
    template <> struct IntegerTypeBySizeAndSign<4, true>    { typedef int32  type; };
    template <> struct IntegerTypeBySizeAndSign<4, false>   { typedef uint32 type; };
    template <> struct IntegerTypeBySizeAndSign<8, true>    { typedef int64  type; };
    template <> struct IntegerTypeBySizeAndSign<8, false>   { typedef uint64 type; };

    // Returns minimum finite value of given integer or floating point type.
    template <class T> inline constexpr T minValueOfType()
    {
        DFG_STATIC_ASSERT(std::numeric_limits<T>::is_integer == true || std::is_floating_point_v<T>, "Only integer or floating point types are allowed");
        return (std::numeric_limits<T>::lowest)();
    }
    // Overload to allow easy checking based on existing object:
    // minValueOfType(val); instead of minValueOfType<decltype(val)>();
    template <class T> inline constexpr T minValueOfType(const T&) { return minValueOfType<T>(); }

    // Returns maximum finite value of given integer or floating point type.
    template <class T> inline constexpr T maxValueOfType()
    {
        DFG_STATIC_ASSERT(std::numeric_limits<T>::is_integer == true || std::is_floating_point_v<T>, "Only integer or floating point types are allowed");
        return (std::numeric_limits<T>::max)();
    }

    template <class T> inline constexpr T maxValueOfType(const T&) { return maxValueOfType<T>(); }

    template <class To_T, class From_T>
    constexpr bool isValWithinLimitsOfType(const From_T& val, const std::true_type, const std::true_type) // Both unsigned
    {
        return (val >= minValueOfType<To_T>() && val <= maxValueOfType<To_T>());
    }

    template <class To_T, class From_T>
    constexpr bool isValWithinLimitsOfType(const From_T& val, const std::false_type, const std::false_type) // Both signed
    {
        return (val >= minValueOfType<To_T>() && val <= maxValueOfType<To_T>());
    }

    template <class To_T, class From_T>
    bool isValWithinLimitsOfType(const From_T& val, const std::true_type, const std::false_type) // To_T unsigned, From_T signed
    {
        if (val < 0)
            return false;
        typedef typename std::make_unsigned<From_T>::type UFrom_T;
        return isValWithinLimitsOfType<To_T>(static_cast<UFrom_T>(val), std::true_type(), std::true_type());
    }

    template <class To_T, class From_T>
    constexpr bool isValWithinLimitsOfType(const From_T& val, const std::false_type, const std::true_type) // To_T signed, From_T unsigned
    {
        typedef typename std::make_unsigned<To_T>::type UTo_T;
        return val <= static_cast<UTo_T>(maxValueOfType<To_T>());
    }

    // Tests whether 'val' is within [minValueOfType<To_T>(), maxValueOfType<To_T>()].
    template <class To_T, class From_T>
    constexpr bool isValWithinLimitsOfType(const From_T& val)
    {
        return isValWithinLimitsOfType<To_T>(val, std::is_unsigned<To_T>(), std::is_unsigned<From_T>());
    }

    // Convenience overload to allow using variables.
    template <class From_T, class To_T>
    constexpr bool isValWithinLimitsOfType(const From_T& val, const To_T)
    {
        return isValWithinLimitsOfType<To_T>(val);
    }

    // Note: Min and Max differ from normal naming to avoid problems with windows headers that
    //       define min and max as macros.
    //template <class T> inline constexpr const T& Min(const T& v1, const T& v2) { return (std::min)(v1, v2); } // Worked in MSVC2015 and Clang, but failed on MinGW 7.3 so not using
    template <class T> inline constexpr const T& Min(const T& v1, const T& v2) { return (v1 <= v2) ? v1 : v2; }
    template <class T> inline constexpr const T& Min(const T& v1, const T& v2, const T& v3) { return Min(Min(v1, v2), v3); }
    template <class T> inline constexpr const T& Min(const T& v1, const T& v2, const T& v3, const T& v4) { return Min(Min(v1, v2, v3), v4); }

    //template <class T> inline constexpr const T& Max(const T& v1, const T& v2) { return (std::max)(v1, v2); } // Worked in MSVC2015 and Clang, but failed on MinGW 7.3 so not using
    template <class T> inline constexpr const T& Max(const T& v1, const T& v2) { return (v1 >= v2) ? v1 : v2; }
    template <class T> inline constexpr const T& Max(const T& v1, const T& v2, const T& v3) { return Max(Max(v1, v2), v3); }
    template <class T> inline constexpr const T& Max(const T& v1, const T& v2, const T& v3, const T& v4) { return Max(Max(v1, v2, v3), v4); }

    namespace DFG_DETAIL_NS
    {
        template <class Int_T>
        inline constexpr auto absAsUnsigned(const Int_T val, std::true_type) -> typename std::make_unsigned<Int_T>::type
        {
            DFG_STATIC_ASSERT(std::is_unsigned<Int_T>::value, "absAsUnsigned: usage error - unsigned version called for signed ");
            return val;
        }

        template <class Int_T>
        inline constexpr auto absAsUnsigned(const Int_T val, std::false_type) -> typename std::make_unsigned<Int_T>::type
        {
            typedef typename std::make_unsigned<Int_T>::type UnsignedType;
            if (val >= 0)
                return static_cast<UnsignedType>(val);
            else
            {
                if (val != (std::numeric_limits<Int_T>::min)())
                    return static_cast<UnsignedType>(-1 * val);
                else
                    return static_cast<UnsignedType>(-1 * (val + 1)) + 1;
            }
        }
    } // DFG_DETAIL_NS

    // For given integer value, returns it's absolute value as unsigned type. The essential differences to std::abs() are:
    //      -Return value is unsigned type
    //      -Return value is mathematically correct even when called with value std::numeric_limits<Int_T>::min();
    template <class Int_T>
    constexpr auto absAsUnsigned(const Int_T val) -> typename std::make_unsigned<Int_T>::type
    {
        return DFG_DETAIL_NS::absAsUnsigned(val, typename std::is_unsigned<Int_T>::type());
    }

    template <class Dst_T, class Src_T>
    constexpr Dst_T saturateCast(const Src_T val);

    namespace DFG_DETAIL_NS
    {
        // Smallar signed to signed
        template <class Dst_T, class Src_T>
        constexpr Dst_T saturateCastImpl(const Src_T val, std::true_type /*src signed*/, std::true_type /*dst signed*/, std::true_type /*sizeof(src) < sizeof(dst)*/)
        {
            return val;
        }

        // Larger or equal-sized signed to signed.
        template <class Dst_T, class Src_T>
        constexpr Dst_T saturateCastImpl(const Src_T val, std::true_type /*src signed*/, std::true_type /*dst signed*/, std::false_type /*sizeof(src) >= sizeof(dst)*/)
        {
            return (val >= 0) ? static_cast<Dst_T>(Min(val, Src_T((std::numeric_limits<Dst_T>::max)()))) : static_cast<Dst_T>(Max(val, Src_T((std::numeric_limits<Dst_T>::min)())));
        }

        // Smaller signed to unsigned.
        template <class Dst_T, class Src_T>
        constexpr Dst_T saturateCastImpl(const Src_T val, std::true_type /*src signed*/, std::false_type /*dst unsigned*/, std::true_type /*sizeof(src) < sizeof(dst)*/)
        {
            return (val >= 0) ? static_cast<Dst_T>(val) : 0;
        }

        // Larger or equal-sized signed to unsigned.
        template <class Dst_T, class Src_T>
        constexpr Dst_T saturateCastImpl(const Src_T val, std::true_type /*src signed*/, std::false_type /*dst unsigned*/, std::false_type /*sizeof(src) >= sizeof(dst)*/)
        {
            return (val >= 0) ? saturateCast<Dst_T>(static_cast<typename std::make_unsigned<Src_T>::type>(val)) : 0;
        }

        // Smaller unsigned to signed
        template <class Dst_T, class Src_T>
        constexpr Dst_T saturateCastImpl(const Src_T val, std::false_type /*src unsigned*/, std::true_type /*dst signed*/, std::true_type /*sizeof(src) < sizeof(dst)*/)
        {
            return static_cast<Dst_T>(val);
        }

        // Larger or equal-sized unsigned to signed.
        template <class Dst_T, class Src_T>
        constexpr Dst_T saturateCastImpl(const Src_T val, std::false_type /*src unsigned*/, std::true_type /*dst signed*/, std::false_type /*sizeof(src) >= sizeof(dst)*/)
        {
            return static_cast<Dst_T>(Min(val, static_cast<Src_T>((std::numeric_limits<Dst_T>::max)())));
        }

        // Smaller unsigned to unsigned
        template <class Dst_T, class Src_T>
        constexpr Dst_T saturateCastImpl(const Src_T val, std::false_type /*src unsigned*/, std::false_type /*dst unsigned*/, std::true_type /*sizeof(src) < sizeof(dst)*/)
        {
            return val;
        }

        // Larger or equal-sized unsigned to unsigned.
        template <class Dst_T, class Src_T>
        constexpr Dst_T saturateCastImpl(const Src_T val, std::false_type /*src unsigned*/, std::false_type /*dst unsigned*/, std::false_type /*sizeof(src) >= sizeof(dst)*/)
        {
            return static_cast<Dst_T>(Min(val, static_cast<Src_T>((std::numeric_limits<Dst_T>::max)())));
        }
    } // DFG_DETAIL_NS

    // Returns value casted from Src_T to Dst_T so that if input value is not within [min, max] of Dst_T, returns min if val < min and max if val > max
    template <class Dst_T, class Src_T>
    constexpr Dst_T saturateCast(const Src_T val)
    {
        return DFG_DETAIL_NS::saturateCastImpl<Dst_T, Src_T>(val, std::is_signed<Src_T>(), std::is_signed<Dst_T>(), std::integral_constant<bool, sizeof(Src_T) < sizeof(Dst_T)>());
    }

    namespace DFG_DETAIL_NS
    {
        template <class Ret_T, class T1, class T2>
        Ret_T saturateAddNonNegative(const T1 a, const T2 b)
        {
            DFG_ASSERT_UB(a >= 0 && b >= 0);
            constexpr auto maxRv = maxValueOfType<Ret_T>();
            if (a >= maxRv || b >= maxRv)
                return maxRv;
            Ret_T rv = static_cast<Ret_T>(a);
            return rv + Min(static_cast<Ret_T>(b), static_cast<Ret_T>(maxRv - rv));
        }

        template <class Ret_T, class T1, class T2>
        Ret_T saturateAddNonPositive(const T1 a, const T2 b)
        {
            DFG_ASSERT_UB(a <= 0 && b <= 0);
            constexpr auto minRv = minValueOfType<Ret_T>();
            if (a <= minRv || b <= minRv)
                return minRv;
            Ret_T rv = static_cast<Ret_T>(a);
            return rv + Max(static_cast<Ret_T>(b), static_cast<Ret_T>(minRv - rv));
        }

        template <class Ret_T, class T> Ret_T negateOrZero(const T, std::true_type)      { return 0; }
        template <class Ret_T, class T> Ret_T negateOrZero(const T val, std::false_type) { return -static_cast<Ret_T>(val); }

        template <class Ret_T, class Pos_T, class Neg_T>
        Ret_T saturateAddMixedImpl(const Pos_T pos, const Neg_T neg)
        {
            DFG_ASSERT_UB(pos > 0 && neg < 0);
            constexpr auto nBiggerArgSize = (sizeof(Pos_T) >= sizeof(Neg_T)) ? sizeof(Pos_T) : sizeof(Neg_T);
            using UArg = typename IntegerTypeBySizeAndSign<nBiggerArgSize, false>::type;
            const auto negAbs = ::DFG_ROOT_NS::absAsUnsigned(neg);
            if (static_cast<UArg>(pos) >= negAbs) // Result non-negative?
                return saturateCast<Ret_T>(static_cast<UArg>(pos) - negAbs);
            else // Case: result negative
            {
                const auto rvAbs = static_cast<UArg>(negAbs) - static_cast<UArg>(pos);
                return (static_cast<UArg>(rvAbs) <= ::DFG_ROOT_NS::absAsUnsigned(minValueOfType<Ret_T>())) ? negateOrZero<Ret_T>(rvAbs, std::is_unsigned<Ret_T>()) : minValueOfType<Ret_T>();
            }
        }

        template <class Ret_T, class T1, class T2>
        Ret_T saturateAddMixed(const T1 a, const T2 b)
        {
            if (a < 0)
                return saturateAddMixedImpl<Ret_T>(b, a);
            else
                return saturateAddMixedImpl<Ret_T>(a, b);
        }

        template <class Ret_T, class T1, class T2>
        Ret_T saturateAddImpl(const T1 a, const T2 b, std::true_type, std::true_type)
        {
            return DFG_DETAIL_NS::saturateAddNonNegative<Ret_T>(a, b);
        }

        template <class Ret_T, class T1, class T2, class IsUnsigned_T1, class IsUnsigned_T2>
        Ret_T saturateAddImpl(const T1 a, const T2 b, IsUnsigned_T1, IsUnsigned_T2)
        {
            DFG_STATIC_ASSERT(IsUnsigned_T1::value == false || IsUnsigned_T2::value == false, "saturateAddImpl(): This version should not get called when both args are unsigned.");
            if (a >= 0 && b >= 0)
                return DFG_DETAIL_NS::saturateAddNonNegative<Ret_T>(a, b);
            else if (a <= 0 && b <= 0)
                return DFG_DETAIL_NS::saturateAddNonPositive<Ret_T>(a, b);
            else // a and c have different signs
                return DFG_DETAIL_NS::saturateAddMixed<Ret_T>(a, b);
        }
    } // DFG_DETAIL_NS

    // Conceptually returns saturateCast<Ret_T>(a + b) taking care that a + b doesn't wrap or overflow but instead results to minimum/maximum value of Ret_T.
    template <class Ret_T, class T1, class T2>
    Ret_T saturateAdd(const T1 a, const T2 b)
    {
        DFG_STATIC_ASSERT((std::is_integral<Ret_T>::value && std::is_integral<T1>::value && std::is_integral<T2>::value), "saturateAdd: types must be integers");
        return DFG_DETAIL_NS::saturateAddImpl<Ret_T>(a, b, typename std::is_unsigned<T1>::type(), typename std::is_unsigned<T2>::type());
    }


    // min/maxValue
    namespace DFG_DETAIL_NS
    {
        using ValueArgClass_same = std::integral_constant<int, 0>;
        using ValueArgClass_integer = std::integral_constant<int, 1>;

        // Case: minValue for identical types.
        template <class T>
        constexpr auto minValueImpl(const T a, const T b, ValueArgClass_same) -> T
        {
            return Min(a, b);
        }

        // Helper for determining common integer type for minValue.
        template <class T0, class T1>
        struct CommonIntegerMinType {
            using type = typename std::conditional<
                std::is_signed<T0>::value == std::is_signed<T1>::value,            // Do types have the same sign?
                decltype(T0() + T1()),                                             // If yes, then use decltype(T0() + T1())
                typename std::conditional<std::is_signed<T0>::value, T0, T1>::type // Otherwise use signed type.
            >::type;
        }; // struct CommonIntegerMinType

        // Helper for determining common integer type for maxValue.
        template <class T0, class T1>
        struct CommonIntegerMaxType {
            using type = typename std::conditional<
                std::is_signed<T0>::value == std::is_signed<T1>::value,              // Do types have the same sign?
                decltype(T0() + T1()),                                               // If yes, then use decltype(T0() + T1())
                typename std::conditional<!std::is_signed<T0>::value, T0, T1>::type  // Otherwise use unsigned type.
            >::type;
        }; // struct CommonIntegerMaxType

        // Case: Integers with the same sign, casting to common type
        template <class T0, class T1>
        constexpr auto minValueImpl(const T0 a, const T1 b, ValueArgClass_integer, std::true_type) -> decltype(a + b)
        {
            using CommonType = decltype(a + b);
            return minValueImpl(CommonType(a), CommonType(b), ValueArgClass_same());
        }

        // Case: Integers with different sign, using saturateCast() to target type.
        template <class T0, class T1>
        constexpr auto minValueImpl(const T0 a, const T1 b, ValueArgClass_integer, std::false_type) -> typename CommonIntegerMinType<T0, T1>::type
        {
            using ReturnType = typename CommonIntegerMinType<T0, T1>::type;
            return minValueImpl(saturateCast<ReturnType>(a), saturateCast<ReturnType>(b), ValueArgClass_same());
        }

        // Entry point for integer types passing to implementation depending on sign difference.
        template <class T0, class T1>
        constexpr auto minValueImpl(const T0 a, const T1 b, ValueArgClass_integer) -> typename CommonIntegerMinType<T0, T1>::type
        {
            return minValueImpl(a, b, ValueArgClass_integer(), std::integral_constant<bool, std::is_signed<T0>::value == std::is_signed<T1>::value>());
        }

        /////////////////////////////////////////////////////
        // maxValue below

        // Case: maxValue for identical types
        template <class T>
        constexpr auto maxValueImpl(const T a, const T b, ValueArgClass_same) -> T
        {
            return Max(a, b);
        }

        // Case: The same sign, casting to common type
        template <class T0, class T1>
        constexpr auto maxValueImpl(const T0 a, const T1 b, ValueArgClass_integer, std::true_type) -> decltype(a + b)
        {
            using CommonType = decltype(a + b);
            return maxValueImpl(CommonType(a), CommonType(b), ValueArgClass_same());
        }

        // Case: different sign, using saturateCast() to target type.
        template <class T0, class T1>
        constexpr auto maxValueImpl(const T0 a, const T1 b, ValueArgClass_integer, std::false_type) -> typename CommonIntegerMaxType<T0, T1>::type
        {
            using ReturnType = typename CommonIntegerMaxType<T0, T1>::type;
            return maxValueImpl(saturateCast<ReturnType>(a), saturateCast<ReturnType>(b), ValueArgClass_same());
        }

        // Entry point for integer types passing to implementation depending on sign difference.
        template <class T0, class T1>
        constexpr auto maxValueImpl(const T0 a, const T1 b, ValueArgClass_integer) -> typename CommonIntegerMaxType<T0, T1>::type
        {
            return maxValueImpl(a, b, ValueArgClass_integer(), std::integral_constant<bool, std::is_signed<T0>::value == std::is_signed<T1>::value>());
        }
    } // namespace DFG_DETAIL_NS

    // Like std::min(), but returns a copy and can be used also with different types including signed/unsigned mixes.
    template <class T0, class T1>
    constexpr auto minValue(const T0 a, const T1 b) -> typename ::DFG_ROOT_NS::DFG_DETAIL_NS::CommonIntegerMinType<T0, T1>::type
    {
        DFG_STATIC_ASSERT((std::is_same<T0, T1>::value || (std::is_integral<T0>::value && std::is_integral<T1>::value)), "minValue requries either identical types or integer types");
        return ::DFG_ROOT_NS::DFG_DETAIL_NS::minValueImpl(a, b, std::integral_constant<int, std::is_integral<T0>::value && std::is_integral<T1>::value>());
    }

    // Convenience overloads for minValue 3 and 4 argument cases.
    template <class T0, class T1, class T2>           constexpr auto minValue(T0 a, T1 b, T2 c)       -> decltype(minValue(a, minValue(b, c)))              { return minValue(a, minValue(b, c)); }
    template <class T0, class T1, class T2, class T3> constexpr auto minValue(T0 a, T1 b, T2 c, T3 d) -> decltype(minValue(a, minValue(b, minValue(c, d)))) { return minValue(a, minValue(b, minValue(c, d))); }

    // Like std::max(), but returns a copy and can be used also with different types including signed/unsigned mixes.
    template <class T0, class T1>
    constexpr auto maxValue(const T0 a, const T1 b) -> typename ::DFG_ROOT_NS::DFG_DETAIL_NS::CommonIntegerMaxType<T0, T1>::type
    {
        DFG_STATIC_ASSERT((std::is_same<T0, T1>::value || (std::is_integral<T0>::value && std::is_integral<T1>::value)), "maxValue requries either identical types or integer types");
        return ::DFG_ROOT_NS::DFG_DETAIL_NS::maxValueImpl(a, b, std::integral_constant<int, std::is_integral<T0>::value && std::is_integral<T1>::value>());
    }

    // Convenience overloads for maxValue 3 and 4 argument cases.
    template <class T0, class T1, class T2>           constexpr auto maxValue(T0 a, T1 b, T2 c)       -> decltype(maxValue(a, maxValue(b, c)))              { return maxValue(a, maxValue(b, c)); }
    template <class T0, class T1, class T2, class T3> constexpr auto maxValue(T0 a, T1 b, T2 c, T3 d) -> decltype(maxValue(a, maxValue(b, maxValue(c, d)))) { return maxValue(a, maxValue(b, maxValue(c, d))); }

    // Defining floating point overloads, which guarantee proper NaN-handling like std::fmin/fmax. 
    // Using overloads instead of e.g. specializing underlying std::min/max function since
    // fmin/fmax are not constexpr and constexpr call chain ending to non-constexpr call causes compile error.
#define DFG_TEMP_DEFINE_FLOATING_POINT_OVERLOADS(FUNC, TYPE, IMPLFUNC) \
    inline TYPE FUNC(TYPE a, TYPE b)                   { return IMPLFUNC(a, b); } \
    inline TYPE FUNC(TYPE a, TYPE b, TYPE c)           { return IMPLFUNC(a, IMPLFUNC(b, c)); } \
    inline TYPE FUNC(TYPE a, TYPE b, TYPE c, TYPE d)   { return IMPLFUNC(a, IMPLFUNC(b, IMPLFUNC(c, d))); }

    DFG_TEMP_DEFINE_FLOATING_POINT_OVERLOADS(minValue, float,       std::fmin)
    DFG_TEMP_DEFINE_FLOATING_POINT_OVERLOADS(minValue, double,      std::fmin)
    DFG_TEMP_DEFINE_FLOATING_POINT_OVERLOADS(minValue, long double, std::fmin)

    DFG_TEMP_DEFINE_FLOATING_POINT_OVERLOADS(maxValue, float,       std::fmax)
    DFG_TEMP_DEFINE_FLOATING_POINT_OVERLOADS(maxValue, double,      std::fmax)
    DFG_TEMP_DEFINE_FLOATING_POINT_OVERLOADS(maxValue, long double, std::fmax)
#undef DFG_TEMP_DEFINE_FLOATING_POINT_OVERLOADS

} // namespace DFG_ROOT_NS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(math) {

// From http://isocpp.org/wiki/faq/intrinsic-types#is-power-of-2
template <class T>
inline constexpr bool isPowerOf2(const T i)
{
    return i > 0 && (i & (i - 1)) == 0;
}

// Returns true iff value is not infinite or NaN.
template <class T>
inline bool isFinite(const T t)
{
#if DFG_LANGFEAT_HAS_ISNAN // Hack: approximate that presence of isnan() implies presence of isfinite()
    return std::isfinite(t);
#elif defined(_MSC_VER)
    return (_finite(t) != 0);
#else
    return (isfinite(t) != 0);
#endif
}

template <class T>
inline bool isInf(const T t)
{
#if DFG_LANGFEAT_HAS_ISNAN // Hack: approximate that presence of isnan() implies presence of isinf()
    return std::isinf(t);
#else
    return t == std::numeric_limits<T>::infinity() || t == -1 * std::numeric_limits<T>::infinity();
#endif
}

namespace DFG_DETAIL_NS
{
#ifdef _MSC_VER
    inline bool IsNanMsvc(float t) { return _isnan(t) != 0; }
    inline bool IsNanMsvc(double t) { return _isnan(t) != 0; }
    inline bool IsNanMsvc(long double t) { return IsNanMsvc(static_cast<double>(t)); } // _isnan() does not have long double overload causing 'conversion from 'const long double' to 'double', possible loss of data' warnings.
#endif // _MSC_VER

    template <class T>
    inline bool isNanImpl(const T t, std::true_type /*T has NaN*/)
    {
    #if DFG_LANGFEAT_HAS_ISNAN
        return (std::isnan(t) != 0);
    #elif defined(_MSC_VER)
        return DFG_DETAIL_NS::IsNanMsvc(t);
    #else
        return (isnan(t) != 0);
    #endif
    }

    template <class T>
    inline constexpr bool isNanImpl(const T, std::false_type /*case: T does not have NaN*/)
    {
        return false;
    }
} // DFG_DETAIL_NS namespace


template <class T>
inline bool isNan(const T t)
{
    DFG_STATIC_ASSERT(std::is_floating_point<T>::value || std::is_integral<T>::value, "isNan() expected numeric type");
    return DFG_DETAIL_NS::isNanImpl(t, std::integral_constant<bool, std::numeric_limits<T>::has_quiet_NaN>());
}

namespace DFG_DETAIL_NS
{
    template <class Val_T>
    inline bool isIntegerValuedImpl(const Val_T, std::true_type) // case: Val_T is integer
    {
        return true;
    }

    template <class Val_T>
    inline bool isIntegerValuedImpl(const Val_T val, std::false_type) // case: Val_T is not integer
    {
        DFG_STATIC_ASSERT(std::is_floating_point<Val_T>::value, "isIntegerValued() expects either integer or floating point type");
        if (!isFinite(val))
            return false;
        Val_T intPart;
        return std::modf(val, &intPart) == 0;
    }

    // Case where integer type min/max can be represented as floating point value
    // Precondition: isIntegerValued(f) == true
    template <class Int_T, class Float_T>
    bool isIntegerFloatConvertibleTo(const Float_T f, Int_T* pInt, std::true_type)
    {
        if (f >= static_cast<Float_T>((std::numeric_limits<Int_T>::min)()) && f <= static_cast<Float_T>((std::numeric_limits<Int_T>::max)()))
        {
            if (pInt)
                *pInt = static_cast<Int_T>(f);
            DFG_ASSERT_CORRECTNESS(static_cast<Float_T>(static_cast<Int_T>(f)) == f);
            return true;
        }
        else
            return false;
    }

    // Case where integer type min/max can not be represented as floating point value
    // Precondition: isIntegerValued(f) == true
    template <class Int_T, class Float_T>
    bool isIntegerFloatConvertibleTo(const Float_T f, Int_T* pInt, std::false_type)
    {
        int exp;
        const auto factor = std::frexp(f, &exp); // exp is value k in f = a * 2^k, where |a| is in [0.5, 1[
        if (factor < 0 && std::is_unsigned<Int_T>::value)
            return false;
        if (exp <= std::numeric_limits<Int_T>::digits)
        {
            if (pInt)
                *pInt = static_cast<Int_T>(f);
            DFG_ASSERT_CORRECTNESS(static_cast<Float_T>(static_cast<Int_T>(f)) == f);
            return true;
        }
        else
            return false;
    }
} // namespace DFG_DETAIL_NS

// Returns true iff value is integer value.
// Return values for some special floating point values:
//      -NaN's: false
//      -+-inf: false
// Note: returned true does not mean that the value can be represented e.g. as int (value can for example be > INT_MAX)
template <class Val_T>
inline bool isIntegerValued(const Val_T val)
{
    return DFG_DETAIL_NS::isIntegerValuedImpl(val, std::is_integral<Val_T>());
}

// Given an integer and radix, returns digit character for multiplier of nRadix^nPos.
// For example with 'nRadix' == 10 and 'n' == 456:
//      'nPos == 0' -> '6'
//      'nPos == 1' -> '5'
//      'nPos == 2' -> '4',
//      'nPos  > 2' -> '0'
// @param n Integer to extract digit from
// @param nPos zero-based index which digit to returns, 0 is little end
// @param nRadix Radix whose representation to use, valid range is [2, 36]
// @return Digit at given pos or if nRadix is invalid, returns '?'. Characters for non-numbers are lower case.
template <class Int_T>
constexpr char integerDigitAtPos(const Int_T n, const size_t nPos, const size_t nRadix = 10)
{
    DFG_STATIC_ASSERT(std::is_integral_v<Int_T>, "Int_T must be an integer");
    if (nRadix < 2 || nRadix > 36)
        return '?';
    constexpr char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    using UIntT = std::make_unsigned_t<Int_T>;
    const auto nUnsignedN = absAsUnsigned(n);
    UIntT nExpInt = 1;
    for (size_t i = 0; i <= nPos; ++i)
    {
        if ((std::numeric_limits<UIntT>::max)() / nRadix < nExpInt)
            return (i == nPos) ? digits[nUnsignedN / nExpInt] : '0';
        nExpInt *= static_cast<UIntT>(nRadix);
    }
    // Getting rid of high digits, e.g. with radix 10 when extrating pos 2 from 123456 (i.e. '4'),
    // nExpInt == 1000 -> nRemainder = 123456 % 1000 = 456
    const auto nRemainder = nUnsignedN % nExpInt;
    const auto nDigit = nRemainder / (nExpInt / nRadix); // With 456, pos 2 and radix 10, this is floor(456 / 100) = 4.
    return digits[nDigit];
}

namespace DFG_DETAIL_NS
{
    // Implementation for case where source and target types are the same.
    template <class Val_T>
    bool isFloatConvertibleToImpl(const Val_T f, Val_T* pDst, std::true_type, Dummy)
    {
        if (pDst)
            *pDst = f;
        return true;
    }

    // Implementation for case where target type is integer.
    template <class Int_T, class Float_T>
    bool isFloatConvertibleToImpl(const Float_T f, Int_T* pInt, std::false_type, std::true_type)
    {
        DFG_STATIC_ASSERT(!(std::is_same<Int_T, Float_T>::value), "This overload should get called only for non-identical types");
        DFG_STATIC_ASSERT(std::is_integral<Int_T>::value, "Currently only integer types are supported as target types");
        DFG_STATIC_ASSERT(std::numeric_limits<Float_T>::is_iec559, "isFloatConvertibleTo() probably requires IEC 559 (IEEE 754) floating points");
        if (!isIntegerValued(f))
            return false;
        return isIntegerFloatConvertibleTo(f, pInt, std::integral_constant<bool, std::numeric_limits<Float_T>::digits >= std::numeric_limits<Int_T>::digits>());
    }

    // Floating point promotion
    // https://en.cppreference.com/mwiki/index.php?title=cpp/language/implicit_conversion&oldid=131250
    //      "Floating-point promotion
    //      A prvalue of type float can be converted to a prvalue of type double.The value does not change."
    template <class DstFloat_T, class SrcFloat_T>
    bool isFloatConvertibleToFloatImpl(const SrcFloat_T f, DstFloat_T* pDst, std::true_type)
    {
        if (pDst)
            *pDst = f;
        return true;
    }

    // Floating point conversion
    // https://en.cppreference.com/mwiki/index.php?title=cpp/language/implicit_conversion&oldid=131250
    //      "Floating-point conversions
    //      A prvalue of a floating-point type can be converted to a prvalue of any other floating-point type. If the conversion is listed under floating-point promotions, it is a promotion and not a conversion.
    //
    //      -If the source value can be represented exactly in the destination type, it does not change.
    //      -If the source value is between two representable values of the destination type, the result is one of those two values(it is implementation-defined which one, although if IEEE arithmetic is supported, rounding defaults to nearest).
    //      -Otherwise, the behavior is undefined."
    //        
    template <class DstFloat_T, class SrcFloat_T>
    bool isFloatConvertibleToFloatImpl(const SrcFloat_T f, DstFloat_T* pDst, std::false_type)
    {
        if (isInf(f))
        {
            if (pDst)
                *pDst = static_cast<DstFloat_T>(f);
            return true;
        }
        if (f > (std::numeric_limits<DstFloat_T>::max)())
            return false;
        if (f < (std::numeric_limits<DstFloat_T>::lowest)())
            return false;
        if (std::fabs(f) < (std::numeric_limits<DstFloat_T>::min)())
            return false;
        const DstFloat_T dst = static_cast<DstFloat_T>(f);
        const auto rv = (dst == f);
        if (rv && pDst)
            *pDst = dst;
        return rv;
    }

    // Implementation for case where target type is floating point and types are different.
    template <class DstFloat_T, class SrcFloat_T>
    bool isFloatConvertibleToImpl(const SrcFloat_T f, DstFloat_T* pDst, std::false_type, std::false_type)
    {
        DFG_STATIC_ASSERT(!(std::is_same<SrcFloat_T, DstFloat_T>::value), "This overload should get called only for non-identical types");
        DFG_STATIC_ASSERT(std::numeric_limits<SrcFloat_T>::is_iec559, "isFloatConvertibleTo() probably requires IEC 559 (IEEE 754) floating points");
        DFG_STATIC_ASSERT(std::numeric_limits<DstFloat_T>::is_iec559, "isFloatConvertibleTo() probably requires IEC 559 (IEEE 754) floating points");

        if (isNan(f))
            return false;

        const bool bIsPromotion = (sizeof(DstFloat_T) > sizeof(SrcFloat_T));
        return isFloatConvertibleToFloatImpl(f, pDst, std::integral_constant<bool, bIsPromotion>());
    }
} // namespace DFG_DETAIL_NS

// Returns true if floating point value can be represent exactly as given type. If pTarget is given and return value is true, it receives the value as Target_T.
// Notes: 
//      -NaN's are considered convertible to target only when both types are identical.
//      -inf's are convertible.
template <class Target_T, class Float_T>
bool isFloatConvertibleTo(const Float_T f, Target_T* pTarget = nullptr)
{
    return DFG_DETAIL_NS::isFloatConvertibleToImpl(f, pTarget, std::is_same<Target_T, Float_T>(), std::is_integral<Target_T>());
}

namespace DFG_DETAIL_NS
{
    template <class T>
    typename std::make_unsigned<T>::type numericDistanceIntegerImpl(const T lesser, const T greater, std::true_type) // case: T is signed
    {
        using UnsignedT = typename std::make_unsigned<T>::type;
        if (greater < 0 || lesser >= 0)
        {
            // Both negative or both non-negative, can't overflow, even if lesser is NumericTraits<T>::minValue (according to answers in below link)
            // https://stackoverflow.com/questions/19015444/is-int-min-subtracted-from-any-integer-considered-undefined-behavior
            return UnsignedT(greater - lesser);
        }
        else
            return UnsignedT(greater) + ::DFG_ROOT_NS::absAsUnsigned(lesser); // Different signs, adding as unsigned.
    }

    template <class T>
    typename std::make_unsigned<T>::type numericDistanceIntegerImpl(const T lesser, const T greater, std::false_type) // case: T is unsigned
    {
        return greater - lesser; // T is unsigned.
    }

    template <class T> struct NumericDistanceReturnType       { using type = std::make_unsigned_t<T>; };
    template <> struct NumericDistanceReturnType<float>       { using type = float; };
    template <> struct NumericDistanceReturnType<double>      { using type = double; };
    template <> struct NumericDistanceReturnType<long double> { using type = long double; };
}

// Returns distance of two numbers:
//      -For floating point values return abs(a - b)
//      -For integers returns distance as unsigned integer guaranteed to be computed in overflow (undefined behaviour) safe manner.
// Rationale: given two signed integer values lesser and greater, computing their distance can't generally be done greater - lesser, because that may overflow.
template <class T>
auto numericDistance(const T a, const T b) -> typename DFG_DETAIL_NS::NumericDistanceReturnType<T>::type
{
    if constexpr (std::is_integral<T>())
    {
        const auto lesser = Min(a, b);
        const auto greater = Max(a, b);
        return DFG_DETAIL_NS::numericDistanceIntegerImpl(lesser, greater, std::is_signed<T>());
    }
    else
    {
        return std::abs(a - b);
    }
}

} } // namespace dfg::math
