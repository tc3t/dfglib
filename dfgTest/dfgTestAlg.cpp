#include "stdafx.h"

#if (DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/alg.hpp>
#include <dfg/alg/arrayCopy.hpp>
#include <dfg/alg/sortMultiple.hpp>
#include <dfg/alg/rank.hpp>
#include <dfg/alg/sortSingleItem.hpp>
#include <dfg/alg/replaceSubarrays.hpp>
#include <vector>
#include <list>
#include <map>
#include <deque>
#include <dfg/time/timerCpu.hpp>
#include <dfg/rand.hpp>
#include <dfg/cont/arrayWrapper.hpp>
#include <dfg/ptrToContiguousMemory.hpp>
#include <dfg/iter/szIterator.hpp>
#include <dfg/rangeIterator.hpp>
#include <dfg/numeric/algNumeric.hpp>
#include <dfg/rand.hpp>
#include <dfg/math.hpp>
#include <dfg/Span.hpp>
#include <functional>
#include <memory>
#include <complex>

namespace
{
    template <class Range_T, class Func>
    Func ptrForEachFwd(Range_T&& range, Func f, std::true_type)
    {
        using namespace DFG_ROOT_NS;
        auto iter = ptrToContiguousMemory(range);
        const auto iterEnd = iter + count(range);
        for (; iter != iterEnd; ++iter)
            f(*iter);
        return std::move(f);
    }

    template <class Range_T, class Func>
    Func ptrForEachFwd(Range_T&&, Func f, std::false_type)
    {
        return f;
    }
}

namespace
{
    template <class FlagCallPtrForEach, class T, class Range_T, class Filler, class Func>
    void forEachFwdPerformance(Range_T&& vals, const size_t nLoopCount, Filler filler, Func func)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_ROOT_NS::DFG_SUB_NS_NAME(alg);

        filler(vals);
        T valForEach = 0;
        T valForEachFwd = 0;

        using Timer = ::DFG_MODULE_NS(time)::TimerCpu;

        {
            Timer timer;

            for (size_t i = 0; i < nLoopCount; ++i)
                forEachFwd(vals, func);
            const auto elapsedForEach = timer.elapsedWallSeconds();
            valForEachFwd = firstOf(vals);
            DFGTEST_MESSAGE(typeid(T).name() << ": alg::forEachFwd elapsed : " << 1000 * elapsedForEach << " ms");
        }
        filler(vals);
        {
            Timer timer;

            for (size_t i = 0; i < nLoopCount; ++i)
                std::for_each(vals.begin(), vals.end(), func);
            const auto elapsedForEach = timer.elapsedWallSeconds();
            valForEach = firstOf(vals);
            DFGTEST_MESSAGE(typeid(T).name() << ": std::for_each elapsed: " << 1000 * elapsedForEach << " ms");
        }
        filler(vals);
        {
            Timer timer;

            for (size_t i = 0; i < nLoopCount; ++i)
                ptrForEachFwd(vals, func, FlagCallPtrForEach());
            const auto elapsedForEach = timer.elapsedWallSeconds();
            DFGTEST_MESSAGE(typeid(T).name() << ": ptrForEachFwd elapsed: " << 1000 * elapsedForEach << " ms");
        }
        filler(vals);
        {
            Timer timer;

            for (size_t i = 0; i < nLoopCount; ++i)
                std::for_each(vals.begin(), vals.end(), func);
            const auto elapsedForEach = timer.elapsedWallSeconds();
            DFGTEST_MESSAGE(typeid(T).name() << ": std::for_each elapsed: " << 1000 * elapsedForEach << " ms");
        }
        filler(vals);
        {
            Timer timer;

            for (size_t i = 0; i < nLoopCount; ++i)
                ::DFG_MODULE_NS(alg)::DFG_DETAIL_NS::forEachFwdImpl(vals, func, std::false_type());
            const auto elapsedForEach = timer.elapsedWallSeconds();
            DFGTEST_MESSAGE(typeid(T).name() << ": alg::forEachFwdImpl non-ptr impl elapsed: " << 1000 * elapsedForEach << " ms");
        }
        filler(vals);
        {
            Timer timer;

            for (size_t i = 0; i < nLoopCount; ++i)
                forEachFwd(vals, func);
            const auto elapsedForEach = timer.elapsedWallSeconds();
            DFGTEST_MESSAGE(typeid(T).name() << ": alg::forEachFwd elapsed: " << 1000 * elapsedForEach << " ms");
        }

        EXPECT_EQ(valForEach, valForEachFwd);
    }
}

TEST(dfgAlg, forEachFwd)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(alg);
    const double valsArray[] = {1,2,3,4};

    // Test that forEachFwd accepts valsArray.
    forEachFwd(valsArray, [](const double&) {});
    forEachFwdWithIndex(valsArray, [](const double& val, const size_t i)
    {
        EXPECT_EQ(val, double(i+1));
    });

    const double arr[] = { 1, 2, 3, 4, 5, 6, 7 };
    std::vector<double> vec(cbegin(arr), cend(arr));
    std::list<double> list(cbegin(arr), cend(arr));
    double sum = 0;
    forEachFwd(vec, [&](const double& d){sum += d; });
    EXPECT_EQ(28, sum);

    sum = 0;
    forEachFwd(arr, [&](const double& d){sum += d; });
    EXPECT_EQ(28, sum);

    sum = 0;
    forEachFwd(list, [&](const double& d){sum += d; });
    EXPECT_EQ(28, sum);

    forEachFwd(vec, [&](double& d){d += 1; });

    sum = 0;
    forEachFwd(vec, [&](const double& d){sum += d; });
    EXPECT_EQ(35, sum);

    forEachFwdWithIndex(arr, [&](const double& d, const size_t i)
    {
        EXPECT_EQ(i + 1, d);
    });

    forEachFwdWithIndex(vec, [&](const double& d, const size_t i)
    {
        EXPECT_EQ(i + 2, d);
    });

