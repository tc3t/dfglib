#pragma once

#include "../dfgDefs.hpp"
#include "iteratorTemplate.hpp"
#include <iterator>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(iter) {

// Semi-Iterator for simple read/write iterating through contiguous interleaved data,
// i.e. an array of format {x0, y0, z0, x1, y1, z1,..., xn, yn, zn}
// Different data arrays are referred to as channels: in above example there are three channels: x,y,z.
// Note: Currently only reading is implemented.
// TODO: test
template <class Data_T>
class DFG_CLASS_NAME(InterleavedSemiIterator) : public DFG_CLASS_NAME(IteratorTemplate)<Data_T, std::random_access_iterator_tag>
{
public:
	typedef typename DFG_CLASS_NAME(IteratorTemplate)<Data_T, std::random_access_iterator_tag>::difference_type difference_type;

	DFG_CLASS_NAME(InterleavedSemiIterator)() : m_pData(nullptr), m_nChannelCount(0) {}
	DFG_CLASS_NAME(InterleavedSemiIterator)(Data_T* pData, difference_type nChannelCount) : 
		m_pData(pData),
		m_nChannelCount(nChannelCount)
	{}

	template <size_t N>
	DFG_CLASS_NAME(InterleavedSemiIterator)(Data_T (&arrData)[N], difference_type nChannelCount) : 
		m_pData(arrData), m_nChannelCount(nChannelCount)
	{}

	Data_T getChannel(const size_t nChannel)
	{
		return *(m_pData + nChannel);
	}

	DFG_CLASS_NAME(InterleavedSemiIterator)& operator++()
	{
		m_pData += m_nChannelCount;
		return *this;
	}

	DFG_CLASS_NAME(InterleavedSemiIterator) operator++(int) const
	{
		auto iter = *this;
		++*this;
		return iter;
	}

	bool operator==(const DFG_CLASS_NAME(InterleavedSemiIterator)& other)
	{
		return (this->m_pData == other.m_pData);
	}

	bool operator!=(const DFG_CLASS_NAME(InterleavedSemiIterator)& other)
	{
		return !(*this == other);
	}

	DFG_CLASS_NAME(InterleavedSemiIterator) operator-(difference_type off) const
	{
		DFG_CLASS_NAME(InterleavedSemiIterator) temp = *this;
		return (temp += -off);
	}

	difference_type operator-(const DFG_CLASS_NAME(InterleavedSemiIterator)& other) const
	{	// return difference of iterators
		return (m_pData - other.m_pData)/m_nChannelCount;
	}

	DFG_CLASS_NAME(InterleavedSemiIterator) operator+(difference_type off) const
	{
		DFG_CLASS_NAME(InterleavedSemiIterator) temp = *this;
		return (temp += off);
	}

	DFG_CLASS_NAME(InterleavedSemiIterator)& operator+=(difference_type off)
	{
		m_pData += m_nChannelCount*off;
		return (*this);
	}

	Data_T* m_pData;
	difference_type m_nChannelCount;
}; // class DFG_CLASS_NAME(InterleavedSemiIterator)

}} // module iter
