#include <stdafx.h>
#include <dfg/cont.hpp>
#include <dfg/cont/table.hpp>
#include <dfg/cont/arrayWrapper.hpp>
#include <string>
#include <dfg/ptrToContiguousMemory.hpp>
#include <dfg/dfgBase.hpp>
#include <dfg/cont/valueArray.hpp>

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

    /*
    auto v3 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(8, 6, 9);
    EXPECT_EQ(v3.size() == 3 && v3[0] == 8 && v3[1] == 6 && v3[2] == 9, true);

    auto v4 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(5, 6, 7, 8);
    EXPECT_EQ(v4.size() == 4 && v4[0] == 5 && v4[1] == 6 && v4[2] == 7 && v4[3] == 8, true);

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

TEST(dfgCont, TableSz)
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

    std::string sTooLong(120, ' ');
    EXPECT_FALSE(table.addString(sTooLong, 10, 10));

    EXPECT_STREQ("cde", table(4, 0));
    EXPECT_STREQ("r1c3", table(1, 3));
    EXPECT_STREQ(nullptr, table(3, 0));
    EXPECT_EQ(nullptr, table(0, 1));
}

TEST(dfgCont, ArrayWrapper)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    const double arr[] = { 1, 2, 3, 4, 5, 6 };
    std::vector<double> vec(cbegin(arr), cend(arr));
    const auto wrapArr = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(arr);
    const auto wrapVec = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(vec);
    EXPECT_EQ(count(arr), wrapArr.size());
    EXPECT_EQ(count(vec), wrapVec.size());
    for (size_t i = 0; i<wrapArr.size(); ++i)
    {
        EXPECT_EQ(&wrapArr[i], &arr[i]);
        EXPECT_EQ(&wrapVec[i], &vec[i]);
    }

    EXPECT_EQ(ptrToContiguousMemory(wrapArr), wrapArr.data());
    EXPECT_EQ(ptrToContiguousMemory(wrapVec), wrapVec.data());
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
