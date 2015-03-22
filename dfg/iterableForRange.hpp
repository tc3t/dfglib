#pragma once

#pragma once

#include "dfgDefs.hpp"

DFG_ROOT_NS_BEGIN
{
	// Defines 'array of read only values'-like object that can be used in place of array of values when
	// the values can be expressed in a simple for-loop like manner, i=first; i<end; i += step.
	// TODO: test
	template <class T>
	class DFG_CLASS_NAME(IterableForRangeSimple)
	{
	public:
		class IterableImpl
		{
		public:
			IterableImpl(T value, T diff) :
				m_val(value),
				m_diff(diff)
			{

			}

			T operator*() const {return m_val;}
			void operator++() {m_val += m_diff;}
			IterableImpl operator++(int)
			{
				auto iter = *this;
				++*this;
				return iter;
			}

			bool operator!=(const IterableImpl& other) const {return (m_val < other.m_val);}

			T m_val;
			T m_diff;
		};

		DFG_CLASS_NAME(IterableForRangeSimple)(T first, T end, T step) :
			m_first(first),
			m_end(end),
			m_step(step)
		{
		}

		IterableImpl begin() const {return IterableImpl(m_first, m_step);}
		IterableImpl cbegin() const {return begin();}
		IterableImpl end() const {return IterableImpl(m_end, T(0));}
		IterableImpl cend() const {return end();}

		T m_first;
		T m_end;
		T m_step;
	}; // class DFG_CLASS_NAME(IterableForRangeSimple)

	
} // namespace
