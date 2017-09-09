#include <stdafx.h>
#include <dfg/cont.hpp>
#include <dfg/cont/table.hpp>
#include <dfg/cont/arrayWrapper.hpp>
#include <string>
#include <deque>
#include <list>
#include <dfg/ptrToContiguousMemory.hpp>
#include <dfg/dfgBase.hpp>
#include <dfg/ReadOnlySzParam.hpp>
#include <dfg/cont/interleavedXsortedTwoChannelWrapper.hpp>
#include <dfg/cont/valueArray.hpp>
#include <dfg/cont/MapVector.hpp>
#include <dfg/cont/ViewableSharedPtr.hpp>
#include <dfg/cont/SetVector.hpp>
#include <dfg/cont/SortedSequence.hpp>
#include <dfg/cont/tableCsv.hpp>
#include <dfg/cont/TorRef.hpp>
#include <dfg/cont/TrivialPair.hpp>
#include <dfg/rand.hpp>
#include <dfg/typeTraits.hpp>
#include <dfg/io/OmcByteStream.hpp>
#include <dfg/iter/szIterator.hpp>
#include <dfg/cont/contAlg.hpp>

TEST(dfgCont, makeVector)
{
    auto v1 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(1);
    ASSERT_EQ(v1.size(), 1);
    EXPECT_EQ(v1[0] == 1, true);

    auto v2 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(1, 2);
    EXPECT_EQ(v2.size(), 2);
    EXPECT_EQ(v2[0], 1);
    EXPECT_EQ(v2[1], 2);

    auto v2_2 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(std::string("a"), std::string("b"));
    EXPECT_EQ(v2_2.size(), 2);
    EXPECT_EQ(v2_2[0], "a");
    EXPECT_EQ(v2_2[1], "b");

    std::string s0 = "c";
    std::string s1 = "d";
    auto v2_3 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(s0, s1);
    EXPECT_EQ(v2_3.size(), 2);
    EXPECT_EQ(v2_3[0], "c");
    EXPECT_EQ(v2_3[1], "d");
    EXPECT_EQ(s0, "c");
    EXPECT_EQ(s1, "d");

    auto v3 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(8, 6, 9);
    EXPECT_EQ(v3.size() == 3 && v3[0] == 8 && v3[1] == 6 && v3[2] == 9, true);

    auto v4 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(5, 6, 7, 8);
    EXPECT_EQ(v4.size() == 4 && v4[0] == 5 && v4[1] == 6 && v4[2] == 7 && v4[3] == 8, true);

    /*
    auto v5 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(1.1, 2.2, 3.3, 4.4, 5.5);
    EXPECT_EQ(v5.size() == 5 && v5[0] == 1.1 && v5[1] == 2.2 && v5[2] == 3.3 && v5[3] == 4.4 && v5[4] == 5.5, true);

    auto v6 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector('a', 'b', 'c', 'd', 'e', 'f');
    EXPECT_EQ(v6.size() == 6 && v6[0] == 'a' && v6[1] == 'b' && v6[2] == 'c' && v6[3] == 'd' && v6[4] == 'e' && v6[5] == 'f', true);
    */
}

TEST(dfgCont, table)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);

    DFG_CLASS_NAME(Table)<int> table;
    table.pushBackOnRow(0, 0);
    table.pushBackOnRow(0, 1);
    table.pushBackOnRow(0, 2);
    EXPECT_EQ(3, table.getColumnCountOnRow(0));

    table.pushBackOnRow(1, 3);
    EXPECT_EQ(3, table.getColumnCountOnRow(0));
    EXPECT_EQ(1, table.getColumnCountOnRow(1));
    
    table.pushBackOnRow(0, 3);
    EXPECT_EQ(4, table.getColumnCountOnRow(0));
    EXPECT_EQ(1, table.getColumnCountOnRow(1));

    table.pushBackOnRow(2, 4);
    table.pushBackOnRow(2, 5);
    table.pushBackOnRow(2, 6);
    table.pushBackOnRow(2, 7);
    EXPECT_EQ(4, table.getColumnCountOnRow(0));
    EXPECT_EQ(1, table.getColumnCountOnRow(1));
    EXPECT_EQ(4, table.getColumnCountOnRow(2));

    table.pushBackOnRow(5, 9);
    EXPECT_EQ(4, table.getColumnCountOnRow(0));
    EXPECT_EQ(1, table.getColumnCountOnRow(1));
    EXPECT_EQ(4, table.getColumnCountOnRow(2));
    EXPECT_EQ(0, table.getColumnCountOnRow(3));
    EXPECT_EQ(0, table.getColumnCountOnRow(4));
    EXPECT_EQ(1, table.getColumnCountOnRow(5));
    EXPECT_EQ(0, table.getColumnCountOnRow(6)); // Non existing row

    table.setElement(7, 0, 70);
    table.setElement(6, 1, 601);
    table.setElement(6, 0, 600);

    EXPECT_EQ(0, table(0, 0));
    EXPECT_EQ(1, table(0, 1));
    EXPECT_EQ(2, table(0, 2));
    EXPECT_EQ(3, table(0, 3));

    EXPECT_EQ(3, table(1, 0));

    EXPECT_EQ(4, table(2, 0));
    EXPECT_EQ(5, table(2, 1));
    EXPECT_EQ(6, table(2, 2));
    EXPECT_EQ(7, table(2, 3));

    // This should cause assert to trigger.
    //EXPECT_EQ(9, table(4, 0));

    EXPECT_EQ(9, table(5, 0));

    EXPECT_EQ(70, table(7, 0));
    EXPECT_EQ(601, table(6, 1));
    EXPECT_EQ(600, table(6, 0));

}

TEST(dfgCont, TableSzUntypedInterface)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    
    DFG_CLASS_NAME(TableSz)<char> table;
    table.setBlockSize(100);

    const std::string sAlmostBlockSize(99, ' ');

    EXPECT_TRUE(table.addString("a", 8, 0));
    EXPECT_TRUE(table.addString(std::string("b"), 0, 0));
    EXPECT_TRUE(table.addString(sAlmostBlockSize, 7, 0));
    EXPECT_TRUE(table.addString("r1c3_initial", 1, 3));
    EXPECT_TRUE(table.addString("",  1, 0));
    EXPECT_TRUE(table.addString("cde", 4, 0));

    EXPECT_STREQ("r1c3_initial", table(1,3));

    EXPECT_TRUE(table.setElement(1, 3, "r1c3"));
    
    const size_t nItemCount = 5;
    const std::array<std::string, nItemCount> arrExpectedStrsInCol0 = { "b", "", "cde", sAlmostBlockSize, "a" };
    const std::array<uint32, nItemCount> arrExpectedRowsInCol0 = { 0, 1, 4, 7, 8 };
    size_t nCounter = 0;
    table.forEachFwdRowInColumn(0, [&](const uint32 row, const char* psz)
    {
        EXPECT_TRUE(DFG_ROOT_NS::isValidIndex(arrExpectedStrsInCol0, nCounter));
        EXPECT_EQ(arrExpectedStrsInCol0[nCounter], psz);
        EXPECT_EQ(arrExpectedRowsInCol0[nCounter], row);
        ++nCounter;
    });
    EXPECT_EQ(nItemCount, nCounter);
    table.forEachFwdRowInColumn(1, [&](const uint32, const char*)
    {
        EXPECT_FALSE(true); // Should not reach here since there are no items in column 1.
    });

    const std::array<std::string, nItemCount> arrExpectedStrsInCol3 = { "r1c3" };
    const std::array<uint32, nItemCount> arrExpectedRowsInCol3 = { 1 };
    nCounter = 0;
    table.forEachFwdRowInColumn(3, [&](const uint32 row, const char* psz)
    {
        EXPECT_TRUE(DFG_ROOT_NS::isValidIndex(arrExpectedStrsInCol3, nCounter));
        EXPECT_EQ(arrExpectedStrsInCol3[nCounter], psz);
        EXPECT_EQ(arrExpectedRowsInCol3[nCounter], row);
        ++nCounter;
    });
    EXPECT_EQ(1, nCounter);

    std::vector<size_t> cols;
    table.forEachFwdColumnIndex([&](size_t nCol)
    {
        cols.push_back(nCol);
    });
    EXPECT_EQ(2, cols.size());
    EXPECT_EQ(0, cols[0]);
    EXPECT_EQ(3, cols[1]);

    std::string sLongerThanBlockSize(120, '0');
    EXPECT_TRUE(table.addString(sLongerThanBlockSize, 10, 10));
    table.setAllowBlockSizeExceptions(false);
    EXPECT_FALSE(table.addString(sLongerThanBlockSize, 11, 11));
    

    EXPECT_STREQ("cde", table(4, 0));
    EXPECT_STREQ("r1c3", table(1, 3));
    EXPECT_STREQ(nullptr, table(3, 0));
    EXPECT_EQ(nullptr, table(0, 1));
}