#if DFG_MSVC_VER != DFG_MSVC_VER_2010 // With VC2010, randImpl lambda causes error "'alg' : a namespace with this name does not exist"
    {
#if defined(_MSC_VER)
        auto randImpl = [](){ return DFG_MODULE_NS(rand)::rand(); };
#else
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();
        const auto randImpl = [&]() { return DFG_MODULE_NS(rand)::rand(randEng); };
#endif

        std::vector<uint32> vals(100000);
        uint32 randVal = static_cast<uint32>(1 + 9 * randImpl());
        if (randVal % 2 == 0)
            randVal++; // Make uneven so that char array won't be full of null's at some point.

        #if DFGTEST_ENABLE_BENCHMARKS && !defined(DFG_BUILD_TYPE_DEBUG)
            const size_t nLoopCount = 10000 + static_cast<size_t>(3 * randImpl());
        #elif DFGTEST_ENABLE_BENCHMARKS && defined(DFG_BUILD_TYPE_DEBUG)
            const size_t nLoopCount = 100 + static_cast<size_t>(3 * randImpl());
        #else
            const size_t nLoopCount = 1 + static_cast<size_t>(3 * randImpl());
        #endif

        const auto fillUint32 = [&](std::vector<uint32>& cont) {std::fill(cont.begin(), cont.end(), randVal); };
        const auto funcUint32 = [](uint32& val) { val += 2; };
        forEachFwdPerformance<std::true_type, uint32>(vals, nLoopCount, fillUint32, funcUint32);
        std::vector<uint8> chars(400000);
        const auto charFiller = [&](DFG_CLASS_NAME(Dummy))
        {
            std::fill(chars.begin(), chars.end(), DFG_ROOT_NS::uint8(randVal));
            DFG_ROOT_NS::lastOf(chars) = '\0';
        };
        const auto funcChar = [](uint8& ch) { ch += 2; };

        #if DFGTEST_ENABLE_BENCHMARKS
            const size_t nLoopCountFactor = 1;
        #else
            const size_t nLoopCountFactor = 1;
        #endif
        
        forEachFwdPerformance<std::false_type, uint8>(makeSzRange(chars.data()), nLoopCountFactor * nLoopCount, charFiller, funcChar);
    }
#endif
}

TEST(dfgAlg, generateAdjacent)
{
    static const int arrComparison1[] = {0,1,2,3,4,5,6,7,8,9};
    static const int arrComparison2[] = {9,8,7,6,5,4,3,2,1,0};
    static const char arrComparison3[] = {10, 15, 20, 25, 30, 35, 40, 45};
    static const size_t arrComparison4[] = {600000, 700000, 800000, 900000, 1000000};
static const size_t arrComparison5[] = { 600000, 500000, 400000, 300000, 200000, 100000, 0 };
using namespace DFG_ROOT_NS;
int arr[10];
DFG_SUB_NS_NAME(alg)::generateAdjacent(makeRange(arr), 0, 1);
EXPECT_EQ(memcmp(arr, arrComparison1, sizeof(arr)), 0);

DFG_SUB_NS_NAME(alg)::generateAdjacent(makeRange(arr), 9, -1);
EXPECT_EQ(memcmp(arr, arrComparison2, sizeof(arr)), 0);

{
    std::vector<char> v(count(arrComparison3));
    DFG_SUB_NS_NAME(alg)::generateAdjacent(v, char(10), char(5));
    EXPECT_EQ(memcmp(&v[0], arrComparison3, sizeof(arrComparison3)), 0);
}

    {
        std::vector<size_t> v(count(arrComparison4));
        DFG_SUB_NS_NAME(alg)::generateAdjacent(v, 600000, 100000);
        EXPECT_EQ(memcmp(&v[0], arrComparison4, sizeof(arrComparison4)), 0);
    }

    {
        std::vector<size_t> v(count(arrComparison5));
        DFG_SUB_NS_NAME(alg)::generateAdjacent(v, 600000, -100000);
        EXPECT_EQ(memcmp(&v[0], arrComparison5, sizeof(arrComparison5)), 0);
    }

}

TEST(dfgAlg, indexOf)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(alg);
    const std::array<double, 3> arr = { -1.5, 2.5, -5.3 };
    EXPECT_EQ(0u, indexOf(arr, -1.5));
    EXPECT_EQ(1u, indexOf(arr, 2.5));
    EXPECT_EQ(2u, indexOf(arr, -5.3));
    EXPECT_FALSE(isValidIndex(arr, indexOf(arr, 1)));

    const std::array<int, 8> arr2 = { 0, 1, 1, 1, 1, 1, 2, 2 };
    EXPECT_EQ(1u, indexOf(arr2, 1));
    EXPECT_EQ(6u, indexOf(arr2, 2));

    std::list<int> list(arr2.begin(), arr2.end());
    EXPECT_EQ(0u, indexOf(list, 0));
    EXPECT_EQ(1u, indexOf(list, 1));
}

TEST(dfgAlg, contains)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(alg);
    const std::array<double, 3> arr = { -1.5, 2.5, -5.3 };
    EXPECT_TRUE(contains(arr, -1.5));
    EXPECT_TRUE(contains(arr, 2.5));
    EXPECT_TRUE(contains(arr, -5.3));
    EXPECT_FALSE(contains(arr, 0));

    const std::array<int, 8> arr2 = { 0, 1, 1, 1, 1, 1, 2, 2 };
    EXPECT_TRUE(contains(arr2, 1));
    EXPECT_TRUE(contains(arr2, 2));
    EXPECT_FALSE(contains(arr2, -1));
    EXPECT_FALSE(contains(arr2, 515616));

    std::list<int> list(arr2.begin(), arr2.end());
    EXPECT_TRUE(contains(list, 0));
    EXPECT_TRUE(contains(list, 1));
    EXPECT_FALSE(contains(list, -1));
    EXPECT_FALSE(contains(list, 9));
}

