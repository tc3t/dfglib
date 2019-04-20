#include <stdafx.h>
#include <dfg/buildConfig.hpp>
#include <dfg/dfgBase.hpp>
#include <limits>
#include <string>
#include <tuple>
#include <dfg/ptrToContiguousMemory.hpp>
#include <array>
#include <vector>
#include <set>
#include <list>
#include <dfg/cont/arrayWrapper.hpp>
#include <dfg/baseConstructorDelegate.hpp>
#include <dfg/typeTraits.hpp>
#include <dfg/build/buildTimeDetails.hpp>
#include <dfg/cont/MapVector.hpp>

std::tuple<std::string, std::string, std::string> FunctionNameTest()
{
    return std::tuple<std::string, std::string, std::string>(DFG_CURRENT_FUNCTION_NAME,
        DFG_CURRENT_FUNCTION_SIGNATURE,
        DFG_CURRENT_FUNCTION_DECORATED_NAME);
}

TEST(dfg, DFG_COUNTOF)
{
    int intArr[3];
    const bool boolArr[50] = { false };
    int* pIa = intArr;
    DFG_UNUSED(intArr);
    DFG_UNUSED(boolArr);
    DFG_UNUSED(pIa);
    DFGTEST_STATIC(3 == DFG_COUNTOF(intArr));
    DFGTEST_STATIC(50 == DFG_COUNTOF(boolArr));
    //DFG_COUNTOF(pIa); // This should fail to compile.
}

TEST(dfg, isEmpty)
{
    using namespace DFG_ROOT_NS;
    int intArr[3];
    std::vector<double> vecDouble(2, 1.0);
    EXPECT_FALSE(isEmpty(intArr));
    EXPECT_TRUE(isEmpty(std::vector<int>()));
    EXPECT_FALSE(isEmpty(std::vector<int>(1,1)));
    EXPECT_TRUE(isEmpty(DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ArrayWrapperT)<double>(vecDouble.data(), 0)));
    EXPECT_FALSE(isEmpty(DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(vecDouble)));
}

TEST(dfg, Endian)
{
#ifdef _WIN32
    EXPECT_EQ(DFG_ROOT_NS::ByteOrderLittleEndian, DFG_ROOT_NS::byteOrderHost());
#endif
}

TEST(DfgDefs, CurrentFunctionNames)
{
#if (DFG_MSVC_VER==DFG_MSVC_VER_2010)
    const auto functionNames = FunctionNameTest();
    EXPECT_EQ("FunctionNameTest", std::get<0>(functionNames));
    EXPECT_EQ("class std::tr1::tuple<class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >,struct std::tr1::_Nil,struct std::tr1::_Nil,struct std::tr1::_Nil,struct std::tr1::_Nil,struct std::tr1::_Nil,struct std::tr1::_Nil,struct std::tr1::_Nil> __cdecl FunctionNameTest(void)", std::get<1>(functionNames));
    EXPECT_EQ("?FunctionNameTest@@YA?AV?$tuple@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V12@V12@U_Nil@tr1@2@U342@U342@U342@U342@U342@U342@@tr1@std@@XZ", std::get<2>(functionNames));

#endif
}

