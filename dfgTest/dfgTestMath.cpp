#include <stdafx.h>
#include <dfg/dfgBaseTypedefs.hpp>
#include <dfg/math.hpp>
#include <dfg/math/pow.hpp>
#include <dfg/math/roundedUpToMultiple.hpp>
#include <dfg/math/interval.hpp>
#include <dfg/math/evalPolynomial.hpp>
#include <dfg/math/interpolationLinear.hpp>
#include <dfg/math/FormulaParser.hpp>
#include <dfg/cont.hpp>
#include <dfg/alg.hpp>

TEST(dfgMath, roundedUpToMultiple)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);
    EXPECT_EQ(505, roundedUpToMultiple(502, 5));
    EXPECT_EQ(5678510, roundedUpToMultiple(5678502, 10));
    EXPECT_EQ(8502, roundedUpToMultiple(8502, 1));
    EXPECT_EQ(8505, roundedUpToMultiple(8502, 7));

    DFGTEST_STATIC(8505 == (DFG_CLASS_NAME(RoundedUpToMultipleT)<8502, 7>::value));
    DFGTEST_STATIC(pow2ToXCt<31>::value == (DFG_CLASS_NAME(RoundedUpToMultipleT)<pow2ToXCt<31>::value, 2>::value));
    DFGTEST_STATIC(uint32_max == (DFG_CLASS_NAME(RoundedUpToMultipleT)<uint32_max, 1>::value));
    DFGTEST_STATIC(4294967294u == (DFG_CLASS_NAME(RoundedUpToMultipleT)<4294967293u, 2>::value));
}

TEST(dfgMath, factorial)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);

#define FACTORIAL_TEST(n, expected) \
    { \
        const auto val = DFG_CLASS_NAME(Factorial_T)<n>::value; /*Without this temporary MinGW generated linker errors*/ \
        EXPECT_EQ(expected, val); \
    }

#define FACTORIAL_TEST_SMALL(n, expected) \
    { \
        const auto val2 = DFG_CLASS_NAME(Factorial_T)<n>::value; /*Without this temporary MinGW generated linker errors*/ \
        EXPECT_EQ(factorialInt(n), val2); \
        FACTORIAL_TEST(n, expected); \
    }

    DFG_STATIC_ASSERT((std::is_same<decltype(factorialInt(1)), size_t>::value == true), "Expecting Factorial to return size_t(see the macro above)");
    
    FACTORIAL_TEST_SMALL(1, 1);
    FACTORIAL_TEST_SMALL(2, 2);
    FACTORIAL_TEST_SMALL(3, 6);
    FACTORIAL_TEST_SMALL(4, 24);
    FACTORIAL_TEST_SMALL(5, 120);
    FACTORIAL_TEST_SMALL(6, 720);
    FACTORIAL_TEST_SMALL(7, 5040);
    FACTORIAL_TEST_SMALL(8, 40320);
    FACTORIAL_TEST_SMALL(9, 362880);
    FACTORIAL_TEST_SMALL(10, 3628800);
    FACTORIAL_TEST_SMALL(11, 39916800);
    FACTORIAL_TEST_SMALL(12, 479001600);
    FACTORIAL_TEST(13, 6227020800);
    FACTORIAL_TEST(14, 87178291200);
    FACTORIAL_TEST(15, 1307674368000);
    FACTORIAL_TEST(16, 20922789888000);
    FACTORIAL_TEST(17, 355687428096000);
    FACTORIAL_TEST(18, 6402373705728000);
    FACTORIAL_TEST(19, 121645100408832000);
    FACTORIAL_TEST(20, 2432902008176640000);
    // uint64 overflows after this.
    //FACTORIAL_TEST(21,	51090942171709440000);


#undef FACTORIAL_TEST

}

