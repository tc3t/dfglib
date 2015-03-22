#pragma once

#include "../dfgDefs.hpp"
#include "accumulate.hpp"
#include <type_traits>
#include <iterator>
#include "../cont/elementType.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(numeric) {

	template <class Iterable_T, class Sum_T, class Div_T>
	auto averageTyped(const Iterable_T& iterable) -> decltype(Sum_T() / Div_T())
	{
		const auto itemCount = static_cast<Div_T>(::DFG_ROOT_NS::count(iterable));
		auto acc = accumulate(iterable, Sum_T(0));
		return acc / itemCount;
	}

	template <class Iterable_T>
	auto average(const Iterable_T& iterable) -> typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Iterable_T>::type
	{
		typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Iterable_T>::type ValueT;
		DFG_STATIC_ASSERT(std::is_integral<ValueT>::value == false, "Currently average for integers is not implemented; use AverageTyped instead.");
		return averageTyped<Iterable_T, ValueT, ValueT>(iterable);
	}

	inline double average(const double v0, const double v1) { return (v0 + v1) / 2.0; }
	inline double average(const double v0, const double v1, const double v2) { return (v0 + v1 + v2) / 3.0; }
	inline double average(const double v0, const double v1, const double v2, const double v3) { return (v0 + v1 + v2 + v3) / 4.0; }
	inline double average(const double v0, const double v1, const double v2, const double v3, const double v4) { return (v0 + v1 + v2 + v3 + v4) / 5.0; }

} } // namespace module math
