#pragma once

#include "../dfgDefs.hpp"
#include "../ptrToContiguousMemory.hpp"
#include "../dfgBase.hpp" // For count. TODO: move count to it's own header.
#include <type_traits>
#include <limits>
#include <numeric>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(numeric) {

	namespace DFG_DETAIL_NS
	{
		template <class Iterable_T, class Sum_T>
		Sum_T accumulateImpl(const Iterable_T& iterable, Sum_T sum, std::true_type)
		{
			auto p = ptrToContiguousMemory(iterable);
			const auto nSize = count(iterable);
			for (size_t i = 0; i < nSize; ++i)
				sum += p[i];
			return sum;
		}

		template <class Iterable_T, class Sum_T>
		Sum_T accumulateImpl(const Iterable_T& iterable, Sum_T sum, std::false_type)
		{
			return std::accumulate(std::begin(iterable), std::end(iterable), sum);
		}
	}
	
	template <class Iterable_T, class Sum_T>
	Sum_T accumulate(const Iterable_T& iterable, Sum_T sum)
	{
		typedef typename DFG_ROOT_NS::RangeToBeginPtrOrIter<Iterable_T>::type IterType;
		return DFG_DETAIL_NS::accumulateImpl(iterable, sum, std::integral_constant<bool, std::is_pointer<IterType>::value>());
	}

    template <class Acc_T, class Iterable_T>
    Acc_T accumulateTyped(const Iterable_T& iterable)
    {
        DFG_STATIC_ASSERT(std::numeric_limits<Acc_T>::has_quiet_NaN, "accumulateTyped() is available only for types that have quiet NaN.");
        return accumulate(iterable, (!isEmpty(iterable)) ? Acc_T() : std::numeric_limits<Acc_T>::quiet_NaN());
    }

    template <class Iterable_T>
    auto accumulate(const Iterable_T& iterable) -> typename std::remove_reference<decltype(*std::begin(iterable))>::type
    {
        typedef typename std::remove_reference<decltype(*std::begin(iterable))>::type ReturnValueT;
        return accumulateTyped<ReturnValueT>(iterable);
    }



} }