TEST(dfgMath, Interval)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);

    EXPECT_FALSE(Interval_T<int8>(-127, 127).isInRangeII(-128));
    EXPECT_TRUE(Interval_T<int8>(-128, 127).isInRangeII(-128));
    EXPECT_TRUE(Interval_T<int8>(-128, 127).isInRangeII(127));

    EXPECT_TRUE(Interval_T<int32>(-5000, 6000).isInRangeII(0));
    EXPECT_FALSE(Interval_T<int32>(-5000, 6000).isInRangeII(6001));

    EXPECT_TRUE(Interval_T<int32>(int32_min, int32_max).isInRangeII(int32_min));
    EXPECT_TRUE(Interval_T<int32>(int32_min, int32_max).isInRangeII(int32_max));

    EXPECT_FALSE(Interval_T<uint32>(1, uint32_max).isInRangeII(0));
    EXPECT_TRUE(Interval_T<uint32>(0, uint32_max).isInRangeII(0));
    EXPECT_TRUE(Interval_T<uint32>(4524, uint32_max).isInRangeII(uint32_max));
    EXPECT_FALSE(Interval_T<uint32>(452, uint32_max - 1).isInRangeII(uint32_max));
}

namespace
{
    template <class T> void powNTestImpl(const size_t i, const T value)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(math);

        const int nPow2Modifier = std::is_integral<T>::value && std::is_signed<T>::value;

        if (i == 2)
            EXPECT_EQ(powN(value, i), pow2(value));
        if (i == 3)
            EXPECT_EQ(powN(value, i), pow3(value));
        if (i == 4)
            EXPECT_EQ(powN(value, i), pow4(value));
        if (i == 5)
            EXPECT_EQ(powN(value, i), pow5(value));
        if (i == 6)
            EXPECT_EQ(powN(value, i), pow6(value));
        if (i == 7)
            EXPECT_EQ(powN(value, i), pow7(value));
        if (i == 8)
            EXPECT_EQ(powN(value, i), pow8(value));
        if (i == 9)
            EXPECT_EQ(powN(value, i), pow9(value));
        if (i < 8 * Min(sizeof(size_t), sizeof(T)) - nPow2Modifier && value == 2)
            EXPECT_EQ(powN(value, i), pow2ToX(i));
    }
}

TEST(dfgMath, pow2ToX)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);

    for (size_t i = 0; i < 8 * sizeof(size_t); ++i)
    {
        EXPECT_EQ(size_t(1) << i, pow2ToX(i));
    }

#ifdef _WIN64
    EXPECT_EQ(pow2ToX(32), 4294967296);
    EXPECT_EQ(pow2ToX(63), 9223372036854775808ull);
#endif
}

TEST(dfgMath, powX)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);

    for (size_t i = 0; i < 64; ++i)
    {
#define TEST_FOR_VALUE(VAL) \
        powNTestImpl(i, size_t(VAL)); \
        powNTestImpl(i, int(VAL)); \
        powNTestImpl(i, uint32(VAL)); \
        powNTestImpl(i, float(VAL)); \
        powNTestImpl(i, double(VAL));

        TEST_FOR_VALUE(-1);
        TEST_FOR_VALUE(1);
        TEST_FOR_VALUE(-2);
        TEST_FOR_VALUE(2);
        TEST_FOR_VALUE(-3);
        TEST_FOR_VALUE(3);
        TEST_FOR_VALUE(-4);
        TEST_FOR_VALUE(4);

#undef TEST_FOR_VALUE

        // Note: These tests can be expected to fail depending on floating point settings.
        powNTestImpl(i, float(-4.5));
        powNTestImpl(i, float(4.5));
        powNTestImpl(i, double(-4.5));
        powNTestImpl(i, double(4.5));
        powNTestImpl(i, (long double)(-4.5));
        powNTestImpl(i, (long double)(4.5));
    }
}

