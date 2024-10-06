#pragma once

#include "../dfgDefs.hpp"
#include "../io/textEncodingTypes.hpp"
#include "../dfgBaseTypedefs.hpp"
#include "../Span.hpp"

#define DFG_UTF_BOM_STR_UTF8    "\xEF" "\xBB" "\xBF"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(utf) {

	// http://unicode.org/faq/utf_bom.html#BOM
	const unsigned char bomUTF8[3] = { 0xEF, 0xBB, 0xBF };
	const unsigned char bomUTF16Le[2] = { 0xFF, 0xFE };
	const unsigned char bomUTF16Be[2] = { 0xFE, 0xFF };
	const unsigned char bomUTF32Le[4] = { 0xFF, 0xFE, 0, 0 };
	const unsigned char bomUTF32Be[4] = { 0, 0, 0xFE, 0xFF };

	inline uint8 bomSizeInBytes(DFG_SUB_NS_NAME(io)::TextEncoding encoding)
	{
		using namespace DFG_MODULE_NS(io);
		switch (encoding)
		{
		case encodingUTF8: return DFG_COUNTOF(bomUTF8);
		case encodingUTF16Le: return DFG_COUNTOF(bomUTF16Le);
		case encodingUTF16Be: return DFG_COUNTOF(bomUTF16Be);
		case encodingUTF32Le: return DFG_COUNTOF(bomUTF32Le);
		case encodingUTF32Be: return DFG_COUNTOF(bomUTF32Be);
		default: return 0;
		}
	}

	inline auto encodingToBom(const ::DFG_MODULE_NS(io)::TextEncoding encoding) -> Span<const char>
	{
		using namespace ::DFG_MODULE_NS(io);
		using ReturnType = Span<const char>;
		#define CREATE_RETURN_VALUE(ITEM) ReturnType(reinterpret_cast<const char*>(&ITEM), DFG_COUNTOF(ITEM));
		switch (encoding)
		{
		case encodingUTF8: return CREATE_RETURN_VALUE(bomUTF8);
		case encodingUTF16Le: return CREATE_RETURN_VALUE(bomUTF16Le);
		case encodingUTF16Be: return CREATE_RETURN_VALUE(bomUTF16Be);
		case encodingUTF32Le: return CREATE_RETURN_VALUE(bomUTF32Le);
		case encodingUTF32Be: return CREATE_RETURN_VALUE(bomUTF32Be);
		default: return ReturnType();
		}
#undef CREATE_RETURN_VALUE
	}

} }

