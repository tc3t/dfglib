#include <stdafx.h>
#include <dfg/str.hpp>
#include <dfg/rand.hpp>
#include <dfg/alg.hpp>
#include <dfg/str/hex.hpp>
#include <dfg/dfgBaseTypedefs.hpp>

TEST(dfgStr, strLen)
{
    using namespace DFG_ROOT_NS;
    
    const char* psz0 = "";
    const char* pszw0 = "";
    const char psz0Sized[] = "";
    const wchar_t pszw0Sized[] = L"";
    const char* psz = "asdmv";
    const wchar_t* pszw = L"asdmv";
    const char pszSized[] = "asdm";
    const wchar_t pszwSized[] = L"asdm";

    const std::string s = "0123456789";
    const std::wstring sw = L"0123456789";

    const char pszEmbNull[] = {'0', '\0', '1', '\0'};
    const wchar_t pszwEmbNull[] = {'0', '\0', '1', '\0'};
    const std::string sEmbNull(pszEmbNull, pszEmbNull + DFG_COUNTOF_CSL(pszEmbNull));
    const std::wstring swEmbNull(pszwEmbNull, pszwEmbNull + DFG_COUNTOF_CSL(pszwEmbNull));

    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(psz0), 0);
    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(pszw0), 0);
    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(psz0Sized), 0);
    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(pszw0Sized), 0);
    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(psz), 5);
    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(pszw), 5);
    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(pszSized), 4);
    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(pszwSized), 4);

    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(s), 10);
    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(sw), 10);

    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(pszEmbNull), 1);
    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(pszwEmbNull), 1);
    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(sEmbNull), 3);
    EXPECT_EQ(DFG_SUB_NS_NAME(str)::strLen(swEmbNull), 3);

    using namespace DFG_MODULE_NS(str);

    EXPECT_EQ(0, strLen(""));
    EXPECT_EQ(1, strLen("a"));
    EXPECT_EQ(4, strLen(std::string("abcd")));
    EXPECT_EQ(6, strLen(std::string("abcd\ne")));
    EXPECT_EQ(1, strLen(std::string("a\0b"))); // Note: The created std::string is "a"
    EXPECT_EQ(0, strLen(L""));
    EXPECT_EQ(1, strLen(L"a"));
    EXPECT_EQ(4, strLen(std::wstring(L"abcd")));
    EXPECT_EQ(6, strLen(std::wstring(L"abcd\ne")));
    EXPECT_EQ(1, strLen(std::wstring(L"a\0b"))); // Note: The created std::wstring is "a"
}

TEST(dfgStr, strToByLexCast)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);

    const char sz[] = "101";
    const wchar_t szW[] = L"101";
    const std::string s = sz;
    const std::wstring sW = szW;
    int a0, a1, a2, a3;
    bool bSuccess = false;
    DFG_SUB_NS_NAME(str)::strToByNoThrowLexCast(sz, a0, &bSuccess);
    EXPECT_EQ(101, a0);
    EXPECT_EQ(true, bSuccess);
    DFG_SUB_NS_NAME(str)::strToByNoThrowLexCast(sz, a1, &bSuccess);
    EXPECT_EQ(101, a1);
    EXPECT_EQ(true, bSuccess);
    DFG_SUB_NS_NAME(str)::strToByNoThrowLexCast(s, a2, &bSuccess);
    EXPECT_EQ(101, a2);
    EXPECT_EQ(true, bSuccess);
    DFG_SUB_NS_NAME(str)::strToByNoThrowLexCast(sW, a3, &bSuccess);
    EXPECT_EQ(101, a3);
    EXPECT_EQ(true, bSuccess);

    DFG_SUB_NS_NAME(str)::strToByNoThrowLexCast("1a0", a0, &bSuccess);
    EXPECT_EQ(false, bSuccess);

}

TEST(dfgStr, strTo)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);

#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant

    char szBufferA[128];
    wchar_t szBufferW[128];