TEST(dfgAlg, floatIndexInSorted)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(alg);

    // Test empty
    {
        EXPECT_TRUE(DFG_MODULE_NS(math)::isNan(floatIndexInSorted(std::vector<double>(), 0)));
    }

    // Test single item
    {
        std::vector<double> vec(1, 5);
        EXPECT_TRUE(DFG_MODULE_NS(math)::isNan(floatIndexInSorted(vec, 0)));
        EXPECT_EQ(0, floatIndexInSorted(vec, 5));
    }

    // Test equivalence with indexOf
    {
        const std::array<double, 3> arr = { -5.3, -1.5, 2.5, };
        EXPECT_EQ(indexOf(arr, -1.5), floatIndexInSorted(arr, -1.5));
        EXPECT_EQ(indexOf(arr, 2.5), floatIndexInSorted(arr, 2.5));
        EXPECT_EQ(indexOf(arr, -5.3), floatIndexInSorted(arr, -5.3));

        const std::array<int, 8> arr2 = { 0, 1, 1, 1, 1, 1, 2, 2 };
        EXPECT_EQ(indexOf(arr2, 1), floatIndexInSorted(arr2, 1));
        EXPECT_EQ(indexOf(arr2, 2), floatIndexInSorted(arr2, 2));

        std::list<int> list(arr2.begin(), arr2.end());
        EXPECT_EQ(indexOf(list, 0), floatIndexInSorted(list, 0));
        EXPECT_EQ(indexOf(list, 1), floatIndexInSorted(list, 1));
    }

    // Test interpolation
    {
        const std::array<double, 4> arr = { -2, 0, 0, 4 };
        EXPECT_EQ(-1, floatIndexInSorted(arr, -4));
        EXPECT_EQ(-0.5, floatIndexInSorted(arr, -3));
        EXPECT_EQ(0, floatIndexInSorted(arr, -2));
        EXPECT_EQ(0.5, floatIndexInSorted(arr, -1));
        EXPECT_EQ(1, floatIndexInSorted(arr, 0.0));
        EXPECT_EQ(2.125, floatIndexInSorted(arr, 0.5));
        EXPECT_EQ(3, floatIndexInSorted(arr, 4));
        EXPECT_EQ(3.5, floatIndexInSorted(arr, 6));
        EXPECT_EQ(4, floatIndexInSorted(arr, 8));
    }

    // Testing non-trivial element type
    {
        using PairT = std::pair<double, double>;
        const PairT arr[] = { PairT(1, 1), PairT(2, 1), PairT(3, 100), PairT(4,1) };
        EXPECT_EQ(2.25, floatIndexInSorted(arr, 3.25, [](const PairT& a) { return a.first; }));
    }
}

namespace SortMultipleAdl
{
    bool gAdlSwapCalled = false;

    struct IntClass { int a; };

    void swap(IntClass& a, IntClass& b)
    {
        std::swap(a.a, b.a);
        gAdlSwapCalled = true;
    }
}

TEST(dfgAlg, sortMultiple)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(alg);

    {
        double x[] = { 5, 3, 4, 6 };
        double y[] = { 10, 20, 30, 40 };
        double z[] = { 300, 200, 100, 0 };

        // Test two array ranges.
        {
            std::map<double, double> originalPairs;
            for (size_t i = 0, nCount = count(x); i < nCount; ++i)
                originalPairs[x[i]] = y[i];

            sortMultiple(x, y);
            EXPECT_TRUE(std::is_sorted(std::begin(x), std::end(x)));
            for (size_t i = 0, nCount = count(x); i < nCount; ++i)
            {
                EXPECT_EQ(originalPairs[x[i]], y[i]);
            }
        }

        // Testing sorting of non-copyable items and custom predicate.
        {
            std::unique_ptr<double> xPtrs[] = { std::unique_ptr<double>(new double(5)), std::unique_ptr<double>(new double(3)), std::unique_ptr<double>(new double(4)) };
            std::unique_ptr<double> yPtrs[] = { std::unique_ptr<double>(new double(10)), std::unique_ptr<double>(new double(20)), std::unique_ptr<double>(new double(30)) };
            sortMultipleWithPred([](const std::unique_ptr<double>& a, const std::unique_ptr<double>& b) {return *a < *b; }, xPtrs, yPtrs);
            EXPECT_EQ(3, *xPtrs[0]);
            EXPECT_EQ(4, *xPtrs[1]);
            EXPECT_EQ(5, *xPtrs[2]);
            EXPECT_EQ(20, *yPtrs[0]);
            EXPECT_EQ(30, *yPtrs[1]);
            EXPECT_EQ(10, *yPtrs[2]);
        }

        // Tests three vector ranges.
        {
            std::vector<double> xVec(std::begin(y), std::end(y));
            std::vector<double> yVec(std::begin(x), std::end(x));
            std::vector<double> zVec(std::begin(z), std::end(z));

            std::map<double, double> originalXy;
            std::map<double, double> originalXz;
            for (size_t i = 0, nCount = count(x); i < nCount; ++i)
            {
                originalXy[xVec[i]] = yVec[i];
                originalXz[xVec[i]] = zVec[i];
            }

            sortMultiple(xVec, yVec, zVec);

            EXPECT_TRUE(std::is_sorted(std::begin(x), std::end(x)));
            for (size_t i = 0, nCount = count(x); i < nCount; ++i)
            {
                EXPECT_EQ(yVec[i], originalXy[xVec[i]]);
                EXPECT_EQ(zVec[i], originalXz[xVec[i]]);
            }
        }
    }

    // Test four vector ranges.
    {
        double x0[] = { 5, 3, 4, 6 };
        double x1[] = { 10, 20, 30, 40 };
        double x2[] = { 300, 200, 100, 0 };
        double x3[] = { -5, -8, -3, -1 };

        std::vector<std::vector<double>> vecs;
        vecs.push_back(std::vector<double>(std::begin(x0), std::end(x0)));
        vecs.push_back(std::vector<double>(std::begin(x1), std::end(x1)));
        vecs.push_back(std::vector<double>(std::begin(x2), std::end(x2)));
        vecs.push_back(std::vector<double>(std::begin(x3), std::end(x3)));

        std::map<double, std::vector<double>> expected;
        for (size_t i = 0; i < count(x0); ++i)
        {
            std::vector<double> vals;
            vals.push_back(x1[i]);
            vals.push_back(x2[i]);
            vals.push_back(x3[i]);
            expected[x0[i]] = std::move(vals);
        }

        std::vector<std::vector<double>*> contPtrs;
        std::for_each(std::begin(vecs), std::end(vecs), [&](std::vector<double>& vec) {contPtrs.push_back(&vec); });
        sortMultipleInPtrCont(contPtrs);

        EXPECT_TRUE(std::is_sorted(std::begin(vecs[0]), std::end(vecs[0])));
        for (size_t i = 0, nCount = count(x0); i < nCount; ++i)
        {
            const auto xCurrent = vecs[0][i];
            for (size_t j = 1; j < vecs.size(); ++j)
                EXPECT_EQ(vecs[j][i], expected[xCurrent][j-1]);
        }

    }

    // Test user defined predicate.
    {
        std::array<double, 4> x = { 5, 3, 4, 6 };
        std::array<double, 4> y = { 10, 20, 30, 40 };
        const std::array<double, 4> xExp = { 6, 5, 4, 3 };
        const std::array<double, 4> yExp = { 40, 10, 30, 20 };
        sortMultipleWithPred(std::greater<double>(), x, y);
        EXPECT_EQ(xExp, x);
        EXPECT_EQ(yExp, y);
    }

    // Test string sorting.
    {
        std::array<std::string, 4> x = { "b", "ab", "aab", "a" };
        std::array<std::string, 4> y = { "4", "5", "6", "7" };
        const std::array<std::string, 4> xExp = { "a", "aab", "ab", "b" };
        const std::array<std::string, 4> yExp = { "7", "6", "5", "4" };
        sortMultiple(x, y);
        EXPECT_EQ(xExp, x);
        EXPECT_EQ(yExp, y);
    }

    {
        std::array<std::string, 4> x = { "b", "ab", "aab", "a" };
        std::array<double, 4> y = { 10, 35, 30, 40 };
        const std::array<std::string, 4> xExp = { "a", "aab", "ab", "b" };
        const std::array<double, 4> yExp = { 40, 30, 35, 10 };
        sortMultiple(x, y);
        EXPECT_EQ(xExp, x);
        EXPECT_EQ(yExp, y);
    }

    // Test with random data
    {
        using namespace DFG_MODULE_NS(rand);
        auto eng = createDefaultRandEngineRandomSeeded();
        const size_t nSize = DFG_MODULE_NS(rand)::rand(eng, 75, 100);
        std::vector<double> arr0(nSize);
        std::vector<double> arr1(nSize);
        std::for_each(std::begin(arr0), std::end(arr0), [&](double& v) {v = DFG_MODULE_NS(rand)::rand(eng, -10000.0, 10000.0); });
        std::for_each(std::begin(arr1), std::end(arr1), [&](double& v) {v = DFG_MODULE_NS(rand)::rand(eng, -10000.0, 10000.0); });
        std::map<double, double> expected;
        for (size_t i = 0; i < nSize; ++i)
            expected[arr0[i]] = arr1[i]; // Note: Lazily hope that there will be no duplicates in arr0; in that case the logic could produce faulty test results.
        sortMultiple(arr0, arr1);
        EXPECT_TRUE(std::is_sorted(std::begin(arr0), std::end(arr0)));
        for (size_t i = 0; i < nSize; ++i)
            EXPECT_EQ(expected[arr0[i]], arr1[i]);
    }

    // ADL-usage
    {
        std::array<int, 2> arr0 = { 1, 0 };
        std::array<SortMultipleAdl::IntClass, 2> arr1 = { SortMultipleAdl::IntClass{1}, SortMultipleAdl::IntClass{2} };
        sortMultiple(arr0, arr1);
        EXPECT_EQ(0, arr0[0]);
        EXPECT_EQ(1, arr0[1]);
        EXPECT_EQ(2, arr1[0].a);
        EXPECT_EQ(1, arr1[1].a);
        EXPECT_TRUE(SortMultipleAdl::gAdlSwapCalled);
    }
}

