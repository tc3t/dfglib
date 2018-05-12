#include "stdafx.h"
#include <dfg/utf.hpp>

TEST(DfgUtf, utfGeneral)
{
	using namespace DFG_ROOT_NS;
	using namespace DFG_MODULE_NS(utf);
	std::string s;
	for (size_t i = 0; i < 256; ++i)
		s.push_back(char(i));

	const auto sUtf8 = latin1ToUtf8(s);
	const auto sConvertedLatin1 = utf8ToLatin1(sUtf8);
	const auto sFixedSize = utf8ToFixedChSizeStr<char>(sUtf8);
	const auto sFixedSizeW = utf8ToFixedChSizeStr<wchar_t>(sUtf8);

	EXPECT_EQ(s, sConvertedLatin1);
	EXPECT_EQ(s, sFixedSize);
	EXPECT_TRUE((std::equal(s.begin(), s.end(), sFixedSizeW.begin(), [](char ch, wchar_t chW){return DFG_ROOT_NS::uint8(ch) == chW; })));

	std::vector<uint16> sUtf16, sUtf16_2;
	std::vector<uint32> sUtf32, sUtf32_2;
	utf8::utf8to16(sUtf8.cbegin(), sUtf8.cend(), std::back_inserter(sUtf16));
	utf8To16Native(sUtf8, std::back_inserter(sUtf16_2));
	EXPECT_EQ(sUtf16, sUtf16_2);
	utf8::utf8to32(sUtf8.cbegin(), sUtf8.cend(), std::back_inserter(sUtf32));
	utf8To32Native(sUtf8, std::back_inserter(sUtf32_2));
	EXPECT_EQ(sUtf32, sUtf32_2);

	std::string sUtf8FromUtf16;
	utf16To8(sUtf16, std::back_inserter(sUtf8FromUtf16));
	EXPECT_EQ(sUtf8, sUtf8FromUtf16);

	std::vector<uint32> sUtf32FromUtf16;
	utf16To32(sUtf16, std::back_inserter(sUtf32FromUtf16));
	EXPECT_EQ(sUtf32, sUtf32FromUtf16);

	std::string sUtf8FromUtf32;
	utf32To8(sUtf32, std::back_inserter(sUtf8FromUtf32));
	EXPECT_EQ(sUtf8, sUtf8FromUtf32);

	std::vector<uint16> sUtf16FromUtf32;
	utf32To16(sUtf32, std::back_inserter(sUtf16FromUtf32));
	EXPECT_EQ(sUtf16, sUtf16FromUtf32);

	const auto sFixedSizeFromUtf16C = utf16ToFixedChSizeStr<char>(sUtf16);
	const auto sFixedSizeFromUtf16W = utf16ToFixedChSizeStr<wchar_t>(sUtf16);
	const auto sFixedSizeFromUtf16_ch16 = utf16ToFixedChSizeStr<char16_t>(sUtf16);
	const auto sFixedSizeFromUtf16_ch32 = utf16ToFixedChSizeStr<char32_t>(sUtf16);
	EXPECT_TRUE(std::equal(sFixedSize.begin(), sFixedSize.end(), sFixedSizeFromUtf16C.begin()));
	EXPECT_TRUE(std::equal(sFixedSizeW.begin(), sFixedSizeW.end(), sFixedSizeFromUtf16W.begin()));
	EXPECT_TRUE(std::equal(sFixedSizeFromUtf16W.begin(), sFixedSizeFromUtf16W.end(), sFixedSizeFromUtf16_ch16.begin()));
	EXPECT_TRUE(std::equal(sFixedSizeFromUtf16W.begin(), sFixedSizeFromUtf16W.end(), sFixedSizeFromUtf16_ch32.begin()));

	const auto sFixedSizeFromUtf32C = utf32ToFixedChSizeStr<char>(sUtf32);
	const auto sFixedSizeFromUtf32W = utf32ToFixedChSizeStr<wchar_t>(sUtf32);
	const auto sFixedSizeFromUtf32_ch16 = utf32ToFixedChSizeStr<char16_t>(sUtf32);
	const auto sFixedSizeFromUtf32_ch32 = utf32ToFixedChSizeStr<char32_t>(sUtf32);
	EXPECT_TRUE(std::equal(sFixedSize.begin(), sFixedSize.end(), sFixedSizeFromUtf32C.begin()));
	EXPECT_TRUE(std::equal(sFixedSizeW.begin(), sFixedSizeW.end(), sFixedSizeFromUtf32W.begin()));
	EXPECT_TRUE(std::equal(sFixedSizeFromUtf16W.begin(), sFixedSizeFromUtf16W.end(), sFixedSizeFromUtf32_ch16.begin()));
	EXPECT_TRUE(std::equal(sFixedSizeFromUtf16W.begin(), sFixedSizeFromUtf16W.end(), sFixedSizeFromUtf32_ch32.begin()));

	const auto sUtf8FromFixedSizeC = codePointsToUtf8(sFixedSizeFromUtf32C);
	const auto sUtf8FromFixedSizeW = codePointsToUtf8(sFixedSizeFromUtf32W);
	const auto sUtf8FromFixedSize16 = codePointsToUtf8(sFixedSizeFromUtf32_ch16);
	const auto sUtf8FromFixedSize32 = codePointsToUtf8(sFixedSizeFromUtf32_ch32);
	EXPECT_EQ(sUtf8, sUtf8FromFixedSizeC);
	EXPECT_EQ(sUtf8, sUtf8FromFixedSizeW);
	EXPECT_EQ(sUtf8, sUtf8FromFixedSize16);
	EXPECT_EQ(sUtf8, sUtf8FromFixedSize32);

	// Test copdePointsToUtf
	{
		std::string sUtf8FromCodePointToUtf;
		codePointsToUtf<char>(sFixedSizeFromUtf32C, std::back_inserter(sUtf8FromCodePointToUtf));
		EXPECT_EQ(sUtf8, sUtf8FromCodePointToUtf);
		sUtf8FromCodePointToUtf.clear();
		codePointsToUtf<char>(sFixedSizeFromUtf32W, std::back_inserter(sUtf8FromCodePointToUtf));
		EXPECT_EQ(sUtf8, sUtf8FromCodePointToUtf);
		sUtf8FromCodePointToUtf.clear();
		codePointsToUtf<char>(sFixedSizeFromUtf32_ch16, std::back_inserter(sUtf8FromCodePointToUtf));
		EXPECT_EQ(sUtf8, sUtf8FromCodePointToUtf);
		sUtf8FromCodePointToUtf.clear();
		codePointsToUtf<char>(sFixedSizeFromUtf32_ch32, std::back_inserter(sUtf8FromCodePointToUtf));
		EXPECT_EQ(sUtf8, sUtf8FromCodePointToUtf);
		sUtf8FromCodePointToUtf.clear();

		std::vector<uint16> sUtf16FromCodePointToUtf;
		codePointsToUtf<uint16>(sFixedSizeFromUtf32C, std::back_inserter(sUtf16FromCodePointToUtf));
		EXPECT_EQ(sUtf16, sUtf16FromCodePointToUtf);
		sUtf16FromCodePointToUtf.clear();
		codePointsToUtf<uint16>(sFixedSizeFromUtf32W, std::back_inserter(sUtf16FromCodePointToUtf));
		EXPECT_EQ(sUtf16, sUtf16FromCodePointToUtf);
		sUtf16FromCodePointToUtf.clear();
		codePointsToUtf<uint16>(sFixedSizeFromUtf32_ch16, std::back_inserter(sUtf16FromCodePointToUtf));
		EXPECT_EQ(sUtf16, sUtf16FromCodePointToUtf);
		sUtf16FromCodePointToUtf.clear();
		codePointsToUtf<uint16>(sFixedSizeFromUtf32_ch32, std::back_inserter(sUtf16FromCodePointToUtf));
		EXPECT_EQ(sUtf16, sUtf16FromCodePointToUtf);
		sUtf16FromCodePointToUtf.clear();

		std::vector<uint32> sUtf32FromCodePointToUtf;
		codePointsToUtf<uint32>(sFixedSizeFromUtf32C, std::back_inserter(sUtf32FromCodePointToUtf));
		EXPECT_EQ(sUtf32, sUtf32FromCodePointToUtf);
		sUtf32FromCodePointToUtf.clear();
		codePointsToUtf<uint32>(sFixedSizeFromUtf32W, std::back_inserter(sUtf32FromCodePointToUtf));
		EXPECT_EQ(sUtf32, sUtf32FromCodePointToUtf);
		sUtf32FromCodePointToUtf.clear();
		codePointsToUtf<uint32>(sFixedSizeFromUtf32_ch16, std::back_inserter(sUtf32FromCodePointToUtf));
		EXPECT_EQ(sUtf32, sUtf32FromCodePointToUtf);
		sUtf32FromCodePointToUtf.clear();
		codePointsToUtf<uint32>(sFixedSizeFromUtf32_ch32, std::back_inserter(sUtf32FromCodePointToUtf));
		EXPECT_EQ(sUtf32, sUtf32FromCodePointToUtf);
		sUtf32FromCodePointToUtf.clear();
	}

	auto iterUtf8 = sUtf8.cbegin();
	const auto iterEndUtf8(sUtf8.cend());
	size_t i = 0;
	for(; iterUtf8 != iterEndUtf8; ++i)
	{
		EXPECT_TRUE(i < sUtf16.size());
		EXPECT_TRUE(i < sUtf32.size());
		uint32 fromUtf8 = next(iterUtf8, iterEndUtf8);
		const int fromUtf16 = (i < sUtf16.size()) ? sUtf16[i] : -1;
		const int fromUtf32 = (i < sUtf32.size()) ? sUtf32[i] : -1;
		EXPECT_EQ(i, fromUtf8);
		EXPECT_EQ(i, fromUtf16);
		EXPECT_EQ(i, fromUtf32);
	}
	EXPECT_EQ(i, 256);
}