TEST(dfg, IntTypedefs)
{
    using namespace DFG_ROOT_NS;

    // Note: Purpose of these tests is also to check that the typedefs are available
    // (should fail to compile if not).
    DFGTEST_STATIC(sizeof(int8) == 1);
    DFGTEST_STATIC(sizeof(int16) == 2);
    DFGTEST_STATIC(sizeof(int32) == 4);
    DFGTEST_STATIC(sizeof(int64) == 8);

    DFGTEST_STATIC(sizeof(uint8) == sizeof(int8));
    DFGTEST_STATIC(sizeof(uint16) == sizeof(int16));
    DFGTEST_STATIC(sizeof(uint32) == sizeof(int32));
    DFGTEST_STATIC(sizeof(uint64) == sizeof(int64));

    EXPECT_EQ(int8_min, (std::numeric_limits<int8>::min)());
    EXPECT_EQ(int8_max, (std::numeric_limits<int8>::max)());
    EXPECT_EQ(uint8_max, (std::numeric_limits<uint8>::max)());
    DFGTEST_STATIC(int8_min == int8(NumericTraits<int8>::minValue));
    DFGTEST_STATIC(int8_max == int8(NumericTraits<int8>::maxValue));
    DFGTEST_STATIC(uint8_min == uint8(NumericTraits<uint8>::minValue));
    DFGTEST_STATIC(uint8_max == uint8(NumericTraits<uint8>::maxValue));

    EXPECT_EQ(int16_min, (std::numeric_limits<int16>::min)());
    EXPECT_EQ(int16_max, (std::numeric_limits<int16>::max)());
    EXPECT_EQ(uint16_max, (std::numeric_limits<uint16>::max)());
    DFGTEST_STATIC(int16_min == int16(NumericTraits<int16>::minValue));
    DFGTEST_STATIC(int16_max == int16(NumericTraits<int16>::maxValue));
    DFGTEST_STATIC(uint16_min == uint16(NumericTraits<uint16>::minValue));
    DFGTEST_STATIC(uint16_max == uint16(NumericTraits<uint16>::maxValue));

    EXPECT_EQ(int32_min, (std::numeric_limits<int32>::min)());
    EXPECT_EQ(int32_max, (std::numeric_limits<int32>::max)());
    EXPECT_EQ(uint32_max, (std::numeric_limits<uint32>::max)());
    DFGTEST_STATIC(int32_min == int32(NumericTraits<int32>::minValue));
    DFGTEST_STATIC(int32_max == int32(NumericTraits<int32>::maxValue));
    DFGTEST_STATIC(uint32_min == uint32(NumericTraits<uint32>::minValue));
    DFGTEST_STATIC(uint32_max == uint32(NumericTraits<uint32>::maxValue));

    EXPECT_EQ(int64_min, (std::numeric_limits<int64>::min)());
    EXPECT_EQ(int64_max, (std::numeric_limits<int64>::max)());
    EXPECT_EQ(uint64_max, (std::numeric_limits<uint64>::max)());
    DFGTEST_STATIC(int64_min == int64(NumericTraits<int64>::minValue));
    DFGTEST_STATIC(int64_max == int64(NumericTraits<int64>::maxValue));
    DFGTEST_STATIC(uint64_min == uint64(NumericTraits<uint64>::minValue));
    DFGTEST_STATIC(uint64_max == uint64(NumericTraits<uint64>::maxValue));

    int8 a;
    int16 b;
    int32 c;
    int64 d;
    uint8 ua;
    uint16 ub;
    uint32 uc;
    uint64 ud;
    EXPECT_EQ(maxValueOfType(a), int8_max);
    EXPECT_EQ(maxValueOfType(b), int16_max);
    EXPECT_EQ(maxValueOfType(c), int32_max);
    EXPECT_EQ(maxValueOfType(d), int64_max);
    EXPECT_EQ(maxValueOfType(ua), uint8_max);
    EXPECT_EQ(maxValueOfType(ub), uint16_max);
    EXPECT_EQ(maxValueOfType(uc), uint32_max);
    EXPECT_EQ(maxValueOfType(ud), uint64_max);
}