TEST(dfgCont, TableSzTypedInterface)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);

    DFG_CLASS_NAME(TableSz)<char, uint32, DFG_MODULE_NS(io)::encodingUTF8> table;
    table.setBlockSize(100);

    StringUtf8 sAlmostBlockSize;
    sAlmostBlockSize.m_s.assign(99, ' ');

    EXPECT_TRUE(table.addString(SzPtrUtf8("a"), 8, 0));
    EXPECT_TRUE(table.addString(StringUtf8(SzPtrUtf8("b")), 0, 0));
    EXPECT_TRUE(table.addString(sAlmostBlockSize, 7, 0));
    EXPECT_TRUE(table.addString(SzPtrUtf8("r1c3_initial"), 1, 3));
    EXPECT_TRUE(table.addString(SzPtrUtf8(""), 1, 0));
    EXPECT_TRUE(table.addString(SzPtrUtf8("cde"), 4, 0));

    EXPECT_STREQ("r1c3_initial", table(1, 3).c_str());
    EXPECT_TRUE(table.setElement(1, 3, SzPtrUtf8("r1c3")));

    const size_t nItemCount = 5;
    const std::array<StringUtf8, nItemCount> arrExpectedStrsInCol0 = { StringUtf8(SzPtrUtf8R("b")), StringUtf8(SzPtrUtf8R("")), StringUtf8(SzPtrUtf8R("cde")), sAlmostBlockSize, StringUtf8(SzPtrUtf8R("a")) };
    const std::array<uint32, nItemCount> arrExpectedRowsInCol0 = { 0, 1, 4, 7, 8 };
    size_t nCounter = 0;
    table.forEachFwdRowInColumn(0, [&](const uint32 row, SzPtrUtf8R tpsz)
    {
        EXPECT_TRUE(DFG_ROOT_NS::isValidIndex(arrExpectedStrsInCol0, nCounter));
        EXPECT_EQ(arrExpectedStrsInCol0[nCounter], tpsz);
        EXPECT_EQ(arrExpectedRowsInCol0[nCounter], row);
        ++nCounter;
    });
    EXPECT_EQ(nItemCount, nCounter);
    table.forEachFwdRowInColumn(1, [&](const uint32, SzPtrUtf8R)
    {
        EXPECT_FALSE(true); // Should not reach here since there are no items in column 1.
    });

    const std::array<StringUtf8, nItemCount> arrExpectedStrsInCol3 = { StringUtf8(SzPtrUtf8R("r1c3")) };
    const std::array<uint32, nItemCount> arrExpectedRowsInCol3 = { 1 };
    nCounter = 0;
    table.forEachFwdRowInColumn(3, [&](const uint32 row, SzPtrUtf8R tpsz)
    {
        EXPECT_TRUE(DFG_ROOT_NS::isValidIndex(arrExpectedStrsInCol3, nCounter));
        EXPECT_EQ(arrExpectedStrsInCol3[nCounter], tpsz);
        EXPECT_EQ(arrExpectedRowsInCol3[nCounter], row);
        ++nCounter;
    });
    EXPECT_EQ(1, nCounter);

    std::vector<size_t> cols;
    table.forEachFwdColumnIndex([&](size_t nCol)
    {
        cols.push_back(nCol);
    });
    EXPECT_EQ(2, cols.size());
    EXPECT_EQ(0, cols[0]);
    EXPECT_EQ(3, cols[1]);

    StringUtf8 sLongerThanBlockSize;
    sLongerThanBlockSize.m_s.assign(120, '0');
    EXPECT_TRUE(table.addString(sLongerThanBlockSize, 10, 10));
    table.setAllowBlockSizeExceptions(false);
    EXPECT_FALSE(table.addString(sLongerThanBlockSize, 11, 11));

    EXPECT_STREQ("cde", table(4, 0).c_str());
    EXPECT_STREQ("r1c3", table(1, 3).c_str());
    EXPECT_EQ(nullptr, table(3, 0).c_str());
    EXPECT_EQ(nullptr, table(0, 1).c_str());
}

TEST(dfgCont, TableSzSorting)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(rand);

    DFG_CLASS_NAME(TableSz)<char> table;
    std::vector<std::vector<std::string>> vecs;

    const size_t nRowCount = 4;
    const size_t nColCount = 3;

    vecs.resize(nColCount);
    auto randEng = createDefaultRandEngineRandomSeeded();
    for (size_t c = 0; c < vecs.size(); ++c)
    {
        for (size_t r = 0; r < nRowCount; ++r)
        {
            const auto val = ::DFG_MODULE_NS(rand)::rand(randEng, 0, NumericTraits<int>::maxValue);
            const auto sVal = DFG_MODULE_NS(str)::toStrC(val);
            vecs[c].push_back(sVal);
            table.setElement(r, c, sVal);
        }
    }

    // Test default text sorting.
    {
        for (uint32 nSortCol = 0; nSortCol < nColCount; ++nSortCol)
        {
            DFG_STATIC_ASSERT(nColCount == 3, "Line below assumed value 3");
            DFG_MODULE_NS(alg)::sortMultiple(vecs[nSortCol], vecs[(nSortCol + 1) % 3], vecs[(nSortCol + 2) % 3]);
            table.sortByColumn(nSortCol);
            for (uint32 c = 0; c < vecs.size(); ++c)
            {
                const auto& curVec = vecs[c];
                for (uint32 r = 0; r < nRowCount; ++r)
                {
                    EXPECT_EQ(curVec[r], table(r, c));
                }
            }
        }
    }

    // Test custom sorting (numeric sorting by str->int conversion)
    {
        for (uint32 nSortCol = 0; nSortCol < nColCount; ++nSortCol)
        {
            const auto pred = [](const char* p0, const char* p1) -> bool
                            {
                                if (p0 == nullptr && p1 != nullptr)
                                    return true;
                                else if (p0 != nullptr && p1 != nullptr)
                                    return DFG_MODULE_NS(str)::strTo<int>(p0) < DFG_MODULE_NS(str)::strTo<int>(p1);
                                else
                                    return false;
                            };
            const auto predStr = [&](const std::string& s0, const std::string& s1)
                            {
                                return pred(s0.c_str(), s1.c_str());
                            };
            DFG_STATIC_ASSERT(nColCount == 3, "Line below assumed value 3");
            DFG_MODULE_NS(alg)::sortMultipleWithPred(predStr, vecs[nSortCol], vecs[(nSortCol + 1) % 3], vecs[(nSortCol + 2) % 3]);
            table.sortByColumn(nSortCol, pred);
            for (uint32 c = 0; c < vecs.size(); ++c)
            {
                const auto& curVec = vecs[c];
                for (uint32 r = 0; r < nRowCount; ++r)
                {
                    EXPECT_EQ(curVec[r], table(r, c));
                }
            }
        }
    }

    // Test trying to sort by invalid column (make sure it won't assert/crash etc.)
    table.sortByColumn(23423);

    // Test empty cell handling
    {
        DFG_CLASS_NAME(TableSz)<char> table2;
        table2.setElement(0, 0, "c");
        table2.setElement(1, 0, "b");
        table2.setElement(2, 0, "a");

        table2.setElement(0, 1, "r0c1");
        table2.setElement(2, 1, "r2c1");

        table2.setElement(3, 2, "r3c2");

        // Sort by column 0
        {
            table2.sortByColumn(0);
            EXPECT_EQ(table2(0, 0), nullptr);
            EXPECT_STREQ(table2(1, 0), "a");
            EXPECT_STREQ(table2(2, 0), "b");
            EXPECT_STREQ(table2(3, 0), "c");

            EXPECT_STREQ(table2(1, 1), "r2c1");
            EXPECT_STREQ(table2(3, 1), "r0c1");

            EXPECT_STREQ(table2(0, 2), "r3c2");
            EXPECT_EQ(table2(3, 2), nullptr);
        }

        // Sort by column 1
        {
            table2.sortByColumn(1);
            EXPECT_EQ(table2(0, 0), nullptr);
            // Sorting is not stable so position of item "b" is not known.
            EXPECT_STREQ(table2(2, 0), "c");
            EXPECT_STREQ(table2(3, 0), "a");

            EXPECT_EQ(table2(0, 1), nullptr);
            EXPECT_EQ(table2(1, 1), nullptr);
            EXPECT_STREQ(table2(2, 1), "r0c1");
            EXPECT_STREQ(table2(3, 1), "r2c1");

            // Sorting is not stable so position of item "r3c2" is not known.
            EXPECT_EQ(table2(2, 2), nullptr);
            EXPECT_EQ(table2(3, 2), nullptr);
        }

        // Sort by column 2
        {
            table2.sortByColumn(2);
            EXPECT_EQ(table2(3, 0), nullptr);
            EXPECT_EQ(table2(3, 1), nullptr);
            EXPECT_STREQ(table2(3, 2), "r3c2");
        }
    }

    // TODO: stable sorting (once implemented)
}

