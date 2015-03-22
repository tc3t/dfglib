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

std::tuple<std::string, std::string, std::string> FunctionNameTest()
{
    return std::tuple<std::string, std::string, std::string>(DFG_CURRENT_FUNCTION_NAME,
        DFG_CURRENT_FUNCTION_SIGNATURE,
        DFG_CURRENT_FUNCTION_DECORATED_NAME);
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
    EXPECT_EQ(sizeof(int8), 1);
    EXPECT_EQ(sizeof(int16), 2);
    EXPECT_EQ(sizeof(int32), 4);
    EXPECT_EQ(sizeof(int64), 8);

    EXPECT_EQ(sizeof(uint8),  sizeof(int8));
    EXPECT_EQ(sizeof(uint16), sizeof(int16));
    EXPECT_EQ(sizeof(uint32), sizeof(int32));
    EXPECT_EQ(sizeof(uint64), sizeof(int64));

    EXPECT_EQ(int8_min, (std::numeric_limits<int8>::min)());
    EXPECT_EQ(int8_max, (std::numeric_limits<int8>::max)());
    EXPECT_EQ(uint8_max, (std::numeric_limits<uint8>::max)());
    EXPECT_EQ(int8_min, int8(NumericTraits<int8>::minValue));
    EXPECT_EQ(int8_max, int8(NumericTraits<int8>::maxValue));
    EXPECT_EQ(uint8_max, uint8(NumericTraits<uint8>::maxValue));

    EXPECT_EQ(int16_min, (std::numeric_limits<int16>::min)());
    EXPECT_EQ(int16_max, (std::numeric_limits<int16>::max)());
    EXPECT_EQ(uint16_max, (std::numeric_limits<uint16>::max)());
    EXPECT_EQ(int16_min, int16(NumericTraits<int16>::minValue));
    EXPECT_EQ(int16_max, int16(NumericTraits<int16>::maxValue));
    EXPECT_EQ(uint16_max, uint16(NumericTraits<uint16>::maxValue));

    EXPECT_EQ(int32_min, (std::numeric_limits<int32>::min)());
    EXPECT_EQ(int32_max, (std::numeric_limits<int32>::max)());
    EXPECT_EQ(uint32_max, (std::numeric_limits<uint32>::max)());
    EXPECT_EQ(int32_min, int32(NumericTraits<int32>::minValue));
    EXPECT_EQ(int32_max, int32(NumericTraits<int32>::maxValue));
    EXPECT_EQ(uint32_max, uint32(NumericTraits<uint32>::maxValue));

    EXPECT_EQ(int64_min, (std::numeric_limits<int64>::min)());
    EXPECT_EQ(int64_max, (std::numeric_limits<int64>::max)());
    EXPECT_EQ(uint64_max, (std::numeric_limits<uint64>::max)());
    EXPECT_EQ(int64_min, int64(NumericTraits<int64>::minValue));
    EXPECT_EQ(int64_max, int64(NumericTraits<int64>::maxValue));
    EXPECT_EQ(uint64_max, uint64(NumericTraits<uint64>::maxValue));

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

    DFG_STATIC_ASSERT(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4, "");
    const wchar_t oneWcharLe = (sizeof(wchar_t) == 2) ? one16Le : one32Le;
    const wchar_t oneWcharBe = (sizeof(wchar_t) == 2) ? one16Be : one32Be;

    byteSwapTestImpl(uint8(1), uint8(1), uint8(1));
    byteSwapTestImpl(uint16(1), one16Le, one16Be);
    byteSwapTestImpl(uint32(1), one32Le, one32Be);
    byteSwapTestImpl(uint64(1), one64Le, one64Be);
    byteSwapTestImpl<char>(1, 1, 1);
    byteSwapTestImpl<unsigned char>(1, 1, 1);
    byteSwapTestImpl(wchar_t(1), oneWcharLe, oneWcharBe);
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

