#pragma once

#include "../dfgDefs.hpp"
#include "StreamBufferMem.hpp"


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {


	class ImcByteStream : public std::istream
	{
	public:
		typedef std::istream BaseClass;

		ImcByteStream(const char* pData = nullptr, const size_t nSize = 0) :
			BaseClass(nullptr),
			m_streamBuf(pData, nSize)
		{
			set_rdbuf(&m_streamBuf);
			this->clear();
		}

		StreamBufferMem<char> m_streamBuf;
	}; // Class ImcByteStream

} } // module namespace
