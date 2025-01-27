#include "stdafx.h"

#if (defined(DFGTEST_BUILD_MODULE_ITER) && DFGTEST_BUILD_MODULE_ITER == 1) || (!defined(DFGTEST_BUILD_MODULE_ITER) && DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/rangeIterator.hpp>
#include <dfg/func/memFunc.hpp>
#include <dfg/alg.hpp>
#include <list>
#include <vector>
#include <deque>
#include <set>
#include <array>
#include <functional>
#include <dfg/ptrToContiguousMemory.hpp>
#include <dfg/iterAll.hpp>
#include <dfg/typeTraits.hpp>
#include <dfg/iter/CustomAccessIterator.hpp>
#include <dfg/cont/TrivialPair.hpp>
#include <dfg/iter/FunctionValueIterator.hpp>
#include <dfg/Span.hpp>

TEST(dfgIter, RangeIterator)
{
    {
        std::vector<double> vec;
        DFG_ROOT_NS::DFG_CLASS_NAME(RangeIterator_T) < std::vector<double>::const_iterator > range(vec.begin(), vec.end());
        std::for_each(range.begin(), range.end(), [](double) {});
        DFG_MODULE_NS(alg)::forEachFwd(range, [](double) {});
        EXPECT_TRUE(range.empty()); // Test empty() -member function.
    }

    // Test construction of const T* range from T* range.
    {
        int values[] = { 1, 2, 3 };
        auto range0 = DFG_ROOT_NS::makeRange(values);
        DFG_ROOT_NS::DFG_CLASS_NAME(RangeIterator_T)<const int*> range0CopyConstructor(range0);
        DFG_ROOT_NS::DFG_CLASS_NAME(RangeIterator_T)<const int*> range0Assignment;
        range0Assignment = range0;
        DFG_ROOT_NS::DFG_CLASS_NAME(RangeIterator_T)<const int*> range0Const(range0);
        EXPECT_EQ(range0.begin(), range0Const.begin());
        EXPECT_EQ(range0.begin(), range0Assignment.begin());
        EXPECT_EQ(range0.end(), range0Const.end());
        EXPECT_EQ(range0.end(), range0Assignment.end());
    }

    // Test construction of T* range from contiguous memory iterator.
    {
        std::vector<int> vec;
        vec.push_back(1);
        vec.push_back(2);
        const auto& vecConst = vec;
        auto vecRange = DFG_ROOT_NS::makeRange(vec);
        auto vecConstRange = DFG_ROOT_NS::makeRange(vecConst);
        DFG_ROOT_NS::DFG_CLASS_NAME(RangeIterator_T)<int*> ptrRange(vecRange);
        //DFG_ROOT_NS::DFG_CLASS_NAME(RangeIterator_T)<int*> ptr2Range(vecConstRange); // This should fail to compile
        DFG_ROOT_NS::DFG_CLASS_NAME(RangeIterator_T)<const int*> cptrRange(vecRange);
        auto dataRange = ::DFG_ROOT_NS::makeRange_data(vec);

        EXPECT_EQ(vecRange.size(), vecConstRange.size());
        EXPECT_EQ(vecRange.size(), ptrRange.size());
        EXPECT_EQ(vecRange.size(), cptrRange.size());

        EXPECT_EQ(dataRange.begin(), cptrRange.begin());
        EXPECT_EQ(dataRange.end(), cptrRange.end());

        EXPECT_EQ(vec[0], *vecRange.begin());
        EXPECT_EQ(vec[0], *vecConstRange.begin());
        EXPECT_EQ(vec[0], *ptrRange.begin());
        EXPECT_EQ(vec[0], *cptrRange.begin());

        EXPECT_EQ(vec[1], *(vecRange.begin() + 1));
        EXPECT_EQ(vec[1], *(vecConstRange.begin() + 1));
        EXPECT_EQ(vec[1], *(ptrRange.begin() + 1));
        EXPECT_EQ(vec[1], *(cptrRange.begin() + 1));

        EXPECT_EQ(1, vecRange.front());
        EXPECT_EQ(2, vecRange.back());

        std::vector<int> vecEmpty;
        auto vecEmptyRange = DFG_ROOT_NS::makeRange(vecEmpty);
        DFG_ROOT_NS::DFG_CLASS_NAME(RangeIterator_T)<int*> ptrEmptyRange(vecEmptyRange);
        EXPECT_TRUE(ptrEmptyRange.empty());

        EXPECT_EQ(2, vecRange.size());
        EXPECT_EQ(2, vecRange.sizeAsInt());
    }

#if 0 // If enabled, should fail to compile - can't create T* range from non-contiguous range.
    {
        std::list<int> list;
        auto listRange = DFG_ROOT_NS::makeRange(list);
        DFG_ROOT_NS::DFG_CLASS_NAME(RangeIterator_T)<int*> ptrRange(listRange);
    }
#endif

    // Test that range copy constructor works when dealing with non-pointer ranges.
    {
        std::list<int> list;
        list.push_back(1);
        auto range = DFG_ROOT_NS::makeRange(list);
        DFG_ROOT_NS::DFG_CLASS_NAME(RangeIterator_T)<std::list<int>::const_iterator> range2(range);
        ASSERT_EQ(range.size(), range2.size());
        EXPECT_EQ(1, *range.begin());
        EXPECT_EQ(1, *range2.begin());
        EXPECT_EQ(range.begin(), range2.begin());

        EXPECT_EQ(1, range.front());
        EXPECT_EQ(1, range.back());
    }

    // Testing operator[].
    {
        std::vector<double> vec = { 5, 6 };
        EXPECT_EQ(5, DFG_ROOT_NS::makeRange(vec)[0]);
        EXPECT_EQ(6, DFG_ROOT_NS::makeRange(vec)[1]);

        const std::vector<double> vecConst = {7, 8};
        EXPECT_EQ(7, DFG_ROOT_NS::makeRange(vecConst)[0]);
        EXPECT_EQ(8, DFG_ROOT_NS::makeRange(vecConst)[1]);

        // Testing that operator[] works even if iterator is not random access.
        std::list<int> list({2, 3});
        EXPECT_EQ(2, DFG_ROOT_NS::makeRange(list)[0]);
        EXPECT_EQ(3, DFG_ROOT_NS::makeRange(list)[1]);
    }

    // Testing construction from containers
    {
        using namespace ::DFG_ROOT_NS;
        const auto func = [](RangeIterator_T<int*> arg) { return arg; };
        const auto func2 = [](RangeIterator_T<std::list<int>::iterator> arg) { return arg; };
        std::vector<int> vec{ 1,2,3 };
        const std::vector<int> vecC{ 1,2,3 };
        auto r0 = func(vec);
        EXPECT_EQ(vec.data(), r0.begin());
        EXPECT_EQ(vec.data() + vec.size(), r0.end());
        std::list<int> lst = { 4, 5, 6 };
        auto r1 = func2(lst);
        EXPECT_EQ(lst.begin(), r1.begin());
        EXPECT_EQ(lst.end(), r1.end());
    }
}

