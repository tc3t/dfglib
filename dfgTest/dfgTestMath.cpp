#include <stdafx.h>
#include <dfg/dfgBaseTypedefs.hpp>
#include <dfg/math.hpp>
#include <dfg/math/pow.hpp>
#include <dfg/math/roundedUpToMultiple.hpp>
#include <dfg/math/interval.hpp>
#include <dfg/math/evalPolynomial.hpp>
#include <dfg/math/interpolationLinear.hpp>
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
        const auto val = DFG_CLASS_NAME(Factorial_T)<n>::value; /*Without this temporary MinGW generated linker errors*/ \
        EXPECT_EQ(factorialInt(n), val); \
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
    EXPECT_EQ(pow2ToX(63), 9223372036854775808);
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