TEST(dfgCont, ArrayWrapper)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    const double arr[] = { 1, 2, 3, 4, 5, 6 };
    std::vector<double> vec(cbegin(arr), cend(arr));
    const std::vector<double> cvec(cbegin(arr), cend(arr));
    std::array<double, DFG_COUNTOF(arr)> stdArr;
    std::copy(std::begin(arr), std::end(arr), stdArr.begin());
    std::string str;
    const std::string strConst;

    // Test existence of default constructor exists (doesn't compile if fails).
    DFG_CLASS_NAME(ArrayWrapperT)<int> wrapEmpty;

    // Test conversion from non-const to const wrapper.
    DFG_CLASS_NAME(ArrayWrapperT)<const int> wrapEmptyConst = wrapEmpty;

    const auto wrapArr = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(arr);
    const auto wrapVec = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(vec);
    const auto wrapcVec = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(cvec);
    const auto wrapStdArr = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(stdArr);
    //const auto wrapStr = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(str); // TODO: this should work, but for now commented out as it doesn't compile.
    const auto wrapStrConst = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(strConst);
    EXPECT_EQ(count(arr), wrapArr.size());
    EXPECT_EQ(count(vec), wrapVec.size());
    EXPECT_EQ(count(cvec), wrapcVec.size());
    EXPECT_EQ(count(stdArr), wrapStdArr.size());
    for (size_t i = 0; i<wrapArr.size(); ++i)
    {
        EXPECT_EQ(&wrapArr[i], &arr[i]);
        EXPECT_EQ(&wrapVec[i], &vec[i]);
        EXPECT_EQ(&wrapcVec[i], &cvec[i]);
        EXPECT_EQ(&wrapStdArr[i], &stdArr[i]);
    }

    EXPECT_EQ(ptrToContiguousMemory(wrapArr), wrapArr.data());
    EXPECT_EQ(ptrToContiguousMemory(wrapVec), wrapVec.data());
    EXPECT_EQ(ptrToContiguousMemory(wrapcVec), wrapcVec.data());
    EXPECT_EQ(ptrToContiguousMemory(wrapStdArr), wrapStdArr.data());

    *wrapVec.begin() = 9;
    EXPECT_EQ(9, vec[0]);
    EXPECT_EQ(9, wrapVec[0]);
    //*wrapVec.cbegin() = 9; // This should fail to compile.
}

namespace
{
    template <class Cont_T>
    void elementTypeTestImpl(const Cont_T& cont)
    {
        DFG_UNUSED(cont);
        using namespace DFG_MODULE_NS(cont);
        // Note: this uses decltype(cont) on purpose.
        DFGTEST_STATIC((std::is_same<typename DFG_CLASS_NAME(ElementType)<decltype(cont)>::type, typename Cont_T::value_type>::value));
    }
}

TEST(dfgCont, elementType)
{
    using namespace DFG_MODULE_NS(cont);
    std::vector<int> vecInt;
    std::array<std::string, 3> arrStrings;
    std::vector<double> arrVectors[3];
    DFGTEST_STATIC((std::is_same<DFG_CLASS_NAME(ElementType)<decltype(vecInt)>::type, int>::value));
    DFGTEST_STATIC((std::is_same<DFG_CLASS_NAME(ElementType)<decltype(arrStrings)>::type, std::string>::value));
    DFGTEST_STATIC((std::is_same<DFG_CLASS_NAME(ElementType)<decltype(arrVectors)>::type, std::vector<double>>::value));
    elementTypeTestImpl(vecInt);
    elementTypeTestImpl(arrStrings);
}

namespace
{
    template <class Cont_T>
    void testValueArrayInterface(Cont_T& cont)
    {
        cont.data();
        EXPECT_TRUE(cont.empty());
        typename Cont_T::value_type arr[] = { 5.0, -3.5, 9.6, 7.8 };
        cont.assign(arr);
        EXPECT_EQ(DFG_COUNTOF(arr), cont.size());
        // TODO: min, max, minmax, average, shiftValues, multiply, divide
        // begin, cbegin, end, cend
    }
}

TEST(dfgCont, ValueArray)
{
    using namespace DFG_MODULE_NS(cont);

    DFG_CLASS_NAME(ValueArray)<double> valArray;
    DFG_CLASS_NAME(ValueVector)<double> valVector;
    DFG_CLASS_NAME(ValueArrayT)<std::valarray<double>> valArrayExplicit;

    testValueArrayInterface(valArray);
    testValueArrayInterface(valVector);
    testValueArrayInterface(valArrayExplicit);

    // TODO: define interface for ValueArray by test cases.
}

#if DFG_LANGFEAT_MUTEX_11