TEST(dfgMath, evalPolynomial)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(alg);

    const double twoToPower15[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 };

    EXPECT_EQ(0, evalPolynomial(0.0, std::vector<double>()));
    EXPECT_EQ(0, evalPolynomial(std::numeric_limits<double>::infinity(), std::vector<double>()));
    EXPECT_EQ(0, evalPolynomial(0.0, makeArray(0.0)));
    EXPECT_EQ(0, evalPolynomial(123456.0, makeArray(0.0)));
    EXPECT_EQ(0, evalPolynomial(std::numeric_limits<double>::infinity(), makeArray(0.0)));
    EXPECT_EQ(0, evalPolynomial(std::numeric_limits<double>::infinity(), makeArray(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)));
    EXPECT_EQ(1, evalPolynomial(0.0, makeArray(1.0)));
    EXPECT_EQ(1, evalPolynomial(0.0, makeArray(1.0, 2.0)));
    EXPECT_EQ(1, evalPolynomial(0.0, makeArray(1.0, 2.0, 3.0)));
    EXPECT_EQ(1, evalPolynomial(0.0, makeArray(1.0, 2.0, 3.0, 4.0)));

    EXPECT_EQ(7.9, evalPolynomial(7.9, makeArray(0.0, 1.0)));
    EXPECT_EQ(0, evalPolynomial(2.0, makeArray(4.0, -2.0)));
    EXPECT_EQ(34, evalPolynomial(2.0, makeArray(4.0, -2.0, 8.5)));
    EXPECT_EQ(117, evalPolynomial(5.0, makeArray(2.0, 3.0, 4.0)));
    EXPECT_EQ(-508, evalPolynomial(5.0, makeArray(2.0, 3.0, 4.0, -5.0)));
    EXPECT_EQ(-508, evalPolynomial(-5.0, makeArray(2.0, -3.0, 4.0, 5.0)));

    EXPECT_NEAR(-566.205116, evalPolynomial(-2.356, makeArray(4.0, -2.0, 8.5, -2.4, 8.9, 12.78)), 1e-6);

    EXPECT_EQ(pow2ToX(15), evalPolynomial(2.0, twoToPower15));

    std::vector<double> vec(300);
    generateAdjacent(vec, 5, 10);
    EXPECT_EQ(arithmeticSumFirstLastStep(5, 2995, 10), evalPolynomial(1.0, vec));

    // Integer polynomial
    EXPECT_EQ(0, evalPolynomial(2, makeArray(4, -2)));

}

TEST(dfgMath, isNan)
{
    using namespace DFG_MODULE_NS(math);

    EXPECT_EQ(isNan(1.0), false);
    EXPECT_EQ(isNan(-1.0), false);
    EXPECT_EQ(isNan(std::numeric_limits<float>::lowest()), false);
    EXPECT_EQ(isNan(std::numeric_limits<float>::max()), false);
    EXPECT_EQ(isNan(std::numeric_limits<double>::lowest()), false);
    EXPECT_EQ(isNan(std::numeric_limits<double>::max()), false);
    EXPECT_EQ(isNan(std::numeric_limits<float>::quiet_NaN()), true);
    EXPECT_EQ(isNan(std::numeric_limits<double>::quiet_NaN()), true);
#if DFG_LANGFEAT_HAS_ISNAN || !defined(_MSC_VER) // MSVC _isnan warns about long double -> double conversion so ignore the test.
    EXPECT_EQ(isNan(std::numeric_limits<long double>::quiet_NaN()), true);
#endif
    EXPECT_EQ(isNan(std::numeric_limits<double>::infinity()), false);
    EXPECT_EQ(isNan(-1 * std::numeric_limits<double>::infinity()), false);

    EXPECT_FALSE(isNan(1)); // Tests that compiles with integer type.
}

TEST(dfgMath, isFinite)
{
    using namespace DFG_MODULE_NS(math);

    EXPECT_EQ(isFinite(1.0), true);
    EXPECT_EQ(isFinite(-1.0), true);
    EXPECT_EQ(isFinite(std::numeric_limits<float>::lowest()), true);
    EXPECT_EQ(isFinite(std::numeric_limits<float>::max()), true);
    EXPECT_EQ(isFinite(std::numeric_limits<double>::lowest()), true);
    EXPECT_EQ(isFinite(std::numeric_limits<double>::max()), true);
    EXPECT_EQ(isFinite(std::numeric_limits<float>::quiet_NaN()), false);
    EXPECT_EQ(isFinite(std::numeric_limits<double>::quiet_NaN()), false);
    EXPECT_EQ(isFinite(std::numeric_limits<double>::infinity()), false);
    EXPECT_EQ(isFinite(-1 * std::numeric_limits<double>::infinity()), false);
}

