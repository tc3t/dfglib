#include <stdafx.h>
#include <dfg/dataAnalysisAll.hpp>
#include <dfg/alg.hpp>
#include <dfg/rand.hpp>
#include <deque>
#include <dfg/numeric/average.hpp>

TEST(dfgDataAnalysis, correlation)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(dataAnalysis);

    const double data1[] = { 1, 2, 3, 4, 5 };
    const double data2[] = { 10, 20, 30, 40, 50 };
    auto val = correlation(data1, data2);
    const auto corrS = correlationRankSpearman(data1, data2);

    EXPECT_EQ(1.0, val);
    EXPECT_EQ(1.0, corrS);

    const double data3[] = { 0.206380011, 0.146362211, 0.198662166, 0.806195132, 0.971970961, 0.79439112 };
    const double data4[] = { 0.628101805, 0.77934745, 0.339630401, 0.737591227, 0.969400793, 0.175143732 };
    val = correlation(data3, data4);

    EXPECT_NEAR(0.176197, val, 0.00001);

    {
        const double arr0[] = { 5, 4, 3, 2, 0 };
        const double arr1[] = { 50, 49, 48, 1, -5 };

        const auto corr = correlation(arr0, arr1);
        const auto corrSpearman = correlationRankSpearman(arr0, arr1);

        EXPECT_NEAR(0.8886215736, corr, 1e-6);
        EXPECT_EQ(1, corrSpearman);

    }

    {
        const double arr0[] = { 5, 4, 3, 2, 0 };
        const double arr1[] = { -3, -1, 50, 300, 1089 };

        const auto corr = correlation(arr0, arr1);
        const auto corrSpearman = correlationRankSpearman(arr0, arr1);

        EXPECT_NEAR(-0.917831613, corr, 1e-6);
        EXPECT_EQ(-1, corrSpearman);

    }

    // Values from example in https://en.wikipedia.org/wiki/Spearman%27s_rank_correlation_coefficient
    {
        const double arr0[] = { 106, 86, 100, 101, 99, 103, 97, 113, 112, 110 };
        const double arr1[] = {   7,  0,  27,  50, 28,  29, 20,  12,   6,  17 };

        const auto corr = correlation(arr0, arr1);
        const auto corrSpearman = correlationRankSpearman(arr0, arr1);

        EXPECT_NEAR(-0.03760147, corr, 1e-6);
        EXPECT_NEAR(-29.0 / 165.0, corrSpearman, 1e-9);

    }

    // wikihow example http://www.wikihow.com/Calculate-Spearman%27s-Rank-Correlation-Coefficient
    {
        const double arr0[] = { 6, 4, 7 };
        const double arr1[] = { 2, 9, 3 };

        const auto corr = correlation(arr0, arr1);
        const auto corrSpearman = correlationRankSpearman(arr0, arr1);

        EXPECT_NEAR(-0.8934051474, corr, 1e-6);
        EXPECT_EQ(-0.5, corrSpearman);

    }
}

