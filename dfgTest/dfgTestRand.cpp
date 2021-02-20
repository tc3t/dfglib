#include <stdafx.h>
#include <dfg/rand.hpp>
#include <dfg/func.hpp>
#include <ctime>
#include <unordered_set>
#include <dfg/rand/distributionHelpers.hpp>
#include <bitset>

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

namespace
{
    bool returnFalse(DFG_ROOT_NS::Dummy) { return false; }
    bool returnTrue(DFG_ROOT_NS::Dummy) { return true; }

    template <class Distr_T, class TestFunc_T>
    void testDistr(const bool bExpectValid, const char* pArg0, const char* pArg1, const char* pArg2, TestFunc_T testFunc)
    {
        using namespace ::DFG_MODULE_NS(rand);
        auto randEng = createDefaultRandEngineRandomSeeded();

        const char* rawParams[] = { pArg0, pArg1, pArg2 };
        const size_t nArraySize = int(pArg0 != nullptr) + int(pArg1 != nullptr) + int(pArg2 != nullptr);
        auto arrayWrapper = DFG_ROOT_NS::makeRange(rawParams, rawParams + nArraySize);
        auto args = makeDistributionConstructArgs<Distr_T>(arrayWrapper);
        const auto bValidityIsAsExpected = (bExpectValid == args.first);
        EXPECT_TRUE(bValidityIsAsExpected);
        if (!args.first)
            return;
        auto distr = makeDistribution<Distr_T>(args);
        for (int i = 0; i < 5; ++i)
        {
            const auto testValue = distr(randEng);
            EXPECT_TRUE(testFunc(testValue));
        }
    }

    template <class Distr_T>
    void verifyCommonSingleRealFailCases()
    {
        testDistr<Distr_T>(false, nullptr, nullptr, nullptr, returnFalse); // Too few args
        testDistr<Distr_T>(false,     "1",     "1", nullptr, returnFalse); // Too many args
        testDistr<Distr_T>(false,     "1",     "1",     "1", returnFalse); // Too many args
        testDistr<Distr_T>(false,   "inf", nullptr, nullptr, returnFalse); // Invalid arg (inf)
        testDistr<Distr_T>(false,   "nan", nullptr, nullptr, returnFalse); // Invalid arg (NaN)
    }

    template <class Distr_T>
    void verifyCommonDoubleRealFailCases()
    {
        testDistr<Distr_T>(false, nullptr, nullptr, nullptr, returnFalse); // Too few args
        testDistr<Distr_T>(false,     "1",     "1",     "1", returnFalse); // Too many args
        testDistr<Distr_T>(false,   "inf",     "1", nullptr, returnFalse); // Invalid arg (inf)
        testDistr<Distr_T>(false,  "-inf",     "1", nullptr, returnFalse); // Invalid arg (-inf)
        testDistr<Distr_T>(false,   "nan",     "1", nullptr, returnFalse); // Invalid arg (NaN)
        testDistr<Distr_T>(false,     "1",   "inf", nullptr, returnFalse); // Invalid arg (inf)
        testDistr<Distr_T>(false,     "1",  "-inf", nullptr, returnFalse); // Invalid arg (-inf)
        testDistr<Distr_T>(false,     "1",   "nan", nullptr, returnFalse); // Invalid arg (NaN)
    }
} // unnamed namespace

