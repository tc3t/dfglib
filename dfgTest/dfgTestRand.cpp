#include <stdafx.h>
#include <dfg/rand.hpp>
#include <dfg/func.hpp>
#include <ctime>
#include <unordered_set>

TEST(dfgRand, dfgRand)
{
    using namespace DFG_ROOT_NS;
    std::mt19937 randEng((unsigned long)(time(nullptr)));
    auto distrEng = DFG_ROOT_NS::DFG_SUB_NS_NAME(rand)::makeDistributionEngineUniform(&randEng, int(-3), int(3));
    auto distrEngSizet = DFG_ROOT_NS::DFG_SUB_NS_NAME(rand)::makeDistributionEngineUniform(&randEng, size_t(0), size_t(3));

    // Test that one can easily set value type parameter.
    DFGTEST_STATIC((std::is_same<decltype(distrEngSizet), decltype(DFG_ROOT_NS::DFG_SUB_NS_NAME(rand)::makeDistributionEngineUniform<size_t>(&randEng, 0, 1))>::value == true));

    std::uniform_int_distribution<int> distrStd(-3, 3);
    std::uniform_int_distribution<size_t> distrStdSizet(0, 3);

    DFG_SUB_NS_NAME(func)::DFG_CLASS_NAME(MemFuncMinMax)<size_t> minMaxSizet;
    DFG_SUB_NS_NAME(func)::DFG_CLASS_NAME(MemFuncMinMax)<int> minMax;
    DFG_SUB_NS_NAME(func)::DFG_CLASS_NAME(MemFuncMinMax)<int> minMaxStd;
    DFG_SUB_NS_NAME(func)::DFG_CLASS_NAME(MemFuncMinMax)<size_t> minMaxStdSizet;
    

    for(int i = 0; i<500; ++i)
    {
        minMax(distrEng());
        minMaxStd(distrStd(randEng));
        minMaxSizet(distrEngSizet());
        minMaxStdSizet(distrStdSizet(randEng));
    }

    // Expect that in 500 calls both min and max values will be generated.
    // (tests inclusion of boundaries of int uniform range)
    // This test has already proven it's worth: it catched a bug in VC2010 implementation of std::uniform_int_distribution<int>
    EXPECT_EQ(minMax.minValue(), -3);
    EXPECT_EQ(minMax.maxValue(), 3);
#if (DFG_MSVC_VER != DFG_MSVC_VER_2010)
    EXPECT_EQ(minMaxStd.minValue(), -3); // This fails on VC2010, minValue() returns -2.
#endif
    EXPECT_EQ(minMaxStd.maxValue(), 3);
    EXPECT_EQ(minMaxSizet.minValue(), 0);
    EXPECT_EQ(minMaxSizet.maxValue(), 3);
    EXPECT_EQ(minMaxStdSizet.minValue(), 0);
    EXPECT_EQ(minMaxStdSizet.maxValue(), 3);
    
    // Test that generated value is within limits.
    for(int i = 0; i<1000; ++i)
    {
        auto low = DFG_SUB_NS_NAME(rand)::rand(randEng, -10.0, 10.0);
        auto up = DFG_SUB_NS_NAME(rand)::rand(randEng, low, low + 10.0);
        if (low == up)
            continue;

        const auto val = DFG_SUB_NS_NAME(rand)::rand(randEng, low, up);
        EXPECT_TRUE(low <= val && val <= up);
    }

#ifdef _MSC_VER // rand() is not available in MinGW.
    // Test non-seeded rand.
    EXPECT_NE(DFG_MODULE_NS(rand)::rand(), DFG_MODULE_NS(rand)::rand());
#endif
}

TEST(dfgRand, defaultRandEng)
{
    auto stdRandEng = std::mt19937();
    auto dfgRandEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();

    DFG_STATIC_ASSERT((std::is_same<decltype(stdRandEng), decltype(dfgRandEng)>::value), "Default rand engine in not expected");

    const unsigned long seed = 498321;

    for (unsigned long i = 0; i < 1000; ++i)
    {
        stdRandEng.seed(seed + i);
        dfgRandEng.seed(seed + i);
        const auto valDfg = DFG_MODULE_NS(rand)::rand(dfgRandEng);
        const auto valStd = std::uniform_real_distribution<double>()(stdRandEng);
        // EXPECT_EQ failed in MinGW GCC 4.8.0 release build so use EXPECT_NEAR
        //EXPECT_EQ(valDfg, valStd);
        EXPECT_NEAR(valDfg, valStd, 1e-14);
    }


}

TEST(dfgRand, simpleRand)
{
    std::mt19937 randEng(56489223);
    std::unordered_set<double> vals;
    const auto nCount = 1000;
    for (size_t i = 0; i < nCount; ++i)
        vals.insert(DFG_MODULE_NS(rand)::rand(randEng));
    EXPECT_EQ(vals.size(), nCount);
}
