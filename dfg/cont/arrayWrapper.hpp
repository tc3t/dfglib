#pragma once

#include "../dfgDefs.hpp"
#include "../dfgAssert.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(cont) {

// Non-owning wrapper for array of T.
// Uses 'pointer const' semantics: constness is defined by the "position" of the array it wraps,
//									not by the contents of the array. Thus is it possible to modify
//									underlying data through e.g. begin() iterator even through const wrapper.
// TODO: test
template <class T>
class DFG_CLASS_NAME(ArrayWrapperT)
{
public:
	typedef T* iterator;
	typedef const T* const_iterator;
	typedef T value_type;

	DFG_CLASS_NAME(ArrayWrapperT)(T* pData, const size_t nCount) : m_pData(pData), m_nCount(nCount) {}

	// Automatic conversion from 'non const T' to 'const T' version.
	operator DFG_CLASS_NAME(ArrayWrapperT)<const T>() const
	{
		return DFG_CLASS_NAME(ArrayWrapperT)<const T>(m_pData, m_nCount);
	}

	T& front() const {return *m_pData;}
	T& back() const {return *(m_pData + (m_nCount - 1));}
	T* begin() const {return m_pData;}
	T* cbegin() const {return m_pData;}
	T* end() const {return m_pData + m_nCount;}
	T* cend() const {return m_pData + m_nCount;}

	T* data() const { return m_pData; }

	bool empty() const {return size() == 0;}

	T& operator[](const size_t n) const
	{
		DFG_ASSERT_UB(n >= 0 && n < m_nCount);
		return m_pData[n];
	}

	size_t size() const
	{
		return m_nCount;
	}

	T* m_pData;
	size_t m_nCount;
};

class DFG_CLASS_NAME(ArrayWrapper)
{
public:
	template <class T>
	static DFG_CLASS_NAME(ArrayWrapperT)<T> createArrayWrapper(T* pData, const size_t nSize)
	{
		return DFG_CLASS_NAME(ArrayWrapperT)<T>(pData, nSize);
	}

	template <class T, size_t N>
	static DFG_CLASS_NAME(ArrayWrapperT)<T> createArrayWrapper(T (&arrData)[N])
	{
		return createArrayWrapper(arrData, N);
	}

	template <class T>
	static DFG_CLASS_NAME(ArrayWrapperT)<T> createArrayWrapper(std::vector<T>& vec)
	{
		return DFG_CLASS_NAME(ArrayWrapperT)<T>(vec.data(), vec.size());
	}

	template <class T>
	static DFG_CLASS_NAME(ArrayWrapperT)<const T> createArrayWrapper(const std::vector<T>& vec)
	{
		return DFG_CLASS_NAME(ArrayWrapperT)<const T>(vec.data(), vec.size());
	}
};

}} // module cont