TEST(dfgMath, isInf)
{
    using namespace DFG_MODULE_NS(math);

    EXPECT_EQ(isInf(1.0), false);
    EXPECT_EQ(isInf(-1.0), false);
    EXPECT_EQ(isInf(std::numeric_limits<float>::lowest()), false);
    EXPECT_EQ(isInf(std::numeric_limits<float>::max()), false);
    EXPECT_EQ(isInf(std::numeric_limits<double>::lowest()), false);
    EXPECT_EQ(isInf(std::numeric_limits<double>::max()), false);
    EXPECT_EQ(isInf(std::numeric_limits<float>::quiet_NaN()), false);
    EXPECT_EQ(isInf(std::numeric_limits<double>::quiet_NaN()), false);
    EXPECT_EQ(isInf(std::numeric_limits<double>::infinity()), true);
    EXPECT_EQ(isInf(-1 * std::numeric_limits<double>::infinity()), true);
}

TEST(dfgMath, interpolationLinear)
{
    using namespace DFG_MODULE_NS(math);

    EXPECT_EQ(1.0, interpolationLinear_X_X0Y0_X1Y1(1.0, 0.0, 2.0, 4.0, -2.0));
    EXPECT_EQ(2.0, interpolationLinear_X_X0Y0_X1Y1(1.0, 1.0, 2.0, 1.0, 2.0));
    EXPECT_EQ(3.0, interpolationLinear_X_X0Y0_X1Y1(1.0, 1.0, 2.0, 1.0, 4.0));
}

namespace
{
    template <class Int_T>
    void signedUnsignedTests(std::true_type)
    {
        EXPECT_EQ(1, ::DFG_ROOT_NS::absAsUnsigned(Int_T(-1)));
    }

    template <class Int_T>
    void signedUnsignedTests(std::false_type)
    {
    }

    template <class Int_T>
    void absUnsignedImpl(const typename std::make_unsigned<Int_T>::type minAbs)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(math);
        typedef typename std::make_unsigned<Int_T>::type UnsignedT;
        const auto zeroVal = absAsUnsigned(Int_T(0));
        DFG_UNUSED(zeroVal);
        DFGTEST_STATIC((std::is_same<const UnsignedT, decltype(zeroVal)>::value));
        EXPECT_EQ(0, absAsUnsigned(Int_T(0)));
        signedUnsignedTests<Int_T>(typename std::is_signed<Int_T>::type());
        EXPECT_EQ(minAbs, absAsUnsigned(NumericTraits<Int_T>::minValue));
        EXPECT_EQ(static_cast<UnsignedT>(NumericTraits<Int_T>::maxValue), absAsUnsigned(NumericTraits<Int_T>::maxValue));
    }
}

TEST(dfgMath, absAsUnsigned)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);
    absUnsignedImpl<int8>(pow2ToXCt<7>::value);
    absUnsignedImpl<uint8>(0);
    absUnsignedImpl<int16>(pow2ToXCt<15>::value);
    absUnsignedImpl<uint16>(0);
    absUnsignedImpl<int32>(pow2ToXCt<31>::value);
    absUnsignedImpl<uint32>(0);
    absUnsignedImpl<int64>(static_cast<uint64>(NumericTraits<int64>::maxValue) + 1);
    absUnsignedImpl<uint64>(0);
}

TEST(dfgMath, logOfBase)
{
    using namespace DFG_MODULE_NS(math);

    // Test for expected return types.
    DFGTEST_STATIC((std::is_same<decltype(logOfBase(10, float(10))), double>::value));
    DFGTEST_STATIC((std::is_same<decltype(logOfBase(10, double(10))), double>::value));
    DFGTEST_STATIC((std::is_same<decltype(logOfBase(float(10), 10)), float>::value));
    DFGTEST_STATIC((std::is_same<decltype(logOfBase(float(10), double(10))), float>::value));
    DFGTEST_STATIC((std::is_same<decltype(logOfBase(double(10), 10)), double>::value));
    DFGTEST_STATIC((std::is_same<decltype(logOfBase(double(10), float(10))), double>::value));
    DFGTEST_STATIC((std::is_same<decltype(logOfBase(static_cast<long double>(10), double(10))), long double>::value));

    EXPECT_DOUBLE_EQ(1, logOfBase(10, 10));
    EXPECT_DOUBLE_EQ(3, logOfBase(8, 2));
    EXPECT_DOUBLE_EQ(-1, logOfBase(0.5, 2));
    EXPECT_DOUBLE_EQ(0, logOfBase(1, 3));
    EXPECT_DOUBLE_EQ(31, logOfBase(pow2ToXCt<31>::value, 2));
    EXPECT_DOUBLE_EQ(2, logOfBase(9, 3));
    EXPECT_DOUBLE_EQ(1, logOfBase(const_e, const_e));
    EXPECT_DOUBLE_EQ(2, logOfBase(const_e * const_e, const_e));
    EXPECT_DOUBLE_EQ(3, logOfBase(1000, 10));
    EXPECT_DOUBLE_EQ(5, logOfBase(16807, 7));
    EXPECT_DOUBLE_EQ(4, logOfBase(16777216, 64));
    EXPECT_FLOAT_EQ(4, logOfBase(float(16777216), float(64)));
    EXPECT_DOUBLE_EQ(4, static_cast<double>(logOfBase(static_cast<long double>(16777216), static_cast<long double>(64))));

    EXPECT_DOUBLE_EQ(-3, logOfBase(8, 0.5));
    EXPECT_DOUBLE_EQ(-6, logOfBase(1e6, 0.1));

    EXPECT_TRUE(isNan(logOfBase(1, 0)));
    EXPECT_TRUE(isNan(logOfBase(1, -1)));
    EXPECT_TRUE(isNan(logOfBase(-2, 2)));
}

