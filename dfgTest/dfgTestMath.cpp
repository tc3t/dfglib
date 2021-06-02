#include "stdafx.h"

#if (DFGTEST_BUILD_MODULE_DEFAULT == 1)

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
#include <dfg/str.hpp>
#include <dfg/cont/contAlg.hpp>
#include <numeric>
#include <thread>

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

    // Testing that converting to self returns true and sets right value to output arg.
    {
        float f = 0;
        double d = 0;
        long double ld = 0;
        EXPECT_TRUE(isFloatConvertibleTo<float>(1.0f, &f));
        EXPECT_TRUE(isFloatConvertibleTo<double>(1.0, &d));
        EXPECT_TRUE(isFloatConvertibleTo<long double>(static_cast<long double>(1.0), &ld));
        EXPECT_EQ(1, f);
        EXPECT_EQ(1, d);
        EXPECT_EQ(1, ld);
    }
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

        // Making sure that multiexpression formulas evaluate to NaN.
        DFGTEST_EXPECT_NAN(parser.evaluateFormulaAsDouble("1,5+2"));
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

    // Error handling
    {
        FormulaParser parser;
        FormulaParser::ReturnStatus rv;
        parser.evaluateFormulaAsDouble(&rv);
        EXPECT_FALSE(rv);
        EXPECT_EQ(2, rv.errorCode()); // This is implementation detail.
        EXPECT_FALSE(rv.errorString().empty());
        parser.setFormula("1");
        EXPECT_EQ(1, parser.evaluateFormulaAsDouble(&rv));
        EXPECT_TRUE(rv);
        EXPECT_TRUE(rv.errorString().empty());
        rv = parser.defineVariable("x", nullptr);
        EXPECT_FALSE(rv);
        EXPECT_TRUE(rv.internalErrorObjectPtr() != nullptr);
        double x;
        rv = parser.defineVariable("x", &x);
        EXPECT_TRUE(rv);
        EXPECT_TRUE(rv.errorString().empty());
        rv = parser.defineConstant("x", 1);
        EXPECT_FALSE(rv);
        rv = parser.defineFunction("+-*/+-1 ab < >", [](double) { return 1.0; }, false);
        EXPECT_FALSE(rv);
        EXPECT_EQ(18 , rv.errorCode()); // This is implementation detail.
        EXPECT_FALSE(rv.errorString().empty());
    }
}