TEST(dfg, round)
{
    using namespace DFG_ROOT_NS;
    EXPECT_EQ( DFG_ROOT_NS::round(1.99), 2.0 );
    EXPECT_EQ( DFG_ROOT_NS::round(1.5), 2.0 );
    EXPECT_EQ( DFG_ROOT_NS::round(1.1), 1.0 );
    EXPECT_EQ( DFG_ROOT_NS::round(0.5), 1.0 );
    EXPECT_EQ( DFG_ROOT_NS::round(-0.1), 0.0 );
    EXPECT_EQ( DFG_ROOT_NS::round(-0.5), -1.0 );
    EXPECT_EQ( DFG_ROOT_NS::round(-0.9), -1.0 );
    EXPECT_EQ( DFG_ROOT_NS::round(-1.4), -1.0 );
    EXPECT_EQ( DFG_ROOT_NS::round(-1.7), -2.0 );
    EXPECT_EQ( DFG_ROOT_NS::round<int32>(int32_max + 0.1), int32_max );
    EXPECT_EQ( DFG_ROOT_NS::round<int32>(int32_max - 0.4), int32_max );
    EXPECT_EQ( DFG_ROOT_NS::round<int32>(int32_max - 0.5), int32_max );
    EXPECT_EQ( DFG_ROOT_NS::round<int32>(int32_min + 0.5), int32_min );
    EXPECT_EQ( DFG_ROOT_NS::round<int32>(int32_min + 0.1), int32_min );
    EXPECT_EQ( DFG_ROOT_NS::round<int32>(int32_min - 0.1), int32_min );
    EXPECT_EQ( DFG_ROOT_NS::round<uint32>(uint32_max + 0.499), uint32_max );
    EXPECT_EQ( DFG_ROOT_NS::round<int8>(110.1), 110 );
    EXPECT_EQ( DFG_ROOT_NS::round<int8>(-110.1), -110 );

    // These should fail to compile
    //DFG_ROOT_NS::round<std::string>(1.0);
    //DFG_ROOT_NS::round<int64>(1.0);
    //DFG_ROOT_NS::round<uint64>(1.0);

    // This should trigger assert.
    //EXPECT_EQ( DFG_ROOT_NS::round<int8>(-129), 0 );
}

namespace
{
    template <class T>
    void byteSwapTestImpl(const T val, const T expectedLe, const T expectedBe)
    {
        using namespace DFG_ROOT_NS;
        EXPECT_EQ(val, byteSwap(val, ByteOrderHost, ByteOrderHost));
        EXPECT_EQ(expectedLe, byteSwap(val, ByteOrderHost, ByteOrderLittleEndian));
        EXPECT_EQ(byteSwap(val, ByteOrderHost, ByteOrderLittleEndian), byteSwapHostToLittleEndian(val));
        EXPECT_EQ(expectedBe, byteSwap(val, ByteOrderHost, ByteOrderBigEndian));
        EXPECT_EQ(byteSwap(val, ByteOrderHost, ByteOrderBigEndian), byteSwapHostToBigEndian(val));
        EXPECT_EQ(expectedBe, byteSwap(expectedLe, ByteOrderLittleEndian, ByteOrderBigEndian));
        EXPECT_EQ(expectedLe, byteSwap(expectedBe, ByteOrderBigEndian, ByteOrderLittleEndian));
    }
}

TEST(dfg, byteSwap)
{
    using namespace DFG_ROOT_NS;

    const uint16 one16Le = (ByteOrderHost == ByteOrderLittleEndian) ? 1 : 1 << 8;
    const uint16 one16Be = (ByteOrderHost == ByteOrderBigEndian) ? 1 : 1 << 8;

    const uint32 one32Le = (ByteOrderHost == ByteOrderLittleEndian) ? 1 : 1 << 24;
    const uint32 one32Be = (ByteOrderHost == ByteOrderBigEndian) ? 1 : 1 << 24;

    const uint64 one64Le = (ByteOrderHost == ByteOrderLittleEndian) ? 1 : (uint64(1) << 56);
    const uint64 one64Be = (ByteOrderHost == ByteOrderBigEndian) ? 1 : (uint64(1) << 56);

    // Note: wchar_t tests are disabled because they fail if wchar_t is signed type (e.g. on GCC).
    //DFG_STATIC_ASSERT(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4, "");
    //const wchar_t oneWcharLe = (sizeof(wchar_t) == 2) ? one16Le : one32Le;
    //const wchar_t oneWcharBe = (sizeof(wchar_t) == 2) ? one16Be : one32Be;

    byteSwapTestImpl(uint8(1), uint8(1), uint8(1));
    byteSwapTestImpl(uint16(1), one16Le, one16Be);
    byteSwapTestImpl(uint32(1), one32Le, one32Be);
    byteSwapTestImpl(uint64(1), one64Le, one64Be);
    byteSwapTestImpl<char>(1, 1, 1);
    byteSwapTestImpl<unsigned char>(1, 1, 1);
    //byteSwapTestImpl(wchar_t(1), oneWcharLe, oneWcharBe);
}

namespace
{
    class PodClass
    {
    public:
        int a;

        bool operator==(const PodClass& other) const
        {
            return this->a == other.a;
        }
    };
}

