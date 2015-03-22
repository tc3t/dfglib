#pragma once

#include "../dfgDefs.hpp"
#include "../ptrToContiguousMemory.hpp"
#include "../dfgBase.hpp"
#include "accumulate.hpp"
#include "average.hpp"


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(numeric) {

	// Using macro since template implementation even with very simple lambdas seemed to easily fail
	// to vectorize both in VC2012 and VC2013.
	// Note: While this macro is not undef'ed at the end of file, this is internal implementation detail.
#define DFG_ZIMPL_VECTORIZING_LOOP(ARR, UPPER, RHS) \
	for(size_t i = 0; i < UPPER; ++i) \
		ARR[i] RHS;

	template <class MutableSource_T, class Source1_T, class Func>
	void transformInPlace(MutableSource_T* const mutableSource, const size_t nSize0, const Source1_T* const source1, const size_t nSize1, Func func)
	{
		using namespace DFG_ROOT_NS;
		const auto nSize = Min(nSize0, nSize1);
		for (size_t i = 0; i < nSize; ++i)
			mutableSource[i] = func(mutableSource[i], source1[i]);
	}

	template <class MutableSource_T, class Iterable1_T, class Func>
	void transformInPlace(MutableSource_T& mutableSource, const Iterable1_T& source1, Func func)
	{
		using namespace DFG_ROOT_NS;
		transformInPlace(ptrToContiguousMemory(mutableSource), count(mutableSource), ptrToContiguousMemory(source1), count(source1), func);
	}

	template <class Source0_T, class Source1_T, class Dest_T, class Func>
	void transform2(const Source0_T* const source0, const size_t nSize0, const Source1_T* const source1, const size_t nSize1, Dest_T* dest, Func func)
	{
		using namespace DFG_ROOT_NS;
		const auto nSize = Min(nSize0, nSize1);
		for (size_t i = 0; i < nSize; ++i)
			dest[i] = func(source0[i], source1[i]);
	}

	template <class Iterable0_T, class Iterable1_T, class Dest_T, class Func>
	void transform2(const Iterable0_T& source0, const Iterable1_T& source1, Dest_T* dest, Func func)
	{
		using namespace DFG_ROOT_NS;
		transform2(ptrToContiguousMemory(source0), count(source0), ptrToContiguousMemory(source1), count(source1), dest, func);
	}

	template <class Source0_T, class Dest_T, class Func>
	void transform1(const Source0_T* const source, const size_t nSize, Dest_T* dest, Func func)
	{
		using namespace DFG_ROOT_NS;
		for (size_t i = 0; i < nSize; ++i)
			dest[i] = func(source[i]);
	}

	template <class Iterable0_T, class Dest_T, class Func>
	void transform1(const Iterable0_T& source0, Dest_T* dest, Func func)
	{
		using namespace DFG_ROOT_NS;
		transform1(ptrToContiguousMemory(source0), count(source0), dest, func);
	}

	template <class Arr_T, class T>
	void forEachAdd(Arr_T* const arr, const size_t nSize, const T addValue)
	{
		if (addValue == 0)
			return;
		DFG_ZIMPL_VECTORIZING_LOOP(arr, nSize, += addValue);
	}

	template <class Cont_T, class T>
	void forEachAdd(Cont_T& cont, const T addValue)
	{
		forEachAdd(ptrToContiguousMemory(cont), count(cont), addValue);
	}

	template <class Arr_T, class T>
	void forEachMultiply(Arr_T* const arr, const size_t nSize, const T mul)
	{
		if (mul == 1)
			return;
		DFG_ZIMPL_VECTORIZING_LOOP(arr, nSize, *= mul);
	}

	template <class Cont_T, class T>
	void forEachMultiply(Cont_T& cont, const T mul)
	{
		forEachMultiply(ptrToContiguousMemory(cont), count(cont), mul);
	}

	// Computes dest[i] = source0[i] + source1[i].
	// If sources are not equal sized, maximum index will be min(nSize0, nSize1)
	template <class Source0_T, class Source1_T, class Dest_T>
	void transformAdd(const Source0_T* const source0, const size_t nSize0, const Source1_T* const source1, const size_t nSize1, Dest_T* dest)
	{
		using namespace DFG_ROOT_NS;
		const auto nSize = Min(nSize0, nSize1);
		DFG_ZIMPL_VECTORIZING_LOOP(dest, nSize, = source0[i] + source1[i]);
	}

	template <class Iterable0_T, class Iterable1_T, class Dest_T>
	void transformAdd(const Iterable0_T& source0, const Iterable1_T& source1, Dest_T* dest)
	{
		using namespace DFG_ROOT_NS;
		transformAdd(ptrToContiguousMemory(source0), count(source0), ptrToContiguousMemory(source1), count(source1), dest);
	}

	// Computes dest[i] = source0[i] * source1[i].
	// If sources are not equal sized, maximum index will be min(nSize0, nSize1)
	template <class Source0_T, class Source1_T, class Dest_T>
	void transformMultiply(const Source0_T* const source0, const size_t nSize0, const Source1_T* const source1, const size_t nSize1, Dest_T* dest)
	{
		using namespace DFG_ROOT_NS;
		const auto nSize = Min(nSize0, nSize1);
		DFG_ZIMPL_VECTORIZING_LOOP(dest, nSize, = source0[i] * source1[i]);
	}

	template <class Iterable0_T, class Iterable1_T, class Dest_T>
	void transformMultiply(const Iterable0_T& source0, const Iterable1_T& source1, Dest_T* dest)
	{
		using namespace DFG_ROOT_NS;
		transformMultiply(ptrToContiguousMemory(source0), count(source0), ptrToContiguousMemory(source1), count(source1), dest);
	}

	namespace DFG_DETAIL_NS
	{
		template <class Iterable_T, class Value_T, class New_T, class Scale_T>
		void rescaleImpl(Iterable_T& iterable, New_T minEnd, Value_T oldMin, Scale_T scaleFactor, std::false_type)
		{
			std::transform(std::begin(iterable), std::end(iterable), std::begin(iterable), [=](const Value_T oldVal)
			{
				return (minEnd + (oldVal - oldMin) * scaleFactor);
			});
		}

		template <class Iterable_T, class Value_T, class New_T, class Scale_T>
		void rescaleImpl(Iterable_T& iterable, New_T minEnd, Value_T oldMin, Scale_T scaleFactor, std::true_type)
		{
			auto p = ptrToContiguousMemory(iterable);
			const auto nSize = count(iterable);
			DFG_ZIMPL_VECTORIZING_LOOP(p, nSize, = minEnd + (p[i] - oldMin) * scaleFactor);
		}
	}

	// For given set of values in range [min_element, max_element], map them so that
	// minimum values map to newLimit0, maximum values map to newLimit1 and points in the middle linearly.
	// If either range has length of zero, new values will be newLimit0.
	// Behaviour is undefined if source or destination range limits contain NaN values.
	template <class Iterable_T, class T>
	void rescale(Iterable_T& iterable, const T& newLimit0, const T& newLimit1)
	{
		const auto iterBegin = std::begin(iterable);
		const auto iterEnd = std::end(iterable);
		auto minMaxPair = std::minmax_element(iterBegin, iterEnd);
		if (minMaxPair.first == iterEnd || minMaxPair.second == iterEnd)
			return;
		const auto oldMin = *minMaxPair.first;
		typedef typename std::remove_const<decltype(oldMin)>::type ValueType;
		DFG_STATIC_ASSERT(std::is_floating_point<ValueType>::value, "Only floating point types are supported in " DFG_CURRENT_FUNCTION_NAME);
		const auto oldMax = *minMaxPair.second;
		const auto oldRange = oldMax - oldMin;
		const auto minEnd = newLimit0;
		const auto maxEnd = newLimit1;
		const auto newRange = maxEnd - minEnd;
		const auto scaleFactor = (oldRange > 0) ? newRange / oldRange : 0;

		if (scaleFactor != 0)
		{
			DFG_DETAIL_NS::rescaleImpl(iterable,
										ValueType(minEnd),
										oldMin,
										scaleFactor,
										std::integral_constant<bool, std::is_pointer<typename RangeToBeginPtrOrIter<Iterable_T>::type>::value>()
										);
		}
		else
			std::fill(iterBegin, iterEnd, ValueType(minEnd));
	}

//#undef DFG_ZIMPL_VECTORIZING_LOOP
} }
