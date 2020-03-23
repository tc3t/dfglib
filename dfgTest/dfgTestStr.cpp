#include <stdafx.h>
#include <dfg/str.hpp>
#include <dfg/rand.hpp>
#include <dfg/alg.hpp>
#include <dfg/str/hex.hpp>
#include <dfg/str/strCat.hpp>
#include <dfg/str/string.hpp>
#include <dfg/dfgBaseTypedefs.hpp>
#include <dfg/str/stringLiteralCharToValue.hpp>
#include <dfg/str/format_fmt.hpp>
#include <dfg/preprocessor/compilerInfoMsvc.hpp>
#include <dfg/cont.hpp>
#include <dfg/utf.hpp>
#include <dfg/iter/szIterator.hpp>

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

    // Test typed null terminated strings
    EXPECT_EQ(3, strLen(SzPtrAscii("abc")));
    EXPECT_EQ(3, strLen(SzPtrLatin1("abc")));
    EXPECT_EQ(3, strLen(SzPtrUtf8("abc")));
}

TEST(dfgStr, strCpyAllThatFit)
{
    using namespace DFG_MODULE_NS(str);
    char szDest[4];

#define STRCPYALLTHATFIT_TESTCASE(SRC, EXPECTED) \
    std::fill(std::begin(szDest), std::end(szDest), 'U'); \
    EXPECT_STREQ(EXPECTED, strCpyAllThatFit(szDest, SRC)); \
    EXPECT_STREQ(EXPECTED, strCpyAllThatFit(szDest, DFG_COUNTOF(szDest), SRC));

    char* pNull = nullptr;

    EXPECT_EQ(nullptr, strCpyAllThatFit(pNull, 0, pNull));
    EXPECT_EQ(nullptr, strCpyAllThatFit(pNull, 0, "a"));
    EXPECT_STREQ("", strCpyAllThatFit(szDest, pNull));

    STRCPYALLTHATFIT_TESTCASE("a", "a");
    STRCPYALLTHATFIT_TESTCASE("ab", "ab");
    STRCPYALLTHATFIT_TESTCASE("abc", "abc");
    STRCPYALLTHATFIT_TESTCASE("abcd", "abc");
    STRCPYALLTHATFIT_TESTCASE("abcdefg", "abc");

#undef STRCPYALLTHATFIT_TESTCASE
}

TEST(dfgStr, isEmptyStr)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);

    EXPECT_TRUE(isEmptyStr(""));
    EXPECT_FALSE(isEmptyStr("a"));
    EXPECT_TRUE(isEmptyStr(L""));
    EXPECT_FALSE(isEmptyStr(L"a"));
    EXPECT_TRUE(isEmptyStr(std::string("")));
    EXPECT_FALSE(isEmptyStr(std::string(1, '\0')));
    EXPECT_TRUE(isEmptyStr(std::wstring(L"")));
    EXPECT_FALSE(isEmptyStr(std::wstring(1, L'\0')));

    // SzPtr
    EXPECT_TRUE(isEmptyStr(SzPtrUtf8("")));
    EXPECT_FALSE(isEmptyStr(SzPtrUtf8("a")));

    // Typed string
    EXPECT_TRUE(isEmptyStr(StringUtf8(SzPtrUtf8(""))));
    EXPECT_FALSE(isEmptyStr(StringUtf8(SzPtrUtf8("a"))));

    // StringView
    EXPECT_TRUE(isEmptyStr(StringViewC("")));
    EXPECT_FALSE(isEmptyStr(StringViewC(std::string(1, '\0'))));
    EXPECT_FALSE(isEmptyStr(StringViewUtf8(SzPtrUtf8("a"))));
}

TEST(dfgStr, strCmp)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);

    // String literal
    EXPECT_EQ(std::strcmp("a", "a"), strCmp("a", "a"));
    EXPECT_EQ(std::strcmp("b", "a"), strCmp("b", "a"));

    // Wide string literal
    EXPECT_EQ(std::wcscmp(L"a", L"a"), strCmp(L"a", L"a"));
    EXPECT_EQ(std::wcscmp(L"b", L"a"), strCmp(L"b", L"a"));

    // SzPtr
    EXPECT_EQ(0, strCmp(SzPtrUtf8("a"), SzPtrUtf8("a")));
    EXPECT_TRUE(strCmp(SzPtrUtf8("a"), SzPtrUtf8("b")) < 0);
}

TEST(dfgStr, strCat)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);

    std::string s;
    std::wstring sw;

    // Verify return types.
    {
        // When given a rvalue, return value should be a string.
        DFGTEST_STATIC((std::is_same<decltype(strCat(std::string(), "a")), std::string>::value));
        DFGTEST_STATIC((std::is_same<decltype(strCat(std::wstring(), L"a")), std::wstring>::value));

        // When given a lvalue, return value should be a string reference.
        DFGTEST_STATIC((std::is_same<decltype(strCat(s, "a")), std::string&>::value)); 
        DFGTEST_STATIC((std::is_same<decltype(strCat(sw, L"a")), std::wstring&>::value));
    }

    // Typed strCat()
    {
        EXPECT_EQ(SzPtrAscii("ascii_strCat_test"), (strCat(DFG_CLASS_NAME(StringAscii)(), SzPtrAscii("ascii_strCat"), DFG_CLASS_NAME(StringAscii)(SzPtrAscii("_")), SzPtrAscii("test"))));
        EXPECT_EQ(SzPtrLatin1("latin1_strCat_test"), (strCat(DFG_CLASS_NAME(StringLatin1)(), SzPtrLatin1("latin1_strCat"), DFG_CLASS_NAME(StringLatin1)(SzPtrLatin1("_")), SzPtrLatin1("test"))));
        EXPECT_EQ(SzPtrUtf8("utf8_strCat_test"), (strCat(DFG_CLASS_NAME(StringUtf8)(), SzPtrUtf8("utf8_strCat"), DFG_CLASS_NAME(StringUtf8)(SzPtrUtf8("_")), SzPtrUtf8("test"))));
    }

    // 1 param
    {
        s.clear();
        sw.clear();
        strCat(s, "1");
        strCat(sw, L"1");
        const auto sFromCopy = strCat(std::string(), "1"); // Test copy version.
        EXPECT_EQ(s, sFromCopy); 
        EXPECT_EQ(sw, strCat(std::wstring(), L"1")); // Test copy version.
        EXPECT_EQ("1", s);
        EXPECT_EQ(L"1", sw);
    }

    // 2 params
    {
        s.clear();
        sw.clear();
        strCat(s, "1", "2");
        strCat(sw, L"1", L"2");
        EXPECT_EQ("12", s);
        EXPECT_EQ(L"12", sw);
    }

    // 3 params
    {
        s.clear();
        sw.clear();
        strCat(s, "1", "2", "3");
        strCat(sw, L"1", L"2", L"3");
        EXPECT_EQ("123", s);
        EXPECT_EQ(L"123", sw);
    }

    // 4 params
    {
        s.clear();
        sw.clear();
        strCat(s, "1", "2", "3", "4");
        strCat(sw, L"1", L"2", L"3", L"4");
        EXPECT_EQ("1234", s);
        EXPECT_EQ(L"1234", sw);
    }

    // 5 params
    {
        s.clear();
        sw.clear();
        strCat(s, "        1", "+", toStrC(123), "+2=", toStrC(1 + 123 + 2));
        strCat(sw, L"        1", L"+", toStrW(123), L"+2=", toStrW(1 + 123 + 2));
        EXPECT_EQ("        1+123+2=126", s);
        EXPECT_EQ(L"        1+123+2=126", sw);
    }

    // 6 params
    {
        s.clear();
        sw.clear();
        strCat(s, "1", "2", "3", "4", "5", "6");
        strCat(sw, L"1", L"2", L"3", L"4", L"5", L"6");
        EXPECT_EQ("123456", s);
        EXPECT_EQ(L"123456", sw);
    }

    // 7 params
    {
        s.clear();
        sw.clear();
        strCat(s, "1", "2", "3", "4", "5", "6", "7");
        strCat(sw, L"1", L"2", L"3", L"4", L"5", L"6", L"7");
        EXPECT_EQ("1234567", s);
        EXPECT_EQ(L"1234567", sw);
    }

    // 8 params
    {
        s.clear();
        sw.clear();
        strCat(s, "1", "2", "3", "4", "5", "6", "7", "8");
        strCat(sw, L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8");
        EXPECT_EQ("12345678", s);
        EXPECT_EQ(L"12345678", sw);
    }

    // 9 params
    {
        s.clear();
        sw.clear();
        strCat(s, "1", "2", "3", "4", "5", "6", "7", "8", "9");
        strCat(sw, L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9");
        EXPECT_EQ("123456789", s);
        EXPECT_EQ(L"123456789", sw);
    }

    // 10 params
    {
        s.clear();
        sw.clear();
        strCat(s, "1", "2", "3", "4", "5", "6", "7", "8", "9", "10");
        strCat(sw, L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"10");
        EXPECT_EQ("12345678910", s);
        EXPECT_EQ(L"12345678910", sw);
    }

    // Check that reserved space is not made smaller
    {
        std::string sCapacityTest;
        sCapacityTest.reserve(50);
        const auto nCapacityAfterReserve = sCapacityTest.capacity();
        strCat(sCapacityTest, "a", "b");
        EXPECT_EQ(nCapacityAfterReserve, sCapacityTest.capacity());
        strCat(sCapacityTest, "");
        EXPECT_EQ(nCapacityAfterReserve, sCapacityTest.capacity());
    }
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
    EXPECT_FALSE(bSuccess);
}

namespace
{
    template <class Char_T>
    void strTo_leadingTrailingWhitespaces()
    {
        // Note: currently only ' ' is considered as whitespace; might want to revise this at some point.
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(str);
        EXPECT_EQ(1, strTo<int>(DFG_STRING_LITERAL_BY_CHARTYPE(Char_T, " 1")));
        EXPECT_EQ(1, strTo<int>(DFG_STRING_LITERAL_BY_CHARTYPE(Char_T, "1 ")));
        EXPECT_EQ(1, strTo<int>(DFG_STRING_LITERAL_BY_CHARTYPE(Char_T, " 1 ")));
        EXPECT_EQ(1.25, strTo<double>(DFG_STRING_LITERAL_BY_CHARTYPE(Char_T, " 1.25 ")));
    }
} // unnamed namespace


TEST(dfgStr, strTo)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);