TEST(dfgAlg, rank)
{
    using namespace DFG_MODULE_NS(alg);

    {
        const std::array<double, 8> arr0 = { 7, 10, 9, -3, 3, 4, 1, 0 };
        const std::array<int, 8> rankExp0 = { 5, 7, 6, 0, 3, 4, 2, 1 };

        const auto rankArr = rank(arr0);
        EXPECT_EQ(rankExp0.size(), rankArr.size());
        EXPECT_TRUE(std::equal(rankExp0.begin(), rankExp0.end(), rankArr.begin()));

        const auto rankArrInt = rankT<int>(arr0);
        EXPECT_EQ(rankExp0.size(), rankArrInt.size());
        EXPECT_TRUE(std::equal(rankExp0.begin(), rankExp0.end(), rankArrInt.begin()));

        // rank should return empty vector if using floating point strategy with integer rank elements.
        const auto rankArrIntFrac = rankT<int>(arr0, RankStrategyFractional);
        EXPECT_TRUE(rankArrIntFrac.empty());
    }

    {
        const std::array<double, 4> arr0 = { 2, 1, 1, 3 };

        const auto rankArr = rank(arr0);
        EXPECT_EQ(4u, rankArr.size());
        EXPECT_EQ(2, rankArr[0]);
        EXPECT_TRUE(rankArr[1] == 0 || rankArr[1] == 1);
        EXPECT_TRUE(rankArr[2] == 0 || rankArr[2] == 1);
        EXPECT_EQ(3, rankArr[3]);
    }

    // Example from https://en.wikipedia.org/wiki/Ranking#Fractional_ranking_.28.221_2.5_2.5_4.22_ranking.29
    {
        const std::array<double, 9> arr0 = { 1, 1, 2, 3, 3, 4, 5, 5, 5 };
        const std::array<double, 9> rankExp0 = { 0.5, 0.5, 2, 3.5, 3.5, 5, 7, 7, 7 };
        const std::array<double, 9> rankExp1 = { 1.5, 1.5, 3, 4.5, 4.5, 6, 8, 8, 8 };
        const std::array<double, 9> rankExp2 = { -9.5, -9.5, -8, -6.5, -6.5, -5, -3, -3, -3 };


        const auto rankArr0 = rank(arr0, RankStrategyFractional);
        EXPECT_EQ(rankExp0.size(), rankArr0.size());
        EXPECT_TRUE(std::equal(rankExp0.begin(), rankExp0.end(), rankArr0.begin()));

        const auto rankArr1 = rank(arr0, RankStrategyFractional, 1);
        EXPECT_EQ(rankExp1.size(), rankArr1.size());
        EXPECT_TRUE(std::equal(rankExp1.begin(), rankExp1.end(), rankArr1.begin()));

        const auto rankArr2 = rank(arr0, RankStrategyFractional, -10);
        EXPECT_EQ(rankExp2.size(), rankArr2.size());
        EXPECT_TRUE(std::equal(rankExp2.begin(), rankExp2.end(), rankArr2.begin()));
    }

    {

        const std::array<double, 12> arr0 = { 2, 1, 3, 2, 1, 3, 4, 1, 2, 0, 3, 3};
        const std::array<double, 12> rankExp0 = { 5, 2, 8.5, 5, 2, 8.5, 11, 2, 5, 0, 8.5, 8.5 };
        const std::array<double, 12> rankExp1 = { 10, 7, 13.5, 10, 7, 13.5, 16, 7, 10, 5, 13.5, 13.5 };

        const auto rankArr0 = rank(arr0, RankStrategyFractional);
        EXPECT_EQ(rankExp0.size(), rankArr0.size());
        EXPECT_TRUE(std::equal(rankExp0.begin(), rankExp0.end(), rankArr0.begin()));

        const auto rankArr1 = rank(arr0, RankStrategyFractional, 5);
        EXPECT_EQ(rankExp1.size(), rankArr1.size());
        EXPECT_TRUE(std::equal(rankExp1.begin(), rankExp1.end(), rankArr1.begin()));
    }
}