TEST(dfgRand, distributionCreation)
{
    using namespace ::DFG_MODULE_NS(rand);

    // Testing uniform_int_distribution
    {
        typedef std::uniform_int_distribution<int> DistrT;
        testDistr<DistrT>(true,    "200",   "201", nullptr, [](const int v) { return v >= 200 && v <= 201; });
        testDistr<DistrT>(true,     "-1",     "1", nullptr, [](const int v) { return v >= -1 && v <= 1; });
        testDistr<DistrT>(true,   "99.0",   "1e2", nullptr, [](const int v) { return v >= 99 && v <= 100; }); // Parsing integer valued floating point as integer.
        testDistr<DistrT>(false,   "200", nullptr, nullptr, returnFalse); // Too few args
        testDistr<DistrT>(false,   "201",   "200", nullptr, returnFalse); // Invalid probability value (200)
        testDistr<DistrT>(false,    "-1",     "1",     "0", returnFalse); // Too many args
        testDistr<DistrT>(false,  "10.5",  "11.5", nullptr, returnFalse); // Non-integer args
    }

    // Testing that interface allows feeding non-string params directly and making validation manually.
    {
        auto randEng = createDefaultRandEngineRandomSeeded();
        typedef std::uniform_int_distribution<int> DistrT;
        auto args = makeUninitializedDistributionArgs<DistrT>();
        std::get<0>(args.second) = 500;
        std::get<1>(args.second) = 501;
        args.first = validateDistributionParams<DistrT>(args);
        EXPECT_TRUE(args.first);
        const auto testVal = makeDistribution<DistrT>(args)(randEng);
        EXPECT_TRUE(testVal >= 500 && testVal <= 501);
    }

    // Testing binomial_distribution
    {
        typedef std::binomial_distribution<int> DistrT;
        testDistr<DistrT>(true,      "0",     "1", nullptr, [](const int v) { return v == 0; });
        testDistr<DistrT>(true,    "5.0",   "0.5", nullptr, [](const int v) { return v >= 0 && v <= 5; });
        testDistr<DistrT>(false,   "200",   "1.1", nullptr, returnFalse); // Invalid probability value (1.1)
        testDistr<DistrT>(false,    "-1",   "0.1", nullptr, returnFalse); // Invalid count argument (-1)
        testDistr<DistrT>(false,    "-1",     "1",     "0", returnFalse); // Invalid first arg and too many args.
        testDistr<DistrT>(false,    "-1", nullptr, nullptr, returnFalse); // Too few args
    }

    // Testing poisson_distribution
    {
        typedef std::poisson_distribution<int> DistrT;
        testDistr<DistrT>(true,    "0.5", nullptr, nullptr, [](const int v) { return v >= 0; });
        testDistr<DistrT>(true,      "1", nullptr, nullptr, [](const int v) { return v >= 0; });
        testDistr<DistrT>(true,     "50", nullptr, nullptr, [](const int v) { return v >= 0; });
        testDistr<DistrT>(false,     "0", nullptr, nullptr, returnFalse); // Invalid mean (0)
        testDistr<DistrT>(false,   "inf", nullptr, nullptr, returnFalse); // Invalid mean (inf)
        testDistr<DistrT>(false,    "-1", nullptr, nullptr, returnFalse); // Invalid mean (-1)
        testDistr<DistrT>(false,     "1",   "0.1", nullptr, returnFalse); // Too many args
        testDistr<DistrT>(false,    "-1",     "1",     "0", returnFalse); // Too many args and invalid mean
    }

    // Testing bernoulli_distribution
    {
        typedef std::bernoulli_distribution DistrT;
        testDistr<DistrT>(true,      "0", nullptr, nullptr, [](const bool v) { return v == false; });
        testDistr<DistrT>(true,      "1", nullptr, nullptr, [](const bool v) { return v == true; });
        testDistr<DistrT>(true,    "0.5", nullptr, nullptr, returnTrue);
        testDistr<DistrT>(false,   "0.5",     "1", nullptr, returnFalse); // Too many args
        testDistr<DistrT>(false,   "-1",    "0.1", nullptr, returnFalse); // Too many args and invalid first.
        testDistr<DistrT>(false,    "-1",     "1",     "0", returnFalse); // Too many args and invalid first.
        testDistr<DistrT>(false,    "-1", nullptr, nullptr, returnFalse); // Invalid probibility (-1)
    }

    // Testing NegativeBinomialDistribution
    {
        typedef DFG_MODULE_NS(rand)::NegativeBinomialDistribution<int> DistrT; // Note: using a wrapper because std::negative_binomial_distribution doesn't handle p == 1 correctly in any of the checked implementations (as of 2020-01)
        testDistr<DistrT>(true,      "5",   "0.5", nullptr, [](const int v) { return v >= 0; });
        testDistr<DistrT>(true,     "20",     "1", nullptr, [](const int v) { return v == 0; });
        testDistr<DistrT>(true,   "20.0",  "0.99", nullptr, [](const int v) { return v >= 0; });
        testDistr<DistrT>(false,     "0",   "0.5", nullptr, returnFalse); // Invalid count (0)
        testDistr<DistrT>(false,     "1",     "0", nullptr, returnFalse); // Invalid probibility (0)
        testDistr<DistrT>(false,     "1", nullptr, nullptr, returnFalse); // Too few args
        testDistr<DistrT>(false,    "10",     "1",     "0", returnFalse); // Too many args
    }

    // Testing geometric_distribution
    {
        typedef std::geometric_distribution<> DistrT;
        testDistr<DistrT>(true,    "0.5", nullptr, nullptr, [](const int v) { return v >= 0; });
        testDistr<DistrT>(false,     "1", nullptr, nullptr, returnFalse); // Invalid probibility (1)
        testDistr<DistrT>(false,     "0", nullptr, nullptr, returnFalse); // Invalid probibility (0)
        testDistr<DistrT>(false,   "1.1", nullptr, nullptr, returnFalse); // Invalid probibility (1.1)
        verifyCommonSingleRealFailCases<DistrT>();
    }

    // Testing uniform_real_distribution
    {
        typedef std::uniform_real_distribution<double> DistrT;
        testDistr<DistrT>(true,     "200",   "201", nullptr, [](const double v) { return v >= 200 && v < 201; });
        testDistr<DistrT>(true,      "-1",     "1", nullptr, [](const double v) { return v >= -1 && v < 1; });
        testDistr<DistrT>(false,    "201",   "200", nullptr, returnFalse); // Invalid range (max < min)
        testDistr<DistrT>(false, "-1e308", "1e308", nullptr, returnFalse); // Invalid range (max - min is too large)
        testDistr<DistrT>(false,      "0",     "0", nullptr, returnFalse); // Invalid range (min == max)
        verifyCommonDoubleRealFailCases<DistrT>();
    }

    // Testing normal_distribution
    {
        typedef std::normal_distribution<double> DistrT;
        testDistr<DistrT>(true,       "0",     "1", nullptr, returnTrue);
        testDistr<DistrT>(true,      "-1",    "10", nullptr, returnTrue);
        testDistr<DistrT>(false,      "0",     "0", nullptr, returnFalse); // Invalid stddev (0)
        verifyCommonDoubleRealFailCases<DistrT>();
    }

    // Testing cauchy_distribution
    {
        typedef std::cauchy_distribution<double> DistrT;
        testDistr<DistrT>(true,       "0",     "1", nullptr, returnTrue);
        testDistr<DistrT>(true,      "-1",    "10", nullptr, returnTrue);
        testDistr<DistrT>(false,      "0",     "0", nullptr, returnFalse); // Invalid second arg (0)
        verifyCommonDoubleRealFailCases<DistrT>();
    }

    // Testing exponential_distribution
    {
        typedef std::exponential_distribution<double> DistrT;
        testDistr<DistrT>(true,       "1", nullptr, nullptr, returnTrue);
        testDistr<DistrT>(true,     "0.2", nullptr, nullptr, returnTrue);
        testDistr<DistrT>(false,      "0", nullptr, nullptr, returnFalse); // Invalid arg (0)
        verifyCommonSingleRealFailCases<DistrT>();
    }

    // Testing chi_squared_distribution
    {
        typedef std::chi_squared_distribution<double> DistrT;
        testDistr<DistrT>(true,       "1", nullptr, nullptr, [](const double v) { return v > 0; });
        testDistr<DistrT>(true,     "0.2", nullptr, nullptr, [](const double v) { return v > 0; });
        testDistr<DistrT>(false,      "0", nullptr, nullptr, returnFalse); // Invalid arg(0)
        verifyCommonSingleRealFailCases<DistrT>();
    }

    // Testing student_t_distribution
    {
        typedef std::student_t_distribution<double> DistrT;
        testDistr<DistrT>(true,       "1", nullptr, nullptr, returnTrue);
        testDistr<DistrT>(true,     "0.2", nullptr, nullptr, returnTrue);
        testDistr<DistrT>(false,      "0", nullptr, nullptr, returnFalse); // Invalid arg(0)
        verifyCommonSingleRealFailCases<DistrT>();
    }

    // Testing gamma_distribution
    {
        typedef std::gamma_distribution<double> DistrT;
        testDistr<DistrT>(true,       "1",     "1", nullptr, returnTrue);
        testDistr<DistrT>(true,     "0.2",  "0.01", nullptr, returnTrue);
        testDistr<DistrT>(false,      "0",     "1", nullptr, returnFalse); // Invalid first arg(0)
        testDistr<DistrT>(false,      "1",     "0", nullptr, returnFalse); // Invalid second arg(0)
        testDistr<DistrT>(false,      "0",     "0", nullptr, returnFalse); // Invalid args(0, 0)
        verifyCommonDoubleRealFailCases<DistrT>();
    }

    // Testing weibull_distribution
    {
        typedef std::weibull_distribution<double> DistrT;
        testDistr<DistrT>(true,       "1",     "1", nullptr, returnTrue);
        testDistr<DistrT>(true,     "0.2",  "0.01", nullptr, returnTrue);
        testDistr<DistrT>(false,      "0",     "1", nullptr, returnFalse); // Invalid first arg(0)
        testDistr<DistrT>(false,      "1",     "0", nullptr, returnFalse); // Invalid second arg(0)
        testDistr<DistrT>(false,      "0",     "0", nullptr, returnFalse); // Invalid args(0, 0)
        verifyCommonDoubleRealFailCases<DistrT>();
    }

    // Testing extreme_value_distribution
    {
        typedef std::extreme_value_distribution<double> DistrT;
        testDistr<DistrT>(true,       "1",     "1", nullptr, returnTrue);
        testDistr<DistrT>(true,     "0.2",  "0.01", nullptr, returnTrue);
        testDistr<DistrT>(true,       "0",     "1", nullptr, returnTrue);
        testDistr<DistrT>(false,      "1",     "0", nullptr, returnFalse); // Invalid second arg(0)
        verifyCommonDoubleRealFailCases<DistrT>();
    }

    // Testing lognormal_distribution
    {
        typedef std::lognormal_distribution<double> DistrT;
        testDistr<DistrT>(true,       "1",     "1", nullptr, returnTrue);
        testDistr<DistrT>(true,     "0.2",  "0.01", nullptr, returnTrue);
        testDistr<DistrT>(true,       "0",     "1", nullptr, returnTrue);
        testDistr<DistrT>(false,      "1",     "0", nullptr, returnFalse); // Invalid second arg(0)
        verifyCommonDoubleRealFailCases<DistrT>();
    }

    // Testing fisher_f_distribution
    {
        typedef std::fisher_f_distribution<double> DistrT;
        testDistr<DistrT>(true,       "1",     "1", nullptr, returnTrue);
        testDistr<DistrT>(true,     "0.2",  "0.01", nullptr, returnTrue);
        testDistr<DistrT>(false,      "0",     "1", nullptr, returnFalse); // Invalid first arg(0)
        testDistr<DistrT>(false,      "1",     "0", nullptr, returnFalse); // Invalid second arg(0)
        testDistr<DistrT>(false,      "0",     "0", nullptr, returnFalse); // Invalid args(0, 0)
        verifyCommonDoubleRealFailCases<DistrT>();
    }
}

