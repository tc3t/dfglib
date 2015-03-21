#pragma once

#include "dfgDefs.hpp"
#include <iterator> // For std::distance
#include "iter/IsContiguousMemoryIterator.hpp"

DFG_ROOT_NS_BEGIN
{
	namespace DFG_DETAIL_NS
	{
		template <class Iter_T>
		class RangeIteratorDefaultBase
		{
		public:
			typedef Iter_T iterator;
			typedef typename std::iterator_traits<Iter_T>::value_type value_type;

			RangeIteratorDefaultBase() : m_iterBegin(Iter_T()), m_iterEnd(Iter_T()) {}
			RangeIteratorDefaultBase(Iter_T iBegin, Iter_T iEnd) : m_iterBegin(iBegin), m_iterEnd(iEnd) {}

			iterator begin()  const	{ return m_iterBegin; }
			iterator cbegin() const	{ return begin(); }
			iterator end()  const	{ return m_iterEnd; }
			iterator cend() const	{ return end(); }
			size_t size() const		{ return std::distance(begin(), end()); }

			iterator m_iterBegin;
			iterator m_iterEnd;
		};

		// Specialized base that defines data()-method for access to contiguous memory.
		template <class Iter_T>
		class RangeIteratorContiguousIterBase : public RangeIteratorDefaultBase<Iter_T>
		{
		public:
			typedef RangeIteratorDefaultBase<Iter_T> BaseClass;
			RangeIteratorContiguousIterBase() {}
			RangeIteratorContiguousIterBase(Iter_T iBegin, Iter_T iEnd) : BaseClass(iBegin, iEnd) {}

			// Note: This is const given the const-semantics explained in comments for RangeIterator_T.
			Iter_T data() const { return this->m_iterBegin; }
		};


	}

	// Defines range through iterator pair.
	// This is iterator-like structure which does not own the data it refers to.
	// To create instances conveniently, use makeRange.
	// Note: Range uses 'pointer const' semantics: range's constness is defined by the range it
	//       defines, not by the contents. Thus is it possible to modify underlying data through
	//       e.g. begin() iterator even through const range.
	// TODO: test
	// TODO: Currently data()-method is defined only for pointer type. Should be defined for all iterators that point
	//       to contiguous memory (e.g. std::vector, std::string, std::array).
	// Remarks: For more robust implementation, see Boost.Range
	template <class Iter_T>
	class DFG_CLASS_NAME(RangeIterator_T) : public std::conditional<IsContiguousMemoryIterator<Iter_T>::value, DFG_DETAIL_NS::RangeIteratorContiguousIterBase<Iter_T>, DFG_DETAIL_NS::RangeIteratorDefaultBase<Iter_T>>::type
	{
	public:
		typedef typename std::conditional<IsContiguousMemoryIterator<Iter_T>::value, DFG_DETAIL_NS::RangeIteratorContiguousIterBase<Iter_T>, DFG_DETAIL_NS::RangeIteratorDefaultBase<Iter_T>>::type BaseClass;
		DFG_CLASS_NAME(RangeIterator_T)() {}
		DFG_CLASS_NAME(RangeIterator_T)(Iter_T iBegin, Iter_T iEnd) : BaseClass(iBegin, iEnd) {}
	};

	template <class Iter_T>
	DFG_CLASS_NAME(RangeIterator_T)<Iter_T> makeRange(Iter_T iBegin, Iter_T iEnd)
	{
		return DFG_CLASS_NAME(RangeIterator_T)<Iter_T>(iBegin, iEnd);
	}

	template <class T, size_t N>
	DFG_CLASS_NAME(RangeIterator_T)<T*> makeRange(T(&arr)[N])
	{
		return makeRange(std::begin(arr), std::end(arr));
	}

	template <class BeginEndInterface_T>
	auto makeRange(BeginEndInterface_T& bei) -> DFG_CLASS_NAME(RangeIterator_T)<decltype(std::begin(bei))>
	{
		return makeRange(std::begin(bei), std::end(bei));
	}
}