#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant

    char szBufferA[128];
    wchar_t szBufferW[128];
#define CHECK_A_AND_W(type, val, radix, szExpected) \
        EXPECT_STREQ(szExpected, toStr(val, szBufferA, radix)); \
        EXPECT_STREQ(L##szExpected, toStr(val, szBufferW, radix)); \
        if (radix == 10) \
                        { \
                    EXPECT_STREQ(szBufferA, toCstr(toStrC(val))); \
                    EXPECT_STREQ(szBufferW, toCstr(toStrW(val))); \
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

    // Testing that strTo() accepts std::string and std::wstring
    {
        EXPECT_EQ(1, strTo<int>(std::string("1")));
        EXPECT_EQ(1, strTo<int>(std::wstring(L"1")));
    }

    // Test that strTo() accepts StringView's
    {
        EXPECT_EQ(1, strTo<int>(DFG_CLASS_NAME(StringViewC)("1")));
        EXPECT_EQ(1, strTo<int>(DFG_CLASS_NAME(StringViewW)(L"1")));
    }

    // Test that strTo() accepts StringViewSz's
    {
        EXPECT_EQ(1, strTo<int>(DFG_CLASS_NAME(StringViewSzC)("1")));
        EXPECT_EQ(1, strTo<int>(DFG_CLASS_NAME(StringViewSzW)(L"1")));
    }

    // Test typed strings
    {
        EXPECT_EQ(1, strTo<int>(DFG_ASCII("1")));
        EXPECT_EQ(1, strTo<int>(DFG_UTF8("1")));
        EXPECT_EQ(1, strTo<int>(DFG_CLASS_NAME(StringViewAscii)(DFG_ASCII("1"))));
        EXPECT_EQ(1, strTo<int>(DFG_CLASS_NAME(StringViewSzAscii)(DFG_ASCII("1"))));
        EXPECT_EQ(1, strTo<int>(DFG_CLASS_NAME(StringViewLatin1)(SzPtrLatin1("1"))));
        EXPECT_EQ(1, strTo<int>(DFG_CLASS_NAME(StringViewSzLatin1)(SzPtrLatin1("1"))));
        EXPECT_EQ(1, strTo<int>(DFG_CLASS_NAME(StringViewUtf8)(DFG_UTF8("1"))));
        EXPECT_EQ(1, strTo<int>(DFG_CLASS_NAME(StringViewSzUtf8)(DFG_UTF8("1"))));
    }

    // Testing that leading or trailing spaces won't break parsing
    {
        strTo_leadingTrailingWhitespaces<char>();
        strTo_leadingTrailingWhitespaces<wchar_t>();
    }

    // Testing that invalid input won't crash etc. and returns zero initialized value.
    {
        const char* p = nullptr;
        EXPECT_EQ(0, strTo<int>(p));
        EXPECT_EQ(0.0, strTo<double>(p));
        EXPECT_EQ(0, strTo<int>("abc"));
        EXPECT_EQ(0, strTo<int>(""));
        EXPECT_EQ(0.0, strTo<double>("  "));
    }

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

namespace
{
    template <class T>
    void toStrCommonFloatingPointTests(const char* pExpectedLowest, const char* pExpectedMax, const char* pExpectedMinPositive)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(str);

        typedef std::numeric_limits<T> NumLim;
        EXPECT_EQ("0.76", DFG_SUB_NS_NAME(str)::toStrC(static_cast<T>(0.76L))); // Note: T(0.76) can be != 0.76L if T=long double and sizeof(double) != sizeof(long double)

        {
            const auto s = DFG_SUB_NS_NAME(str)::toStrC(static_cast<T>(1e-9L));
            EXPECT_TRUE(s == "1e-9" || s == "1e-09" || s == "1e-009");
        }

        if (pExpectedLowest)
            EXPECT_EQ(pExpectedLowest, DFG_SUB_NS_NAME(str)::toStrC(NumLim::lowest()));
        if (pExpectedMax)
            EXPECT_EQ(pExpectedMax, DFG_SUB_NS_NAME(str)::toStrC(NumLim::max()));
        if (pExpectedMinPositive)
            EXPECT_EQ(pExpectedMinPositive, DFG_SUB_NS_NAME(str)::toStrC(NumLim::min()));

        // +-inf
        EXPECT_EQ("inf", DFG_SUB_NS_NAME(str)::toStrC(NumLim::infinity()));
        EXPECT_EQ("-inf", DFG_SUB_NS_NAME(str)::toStrC(-1 * NumLim::infinity()));

        // NaN
        EXPECT_TRUE(beginsWith(toStrC(NumLim::quiet_NaN()), "nan"));
    }

    template <class Int_T>
    void testIntegerToStr()
    {
        using namespace DFG_ROOT_NS;
        char buffer[32];
        DFG_MODULE_NS(str)::toStr((std::numeric_limits<Int_T>::max)(),  buffer);
        EXPECT_EQ((std::numeric_limits<Int_T>::max)(), DFG_MODULE_NS(str)::strTo<Int_T>(buffer));
        DFG_MODULE_NS(str)::toStr((std::numeric_limits<Int_T>::min)(), buffer);
        EXPECT_EQ((std::numeric_limits<Int_T>::min)(), DFG_MODULE_NS(str)::strTo<Int_T>(buffer));
    }
} // unnamed namespace

TEST(dfgStr, toStr)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);

    EXPECT_EQ("-1", DFG_SUB_NS_NAME(str)::toStrC(-1));
    EXPECT_EQ("123456789", DFG_SUB_NS_NAME(str)::toStrC(123456789.0));
    EXPECT_EQ("123456789.012345", DFG_SUB_NS_NAME(str)::toStrC(123456789.012345));
    EXPECT_EQ("-123456789.01234567", DFG_SUB_NS_NAME(str)::toStrC(-123456789.01234567));
    EXPECT_EQ("0.3", DFG_SUB_NS_NAME(str)::toStrC(0.3));
    EXPECT_EQ("5896249", DFG_SUB_NS_NAME(str)::toStrC(5896249));

    // Floating point tests
    {
#if (DFG_MSVC_VER != 0 && DFG_MSVC_VER < DFG_MSVC_VER_2015) || defined(__MINGW32__)
        const char szFloatMin[]         = "-3.40282347e+038";
        const char szFloatMax[]         = "3.40282347e+038";
        const char szFloatMinPositive[] = "1.17549435e-038";
#else
        const char szFloatMin[]         = "-3.40282347e+38";
        const char szFloatMax[]         = "3.40282347e+38";
        const char szFloatMinPositive[] = "1.17549435e-38";
#endif
        toStrCommonFloatingPointTests<float>(szFloatMin, szFloatMax, szFloatMinPositive);
        toStrCommonFloatingPointTests<double>("-1.7976931348623157e+308", "1.7976931348623157e+308", "2.2250738585072014e-308");
        toStrCommonFloatingPointTests<long double>(nullptr, nullptr, nullptr);
    }

    // Test that integer overloads work correctly
    {
        testIntegerToStr<short>();
        testIntegerToStr<unsigned short>();
        testIntegerToStr<int>();
        testIntegerToStr<unsigned int>();
        testIntegerToStr<long>();
        testIntegerToStr<unsigned long>();
        testIntegerToStr<long long>();
        testIntegerToStr<unsigned long long>();

        testIntegerToStr<int16>();
        testIntegerToStr<uint16>();
        testIntegerToStr<int32>();
        testIntegerToStr<uint32>();
        testIntegerToStr<int64>();
        testIntegerToStr<uint64>();
    }
}

