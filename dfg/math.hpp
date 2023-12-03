#pragma once

#include "dfgBase.hpp"
#include "math/interpolationLinear.hpp"
#include "math/constants.hpp"
#include "math/pdf.hpp"
#include <cmath>
#include <limits>
#include "build/languageFeatureInfo.hpp"
#include "numericTypeTools.hpp"

#if !DFG_LANGFEAT_HAS_ISNAN && _MSC_VER
	#include <float.h>
#endif

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(math) {


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
