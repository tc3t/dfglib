#pragma once

#include "dfgBase.hpp"
#include "math/interpolationLinear.hpp"
#include "math/constants.hpp"
#include "math/pdf.hpp"
#include <cmath>
#include "build/languagefeatureinfo.hpp"

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

template <class T>
inline bool isNan(const T t)
{
#if DFG_LANGFEAT_HAS_ISNAN
	return (std::isnan(t) != 0);
#elif defined(_MSC_VER)
	return (_isnan(t) != 0);
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

}} // module math