TEST(dfgAlg, findNearest)
{
    using namespace DFG_MODULE_NS(alg);

    // Basic tests
    {
        const std::array<double, 12> arr0 = { 2, 1, 3, 2, 1, 3, 4, 1, 2, 0, 3, 3 };

        const auto nearestN1 = findNearest(arr0, -1);
        const auto nearest0 = findNearest(arr0, 0);
        const auto nearest0d4 = findNearest(arr0, 0.4);
        const auto nearest0d5 = findNearest(arr0, 0.5);
        const auto nearest0d51 = findNearest(arr0, 0.51);
        const auto nearest1 = findNearest(arr0, 0.51);
        const auto nearest3d4 = findNearest(arr0, 3.4);
        const auto nearest4 = findNearest(arr0, 4);
        const auto nearest10 = findNearest(arr0, 10);
        EXPECT_EQ(0, *nearestN1);
        EXPECT_EQ(9, nearestN1 - arr0.begin());
        EXPECT_EQ(0, *nearest0);
        EXPECT_EQ(9, nearest0 - arr0.begin());
        EXPECT_EQ(0, *nearest0d4);
        EXPECT_EQ(9, nearest0d4 - arr0.begin());
        EXPECT_EQ(1, *nearest0d5);
        EXPECT_EQ(1, nearest0d5 - arr0.begin());
        EXPECT_EQ(1, *nearest0d51);
        EXPECT_EQ(1, nearest0d51 - arr0.begin());
        EXPECT_EQ(1, *nearest1);
        EXPECT_EQ(1, nearest1 - arr0.begin());
        EXPECT_EQ(3, *nearest3d4);
        EXPECT_EQ(2, nearest3d4 - arr0.begin());
        EXPECT_EQ(4, *nearest4);
        EXPECT_EQ(6, nearest4 - arr0.begin());
        EXPECT_EQ(4, *nearest10);
        EXPECT_EQ(6, nearest10 - arr0.begin());
    }

    // Test custom difference function.
    {
        std::vector<std::complex<double>> vecVals;
        vecVals.push_back(std::complex<double>(1));
        vecVals.push_back(std::complex<double>(3));
        vecVals.push_back(std::complex<double>(5));
        vecVals.push_back(std::complex<double>(7));
        vecVals.push_back(std::complex<double>(9));
        std::array<double, 1> searchElem = {3};
        const auto diffLb = [](const std::complex<double>& c, const std::array<double, 1>& val) {return std::abs(c.real() - val[0]); };
        const auto nearest = findNearest(vecVals, searchElem, diffLb);
        EXPECT_EQ(3, nearest->real());
        EXPECT_EQ(1, nearest - vecVals.begin());
    }

    // Test empty range
    {
        std::vector<double> v;
        EXPECT_EQ(v.end(), findNearest(v, 0));
    }
}

TEST(dfgAlg, sortSingleItem)
{
    using namespace DFG_MODULE_NS(alg);
    using namespace DFG_MODULE_NS(rand);

    auto randEng = createDefaultRandEngineRandomSeeded();
    auto distrEng = makeDistributionEngineUniform(&randEng, -1000, 1000);
    std::vector<int> vec;
    
    // Empty range handling
    sortSingleItem(vec, vec.end());

    // Single item handling
    vec.push_back(10);
    sortSingleItem(vec, vec.begin());
    sortSingleItem(vec, vec.end());

    // Handling of two elements.
    {
        std::array<int, 2> arr2 = { 2, 1 };
        std::array<int, 2> arr2Expected = { 1, 2 };
        auto arr2_0 = arr2;
        auto arr2_1 = arr2;
        auto rv0 = sortSingleItem(arr2_0, arr2_0.begin());
        EXPECT_EQ(rv0, arr2_0.begin() + 1);
        rv0 = sortSingleItem(arr2_0, arr2_0.begin()); // Should do nothing.
        EXPECT_EQ(rv0, arr2_0.begin());

        auto rv1 = sortSingleItem(arr2_1, arr2_1.end());
        EXPECT_EQ(rv1, arr2_1.begin());
        rv1 = sortSingleItem(arr2_1, arr2_1.end()); // Should do nothing.
        EXPECT_EQ(rv1, arr2_1.begin() + 1);

        EXPECT_EQ(arr2Expected, arr2_0);
        EXPECT_EQ(arr2Expected, arr2_1);
    }

    const int nCount = 50;
    
    vec.reserve(vec.size() + 50 + nCount);
    vec.resize(50);
    for (size_t i = 0; i < vec.size(); ++i)
        vec[i] = distrEng();
    //std::generate(vec.begin(), vec.end(), [&]() {return distrEng(); }); // This didn't compile in VC2010
    std::sort(vec.begin(), vec.end());

    
    auto vecSsi = vec;
    for (int i = 0; i < nCount; ++i)
    {
        // Push back
        vecSsi.push_back(distrEng());
        sortSingleItem(vecSsi, vecSsi.end());
        EXPECT_TRUE(std::is_sorted(vecSsi.begin(), vecSsi.end()));

        // Push in middle.
        const auto nPos = DFG_MODULE_NS(rand)::rand(randEng, size_t(0), vecSsi.size() - 1);
        const auto newVal = distrEng();
        vecSsi.insert(vecSsi.begin() + nPos, newVal);
        auto iterNewPos = sortSingleItem(vecSsi, vecSsi.begin() + nPos);
        EXPECT_EQ(newVal, *iterNewPos);
        //EXPECT_TRUE(std::is_sorted(vecSsi.begin(), vecSsi.end()));
    }

#if 0 // Performance test for comparing sortSingleItem() and sort() in case of moving push_backed item to sorted place.
    {
        const int nPerfCount = 5000;
        auto vecSsi = vec;
        DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timerSsi;
        for (int i = 0; i < nPerfCount; ++i)
        {
            vecSsi.push_back(distrEng());
            sortSingleItem(vecSsi, vecSsi.end());
        }
        const auto timeSsi = timerSsi.elapsedWallSeconds();

        auto vecStdSort = vec;
        DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timerStdSort;
        for (int i = 0; i < nPerfCount; ++i)
        {
            vecStdSort.push_back(distrEng());
            std::sort(vecStdSort.begin(), vecStdSort.end());
        }
        const auto timeStdSort = timerStdSort.elapsedWallSeconds();
        std::cout << "TimeSsi: " << timeSsi << ", timeStdSort: " << timeStdSort << '\n';
        // In couple of tests the difference was almost 10x (sortSingleItem() faster). Tested with VC2012 and VC2013 release builds.
        std::system("pause");
    }
#endif
}