namespace
{
    struct RadixAndExpectedResult
    {
        RadixAndExpectedResult(int r, const char* pszExpected) :
            m_radix(r),
            m_pszExpected(pszExpected)
        {}

        int m_radix;
        const char* m_pszExpected;
    };

    char toDigitCharFunc(const size_t i) { return "0123456789abcdefghijklmnopqrstuvwxyz"[i]; }

    template <class T, size_t N>
    void testIntToRadixRepresentationWithSingleValue(const T val, const std::array<RadixAndExpectedResult, N>& testItems)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(str);
        
        char buffer[128] = "";
        wchar_t wbuffer[128] = L"";
        for (auto iter = testItems.cbegin(); iter != testItems.cend(); ++iter)
        {
            const auto radix = iter->m_radix;
            DFG_MODULE_NS(str)::itoaSz(val, radix, buffer, DFG_COUNTOF(buffer));
            EXPECT_STREQ(iter->m_pszExpected, buffer);
            EXPECT_EQ(iter->m_pszExpected, DFG_MODULE_NS(str)::intToRadixRepresentation<char>(val, radix, toDigitCharFunc, '-').second);

            // The same but with wchar_t
            DFG_MODULE_NS(str)::itoaSz(val, radix, wbuffer, DFG_COUNTOF(wbuffer));
            EXPECT_EQ(iter->m_pszExpected, DFG_MODULE_NS(utf)::codePointsToUtf8(makeSzRange(wbuffer)));
            EXPECT_EQ(iter->m_pszExpected, DFG_MODULE_NS(utf)::codePointsToUtf8(DFG_MODULE_NS(str)::intToRadixRepresentation<wchar_t>(val, radix, toDigitCharFunc, '-').second));
        }
    }

    template <class Int_T>
    void convertAndTest(const Int_T val, const std::vector<DFG_ROOT_NS::uint16>& expected)
    {
        using namespace DFG_ROOT_NS;
        const auto base4096charFunc = [](const size_t i) { return static_cast<::DFG_ROOT_NS::uint16>(i); };
        std::vector<uint16> buffer(32);
        const auto negativeIndicator = NumericTraits<uint16>::maxValue;
        const auto size = DFG_MODULE_NS(str)::intToRadixRepresentation(val, 4096, base4096charFunc, negativeIndicator, buffer.begin(), buffer.end());
        if (size >= 0)
            buffer.resize(static_cast<size_t>(size));
        EXPECT_EQ(expected, buffer);
    }
}