TEST(dfgCont, ViewableSharedPtr)
{
    using namespace DFG_MODULE_NS(cont);

    int resetNotifier0 = 0;
    int resetNotifier1 = 0;
    int resetNotifier2 = 0;

    DFG_CLASS_NAME(ViewableSharedPtr)<const int> sp(std::make_shared<int>(1));
    auto spViewer0 = sp.createViewer();
    auto spViewer1 = sp.createViewer();
    sp.addResetNotifier(DFG_CLASS_NAME(SourceResetNotifierId)(spViewer0.get()), [&](DFG_CLASS_NAME(SourceResetParam)) { ++resetNotifier0; });
    sp.addResetNotifier(DFG_CLASS_NAME(SourceResetNotifierId)(spViewer1.get()), [&](DFG_CLASS_NAME(SourceResetParam)) { ++resetNotifier1; });

    EXPECT_EQ(1, *spViewer0->view());
    EXPECT_EQ(1, *spViewer1->view());
    EXPECT_EQ(0, resetNotifier0);
    EXPECT_EQ(0, resetNotifier1);

    sp.reset(std::make_shared<int>(2));

    EXPECT_EQ(2, *spViewer0->view());
    EXPECT_EQ(2, *spViewer0->view());
    EXPECT_EQ(1, resetNotifier0);
    EXPECT_EQ(1, resetNotifier1);

    // Test automatic handling of short-lived viewer (e.g. automatic removal of reset notifier).
    {
        auto spViewer2 = sp.createViewer();
        EXPECT_EQ(2, *spViewer2->view());
        sp.addResetNotifier(DFG_CLASS_NAME(SourceResetNotifierId)(spViewer2.get()), [&](DFG_CLASS_NAME(SourceResetParam)) { ++resetNotifier2; });
    }
    sp.reset(std::make_shared<int>(3));
    EXPECT_EQ(3, *spViewer0->view());
    EXPECT_EQ(3, *spViewer1->view());
    EXPECT_EQ(2, resetNotifier0);
    EXPECT_EQ(2, resetNotifier1);
    EXPECT_EQ(0, resetNotifier2);

    spViewer0.reset();
    sp.reset(std::make_shared<int>(4));
    EXPECT_EQ(2, resetNotifier0);
    EXPECT_EQ(3, resetNotifier1);
    EXPECT_EQ(4, *spViewer1->view());
    auto spCopy = sp.sharedPtrCopy();
    auto pData = sp.get();
    sp.reset();
    EXPECT_EQ(4, resetNotifier1);
    EXPECT_TRUE(spCopy.unique());
    EXPECT_EQ(nullptr, spViewer1->view().get());
    EXPECT_EQ(4, *spCopy);
    EXPECT_EQ(pData, spCopy.get());
}

#endif // DFG_LANGFEAT_MUTEX_11

namespace
{
    template <class T>
    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TorRef)<T> TorRefHelper(T& item, const bool bRef)
    {
        using namespace DFG_MODULE_NS(cont);
        if (bRef)
            return &item;
        else
            return DFG_CLASS_NAME(TorRef)<T>::makeInternallyOwning(item);
    }

    template <class Cont_T>
    void testArrowOperator()
    {
        Cont_T tor;
        tor->append("a");
        EXPECT_EQ("a", tor.item());
    }
}

TEST(dfgCont, TorRef)
{
    using namespace DFG_MODULE_NS(cont);

    // Basic test with non-const T.
    {
        DFG_CLASS_NAME(TorRef)<int> tor;
        EXPECT_FALSE(tor.hasRef());
        int& ref = tor;
        EXPECT_EQ(0, ref);
    }

    // // Basic test with const T.
    {
        DFG_CLASS_NAME(TorRef)<const int> tor;
        EXPECT_FALSE(tor.hasRef());
        const int& ref = tor;
        EXPECT_EQ(0, ref);
    }

    // Basic test with non-integral type.
    {
        std::vector<double> vec;
        auto rvRef = TorRefHelper(vec, true);
        EXPECT_TRUE(rvRef.hasRef());
        EXPECT_EQ(&vec, &rvRef.item());
        EXPECT_TRUE(rvRef->empty()); // Test operator-> overload.
        rvRef->resize(2); // Test calling non-const.

        const auto rvNonRef = TorRefHelper(vec, false);
        EXPECT_FALSE(rvNonRef.hasRef());
        EXPECT_EQ(rvNonRef.internalStorage().itemPtr(), &rvNonRef.item());
    }

    // Basic test with non-integral const type.
    {
        const std::vector<double> vec;
        const auto rvRef = TorRefHelper(vec, true);
        EXPECT_TRUE(rvRef.hasRef());
        EXPECT_EQ(&vec, &rvRef.item());
        EXPECT_TRUE(rvRef->empty()); // Test operator-> overload.
        //rvRef->resize(2); // This should fail to compile because vec is const.

        const auto rvNonRef = TorRefHelper(vec, false);
        EXPECT_FALSE(rvNonRef.hasRef());
        EXPECT_EQ(rvNonRef.internalStorage().itemPtr(), &rvNonRef.item());
    }

    // Basic test with shared_ptr reference.
    {
        auto sp = std::make_shared<const int>(10);
        DFG_CLASS_NAME(TorRefShared)<const int> rv(sp);
        EXPECT_TRUE(rv.hasRef());
        EXPECT_EQ(10, rv.item());
        EXPECT_EQ(2, sp.use_count());

        // Test that reference is alive even after the original shared_ptr get's reset.
        sp.reset();
        EXPECT_EQ(1, rv.referenceObject().use_count());
        EXPECT_EQ(10, rv.item());
    }

    // // Basic test with stack storage.
    {
        DFG_CLASS_NAME(TorRef)<const int, DFG_DETAIL_NS::TorRefInternalStorageStack<int>> tor;
        EXPECT_FALSE(tor.hasRef());
        const int& ref = tor;
        EXPECT_EQ(0, ref);
    }

    // Test operator -> 
    {
        testArrowOperator<DFG_CLASS_NAME(TorRef)<std::string>>();
        testArrowOperator<DFG_CLASS_NAME(TorRefShared)<std::string>>();
        testArrowOperator<DFG_CLASS_NAME(TorRef)<std::string, DFG_DETAIL_NS::TorRefInternalStorageStack<std::string>>>();
    }
}

#include <dfg/str.hpp>
#include <dfg/os.hpp>
#include <dfg/str/stringLiteralCharToValue.hpp>

TEST(dfgCont, TableCsv)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(io);
    using namespace DFG_MODULE_NS(utf);
    
    std::vector<std::string> paths;
    std::vector<TextEncoding> encodings;
    std::vector<char> separators;
    // TODO: Read these properties from file names once "for-each-file-in-folder"-function is available.
    const TextEncoding contEncodings[] = { encodingUTF8,
                                            encodingUTF16Be,
                                            encodingUTF16Le,
                                            encodingUTF32Be,
                                            encodingUTF32Le };
    const std::string contSeparators[] = { "2C", "09", "3B" }; // ',' '\t', ';'
    
    const std::string sPathTemplate = "testfiles/csv_testfiles/csvtest%1%_BOM_sep_%2%_eol_n.csv";
    for (size_t iE = 0; iE < count(contEncodings); ++iE)
    {
        for (size_t iS = 0; iS < count(contSeparators); ++iS)
        {
            auto s = DFG_MODULE_NS(str)::replaceSubStrs(sPathTemplate, "%1%", encodingToStrId(contEncodings[iE]));
            DFG_MODULE_NS(str)::replaceSubStrsInplace(s, "%2%", contSeparators[iS]);
            EXPECT_TRUE(DFG_MODULE_NS(os)::isPathFileAvailable(s.c_str(), DFG_MODULE_NS(os)::FileModeRead));
            paths.push_back(std::move(s));
            encodings.push_back(contEncodings[iE]);
            const auto charValOpt = DFG_MODULE_NS(str)::stringLiteralCharToValue<char>(std::string("\\x") + contSeparators[iS]);
            EXPECT_TRUE(charValOpt.first);
            separators.push_back(charValOpt.second);
            
        }
    }
    typedef DFG_CLASS_NAME(TableCsv)<char, size_t> Table;
    std::vector<std::unique_ptr<Table>> tables;
    tables.resize(paths.size());

    for (size_t i = 0; i < paths.size(); ++i)
    {
        const auto& s = paths[i];
        tables[i].reset(new Table);
        auto& table = *tables[i];
        // Read from file...
        table.readFromFile(s);
        // ...check that read properties separator, separator char and encoding, matches
        {
            EXPECT_EQ(encodings[i], table.m_readFormat.textEncoding()); // TODO: use access function for format info.
            EXPECT_EQ(separators[i], table.m_readFormat.separatorChar()); // TODO: use access function for format info.
        }

        // ...write to memory using the same encoding as in file...
        std::string bytes;
        DFG_CLASS_NAME(OmcByteStream)<std::string> ostrm(&bytes);
        table.writeToStream(ostrm);
        // ...read file bytes...
        const auto fileBytes = fileToByteContainer<std::string>(s);
        // ...and check that bytes match.
        EXPECT_EQ(fileBytes, bytes);
    }

    // Test that results are the same as with DelimitedTextReader.
    {
        std::wstring sFromFile;
        DFG_CLASS_NAME(IfStreamWithEncoding) istrm(paths.front());
        DFG_CLASS_NAME(DelimitedTextReader)::read(istrm, wchar_t(','), wchar_t('"'), wchar_t('\n'), [&](const size_t nRow, const size_t nCol, const wchar_t* const p, const size_t nSize)
        {
            std::wstring sUtfConverted;
            auto inputRange = DFG_ROOT_NS::makeSzRange((*tables.front())(nRow, nCol).c_str());
            DFG_MODULE_NS(utf)::utf8To16Native(inputRange, std::back_inserter(sUtfConverted));
            EXPECT_EQ(std::wstring(p, nSize), sUtfConverted);
        });
    }

    for (size_t i = 1; i < tables.size(); ++i)
    {
        EXPECT_TRUE(tables[0]->isContentAndSizesIdenticalWith(*tables[i]));
    }
    
}