TEST(dfgRand, DistributionFunctor)
{
    using namespace ::DFG_MODULE_NS(rand);
    {
        auto randEng = createDefaultRandEngineRandomSeeded();
        // Integer valued
        DistributionFunctor<std::uniform_int_distribution<int>> distUi(&randEng);
        DistributionFunctor<std::binomial_distribution<int>>    distBin(&randEng);
        DistributionFunctor<std::bernoulli_distribution>        distBer(&randEng);
        DistributionFunctor<NegativeBinomialDistribution<int>>  distNegBin(&randEng);
        DistributionFunctor<std::geometric_distribution<int>>   distGeo(&randEng);
        DistributionFunctor<std::poisson_distribution<int>>     distPoi(&randEng);
        // Real valued
        DistributionFunctor<std::uniform_real_distribution<double>>  distUr(&randEng);
        DistributionFunctor<std::normal_distribution<double>>        distNormal(&randEng);
        DistributionFunctor<std::cauchy_distribution<double>>        distCauchy(&randEng);
        DistributionFunctor<std::exponential_distribution<double>>   distExp(&randEng);
        DistributionFunctor<std::gamma_distribution<double>>         distGamma(&randEng);
        DistributionFunctor<std::weibull_distribution<double>>       distWei(&randEng);
        DistributionFunctor<std::extreme_value_distribution<double>> distExt(&randEng);
        DistributionFunctor<std::lognormal_distribution<double>>     distLog(&randEng);
        DistributionFunctor<std::chi_squared_distribution<double>>   distChi(&randEng);
        DistributionFunctor<std::fisher_f_distribution<double>>      distFisher(&randEng);
        DistributionFunctor<std::student_t_distribution<double>>     distStudent(&randEng);

        DFGTEST_EXPECT_WITHIN_GE_LE(distUi(10.0, 200.0), 10, 200);
        DFGTEST_EXPECT_WITHIN_GE_LE(distUi(10.0, 200.0), 10, 200);
        DFGTEST_EXPECT_WITHIN_GE_LE(distUi(5.0, 8.0)   ,  5, 8);
        DFGTEST_EXPECT_WITHIN_GE_LE(distUi(5.0, 8.0)   ,  5, 8);
        DFGTEST_EXPECT_NAN(distUi(5.5, 8.0));
        DFGTEST_EXPECT_NAN(distUi(5.0, 8.5));

        DFGTEST_EXPECT_WITHIN_GE_LE(distBin(10.0, 0.5), 0, 10);
        DFGTEST_EXPECT_NAN(distBin(-1, 0.5));
        DFGTEST_EXPECT_NAN(distBin(10, 1.01));

        DFGTEST_EXPECT_WITHIN_GE_LE(distBer(0.5), 0, 1);
        DFGTEST_EXPECT_NAN(distBer(1.01));

        DFGTEST_EXPECT_WITHIN_GE_LE(distNegBin(10.0, 0.5), 0, (std::numeric_limits<int>::max)());
        DFGTEST_EXPECT_NAN(distNegBin(-1.0, 0.5));
        DFGTEST_EXPECT_NAN(distNegBin(10, 1.01));

        DFGTEST_EXPECT_WITHIN_GE_LE(distGeo(0.5), 0, (std::numeric_limits<int>::max)());
        DFGTEST_EXPECT_NAN(distGeo(1.01));

        DFGTEST_EXPECT_WITHIN_GE_LE(distPoi(10.0), 0, (std::numeric_limits<int>::max)());
        DFGTEST_EXPECT_NAN(distPoi(-10.0));

        DFGTEST_EXPECT_WITHIN_GE_LE(distUr(10.0, 200.0), 10, 200);
        DFGTEST_EXPECT_NAN(distUr(5.0, -8.0));

        DFGTEST_EXPECT_NON_NAN(distNormal(0.0, 1.0));
        DFGTEST_EXPECT_NAN(distNormal(0.0, -1.0));

        DFGTEST_EXPECT_NON_NAN(distExp(1.0));
        DFGTEST_EXPECT_NAN(distExp(0.0));

        DFGTEST_EXPECT_NON_NAN(distGamma(1.0, 2.0));
        DFGTEST_EXPECT_NAN(distGamma(0.0, 1.0));

        DFGTEST_EXPECT_NON_NAN(distWei(1.0, 2.0));
        DFGTEST_EXPECT_NAN(distWei(0.0, 1.0));

        DFGTEST_EXPECT_NON_NAN(distExt(-1.0, 2.0));
        DFGTEST_EXPECT_NAN(distExt(-1.0, 0.0));

        DFGTEST_EXPECT_NON_NAN(distLog(-1.0, 2.0));
        DFGTEST_EXPECT_NAN(distLog(-1.0, 0.0));

        DFGTEST_EXPECT_NON_NAN(distChi(1.0));
        DFGTEST_EXPECT_NAN(distChi(0.0));

        DFGTEST_EXPECT_NON_NAN(distCauchy(-1.0, 2.0));
        DFGTEST_EXPECT_NAN(distCauchy(-1.0, 0.0));

        DFGTEST_EXPECT_NON_NAN(distFisher(1.0, 2.0));
        DFGTEST_EXPECT_NAN(distFisher(0.0, 2.0));

        DFGTEST_EXPECT_NON_NAN(distStudent(1.0));
        DFGTEST_EXPECT_NAN(distStudent(0.0));
    }
}