TEST(dfgStr, intToRadixRepresentation)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);
    using namespace DFG_MODULE_NS(cont);
    typedef RadixAndExpectedResult RE;

    testIntToRadixRepresentationWithSingleValue(int(0), makeArray(RE(2, "0"), RE(3, "0"), RE(10, "0"), RE(16, "0"), RE(36, "0") ));
    testIntToRadixRepresentationWithSingleValue(int(10), makeArray(RE(2, "1010"), RE(3, "101"), RE(10, "10"), RE(16, "a"), RE(36, "a")));
    testIntToRadixRepresentationWithSingleValue(int(-10), makeArray(RE(2, "-1010"), RE(3, "-101"), RE(10, "-10"), RE(16, "-a"), RE(36, "-a")));

    testIntToRadixRepresentationWithSingleValue(NumericTraits<int8>::maxValue, makeArray(RE(2, "1111111"), RE(3, "11201"), RE(10, "127"), RE(16, "7f"), RE(36, "3j")));
    testIntToRadixRepresentationWithSingleValue(NumericTraits<int8>::minValue, makeArray(RE(2, "-10000000"), RE(3, "-11202"), RE(10, "-128"), RE(16, "-80"), RE(36, "-3k")));
    testIntToRadixRepresentationWithSingleValue(NumericTraits<uint8>::maxValue, makeArray(RE(2, "11111111"), RE(3, "100110"), RE(10, "255"), RE(16, "ff"), RE(36, "73")));

    testIntToRadixRepresentationWithSingleValue(NumericTraits<int16>::maxValue, makeArray(RE(2, "111111111111111"), RE(3, "1122221121"), RE(10, "32767"), RE(16, "7fff"), RE(36, "pa7")));
    testIntToRadixRepresentationWithSingleValue(NumericTraits<int16>::minValue, makeArray(RE(2, "-1000000000000000"), RE(3, "-1122221122"), RE(10, "-32768"), RE(16, "-8000"), RE(36, "-pa8")));
    testIntToRadixRepresentationWithSingleValue(NumericTraits<uint16>::maxValue, makeArray(RE(2, "1111111111111111"), RE(3, "10022220020"), RE(10, "65535"), RE(16, "ffff"), RE(36, "1ekf")));

    testIntToRadixRepresentationWithSingleValue(NumericTraits<int32>::maxValue, makeArray(RE(2, "1111111111111111111111111111111"), RE(3, "12112122212110202101"), RE(10, "2147483647"), RE(16, "7fffffff"), RE(36, "zik0zj")));
    testIntToRadixRepresentationWithSingleValue(NumericTraits<int32>::minValue, makeArray(RE(2, "-10000000000000000000000000000000"), RE(3, "-12112122212110202102"), RE(10, "-2147483648"), RE(16, "-80000000"), RE(36, "-zik0zk")));
    testIntToRadixRepresentationWithSingleValue(NumericTraits<uint32>::maxValue, makeArray(RE(2, "11111111111111111111111111111111"), RE(3, "102002022201221111210"), RE(10, "4294967295"), RE(16, "ffffffff"), RE(36, "1z141z3")));

    testIntToRadixRepresentationWithSingleValue(NumericTraits<int64>::maxValue, makeArray(RE(2, "111111111111111111111111111111111111111111111111111111111111111"), RE(3, "2021110011022210012102010021220101220221"), RE(10, "9223372036854775807"), RE(16, "7fffffffffffffff"), RE(36, "1y2p0ij32e8e7")));
    testIntToRadixRepresentationWithSingleValue(NumericTraits<int64>::minValue, makeArray(RE(2, "-1000000000000000000000000000000000000000000000000000000000000000"), RE(3, "-2021110011022210012102010021220101220222"), RE(10, "-9223372036854775808"), RE(16, "-8000000000000000"), RE(36, "-1y2p0ij32e8e8")));
    testIntToRadixRepresentationWithSingleValue(NumericTraits<uint64>::maxValue, makeArray(RE(2, "1111111111111111111111111111111111111111111111111111111111111111"), RE(3, "11112220022122120101211020120210210211220"), RE(10, "18446744073709551615"), RE(16, "ffffffffffffffff"), RE(36, "3w5e11264sgsf")));

    {
        char buffer[128];
        EXPECT_EQ(ItoaError_badRadix, intToRadixRepresentation<char>(0, 0, toDigitCharFunc, '-').first);
        EXPECT_EQ(ItoaError_badRadix, intToRadixRepresentation<char>(0, 1, toDigitCharFunc, '-').first);
        EXPECT_EQ(1, intToRadixRepresentation(0, 2, toDigitCharFunc, '-', buffer, buffer + 1));
        EXPECT_EQ('0', buffer[0]);
        EXPECT_EQ(1, intToRadixRepresentation(1, 2, toDigitCharFunc, '-', buffer, buffer + 1));
        EXPECT_EQ('1', buffer[0]);
        EXPECT_EQ(ItoaError_bufferTooSmall, intToRadixRepresentation(-1, 2, toDigitCharFunc, '-', buffer, buffer + 1));
        EXPECT_EQ(ItoaError_emptyOutputBuffer, intToRadixRepresentation(2, 2, toDigitCharFunc, '-', buffer, buffer));
        EXPECT_EQ(ItoaError_bufferTooSmall, intToRadixRepresentation(2, 2, toDigitCharFunc, '-', buffer, buffer + 1));
    }

    // Test 4096-base 
    {
        using namespace DFG_MODULE_NS(math);

        convertAndTest(0, makeVector<uint16>(0));
        convertAndTest(4095, makeVector<uint16>(4095));
        convertAndTest(4096, makeVector<uint16>(1, 0));
        convertAndTest(pow2ToXCt<16>::value, makeVector<uint16>(16, 0));
        convertAndTest(pow2ToXCt<24>::value - 1, makeVector<uint16>(4095, 4095));
        convertAndTest(pow2ToXCt<24>::value, makeVector<uint16>(1, 0, 0));
        convertAndTest((uint64(1) << 36) - 1, makeVector<uint16>(4095, 4095, 4095));
        convertAndTest(-1 * (int64(1) << 36), makeVector<uint16>(uint16(NumericTraits<uint16>::maxValue), 1, 0, 0, 0));
        convertAndTest(uint64(1) << 36, makeVector<uint16>(1, 0, 0, 0));
        convertAndTest(NumericTraits<uint64>::maxValue, makeVector<uint16>(15, 4095, 4095, 4095, 4095, 4095));
    }
}

namespace
{
    void ReadOnlySzParamOverloadTest(DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlySzParamC) sC)
    {
        std::string s = sC.c_str();
    }
    void ReadOnlySzParamOverloadTest(DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlySzParamW) sW)
    {
        std::wstring s = sW.c_str(); 
    }

    void ReadOnlySzParamWithSizeOverloadTest(DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlySzParamWithSizeC) sC)
    {
        std::string s = sC.c_str();
    }
    void ReadOnlySzParamWithSizeOverloadTest(DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlySzParamWithSizeW) sW)
    {
        std::wstring s = sW.c_str();
    }
}

TEST(dfgStr, ReadOnlySzParam)
{
    using namespace DFG_ROOT_NS;
    const auto func0 = [](DFG_CLASS_NAME(ReadOnlySzParamC) s)
    {
        return DFG_ROOT_NS::DFG_SUB_NS_NAME(str)::strTo<size_t>(s);
    };
    const auto func1 = [](DFG_CLASS_NAME(ReadOnlySzParamW) s)
    {
        return DFG_ROOT_NS::DFG_SUB_NS_NAME(str)::strTo<size_t>(s);
    };
    const auto func2 = [](DFG_CLASS_NAME(ReadOnlySzParamWithSizeC) s)
    {
        return DFG_ROOT_NS::DFG_SUB_NS_NAME(str)::strTo<size_t>(s) + s.length();
    };
    const auto func3 = [](DFG_CLASS_NAME(ReadOnlySzParamWithSizeW) s)
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

    ReadOnlySzParamOverloadTest("test");
    ReadOnlySzParamOverloadTest(L"test");
    ReadOnlySzParamOverloadTest(std::string("test"));
    ReadOnlySzParamOverloadTest(std::wstring(L"test"));

    ReadOnlySzParamWithSizeOverloadTest("test");
    ReadOnlySzParamWithSizeOverloadTest(L"test");
    ReadOnlySzParamWithSizeOverloadTest(std::string("test"));
    ReadOnlySzParamWithSizeOverloadTest(std::wstring(L"test"));
}

namespace
{
    template <class Str_T>
    void TestTypedString()
    {
        using namespace DFG_ROOT_NS;
        typedef typename Str_T::SzPtrR SzPtrType;

        {
            Str_T s(SzPtrType("abc"));
            DFGTEST_STATIC((std::is_same<typename Str_T::SzPtrR, decltype(s.c_str())>::value));
            EXPECT_TRUE(s == SzPtrType("abc"));
            EXPECT_FALSE(s != SzPtrType("abc"));
            EXPECT_FALSE(s == SzPtrType("abc2"));
            EXPECT_TRUE(s != SzPtrType("abc2"));
            EXPECT_TRUE(s == s);
            EXPECT_FALSE(s != s);
            EXPECT_FALSE(s < s);
            EXPECT_FALSE(s.c_str() < s);
            EXPECT_FALSE(s < s.c_str());
        }

        // Test moving
        {
            const char szData[] = "0123456789abcdefghijklmn0123456789abcdefghijklmn0123456789abcdefghijklmn0123456789abcdefghijklmn";
            Str_T a = Str_T(SzPtrType(szData));
            const auto pData = a.rawStorage().data();
            Str_T b(std::move(a));
            EXPECT_TRUE(a.empty());
            Str_T c;
            c = std::move(b);
            EXPECT_TRUE(b.empty());
            Str_T d = c;
            Str_T e;
            e = d;
            EXPECT_EQ(SzPtrType(szData), c);
            EXPECT_EQ(c, d);
            EXPECT_EQ(c, e);
            // Test that the actual data location hasn't changed; note that the input string must long enough to avoid SSO-effects.
            EXPECT_TRUE(pData == c.rawStorage().data());
        }

        // Testing construction with fromRawString()
        {
            const char sz[] = "abc";
            const auto s = Str_T::fromRawString(std::string(sz, sz + 2));
            const auto s2 = Str_T::fromRawString(sz, sz + 2);
            EXPECT_EQ(SzPtrType("ab"), s);
            EXPECT_EQ(SzPtrType("ab"), s2);
            EXPECT_EQ(s, s2);
        }

        // Testing raw iterator access
        {
            Str_T s(SzPtrType("abc"));
            EXPECT_EQ('a', *s.beginRaw());
            EXPECT_EQ(s.beginRaw() + 3, s.endRaw());
        }
    }
}