TEST(dfgMath, FormulaParser_functors)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);
    using namespace DFG_MODULE_NS(str);

    const auto nMaxFunctorCount = FormulaParser::maxFunctorCountPerType();

    // Basic tests
    {
        double a = 1, b = 2, c = 3;
        FormulaParser parser;
        parser.defineFunctor("f0" , [=]() { return a; }, true);
        parser.defineFunctor("f1" , [=](double a0) { return b + a0; }, true);
        parser.defineFunctor("f2" , [=](double a0, double a1) { return c + a0 + a1; }, true);
        parser.defineFunctor("fr0", [&]() { return a; }, false);
        a = 5; // This should affect fr0, but not f0.
        EXPECT_EQ(1,   parser.setFormulaAndEvaluateAsDouble("f0()"));
        EXPECT_EQ(12,  parser.setFormulaAndEvaluateAsDouble("f1(10)"));
        EXPECT_EQ(323, parser.setFormulaAndEvaluateAsDouble("f2(20, 300)"));
        EXPECT_EQ(5,   parser.setFormulaAndEvaluateAsDouble("fr0()"));
    }

    // Testing functor limits
    {
        FormulaParser parser;
        for (uint32 i = 0; i < nMaxFunctorCount; ++i)
        {
            EXPECT_TRUE(parser.defineFunctor("f0_" + toStrC(i), [=]() { return i; }, true));
            EXPECT_TRUE(parser.defineFunctor("f1_" + toStrC(i), [=](double) { return i; }, true));
            EXPECT_TRUE(parser.defineFunctor("f2_" + toStrC(i), [=](double, double) { return i; }, true));
        }
        // These calls are expected to fail as max count has been added already.
        EXPECT_FALSE(parser.defineFunctor("f0", []() { return 0.0; }, true));
        EXPECT_FALSE(parser.defineFunctor("f1", [](double) { return 0.0; }, true));
        EXPECT_FALSE(parser.defineFunctor("f2", [](double, double) { return 0.0; }, true));
    }

    // Testing that clean up works correctly when there are multiple parsers.
    {
        FormulaParser parser0;
        {
            FormulaParser parser1;
            DFG_STATIC_ASSERT(nMaxFunctorCount % 2 == 0, "This test expects even max functor count");
            for (uint32 i = 0; i < nMaxFunctorCount / 2; ++i)
            {
                EXPECT_TRUE(parser0.defineFunctor("f0_" + toStrC(i), [=]() { return i; }, true));
                EXPECT_TRUE(parser1.defineFunctor("f0_" + toStrC(i), [=]() { return i; }, true));
                EXPECT_TRUE(parser0.defineFunctor("f1_" + toStrC(i), [=](double) { return i; }, true));
                EXPECT_TRUE(parser1.defineFunctor("f1_" + toStrC(i), [=](double) { return i; }, true));
                EXPECT_TRUE(parser0.defineFunctor("f2_" + toStrC(i), [=](double, double) { return i; }, true));
                EXPECT_TRUE(parser1.defineFunctor("f2_" + toStrC(i), [=](double, double) { return i; }, true));
            }
            EXPECT_FALSE(parser0.defineFunctor("f0", []() { return 0.0; }, true));
            EXPECT_FALSE(parser1.defineFunctor("f0", []() { return 0.0; }, true));
        }
        for (uint32 i = 0; i < nMaxFunctorCount / 2; ++i)
        {
            EXPECT_TRUE(parser0.defineFunctor("f0_a" + toStrC(i), [=]() { return i; }, true));
            EXPECT_TRUE(parser0.defineFunctor("f1_a" + toStrC(i), [=](double) { return i; }, true));
            EXPECT_TRUE(parser0.defineFunctor("f2_a" + toStrC(i), [=](double, double) { return i; }, true));
        }
    }

    // Adding max count again to make sure that destruction of previous FormulaParsers has correctly cleaned up functors.
    {
        FormulaParser parser;
        for (uint32 i = 0; i < nMaxFunctorCount; ++i)
        {
            // Trying to add some invalid functors to make sure that they don't consume resources.
            EXPECT_FALSE(parser.defineFunctor("i + n - v * a / l ^ i + d", [=]() { return i; }, true));
            EXPECT_FALSE(parser.defineFunctor("i + n - v * a / l ^ i + d", [=](double) { return i; }, true));
            EXPECT_FALSE(parser.defineFunctor("i + n - v * a / l ^ i + d", [=](double, double) { return i; }, true));

            EXPECT_TRUE(parser.defineFunctor("f0_" + toStrC(i), [=]() { return i; }, true));
            EXPECT_TRUE(parser.defineFunctor("f1_" + toStrC(i), [=](double) { return i; }, true));
            EXPECT_TRUE(parser.defineFunctor("f2_" + toStrC(i), [=](double, double) { return i; }, true));
        }
    }
}

TEST(dfgMath, FormulaParser_randomFunctions)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);
    {
        FormulaParser parser;
        EXPECT_TRUE(parser.defineRandomFunctions());

        DFGTEST_EXPECT_WITHIN_GE_LE(parser.setFormulaAndEvaluateAsDouble("rand_uniformInt(100, 200)"), 100, 200);
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_uniformInt(100, 99)"));

        DFGTEST_EXPECT_WITHIN_GE_LE(parser.setFormulaAndEvaluateAsDouble("rand_binomial(10, 0.5)"), 0, 10);
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_binomial(10, 1.1)"));

        DFGTEST_EXPECT_WITHIN_GE_LE(parser.setFormulaAndEvaluateAsDouble("rand_bernoulli(0.5)"), 0, 1);
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_bernoulli(1.1)"));

        DFGTEST_EXPECT_WITHIN_GE_LE(parser.setFormulaAndEvaluateAsDouble("rand_negBinomial(10, 0.5)"), 0, (std::numeric_limits<int>::max)());
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_negBinomial(10, 0)"));

        DFGTEST_EXPECT_WITHIN_GE_LE(parser.setFormulaAndEvaluateAsDouble("rand_geometric(0.5)"), 0, (std::numeric_limits<int>::max)());
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_geometric(1.01)"));

        DFGTEST_EXPECT_WITHIN_GE_LE(parser.setFormulaAndEvaluateAsDouble("rand_poisson(10)"), 0, (std::numeric_limits<int>::max)());
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_poisson(-1)"));

        const auto udouble0 = parser.setFormulaAndEvaluateAsDouble("rand_uniformReal(-1e300, 1e300)");
        const auto udouble1 = parser.evaluateFormulaAsDouble();
        DFGTEST_EXPECT_NON_NAN(udouble0);
        DFGTEST_EXPECT_NON_NAN(udouble1);
        EXPECT_TRUE(udouble0 != udouble1); // This is critical test: failing means that parser most likely had erroneously optimized call to constant.
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_uniformReal(1e300, -1e300)"));

        DFGTEST_EXPECT_NON_NAN(parser.setFormulaAndEvaluateAsDouble("rand_normal(0.0, 1.0)"));
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_normal(0.0, -1.0)"));

        DFGTEST_EXPECT_NON_NAN(parser.setFormulaAndEvaluateAsDouble("rand_exponential(1.0)"));
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_exponential(0.0)"));

        DFGTEST_EXPECT_NON_NAN(parser.setFormulaAndEvaluateAsDouble("rand_gamma(1.0, 2.0)"));
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_gamma(0.0, 1.0)"));

        DFGTEST_EXPECT_NON_NAN(parser.setFormulaAndEvaluateAsDouble("rand_weibull(1.0, 2.0)"));
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_weibull(0.0, 1.0)"));

        DFGTEST_EXPECT_NON_NAN(parser.setFormulaAndEvaluateAsDouble("rand_extremeValue(-1.0, 2.0)"));
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_extremeValue(-1.0, 0.0)"));

        DFGTEST_EXPECT_NON_NAN(parser.setFormulaAndEvaluateAsDouble("rand_logNormal(-1.0, 2.0)"));
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_logNormal(-1.0, 0.0)"));

        DFGTEST_EXPECT_NON_NAN(parser.setFormulaAndEvaluateAsDouble("rand_chiSquared(1.0)"));
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_chiSquared(0.0)"));

        DFGTEST_EXPECT_NON_NAN(parser.setFormulaAndEvaluateAsDouble("rand_cauchy(-1.0, 2.0)"));
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_cauchy(-1.0, 0.0)"));

        DFGTEST_EXPECT_NON_NAN(parser.setFormulaAndEvaluateAsDouble("rand_fisherF(1.0, 2.0)"));
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_fisherF(0.0, 2.0)"));

        DFGTEST_EXPECT_NON_NAN(parser.setFormulaAndEvaluateAsDouble("rand_studentT(1.0)"));
        DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("rand_studentT(0.0)"));
    }
}