TEST(dfgIter, RangeIterator_isSizeOne)
{
    using namespace DFG_ROOT_NS;
    {
        std::vector<int> v1 = { 1 };
        std::vector<int> v2 = { 1, 2 };
        std::list<int> l1 = { 1 };
        std::list<int> l2 = { 1, 2 };
        DFGTEST_EXPECT_FALSE(makeRange<int*>(nullptr, nullptr).isSizeOne());
        DFGTEST_EXPECT_TRUE(makeRange(v1).isSizeOne());
        DFGTEST_EXPECT_FALSE(makeRange(v2).isSizeOne());
        DFGTEST_EXPECT_TRUE(makeRange(l1).isSizeOne());
        DFGTEST_EXPECT_FALSE(makeRange(l2).isSizeOne());
    }
}

TEST(dfgIter, makeRange)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(alg);
    const double arr[] = { 1, 2, 3, 5, 6, 7 };
    std::vector<double> vec(cbegin(arr), cend(arr));
    const auto vecConst = vec;
    std::vector<double> vecEmpty;
    std::list<double> list(cbegin(arr), cend(arr));
    auto avgFunc = DFG_MODULE_NS(func)::DFG_CLASS_NAME(MemFuncAvg)<double>();
    auto rangeArr = makeRange(arr);
    DFGTEST_STATIC((std::is_same<std::remove_reference<decltype(*rangeArr.begin())>::type, const double>::value));
    auto rangeVec = makeRange(vec);
    auto rangeVecConst = makeRange(vecConst);
    auto rangeVecEmpty = makeRange(vecEmpty);
    DFGTEST_STATIC((std::is_same<decltype(rangeVec.data()), double*>::value));
    DFGTEST_STATIC((std::is_same<decltype(rangeVecConst.data()), const double*>::value));
    auto rangeList = makeRange(list);
    avgFunc = DFG_MODULE_NS(alg)::forEachFwd(rangeArr, avgFunc);
    EXPECT_EQ(4.0, avgFunc.average());
    avgFunc = DFG_MODULE_NS(alg)::forEachFwd(rangeVec, avgFunc);
    EXPECT_EQ(4.0, avgFunc.average());
    avgFunc = DFG_MODULE_NS(alg)::forEachFwd(rangeList, avgFunc);
    EXPECT_EQ(4.0, avgFunc.average());

    int vals[3] = { 10, 20, 30 };
    auto rangeVals = makeRange(vals);
    DFGTEST_STATIC((std::is_same<std::remove_reference<decltype(*rangeVals.begin())>::type, int>::value));
    forEachFwd(rangeVals, [](int& v) {v += 1; });
    EXPECT_EQ(11, vals[0]);
    EXPECT_EQ(21, vals[1]);
    EXPECT_EQ(31, vals[2]);
}