TEST(DfgUtf, utfUnconvertableHandling)
{
	using namespace DFG_ROOT_NS;
	using namespace DFG_MODULE_NS(utf);

	std::wstring s;
	s.push_back('u' + 200);
	s.push_back('t');
	s.push_back('f' + 200);

	const auto sUtf8 = codePointsToUtf8(s);
	auto sLatin1 = utf8ToLatin1(sUtf8);
	EXPECT_EQ("?t?", sLatin1);

	size_t nUnconvertableCounter = 0;
	std::vector<uint32> unconvertable;
	std::vector<uint32> unconvertableExpected;
	unconvertableExpected.push_back('u' + 200);
	unconvertableExpected.push_back('f' + 200);
	sLatin1 = utf8ToLatin1(sUtf8, [&](UnconvertableCpHandlerParam param) -> uint32
									{
										nUnconvertableCounter++;
										unconvertable.push_back(param.cp);
										return param.cp - 200;
									});
	EXPECT_EQ(2, nUnconvertableCounter);
	EXPECT_EQ("utf", sLatin1);
	EXPECT_EQ(unconvertableExpected, unconvertable);
}

TEST(DfgUtf, cpToUtfToCp)
{
	using namespace DFG_ROOT_NS;
	using namespace DFG_MODULE_NS(io);
	using namespace DFG_MODULE_NS(utf);

	std::basic_string<char32_t> cpExpected;
#ifdef _DEBUG // 70000 takes seconds in debug run so don't go that far.
	for (uint32 i = 0; i < 5000; ++i)
#else
	for (uint32 i = 0; i < utf8::internal::LEAD_SURROGATE_MIN; ++i) // Note: utf16 won't work if upper limit is > LEAD_SURROGATE_MIN
#endif
		cpExpected.push_back(i);

	std::string sUtf8;
	std::basic_string<char16_t> sUtf16Le;
	std::basic_string<char16_t> sUtf16Be;
	std::basic_string<char32_t> sUtf32Le;
	std::basic_string<char32_t> sUtf32Be;

	auto inserterUtf8 = std::back_inserter(sUtf8);
	auto inserterUtf16Le = std::back_inserter(sUtf16Le);
	auto inserterUtf16Be = std::back_inserter(sUtf16Be);
	auto inserterUtf32Le = std::back_inserter(sUtf32Le);
	auto inserterUtf32Be = std::back_inserter(sUtf32Be);

	for (size_t i = 0; i < cpExpected.size(); ++i)
	{
		cpToEncoded(cpExpected[i], inserterUtf8, encodingUTF8);
		cpToEncoded(cpExpected[i], inserterUtf16Le, encodingUTF16Le);
		cpToEncoded(cpExpected[i], inserterUtf16Be, encodingUTF16Be);
		cpToEncoded(cpExpected[i], inserterUtf32Le, encodingUTF32Le);
		cpToEncoded(cpExpected[i], inserterUtf32Be, encodingUTF32Be);
	}

	const auto cpFromUtf8 = utf8ToFixedChSizeStr<char32_t>(sUtf8);
	const auto cpFromUtf16Le = utf16ToFixedChSizeStr<char32_t>(sUtf16Le, ByteOrderLittleEndian);
	const auto cpFromUtf16Be = utf16ToFixedChSizeStr<char32_t>(sUtf16Be, ByteOrderBigEndian);
	const auto cpFromUtf32Le = utf32ToFixedChSizeStr<char32_t>(sUtf32Le, ByteOrderLittleEndian);
	const auto cpFromUtf32Be = utf32ToFixedChSizeStr<char32_t>(sUtf32Be, ByteOrderBigEndian);

	EXPECT_EQ(cpExpected, cpFromUtf8);
	EXPECT_EQ(cpExpected, cpFromUtf16Le);
	EXPECT_EQ(cpExpected, cpFromUtf16Be);
	EXPECT_EQ(cpExpected, cpFromUtf32Le);
	EXPECT_EQ(cpExpected, cpFromUtf32Be);
}