TEST(dfgCont, SortedSequence)
{
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(alg);
    using namespace DFG_MODULE_NS(rand);

    auto randEng = createDefaultRandEngineRandomSeeded();
    auto distrEng = makeDistributionEngineUniform(&randEng, -1000, 1000);

    {
        const size_t nCount = 100;

        DFG_CLASS_NAME(SortedSequence)<std::vector<int>> sseq;
        DFG_CLASS_NAME(SortedSequence)<std::deque<int>> sseqDeque;
        DFG_CLASS_NAME(SortedSequence)<std::list<int>> sseqList;

        EXPECT_TRUE(sseq.empty());
        EXPECT_TRUE(sseqDeque.empty());
        EXPECT_TRUE(sseqList.empty());

        sseq.reserve(nCount);

        for (size_t i = 0; i < nCount; ++i)
        {
            auto val = distrEng();
            sseq.insert(val);
            sseqDeque.insert(val);
            sseqList.insert(val);
            EXPECT_TRUE(std::is_sorted(sseq.begin(), sseq.end()));
            EXPECT_TRUE(std::equal(sseq.begin(), sseq.end(), sseqDeque.begin()));
            EXPECT_TRUE(std::equal(sseq.begin(), sseq.end(), sseqList.begin()));

            ASSERT_TRUE(sseq.find(val) != sseq.end());
            EXPECT_EQ(val, *sseq.find(val));

            ASSERT_TRUE(sseqDeque.find(val) != sseqDeque.end());
            EXPECT_EQ(val, *sseqDeque.find(val));

            ASSERT_TRUE(sseqList.find(val) != sseqList.end());
            EXPECT_EQ(val, *sseqList.find(val));
        }
    }
}

TEST(dfgCont, InterleavedXsortedTwoChannelWrapper)
{
    using namespace DFG_MODULE_NS(cont);
    const std::array<double, 6> arr = { 1, 3, 5, 10, 15, 60 };
    DFG_CLASS_NAME(InterleavedXsortedTwoChannelWrapper)<const double> wrap(arr);
    EXPECT_EQ(3, wrap.getFirstY());
    EXPECT_EQ(60, wrap.getLastY());
    EXPECT_EQ(15, wrap.getXMax());
    EXPECT_EQ(1, wrap.getXMin());
    EXPECT_EQ(3, wrap.getY(0));
    EXPECT_EQ(10, wrap.getY(1));
    EXPECT_EQ(60, wrap.getY(2));

    EXPECT_EQ(3, wrap(0));
    EXPECT_EQ(3, wrap(1));
    EXPECT_EQ(6.5, wrap(3));
    EXPECT_EQ(10, wrap(5));
    EXPECT_EQ(35, wrap(10));
    EXPECT_EQ(60, wrap(15));
    EXPECT_EQ(60, wrap(150));
}

namespace
{
    template  <class Map_T>
    std::map<std::string, int> createStdMap(const Map_T& m)
    {
        std::map<std::string, int> rv;
        for (auto iter = m.begin(), iterEnd = m.end(); iter != iterEnd; ++iter)
            rv[iter->first] = iter->second;
        return rv;
    }

    template <class Map_T>
    void testMapInterface(Map_T& m, const unsigned long nRandEngSeed)
    {
        using namespace DFG_ROOT_NS;

        const Map_T& mConst = m;
        
        EXPECT_EQ(0, m.size());
        EXPECT_EQ(0, mConst.size());
        EXPECT_EQ(true, m.empty());
        EXPECT_EQ(true, mConst.empty());

        // operator[]
        m["a"] = 1;
        EXPECT_EQ(1, m.size());
        EXPECT_EQ(1, mConst.size());
        m["b"] = 2;
        EXPECT_EQ(2, m.size());
        EXPECT_EQ(2, mConst.size());
        m["a"] = 3; // Replacing.
        EXPECT_EQ(2, m.size());
        EXPECT_EQ(2, mConst.size());
        m[std::string("c")] = 'c';
        EXPECT_EQ(3, m.size());
        EXPECT_EQ(3, mConst.size());

        const std::string sConstKey = "f";
        m[sConstKey] = 'f';
        EXPECT_EQ(4, m.size());
        EXPECT_EQ(4, mConst.size());

        // find
        m.find("a");
        mConst.find("a");

        // insert
        {
            auto i1 = m.insert(std::pair<std::string, int>("d", 'd'));
            EXPECT_EQ(true, i1.second);
            EXPECT_EQ("d", i1.first->first);
            EXPECT_EQ('d', i1.first->second);

            auto i2 = m.insert(std::pair<std::string, int>("d", 'e'));
            EXPECT_EQ(false, i2.second);
            EXPECT_EQ("d", i2.first->first);
            EXPECT_EQ('d', i2.first->second);

            auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
            randEng.seed(nRandEngSeed);
            for (size_t i = 0; i < 50; ++i)
            {
                std::string s(1, DFG_MODULE_NS(rand)::rand<int8>(randEng, 0, 127));
                m.insert(std::pair<std::string, int>(s, DFG_MODULE_NS(rand)::rand(randEng, 1000, 2000)));
            }
        }

        // move constructor and move assignment
        {
            ASSERT_FALSE(m.empty());
            Map_T mCopy = m;
            Map_T mCopy2 = m;
            Map_T mCopy3 = m;
            Map_T mMoveCtor(std::move(mCopy));
            Map_T mMoveAssign;
            mMoveAssign = std::move(mCopy2);
            EXPECT_TRUE(mCopy.empty());
            EXPECT_TRUE(mCopy2.empty());
            ASSERT_EQ(m.size(), mCopy3.size());
            ASSERT_EQ(m.size(), mMoveCtor.size());
            ASSERT_EQ(m.size(), mMoveAssign.size());
            EXPECT_TRUE(createStdMap(m) == createStdMap(mCopy3));
            EXPECT_TRUE(createStdMap(m) == createStdMap(mMoveCtor));
            EXPECT_TRUE(createStdMap(m) == createStdMap(mMoveAssign));
        }
    }

    template <class Map_T>
    void verifyEqual(const std::map<std::string, int>& mStd, const Map_T& m)
    {
        EXPECT_TRUE(mStd == createStdMap(m));
    }