struct SimpleStruct { int a; };

TEST(dfgAlg, arrayCopy)
{
    using namespace DFG_MODULE_NS(alg);

    DFGTEST_STATIC(DFG_DETAIL_NS::IsArrayCopyMemCpy<int>::value         == true);
    DFGTEST_STATIC(DFG_DETAIL_NS::IsArrayCopyMemCpy<float>::value       == true);
    DFGTEST_STATIC(DFG_DETAIL_NS::IsArrayCopyMemCpy<double>::value      == true);
    DFGTEST_STATIC(DFG_DETAIL_NS::IsArrayCopyMemCpy<void*>::value       == true);
    DFGTEST_STATIC(DFG_DETAIL_NS::IsArrayCopyMemCpy<int*>::value        == true);
    DFGTEST_STATIC(DFG_DETAIL_NS::IsArrayCopyMemCpy<std::string>::value == false);

#if DFG_LANGFEAT_HAS_IS_TRIVIALLY_COPYABLE==1
    DFGTEST_STATIC(DFG_DETAIL_NS::IsArrayCopyMemCpy<SimpleStruct>::value == true);
#endif

    {
        int a[2] = { 0, 0 };
        int b[2] = { 1, 2 };
        arrayCopy(a, b);
        EXPECT_EQ(1, a[0]);
        EXPECT_EQ(2, a[1]);
        EXPECT_EQ(1, b[0]);
        EXPECT_EQ(2, b[1]);

        int* pa[2];
        int* pb[2] = { &b[0], &b[1] };
        arrayCopy(pa, pb);
        EXPECT_EQ(pb[0], pa[0]);
        EXPECT_EQ(pb[1], pa[1]);
    }

    // Test array ofg non-trivial type
    {
        std::string a[2] = { "abc", ""};
        std::string b[2] = { "def", "ghi" };
        arrayCopy(a, b);
        EXPECT_EQ("def", a[0]);
        EXPECT_EQ("ghi", a[1]);
        EXPECT_EQ("def", b[0]);
        EXPECT_EQ("ghi", b[1]);
    }
}

