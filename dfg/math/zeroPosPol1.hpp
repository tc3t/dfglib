#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(math) {

// Given to a line defined by (x1, y1), (x2, y2), returns the zero position of the line.
// TODO: test
template <class X_T, class Y_T>
X_T zeroPosPol1(const X_T& x1, const Y_T& y1, const X_T& x2, const Y_T& y2)
{
	if (y1 == y2)
		return std::numeric_limits<X_T>::quiet_NaN();
	else
	{
		// f(x) = dY/dX * x + C, f(x1) = dY/dX * x1 + C = y1 <=> C = y1 - dY/dX*x1
		// f(x) = 0 <=> x = -C * dX/dY = (-y1 + dY/dX*x1) * dX/dY
		const auto k = (y2 - y1) / (x2 - x1);
		const auto C = y1 - k * x1;
		return static_cast<X_T>(-C / k);
	}
}

}} // module math
