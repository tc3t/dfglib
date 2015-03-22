#ifndef DFG_CONT_WJEQXMXY
#define DFG_CONT_WJEQXMXY

#include "dfgBase.hpp"
#include "math/interpolationLinear.hpp"
#include <limits>
#include <vector>
#include <array>
#include "cont/elementType.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(cont) {

template <class ContT> void pushBack(ContT& cont, typename DFG_CLASS_NAME(ElementType)<ContT>::type&& a0)
{
	cont.reserve(cont.size() + 1);
	cont.push_back(std::forward<typename DFG_CLASS_NAME(ElementType)<ContT>::type>(a0));
}

template <class ContT> void pushBack(ContT& cont, typename DFG_CLASS_NAME(ElementType)<ContT>::type&& a0,
												typename DFG_CLASS_NAME(ElementType)<ContT>::type&& a1)
{
	typedef typename DFG_CLASS_NAME(ElementType)<ContT>::type T;
	cont.reserve(cont.size() + 2);
	cont.push_back(std::forward<T>(a0));
	cont.push_back(std::forward<T>(a1));
}

template <class T> std::vector<T> makeVector(T a0)
{
	std::vector<T> vec;
	pushBack(vec, std::move(a0));
	return vec;
}

template <class T> std::vector<T> makeVector(T a0, T a1)
{
	std::vector<T> vec;
	pushBack(vec, std::move(a0), std::move(a1));
	return vec;
}

// TODO: test
template <class T> std::array<T, 1> makeArray(T&& a0) { std::array<T, 1> a = { std::move(a0) }; return a; }
template <class T> std::array<T, 2> makeArray(T&& a0, T&& a1) { std::array<T, 2> a = { std::move(a0), std::move(a1) }; return a; }
template <class T> std::array<T, 3> makeArray(T&& a0, T&& a1, T&& a2) { std::array<T, 3> a = { std::move(a0), std::move(a1), std::move(a2) }; return a; }
template <class T> std::array<T, 4> makeArray(T&& a0, T&& a1, T&& a2, T&& a3) { std::array<T, 4> a = { std::move(a0), std::move(a1), std::move(a2), std::move(a3) }; return a; }
template <class T> std::array<T, 5> makeArray(T&& a0, T&& a1, T&& a2, T&& a3, T&& a4) { std::array<T, 5> a = { std::move(a0), std::move(a1), std::move(a2), std::move(a3), std::move(a4) }; return a; }
template <class T> std::array<T, 6> makeArray(T&& a0, T&& a1, T&& a2, T&& a3, T&& a4, T&& a5) { std::array<T, 6> a = { std::move(a0), std::move(a1), std::move(a2), std::move(a3), std::move(a4), std::move(a5) }; return a; }

// Interpolated access to indexed value list, i.e. instead of using integer-indexes to get values from a
// container, one can use value like 1.35 to get value between [1] and [2] using linear interpolation.
// Note: No extropolation is done: if index is not with [0, lastIndex], NaN will be returned.
// TODO: test
// TODO: extend to non-floating point types (e.g. values with units).
template <class Cont_T>
auto getInterpolatedValue(const Cont_T& cont, const double& index) -> typename std::remove_reference<decltype(*cont.begin())>::type
{
	typedef typename std::remove_reference<decltype(*cont.begin())>::type ValueType;
	static_assert(std::is_floating_point<ValueType>::value == true, "This function is currently implemented only for floating points.");
	if (cont.empty() || index < 0 || index > cont.size() - 1)
		return std::numeric_limits<ValueType>::quiet_NaN();
	const auto floorBin = round<size_t>(std::floor(index));
	const auto ceilBin = Min(floorBin + 1, cont.size() - 1);
	return DFG_SUB_NS_NAME(math)::interpolationLinear(index, ValueType(floorBin), ValueType(cont[floorBin]), ValueType(ceilBin), ValueType(cont[ceilBin]));
}


}} // module cont



#endif // Include guard