#define CHECK_A_AND_W(type, val, radix, szExpected) \
        EXPECT_FALSE(strCmp(toStr(val, szBufferA, radix), szExpected)); \
        EXPECT_FALSE(strCmp(toStr(val, szBufferW, radix), L##szExpected)); \
        if (radix == 10) \
                        { \
                    EXPECT_FALSE(strCmp(szBufferA, toCstr(toStrC(val)))); \
                    EXPECT_FALSE(strCmp(szBufferW, toCstr(toStrW(val)))); \
                    EXPECT_EQ(strTo<type>(szBufferA), val); \
                    EXPECT_EQ(strTo<type>(szBufferW), val); \
                        }

    //CHECK_A_AND_W(int8, int8_min, 10, "-128");
    CHECK_A_AND_W(int16, int16_min, 10, "-32768");
    CHECK_A_AND_W(int32, int32_min, 10, "-2147483648");
    CHECK_A_AND_W(int64, int64_min, 10, "-9223372036854775808");

    //CHECK_A_AND_W(int8, int8_max, 2, "1111111");
    //CHECK_A_AND_W(int8, int8_max, 10, "127");
    //CHECK_A_AND_W(int8, int8_max, 16, "7f");

    CHECK_A_AND_W(int16, int16_max, 2, "111111111111111");
    CHECK_A_AND_W(int16, int16_max, 10, "32767");
    CHECK_A_AND_W(int16, int16_max, 16, "7fff");

    CHECK_A_AND_W(int32, int32_max, 2, "1111111111111111111111111111111");
    CHECK_A_AND_W(int32, int32_max, 10, "2147483647");
    CHECK_A_AND_W(int32, int32_max, 16, "7fffffff");

    CHECK_A_AND_W(int64, int64_max, 2, "111111111111111111111111111111111111111111111111111111111111111");
    CHECK_A_AND_W(int64, int64_max, 10, "9223372036854775807");
    CHECK_A_AND_W(int64, int64_max, 16, "7fffffffffffffff");

    //CHECK_A_AND_W(uint8, uint8_max, 2, "11111111");
    //CHECK_A_AND_W(uint8, uint8_max, 10, "255");
    //CHECK_A_AND_W(uint8, uint8_max, 16, "ff");

    CHECK_A_AND_W(uint16, uint16_max, 2, "1111111111111111");
    CHECK_A_AND_W(uint16, uint16_max, 10, "65535");
    CHECK_A_AND_W(uint16, uint16_max, 16, "ffff");

    CHECK_A_AND_W(uint32, uint32_max, 2, "11111111111111111111111111111111");
    CHECK_A_AND_W(uint32, uint32_max, 10, "4294967295");
    CHECK_A_AND_W(uint32, uint32_max, 16, "ffffffff");

    CHECK_A_AND_W(uint64, uint64_max, 2, "1111111111111111111111111111111111111111111111111111111111111111");
    CHECK_A_AND_W(uint64, uint64_max, 10, "18446744073709551615");
    CHECK_A_AND_W(uint64, uint64_max, 16, "ffffffffffffffff");
    CHECK_A_AND_W(uint64, uint64_max, 32, "fvvvvvvvvvvvv");


    EXPECT_EQ(strTo<int32>("2147483647"), int32_max);
    EXPECT_EQ(strTo<const int32>("2147483647"), int32_max);

    EXPECT_EQ(strTo<uint32>("4294967295"), uint32_max);
    EXPECT_EQ(strTo<const uint32>("4294967295"), uint32_max);

    EXPECT_EQ(strTo<int64>("-9223372036854775808"), int64_min);
    EXPECT_EQ(strTo<const int64>("-9223372036854775808"), int64_min);

    EXPECT_EQ(strTo<int64>("9223372036854775807"), int64_max);
    EXPECT_EQ(strTo<const int64>("9223372036854775807"), int64_max);

    EXPECT_EQ(strTo<uint64>("18446744073709551615"), uint64_max);
    EXPECT_EQ(strTo<const uint64>("18446744073709551615"), uint64_max);
    
    volatile uint32 volVal = 1;
    strTo("4294967295", volVal);
    EXPECT_EQ(volVal, uint32_max);

    EXPECT_EQ(1.0, strTo<long double>("1"));

    /*
    for(size_t i = 0; i < 37; i++)
    {
        toStr(i, szBufferA, 36);
        std::cout << szBufferA << endl;
    }
    */

#undef CHECK_A_AND_W

#pragma warning(pop)
}

TEST(dfgStr, toStr)
{
    using namespace DFG_ROOT_NS;
    EXPECT_EQ("-1", DFG_SUB_NS_NAME(str)::toStrC(-1));
    EXPECT_EQ("123456789", DFG_SUB_NS_NAME(str)::toStrC(123456789.0));
    EXPECT_EQ("123456789.012345", DFG_SUB_NS_NAME(str)::toStrC(123456789.012345));
    EXPECT_EQ("-123456789.01234567", DFG_SUB_NS_NAME(str)::toStrC(-123456789.01234567));
    EXPECT_EQ("5896249", DFG_SUB_NS_NAME(str)::toStrC(5896249));
}

namespace
{
    void ReadOnlyParamStrOverloadTest(DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlyParamStrC) sC)
    {
        std::string s = sC.c_str();
    }
    void ReadOnlyParamStrOverloadTest(DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlyParamStrW) sW)
    {
        std::wstring s = sW.c_str(); 
    }

    void ReadOnlyParamStrWithSizeOverloadTest(DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlyParamStrWithSizeC) sC)
    {
        std::string s = sC.c_str();
    }
    void ReadOnlyParamStrWithSizeOverloadTest(DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlyParamStrWithSizeW) sW)
    {
        std::wstring s = sW.c_str();
    }
}

