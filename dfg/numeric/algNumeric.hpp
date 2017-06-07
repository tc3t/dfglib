#pragma once

#include "../dfgDefs.hpp"
#include "../ptrToContiguousMemory.hpp"
#include "../dfgBase.hpp"
#include "accumulate.hpp"
#include "average.hpp"
#include "vectorizingLoop.hpp"
#include "rescale.hpp"


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(numeric) {

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
        DFG_ZIMPL_VECTORIZING_LOOP_RHS(arr, nSize, += addValue);
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
        DFG_ZIMPL_VECTORIZING_LOOP_RHS(arr, nSize, *= mul);
	}

	template <class Cont_T, class T>
	void forEachMultiply(Cont_T& cont, const T mul)
	{
		forEachMultiply(ptrToContiguousMemory(cont), count(cont), mul);
	}

    template <class Arr_T, class T>
    void forEachSubtract(Arr_T* const arr, const size_t nSize, const T val)
    {
        if (val == 0)
            return;
        DFG_ZIMPL_VECTORIZING_LOOP_RHS(arr, nSize, -= val);
    }

    template <class Cont_T, class T>
    void forEachSubtract(Cont_T& cont, const T val)
    {
        forEachSubtract(ptrToContiguousMemory(cont), count(cont), val);
    }

    template <class Arr_T, class T>
    void forEachDivide(Arr_T* const arr, const size_t nSize, const T val)
    {
        if (val == 1)
            return;
        DFG_ZIMPL_VECTORIZING_LOOP_RHS(arr, nSize, /= val);
    }

    template <class Cont_T, class T>
    void forEachDivide(Cont_T& cont, const T val)
    {
        forEachDivide(ptrToContiguousMemory(cont), count(cont), val);
    }

	// Computes dest[i] = source0[i] + source1[i].
	// If sources are not equal sized, maximum index will be min(nSize0, nSize1)
	template <class Source0_T, class Source1_T, class Dest_T>
	void transformAdd(const Source0_T* const source0, const size_t nSize0, const Source1_T* const source1, const size_t nSize1, Dest_T* dest)
	{
		using namespace DFG_ROOT_NS;
		const auto nSize = Min(nSize0, nSize1);
        DFG_ZIMPL_VECTORIZING_LOOP_RHS(dest, nSize, = source0[i] + source1[i]);
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
        DFG_ZIMPL_VECTORIZING_LOOP_RHS(dest, nSize, = source0[i] * source1[i]);
	}

	template <class Iterable0_T, class Iterable1_T, class Dest_T>
	void transformMultiply(const Iterable0_T& source0, const Iterable1_T& source1, Dest_T* dest)
	{
		using namespace DFG_ROOT_NS;
		transformMultiply(ptrToContiguousMemory(source0), count(source0), ptrToContiguousMemory(source1), count(source1), dest);
	}

} }