TEST(dfg, ptrToContiguousMemory)
{
    using namespace DFG_ROOT_NS;

#define TEST_WITH_TYPE(T) \
                { \
                T arr[4] = {T(), T(), T(), T()}; \
                const T arrConst[4] = {T(), T(), T(), T()}; \
                std::vector<T> vec(std::begin(arr), std::end(arr)); \
                std::array<T,4> stdArr = {T(), T(), T(), T()}; \
                auto pArr = ptrToContiguousMemory(arr); \
                auto pArrConst = ptrToContiguousMemory(arrConst); \
                auto pVec = ptrToContiguousMemory(vec); \
                auto pStdArr = ptrToContiguousMemory(stdArr); \
                EXPECT_TRUE(pArr == arr); \
                EXPECT_TRUE(pArrConst == arrConst); \
                EXPECT_TRUE(std::equal(pVec, pVec + vec.size(), arr)); \
                EXPECT_TRUE(std::equal(pStdArr, pStdArr + vec.size(), arr)); \
                *pArr = T(); /*Make sure that returned ptr can also used for writing*/ \
                *pVec = T(); \
                *pStdArr = T(); \
            }

    TEST_WITH_TYPE(char);
    TEST_WITH_TYPE(int);
    TEST_WITH_TYPE(double);
    TEST_WITH_TYPE(PodClass);
    TEST_WITH_TYPE(std::string);
    TEST_WITH_TYPE(std::set<int>);


    std::string s = "abcdef";
    std::wstring ws = L"ghijkl";

    const std::string s2(ptrToContiguousMemory(s), s.size());
    const std::wstring ws2(ptrToContiguousMemory(ws), ws.size());
    EXPECT_EQ(s, s2);
    EXPECT_EQ(ws, ws2);
    EXPECT_EQ(&s2[0], ptrToContiguousMemory(s2));
    EXPECT_EQ(&ws2[0], ptrToContiguousMemory(ws2));

    auto pszC = ptrToContiguousMemory(s);
    auto pszW = ptrToContiguousMemory(ws);
    DFGTEST_STATIC((std::is_same<decltype(pszC), char*>::value));
    DFGTEST_STATIC((std::is_same<decltype(pszW), wchar_t*>::value));
    pszC[0] = '0';
    pszW[0] = L'1';

#undef TEST_WITH_TYPE

}

TEST(dfg, RangeToBeginPtrOrIter)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);

    const double arrDouble[2] = { 0, 0 };
    DFG_UNUSED(arrDouble);

    DFGTEST_STATIC((std::is_same<RangeToBeginPtrOrIter<std::vector<double>>::type, double*>::value));
    DFGTEST_STATIC((std::is_same<RangeToBeginPtrOrIter<std::string>::type, char*>::value));
    DFGTEST_STATIC((std::is_same<RangeToBeginPtrOrIter<std::array<float, 3>>::type, float*>::value));
    DFGTEST_STATIC((std::is_same<RangeToBeginPtrOrIter<std::list<double>>::type, std::list<double>::iterator>::value));
    DFGTEST_STATIC((std::is_same<RangeToBeginPtrOrIter<std::set<double>>::type, std::set<double>::iterator>::value));
    DFGTEST_STATIC((std::is_same<RangeToBeginPtrOrIter<decltype(arrDouble)>::type, const double*>::value));


    DFGTEST_STATIC((std::is_same<RangeToBeginPtrOrIter<DFG_CLASS_NAME(ArrayWrapperT<double>)>::type, double*>::value));
    DFGTEST_STATIC((std::is_same<RangeToBeginPtrOrIter<DFG_CLASS_NAME(ArrayWrapperT<const double>)>::type, const double*>::value));

    DFGTEST_STATIC((std::is_same<RangeToBeginPtrOrIter<const std::vector<double>>::type, const double*>::value));
    DFGTEST_STATIC((std::is_same<RangeToBeginPtrOrIter<const std::list<double>>::type, std::list<double>::const_iterator>::value));
}


