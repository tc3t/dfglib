#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(math) {

// Returns value of x within [x1, x2] using linear interpolation. If x is outside [x1, x2], behaviour is undefined.
// TODO: test
template <class X_T, class Y_T>
Y_T interpolationLinear(const X_T x, const X_T& x1, const Y_T& y1, const X_T& x2, const Y_T& y2)
{
	return y1 + (x-x1)*(y2-y1)/(x2-x1);
}

}} // module namespace
