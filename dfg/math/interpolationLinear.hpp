#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(math) {

// Returns value of x within [x0, x1] using linear interpolation. If x is outside of [x0, x1], behaviour is undefined.
// If x0==x1==x, returns (y0+y1)/2.
template <class X_T, class Y_T>
Y_T interpolationLinear_X_X0Y0_X1Y1(const X_T x, const X_T& x0, const Y_T& y0, const X_T& x1, const Y_T& y1)
{
	return (x0 != x1) ? y0 + (x-x0)*(y1-y0)/(x1-x0) : (y0 + y1)/2;
}

}} // module namespace
