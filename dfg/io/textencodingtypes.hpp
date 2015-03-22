#pragma once

#include "../dfgDefs.hpp"
#include "../bits/byteSwap.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

	enum TextEncoding
	{
		encodingUnknown,
		encodingUTF8,
		encodingUTF16Le,
		encodingUTF16Be,
		encodingUTF32Le,
		encodingUTF32Be,
		encodingLatin1,
		encodingUCS2Le,
		encodingUCS2Be,
		encodingUCS4Le,
		encodingUCS4Be
	};

	inline bool isUtfEncoding(TextEncoding encoding)
	{
		return (encoding >= encodingUTF8 && encoding <= encodingUTF32Be);
	}

	inline bool isBigEndianEncoding(TextEncoding encoding)
	{
		return (encoding == encodingUTF16Be || encoding == encodingUTF32Be);
	}

	inline TextEncoding hostNativeFixedSizeUtfEncodingFromCharType(const size_t charSizeInBytes)
	{
		if (charSizeInBytes == 1)
			return encodingLatin1;
		if (charSizeInBytes == 2)
			return (byteOrderHost() == ByteOrderLittleEndian) ? encodingUCS2Le : encodingUCS2Be;
		else if (charSizeInBytes == 4)
			return (byteOrderHost() == ByteOrderLittleEndian) ? encodingUCS4Le : encodingUCS4Be;
		return encodingUnknown;
	}

	inline size_t baseCharacterSize(TextEncoding encoding)
	{
		switch (encoding)
		{
			case encodingUTF8:
				return 1;
			case encodingUTF16Be:
			case encodingUTF16Le:
				return 2;
			case encodingUTF32Be:
			case encodingUTF32Le:
				return 4;
			default: return 0;
		}
	}

} }