TEST(dfgMath, isIntegerValued)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);

    // Testing integer types
    {
        EXPECT_TRUE(true);
        EXPECT_TRUE(isIntegerValued(1));
        EXPECT_TRUE(isIntegerValued(1ull));
    }

    {
        EXPECT_TRUE(isIntegerValued(0.0f));
        EXPECT_TRUE(isIntegerValued(0.0));
        EXPECT_TRUE(isIntegerValued(1.0f));
        EXPECT_TRUE(isIntegerValued(1.0));
        EXPECT_TRUE(isIntegerValued(-1.0f));
        EXPECT_TRUE(isIntegerValued(-1.0));

        EXPECT_FALSE(isIntegerValued(-0.99999f));
        EXPECT_FALSE(isIntegerValued(-0.999999));

        EXPECT_FALSE(isIntegerValued(std::numeric_limits<float>::epsilon()));
        EXPECT_FALSE(isIntegerValued(std::numeric_limits<double>::epsilon()));

        EXPECT_TRUE(isIntegerValued(double(NumericTraits<int>::minValue)));
        EXPECT_TRUE(isIntegerValued(double(NumericTraits<int>::maxValue)));

        EXPECT_TRUE(isIntegerValued(std::numeric_limits<float>::lowest()));
        EXPECT_FALSE(isIntegerValued(std::numeric_limits<float>::min()));
        EXPECT_TRUE(isIntegerValued(std::numeric_limits<float>::max()));
        EXPECT_TRUE(isIntegerValued(std::numeric_limits<double>::lowest()));
        EXPECT_FALSE(isIntegerValued(std::numeric_limits<double>::min()));
        EXPECT_TRUE(isIntegerValued(std::numeric_limits<double>::max()));

        EXPECT_FALSE(isIntegerValued(std::numeric_limits<float>::quiet_NaN()));
        EXPECT_FALSE(isIntegerValued(std::numeric_limits<double>::quiet_NaN()));

        EXPECT_FALSE(isIntegerValued(-std::numeric_limits<float>::infinity()));
        EXPECT_FALSE(isIntegerValued(-std::numeric_limits<double>::infinity()));
        EXPECT_FALSE(isIntegerValued(std::numeric_limits<float>::infinity()));
        EXPECT_FALSE(isIntegerValued(std::numeric_limits<double>::infinity()));
    }
}