TEST(dfgStr, String)
{
    using namespace DFG_ROOT_NS;
    TestTypedString<DFG_CLASS_NAME(StringAscii)>();
    TestTypedString<DFG_CLASS_NAME(StringLatin1)>();
    TestTypedString<DFG_CLASS_NAME(StringUtf8)>();

    // Test that can compare e.g. StringUtf8 and SzPtrAsciiR
    {
        SzPtrAsciiR tpszAscii = DFG_ASCII("abc");
        EXPECT_TRUE(DFG_CLASS_NAME(StringUtf8)(tpszAscii) == tpszAscii);
        EXPECT_FALSE(DFG_CLASS_NAME(StringUtf8)(tpszAscii) != tpszAscii);
    }

    // Test that can compare TypedString and string views.
    {
        DFG_CLASS_NAME(StringUtf8) s(DFG_ASCII("abc"));
        EXPECT_TRUE(s == s);
        EXPECT_TRUE(s == StringViewUtf8(s));
        EXPECT_TRUE(s == StringViewSzUtf8(s));
        //EXPECT_TRUE(s == StringViewAscii(DFG_ASCII("abc")));    // TODO: make this work.
        //EXPECT_TRUE(s == StringViewSzAscii(DFG_ASCII("abc")));  // TODO: make this work.
    }
}

namespace
{
    template <class StringView_T>
    void TestStringViewIndexAccess(StringView_T sv, std::true_type)
    {
        // Note: Code below expects sv == "abc".
        typedef decltype(sv.front()) CodePointT;

        ASSERT_EQ(3, sv.size());
        EXPECT_EQ(CodePointT('a'), sv.front());
        EXPECT_EQ(CodePointT('a'), sv[0]);
        EXPECT_EQ(CodePointT('c'), sv.back());

        sv.pop_front();
        sv.pop_back();

        EXPECT_EQ(CodePointT('b'), sv.front());
        EXPECT_EQ(CodePointT('b'), sv[0]);
        EXPECT_EQ(CodePointT('b'), sv.back());

        sv.clear();
        EXPECT_TRUE(sv.empty());
    }

    template <class StringView_T>
    void TestStringViewIndexAccess(const StringView_T&, std::false_type)
    {
        // Nothing to do here.
    }

    template <class StringView_T, class Char_T, class Str_T, bool TestIndexAccess, class RawToTypedConv>
    void TestStringViewImpl(RawToTypedConv conv, const bool bTestNonSzViews = true)
    {
        using namespace DFG_ROOT_NS;
        const Char_T szEmpty[] = { '\0' };
        const Char_T sz[]  = { 'a', 'b', 'c', '\0' };
        const Char_T sz2[] = { 'b', 'c', 'a', '\0' };
        const Str_T s(conv(sz));

        StringView_T view(conv(sz));
        StringView_T view2Chars(conv(&sz[0] + 1), 2);
        EXPECT_EQ(conv(&sz[1]), view2Chars.begin());
        EXPECT_STREQ(&sz[1], toCharPtr_raw(view2Chars.begin()));
        EXPECT_EQ(2, view2Chars.length());
        StringView_T view2(s);
        EXPECT_EQ(s.c_str(), view2.begin());

        // Test empty()
        EXPECT_TRUE(StringView_T(conv(szEmpty)).empty());

        // Test operator==
        EXPECT_TRUE(StringView_T(conv(szEmpty)) == StringView_T(conv(szEmpty)));
        EXPECT_TRUE(view == view2);

        // Test toString
        {
            const auto sFromToString = view.toString();
            EXPECT_TRUE(sFromToString == view);
        }
        
        EXPECT_TRUE(StringView_T(Str_T()) == StringView_T(Str_T()));
        if (bTestNonSzViews)
        { 
            EXPECT_TRUE(StringView_T(conv(sz + 1), 2) == StringView_T(conv(sz2), 2));
            EXPECT_FALSE(StringView_T(conv(sz + 1), 2) == StringView_T(conv(sz2), 3));
            EXPECT_FALSE(StringView_T(conv(sz), 2) == StringView_T(conv(sz2), 2));
        }
        
        EXPECT_TRUE(view == s);
        EXPECT_FALSE(view != s);
        EXPECT_TRUE(s == view);
        EXPECT_FALSE(s != view);
        EXPECT_TRUE(view == s.c_str());
        EXPECT_FALSE(view != s.c_str());
        EXPECT_TRUE(s.c_str() == view);
        EXPECT_FALSE(s.c_str() != view);
        EXPECT_FALSE(StringView_T(conv(sz)) == StringView_T(conv(sz2)));
        EXPECT_TRUE(StringView_T(conv(sz)) != StringView_T(conv(sz2)));

        // Test operator<
        {
            // TODO: Better tests.
            EXPECT_EQ(s < Str_T(view.begin(), view.end()), s < view);
            EXPECT_EQ(s < Str_T(view2Chars.begin(), view2Chars.end()), s < view2Chars);
        }

        // Test data()
        {
            EXPECT_EQ(view.begin(), view.data());
            EXPECT_EQ(view2.begin(), view2.data());
        }

        //DFG_CLASS_NAME(StringViewUtf8)() == DFG_ASCII("a"); // TODO: make this work

        TestStringViewIndexAccess(view, std::integral_constant<bool, TestIndexAccess>());
    }

}

TEST(dfgStr, StringView)
{
    using namespace DFG_ROOT_NS;

    TestStringViewImpl<DFG_CLASS_NAME(StringViewC), char, std::string, true>([](const char* psz) {return psz; });
    TestStringViewImpl<DFG_CLASS_NAME(StringViewW), wchar_t, std::wstring, true>([](const wchar_t* psz) {return psz; });

    TestStringViewImpl<DFG_CLASS_NAME(StringViewAscii), char, StringAscii, true>([](const char* psz) {return DFG_ROOT_NS::SzPtrAscii(psz); });
    TestStringViewImpl<DFG_CLASS_NAME(StringViewLatin1), char, StringLatin1, true>([](const char* psz) {return DFG_ROOT_NS::SzPtrLatin1(psz); });
    TestStringViewImpl<DFG_CLASS_NAME(StringViewUtf8), char, StringUtf8, false>([](const char* psz) {return DFG_ROOT_NS::SzPtrUtf8(psz); });

    // Test that StringViewUtf8 accepts SzPtrAscii.
    {
        auto tpszAscii = SzPtrAscii("abc");
        const DFG_CLASS_NAME(StringViewUtf8)& svUtf8 = tpszAscii;
        DFG_UNUSED(svUtf8);
    }

    // Testing that view can be constructed from (typed) pointer pair
    {
        const char sz[] = "abc";
        DFG_CLASS_NAME(StringViewC) svc(sz, sz + 2);
        DFG_CLASS_NAME(StringViewAscii) sva(SzPtrAscii(sz), SzPtrAscii(sz + 2));
        DFG_CLASS_NAME(StringViewUtf8) svu(SzPtrUtf8(sz), SzPtrUtf8(sz + 2));
        EXPECT_TRUE("ab" == svc);
        EXPECT_TRUE(SzPtrAscii("ab") == sva);
        EXPECT_TRUE(DFG_UTF8("ab") == svu);
    }

    // Test that can compare views that have compare-compatible types.
    // TODO: make this work.
    /*
    {
        EXPECT_TRUE(StringViewAscii(DFG_ASCII("abc")) == StringViewUtf8(DFG_ASCII("abc")));
        EXPECT_TRUE(StringViewUtf8(DFG_ASCII("abc")) == StringViewAscii(DFG_ASCII("abc")));
    }
    */

    // TODO: Test that StringViewUtf8 accepts StringAscii.
    /*
    {
        DFG_CLASS_NAME(StringAscii) sAscii(DFG_ASCII("a"));
        const DFG_CLASS_NAME(StringViewUtf8)& svUtf8 = sAscii;
        DFG_UNUSED(svUtf8);
    }
    */
}

