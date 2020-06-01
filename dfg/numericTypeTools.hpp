#pragma once

#include "dfgDefs.hpp"
#include "dfgBaseTypedefs.hpp"
#include <limits>
#include <type_traits>
#include <algorithm> // for std::max

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

    // Returns minimum value of given integer type.
    template <class T> inline T minValueOfType()
    {
        DFG_STATIC_ASSERT(std::numeric_limits<T>::is_integer == true, "Only interger types are allowed.");
        return (std::numeric_limits<T>::min)();
    }
    // Overload to allow easy checking based on existing object:
    // minValueOfType(val); instead of minValueOfType<decltype(val)>();
    template <class T> inline T minValueOfType(const T&) { return minValueOfType<T>(); }

    // Returns maximum value of given integer type.
    template <class T> inline T maxValueOfType()
    {
        DFG_STATIC_ASSERT(std::numeric_limits<T>::is_integer == true, "Only integer types are allowed.");
        return (std::numeric_limits<T>::max)();
    }

    template <class T> inline T maxValueOfType(const T&) { return maxValueOfType<T>(); }

    template <class To_T, class From_T>
    bool isValWithinLimitsOfType(const From_T& val, const std::true_type, const std::true_type) // Both unsigned
    {
        return (val >= minValueOfType<To_T>() && val <= maxValueOfType<To_T>());
    }

    template <class To_T, class From_T>
    bool isValWithinLimitsOfType(const From_T& val, const std::false_type, const std::false_type) // Both signed
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
    bool isValWithinLimitsOfType(const From_T& val, const std::false_type, const std::true_type) // To_T signed, From_T unsigned
    {
        typedef typename std::make_unsigned<To_T>::type UTo_T;
        return val <= static_cast<UTo_T>(maxValueOfType<To_T>());
    }

    // Tests whether 'val' is within [minValueOfType<To_T>(), maxValueOfType<To_T>()].
    template <class To_T, class From_T>
    bool isValWithinLimitsOfType(const From_T& val)
    {
        return isValWithinLimitsOfType<To_T>(val, std::is_unsigned<To_T>(), std::is_unsigned<From_T>());
    }

    // Convenience overload to allow using variables.
    template <class From_T, class To_T>
    bool isValWithinLimitsOfType(const From_T& val, const To_T)
    {
        return isValWithinLimitsOfType<To_T>(val);
    }

    // Note: Min and Max differ from normal naming to avoid problems with windows headers that
    //       define min and max as macros.
    template <class T> inline const T& Min(const T& v1, const T& v2) { return (std::min)(v1, v2); }
    template <class T> inline const T& Min(const T& v1, const T& v2, const T& v3) { return Min(Min(v1, v2), v3); }
    template <class T> inline const T& Min(const T& v1, const T& v2, const T& v3, const T& v4) { return Min(Min(v1, v2, v3), v4); }

    template <class T> inline const T& Max(const T& v1, const T& v2) { return (std::max)(v1, v2); }
    template <class T> inline const T& Max(const T& v1, const T& v2, const T& v3) { return Max(Max(v1, v2), v3); }
    template <class T> inline const T& Max(const T& v1, const T& v2, const T& v3, const T& v4) { return Max(Max(v1, v2, v3), v4); }

    template <class Dst_T, class Src_T>
    Dst_T saturateCast(const Src_T val);

    namespace DFG_DETAIL_NS
    {
        // Smallar signed to signed
        template <class Dst_T, class Src_T>
        Dst_T saturateCastImpl(const Src_T val, std::true_type /*src signed*/, std::true_type /*dst signed*/, std::true_type /*sizeof(src) < sizeof(dst)*/)
        {
            return val;
        }

        // Larger or equal-sized signed to signed.
        template <class Dst_T, class Src_T>
        Dst_T saturateCastImpl(const Src_T val, std::true_type /*src signed*/, std::true_type /*dst signed*/, std::false_type /*sizeof(src) >= sizeof(dst)*/)
        {
            if (val >= 0)
                return static_cast<Dst_T>(Min(val, Src_T(NumericTraits<Dst_T>::maxValue)));
            else
                return static_cast<Dst_T>(Max(val, Src_T(NumericTraits<Dst_T>::minValue)));
        }

        // Smaller signed to unsigned.
        template <class Dst_T, class Src_T>
        Dst_T saturateCastImpl(const Src_T val, std::true_type /*src signed*/, std::false_type /*dst unsigned*/, std::true_type /*sizeof(src) < sizeof(dst)*/)
        {
            return (val >= 0) ? static_cast<Dst_T>(val) : 0;
        }

        // Larger or equal-sized signed to unsigned.
        template <class Dst_T, class Src_T>
        Dst_T saturateCastImpl(const Src_T val, std::true_type /*src signed*/, std::false_type /*dst unsigned*/, std::false_type /*sizeof(src) >= sizeof(dst)*/)
        {
            return (val >= 0) ? saturateCast<Dst_T>(static_cast<typename std::make_unsigned<Src_T>::type>(val)) : 0;
        }

        // Smaller unsigned to signed
        template <class Dst_T, class Src_T>
        Dst_T saturateCastImpl(const Src_T val, std::false_type /*src unsigned*/, std::true_type /*dst signed*/, std::true_type /*sizeof(src) < sizeof(dst)*/)
        {
            return static_cast<Dst_T>(val);
        }

        // Larger or equal-sized unsigned to signed.
        template <class Dst_T, class Src_T>
        Dst_T saturateCastImpl(const Src_T val, std::false_type /*src unsigned*/, std::true_type /*dst signed*/, std::false_type /*sizeof(src) >= sizeof(dst)*/)
        {
            return static_cast<Dst_T>(Min(val, static_cast<Src_T>(NumericTraits<Dst_T>::maxValue)));
        }

        // Smaller unsigned to unsigned
        template <class Dst_T, class Src_T>
        Dst_T saturateCastImpl(const Src_T val, std::false_type /*src unsigned*/, std::false_type /*dst unsigned*/, std::true_type /*sizeof(src) < sizeof(dst)*/)
        {
            return val;
        }

        // Larger or equal-sized unsigned to unsigned.
        template <class Dst_T, class Src_T>
        Dst_T saturateCastImpl(const Src_T val, std::false_type /*src unsigned*/, std::false_type /*dst unsigned*/, std::false_type /*sizeof(src) >= sizeof(dst)*/)
        {
            return static_cast<Dst_T>(Min(val, static_cast<Src_T>(NumericTraits<Dst_T>::maxValue)));
        }
    } // DFG_DETAIL_NS

    // Returns value casted from Src_T to Dst_T so that if input value is not within [min, max] of Dst_T, returns min if val < min and max if val > max
    template <class Dst_T, class Src_T>
    Dst_T saturateCast(const Src_T val)
    {
        return DFG_DETAIL_NS::saturateCastImpl<Dst_T, Src_T>(val, std::is_signed<Src_T>(), std::is_signed<Dst_T>(), std::integral_constant<bool, sizeof(Src_T) < sizeof(Dst_T)>());
    }

} // namespace