TEST(dfgMath, isFloatConvertibleTo)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);

    /*
    int8_min	= -128
    int16_min   = -32768
    int32_min   = -2147483648
    int64_min   = -9223372036854775808

    int8_max    = 127
    int16_max   = 32767
    int32_max   = 2147483647 = 2.1xxe9
    int64_max   = 9223372036854775807 = 9.2xxxe18

    uint8_max  = 255
    uint16_max = 65535
    uint32_max = 4294967295
    uint64_max = 18446744073709551615 = 1.8xxe19
    */

    int16 s16;
    uint16 u16;
    int32 s32;
    uint32 u32;
    int64 s64;
    uint64 u64;

    EXPECT_TRUE(isFloatConvertibleTo<uint32>(0.0, &u32));
    EXPECT_EQ(0, u32);
    EXPECT_TRUE(isFloatConvertibleTo(123.0, &u32));
    EXPECT_EQ(123, u32);
    EXPECT_FALSE(isFloatConvertibleTo<uint32>(-123.0, &u32));
    EXPECT_TRUE(isFloatConvertibleTo<int32>(-123.0, &s32));
    EXPECT_EQ(-123, s32);
    EXPECT_FALSE(isFloatConvertibleTo<uint32>(1.25));

    // (u)int16 tests
    EXPECT_FALSE(isFloatConvertibleTo(-32768.0, &u16));
    EXPECT_TRUE(isFloatConvertibleTo(-32768.0, &s16));
    EXPECT_EQ(int16_min, s16);
    EXPECT_TRUE(isFloatConvertibleTo(65535.0f, &u16));
    EXPECT_EQ(65535, u16);
    EXPECT_TRUE(isFloatConvertibleTo(65535.0, &u16));
    EXPECT_EQ(65535, u16);
    EXPECT_FALSE(isFloatConvertibleTo<uint16>(65536.0));

    EXPECT_TRUE(isFloatConvertibleTo(-2147483648.0, &s32));
    EXPECT_EQ(int32_min, s32);
    EXPECT_FALSE(isFloatConvertibleTo<int32>(-2147483649.0));
    EXPECT_TRUE(isFloatConvertibleTo(-2147483649.0, &s64));
    EXPECT_EQ(int64(int32_min) - 1, s64);

    EXPECT_FALSE(isFloatConvertibleTo<int32>(3e9f));
    EXPECT_FALSE(isFloatConvertibleTo<int32>(3e9));
    EXPECT_TRUE(isFloatConvertibleTo<uint32>(3e9f));
    EXPECT_TRUE(isFloatConvertibleTo<uint32>(3e9));
    EXPECT_TRUE(isFloatConvertibleTo<int64>(3e9f, &s64));
    EXPECT_EQ(3000000000, s64);
    EXPECT_TRUE(isFloatConvertibleTo<uint64>(3e9, &u64));
    EXPECT_EQ(3000000000, u64);
    EXPECT_TRUE(isFloatConvertibleTo<int64>(1e15f, &s64));
    EXPECT_EQ(999999986991104, s64); // This obviously is not 1e15, but it's not the conversion that "fails", it's that the 1e15 can't be presented exactly as float so conversion didn't get 1e15 as input.
    EXPECT_FALSE(isFloatConvertibleTo<int64>(1e19f));
    EXPECT_TRUE(isFloatConvertibleTo<uint64>(1e19f, &u64));
    EXPECT_EQ(9999999980506447872ull, u64);
    EXPECT_FALSE(isFloatConvertibleTo<uint64>(2e19));

    EXPECT_TRUE(isFloatConvertibleTo<uint64>(1e15, &u64));
    EXPECT_EQ(1000000000000000ull, u64);

    EXPECT_FALSE(isFloatConvertibleTo<int32>(4294967295.0, &s32));
    EXPECT_TRUE(isFloatConvertibleTo<uint32>(4294967295.0, &u32));
    EXPECT_EQ(uint32_max, u32);
    EXPECT_FALSE(isFloatConvertibleTo<uint32>(4294967296.0, &u32));
}

namespace
{
    template <class T>
    void testnumericDistanceLimits()
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(math);
        using UnsignedT = typename std::make_unsigned<T>::type;
        const auto minValue = NumericTraits<T>::minValue;
        const auto maxValue = NumericTraits<T>::maxValue;
        const auto minUValue = NumericTraits<UnsignedT>::minValue;
        const auto maxUValue = NumericTraits<UnsignedT>::maxValue;
        
        EXPECT_EQ(maxUValue, numericDistance(minValue, maxValue));
        EXPECT_EQ(maxUValue, numericDistance(maxValue, minValue));
        EXPECT_EQ(maxUValue, numericDistance(minUValue, maxUValue));
        EXPECT_EQ(maxUValue, numericDistance(maxUValue, minUValue));
        EXPECT_EQ(UnsignedT(0), numericDistance(maxValue, maxValue));
        EXPECT_EQ(UnsignedT(0), numericDistance(minValue, minValue));  
    }
}