namespace
{
    template <class StringView_T, class Char_T, class Str_T, class RawToTypedConv>
    void TestStringViewSzImpl(RawToTypedConv conv)
    {
        using namespace DFG_ROOT_NS;

        TestStringViewImpl<StringView_T, Char_T, Str_T, false>(conv, false);

        const Char_T sz0[] = { 'a', 'b', 'c', '\0' };
        const Char_T sz1[] = { 'a', 'b', 'c', 'd', '\0' };
        const Char_T sz2[] = { 'a', 'b', 'd', '\0' };
        StringView_T szView(conv(sz0));
        EXPECT_EQ(conv(sz0), szView.c_str());             // Test existence of c_str()
        EXPECT_TRUE(szView == conv(sz0));
        EXPECT_TRUE(conv(sz0) == szView);
        EXPECT_FALSE(szView == conv(sz1));
        EXPECT_FALSE(szView == conv(sz2));
        EXPECT_EQ(DFG_DETAIL_NS::gnStringViewSzSizeNotCalculated, szView.m_nSize); // This tests implementation detail, not interface.
        EXPECT_EQ(3, szView.length());
        EXPECT_EQ(3, szView.m_nSize); // This tests implementation detail, not interface.
        EXPECT_TRUE(szView == conv(sz0));
        EXPECT_TRUE(conv(sz0) == szView);
        EXPECT_FALSE(szView == conv(sz1));
        EXPECT_FALSE(szView == conv(sz2));
        EXPECT_TRUE(szView != conv(sz2));
        EXPECT_FALSE(szView != conv(sz0));
    }
}

TEST(dfgStr, StringViewSz)
{
    using namespace DFG_ROOT_NS;

    TestStringViewSzImpl<DFG_CLASS_NAME(StringViewSzC), char, std::string>([](const char* psz)          { return psz; });
    TestStringViewSzImpl<DFG_CLASS_NAME(StringViewSzW), wchar_t, std::wstring>([](const wchar_t* psz)   { return psz; });

    TestStringViewSzImpl<DFG_CLASS_NAME(StringViewSzAscii), char, StringAscii>([](const char* psz)      { return DFG_ROOT_NS::SzPtrAscii(psz); });
    TestStringViewSzImpl<DFG_CLASS_NAME(StringViewSzLatin1), char, StringLatin1>([](const char* psz)    { return DFG_ROOT_NS::SzPtrLatin1(psz); });
    TestStringViewSzImpl<DFG_CLASS_NAME(StringViewSzUtf8), char, StringUtf8>([](const char* psz)        { return DFG_ROOT_NS::SzPtrUtf8(psz); });

    // Test that can compare views that have compare-compatible types.
    // TODO: make this work.
    /*
    {
        EXPECT_TRUE(StringViewSzAscii(DFG_ASCII("abc")) == StringViewSzUtf8(DFG_ASCII("abc")));
        EXPECT_TRUE(StringViewSzUtf8(DFG_ASCII("abc")) == StringViewSzAscii(DFG_ASCII("abc")));
    }
    */
}

namespace
{
    template <class StringView_T, class UntypedStringView_T>
    static void StringView_autoConvToUntyped_impl(const StringView_T& sv)
    {
        using namespace DFG_ROOT_NS;
        UntypedStringView_T svUt(sv);
        EXPECT_STREQ(toCharPtr_raw(sv.toString().c_str()), toCharPtr_raw(svUt.toString().c_str()));
    }
}

TEST(dfgStr, StringView_autoConvToUntyped)
{
    using namespace DFG_ROOT_NS;
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewC), DFG_CLASS_NAME(StringViewC)>("abc");
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzC), DFG_CLASS_NAME(StringViewSzC)>("abc");
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzC), DFG_CLASS_NAME(StringViewC)>("abc");

    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewAscii), DFG_CLASS_NAME(StringViewC)>(DFG_ASCII("abc"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzAscii), DFG_CLASS_NAME(StringViewSzC)>(DFG_ASCII("abc"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzAscii), DFG_CLASS_NAME(StringViewC)>(DFG_ASCII("abc"));

    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewLatin1), DFG_CLASS_NAME(StringViewC)>(SzPtrLatin1("abc"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzLatin1), DFG_CLASS_NAME(StringViewSzC)>(SzPtrLatin1("abc"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzLatin1), DFG_CLASS_NAME(StringViewC)>(SzPtrLatin1("abc"));

    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewUtf8), DFG_CLASS_NAME(StringViewC)>(DFG_UTF8("ab\xC3" "\xA4" "c"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzUtf8), DFG_CLASS_NAME(StringViewSzC)>(DFG_UTF8("ab\xC3" "\xA4" "c"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzUtf8), DFG_CLASS_NAME(StringViewC)>(DFG_UTF8("ab\xC3" "\xA4" "c"));

    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewW), DFG_CLASS_NAME(StringViewW)>(L"abc");
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzW), DFG_CLASS_NAME(StringViewSzW)>(L"abc");
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzW), DFG_CLASS_NAME(StringViewW)>(L"abc");
}

