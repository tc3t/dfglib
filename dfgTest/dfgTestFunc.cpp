#include "stdafx.h"

#if (DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/funcAll.hpp>
#include <dfg/alg.hpp>
#include <array>
#include <functional> 
#include <dfg/math.hpp>
#include <dfg/rand.hpp>
#include <dfg/numeric/median.hpp>

TEST(dfgFunc, MemFuncMinMax)
{
    using namespace DFG_ROOT_NS;

    DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncMinMax)<double> mfMinMax;
    EXPECT_FALSE(mfMinMax.isValid());

    EXPECT_EQ(mfMinMax.minValue(), DFG_MODULE_NS(func)::DFG_DETAIL_NS::initialValueForMin<double>());
    EXPECT_EQ(mfMinMax.maxValue(), DFG_MODULE_NS(func)::DFG_DETAIL_NS::initialValueForMax<double>());
    EXPECT_GT(std::numeric_limits<double>::lowest(), mfMinMax.maxValue());
    EXPECT_LT(std::numeric_limits<double>::max(), mfMinMax.minValue());

    DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncMinMax)<double> mfMinMaxFloat;
    EXPECT_FALSE(mfMinMaxFloat.isValid());
    EXPECT_EQ(mfMinMaxFloat.minValue(), DFG_MODULE_NS(func)::DFG_DETAIL_NS::initialValueForMin<float>());
    EXPECT_EQ(mfMinMaxFloat.maxValue(), DFG_MODULE_NS(func)::DFG_DETAIL_NS::initialValueForMax<float>());
    EXPECT_GT(std::numeric_limits<float>::lowest(), mfMinMaxFloat.maxValue());
    EXPECT_LT(std::numeric_limits<float>::max(), mfMinMaxFloat.minValue());

    DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncMinMax)<long double> mfMinMaxLd;
    EXPECT_EQ(mfMinMaxLd.minValue(), DFG_MODULE_NS(func)::DFG_DETAIL_NS::initialValueForMin<long double>());
    EXPECT_EQ(mfMinMaxLd.maxValue(), DFG_MODULE_NS(func)::DFG_DETAIL_NS::initialValueForMax<long double>());
    EXPECT_GT(std::numeric_limits<long double>::lowest(), mfMinMaxLd.maxValue());
    EXPECT_LT(std::numeric_limits<long double>::max(), mfMinMaxLd.minValue());

    std::array<double, 6> testArray = {1.2, 3.6, -5.78, 6.3, 1e9, 1e-21};
    EXPECT_FALSE(mfMinMax.isValid());
    mfMinMax(1.0);
    EXPECT_TRUE(mfMinMax.isValid());
    DFG_MODULE_NS(alg)::forEachFwdFuncRef(testArray, mfMinMax);
    EXPECT_EQ(mfMinMax.minValue(), -5.78);
    EXPECT_EQ(mfMinMax.maxValue(), 1e9);

    DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncMinMax)<uint64> mfMinMaxUint64;
    EXPECT_EQ(mfMinMaxUint64.minValue(), DFG_MODULE_NS(func)::DFG_DETAIL_NS::initialValueForMin<uint64>());
    EXPECT_EQ(mfMinMaxUint64.maxValue(), DFG_MODULE_NS(func)::DFG_DETAIL_NS::initialValueForMax<uint64>());
    EXPECT_EQ(std::numeric_limits<uint64>::lowest(), mfMinMaxUint64.maxValue());
    EXPECT_EQ(std::numeric_limits<uint64>::max(), mfMinMaxUint64.minValue());
    std::array<uint64, 6> testArrayUint64 = {5652, 2, 9848916213, 9, 9151515615, 3669941};
    DFG_MODULE_NS(alg)::forEachFwdFuncRef(testArrayUint64, mfMinMaxUint64);
    EXPECT_TRUE(mfMinMaxUint64.isValid());
    EXPECT_EQ(mfMinMaxUint64.minValue(), 2);
    EXPECT_EQ(mfMinMaxUint64.maxValue(), 9848916213);
    EXPECT_EQ(mfMinMaxUint64.diff(), 9848916213 - 2);
}