TEST(DfgUtf, bomSizeInBytes)
{
	using namespace DFG_ROOT_NS;
	using namespace DFG_MODULE_NS(io);
	using namespace DFG_MODULE_NS(utf);

	EXPECT_EQ(3, bomSizeInBytes(encodingUTF8));
	EXPECT_EQ(2, bomSizeInBytes(encodingUTF16Le));
	EXPECT_EQ(2, bomSizeInBytes(encodingUTF16Be));
	EXPECT_EQ(4, bomSizeInBytes(encodingUTF32Le));
	EXPECT_EQ(4, bomSizeInBytes(encodingUTF32Be));
	EXPECT_EQ(0, bomSizeInBytes(encodingUnknown));
	EXPECT_EQ(0, bomSizeInBytes(encodingLatin1));
}

TEST(DfgUtf, encodingToBom)
{
	using namespace DFG_ROOT_NS;
	using namespace DFG_MODULE_NS(io);
	using namespace DFG_MODULE_NS(utf);

	const auto utf8Bom = encodingToBom(encodingUTF8);
	const auto utf16LeBom = encodingToBom(encodingUTF16Le);
	const auto utf16BeBom = encodingToBom(encodingUTF16Be);
	const auto utf32LeBom = encodingToBom(encodingUTF32Le);
	const auto utf32BeBom = encodingToBom(encodingUTF32Be);
	const auto unknownBom = encodingToBom(encodingUnknown);
	const auto latin1Bom = encodingToBom(encodingLatin1);

	EXPECT_EQ(-17, utf8Bom[0]);
	EXPECT_EQ(-69, utf8Bom[1]);
	EXPECT_EQ(-65, utf8Bom[2]);

	EXPECT_EQ(-1, utf16LeBom[0]);
	EXPECT_EQ(-2, utf16LeBom[1]);

	EXPECT_EQ(-2, utf16BeBom[0]);
	EXPECT_EQ(-1, utf16BeBom[1]);

	EXPECT_EQ(-1, utf32LeBom[0]);
	EXPECT_EQ(-2, utf32LeBom[1]);
	EXPECT_EQ(0, utf32LeBom[2]);
	EXPECT_EQ(0, utf32LeBom[3]);

	EXPECT_EQ(0, utf32BeBom[0]);
	EXPECT_EQ(0, utf32BeBom[1]);
	EXPECT_EQ(-2, utf32BeBom[2]);
	EXPECT_EQ(-1, utf32BeBom[3]);
}