TEST(dfgIter, indexRangeIE)
{
    using namespace DFG_ROOT_NS;

    // Usage in range for
    {
        size_t nSum = 0;

        for (const auto i : indexRangeIE(1, 1))
            nSum += i;
        DFGTEST_EXPECT_LEFT(nSum, 0);

        for (const auto i : indexRangeIE(1, 2))
            nSum += i;
        DFGTEST_EXPECT_LEFT(nSum, 1);
        DFGTEST_STATIC_TEST(sizeof(decltype(indexRangeIE(1, 2))) == 2 * sizeof(int));
    }

    // Handling of data type max boundary
    {
        int nSum = 0;
        const auto range = indexRangeIE<int8>(int8_min, int8_max);
        for (const auto i : range)
            nSum += i;
        DFGTEST_EXPECT_LEFT(nSum, -128 - 127);
        DFGTEST_EXPECT_LEFT(255, range.end() - range.begin());
        DFGTEST_EXPECT_LEFT(255u, range.size());
        DFGTEST_STATIC_TEST(sizeof(range) == 2 * sizeof(char));
    }
}

TEST(dfgIter, makeRangeFromStartAndEndIndex)
{
    using namespace DFG_ROOT_NS;

    {
        std::vector<int> v{ 1,2,3,4 };
        const auto range04 = makeRangeFromStartAndEndIndex(v, 0, 4);
        const auto range13 = makeRangeFromStartAndEndIndex(v, 1, 3);
        DFGTEST_EXPECT_LEFT(4, range04.size());
        DFGTEST_EXPECT_LEFT(v.data(), range04.data());
        DFGTEST_EXPECT_LEFT(true, makeRangeFromStartAndEndIndex(v, 0, 0).empty());
        DFGTEST_EXPECT_LEFT(true, makeRangeFromStartAndEndIndex(v, 4, 4).empty());
        DFGTEST_EXPECT_LEFT(2, range13.size());
        DFGTEST_EXPECT_LEFT(v.data() + 1, range13.data());

        DFGTEST_EXPECT_LEFT(0, makeRangeFromStartAndEndIndex(std::vector<int>(), 0, 0).size());
    }
}

TEST(dfgIter, headRange)
{
    using namespace DFG_ROOT_NS;
    int vals[] = { 10, 20, 30, 40, 50 };
    auto rangeVals = makeRange(vals);
    EXPECT_TRUE(headRange(rangeVals, 0).empty());
    EXPECT_EQ(1, headRange(rangeVals, 1).size());
    EXPECT_EQ(10, *headRange(rangeVals, 1).begin());

    DFGTEST_STATIC((std::is_same<decltype(rangeVals), decltype(headRange(rangeVals, 0))>::value));
    DFGTEST_STATIC((std::is_same<decltype(rangeVals), decltype(headRange(vals, 0))>::value));

    EXPECT_EQ(2, headRange(rangeVals, 2).size());
    EXPECT_EQ(10, *headRange(rangeVals, 2).begin());
    EXPECT_EQ(20, *(headRange(rangeVals, 2).begin() + 1));

    EXPECT_EQ(3, headRange(vals, 3).size());
    EXPECT_EQ(10, *headRange(vals, 3).begin());
    EXPECT_EQ(20, *(headRange(vals, 3).begin() + 1));
    EXPECT_EQ(30, *(headRange(vals, 3).begin() + 2));

    std::list<int> listCont(rangeVals.begin(), rangeVals.end());
    EXPECT_EQ(2, headRange(listCont, 2).size());
    EXPECT_EQ(10, *(headRange(listCont, 2).begin()));
}

TEST(dfgIter, dataMethod)
{
    using namespace DFG_ROOT_NS;
    const double arr[] = { 1, 2, 3, 5, 6, 7 };
    
    auto range = makeRange(arr);
    const auto crange = makeRange(arr);
    auto range2 = makeRange(arr, arr + DFG_COUNTOF(arr));
    auto crange2 = makeRange(arr, arr + DFG_COUNTOF(arr));
    EXPECT_EQ(arr, range.data());
    EXPECT_EQ(arr, range2.data());

    EXPECT_EQ(arr, ptrToContiguousMemory(range));
    EXPECT_EQ(arr, ptrToContiguousMemory(crange));
    EXPECT_EQ(arr, ptrToContiguousMemory(range2));
    EXPECT_EQ(arr, ptrToContiguousMemory(crange2));

    // This should fail to compile
#if 0
    std::vector<int> vec;
    auto range3 = makeRange(vec);
    range3.data();
#endif
}

