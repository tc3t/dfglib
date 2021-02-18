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
		using Streambuffer = DFG_CLASS_NAME(StreamBufferMemWithEncoding);

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

		static constexpr int_type eofValue()
		{
			return Streambuffer::eofValue();
		}

		Streambuffer m_streamBuf;

	};

#if DFG_LANGFEAT_MOVABLE_STREAMS
	// If text encoding is given, no BOM check is made. Otherwise it is checked and consumed if found.
	inline ImStreamWithEncoding createImStreamWithEncoding(const char* p, const size_t nSize, TextEncoding encoding = encodingUnknown)
	{
		return ImStreamWithEncoding(p, nSize, encoding);
	}

	// Overload for wchar_t.
	inline ImStreamWithEncoding createImStreamWithEncoding(const wchar_t* p, const size_t nCharCount)
	{
		return ImStreamWithEncoding(p, nCharCount);
	}
#endif

} } // module namespace