TEST(dfgDataAnalysis, smoothWithNeighbourAverages)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(alg);
    using namespace DFG_MODULE_NS(dataAnalysis);
    using namespace DFG_MODULE_NS(numeric);

    {

        const std::array<double, 5> arr5 = { 1, 2, 3, 4, 5 };
        const std::array<double, 5> arr5_exp0 = arr5;
        const std::array<double, 5> arr5_exp1 = { 1.5, 2, 3, 4, 4.5 };
        const std::array<double, 5> arr5_exp2 = { 2, 10.0 / 4, 3, 14.0 / 4, 4.0 };
        const std::array<double, 5> arr5_exp3 = { 10.0 / 4, 3, 3, 3, 14.0 / 4 };
        const std::array<double, 5> arr5_exp4 = { 3, 3, 3, 3, 3 };
        const std::array<double, 5> arr5_exp5 = arr5_exp4;
        const std::array<double, 5> arr5_exp10 = arr5_exp4;

        std::array<double, 5> temp;
        temp = arr5;
        smoothWithNeighbourAverages(temp, 0);
        EXPECT_EQ(arr5_exp0, temp);

        temp = arr5;
        smoothWithNeighbourAverages(temp, 1);
        EXPECT_EQ(arr5_exp1, temp);

        temp = arr5;
        smoothWithNeighbourAverages(temp, 2);
        EXPECT_EQ(arr5_exp2, temp);

        // Verify that function accepts rvalue items (wrappers in particular)
        {
            temp = arr5;
            smoothWithNeighbourAverages(DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ArrayWrapperT)<double>(temp.data(), temp.size()), 2);
            EXPECT_EQ(arr5_exp2, temp);
        }

        temp = arr5;
        smoothWithNeighbourAverages(temp, 3);
        EXPECT_EQ(arr5_exp3, temp);

        temp = arr5;
        smoothWithNeighbourAverages(temp, 4);
        EXPECT_EQ(arr5_exp4, temp);

        temp = arr5;
        smoothWithNeighbourAverages(temp, 5);
        EXPECT_EQ(arr5_exp5, temp);

        temp = arr5;
        smoothWithNeighbourAverages(temp, 10);
        EXPECT_EQ(arr5_exp10, temp);
    }

    {
        std::vector<double> vec1024wnd10(1024);
        generateAdjacent(vec1024wnd10, 0, 1);
        smoothWithNeighbourAverages(vec1024wnd10, 10);
        EXPECT_EQ(55.0 / 11, vec1024wnd10[0]);
        EXPECT_EQ(512, vec1024wnd10[512]);
        EXPECT_EQ(1018, lastOf(vec1024wnd10));
    }

    {
        std::vector<double> vec8000wnd6000(8000);
        for (size_t i = 0; i < vec8000wnd6000.size(); ++i)
            vec8000wnd6000[i] = static_cast<double>(2 * i);
        smoothWithNeighbourAverages(vec8000wnd6000, 6000);
        EXPECT_EQ(6000, vec8000wnd6000[0]);
        EXPECT_NEAR(9998, lastOf(vec8000wnd6000), 1e-11);

    }

    const auto testWithRandomData = [](const size_t nWindowSize)
            {
                using namespace DFG_ROOT_NS;
                std::deque<float> data(20);
                auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();
                std::generate(data.begin(), data.end(), [&]()
                {
                    return DFG_MODULE_NS(rand)::rand(randEng, float(-100), float(100));
                });
                auto smoothed = data;
                DFG_MODULE_NS(dataAnalysis)::smoothWithNeighbourAverages(smoothed, nWindowSize);

                for (size_t i = 0; i < count(data); ++i)
                {
                    const size_t nLeft = i - Min(i, nWindowSize);
                    const size_t nRight = i + Min(data.size() - i - 1, nWindowSize) + 1;
                    EXPECT_NEAR(DFG_MODULE_NS(numeric)::average(makeRange(data.begin() + nLeft, data.begin() + nRight)), smoothed[i], 1e-4);
                }
            };
    testWithRandomData(0);
    testWithRandomData(1);
    testWithRandomData(2);
    testWithRandomData(3);
    testWithRandomData(4);
    testWithRandomData(9);
    testWithRandomData(10);
    testWithRandomData(11);
    testWithRandomData(13);
    testWithRandomData(18);
    testWithRandomData(19);
    testWithRandomData(20);
    testWithRandomData(21);
    testWithRandomData(22);
    testWithRandomData(22000);
    testWithRandomData(NumericTraits<size_t>::maxValue);
}