TEST(dfgIter, IsContiguousMemoryIterator)
{
    using namespace DFG_ROOT_NS;

    DFGTEST_STATIC((IsStdArrayContainer<std::vector<int>>::value == false));
    DFGTEST_STATIC((IsStdArrayContainer<std::array<int, 1>>::value == true));

    // There didn't seem to be feature test macro for std::contiguous_iterator, so filtering by C++ standard
#if DFG_CPLUSPLUS >= DFG_CPLUSPLUS_20
    #define DFG_TEMP_CPP20_CASES(ELEMTYPE) \
        DFGTEST_STATIC((IsContiguousMemoryIterator<std::array<ELEMTYPE, 1>::iterator>::value == true)); \
        DFGTEST_STATIC((IsContiguousMemoryIterator<std::array<ELEMTYPE, 1>::const_iterator>::value == true)); \
        DFGTEST_STATIC(IsContiguousMemoryIterator<Span<ELEMTYPE>::iterator>::value == true); \
        DFGTEST_STATIC(IsContiguousMemoryIterator<Span<const ELEMTYPE>::iterator>::value == true);
#else
    #define DFG_TEMP_CPP20_CASES(ELEMTYPE)
#endif

#define TEST_FOR_ALL_CONTS(ELEMTYPE) \
    DFGTEST_STATIC(IsContiguousMemoryIterator<ELEMTYPE*>::value == true); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<const ELEMTYPE*>::value == true); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::vector<ELEMTYPE>::const_iterator>::value == true); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::vector<ELEMTYPE>::const_iterator>::value == true); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::list<ELEMTYPE>::iterator>::value == false); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::list<ELEMTYPE>::const_iterator>::value == false); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::deque<ELEMTYPE>::iterator>::value == false); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::deque<ELEMTYPE>::const_iterator>::value == false); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::set<ELEMTYPE>::iterator>::value == false); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::set<ELEMTYPE>::const_iterator>::value == false); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<Span<ELEMTYPE>::iterator>::value == true); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<Span<const ELEMTYPE>::iterator>::value == true); \
    DFG_TEMP_CPP20_CASES(ELEMTYPE)

    TEST_FOR_ALL_CONTS(char);
    TEST_FOR_ALL_CONTS(int);
    TEST_FOR_ALL_CONTS(double);
    TEST_FOR_ALL_CONTS(std::string);
    TEST_FOR_ALL_CONTS(std::vector<double>);

    DFGTEST_STATIC(IsContiguousMemoryIterator<std::string::iterator>::value == true);
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::string::const_iterator>::value == true);

#undef DFG_TEMP_CPP20_CASES
#undef TEST_FOR_ALL_CONTS
}

TEST(dfgIter, szIterator)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(iter);
    using namespace DFG_MODULE_NS(alg);
    using namespace DFG_MODULE_NS(TypeTraits);

    char sz[] = "abc";
    const char szConst[] = "def";
    auto range = makeSzRange(sz);
    auto rangeConst = makeSzRange(szConst);
    DFG_UNUSED(rangeConst);

    DFGTEST_STATIC((std::is_same<TypeIdentity<decltype(makeSzIterator(sz))>::type::pointer, char*>::value));
    DFGTEST_STATIC((std::is_same<TypeIdentity<decltype(makeSzIterator(szConst))>::type::pointer, const char*>::value));
    DFGTEST_STATIC((std::is_same<TypeIdentity<decltype(makeSzRange(sz))>::type::pointer, char*>::value));
    DFGTEST_STATIC((std::is_same<TypeIdentity<decltype(makeSzRange(szConst))>::type::pointer, const char*>::value));

    EXPECT_EQ(&sz[0], range.data()); // Note: pointer comparison is intended.
    for (auto iter = range.begin(), iterEnd = range.end(); !isAtEnd(iter, iterEnd); ++iter)
        *iter += 1;
    EXPECT_STREQ(sz, "bcd");

    forEachFwd(range, [](char& ch)
    {
        ch += 1;
    });
    EXPECT_STREQ(sz, "cde");

    std::string s;
    std::copy(range.cbegin(), range.cend(), std::back_inserter(s));
    EXPECT_STREQ(sz, s.c_str());

    EXPECT_EQ(3, range.size());

    // Test post-increment operator
    {
        const char szText[] = "abc";
        auto iter = makeSzIterator(szText);
        const char* ptrIter = szText;
        EXPECT_EQ(*ptrIter++, *iter++);
        EXPECT_EQ(*ptrIter, *iter);
        EXPECT_EQ(*++ptrIter, *++iter);
    }
}