TEST(dfgAlg, nearestRangeInSorted)
{
    using namespace DFG_MODULE_NS(alg);
    // Empty handling
    {
        const auto rv = nearestRangeInSorted(std::vector<int>(), 1, 1);
        EXPECT_TRUE(rv.empty());
        EXPECT_EQ(rv.end(), rv.begin());
        EXPECT_EQ(rv.end(), rv.nearest());
    }
    // Simple cases with evenly distributed input
    {
        const double arr[] = {1,2,3,4,5,6};
        // Value smaller than first
        {
            const auto rv = nearestRangeInSorted(arr, 0.0, 1);
            ASSERT_EQ(1u, rv.size());
            EXPECT_EQ(0u, rv.leftRange().size());
            EXPECT_EQ(0u, rv.rightRange().size());
            EXPECT_EQ(std::begin(arr), rv.nearest());
        }
        // Value greater than last
        {
            const auto rv = nearestRangeInSorted(arr, 10.0, 1);
            ASSERT_EQ(1u, rv.size());
            EXPECT_EQ(0u, rv.leftRange().size());
            EXPECT_EQ(0u, rv.rightRange().size());
            EXPECT_EQ(std::end(arr) - 1, rv.nearest());
        }
        // Value smaller than first, include more than element count
        {
            const auto rv = nearestRangeInSorted(arr, 0.0, 10);
            ASSERT_EQ(6u, rv.size());
            EXPECT_EQ(0u, rv.leftRange().size());
            EXPECT_EQ(5u, rv.rightRange().size());
            EXPECT_EQ(std::begin(arr), rv.nearest());
            EXPECT_EQ(std::end(arr), rv.end());
        }
        // Simple case
        {
            const auto rv = nearestRangeInSorted(arr, 2.8, 2);
            ASSERT_EQ(2u, rv.size());
            EXPECT_EQ(1u, rv.leftRange().size());
            EXPECT_EQ(0u, rv.rightRange().size());
            EXPECT_EQ(std::begin(arr) + 1, rv.begin());
            EXPECT_EQ(std::begin(arr) + 2, rv.begin() + 1);
            EXPECT_EQ(std::begin(arr) + 3, rv.end());
            EXPECT_EQ(std::begin(arr) + 2, rv.nearest());
        }
        // Even distance to second nearest
        {
            const auto rv = nearestRangeInSorted(arr, 3.0, 2);
            ASSERT_EQ(2u, rv.size());
            EXPECT_EQ(1u, rv.leftRange().size());
            EXPECT_EQ(0u, rv.rightRange().size());
            EXPECT_EQ(std::begin(arr) + 1, rv.begin());
            EXPECT_EQ(std::begin(arr) + 2, rv.begin() + 1);
            EXPECT_EQ(std::begin(arr) + 2, rv.nearest());
        }

        // Early stop on left side
        {
            const auto rv = nearestRangeInSorted(arr, 2.0, 4);
            ASSERT_EQ(4u, rv.size());
            EXPECT_EQ(1u, rv.leftRange().size());
            EXPECT_EQ(2u, rv.rightRange().size());
            EXPECT_EQ(std::begin(arr) + 1, rv.nearest());
            EXPECT_EQ(std::begin(arr), rv.begin());
            EXPECT_EQ(std::begin(arr) + 4, rv.end());
        }

        // Early stop on right side
        {
            const auto rv = nearestRangeInSorted(arr, 5.0, 4);
            ASSERT_EQ(4u, rv.size());
            EXPECT_EQ(2u, rv.leftRange().size());
            EXPECT_EQ(1u, rv.rightRange().size());
            EXPECT_EQ(std::begin(arr) + 4, rv.nearest());
            EXPECT_EQ(std::begin(arr) + 2, rv.begin());
            EXPECT_EQ(std::end(arr), rv.end());
        }
    }

    // Nearest only from left side from uneven input
    {
        const double arr[] = { 90, 95, 99, 100, 110, 115 };
        const auto rv = nearestRangeInSorted(arr, 101.0, 3);
        ASSERT_EQ(3u, rv.size());
        EXPECT_EQ(2u, rv.leftRange().size());
        EXPECT_EQ(0u, rv.rightRange().size());
        EXPECT_EQ(std::begin(arr) + 1, rv.begin());
        EXPECT_EQ(std::begin(arr) + 2, rv.begin() + 1);
        EXPECT_EQ(std::begin(arr) + 3, rv.begin() + 2);
        EXPECT_EQ(std::begin(arr) + 4, rv.end());
        EXPECT_EQ(std::begin(arr) + 3, rv.nearest());
    }

    // Nearest only from right side from uneven input
    {
        const double arr[] = { 90, 95, 100, 101, 102, 115 };
        const auto rv = nearestRangeInSorted(arr, 99.0, 3);
        ASSERT_EQ(3u, rv.size());
        EXPECT_EQ(0u, rv.leftRange().size());
        EXPECT_EQ(2u, rv.rightRange().size());
        EXPECT_EQ(std::begin(arr) + 2, rv.begin());
        EXPECT_EQ(std::begin(arr) + 3, rv.begin() + 1);
        EXPECT_EQ(std::begin(arr) + 4, rv.begin() + 2);
        EXPECT_EQ(std::begin(arr) + 5, rv.end());
        EXPECT_EQ(std::begin(arr) + 2, rv.nearest());
    }

    // Testing with custom sort.
    {
        using PairT = std::pair<double, double>;
        const PairT arr[] = { PairT(1, 1), PairT(2, 1), PairT(3, 100), PairT(4,1) };
        // arr is sorted with respect to first-items
        const auto rv = nearestRangeInSorted(arr, 3.2, 3, [](const PairT& a) { return a.first; });
        ASSERT_EQ(3u, rv.size());
        EXPECT_EQ(1u, rv.leftRange().size());
        EXPECT_EQ(1u, rv.rightRange().size());
        EXPECT_EQ(std::begin(arr) + 2, rv.nearest());
        EXPECT_EQ(std::begin(arr) + 1, rv.begin());
        EXPECT_EQ(std::end(arr), rv.end());
    }
}

TEST(dfgAlg, replaceSubarrays)
{
    using namespace ::DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(alg);

    // Basic one-to-one replacement
    {
        std::vector<int> v{ 1,2,3 };
        const auto oldS = { 1 };
        const auto newS = { 4 };
        auto nReplaceCount = replaceSubarrays(v, oldS, newS);
        DFGTEST_EXPECT_LEFT(1, nReplaceCount);
        DFGTEST_EXPECT_LEFT((std::vector<int>{4, 2, 3}), v);

        // Testing that initializer list arguments are accepted
        {
            nReplaceCount = replaceSubarrays(v, { 2 }, oldS);
            DFGTEST_EXPECT_LEFT(1, nReplaceCount);
            DFGTEST_EXPECT_LEFT((std::vector<int>{4, 1, 3}), v);

            nReplaceCount = replaceSubarrays(v, oldS, { 5 });
            DFGTEST_EXPECT_LEFT(1, nReplaceCount);
            DFGTEST_EXPECT_LEFT((std::vector<int>{4, 5, 3}), v);

            nReplaceCount = replaceSubarrays(v, { 4 }, { 1 });
            DFGTEST_EXPECT_LEFT(1, nReplaceCount);
            DFGTEST_EXPECT_LEFT((std::vector<int>{1, 5, 3}), v);
        }
    }

    // Basic shrinking replacement
    {
        std::vector<int> v{ 1,2,3 };
        const auto nReplaceCount = replaceSubarrays(v, { 1, 2 }, { 4 });
        DFGTEST_EXPECT_LEFT(1, nReplaceCount);
        DFGTEST_EXPECT_LEFT((std::vector<int>{4, 3}), v);
    }

    // More complex shrinking replacement
    {
        std::vector<int> v{ 0,1,2,3,4,5,1,2,6,7,1,2 };
        const auto nReplaceCount = replaceSubarrays(v, { 1, 2 }, { 9 });
        DFGTEST_EXPECT_LEFT(3, nReplaceCount);
        DFGTEST_EXPECT_LEFT((std::vector<int>{0,9,3,4,5,9,6,7,9}), v);
    }

    // Identical replacement in size-preserving replacement
    {
        std::vector<int> v{ 1,2,3,4 };
        const auto nReplaceCount = replaceSubarrays(v, { 2 }, { 2 });
        DFGTEST_EXPECT_LEFT(1, nReplaceCount);
        DFGTEST_EXPECT_LEFT((std::vector<int>{1, 2, 3, 4}), v);
    }

    // Size-preserving function should work even if container is not resizable
    {
        std::array<int, 4> a{ 1, 2, 3, 4 };
        const auto oldS = { 2 };
        const auto newS = { 6 };
        const auto nReplaceCount = replaceSubarrays_sizeEqual(a, oldS, newS);
        DFGTEST_EXPECT_LEFT(1, nReplaceCount);
        DFGTEST_EXPECT_LEFT((std::array<int, 4>{1, 6, 3, 4}), a);
    }

    // Basic expanding test
    {
        std::vector<int> v{ 1,2,2,3,2,4,5,6,2 };
        const auto nReplaceCount = replaceSubarrays(v, { 2 }, {7,8,9});
        DFGTEST_EXPECT_LEFT(4, nReplaceCount);
        DFGTEST_EXPECT_LEFT((std::vector<int>{1,7,8,9,7,8,9,3,7,8,9,4,5,6,7,8,9}), v);
    }

    // Basic test with non-contiguous container
    {
        std::list<int> c{ 1,2,3 };
        const auto nReplaceCount = replaceSubarrays(c, { 1, 2 }, { 4 });
        DFGTEST_EXPECT_LEFT(1, nReplaceCount);
        DFGTEST_EXPECT_LEFT((std::list<int>{4, 3}), c);
    }

    // Heterogenous argument types
    {
        std::list<int> c{ 1,2,3 };
        const auto nReplaceCount = replaceSubarrays(c, std::deque<int>{ 1, 2 }, Span<const int>(std::vector<int>{4}));
        DFGTEST_EXPECT_LEFT(1, nReplaceCount);
        DFGTEST_EXPECT_LEFT((std::list<int>{4, 3}), c);
    }

    // Testing that replacing range can overlap with original container (given it's an acceptable overlap)
    {
        std::vector<int> v{1,2,3,4,5,6,7,4,5,6,8};
        const auto replacingRange = makeRange(v.begin() + 1, v.begin() + 3);
        const auto nReplaceCount = replaceSubarrays(v, { 4, 5, 6 }, replacingRange);
        DFGTEST_EXPECT_LEFT(2, nReplaceCount);
        DFGTEST_EXPECT_LEFT((std::vector<int>{1, 2, 3, 2, 3, 7, 2, 3, 8}), v);
    }

    // Helper container handling in expanding case
    {
        std::vector<int> v{1,2,3,1,2,3,1};
        std::vector<int> vHelper;
        vHelper.reserve(64);
        const auto pHelperData = vHelper.data();
        const auto nOrigCapacity = vHelper.capacity();
        replaceSubarrays_expanding(v, std::array<int, 2>{2, 3}, std::array<int, 9>{1,2,3,4,5,6,7,8,9}, &vHelper);
        DFGTEST_EXPECT_LEFT((std::vector<int>{1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1}), v);
        DFGTEST_EXPECT_LEFT((std::vector<int>{1, 2, 3, 1, 2, 3, 1}), vHelper);
        DFGTEST_EXPECT_LEFT(pHelperData, v.data());
        DFGTEST_EXPECT_LEFT(nOrigCapacity, v.capacity());
    }
}