    template <class Map_T>
    void eraseTester(Map_T& m)
    {
        const auto rv0 = m.erase(m.begin());
        EXPECT_EQ(m.begin(), rv0);
        EXPECT_EQ(1, m.erase(m.frontKey()));
        EXPECT_EQ(0, m.erase("invalid_key"));

        // Test unsorted removal
        if (!m.isSorted())
        {
            auto mEqualityTester = createStdMap(m);
            mEqualityTester.erase(m.backKey());
            m.erase(m.backIter());
            verifyEqual(mEqualityTester, m);
            
            mEqualityTester.erase(m.frontKey());
            m.erase(m.begin());
            verifyEqual(mEqualityTester, m);

            EXPECT_TRUE(m.size() > 30); // Just a arbitrary test to make sure that m has big enough size for these tests.
            const auto nRemoveCount = 3 * m.size() / 4;
            const auto iterFirst = m.begin() + 3;
            const auto iterEnd = iterFirst + nRemoveCount;
            for (auto i = iterFirst; i != iterEnd; ++i)
                mEqualityTester.erase(i->first);
            m.erase(iterFirst, iterEnd);
            verifyEqual(mEqualityTester, m);
        }

        // Erase all
        const auto rv1 = m.erase(m.begin(), m.end());
        EXPECT_EQ(0, m.size());
        EXPECT_TRUE(m.empty());
        EXPECT_EQ(m.end(), rv1);

        m["a"] = 1;
        m.clear();
        EXPECT_TRUE(m.empty());
    }

    template <class Map_T>
    void verifyMapVectors(Map_T& mSorted, Map_T& mUnsorted, const std::map<std::string, int>& mapExpected)
    {
        DFGTEST_STATIC((std::is_same<std::string, typename Map_T::key_type>::value));
        DFGTEST_STATIC((std::is_same<int, typename Map_T::mapped_type>::value));

        ASSERT_FALSE(mapExpected.empty());

        EXPECT_TRUE(mSorted.hasKey(mapExpected.begin()->first));
        EXPECT_TRUE(mUnsorted.hasKey(mapExpected.begin()->first));

        EXPECT_FALSE(mSorted.hasKey("invalid_key"));
        EXPECT_FALSE(mUnsorted.hasKey("invalid_key"));

        EXPECT_EQ(mSorted.backIter()->first, mSorted.backKey());
        EXPECT_EQ(mUnsorted.backIter()->first, mUnsorted.backKey());

        EXPECT_EQ(mSorted.backIter()->second, mSorted.backValue());
        EXPECT_EQ(mUnsorted.backIter()->second, mUnsorted.backValue());

        EXPECT_EQ(mSorted.begin()->first, mSorted.frontKey());
        EXPECT_EQ(mUnsorted.begin()->first, mUnsorted.frontKey());

        EXPECT_EQ(mSorted.begin()->second, mSorted.frontValue());
        EXPECT_EQ(mUnsorted.begin()->second, mUnsorted.frontValue());

        {
            const auto mapFromSorted = createStdMap(mSorted);
            EXPECT_EQ(mapExpected, mapFromSorted);
        }

        {
            const auto mapFromUnsorted = createStdMap(mUnsorted);
            EXPECT_EQ(mapExpected, mapFromUnsorted);
        }

        // Test insert(key, value) -interface.
        mSorted.insert(std::string("insert_two_param"), 123);
        mUnsorted.insert(std::string("insert_two_param"), 123);

        // Test that setSorting(true) sorts.
        {
            auto mSortedFromUnsorted = mUnsorted;
            mSortedFromUnsorted.setSorting(true);

            //EXPECT_TRUE(std::equal(mSorted.begin(), mSorted.end(), mUnsorted.begin())); // Note: this does not work for SoA-iterator due to missing value_type typedef.
            auto iter2 = mSortedFromUnsorted.begin();
            for (auto iter = mSorted.begin(), iterEnd = mSorted.end(); iter != iterEnd; ++iter, ++iter2)
            {
                EXPECT_EQ(iter->first, iter2->first);
                EXPECT_EQ(iter->second, iter2->second);
            }
        }

        eraseTester(mSorted);
        eraseTester(mUnsorted);
    }

    template <class Map_T>
    void testMapKeyValueMoves()
    {
        DFG_STATIC_ASSERT((std::is_same<typename Map_T::key_type, std::string>::value), "key_type should be std::string");
        DFG_STATIC_ASSERT((std::is_same<typename Map_T::mapped_type, std::string>::value), "key_type should be std::string");
        Map_T mm;
        std::string s(30, 'a');
        std::string s2(30, 'b');
        std::string s3(30, 'c');
        std::string s4(30, 'd');
        mm[std::move(s)] = std::move(s2);
        mm.insert(std::move(s3), std::move(s4));
        EXPECT_TRUE(s.empty());
        EXPECT_TRUE(s2.empty());
        EXPECT_TRUE(s3.empty());
        EXPECT_TRUE(s4.empty());
        EXPECT_EQ(2, mm.size());
    }

} // unnamed namespace

TEST(dfgCont, MapVector)
{
    using namespace DFG_MODULE_NS(cont);

    std::map<std::string, int> mStd;
    DFG_CLASS_NAME(MapVectorSoA)<std::string, int> mapVectorSoaSorted;
    DFG_CLASS_NAME(MapVectorSoA)<std::string, int> mapVectorSoaUnsorted;
    mapVectorSoaUnsorted.setSorting(false);
    DFG_CLASS_NAME(MapVectorAoS)<std::string, int> mapVectorAosSorted;
    DFG_CLASS_NAME(MapVectorAoS)<std::string, int> mapVectorAosUnsorted;
    mapVectorAosUnsorted.setSorting(false);

    const int randEngSeed = 12345678;
    testMapInterface(mStd, randEngSeed);
    mapVectorSoaSorted.reserve(10);
    testMapInterface(mapVectorSoaSorted, randEngSeed);
    testMapInterface(mapVectorSoaUnsorted, randEngSeed);
    EXPECT_EQ(mStd.size(), mapVectorSoaSorted.size());
    EXPECT_EQ(mStd.size(), mapVectorSoaUnsorted.size());

    testMapInterface(mapVectorAosSorted, randEngSeed);
    testMapInterface(mapVectorAosUnsorted, randEngSeed);
    EXPECT_EQ(mStd.size(), mapVectorAosSorted.size());
    EXPECT_EQ(mStd.size(), mapVectorAosUnsorted.size());

    // Test some basic []-operations.
    {
        DFG_CLASS_NAME(MapVectorAoS)<size_t, double> mm;
        size_t i = 0;
        mm[i] = 2;
        mm[size_t(3)] = 4;
    }

    testMapKeyValueMoves<DFG_CLASS_NAME(MapVectorSoA)<std::string, std::string>>();
    testMapKeyValueMoves<DFG_CLASS_NAME(MapVectorAoS)<std::string, std::string>>();

    verifyMapVectors(mapVectorSoaSorted, mapVectorSoaUnsorted, mStd);
    verifyMapVectors(mapVectorAosSorted, mapVectorAosUnsorted, mStd);
}