TEST(dfgDataAnalysis, smoothWithNeighbourMedians)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(alg);
    using namespace DFG_MODULE_NS(dataAnalysis);
    using namespace DFG_MODULE_NS(numeric);

    {
        const std::array<double, 5> arr5 = { 1, 2, 3, 4, 5 };
        const std::array<double, 5> arr5_exp0 = arr5;
        const std::array<double, 5> arr5_exp1 = { 1.5, 2, 3, 4, 4.5 };
        const std::array<double, 5> arr5_exp2 = { 2, 2.5, 3, 3.5, 4 };
        const std::array<double, 5> arr5_exp3 = { 2.5, 3, 3, 3, 3.5 };
        const std::array<double, 5> arr5_exp4 = { 3, 3, 3, 3, 3 };
        const std::array<double, 5> arr5_exp5 = arr5_exp4;
        const std::array<double, 5> arr5_exp10 = arr5_exp4;

		std::array<double, 5> temp;
        temp = arr5;
        smoothWithNeighbourMedians(temp, 0);
        EXPECT_EQ(arr5_exp0, temp);

        temp = arr5;
        smoothWithNeighbourMedians(temp, 1);
        EXPECT_EQ(arr5_exp1, temp);

        temp = arr5;
        smoothWithNeighbourMedians(temp, 2);
        EXPECT_EQ(arr5_exp2, temp);

        // Verify that function accepts rvalue items (wrappers in particular)
        {
            temp = arr5;
            smoothWithNeighbourMedians(DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ArrayWrapperT)<double>(temp.data(), temp.size()), 2);
            EXPECT_EQ(arr5_exp2, temp);
        }

        temp = arr5;
        smoothWithNeighbourMedians(temp, 3);
        EXPECT_EQ(arr5_exp3, temp);

        temp = arr5;
        smoothWithNeighbourMedians(temp, 4);
        EXPECT_EQ(arr5_exp4, temp);

        temp = arr5;
        smoothWithNeighbourMedians(temp, 5);
        EXPECT_EQ(arr5_exp5, temp);

        temp = arr5;
        smoothWithNeighbourMedians(temp, 10);
        EXPECT_EQ(arr5_exp10, temp);
    }

    {
        std::vector<double> vec1024wnd10(1024);
        generateAdjacent(vec1024wnd10, 0, 1);
        smoothWithNeighbourMedians(vec1024wnd10, 10);
        EXPECT_EQ(5, vec1024wnd10[0]);
        EXPECT_EQ(512, vec1024wnd10[512]);
        EXPECT_EQ(1018, lastOf(vec1024wnd10));
    }

    {
        std::vector<double> vec8000wnd6000(8000);
        for (size_t i = 0; i < vec8000wnd6000.size(); ++i)
            vec8000wnd6000[i] = static_cast<double>(2 * i);
        smoothWithNeighbourMedians(vec8000wnd6000, 6000);
        EXPECT_EQ(6000, vec8000wnd6000[0]);
        EXPECT_NEAR(9998, lastOf(vec8000wnd6000), 1e-11);

    }

    const auto testWithRandomData = [](const size_t nWindowSize)
    {
        using namespace DFG_ROOT_NS;
        std::deque<float> data(20);
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();
        std::generate(data.begin(), data.end(), [&]()
        {
            return DFG_MODULE_NS(rand)::rand(randEng, float(-100), float(100));
        });
        auto smoothed = data;
        DFG_MODULE_NS(dataAnalysis)::smoothWithNeighbourMedians(smoothed, nWindowSize);

        for (size_t i = 0; i < count(data); ++i)
        {
            const size_t nLeft = i - Min(i, nWindowSize);
            const size_t nRight = i + Min(data.size() - i - 1, nWindowSize) + 1;
            EXPECT_NEAR(DFG_MODULE_NS(numeric)::median(makeRange(data.begin() + nLeft, data.begin() + nRight)), smoothed[i], 1e-4);
        }
    };
    testWithRandomData(0);
    testWithRandomData(1);
    testWithRandomData(2);
    testWithRandomData(3);
    testWithRandomData(4);
    testWithRandomData(9);
    testWithRandomData(10);
    testWithRandomData(11);
    testWithRandomData(13);
    testWithRandomData(18);
    testWithRandomData(19);
    testWithRandomData(20);
    testWithRandomData(21);
    testWithRandomData(22);
    testWithRandomData(22000);
    testWithRandomData(NumericTraits<size_t>::maxValue);
}