namespace
{
    template <class View_T, class Owned_T>
    static void testStringViewOrOwnerImpl()
    {
        using CharT = typename View_T::CharT;
        {
            auto svo = DFG_ROOT_NS::StringViewOrOwner<View_T, Owned_T>::makeView(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc"));
            EXPECT_EQ(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc"), svo);
            EXPECT_FALSE(svo.isOwner());
        }

        {
            const CharT szLiteral[] = { 'a', 'b', 'c', '\0' };
            auto svo = DFG_ROOT_NS::StringViewOrOwner<View_T, Owned_T>::makeView(szLiteral);
            EXPECT_EQ(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc"), svo);
            EXPECT_FALSE(svo.isOwner());
            EXPECT_EQ(szLiteral, svo.data());
        }

        // Testing makeOwned() with both small and long string (i.e. one with expected SSO-buffer and one with allocated buffer)
        for (int i = 0; i < 2; ++i)
        {
            const CharT* literals[] = { DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc"), DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "0123456789012345678901234567890123456789") };
            const auto& pszTest = literals[i];
            auto svo = DFG_ROOT_NS::StringViewOrOwner<View_T, Owned_T>::makeOwned(pszTest);
            EXPECT_EQ(pszTest, svo);
            EXPECT_TRUE(svo.isOwner());
            auto s = svo.release();
            EXPECT_TRUE(svo.owned().empty());
            EXPECT_EQ(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, ""), svo);
            EXPECT_EQ(pszTest, s);
        }
    }

    template <class View_T, class Owned_T, class RawToTypedConv_T>
    static void testStringViewOrOwnerTyped(RawToTypedConv_T conv)
    {
        using namespace DFG_ROOT_NS;
        using CharT = typename View_T::CharT;
        {
            auto svo = StringViewOrOwner<View_T, Owned_T>::makeView(conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc")));
            EXPECT_TRUE(conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc")) == svo);
            EXPECT_FALSE(svo.isOwner());
        }

        // Testing makeOwned() with both small and long string (i.e. one with expected SSO-buffer and one with allocated buffer)
        for (int i = 0; i < 2; ++i)
        {
            using PtrT = decltype(conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc")));
            const PtrT literals[] = { conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc")), conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "0123456789012345678901234567890123456789")) };
            const auto& pszTest = literals[i];
            auto svo = StringViewOrOwner<View_T, Owned_T>::makeOwned(Owned_T(pszTest));
            EXPECT_TRUE(pszTest == svo);
            EXPECT_TRUE(svo.isOwner());
            auto s = svo.release();
            EXPECT_TRUE(svo.owned().empty());
            EXPECT_TRUE(conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "")) == svo);
            EXPECT_TRUE(pszTest == s);
        }
    }
}

TEST(dfgStr, StringViewOrOwner)
{
    using namespace DFG_ROOT_NS;
    testStringViewOrOwnerImpl<DFG_CLASS_NAME(StringViewC), std::string>();
    testStringViewOrOwnerImpl<DFG_CLASS_NAME(StringViewSzC), std::string>();
    testStringViewOrOwnerImpl<DFG_CLASS_NAME(StringViewW), std::wstring>();
    testStringViewOrOwnerImpl<DFG_CLASS_NAME(StringViewSzW), std::wstring>();

    testStringViewOrOwnerTyped<DFG_CLASS_NAME(StringViewAscii), DFG_CLASS_NAME(StringAscii)>([](const char* psz) { return SzPtrAscii(psz); });
    testStringViewOrOwnerTyped<DFG_CLASS_NAME(StringViewLatin1), DFG_CLASS_NAME(StringLatin1)>([](const char* psz) { return SzPtrLatin1(psz); });
    testStringViewOrOwnerTyped<DFG_CLASS_NAME(StringViewUtf8), DFG_CLASS_NAME(StringUtf8)>([](const char* psz) { return SzPtrUtf8(psz); });
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
        EXPECT_TRUE(isValidHexStr(s));
        hexStrToBytes(s.c_str(), &v2[0], v2.size());
        EXPECT_EQ(memcmp(&v[0], &v2[0], v.size()), 0);
    }

    int32 b[10];
    char b2[sizeof(int32)*10];
    // +1 in int32_min is for VC2010 buggy uniform_int_distribution, for which the workaround does not available in all cases.
    forEachFwd(b, [&](int& v){v = DFG_MODULE_NS(rand)::rand(re, DFG_ROOT_NS::int32_min+1, DFG_ROOT_NS::int32_max); });
    auto str = bytesToHexStr(b, sizeof(b));
    EXPECT_TRUE(isValidHexStr(str));
    hexStrToBytes(str.c_str(), b2, sizeof(b2));
    EXPECT_EQ(memcmp(b, b2, sizeof(b)), 0);

    // Test that reading hex string that has letters works regardless of the case.
    {
        const char szHexUpper[] = { 55, 65, 48, 66, 54, 70,'\0'};       // 7A0B6F
        const char szHexLower[] = { 55, 97, 48, 98, 54, 102, '\0' };    // 7a0b6f
        const char szHexMixed[] = { 55, 65, 48, 98, 54, 102, '\0' };    // 7A0b6f
        const char szHexExpected[] = { (7 << 4) + 0xa, 0xb, (6 << 4) + 0xf, '\0' };
        std::string sUpper(3, '\0');
        std::string sLower(3, '\0');
        std::string sMixed(3, '\0');
        
        hexStrToBytes(szHexUpper, ptrToContiguousMemory(sUpper), sUpper.size());
        hexStrToBytes(szHexLower, ptrToContiguousMemory(sLower), sLower.size());
        hexStrToBytes(szHexMixed, ptrToContiguousMemory(sMixed), sMixed.size());
        EXPECT_TRUE(isValidHexStr(szHexUpper));
        EXPECT_TRUE(isValidHexStr(szHexLower));
        EXPECT_TRUE(isValidHexStr(szHexMixed));
        EXPECT_EQ(szHexExpected, sUpper);
        EXPECT_EQ(szHexExpected, sLower);
        EXPECT_EQ(szHexExpected, sMixed);
    }

    // Test that less-than-needed output buffer is not overwritten.
    {
        const char szHexUpper[] = "aaaa";
        std::string s(1, '\0');
        hexStrToBytes(szHexUpper, ptrToContiguousMemory(s), s.size());
        EXPECT_EQ(0xAA, static_cast<unsigned char>(s[0]));
        EXPECT_EQ('\0', *(s.data() + 1));
    }

    // isValidHexStr tests
    {
        EXPECT_FALSE(isValidHexStr("abc"));
        EXPECT_FALSE(isValidHexStr("abcr"));
        EXPECT_FALSE(isValidHexStr("ffbbccdd88\t"));
        EXPECT_FALSE(isValidHexStr("ffbbccdd88F\t"));
        EXPECT_FALSE(isValidHexStr("gf"));
    }
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

namespace
{
    template <class Char_T>
    void testEscaped(const Char_T* psz, const Char_T val)
    {
        using namespace DFG_MODULE_NS(str);
        std::basic_string<Char_T> s(psz);
        EXPECT_TRUE(s.size() >= 2);
        auto rv = stringLiteralCharToValue<Char_T>(s);
        EXPECT_TRUE(rv.first);
        EXPECT_EQ(val, rv.second);
    }
}

TEST(dfgStr, stringLiteralCharToValue)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);

    testEscaped("\\a", '\a');
    testEscaped("\\b", '\b');
    testEscaped("\\f", '\f');
    testEscaped("\\n", '\n');
    testEscaped("\\r", '\r');
    testEscaped("\\t", '\t');
    testEscaped("\\v", '\v');
    testEscaped("\\'", '\'');
    testEscaped("\\\"", '\"');
    testEscaped("\\\\", '\\');
    testEscaped("\\\?", '\?');
    testEscaped("\\0", '\0');

    for (int c = 0; c < 255; ++c)
    {
        std::string s(1, static_cast<char>(c));
        auto vals = stringLiteralCharToValue<char>(s);
        auto valu = stringLiteralCharToValue<uint8>(s);
        EXPECT_TRUE(vals.first);
        EXPECT_EQ(c < 128, valu.first);
        EXPECT_EQ(static_cast<char>(c), vals.second);
        if (valu.first)
            EXPECT_EQ(c, valu.second);
    }

    auto rv0 = stringLiteralCharToValue<uint8>("\\xff");
    EXPECT_TRUE(rv0.first);
    EXPECT_EQ(0xff, rv0.second);

    auto rv1 = stringLiteralCharToValue<int>("\\xffff");
    EXPECT_TRUE(rv1.first);
    EXPECT_EQ(0xffff, rv1.second);

    auto rv2 = stringLiteralCharToValue<uint32>("\\xffffffff");
    EXPECT_TRUE(rv2.first);
    EXPECT_EQ(0xffffffff, rv2.second);

    auto rv3 = stringLiteralCharToValue<int>("\\033");
    EXPECT_TRUE(rv3.first);
    EXPECT_EQ(033, rv3.second);

    auto rv4 = stringLiteralCharToValue<char>(L"\xff");
    EXPECT_FALSE(rv4.first);

    auto rv5 = stringLiteralCharToValue<char>(L"\x7f");
    EXPECT_TRUE(rv5.first);
    EXPECT_EQ(0x7f, rv5.second);
}

TEST(dfgStr, beginsWith)
{
    using namespace DFG_MODULE_NS(str);

    EXPECT_TRUE(beginsWith("abc", "a"));
    EXPECT_TRUE(beginsWith(std::string("abc"), std::string("a")));
    EXPECT_TRUE(beginsWith("abc", std::string("a")));
    EXPECT_TRUE(beginsWith(std::string("abc"), "a"));
    EXPECT_TRUE(beginsWith("abc", "ab"));
    EXPECT_TRUE(beginsWith("abc", "abc"));
    EXPECT_FALSE(beginsWith("abc", "b"));
    EXPECT_TRUE(beginsWith("abc", "")); // Searching for empty is to return true.
    EXPECT_TRUE(beginsWith("", "")); // Searching for empty is to return true.

    EXPECT_TRUE(beginsWith(L"abc", L"a"));
    EXPECT_TRUE(beginsWith(std::wstring(L"abc"), std::wstring(L"a")));
    EXPECT_TRUE(beginsWith(L"abc", std::wstring(L"a")));
    EXPECT_TRUE(beginsWith(std::wstring(L"abc"), L"a"));
}

TEST(dfgStr, beginsWith_TypedStrings)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);
    typedef DFG_CLASS_NAME(StringUtf8) SUtf8;
    SUtf8 a(DFG_UTF8("ab"));
    SUtf8 b(DFG_UTF8("a"));
    EXPECT_TRUE(beginsWith(a, b));
    EXPECT_FALSE(beginsWith(b, a));
}

namespace
{
    template <class StringView_T, class FromRawStringToTyped_T>
    void beginsWith_StringViews_impl()
    {
        using namespace DFG_MODULE_NS(str);
        typedef StringView_T Sv;
        typedef FromRawStringToTyped_T Conv;
        EXPECT_TRUE(beginsWith(Sv(Conv("ab")), Sv(Conv("a"))));
        EXPECT_TRUE(beginsWith(Sv(Conv("ab")), Sv(Conv(""))));
        EXPECT_FALSE(beginsWith(Sv(Conv("")), Sv(Conv("a"))));
        EXPECT_FALSE(beginsWith(Sv(Conv("a")), Sv(Conv("ab"))));
    }
}

TEST(dfgStr, beginsWith_StringViews)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);
    typedef DFG_CLASS_NAME(StringViewC) SvC;
    typedef DFG_CLASS_NAME(StringViewSzC) SvSzC;
    typedef DFG_CLASS_NAME(StringViewUtf8) SvUtf8;
    typedef DFG_CLASS_NAME(StringViewSzUtf8) SvSzUtf8;

    beginsWith_StringViews_impl<SvC, const char*>();
    beginsWith_StringViews_impl<SvSzC, const char*>();
    beginsWith_StringViews_impl<SvUtf8, SzPtrUtf8R>();
    beginsWith_StringViews_impl<SvSzUtf8, SzPtrUtf8R>();

    // Test StringViewSz and SzPtr mixes
    EXPECT_TRUE(beginsWith(SvSzC("ab"), "a"));
    EXPECT_TRUE(beginsWith("ab", SvSzC("a")));
    EXPECT_TRUE(beginsWith(SvSzUtf8(DFG_UTF8("ab")), DFG_UTF8("a")));
    EXPECT_TRUE(beginsWith(DFG_UTF8("ab"), SvSzUtf8(DFG_UTF8("a"))));

    // TODO: (Sz, nonSz) mixes won't compile.
    //EXPECT_TRUE(beginsWith(SvC("ab"), SvSzC("a")));   
}