TEST(dfg, sizeInBytes)
{
    using namespace DFG_ROOT_NS;

#define TEST_SIZE_IN_BYTES(VALUE_TYPE) \
    { \
        std::vector<VALUE_TYPE> vec(10); \
        std::array<VALUE_TYPE, 10> stdArr; \
        VALUE_TYPE plainArray[10]; \
        EXPECT_EQ(sizeof(VALUE_TYPE) * vec.size(), sizeInBytes(vec)); \
        EXPECT_EQ(sizeof(VALUE_TYPE) * stdArr.size(), sizeInBytes(stdArr)); \
        EXPECT_EQ(sizeof(VALUE_TYPE) * count(plainArray), sizeInBytes(plainArray)); \
    }

    TEST_SIZE_IN_BYTES(char);
    TEST_SIZE_IN_BYTES(double);
    TEST_SIZE_IN_BYTES(std::string);

#undef TEST_SIZE_IN_BYTES
}

class ConstructorDelegateTest
{
public:
    DFG_CONSTRUCTOR_INITIALIZATION_DELEGATE_2(ConstructorDelegateTest, m_vec) {}

    std::vector<int> m_vec;
};

TEST(dfg, ConstructorInitializationDelegate)
{
    const std::array<int, 3> arr = { 1, 2, 3 };
    ConstructorDelegateTest test(std::begin(arr), std::end(arr));
    EXPECT_EQ(1, test.m_vec[0]);
    EXPECT_EQ(2, test.m_vec[1]);
    EXPECT_EQ(3, test.m_vec[2]);
}

TEST(dfg, isValWithinLimitsOfType)
{
    using namespace DFG_ROOT_NS;

    char a = 1;
    EXPECT_FALSE(isValWithinLimitsOfType<char>(int(128)));
    EXPECT_FALSE(isValWithinLimitsOfType(int(128), a));
    EXPECT_TRUE(isValWithinLimitsOfType<char>(int(127)));
    EXPECT_TRUE(isValWithinLimitsOfType(int(127), a));
    EXPECT_TRUE(isValWithinLimitsOfType<char>(int(-127)));
    EXPECT_FALSE(isValWithinLimitsOfType<int8>(int16(3210)));
    EXPECT_FALSE(isValWithinLimitsOfType<uint8>(int16(3210)));
    EXPECT_FALSE(isValWithinLimitsOfType<uint16>(int(-1)));
    EXPECT_FALSE(isValWithinLimitsOfType<uint8>(char(-1)));
    EXPECT_FALSE(isValWithinLimitsOfType<uint16>(char(-1)));
    EXPECT_FALSE(isValWithinLimitsOfType<uint16>(int16(-1)));
    EXPECT_FALSE(isValWithinLimitsOfType<uint32>(char(-1)));
    EXPECT_FALSE(isValWithinLimitsOfType<uint32>(int(-1)));
    EXPECT_FALSE(isValWithinLimitsOfType<uint64>(char(-1)));
    EXPECT_FALSE(isValWithinLimitsOfType<uint64>(int64(-1)));
    EXPECT_TRUE(isValWithinLimitsOfType<uint64>(uint8(255)));
    EXPECT_TRUE(isValWithinLimitsOfType<uint8>(uint64(255)));
}

namespace
{
    void funcAscii(DFG_ROOT_NS::SzPtrAsciiR psz)
    {
        EXPECT_STREQ("abcd", psz.c_str());
    }

    void funcLatin1(DFG_ROOT_NS::SzPtrLatin1R psz)
    {
        EXPECT_STREQ("abcd", psz.c_str());
    }

    void funcUtf8(DFG_ROOT_NS::SzPtrUtf8R psz)
    {
        EXPECT_STREQ("abcd", psz.c_str());
    }
} // unnamed namespace

