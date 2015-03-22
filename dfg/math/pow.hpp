#pragma once

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(math) {

inline size_t pow2ToX(const size_t& n) {return (size_t(1) << n);}
template<unsigned int p> struct pow2ToXCt {static const auto value = (1u << p);}; // Ct = Compile time

template <class T> inline T pow2(const T& x) {return x*x;}
template <class T> inline T pow3(const T& x) {return x*x*x;}
template <class T> inline T pow4(const T& x) {const T x2 = x*x; return x2*x2;}
template <class T> inline T pow5(const T& x) {const T x2 = x*x; return x2*x2*x;}
template <class T> inline T pow6(const T& x) {const T x2 = x*x; return x2*x2*x2;}
template <class T> inline T pow7(const T& x) {const T x3 = x*x*x; return x3*x3*x;}
template <class T> inline T pow8(const T& x) {T xTemp = x*x; xTemp *= xTemp; xTemp *= xTemp; return xTemp;}
template <class T> inline T pow9(const T& x) {const T x3 = x*x*x; return x3*x3*x3;}

// Returns x^n, where n is non-negative integer. If both are zero, return value is unspecified.
template <class T>
T powN(T x, const size_t n)
{
	if (n == 0 || x == 1)
		return T(1);
	auto a = x;
	for(size_t i = 1; i<n; i++) {a *= x;}
	return a;
}

}} // module math