namespace
{
    struct DistributionHandler
    {
        template <class T> void operator()(const std::uniform_int_distribution<T>*)                       { m_found.set(0); }
        template <class T> void operator()(const std::binomial_distribution<T>*)                          { m_found.set(1); }
                           void operator()(const std::bernoulli_distribution*)                            { m_found.set(2); }
        template <class T> void operator()(const ::DFG_MODULE_NS(rand)::NegativeBinomialDistribution<T>*) { m_found.set(3); }
        template <class T> void operator()(const std::geometric_distribution<T>*)                         { m_found.set(4); }
        template <class T> void operator()(const std::poisson_distribution<T>*)                           { m_found.set(5); }
        // Real valued
        template <class T> void operator()(const std::uniform_real_distribution<T>*)                      { m_found.set(6); }
        template <class T> void operator()(const std::normal_distribution<T>*)                            { m_found.set(7); }
        template <class T> void operator()(const std::cauchy_distribution<T>*)                            { m_found.set(8); }
        template <class T> void operator()(const std::exponential_distribution<T>*)                       { m_found.set(9); }
        template <class T> void operator()(const std::gamma_distribution<T>*)                             { m_found.set(10); }
        template <class T> void operator()(const std::weibull_distribution<T>*)                           { m_found.set(11); }
        template <class T> void operator()(const std::extreme_value_distribution<T>*)                     { m_found.set(12); }
        template <class T> void operator()(const std::lognormal_distribution<T>*)                         { m_found.set(13); }
        template <class T> void operator()(const std::chi_squared_distribution<T>*)                       { m_found.set(14); }
        template <class T> void operator()(const std::fisher_f_distribution<T>*)                          { m_found.set(15); }
        template <class T> void operator()(const std::student_t_distribution<T>*)                         { m_found.set(16); }

        template <class T> void operator()(const T*)
        {
            DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(T, "Test is missing a distribution");
        }

        std::bitset<17> m_found;
    };
    
}

TEST(dfgRand, forEachDistributionType)
{
    using namespace ::DFG_MODULE_NS(rand);
    DistributionHandler handler;
    DFG_DETAIL_NS::forEachDistributionType(handler);
    EXPECT_TRUE(handler.m_found.all());
}
