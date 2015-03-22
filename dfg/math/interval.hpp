#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(math) {

// Defines a (numerical) interval [(min, max)].
// TODO: test
// NOTE: Taking a look at boost::ICL is recommended before intending to use this class.
template <class T> class Interval_T
{
public:
	typedef T Type;

	Interval_T(const T& a, const T& b) : m_lower(a), m_upper(b) {}

	T length() const {return m_upper - m_lower;}

	bool operator==(const Interval_T& other) const {return (this->lower == other.lower && this->upper == other.upper);}
	bool operator!=(const Interval_T& other) const {return !((*this) == other);}
	bool isInRangeII(const T& val) const
	{
		return (val >= m_lower && val <= m_upper);
	}
	// Shifts both ends by given value.
	void shift(const T& shift) {m_lower += shift; m_upper += shift;}

	T m_lower;
	T m_upper;
};

typedef Interval_T<long double>	IntervalRealLd;
typedef Interval_T<double>		IntervalRealD;
typedef Interval_T<float>		IntervalRealF;

}} // module math