TEST(dfgIter, szIterator_operatorLt)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(iter);
    const char szText[] = "abc";
    const char szEmpty[] = "";
    auto iter = makeSzIterator(szText);
    auto iterRange = makeSzRange(szText);
    auto iterRangeEmpty = makeSzRange(szEmpty);
    EXPECT_TRUE(iterRange.begin() < iterRange.end());
    EXPECT_TRUE(iterRange.cbegin() < iterRange.cend());
    EXPECT_FALSE(iterRange.cend() < iterRange.cbegin());
    EXPECT_FALSE(iterRangeEmpty.begin() < iterRangeEmpty.end());
    EXPECT_FALSE(iterRangeEmpty.cbegin() < iterRangeEmpty.cend());
    EXPECT_FALSE(iterRangeEmpty.cend() < iterRangeEmpty.cbegin());
    EXPECT_FALSE(iter < iter);
    EXPECT_FALSE(iterRange.cend() < iterRange.cend());
    EXPECT_TRUE(iter < makeSzIterator(szText + 1));
    EXPECT_TRUE(iter < makeSzIterator(szText + 2));
    EXPECT_TRUE(iter < makeSzIterator(szText + 3));
    EXPECT_FALSE(makeSzIterator(szText + 1) < iter);
    EXPECT_FALSE(makeSzIterator(szText + 2) < iter);
    EXPECT_FALSE(makeSzIterator(szText + 3) < iter);
}

namespace
{
    template <class T>
    static void rawStorageIteratorTestImpl()
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(iter);
        char storage[5*sizeof(T)];

        char* pAlignedStorageStart = storage;

        while (reinterpret_cast<uintptr_t>(pAlignedStorageStart) % std::alignment_of<T>::value != 0)
            pAlignedStorageStart++;

        char* pAlignedStorageEnd = pAlignedStorageStart + 2*sizeof(T);

        char* const pUnalignedStorageStart = pAlignedStorageEnd + 1;
        char* const pUnalignedStorageEnd = pUnalignedStorageStart + 2*sizeof(T);

        T val = 1;
        memcpy(pAlignedStorageStart, &val, sizeof(val));
        val++;
        memcpy(pAlignedStorageStart + sizeof(val), &val, sizeof(val));
        val++;

        memcpy(pUnalignedStorageStart, &val, sizeof(val));
        val++;

        memcpy(pUnalignedStorageStart + sizeof(val), &val, sizeof(val));
        val++;

        RawStorageIterator<T> rsIter0(pAlignedStorageStart);
        RawStorageIterator<T> rsIter0Copy(rsIter0);
        DFGTEST_STATIC_TEST(sizeof(*rsIter0) == sizeof(T));
        EXPECT_TRUE(rsIter0 == rsIter0Copy);
        EXPECT_EQ(pAlignedStorageStart, rsIter0.ptrChar());
        EXPECT_EQ(1, *rsIter0);
        rsIter0++;
        EXPECT_EQ(2, *rsIter0);

        int16 i = 3;
        for (RawStorageIterator<T> rsIter1(pUnalignedStorageStart), iterEnd(pUnalignedStorageEnd); rsIter1 != iterEnd; ++rsIter1)
        {
            EXPECT_EQ(i, *rsIter1);
            i++;
        }
    }
}

TEST(dfgIter, RawStorageIterator)
{
    rawStorageIteratorTestImpl<int>();
    rawStorageIteratorTestImpl<double>();
}

TEST(dfgIter, InterleavedSemiIterator)
{
    double arr[] = { 1, 2, 3, 4, 5, 6 };
    DFG_MODULE_NS(iter)::DFG_CLASS_NAME(InterleavedSemiIterator)<double> iter(arr, 2);
    EXPECT_EQ(1, *iter);
    EXPECT_EQ(2, iter.getChannel(1));
    EXPECT_EQ(1, *iter++);
    EXPECT_EQ(3, *iter);
    ++iter;
    EXPECT_EQ(5, *iter);
    EXPECT_EQ(6, iter.getChannel(1));
    --iter;
    EXPECT_EQ(3, *iter);
    EXPECT_EQ(3, *iter--);
    EXPECT_EQ(1, *iter);
    EXPECT_EQ(2, iter.getChannel(1));
}

namespace
{
    class TestStruct
    {
    public:
        double d = 1.5;
        std::string s = "a";
        int i = 3;

        class IntAccesser
        {
        public:
            int* operator()(const std::vector<TestStruct>::iterator& iter)
            {
                return &iter->i;
            }

        };

        class StringAccesser
        {
        public:
            std::string* operator()(const std::vector<TestStruct>::iterator& iter)
            {
                return &iter->s;
            }

        };
    };