namespace
{
    template <class Set_T>
    void testSetInterface(Set_T& se, const unsigned long nRandEngSeed)
    {
        using namespace DFG_ROOT_NS;

        const Set_T& seConst = se;

        EXPECT_EQ(0, se.size());
        EXPECT_EQ(0, seConst.size());
        EXPECT_EQ(true, se.empty());
        EXPECT_EQ(true, seConst.empty());

        // insert
        se.insert("a");
        EXPECT_EQ(1, se.size());
        EXPECT_EQ(1, seConst.size());
        se.insert("b");
        EXPECT_EQ(2, se.size());
        EXPECT_EQ(2, seConst.size());
        se.insert("a");
        EXPECT_EQ(2, se.size());
        EXPECT_EQ(2, seConst.size());
        se.insert("c");
        EXPECT_EQ(3, se.size());
        EXPECT_EQ(3, seConst.size());

        // find
        se.find("a");
        seConst.find("a");

        auto i1 = se.insert("d");
        EXPECT_EQ(true, i1.second);
        EXPECT_EQ("d", *i1.first);

        auto i2 = se.insert("d");
        EXPECT_EQ(false, i2.second);
        EXPECT_EQ("d", *i2.first);

        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        randEng.seed(nRandEngSeed);
        for (size_t i = 0; i < 50; ++i)
        {
            std::string s(1, DFG_MODULE_NS(rand)::rand<int8>(randEng, 0, 127));
            se.insert(s);
        }

        Set_T seCopy = se;
        Set_T seCopy2 = se;
        Set_T seMoved = std::move(seCopy);
        Set_T seAssign;
        Set_T seAssign2;
        seAssign = std::move(seCopy2);
        seAssign2 = se;
        EXPECT_TRUE(seCopy.empty());
        EXPECT_TRUE(seCopy2.empty());
        EXPECT_TRUE(se == se);
        EXPECT_FALSE(se == seCopy);
        EXPECT_TRUE(se != seCopy);
        EXPECT_EQ(se, seMoved);
        EXPECT_EQ(se, seAssign);
        EXPECT_EQ(se, seAssign2);
    }

    template  <class Set_T>
    std::set<std::string> createStdSet(const Set_T& se)
    {
        std::set<std::string> rv;
        for (auto iter = se.begin(), iterEnd = se.end(); iter != iterEnd; ++iter)
            rv.insert(*iter);
        return rv;
    }

    template <class Set_T>
    void verifyEqual(const std::set<std::string>& seStd, const Set_T& se)
    {
        EXPECT_TRUE(seStd == createStdSet(se));
    }

    template <class Set_T>
    void eraseTesterForSet(Set_T& se)
    {
        const auto rv0 = se.erase(se.begin());
        EXPECT_EQ(se.begin(), rv0);
        EXPECT_EQ(1, se.erase(se.front()));
        EXPECT_EQ(0, se.erase("invalid_key"));

        // Test unsorted removal
        if (!se.isSorted())
        {
            auto seEqualityTester = createStdSet(se);
            seEqualityTester.erase(se.back());
            se.erase(se.backIter());
            verifyEqual(seEqualityTester, se);

            seEqualityTester.erase(se.front());
            se.erase(se.begin());
            verifyEqual(seEqualityTester, se);

            EXPECT_TRUE(se.size() > 30); // Just a arbitrary test to make sure that m has big enough size for these tests.
            const auto nRemoveCount = 3 * se.size() / 4;
            const auto iterFirst = se.begin() + 3;
            const auto iterEnd = iterFirst + nRemoveCount;
            for (auto i = iterFirst; i != iterEnd; ++i)
                seEqualityTester.erase(*i);
            se.erase(iterFirst, iterEnd);
            verifyEqual(seEqualityTester, se);
        }

        // Erase all
        const auto rv1 = se.erase(se.begin(), se.end());
        EXPECT_EQ(0, se.size());
        EXPECT_TRUE(se.empty());
        EXPECT_EQ(se.end(), rv1);

        se.insert("a");
        se.clear();
        EXPECT_TRUE(se.empty());
    }

    template <class Set_T>
    void verifySetVectors(Set_T& seSorted, Set_T& seUnsorted, const std::set<std::string>& setExpected)
    {
        DFGTEST_STATIC((std::is_same<std::string, typename Set_T::key_type>::value));

        ASSERT_FALSE(setExpected.empty());

        EXPECT_TRUE(seSorted.hasKey(*setExpected.begin()));
        EXPECT_TRUE(seUnsorted.hasKey(*setExpected.begin()));

        EXPECT_FALSE(seSorted.hasKey("invalid_key"));
        EXPECT_FALSE(seUnsorted.hasKey("invalid_key"));

        EXPECT_EQ(*seSorted.backIter(), seSorted.back());
        EXPECT_EQ(*seUnsorted.backIter(), seUnsorted.back());

        EXPECT_EQ(*seSorted.begin(), seSorted.front());
        EXPECT_EQ(*seUnsorted.begin(), seUnsorted.front());

        verifyEqual(setExpected, seSorted);
        verifyEqual(setExpected, seUnsorted);

        // Test that setSorting(true) sorts.
        {
            auto seSortedFromUnsorted = seUnsorted;
            seSortedFromUnsorted.setSorting(true);

            EXPECT_TRUE(std::equal(seSorted.begin(), seSorted.end(), seSortedFromUnsorted.begin())); // Note: this does not work for SoA-iterator due to missing value_type typedef.
        }

        eraseTesterForSet(seSorted);
        eraseTesterForSet(seUnsorted);
    }

} // unnamed namespace

TEST(dfgCont, SetVector)
{
    using namespace DFG_MODULE_NS(cont);

    std::set<std::string> mStd;
    DFG_CLASS_NAME(SetVector)<std::string> setVectorSorted;
    DFG_CLASS_NAME(SetVector)<std::string> setVectorUnsorted; setVectorUnsorted.setSorting(false);
    
    const int randEngSeed = 12345678;
    testSetInterface(mStd, randEngSeed);
    testSetInterface(setVectorSorted, randEngSeed);
    testSetInterface(setVectorUnsorted, randEngSeed);

    verifyEqual(mStd, setVectorSorted);
    verifyEqual(mStd, setVectorUnsorted);

    verifySetVectors(setVectorSorted, setVectorUnsorted, mStd);

    // Test that SetVector works with std::inserter.
    {
        DFG_CLASS_NAME(SetVector)<std::string> setVectorSorted2;
        DFG_CLASS_NAME(SetVector)<std::string> setVectorUnsorted2; setVectorUnsorted2.setSorting(false);
        auto insertIter = std::inserter(setVectorSorted2, setVectorSorted2.end());
        *insertIter = "bcd";
        *insertIter = "abc";
        *insertIter = "abc";
        auto insertIter2 = std::inserter(setVectorUnsorted2, setVectorUnsorted2.end());
        *insertIter2 = "bcd";
        *insertIter2 = "abc";
        *insertIter2 = "abc";
        EXPECT_EQ(2, setVectorSorted2.size());
        EXPECT_EQ(setVectorSorted2.size(), setVectorUnsorted2.size());
        EXPECT_EQ(setVectorSorted2, setVectorUnsorted2);
    }

    // Test operator==
    {
        DFG_CLASS_NAME(SetVector)<std::string> setVectorSorted3;
        DFG_CLASS_NAME(SetVector)<std::string> setVectorUnsorted3; setVectorUnsorted3.setSorting(false);
        DFG_CLASS_NAME(SetVector)<std::string> setVectorUnsorted4; setVectorUnsorted4.setSorting(false);
        setVectorSorted3.insert("b");
        setVectorSorted3.insert("a");
        setVectorUnsorted3.insert("b");
        setVectorUnsorted3.insert("a");
        setVectorUnsorted4.insert("a");
        setVectorUnsorted4.insert("b");
        EXPECT_EQ(setVectorSorted3, setVectorUnsorted3); // Since sets have different sorting, their internal data storage is different
        EXPECT_EQ(setVectorUnsorted3, setVectorUnsorted4); // Both are unsorted but elements are stored in different order.
        setVectorSorted3.insert("d");
        setVectorUnsorted3.insert("e");
        setVectorUnsorted4.insert("c");
        EXPECT_NE(setVectorSorted3, setVectorUnsorted4);
        EXPECT_NE(setVectorUnsorted3, setVectorUnsorted4);
    }

    // Test some basic []-operations.
    {
        DFG_CLASS_NAME(SetVector)<int> si;
        si.insert(1);
        int i = 2;
        si.insert(i);
        const int ci = 3;
        si.insert(ci);
        EXPECT_EQ(3, si.size());
    }

    // Test moves
    {
        DFG_CLASS_NAME(SetVector)<std::string> se;
        std::string s(30, 'a');
        se.insert(std::move(s));
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(1, se.size());
    }
}

