#pragma once

#include "../dfgDefs.hpp"
#include "StreamBufferMem.hpp"
#include "textEncodingTypes.hpp"
#include "../build/languageFeatureInfo.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

	// TODO: inherit from ImcByteStream
	class DFG_CLASS_NAME(ImStreamWithEncoding) : public std::istream
	{
	public:
		typedef std::istream BaseClass;
		typedef DFG_CLASS_NAME(ImStreamWithEncoding) ThisClass;

		DFG_CLASS_NAME(ImStreamWithEncoding)(const char* p, const size_t nSize, TextEncoding encoding) :
			BaseClass(&m_streamBuf),
			m_streamBuf(p, nSize, encoding)
		{
		}

		DFG_CLASS_NAME(ImStreamWithEncoding)(const wchar_t* p, const size_t nCharCount, TextEncoding encoding = hostNativeFixedSizeUtfEncodingFromCharType(sizeof(wchar_t))) :
			BaseClass(&m_streamBuf),
			m_streamBuf(reinterpret_cast<const char*>(p), sizeof(wchar_t) * nCharCount, encoding)
		{
		}

#if DFG_LANGFEAT_MOVABLE_STREAMS
		DFG_CLASS_NAME(ImStreamWithEncoding)(DFG_CLASS_NAME(ImStreamWithEncoding)&& other) :
			BaseClass(&m_streamBuf),
			m_streamBuf(std::move(other.m_streamBuf))
		{
		}
#endif

		DFG_CLASS_NAME(StreamBufferMemWithEncoding) m_streamBuf;

	};

#if DFG_LANGFEAT_MOVABLE_STREAMS
	// If text encoding is given, no BOM check is made. Otherwise it is checked and consumed if found.
	inline DFG_CLASS_NAME(ImStreamWithEncoding) createImStreamWithEncoding(const char* p, const size_t nSize, TextEncoding encoding = encodingUnknown)
	{
		DFG_CLASS_NAME(ImStreamWithEncoding) strm(p, nSize, encoding);
		return std::move(strm);
	}

	// Overload for wchar_t.
	inline DFG_CLASS_NAME(ImStreamWithEncoding) createImStreamWithEncoding(const wchar_t* p, const size_t nCharCount)
	{
		DFG_CLASS_NAME(ImStreamWithEncoding) strm(p, nCharCount);
		return std::move(strm);
	}
#endif

} } // module namespace