    template <size_t Index_T, class Cont_T, class ExpectedCont_T>
    static void testCustomAccessIterator(Cont_T& cont, const ExpectedCont_T& expected)
    {
        using namespace DFG_MODULE_NS(iter);
        auto iter = makeTupleElementAccessIterator<Index_T>(cont.begin());

        EXPECT_EQ(cont.begin(), iter.underlyingIterator());
        EXPECT_EQ(expected[0], *iter);
        EXPECT_EQ(expected[1], *(iter + 1));
        EXPECT_EQ(expected[2], *(iter + 2));
        EXPECT_EQ(expected[0], *iter);
        EXPECT_EQ(expected[1], *++iter);
        EXPECT_EQ(expected[1], *iter++);
        EXPECT_EQ(expected[2], *iter);
        iter += expected.size() - 2;
        EXPECT_EQ(cont.end(), iter.underlyingIterator());
        --iter;
        iter -= expected.size() - 3;
        EXPECT_EQ(expected[2], *iter);
        EXPECT_EQ(expected[2], *iter--);
        EXPECT_EQ(expected[1], *iter);
        EXPECT_EQ(expected[0], *(iter - 1));
        iter += 1;
        EXPECT_EQ(expected[2], *iter);
        iter -= 2;
        EXPECT_EQ(expected[0], *iter);
        EXPECT_EQ(0, iter - iter);
        EXPECT_EQ(2, (iter + 2) - iter);
    }
}

TEST(dfgIter, CustomAccessIterator)
{
    using namespace DFG_MODULE_NS(iter);

    // Testing makeTupleElementAccessIterator()
    {
        std::vector<std::pair<std::string, double>> vecPairs;
        vecPairs.push_back(std::pair<std::string, double>("a", 1.25));
        vecPairs.push_back(std::pair<std::string, double>("b", 1.5));
        vecPairs.push_back(std::pair<std::string, double>("c", 1.75));
        const auto vecPairsConstRef = vecPairs;

        testCustomAccessIterator<0>(vecPairs, std::array<std::string, 3>({"a", "b", "c"}));
        testCustomAccessIterator<0>(vecPairsConstRef, std::array<std::string, 3>({ "a", "b", "c" }));
        testCustomAccessIterator<1>(vecPairs, std::array<double, 3>({ 1.25, 1.5, 1.75 }));
        testCustomAccessIterator<1>(vecPairsConstRef, std::array<double, 3>({ 1.25, 1.5, 1.75 }));

        // Testing operator->
        EXPECT_EQ(1, makeTupleElementAccessIterator<0>(vecPairs.begin())->length());

        // Testing modification
        {
            auto iter0 = makeTupleElementAccessIterator<0>(vecPairs.begin());
            auto iter1 = makeTupleElementAccessIterator<1>(vecPairs.begin());
            *iter0 = "aa";
            *iter1 = 2.25;
            EXPECT_EQ("aa", vecPairs.begin()->first);
            EXPECT_EQ(2.25, vecPairs.begin()->second);
        }
    }

    // Testing makeTupleElementAccessIterator() with TrivialPair
    {
        using namespace DFG_MODULE_NS(cont);
        std::vector<TrivialPair<std::string, double>> vecPairs;
        vecPairs.push_back(TrivialPair<std::string, double>("a", 1.25));
        vecPairs.push_back(TrivialPair<std::string, double>("b", 1.5));
        auto iter0 = makeTupleElementAccessIterator<0>(vecPairs.begin());
        auto iter1 = makeTupleElementAccessIterator<1>(vecPairs.begin());
        EXPECT_EQ("a", *iter0++);
        EXPECT_EQ(1.25, *iter1++);
        EXPECT_EQ("b", *iter0++);
        EXPECT_EQ(1.5, *iter1++);
        EXPECT_EQ(vecPairs.end(), iter0.underlyingIterator());
        EXPECT_EQ(vecPairs.end(), iter1.underlyingIterator());
    }

    // Testing custom access func
    {
        std::vector<TestStruct> cont;
        cont.push_back(TestStruct());
        cont.push_back(TestStruct());
        cont.push_back(TestStruct());
        cont.front().i = -1;
        cont.front().s = "abc";
        auto iterInt = CustomAccessIterator<decltype(cont.begin()), TestStruct::IntAccesser>(cont.begin());
        auto iterString = CustomAccessIterator<decltype(cont.begin()), TestStruct::StringAccesser>(cont.begin());
        EXPECT_EQ(-1, *iterInt);
        EXPECT_EQ("abc", *iterString);
        iterInt += 2;
        iterString += 2;
        EXPECT_EQ(3, *iterInt);
        EXPECT_EQ("a", *iterString);
        ++iterInt;
        ++iterString;
        EXPECT_EQ(cont.end(), iterInt.underlyingIterator());
        EXPECT_EQ(cont.end(), iterString.underlyingIterator());
    }   
}