namespace
{
    template <class T0, class T1, bool IsTrivial>
    void testTrivialPair()
    {
        using namespace DFG_MODULE_NS(cont);
        DFG_CLASS_NAME(TrivialPair)<T0, T1> tp;

        {
            typedef DFG_MODULE_NS(TypeTraits)::IsTriviallyCopyable<decltype(tp)> IsTriviallyCopyableType;
#if DFG_MSVC_VER != DFG_MSVC_VER_2012
            // Test that either IsTriviallyCopyableType is not available or that it returns expected result.
            DFGTEST_STATIC((std::is_base_of<DFG_MODULE_NS(TypeTraits)::UnknownAnswerType, IsTriviallyCopyableType>::value ||
                std::is_base_of<std::integral_constant<bool, IsTrivial>, IsTriviallyCopyableType>::value));
#else // DFG_MSVC_VER == DFG_MSVC_VER_2012
            DFGTEST_STATIC((std::is_base_of<std::is_trivially_copyable<decltype(tp)>, IsTriviallyCopyableType>::value));
#endif
        }

        // Test existence of two parameter constructor.
        {
            auto t0 = T0();
            auto t1 = T1();
            DFG_CLASS_NAME(TrivialPair)<T0, T1> tp2(t0, t1);
            DFG_CLASS_NAME(TrivialPair)<T0, T1> tp22(tp2.first, tp2.second);
            EXPECT_EQ(tp2, tp22);
        }

        // Test move constructor
        {
            std::vector<T0> vec0(2, T0());
            std::vector<T1> vec1(3, T1());
            DFG_CLASS_NAME(TrivialPair)<std::vector<T0>, std::vector<T1>> tp3(std::move(vec0), std::move(vec1));
            EXPECT_TRUE(vec0.empty());
            EXPECT_TRUE(vec1.empty());
            EXPECT_EQ(2, tp3.first.size());
            EXPECT_EQ(3, tp3.second.size());
        }
    }
}

TEST(dfgCont, TrivialPair)
{
    using namespace DFG_MODULE_NS(cont);
    testTrivialPair<int, int, true>();
    testTrivialPair<char, double, true>();
    testTrivialPair<DFG_CLASS_NAME(TrivialPair)<int, int>, bool, true>();
    testTrivialPair<std::string, bool, false>();

    // Test move assignment of items
    {
        DFG_CLASS_NAME(TrivialPair)<std::string, std::string> tp;
        DFG_CLASS_NAME(TrivialPair)<std::string, std::string> tp2("aa", "bb");
        tp = std::move(tp2);
        EXPECT_TRUE(tp2.first.empty());
        EXPECT_TRUE(tp2.second.empty());
        EXPECT_EQ("aa", tp.first);
        EXPECT_EQ("bb", tp.second);
    }
}

TEST(dfgCont, Vector)
{
    using namespace DFG_MODULE_NS(cont);

    DFG_CLASS_NAME(Vector)<int> v;
    v.push_back(1);
    auto v2(v);
    auto v3(std::move(v));
    decltype(v) v4;
    v4 = std::move(v2);
    EXPECT_TRUE(v.empty());
    EXPECT_TRUE(v2.empty());
    EXPECT_EQ(v3, v4);
}

namespace
{
    template <class Cont_T>
    void contAlgImpl()
    {
        Cont_T v;
        v.push_back(1);
        v.push_back(2);
        DFG_MODULE_NS(cont)::popFront(v);
        EXPECT_EQ(Cont_T(1, 2), v);
        v.push_back(3);
        v.push_back(4);
        auto iterTail = v.end();
        std::advance(iterTail, -1);
        DFG_MODULE_NS(cont)::cutTail(v, iterTail);
        DFG_MODULE_NS(cont)::popFront(v);
        EXPECT_EQ(Cont_T(1, 3), v);
    }

    template <class StringView_T, class Ptr_T>
    void StringViewCutTailTests(Ptr_T psz)
    {
        StringView_T sv(psz);
        ASSERT_EQ(3, sv.length());
        DFG_MODULE_NS(cont)::cutTail(sv, sv.end() - 2);
        EXPECT_EQ(1, sv.length());
        typedef decltype(sv[0]) CodePointType;
        EXPECT_EQ(CodePointType('a'), sv[0]);
    }
}

TEST(dfgCont, contAlg)
{
    DFGTEST_STATIC(DFG_MODULE_NS(cont)::DFG_DETAIL_NS::cont_contAlg_hpp::Has_pop_front<int>::value == false);
    DFGTEST_STATIC(DFG_MODULE_NS(cont)::DFG_DETAIL_NS::cont_contAlg_hpp::Has_pop_front<std::deque<int>>::value == true);
    DFGTEST_STATIC(DFG_MODULE_NS(cont)::DFG_DETAIL_NS::cont_contAlg_hpp::Has_pop_front<std::list<int>>::value == true);

    DFGTEST_STATIC(DFG_MODULE_NS(cont)::DFG_DETAIL_NS::cont_contAlg_hpp::Has_cutTail<std::vector<int>>::value == false);
    DFGTEST_STATIC(DFG_MODULE_NS(cont)::DFG_DETAIL_NS::cont_contAlg_hpp::Has_cutTail<DFG_ROOT_NS::DFG_CLASS_NAME(StringViewC)>::value == true);

    contAlgImpl<std::deque<int>>();
    contAlgImpl<std::list<int>>();
    contAlgImpl<std::string>();
    contAlgImpl<std::vector<int>>();

    // Test cutTail for StringViewC
    {
        StringViewCutTailTests<DFG_ROOT_NS::DFG_CLASS_NAME(StringViewC)>("abc");
        StringViewCutTailTests<DFG_ROOT_NS::DFG_CLASS_NAME(StringViewAscii)>(DFG_ROOT_NS::SzPtrAscii("abc"));
        StringViewCutTailTests<DFG_ROOT_NS::DFG_CLASS_NAME(StringViewLatin1)>(DFG_ROOT_NS::SzPtrLatin1("abc"));
    }
}

TEST(dfgCont, VectorSso)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);

    const size_t nTotalSize = 10;

    DFG_CLASS_NAME(VectorSso)<size_t, nTotalSize-2> v;

    DFGTEST_STATIC(v.s_ssoBufferSize == nTotalSize - 2);

    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    v.push_back(5);
    v.cutTail(v.begin() + 3);
    ASSERT_EQ(3, v.size());
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_TRUE(v.isSsoStorageInUse());
    v.clear();
    EXPECT_TRUE(v.isSsoStorageInUse());

    std::vector<size_t> vVector;
    EXPECT_TRUE(v.empty());
    for (size_t i = 0; i < nTotalSize; ++i)
    {
        v.push_back(i);
        vVector.push_back(i);
    }
    EXPECT_FALSE(v.isSsoStorageInUse());
    EXPECT_EQ(nTotalSize, v.size());
    EXPECT_FALSE(v.empty());
    for (size_t i = 0; i < v.size(); ++i)
        EXPECT_EQ(i, v[i]);

    EXPECT_EQ(0, v.front());
    EXPECT_EQ(nTotalSize - 1 , v.back());

    ASSERT_EQ(v.size(), vVector.size());

    EXPECT_TRUE((std::equal(v.begin(), v.end(), vVector.begin())));

    v.pop_front();
    EXPECT_EQ(nTotalSize - 1, v.size());
    EXPECT_EQ(1, v.front());

    v.pop_back();
    EXPECT_EQ(nTotalSize - 2, v.size());
    EXPECT_EQ(nTotalSize - 2, v.back());

    v.cutTail(v.begin() + 3);
    EXPECT_EQ(3, v.size());
    EXPECT_EQ(1, v.front());
    EXPECT_EQ(3, v.back());

    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_TRUE(v.isSsoStorageInUse());
}