TEST(dfg, SzPtrTypes)
{
    using namespace DFG_ROOT_NS;

    char szNonConst[] = "nonConst";
    const char szConst[] = "const";
    DFG_UNUSED(szNonConst);
    DFG_UNUSED(szConst);

    // Test existence of SzPtrAscii, SzPtrLatin1 and SzPtrUtf8 functions and their return types.
    DFGTEST_STATIC((std::is_same<char*, decltype(SzPtrAscii(szNonConst).c_str())>::value));
    DFGTEST_STATIC((std::is_same<const char*, decltype(SzPtrAscii(szConst).c_str())>::value));
    DFGTEST_STATIC((std::is_same<char*, decltype(SzPtrLatin1(szNonConst).c_str())>::value));
    DFGTEST_STATIC((std::is_same<const char*, decltype(SzPtrLatin1(szConst).c_str())>::value));
    DFGTEST_STATIC((std::is_same<char*, decltype(SzPtrUtf8(szNonConst).c_str())>::value));
    DFGTEST_STATIC((std::is_same<const char*, decltype(SzPtrUtf8(szConst).c_str())>::value));

    // Test automatic conversion from SzPtrXW -> SzPtrXR
    {
        SzPtrAsciiW pszAsciiW = SzPtrAscii(szNonConst);
        SzPtrAsciiR pszAsciiR = pszAsciiW;
        EXPECT_EQ(pszAsciiW.c_str(), pszAsciiR.c_str()); // Note: pointer comparison is intended

        SzPtrLatin1W pszLatin1W = SzPtrLatin1(szNonConst);
        SzPtrLatin1R pszLatin1R = pszLatin1W;
        EXPECT_EQ(pszLatin1W.c_str(), pszLatin1R.c_str()); // Note: pointer comparison is intended

        SzPtrUtf8W pszUtf8W = SzPtrUtf8(szNonConst);
        SzPtrUtf8R pszUtf8R = pszUtf8W;
        EXPECT_EQ(pszUtf8W.c_str(), pszUtf8R.c_str()); // Note: pointer comparison is intended
    }

    // Test conversion SzPtr<char> -> TypedCharPtr<const char>
    {
        SzPtrAsciiW pszAsciiW = SzPtrAscii(szNonConst);
        TypedCharPtrAsciiR pAsciiR = pszAsciiW;
        TypedCharPtrLatin1R pLatin1R = pszAsciiW;
        TypedCharPtrUtf8R pUtf8R = pszAsciiW;
    }

    // Basic usage test
    funcAscii(SzPtrAscii("abcd"));
    //funcAscii(SzPtrUtf8("abcd")); // Should fail to compile
    funcLatin1(SzPtrLatin1("abcd"));
    //funcLatin1(SzPtrUtf8("abcd")); // Should fail to compile
    funcUtf8(SzPtrUtf8("abcd"));
    //funcUtf8(SzPtrLatin1("abcd")); // Should fail to compile

    // Test automatic conversion from ASCII to ASCII superset
    funcLatin1(SzPtrAscii("abcd"));
    funcUtf8(SzPtrAscii("abcd"));

    EXPECT_TRUE(SzPtrAscii(szConst) == SzPtrAscii(szConst));
    EXPECT_FALSE(SzPtrAscii(szConst) == SzPtrAscii(szNonConst));
    EXPECT_TRUE(SzPtrLatin1(szConst) == SzPtrLatin1(szConst));
    EXPECT_FALSE(SzPtrLatin1(szConst) == SzPtrLatin1(szNonConst));
    EXPECT_TRUE(SzPtrUtf8(szConst) == SzPtrUtf8(szConst));
    EXPECT_FALSE(SzPtrUtf8(szConst) == SzPtrUtf8(szNonConst));

    // Test bool conversion and comparison with nullptr
    {
        //EXPECT_TRUE(SzPtrAscii("")); // This does not compile on VC2015
        if (SzPtrAscii(""))
            EXPECT_TRUE(true);
        else
            EXPECT_TRUE(false);
        EXPECT_TRUE(SzPtrAscii("") != nullptr);
        EXPECT_TRUE(SzPtrAscii(nullptr) == nullptr);
    }
}