TEST(dfgStr, ReadOnlyParamStr)
{
    using namespace DFG_ROOT_NS;
    const auto func0 = [](DFG_CLASS_NAME(ReadOnlyParamStrC) s)
    {
        return DFG_ROOT_NS::DFG_SUB_NS_NAME(str)::strTo<size_t>(s);
    };
    const auto func1 = [](DFG_CLASS_NAME(ReadOnlyParamStrW) s)
    {
        return DFG_ROOT_NS::DFG_SUB_NS_NAME(str)::strTo<size_t>(s);
    };
    const auto func2 = [](DFG_CLASS_NAME(ReadOnlyParamStrWithSizeC) s)
    {
        return DFG_ROOT_NS::DFG_SUB_NS_NAME(str)::strTo<size_t>(s) + s.length();
    };
    const auto func3 = [](DFG_CLASS_NAME(ReadOnlyParamStrWithSizeW) s)
    {
        return DFG_ROOT_NS::DFG_SUB_NS_NAME(str)::strTo<size_t>(s) + s.length();
    };

    EXPECT_EQ(987654321, func0("987654321"));
    EXPECT_EQ(987654321, func0(std::string("987654321")));

    EXPECT_EQ(size_t(3876543210), func1(L"3876543210"));
    EXPECT_EQ(size_t(3876543210), func1(std::wstring(L"3876543210")));

    EXPECT_EQ(990, func2("987"));
    EXPECT_EQ(990, func2(std::string("987")));

    EXPECT_EQ(990, func3(L"987"));
    EXPECT_EQ(990, func3(std::wstring(L"987")));

    ReadOnlyParamStrOverloadTest("test");
    ReadOnlyParamStrOverloadTest(L"test");
    ReadOnlyParamStrOverloadTest(std::string("test"));
    ReadOnlyParamStrOverloadTest(std::wstring(L"test"));

    ReadOnlyParamStrWithSizeOverloadTest("test");
    ReadOnlyParamStrWithSizeOverloadTest(L"test");
    ReadOnlyParamStrWithSizeOverloadTest(std::string("test"));
    ReadOnlyParamStrWithSizeOverloadTest(std::wstring(L"test"));
}

TEST(dfgStr, HexStr)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(alg);
    using namespace DFG_MODULE_NS(str);
    using namespace DFG_MODULE_NS(rand);

    auto re = DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();
    for (size_t i = 0; i<20; i++)
    {
        std::vector<char> v(DFG_MODULE_NS(rand)::rand(re, 10, 1000));
        forEachFwd(v, [&](char& ch) {ch = DFG_MODULE_NS(rand)::rand(re, DFG_ROOT_NS::int8_min, DFG_ROOT_NS::int8_max); });
        const auto s = bytesToHexStr(&v[0], v.size());
        std::vector<char> sComp(v.size() * 2);
        bytesToHexStr(&v[0], v.size(), &sComp[0], false);
        EXPECT_EQ(memcmp(s.c_str(), &sComp[0], sComp.size()), 0);
        std::vector<char> v2(v.size(), 0);
        EXPECT_EQ(v.size(), v2.size());
        hexStrToBytes(s.c_str(), &v2[0], v2.size());
        EXPECT_EQ(memcmp(&v[0], &v2[0], v.size()), 0);
    }

    int32 b[10];
    char b2[sizeof(int32)*10];
    // +1 in int32_min is for VC2010 buggy uniform_int_distribution, for which the workaround does not available in all cases.
    forEachFwd(b, [&](int& v){v = DFG_MODULE_NS(rand)::rand(re, DFG_ROOT_NS::int32_min+1, DFG_ROOT_NS::int32_max); });
    auto str = bytesToHexStr(b, sizeof(b));
    hexStrToBytes(str.c_str(), b2, sizeof(b2));
    EXPECT_EQ(memcmp(b, b2, sizeof(b)), 0);
}

TEST(dfgStr, skipWhitespacesSz)
{
    using namespace DFG_MODULE_NS(str);
    const char sz0C[] = "abc";
    const char sz1C[] = " abc";
    const char sz2C[] = " \t abc";
    char sz3C[] = " \t abc";
    const wchar_t sz0W[] = L"abc";
    const wchar_t sz1W[] = L" abc";
    const wchar_t sz2W[] = L" \t abc";
    wchar_t sz3W[] = L" \t abc";

    EXPECT_EQ('a', *skipWhitespacesSz(sz0C));
    EXPECT_EQ('a', *skipWhitespacesSz(sz1C));
    EXPECT_EQ('a', *skipWhitespacesSz(sz2C, " \t"));
    EXPECT_EQ('a', *skipWhitespacesSz(sz0W));
    EXPECT_EQ('a', *skipWhitespacesSz(sz1W));
    EXPECT_EQ('a', *skipWhitespacesSz(sz2W, L" \t"));

    // Test that modifiable input returns modifiable output.
    *skipWhitespacesSz(sz3C) = '1';
    *skipWhitespacesSz(sz3W) = '2';
    EXPECT_EQ('1', sz3C[1]);
    EXPECT_EQ('2', sz3W[1]);

}
