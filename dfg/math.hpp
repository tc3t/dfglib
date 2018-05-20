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
} // unnamed namespace


template <class T>
inline bool isNan(const T t)
{
#if DFG_LANGFEAT_HAS_ISNAN
	return (std::isnan(t) != 0);
#elif defined(_MSC_VER)
	return DFG_DETAIL_NS::IsNanMsvc(t);
#else
	return (isnan(t) != 0);
#endif
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

}} // module math