TEST(dfg, toSzPtr)
{
    using namespace DFG_ROOT_NS;

    const char sz[] = "abc";
    const wchar_t wsz[] = L"abc";
    const auto tszAscii = SzPtrAscii(sz);
    const auto tszLatin1 = SzPtrLatin1(sz);
    const auto tszUtf8 = SzPtrUtf8(sz);
    std::string s(sz);
    std::wstring ws(wsz);

    DFGTEST_STATIC((std::is_same<const char*, decltype(toCharPtr_raw(sz))>::value));
    DFGTEST_STATIC((std::is_same<const wchar_t*, decltype(toCharPtr_raw(wsz))>::value));
    DFGTEST_STATIC((std::is_same<const char*, decltype(toCharPtr_raw(s))>::value));
    DFGTEST_STATIC((std::is_same<const wchar_t*, decltype(toCharPtr_raw(ws))>::value));
    DFGTEST_STATIC((std::is_same<const char*, decltype(toCharPtr_raw(tszAscii))>::value));
    DFGTEST_STATIC((std::is_same<const char*, decltype(toCharPtr_raw(tszLatin1))>::value));
    DFGTEST_STATIC((std::is_same<const char*, decltype(toCharPtr_raw(tszUtf8))>::value));

    DFGTEST_STATIC((std::is_same<TypedCharPtrAsciiR, decltype(toCharPtr_typed(tszAscii))>::value));
    DFGTEST_STATIC((std::is_same<TypedCharPtrLatin1R, decltype(toCharPtr_typed(tszLatin1))>::value));
    DFGTEST_STATIC((std::is_same<TypedCharPtrUtf8R, decltype(toCharPtr_typed(tszUtf8))>::value));

    DFGTEST_STATIC((std::is_same<SzPtrAsciiR, decltype(toSzPtr_typed(tszAscii))>::value));
    DFGTEST_STATIC((std::is_same<SzPtrLatin1R, decltype(toSzPtr_typed(tszLatin1))>::value));
    DFGTEST_STATIC((std::is_same<SzPtrUtf8R, decltype(toSzPtr_typed(tszUtf8))>::value));

    EXPECT_EQ(sz, toCharPtr_raw(sz));
    EXPECT_EQ(wsz, toCharPtr_raw(wsz));
    EXPECT_EQ(s.c_str(), toCharPtr_raw(s));
    EXPECT_EQ(ws.c_str(), toCharPtr_raw(ws));
    EXPECT_EQ(sz, toCharPtr_raw(tszAscii));
    EXPECT_EQ(sz, toCharPtr_raw(tszLatin1));
    EXPECT_EQ(sz, toCharPtr_raw(tszUtf8));

    EXPECT_EQ(tszAscii, toSzPtr_typed(tszAscii));
    EXPECT_EQ(tszLatin1, toSzPtr_typed(tszLatin1));
    EXPECT_EQ(tszUtf8, toSzPtr_typed(tszUtf8));
}

namespace
{
    static DFG_CONSTEXPR int constexprMarkedFunc() { return 2; }
}

TEST(dfg, constExpr)
{
#if DFG_LANGFEAT_CONSTEXPR
    DFGTEST_STATIC((std::integral_constant<int, constexprMarkedFunc()>::value == 2));
#else
    EXPECT_EQ(2, constexprMarkedFunc()); // Makes sure that unsupported DFG_CONSTEXPR doesn't break compiling.
#endif
}

TEST(dfg, ScopedCaller)
{
    using namespace DFG_ROOT_NS;
    {
        int val = 0;
        {
            const auto scopedCaller = makeScopedCaller([&]() { val++; }, [&]() { val -= 2; });
            EXPECT_EQ(1, val);
        }
        EXPECT_EQ(-1, val);
    }

    {
        int val = 0;
        {
            auto scopedCaller = makeScopedCaller([&]() { val++; }, [&]() { val -= 2; });
            EXPECT_EQ(1, val);
            auto scopedCaller2 = std::move(scopedCaller);
            EXPECT_EQ(1, val);
        }
        EXPECT_EQ(-1, val);
    }
}

TEST(dfgTypeTraits, IsTrueTrait)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(TypeTraits);
    DFGTEST_STATIC(IsTrueTrait<UnknownAnswerType>::value == false);
    DFGTEST_STATIC(IsTrueTrait<IsTriviallyCopyable<int*>>::value == true);
}

