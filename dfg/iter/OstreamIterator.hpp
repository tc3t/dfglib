#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(iter) {

	// Similar to std::ostream_iterator. Some differences:
	//   -Works e.g. with QTextStream
	//   -Less template parameters
	//   -Templated assignment operator.
	// TODO: test
	// TODO: Define clearer specifications.
	// TODO: Make behaviour of operator= user definable.
	template <class Stream_T>
	class DFG_CLASS_NAME(OstreamIterator)
	{
	public:
        typedef std::output_iterator_tag    iterator_category;
        typedef void                        value_type;
        typedef void                        difference_type;
        typedef void                        pointer;
        typedef void                        reference;

		DFG_CLASS_NAME(OstreamIterator)(Stream_T& rStrm)
			: m_pStream(&rStrm)
		{
		}

		template <class Item_T>
		DFG_CLASS_NAME(OstreamIterator)& operator=(const Item_T& item)
		{
			*m_pStream << item;
			return *this;
		}

		DFG_CLASS_NAME(OstreamIterator)& operator*()
		{
			return *this;
		}

		DFG_CLASS_NAME(OstreamIterator)& operator++()
		{
			return *this;
		}

		DFG_CLASS_NAME(OstreamIterator) operator++(int)
		{
			return *this;
		}

		Stream_T* m_pStream; // Non-owned.
	};


	template <class Stream_T>
	DFG_CLASS_NAME(OstreamIterator)<Stream_T> makeOstreamIterator(Stream_T& strm)
	{
		return DFG_CLASS_NAME(OstreamIterator)<Stream_T>(strm);
	}

} } // module namespace
