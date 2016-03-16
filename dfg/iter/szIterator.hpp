#pragma once

#include "../dfgDefs.hpp"
#include "iteratorTemplate.hpp"
#include "../Rangeiterator.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(iter) {

	// Iterator for null terminated strings.
	template <class T>
	//class DFG_CLASS_NAME(SzIterator) : public DFG_CLASS_NAME(IteratorTemplate)<typename std::remove_const<T>::type, std::forward_iterator_tag>
    class DFG_CLASS_NAME(SzIterator) : public DFG_CLASS_NAME(IteratorTemplate)<T, std::forward_iterator_tag>
	{
	public:
		typedef T* PointerType;
		typedef const T* ConstPointerType;

		DFG_CLASS_NAME(SzIterator)() : m_psz(nullptr)
		{}
		DFG_CLASS_NAME(SzIterator)(T* psz) : m_psz(psz)
		{}

		bool operator==(const DFG_CLASS_NAME(SzIterator)& other) const
		{
			if (m_psz == nullptr)
				return other.m_psz == nullptr;
			else if (other.m_psz == nullptr)
				return *m_psz == '\0';
			else
				return m_psz == other.m_psz;
		}

		bool operator!=(const DFG_CLASS_NAME(SzIterator)& other) const
		{
			return !(*this == other);
		}

		bool operator<(const DFG_CLASS_NAME(SzIterator)& other) const
		{
			return (m_psz != nullptr && other.m_psz == nullptr);
		}

		DFG_CLASS_NAME(SzIterator)& operator++() { ++m_psz; return *this; }
		DFG_CLASS_NAME(SzIterator) operator++(int)
		{ 
			DFG_CLASS_NAME(SzIterator) copyIter(*this);
			++copyIter;
			return copyIter;
		}

		T& operator*() { return *m_psz; }
		const T& operator*() const { return *m_psz; }

		T* m_psz;
	};

} } // module iter

DFG_ROOT_NS_BEGIN
{
    // Specialize RangeIterator_T for SzIterator.
    template <class T>
    class DFG_CLASS_NAME(RangeIterator_T)<DFG_SUB_NS_NAME(iter)::DFG_CLASS_NAME(SzIterator)<T>> : public ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorContiguousIterBase<DFG_SUB_NS_NAME(iter)::DFG_CLASS_NAME(SzIterator)<T>>
    {
    public:
        typedef DFG_SUB_NS_NAME(iter)::DFG_CLASS_NAME(SzIterator)<T> IterT;
        typedef typename ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorContiguousIterBase<IterT> BaseClass;
        DFG_CLASS_NAME(RangeIterator_T)() {}
        DFG_CLASS_NAME(RangeIterator_T)(IterT iBegin, IterT iEnd) : BaseClass(iBegin, iEnd) {}
    };

	template <class T>
	inline bool isAtEnd(const DFG_SUB_NS_NAME(iter)::DFG_CLASS_NAME(SzIterator)<T>& iter, const DFG_SUB_NS_NAME(iter)::DFG_CLASS_NAME(SzIterator)<T>&)
	{
		return *iter == '\0';
	}

	template <class Char_T>
	inline auto makeSzIterator(Char_T* psz)->DFG_SUB_NS_NAME(iter)::DFG_CLASS_NAME(SzIterator)<Char_T>
	{
		return DFG_SUB_NS_NAME(iter)::DFG_CLASS_NAME(SzIterator)<Char_T>(psz);
	}

	template <class Char_T>
	inline auto makeSzRange(Char_T* psz) -> decltype(makeRange(makeSzIterator(psz), makeSzIterator((Char_T*)(nullptr))))
	{
		return makeRange(makeSzIterator(psz), makeSzIterator((Char_T*)(nullptr)));
	}

}
