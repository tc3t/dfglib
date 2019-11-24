#include <stdafx.h>
#include <dfg/rangeIterator.hpp>
#include <dfg/func/memFunc.hpp>
#include <dfg/alg.hpp>
#include <list>
#include <vector>
#include <deque>
#include <set>
#include <array>
#include <dfg/ptrToContiguousMemory.hpp>
#include <dfg/iterAll.hpp>
#include <dfg/typeTraits.hpp>

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

        EXPECT_EQ(vecRange.size(), vecConstRange.size());
        EXPECT_EQ(vecRange.size(), ptrRange.size());
        EXPECT_EQ(vecRange.size(), cptrRange.size());

        EXPECT_EQ(vec[0], *vecRange.begin());
        EXPECT_EQ(vec[0], *vecConstRange.begin());
        EXPECT_EQ(vec[0], *ptrRange.begin());
        EXPECT_EQ(vec[0], *cptrRange.begin());

        EXPECT_EQ(vec[1], *(vecRange.begin() + 1));
        EXPECT_EQ(vec[1], *(vecConstRange.begin() + 1));
        EXPECT_EQ(vec[1], *(ptrRange.begin() + 1));
        EXPECT_EQ(vec[1], *(cptrRange.begin() + 1));

        std::vector<int> vecEmpty;
        auto vecEmptyRange = DFG_ROOT_NS::makeRange(vecEmpty);
        DFG_ROOT_NS::DFG_CLASS_NAME(RangeIterator_T)<int*> ptrEmptyRange(vecEmptyRange);
        EXPECT_TRUE(ptrEmptyRange.empty());
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

#define TEST_FOR_ALL_CONTS(ELEMTYPE) \
    DFGTEST_STATIC(IsContiguousMemoryIterator<ELEMTYPE*>::value == true); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<const ELEMTYPE*>::value == true); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::vector<ELEMTYPE>::const_iterator>::value == true); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::vector<ELEMTYPE>::const_iterator>::value == true); \
    /*DFGTEST_STATIC((IsContiguousMemoryIterator<std::array<ELEMTYPE, 1>::iterator>::value == true));*/ \
    /*DFGTEST_STATIC((IsContiguousMemoryIterator<std::array<ELEMTYPE, 1>::const_iterator>::value == true));*/ \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::list<ELEMTYPE>::iterator>::value == false); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::list<ELEMTYPE>::const_iterator>::value == false); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::deque<ELEMTYPE>::iterator>::value == false); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::deque<ELEMTYPE>::const_iterator>::value == false); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::set<ELEMTYPE>::iterator>::value == false); \
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::set<ELEMTYPE>::const_iterator>::value == false);
    

    TEST_FOR_ALL_CONTS(char);
    TEST_FOR_ALL_CONTS(int);
    TEST_FOR_ALL_CONTS(double);
    TEST_FOR_ALL_CONTS(std::string);
    TEST_FOR_ALL_CONTS(std::vector<double>);

    DFGTEST_STATIC(IsContiguousMemoryIterator<std::string::iterator>::value == true);
    DFGTEST_STATIC(IsContiguousMemoryIterator<std::string::const_iterator>::value == true);

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
