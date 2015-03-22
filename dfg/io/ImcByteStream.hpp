#pragma once

#include "../dfgDefs.hpp"
#include <strstream>


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {


	class DFG_CLASS_NAME(ImcByteStream) : public std::istrstream
	{
	public:
		typedef std::istrstream BaseClass;

		DFG_CLASS_NAME(ImcByteStream)() :
			BaseClass((const char*)nullptr, 0)
		{}

		DFG_CLASS_NAME(ImcByteStream)(const char* psz, const size_t nSize) : 
			BaseClass(psz, nSize)
		{}

	}; // Class ImcByteStream


} } // module ns