namespace
{
    // Tests integer arithmetics, e.g. that negative shifts behave reasonably when Index_T is unsigned
    template <class Int_T>
    void testFunctionValueIteratorArithmetic()
    {
        using namespace DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(iter);
        using namespace ::DFG_MODULE_NS(math);
        auto iter = makeIndexIterator(Int_T(21));
        iter -= (-2);
        iter += (-3);
        DFGTEST_EXPECT_LEFT(20, iter.index());
        DFGTEST_EXPECT_LEFT(10, (iter + (-10)).index());
        DFGTEST_EXPECT_LEFT(30, (iter - (-10)).index());

        DFGTEST_EXPECT_LEFT(-2, makeIndexIterator(Int_T(1)) - makeIndexIterator(Int_T(3)));
        using diffT = typename decltype(iter)::difference_type;
        if constexpr (sizeof(Int_T) < sizeof(diffT))
        {
            const auto maxVal = maxValueOfType<Int_T>();
            const auto minVal = minValueOfType<Int_T>();
            DFGTEST_EXPECT_LEFT(numericDistance(maxVal, minVal), makeIndexIterator(maxVal) - makeIndexIterator(minVal));
            DFGTEST_EXPECT_LEFT(absAsUnsigned(minVal), makeIndexIterator(Int_T(0)) - makeIndexIterator(minVal));
            DFGTEST_EXPECT_LEFT(diffT(-1) * maxVal, makeIndexIterator(Int_T(0)) - makeIndexIterator(maxVal));
            DFGTEST_EXPECT_LEFT(diffT(minVal) - diffT(maxVal), makeIndexIterator(minVal) - makeIndexIterator(maxVal));
        }
    }
}