TEST(dfgMath, FormulaParser_forEachDefinedFunctionNameWhile)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);

    {
        std::vector<const char*> expectedFunctions {
            "abs", "acos", "acosh", "asin", "asinh", "atan", "atan2", "atanh", "avg", "cbrt",
            "cos", "cosh", "erf", "erfc", "exp", "hypot", "ln", "log", "log10", "log2", "max", "min", "rand_bernoulli",
            "rand_binomial", "rand_cauchy", "rand_chiSquared", "rand_exponential", "rand_extremeValue", "rand_fisherF",
            "rand_gamma", "rand_geometric", "rand_logNormal", "rand_negBinomial", "rand_normal", "rand_poisson", 
            "rand_studentT", "rand_uniformInt", "rand_uniformReal", "rand_weibull",
            "rint", "sign", "sin", "sinh", "sqrt", "sum", "tan", "tanh", "tgamma", "time_epochMsec"
            };
#if defined(__cpp_lib_math_special_functions) ||  (defined(__STDCPP_MATH_SPEC_FUNCS__) && (__STDCPP_MATH_SPEC_FUNCS__ >= 201003L))
        expectedFunctions.push_back("assoc_laguerre");
        expectedFunctions.push_back("assoc_legendre");
        expectedFunctions.push_back("beta");
        expectedFunctions.push_back("comp_ellint_1");
        expectedFunctions.push_back("comp_ellint_2");
        expectedFunctions.push_back("comp_ellint_3");
        expectedFunctions.push_back("cyl_bessel_i");
        expectedFunctions.push_back("cyl_bessel_j");
        expectedFunctions.push_back("cyl_bessel_k");
        expectedFunctions.push_back("cyl_neumann");
        expectedFunctions.push_back("ellint_1");
        expectedFunctions.push_back("ellint_2");
        expectedFunctions.push_back("ellint_3");
        expectedFunctions.push_back("expint");
        expectedFunctions.push_back("gcd");
        expectedFunctions.push_back("hermite");
        expectedFunctions.push_back("laguerre");
        expectedFunctions.push_back("legendre");
        expectedFunctions.push_back("lcm");
        expectedFunctions.push_back("riemann_zeta");
        expectedFunctions.push_back("sph_bessel");
        expectedFunctions.push_back("sph_legendre");
        expectedFunctions.push_back("sph_neumann");
        std::sort(expectedFunctions.begin(), expectedFunctions.end(), [](const char* a, const char* b) { return (std::strcmp(a, b) < 0); });
#endif

        std::vector<std::string> foundFunctions;
        FormulaParser parser;
        EXPECT_TRUE(parser.defineRandomFunctions());
        parser.forEachDefinedFunctionNameWhile([&](const StringViewC& sv)
        {
            foundFunctions.push_back(sv.toString());
            return true;
        });
        EXPECT_TRUE(::DFG_MODULE_NS(cont)::isEqualContent(expectedFunctions, foundFunctions));
    }
}

