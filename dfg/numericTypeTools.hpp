#pragma once

#include "dfgDefs.hpp"
#include <limits>
#include <type_traits>

DFG_ROOT_NS_BEGIN
{
    template <class T> struct NumericTraits {};
    template <> struct NumericTraits<int8> { static const int8 maxValue = int8_max; static const int8 minValue = int8_min; };
    template <> struct NumericTraits<int16> { static const int16 maxValue = int16_max; static const int16 minValue = int16_min; };
    template <> struct NumericTraits<int32> { static const int32 maxValue = int32_max; static const int32 minValue = int32_min; };
    template <> struct NumericTraits<int64> { static const int64 maxValue = int64_max; static const int64 minValue = int64_min; };

    template <> struct NumericTraits<uint8> { static const uint8 maxValue = uint8_max; static const uint8 minValue = 0; };
    template <> struct NumericTraits<uint16> { static const uint16 maxValue = uint16_max; static const uint16 minValue = 0; };
    template <> struct NumericTraits<uint32> { static const uint32 maxValue = uint32_max; static const uint32 minValue = 0; };
    template <> struct NumericTraits<uint64> { static const uint64 maxValue = uint64_max; static const uint64 minValue = 0; };

#if !(DFG_IS_QT_AVAILABLE) // TODO: Add robust check. This is used because at least some versions of Qt seems to have ushort == wchar_t
    template <> struct NumericTraits<wchar_t> { static const wchar_t maxValue = WCHAR_MAX; static const wchar_t minValue = WCHAR_MIN; };
#endif

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
        typedef std::make_unsigned<From_T>::type UFrom_T;
        return isValWithinLimitsOfType<To_T>(static_cast<UFrom_T>(val));
    }

    template <class To_T, class From_T>
    bool isValWithinLimitsOfType(const From_T& val, const std::false_type, const std::true_type) // To_T signed, From_T unsigned
    {
        typedef std::make_unsigned<To_T>::type UTo_T;
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
}