TEST(dfgIter, FunctionValueIterator)
{
    using namespace ::DFG_MODULE_NS(iter);

    const auto testAssignment = [](const auto& iter)
    {
        std::remove_const_t<std::remove_reference_t<decltype(iter)>> iter2 = iter;
        iter2 = iter;
        DFG_UNUSED(iter2);
    };

    // FuncCat tests
    {
        using namespace ::DFG_MODULE_NS(iter)::DFG_DETAIL_NS;
        const auto statelessLambda = [](size_t i) { return i; };
        int i = 0;
        const auto statefullLambda = [&](int n) { return n + i; };
        using FreeFuncPtr = size_t (*)(size_t);
#if (DFG_CPLUSPLUS >= DFG_CPLUSPLUS_20)
        DFGTEST_STATIC_TEST((std::is_same_v<FuncCatStateless,   FuncCat<decltype(statelessLambda)>>));
#else // Case: before C++20
        DFGTEST_STATIC_TEST((std::is_same_v<FuncCatStdFunction, FuncCat<decltype(statelessLambda)>>));
#endif // (DFG_CPLUSPLUS >= DFG_CPLUSPLUS_20)

        DFGTEST_STATIC_TEST((std::is_same_v<FuncCatFuncPtr,     FuncCat<FreeFuncPtr>>));
        DFGTEST_STATIC_TEST((std::is_same_v<FuncCatStdFunction, FuncCat<decltype(statefullLambda)>>));
    }

    // Basic test
    {
        const auto func = [](size_t i) { return i; };
        auto iter = makeFunctionValueIterator(size_t(0), func);
        std::vector<size_t> vals;
        std::copy(iter, iter + 5, std::back_inserter(vals));
        EXPECT_EQ(std::vector<size_t>({0, 1, 2, 3, 4}), vals);
        // Captureless lambdas are default constructible and copy assignable only since C++20
#if (DFG_CPLUSPLUS >= DFG_CPLUSPLUS_20)
        DFGTEST_STATIC_TEST(sizeof(iter) == sizeof(size_t));
#endif // (DFG_CPLUSPLUS >= DFG_CPLUSPLUS_20)
        testAssignment(iter);
    }

    // Testing operators 
    {
        auto iter = makeFunctionValueIterator(size_t(0), [](size_t i) { return i * 10; });
        EXPECT_EQ(0, *iter);
        EXPECT_EQ(10, *(iter + 1));
        EXPECT_EQ(20, *(iter + 2));
        EXPECT_EQ(0, *iter);
        EXPECT_EQ(10, *++iter);
        EXPECT_EQ(10, *iter++);
        EXPECT_EQ(20, *iter);
        EXPECT_EQ(20, *iter--);
        EXPECT_EQ(10, *iter);
        EXPECT_EQ(0, *(iter - 1));
        iter += 1;
        EXPECT_EQ(20, *iter);
        iter -= 2;
        EXPECT_EQ(0, *iter);
        EXPECT_EQ(0, iter - iter);
        EXPECT_EQ(2, (iter + 2) - iter);
    }

    // Testing handling of custom functor
    {
        struct Functor { int operator()(int i) const { return i * 2; } };
        auto iter = makeFunctionValueIterator(int(0), Functor());
        DFGTEST_EXPECT_LEFT(0, *iter++);
        DFGTEST_EXPECT_LEFT(1, iter.index());
        DFGTEST_EXPECT_LEFT(2, *iter++);
        DFGTEST_EXPECT_LEFT(2, iter.index());
        DFGTEST_EXPECT_LEFT(4, *iter++);
        DFGTEST_EXPECT_LEFT(3, iter.index());
        DFGTEST_EXPECT_LEFT(13, (iter + 10).index());
        DFGTEST_EXPECT_LEFT(0, (iter - 3).index());
        DFGTEST_STATIC_TEST(sizeof(iter) == sizeof(int));
        testAssignment(iter);
    }

    // Testing handling of function pointer
    {
        using FreeFuncPtr = int (*)(int);
        FreeFuncPtr p = [](int i) { return i; };
        auto iter = makeFunctionValueIterator(int(0), p);
        DFGTEST_EXPECT_LEFT(0, *iter++);
        DFGTEST_EXPECT_LEFT(1, *iter++);
        DFGTEST_EXPECT_LEFT(2, *iter++);
        testAssignment(iter);
    }

    // Testing use of std::function for stateful lambdas
    {
        double val = 1;
        const auto func = [&](size_t i) { return static_cast<double>(i) + val; };
        auto iter = makeFunctionValueIterator(size_t(0), std::function<double (size_t)>(func));
        EXPECT_EQ(1, *iter);
        val = 2;
        EXPECT_EQ(2, *iter);
        DFGTEST_STATIC_TEST(sizeof(iter) > sizeof(size_t));
        testAssignment(iter);
    }

    // makeIndexIterator
    {
        auto iter = makeIndexIterator(size_t(0));
        std::vector<size_t> vals;
        std::copy(iter, iter + 3, std::back_inserter(vals));
        EXPECT_EQ(std::vector<size_t>({ 0, 1, 2 }), vals);
        std::copy(iter + 10, ((iter + 12) - 5) + 5, std::back_inserter(vals));
        EXPECT_EQ(std::vector<size_t>({ 0, 1, 2, 10, 11 }), vals);
        testAssignment(iter);

        DFGTEST_STATIC_TEST(sizeof(iter) == sizeof(size_t));
        DFGTEST_STATIC_TEST(sizeof(decltype(makeIndexIterator(short(0)))) == sizeof(short));
    }

    // signed/unsigned handling
    {
        using namespace ::DFG_ROOT_NS;
        testFunctionValueIteratorArithmetic<uint16>();
        testFunctionValueIteratorArithmetic<int16>();
        testFunctionValueIteratorArithmetic<uint32>();
        testFunctionValueIteratorArithmetic<int32>();
        testFunctionValueIteratorArithmetic<size_t>();
        testFunctionValueIteratorArithmetic<uint64>();
        testFunctionValueIteratorArithmetic<int64>();
    }

    // makeIndexIterator() with double
    {
        auto iter = makeIndexIterator(double(0));
        std::vector<double> vals;
        std::copy(iter, iter + 3, std::back_inserter(vals));
        DFGTEST_EXPECT_LEFT(std::vector<double>({ 0, 1, 2 }), vals);
        std::copy(iter + 10, ((iter + 12) - 5) + 5, std::back_inserter(vals));
        DFGTEST_EXPECT_LEFT(std::vector<double>({ 0, 1, 2, 10, 11 }), vals);
        testAssignment(iter);
        DFGTEST_EXPECT_LEFT(iter.index() + 10, (iter + 10).index());
        DFGTEST_EXPECT_LEFT(iter.index() + 3, ((iter + 5) - 2).index());
        const auto nIndex1 = iter.index();
        iter += 2;
        DFGTEST_EXPECT_LEFT(nIndex1 + 2, iter.index());
        iter -= 1;
        DFGTEST_EXPECT_LEFT(nIndex1 + 1, iter.index());
        DFGTEST_STATIC_TEST(sizeof(iter) == sizeof(double));

        // These should trigger a failing assert
        DFGTEST_EXPECT_ASSERT_FAILURE(makeIndexIterator(std::numeric_limits<double>::infinity()));
        DFGTEST_EXPECT_ASSERT_FAILURE(makeIndexIterator(std::numeric_limits<double>::quiet_NaN()));
    }
}

#endif

