#pragma once

#include <boost/math/special_functions/sign.hpp>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(math) {

// Returns sign of a value: 1 if value is positive, 0 if value is 0, -1 if value is < 0.
// Note: Behaviour for NaN's is unspecified.
// See also: std::signbit, std::copysign (C++11)
template <class T>
int sign(const T& val)
{
	return boost::math::sign(val);
	/*
	if (val > 0)
		return 1;
	else if (val == 0)
		return 0;
	else
		return -1;
		*/
}

template <class T>
auto signBit(const T& val) -> decltype(boost::math::signbit(val))
{
	return boost::math::signbit(val);
}

// Returns value whose absolute value is from val0 and sign from val1.
template <class T0, class T1>
auto signCopied(const T0& val0, const T1& val1) -> decltype(boost::math::copysign(val0, val1))
{
	return boost::math::copysign(val0, val1);
}

// Returns val with sign changed.
template <class T>
auto signChanged(const T& val) -> decltype(boost::math::changesign(val))
{
	return boost::math::changesign(val);
}

}} // module namespace