TEST(dfgMath, FormulaParser_cmath)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);
    FormulaParser parser;

#define DFGTEST_TEMP_TEST_VALUE1(FUNC, a)       ASSERT_DOUBLE_EQ(std::FUNC(a),       parser.setFormulaAndEvaluateAsDouble(#FUNC "(" #a ")"))
#define DFGTEST_TEMP_TEST_VALUE2(FUNC, a, b)    ASSERT_DOUBLE_EQ(std::FUNC(a,b),     parser.setFormulaAndEvaluateAsDouble(#FUNC "(" #a ", " #b ")"))
#define DFGTEST_TEMP_TEST_VALUE3(FUNC, a, b, c) ASSERT_DOUBLE_EQ(std::FUNC(a, b, c), parser.setFormulaAndEvaluateAsDouble(#FUNC "(" #a ", " #b ", " #c ")"))

    DFGTEST_TEMP_TEST_VALUE1(cbrt, 3);
    DFGTEST_TEMP_TEST_VALUE1(erf, 3);
    DFGTEST_TEMP_TEST_VALUE1(erfc, 3);
    DFGTEST_TEMP_TEST_VALUE2(hypot, 1, 2);
    DFGTEST_TEMP_TEST_VALUE1(tgamma, 3);

#if defined(__cpp_lib_math_special_functions) ||  (defined(__STDCPP_MATH_SPEC_FUNCS__) && (__STDCPP_MATH_SPEC_FUNCS__ >= 201003L))
    DFGTEST_TEMP_TEST_VALUE3(assoc_laguerre, 1, 2, 0.5);
    DFGTEST_TEMP_TEST_VALUE3(assoc_legendre, 1, 2, 0.5);
    DFGTEST_TEMP_TEST_VALUE2(beta, 1, 2);
    DFGTEST_TEMP_TEST_VALUE1(comp_ellint_1, 0.5);
    DFGTEST_TEMP_TEST_VALUE1(comp_ellint_2, 0.5);
    DFGTEST_TEMP_TEST_VALUE2(comp_ellint_3, 0.25, 0.5);
    DFGTEST_TEMP_TEST_VALUE2(cyl_bessel_i, 1, 2);
    DFGTEST_TEMP_TEST_VALUE2(cyl_bessel_j, 1, 2);
    DFGTEST_TEMP_TEST_VALUE2(cyl_bessel_k, 1, 2);
    DFGTEST_TEMP_TEST_VALUE2(cyl_neumann, 1, 2);
    DFGTEST_TEMP_TEST_VALUE2(ellint_1, 0.5, 2);
    DFGTEST_TEMP_TEST_VALUE2(ellint_2, 1, 2);
    DFGTEST_TEMP_TEST_VALUE3(ellint_3, 0.5, 0.3, 3);
    DFGTEST_TEMP_TEST_VALUE1(expint, 3);
    DFGTEST_TEMP_TEST_VALUE2(gcd, 48564, -156165);
    DFGTEST_TEMP_TEST_VALUE2(hermite, 1, 2);
    DFGTEST_TEMP_TEST_VALUE2(laguerre, 1, 2);
    DFGTEST_TEMP_TEST_VALUE2(legendre, 1, 0.5);
    DFGTEST_TEMP_TEST_VALUE2(lcm, 23, -75);
    DFGTEST_TEMP_TEST_VALUE1(riemann_zeta, 3);
    DFGTEST_TEMP_TEST_VALUE2(sph_bessel, 1, 3);
    DFGTEST_TEMP_TEST_VALUE3(sph_legendre, 1, 2, 3);
    DFGTEST_TEMP_TEST_VALUE2(sph_neumann, 1, 3);

    DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("assoc_laguerre(1.5, 2, 3)")); // Non-integer in place where integer is expected -> NaN expected as result
    DFGTEST_EXPECT_NAN(parser.setFormulaAndEvaluateAsDouble("gcd(1.5, 2)")); // Non-integer in place where integer is expected -> NaN expected as result
#endif


#undef DFGTEST_TEMP_TEST_VALUE1
#undef DFGTEST_TEMP_TEST_VALUE2
#undef DFGTEST_TEMP_TEST_VALUE3
}

TEST(dfgMath, FormulaParser_time_epochMsec)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(math);
    {
        FormulaParser parser;
        EXPECT_TRUE(parser.setFormula("time_epochMsec()"));
        const auto t0 = parser.evaluateFormulaAsDouble();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const auto t1 = parser.evaluateFormulaAsDouble();
        EXPECT_GT(t1 - t0, 50);
    }
}

#endif