TEST(dfgFunc, MemFuncAvg)
{
    // Test return value for empty.
    {
        DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncAvg)<int> avgI;
        DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncAvg)<float> avgF;
        DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncAvg)<double> avgD;
        EXPECT_EQ(int(), avgI.average());
        EXPECT_TRUE(DFG_MODULE_NS(math)::isNan(avgF.average()));
        EXPECT_TRUE(DFG_MODULE_NS(math)::isNan(avgD.average()));
    }

    {
        const double vals[] = { 1, 2, 3, 4, 5 };
        DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncAvg)<double> avg;
        DFG_MODULE_NS(alg)::forEachFwdFuncRef(vals, avg);
        EXPECT_EQ(avg.average(), 3);
    }

    {
        const int valsInt[] = { 1, 2 };
        DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncAvg)<int, double> avg;
        DFG_MODULE_NS(alg)::forEachFwdFuncRef(valsInt, avg);
        EXPECT_EQ(1.5, avg.average());
    }
}

TEST(dfgFunc, MemFuncMedian)
{
    using namespace DFG_MODULE_NS(func);
    using namespace DFG_MODULE_NS(alg);
    using namespace DFG_MODULE_NS(math);
    using namespace DFG_MODULE_NS(numeric);

    {
        DFG_CLASS_NAME(MemFuncMedian)<double> mfMedian;
        const std::array<double, 5> arr = { 9, 6, 3, 5, 1 };
        const std::array<double, 5> expectedMedian = { 9, 7.5, 6, 5.5, 5 };
        EXPECT_TRUE(isNan(mfMedian.median()));
        for (size_t i = 0; i < arr.size(); ++i)
        {
            mfMedian(arr[i]);
            EXPECT_EQ(expectedMedian[i], mfMedian.median());
        }
    }

    // Test with random data and verify that the result is the same as with numeric::median().
    {
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        const size_t nSize = 501;
        std::vector<double> vals;
        DFG_CLASS_NAME(MemFuncMedian)<double> mfMedian;
        for (size_t i = 0; i < nSize; ++i)
        {
            vals.push_back(DFG_MODULE_NS(rand)::rand(randEng, -200.0, 50.0));
            mfMedian(vals.back());
            EXPECT_EQ(median(vals), mfMedian.median());
        }
    }
}

TEST(dfgFunc, CastStatic)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(func);

    EXPECT_EQ((CastStatic<int, char>()(58)), 58);
    EXPECT_EQ((CastStatic<char, int>()(-86)), -86);
    EXPECT_EQ((CastStatic<size_t, int32>()(uint32_max)), static_cast<int>(size_t(uint32_max)));
}

TEST(dfgFunc, MultiUnaryCaller)
{
    using namespace DFG_MODULE_NS(func);

    
    
    DFG_CLASS_NAME(MultiUnaryCaller)<std::tuple<DFG_CLASS_NAME(MemFuncSum)<int, int>,
                                                DFG_CLASS_NAME(MemFuncMax)<int>,
                                                DFG_CLASS_NAME(MemFuncMin)<int>,
                                                DFG_CLASS_NAME(MemFuncSquareSum)<int, int>>,
                                                int> compo;
    std::vector<int> a;
    a.push_back(5);
    a.push_back(8);
    a.push_back(2);
    compo = DFG_MODULE_NS(alg)::forEachFwd(a, compo);
    //compo = std::for_each(a.begin(), a.end(), compo);
    //compo = std::for_each(a.begin(), a.end(), std::ref(compo));
    EXPECT_EQ(15, compo.get<0>().value());
    EXPECT_EQ(8, compo.get<1>().value());
    EXPECT_EQ(2, compo.get<2>().value());
    EXPECT_EQ(93, compo.get<3>().sum());
}

TEST(dfgFunc, forEachInTuple)
{
    using namespace DFG_MODULE_NS(func);

    {
        std::tuple<> emptyTuple;

        forEachInTuple(emptyTuple, [](int& a) {a++; });
    }

    {
        std::tuple<int> intTuple(0);

        forEachInTuple(intTuple, [](int& a) {a++; });

        EXPECT_EQ(1, std::get<0>(intTuple));
    }

    {
        std::tuple<int, int> intTuple(0, 10);

        forEachInTuple(intTuple, [](int& a) {a++; });

        EXPECT_EQ(1, std::get<0>(intTuple));
        EXPECT_EQ(11, std::get<1>(intTuple));
    }

    {
        std::tuple<int, int, int> intTuple(0, 10, 100);

        forEachInTuple(intTuple, [](int& a) {a++; });

        EXPECT_EQ(1, std::get<0>(intTuple));
        EXPECT_EQ(11, std::get<1>(intTuple));
        EXPECT_EQ(101, std::get<2>(intTuple));
    }
}

#endif