TEST(dfgStr, format_fmt)
{
    using namespace DFG_ROOT_NS;

    EXPECT_EQ("1", format_fmt("{0}", 1));
    EXPECT_EQ("12", format_fmt("{0}{1}", 1, 2));
    EXPECT_EQ("123", format_fmt("{0}{1}{2}", 1, 2, 3));
    EXPECT_EQ("1234", format_fmt("{0}{1}{2}{3}", 1, 2, 3, 4));
    EXPECT_EQ("12345", format_fmt("{0}{1}{2}{3}{4}", 1, 2, 3, 4, 5));
    EXPECT_EQ("123456", format_fmt("{0}{1}{2}{3}{4}{5}", 1, 2, 3, 4, 5, 6));
    EXPECT_EQ("1234567", format_fmt("{0}{1}{2}{3}{4}{5}{6}", 1, 2, 3, 4, 5, 6, 7));
    EXPECT_EQ("12345678", format_fmt("{0}{1}{2}{3}{4}{5}{6}{7}", 1, 2, 3, 4, 5, 6, 7, 8));
    EXPECT_EQ("123456789", format_fmt("{0}{1}{2}{3}{4}{5}{6}{7}{8}", 1, 2, 3, 4, 5, 6, 7, 8, 9));

    EXPECT_TRUE(DFG_MODULE_NS(str)::beginsWith(format_fmt("{0}", float(123456789)), "1234567"));
    EXPECT_TRUE(DFG_MODULE_NS(str)::beginsWith(format_fmt("{0}", double(123456789)), "123456789"));
    EXPECT_TRUE(DFG_MODULE_NS(str)::beginsWith(format_fmt("{0}", (long double)(123456789)), "123456789"));

    // TODO: this should work
    //EXPECT_TRUE(DFG_MODULE_NS(str)::beginsWith(format_fmt("{0}", double(1.23456789e-9)), "1.23456789e"));
}

TEST(dfgStr, toCstr)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);
    char szNonConst[] = "";
    const char szConst[] = "";
    std::string s;
    std::wstring ws;
    StringAscii sA;
    StringLatin1 sL1;
    StringUtf8 sU8;

    EXPECT_EQ(szNonConst, toCstr(szNonConst));
    EXPECT_EQ(szConst, toCstr(szConst));
    EXPECT_EQ(s.c_str(), toCstr(s));
    EXPECT_EQ(ws.c_str(), toCstr(ws));
    EXPECT_EQ(sA.c_str(), toCstr(sA));
    EXPECT_EQ(sL1.c_str(), toCstr(sL1));
    EXPECT_EQ(sU8.c_str(), toCstr(sU8));

    EXPECT_EQ(SzPtrAscii(szConst), toCstr(SzPtrAscii(szConst)));
    EXPECT_EQ(SzPtrLatin1(szConst), toCstr(SzPtrLatin1(szConst)));
    EXPECT_EQ(SzPtrUtf8(szConst), toCstr(SzPtrUtf8(szConst)));
}

TEST(dfgStr, StringLiteralMacros)
{
    using namespace DFG_MODULE_NS(str);

    {
        EXPECT_EQ(97, DFG_U8_CHAR('a'));
        auto sz = DFG_ASCII("abc");
        EXPECT_EQ(97, sz.c_str()[0]);
        EXPECT_EQ(98, sz.c_str()[1]);
        EXPECT_EQ(99, sz.c_str()[2]);
    }

#if DFG_LANGFEAT_UNICODE_STRING_LITERALS
    {
        auto sz = DFG_UTF8("a\u00E4\u00F6");
        ASSERT_EQ(5, strLen(sz.c_str()));
        EXPECT_EQ(97,   sz.c_str()[0]);
        EXPECT_EQ(0xC3, static_cast<unsigned char>(sz.c_str()[1]));
        EXPECT_EQ(0xA4, static_cast<unsigned char>(sz.c_str()[2]));
        EXPECT_EQ(0xC3, static_cast<unsigned char>(sz.c_str()[3]));
        EXPECT_EQ(0xB6, static_cast<unsigned char>(sz.c_str()[4]));
    }
#endif
}

TEST(dfgStr, TypedCharPtrT)
{
    using namespace DFG_ROOT_NS;
    EXPECT_EQ('a', (*SzPtrAscii("abc")).toInt());
    EXPECT_EQ('a', (*SzPtrLatin1("abc")).toInt());
    //EXPECT_EQ('a', (*SzPtrUtf8("abc")).toInt()); // This should fail to compile.
    TypedCharPtrT<const char, CharPtrTypeChar> typedRawCharPtr("abc");
    EXPECT_EQ('a', (*typedRawCharPtr).toInt());
}

namespace
{
    template <class T>
    void testFloatingPointTypeToSprintfType()
    {
        char buffer[16];
        const T val = 0.25;
        DFG_MODULE_NS(str)::DFG_DETAIL_NS::sprintf_s(buffer,
            DFG_COUNTOF(buffer),
            (std::string("%") + DFG_MODULE_NS(str)::DFG_DETAIL_NS::floatingPointTypeToSprintfType<T>()).c_str(),
            val);
        EXPECT_STREQ("0.25", buffer);
    }
}

TEST(dfgStr, floatingPointTypeToSprintfType)
{
    testFloatingPointTypeToSprintfType<float>();
    testFloatingPointTypeToSprintfType<double>();
    testFloatingPointTypeToSprintfType<long double>();
}