TEST(dfgBuild, buildTimeDetails)
{
    using namespace DFG_ROOT_NS;
    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorAoS)<BuildTimeDetail, NonNullCStr> vals;
    getBuildTimeDetailStrs([&](BuildTimeDetail btd, const NonNullCStr psz)
    {
        vals[btd] = psz;
    });

    EXPECT_EQ(DFG_COUNTOF(DFG_DETAIL_NS::buildTimeDetailStrs), vals.size()); // If this fails, check whether getBuildTimeDetailStrs() includes all id's.
    EXPECT_STRNE("", vals[BuildTimeDetail_dateTime]);
    EXPECT_STREQ(DFG_COMPILER_NAME_SIMPLE, vals[BuildTimeDetail_compilerAndShortVersion]);
    EXPECT_STREQ(DFG_STRINGIZE(DFG_COMPILER_FULL_VERSION), vals[BuildTimeDetail_compilerFullVersion]);
    EXPECT_STREQ("", vals[BuildTimeDetail_qtVersion]); // Qt should not be in the build so make sure the detail is empty.
#ifdef BOOST_LIB_VERSION
    EXPECT_STREQ(BOOST_LIB_VERSION, vals[BuildTimeDetail_boostVersion]);
#endif
}

TEST(dfgBuild, DFG_STRING_LITERAL_TO_TYPED_LITERAL)
{
#define DFGTEST_TEMP_TEST0 "abc"

    EXPECT_STREQ(L"abc", DFG_STRING_LITERAL_TO_WSTRING_LITERAL("abc"));
    EXPECT_STREQ(L"abc", DFG_STRING_LITERAL_TO_WSTRING_LITERAL(DFGTEST_TEMP_TEST0));

#if !defined(_MSC_VER) || (DFG_MSVC_VER >= DFG_MSVC_VER_2015) // u and U prefixes are supported in MSVC only since MSVC2015
    // char16_t
    EXPECT_EQ(std::basic_string<char16_t>(u"abc"), DFG_STRING_LITERAL_TO_CHAR16_LITERAL("abc"));
    EXPECT_EQ(std::basic_string<char16_t>(u"abc"), DFG_STRING_LITERAL_TO_CHAR16_LITERAL(DFGTEST_TEMP_TEST0));

    // char32_t
    EXPECT_EQ(std::basic_string<char32_t>(U"abc"), DFG_STRING_LITERAL_TO_CHAR32_LITERAL("abc"));
    EXPECT_EQ(std::basic_string<char32_t>(U"abc"), DFG_STRING_LITERAL_TO_CHAR32_LITERAL(DFGTEST_TEMP_TEST0));
#endif // !defined(_MSC_VER) || (DFG_MSVC_VER >= DFG_MSVC_VER_2015)

#undef DFGTEST_TEMP_TEST0
}

TEST(dfgBuild, DFG_STRING_LITERAL_BY_CHARTYPE)
{
#define DFGTEST_TEMP_TEST0 "abc"

    // Char
    EXPECT_STREQ("abc", DFG_STRING_LITERAL_BY_CHARTYPE(char, DFGTEST_TEMP_TEST0));
    EXPECT_STREQ("abc", DFG_STRING_LITERAL_BY_CHARTYPE(char, "abc"));

    // wchar_t
    EXPECT_STREQ(L"abc", DFG_STRING_LITERAL_BY_CHARTYPE(wchar_t, DFGTEST_TEMP_TEST0));
    EXPECT_STREQ(L"abc", DFG_STRING_LITERAL_BY_CHARTYPE(wchar_t, "abc"));

#if !defined(_MSC_VER) || (DFG_MSVC_VER >= DFG_MSVC_VER_2015) // u and U prefixes are supported in MSVC only since MSVC2015
    // char16_t
    EXPECT_EQ(std::basic_string<char16_t>(u"abc"), DFG_STRING_LITERAL_BY_CHARTYPE(char16_t, DFGTEST_TEMP_TEST0));
    EXPECT_EQ(std::basic_string<char16_t>(u"abc"), DFG_STRING_LITERAL_BY_CHARTYPE(char16_t, "abc"));

    // char32_t
    EXPECT_EQ(std::basic_string<char32_t>(U"abc"), DFG_STRING_LITERAL_BY_CHARTYPE(char32_t, DFGTEST_TEMP_TEST0));
    EXPECT_EQ(std::basic_string<char32_t>(U"abc"), DFG_STRING_LITERAL_BY_CHARTYPE(char32_t, "abc"));

#endif // !defined(_MSC_VER) || (DFG_MSVC_VER >= DFG_MSVC_VER_2015)
#undef DFGTEST_TEMP_TEST0
}