TEST(dfgMath, numericDistance)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);

    testnumericDistanceLimits<int8>();
    testnumericDistanceLimits<int16>();
    testnumericDistanceLimits<int32>();
    testnumericDistanceLimits<int64>();

    EXPECT_EQ(uint8(254), numericDistance(int8(-127), int8(127)));
    EXPECT_EQ(uint8(128), numericDistance(int8(-1), int8(127)));
    EXPECT_EQ(uint8(128), numericDistance(int8(127), int8(-1)));
    EXPECT_EQ(uint8(128), numericDistance(int8(0), int8(-128)));
    EXPECT_EQ(uint8(128), numericDistance(int8(1), int8(-127)));
    EXPECT_EQ(uint8(10), numericDistance(int8(12), int8(2)));
    EXPECT_EQ(uint8(127), numericDistance(int8(127), int8(0)));
    EXPECT_EQ(uint8(127), numericDistance(int8(-128), int8(-1)));
}

namespace
{
    static int globalValue = 0;
}

TEST(dfgMath, FormulaParser)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);

    {
        FormulaParser parser;
        double x = 1;
        double dummy = std::numeric_limits<double>::quiet_NaN();
        EXPECT_TRUE(parser.defineVariable("x", &dummy));
        EXPECT_TRUE(parser.defineVariable("x", &x)); // Redefining should simply overwrite existing.
        EXPECT_TRUE(parser.setFormula("x+2*3"));
        EXPECT_EQ(7, parser.evaluateFormulaAsDouble());
        EXPECT_EQ(7, FormulaParser::evaluateFormulaAsDouble("1+2*3"));

        EXPECT_TRUE(parser.defineConstant("constVal", 20));
        EXPECT_TRUE(parser.defineConstant("constVal", 10)); // Redefining should simply overwrite existing.
        parser.setFormula("1 + constVal + 2*constVal + x");
        EXPECT_EQ(32, parser.evaluateFormulaAsDouble());
        EXPECT_FALSE(parser.defineConstant("x", 30)); // Trying to define a constant when there's already a variable with the same identifier should fail.

        EXPECT_FALSE(parser.defineVariable("constVal", &x)); // Should fail as a constant with the same identifier already exists.
        EXPECT_FALSE(parser.defineVariable("", &x)); // Invalid variable symbol
        EXPECT_FALSE(parser.defineVariable("y", nullptr)); // Invalid variable pointer
        EXPECT_TRUE(isNan(FormulaParser::evaluateFormulaAsDouble("2+-*/6")));
    }

    // defineFunction
    {
        FormulaParser parser;
        EXPECT_TRUE(parser.defineFunction("f0", []() {return 2.0; }, true));
        EXPECT_TRUE(parser.defineFunction("f0", []() {return 1.0; }, true)); // Redefining should simply overwrite existing.
        EXPECT_TRUE(parser.defineFunction("f1", [](double a) {return a; }, true));
        EXPECT_TRUE(parser.defineFunction("f2", [](double a, double b) {return a + b; }, true));
        EXPECT_TRUE(parser.defineFunction("f3", [](double a, double b, double c) {return a + b + c; }, true));
        EXPECT_TRUE(parser.defineFunction("f4", [](double a, double b, double c, double d) {return a + b + c + d; }, true));
        EXPECT_TRUE(parser.defineFunction("fglobal", [](double a) -> double{ return a + globalValue++; }, false));

        EXPECT_EQ(1,  parser.setFormulaAndEvaluateAsDouble("f0()"));
        EXPECT_EQ(2,  parser.setFormulaAndEvaluateAsDouble("f1(2)"));
        EXPECT_EQ(5,  parser.setFormulaAndEvaluateAsDouble("f2(2, 3)"));
        EXPECT_EQ(9,  parser.setFormulaAndEvaluateAsDouble("f3(2, 3, 4)"));
        EXPECT_EQ(14, parser.setFormulaAndEvaluateAsDouble("f4(2, 3, 4, 5)"));
        EXPECT_EQ(1,  parser.setFormulaAndEvaluateAsDouble("fglobal(0)+fglobal(0)")); // Tests that fglobal(0) is not optimized to single call.
        EXPECT_TRUE(isNan(parser.setFormulaAndEvaluateAsDouble("f0(2)")));
    }
}