TEST(dfgAlg, detail_areContiguousByTypeAndOverlappingRanges)
{
    using namespace ::DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(alg);
    using namespace ::DFG_MODULE_NS(alg)::DFG_DETAIL_NS;

    // Tests with array
    {
        int arr[5] = { 0, 1, 2, 3, 4 };
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(arr, arr));
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(arr, Span<int>(arr)));
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(arr, Span<int>(arr + 4, 1)));
        // Empty range overlapping
        {
            const auto emptyStartRange = Span<int>(arr, 0);
            DFGTEST_EXPECT_FALSE(areContiguousByTypeAndOverlappingRanges(emptyStartRange, emptyStartRange)); // Both empty with identical address
            DFGTEST_EXPECT_FALSE(areContiguousByTypeAndOverlappingRanges(arr, emptyStartRange)); // Only one is empty
        }
        DFGTEST_EXPECT_FALSE(areContiguousByTypeAndOverlappingRanges(Span<int>(arr + 0, 2), Span<int>(arr + 2, 2)));
    }
    // Tests with std::vector
    {
        std::vector<int> v = { 1,2,3,4 };
        auto& v2 = v;
        auto v3 = v;
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(v, v));
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(v, v2));
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(v, Span<const int>(v.data() + 1, 1)));
        DFGTEST_EXPECT_FALSE(areContiguousByTypeAndOverlappingRanges(v, v3));
        DFGTEST_EXPECT_FALSE(areContiguousByTypeAndOverlappingRanges(v, makeRange(v.begin(), v.begin())));

    }
    // Tests with std::list
    {
        std::list<int> stdList = { 1,2,3,4 };
        DFGTEST_EXPECT_FALSE(areContiguousByTypeAndOverlappingRanges(stdList, stdList));
        const auto iterFirst = stdList.begin();
        const auto iterSecond = [&]() { auto iter = stdList.begin(); return ++iter; }();
        const auto iterThird = [&]() { auto iter = iterSecond; return ++iter; }();
        DFGTEST_EXPECT_FALSE(areContiguousByTypeAndOverlappingRanges(makeRange(iterFirst, iterFirst), makeRange(iterFirst, iterFirst)));
        // Single element ranges are considered contiguous even if container is not contiguous
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(makeRange(iterFirst, iterSecond), makeRange(iterFirst, iterSecond)));
        // With two elements should not get contiguous range anymore.
        DFGTEST_EXPECT_FALSE(areContiguousByTypeAndOverlappingRanges(makeRange(iterFirst, iterThird), makeRange(iterFirst, iterThird)));

        const std::list<int> singleList = { 1 };
        const auto singleListCopy = singleList;
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(singleList, singleList));
        DFGTEST_EXPECT_FALSE(areContiguousByTypeAndOverlappingRanges(singleList, singleListCopy));
        DFGTEST_EXPECT_FALSE(areContiguousByTypeAndOverlappingRanges(singleListCopy, singleList));
    }

    // Ranges of different types
    {
        std::vector<int> v = { 0, 1, 2, 3, 4 };
        const char* pBegin = reinterpret_cast<const char*>(v.data());
        const char* pLast = reinterpret_cast<const char*>(v.data() + 4);
        const char* pEnd = pLast + sizeof(int);
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(v, makeRange(pBegin, pLast)));
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(makeRange(pBegin, pLast), v));
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(v, makeRange(pLast, pEnd)));
        DFGTEST_EXPECT_TRUE(areContiguousByTypeAndOverlappingRanges(makeRange(pLast, pEnd), v));
        DFGTEST_EXPECT_FALSE(areContiguousByTypeAndOverlappingRanges(v, makeRange(pEnd, pEnd)));
    }
}

#endif
