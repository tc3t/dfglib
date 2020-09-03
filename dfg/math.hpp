#pragma once

#include "dfgBase.hpp"
#include "math/interpolationLinear.hpp"
#include "math/constants.hpp"
#include "math/pdf.hpp"
#include <cmath>
#include <limits>
#include "build/languageFeatureInfo.hpp"

#if !DFG_LANGFEAT_HAS_ISNAN && _MSC_VER
	#include <float.h>
#endif

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(math) {

// From http://isocpp.org/wiki/faq/intrinsic-types#is-power-of-2
// TODO: test
template <class T>
inline bool isPowerOf2(T i)
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
    inline bool IsNanMsvc(float t)             { return _isnan(t) != 0; }
    inline bool IsNanMsvc(double t)            { return _isnan(t) != 0; }
    inline bool IsNanMsvc(long double t)       { return IsNanMsvc(static_cast<double>(t)); } // _isnan() does not have long double overload causing 'conversion from 'const long double' to 'double', possible loss of data' warnings.
#endif // _MSC_VER

    template <class Int_T>
    inline auto absAsUnsigned(const Int_T val, std::true_type) -> typename std::make_unsigned<Int_T>::type
    {
        DFG_STATIC_ASSERT(std::is_unsigned<Int_T>::value, "absAsUnsigned: usage error - unsigned version called for signed ");
        return val;
    }

    template <class Int_T>
    inline auto absAsUnsigned(const Int_T val, std::false_type) -> typename std::make_unsigned<Int_T>::type
    {
        typedef typename std::make_unsigned<Int_T>::type UnsignedType;
        if (val >= 0)
            return static_cast<UnsignedType>(val);
        else
        {
            if (val != NumericTraits<Int_T>::minValue)
                return static_cast<UnsignedType>(-1 * val);
            else
                return static_cast<UnsignedType>(-1 * (val + 1)) + 1;
        }
    }

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

// Returns first + (first + step) + (first + 2*step)... until first + n*step <= last
// TODO: Test
// TODO: Create a compile time version.
template <class T>
T arithmeticSumFirstLastStep(const T& first, const T& last, const T& step = 1)
{
	DFG_STATIC_ASSERT(std::numeric_limits<T>::is_integer == true, "Data type must be integer");
	if (first > last)
		return 0;
	const T stepCount = 1 + (last - first) / step;
	//sum(a + (a+k) + (a+2k) + ... + (a+N*k)) = 
	return first*stepCount + (step * (stepCount-1) * (stepCount) / 2);
}

// See boost for more robust factorial implementation.
inline size_t factorialInt(const size_t n) { return (n > 1) ? n * factorialInt(n - 1) : 1; }
template <uint64 N> struct DFG_CLASS_NAME(Factorial_T) { static const uint64 value = N * DFG_CLASS_NAME(Factorial_T)<N - 1>::value; };
template <> struct DFG_CLASS_NAME(Factorial_T)<0> {static const uint64 value = 1; };


// For given integer value, returns it's absolute value as unsigned type. The essential differences to std::abs() are:
//      -Return value is unsigned type
//      -Return value is mathematically correct even when called with value std::numeric_limits<Int_T>::min();
template <class Int_T>
auto absAsUnsigned(const Int_T val) -> typename std::make_unsigned<Int_T>::type
{
    return DFG_DETAIL_NS::absAsUnsigned(val, typename std::is_unsigned<Int_T>::type());
}

namespace DFG_DETAIL_NS
{
    template <class T>
    inline T logOfBaseImpl(const T x, const T base)
    {
        if (base <= 0)
            return std::numeric_limits<T>::quiet_NaN();
        return log(x) / log(base);
    }

} // namespace DFG_DETAIL_NS 

// Returns log x in base 'base'.
template <class Value_T, class Base_T>
auto logOfBase(const Value_T x, const Base_T base) -> typename std::conditional<std::numeric_limits<Value_T>::is_integer, double, Value_T>::type
{
    typedef typename std::conditional<std::numeric_limits<Value_T>::is_integer, double, Value_T>::type ReturnValueType;
    typedef typename std::conditional<std::numeric_limits<Value_T>::is_integer, double, Value_T>::type ValueType;
    typedef typename std::conditional<std::numeric_limits<Base_T>::is_integer, double, Base_T>::type BaseType;
    typedef typename std::common_type<ValueType, BaseType>::type CommonType; // If compiler error points here stating that type is not member of std::common_type, reason can be that Value_T and Base_T are not compatible, e.g. logOfBase(2, "");
    return static_cast<ReturnValueType>(DFG_DETAIL_NS::logOfBaseImpl(static_cast<CommonType>(x), static_cast<CommonType>(base)));
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

} // unnamed namespace

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

// Returns true if floating point value can be represent exactly as given integer type. If pInt is given and and return value is true, it receives the integer value.
template <class Int_T, class Float_T>
bool isFloatConvertibleTo(const Float_T f, Int_T* pInt = nullptr)
{
    DFG_STATIC_ASSERT(std::is_integral<Int_T>::value, "Int_T should be integer type");
    DFG_STATIC_ASSERT(std::numeric_limits<Float_T>::is_iec559, "isFloatConvertibleTo() probably requires IEC 559 (IEEE 754) floating points");
    if (!isIntegerValued(f))
        return false;
    return DFG_DETAIL_NS::isIntegerFloatConvertibleTo(f, pInt, std::integral_constant<bool, std::numeric_limits<Float_T>::digits >= std::numeric_limits<Int_T>::digits>());
}

namespace DFG_DETAIL_NS
{
    template <class T>
    typename std::make_unsigned<T>::type numericDistanceImpl(const T lesser, const T greater, std::true_type) // case: T is signed
    {
        using UnsignedT = typename std::make_unsigned<T>::type;
        if (greater < 0 || lesser >= 0)
        {
            // Both negative or both non-negative, can't overflow, even if lesser is NumericTraits<T>::minValue (according to answers in below link)
            // https://stackoverflow.com/questions/19015444/is-int-min-subtracted-from-any-integer-considered-undefined-behavior
            return UnsignedT(greater - lesser);
        }
        else
            return UnsignedT(greater) + ::DFG_MODULE_NS(math)::absAsUnsigned(lesser); // Different signs, adding as unsigned.
    }

    template <class T>
    typename std::make_unsigned<T>::type numericDistanceImpl(const T lesser, const T greater, std::false_type) // case: T is unsigned
    {
        return greater - lesser; // T is unsigned.
    }
}

// Returns distance of two integer values as unsigned guaranteed to be done in overflow (undefined behaviour) safe manner.
// Rationale: given two signed values lesser and greater, computing their distance can't generally be done greater - lesser, because that may overflow.
template <class T>
typename std::make_unsigned<T>::type numericDistance(const T a, const T b)
{
    DFG_STATIC_ASSERT(std::is_integral<T>::value, "numericDistance() expects integer type"); // This doesn't seems to get shown for floats since std::make_unsigned<T>::type already fails.
    const auto lesser = Min(a, b);
    const auto greater = Max(a, b);
    return DFG_DETAIL_NS::numericDistanceImpl(lesser, greater, std::is_signed<T>());
}

}} // module math
