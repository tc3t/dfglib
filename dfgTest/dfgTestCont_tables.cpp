#include "stdafx.h"
#include <dfg/cont.hpp>
#include <dfg/cont/table.hpp>
#include <dfg/cont/arrayWrapper.hpp>
#include <string>
#include <deque>
#include <list>
#include <memory>
#include <strstream>
#include <dfg/ptrToContiguousMemory.hpp>
#include <dfg/dfgBase.hpp>
#include <dfg/ReadOnlySzParam.hpp>
#include <dfg/cont/CsvConfig.hpp>
#include <dfg/cont/MapToStringViews.hpp>
#include <dfg/cont/MapVector.hpp>
#include <dfg/cont/ViewableSharedPtr.hpp>
#include <dfg/cont/tableCsv.hpp>
#include <dfg/cont/TrivialPair.hpp>
#include <dfg/rand.hpp>
#include <dfg/typeTraits.hpp>
#include <dfg/io/BasicOmcByteStream.hpp>
#include <dfg/io/OmcByteStream.hpp>
#include <dfg/iter/szIterator.hpp>
#include <dfg/cont/contAlg.hpp>
#include <dfg/io/cstdio.hpp>
#include <dfg/time/timerCpu.hpp>
#include <dfg/str.hpp>
#include <dfg/os.hpp>
#include <dfg/str/stringLiteralCharToValue.hpp>
#include <dfg/cont/IntervalSetSerialization.hpp>
#include <dfg/cont/detail/MapBlockIndex.hpp>

#if (DFG_LANGFEAT_MUTEX_11 == 1)
    DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
        #include <thread> // Causes compiler warning at least in VC2012
    DFG_END_INCLUDE_WITH_DISABLED_WARNINGS
#endif // (DFG_LANGFEAT_MUTEX_11 == 1)

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
    EXPECT_EQ(4, cols.size());
    EXPECT_EQ(0, cols[0]);
    EXPECT_EQ(1, cols[1]);
    EXPECT_EQ(2, cols[2]);
    EXPECT_EQ(3, cols[3]);

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
    EXPECT_EQ(4, cols.size());
    EXPECT_EQ(0, cols[0]);
    EXPECT_EQ(1, cols[1]);
    EXPECT_EQ(2, cols[2]);
    EXPECT_EQ(3, cols[3]);

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

TEST(dfgCont, TableSz_contentStorageSizeInBytes)
{
    using namespace DFG_MODULE_NS(cont);
    DFG_CLASS_NAME(TableSz)<char> table;
    EXPECT_EQ(0, table.contentStorageSizeInBytes());
    table.setElement(0, 0, "a");
    EXPECT_EQ(2, table.contentStorageSizeInBytes());
    table.setElement(0, 1, "ab");
    EXPECT_EQ(5, table.contentStorageSizeInBytes());
    table.setElement(0, 0, "abc");
    EXPECT_EQ(9, table.contentStorageSizeInBytes());
    table.setElement(10, 0, "abcd");
    EXPECT_EQ(14, table.contentStorageSizeInBytes());
}

TEST(dfgCont, TableSz_forEachFwdColumnIndex)
{
    using namespace DFG_MODULE_NS(cont);
    {
        DFG_CLASS_NAME(TableSz)<char> table;
        table.setElement(0, 0, "");
        int nCount = 0;
        table.forEachFwdColumnIndex([&](size_t) { nCount++; });
        EXPECT_EQ(1, nCount);
    }

    {
        DFG_CLASS_NAME(TableSz)<char> table;
        table.setElement(1, 10, "");
        int nCount = 0;
        table.forEachFwdColumnIndex([&](size_t) { nCount++; });
        EXPECT_EQ(11, nCount);
    }
}

TEST(dfgCont, TableSz_forEachNonNullCell)
{
    using namespace DFG_MODULE_NS(cont);

    {
        typedef DFG_CLASS_NAME(TrivialPair)<size_t, size_t> PairT;
        DFG_CLASS_NAME(Vector)<PairT> expected;
        expected.push_back(PairT(1, 5));
        expected.push_back(PairT(2, 10));
        decltype(expected) cellIndexes;
        DFG_CLASS_NAME(TableSz)<char> table;
        table.setElement(1, 5, "");
        table.setElement(2, 10, "");
        table.forEachNonNullCell([&](size_t r, size_t c, const char*) { cellIndexes.push_back(PairT(r, c)); });
        EXPECT_EQ(expected, cellIndexes);
    }
}

TEST(dfgCont, TableSz_maxLimits)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    EXPECT_EQ(maxValueOfType<int16>(), (TableSz<char, int16>().maxColumnCount()));
    EXPECT_EQ(maxValueOfType<int16>() - 1, (TableSz<char, int16>().maxColumnIndex()));
    EXPECT_EQ(maxValueOfType<int16>(), (TableSz<char, int16>().maxRowCount()));
    EXPECT_EQ(maxValueOfType<int16>() - 1, (TableSz<char, int16>().maxRowIndex()));

    // These are commented out as they are not valid tests for non-AoS row storage.
    //EXPECT_EQ((TableSz<char, size_t>().m_colToRows.max_size()), (TableSz<char, size_t>().maxColumnCount()));
    //EXPECT_EQ(maxValueOfType<size_t>(), (TableSz<char, size_t>().maxRowCount()));
    //EXPECT_EQ(maxValueOfType<size_t>() - 1, (TableSz<char, size_t>().maxRowIndex()));

    TableSz<char, int> t;
    t.setElement(t.maxRowIndex(), 0, "a");
    EXPECT_EQ(t.maxRowCount(), t.rowCountByMaxRowIndex());
    t.setElement(t.maxRowIndex() + 1, 0, "b"); // This shouldn't add anything since index is too big.
    t.setElement(0, t.maxColumnIndex() + 1, "b"); // Nor this
    EXPECT_EQ(1, t.cellCountNonEmpty());
}

TEST(dfgCont, TableSz_insertRowsAt)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    // Overflow handling
    {
        TableSz<char, int16> t;
        t.setElement(0, 0, "a");
        t.insertRowsAt(0, maxValueOfType<int16>());
        EXPECT_EQ(t.maxRowCount(), t.rowCountByMaxRowIndex());
        t.removeRows(0, 32000);
        t.setElement(1000, 0, "b");
        EXPECT_EQ(1001, t.rowCountByMaxRowIndex());
        t.insertRowsAt(0, maxValueOfType<int16>());
        EXPECT_EQ(t.maxRowCount(), t.rowCountByMaxRowIndex());
    }
}

TEST(dfgCont, TableSz_removeRows)
{
    using namespace DFG_MODULE_NS(cont);
    DFG_CLASS_NAME(TableSz)<char> table;

    table.setElement(0, 0, "a");
    table.setElement(1, 0, "b");
    ASSERT_EQ(2, table.rowCountByMaxRowIndex());
    table.removeRows(0, 1);
    ASSERT_EQ(1, table.rowCountByMaxRowIndex());
    EXPECT_STREQ("b", table(0, 0));

    // Test removing rows that have no content.
    {
        table.setElement(10, 0, "c");
        ASSERT_EQ(11, table.rowCountByMaxRowIndex());
        table.removeRows(1, 9);
        ASSERT_EQ(2, table.rowCountByMaxRowIndex());
        EXPECT_STREQ("b", table(0, 0));
        EXPECT_STREQ("c", table(1, 0));
    }

    // Test removing last row
    {
        table.removeRows(1, 1);
        ASSERT_EQ(1, table.rowCountByMaxRowIndex());
        EXPECT_STREQ("b", table(0, 0));
    }

    // Test handling of zero-length removal
    {
        table.removeRows(1, 0);
        ASSERT_EQ(1, table.rowCountByMaxRowIndex());
    }

    // Test generic remove from middle.
    {
        table.insertRowsAt(0, 5);
        EXPECT_STREQ("b", table(5, 0));
        table.setElement(1, 0, "d");
        table.setElement(3, 0, "e");
        table.removeRows(0, 4);
        ASSERT_EQ(2, table.rowCountByMaxRowIndex());
        EXPECT_STREQ("b", table(1, 0));
    }

    // Test handling of big indexes.
    {
        TableSz<char, int> tableInt;
        tableInt.setElement(0, 0, "a");
        tableInt.setElement(1, 0, "b");
        tableInt.removeRows(1, DFG_ROOT_NS::maxValueOfType<int>());
        EXPECT_EQ(1, tableInt.rowCountByMaxRowIndex());
        EXPECT_STREQ("a", tableInt(0, 0));

        tableInt.setElement(tableInt.maxRowIndex(), 0, "c");
        EXPECT_EQ(2, tableInt.cellCountNonEmpty());
        tableInt.removeRows(tableInt.maxRowCount(), tableInt.maxRowCount());
        EXPECT_EQ(2, tableInt.cellCountNonEmpty());
        tableInt.removeRows(tableInt.maxRowIndex(), tableInt.maxRowCount());
        EXPECT_EQ(1, tableInt.cellCountNonEmpty());
    }
}

TEST(dfgCont, TableSz_addRemoveColumns)
{
    using namespace DFG_MODULE_NS(cont);
    DFG_CLASS_NAME(TableSz)<char> table;

    const auto isNullOrEmpty = [](const char* p) { return !p || p[0] == '\0'; };

    table.setElement(0, 0, "a");
    table.setElement(0, 1, "b");
    table.setElement(0, 2, "c");

    // Test that inserting 0 count behaves
    table.insertColumnsAt(0, 0);
    table.insertColumnsAt(5, 0);
    EXPECT_EQ(3, table.colCountByMaxColIndex());

    // Insert to front
    table.insertColumnsAt(0, 1);
    EXPECT_EQ(4, table.colCountByMaxColIndex());
    EXPECT_TRUE(isNullOrEmpty(table(0, 0)));
    EXPECT_STREQ("a", table(0, 1));
    EXPECT_STREQ("b", table(0, 2));
    EXPECT_STREQ("c", table(0, 3));

    // Insert one to middle
    table.insertColumnsAt(2, 1);
    EXPECT_EQ(5, table.colCountByMaxColIndex());
    EXPECT_TRUE(isNullOrEmpty(table(0, 0)));
    EXPECT_STREQ("a", table(0, 1));
    EXPECT_TRUE(isNullOrEmpty(table(0, 2)));
    EXPECT_STREQ("b", table(0, 3));
    EXPECT_STREQ("c", table(0, 4));

    // Insert multiple to middle
    table.insertColumnsAt(4, 2);
    EXPECT_EQ(7, table.colCountByMaxColIndex());
    EXPECT_TRUE(isNullOrEmpty(table(0, 0)));
    EXPECT_STREQ("a", table(0, 1));
    EXPECT_TRUE(isNullOrEmpty(table(0, 2)));
    EXPECT_STREQ("b", table(0, 3));
    table.setElement(0, 4, "d");
    EXPECT_STREQ("d", table(0, 4));
    EXPECT_TRUE(isNullOrEmpty(table(0, 5)));
    EXPECT_STREQ("c", table(0, 6));

    // Insert to end
    table.insertColumnsAt(10, 2);
    EXPECT_EQ(9, table.colCountByMaxColIndex());
    EXPECT_TRUE(isNullOrEmpty(table(0, 0)));
    EXPECT_STREQ("a", table(0, 1));
    EXPECT_TRUE(isNullOrEmpty(table(0, 2)));
    EXPECT_STREQ("b", table(0, 3));
    EXPECT_STREQ("d", table(0, 4));
    EXPECT_TRUE(isNullOrEmpty(table(0, 5)));
    EXPECT_STREQ("c", table(0, 6));
    EXPECT_TRUE(isNullOrEmpty(table(0, 7)));
    EXPECT_TRUE(isNullOrEmpty(table(0, 8)));

    // Test that deleting 0 count behaves
    table.eraseColumnsByPosAndCount(0, 0);
    table.eraseColumnsByPosAndCount(5, 0);
    table.eraseColumnsByPosAndCount(15, 0);
    EXPECT_EQ(9, table.colCountByMaxColIndex());

    // Test that deleting past end behaves
    table.eraseColumnsByPosAndCount(15, 10);
    EXPECT_EQ(9, table.colCountByMaxColIndex());

    // Erase one from front
    table.eraseColumnsByPosAndCount(0, 1);
    EXPECT_STREQ("a", table(0, 0));
    EXPECT_TRUE(isNullOrEmpty(table(0, 1)));
    EXPECT_STREQ("b", table(0, 2));
    EXPECT_STREQ("d", table(0, 3));
    EXPECT_TRUE(isNullOrEmpty(table(0, 4)));
    EXPECT_STREQ("c", table(0, 5));
    EXPECT_TRUE(isNullOrEmpty(table(0, 6)));
    EXPECT_TRUE(isNullOrEmpty(table(0, 7)));
    EXPECT_EQ(8, table.colCountByMaxColIndex());

    // Erase two from middle
    table.eraseColumnsByPosAndCount(1, 2);
    EXPECT_STREQ("a", table(0, 0));
    EXPECT_STREQ("d", table(0, 1));
    EXPECT_TRUE(isNullOrEmpty(table(0, 2)));
    EXPECT_STREQ("c", table(0, 3));
    EXPECT_TRUE(isNullOrEmpty(table(0, 4)));
    EXPECT_TRUE(isNullOrEmpty(table(0, 5)));
    EXPECT_EQ(6, table.colCountByMaxColIndex());

    // Erase multiple from end
    table.eraseColumnsByPosAndCount(2, 4);
    EXPECT_STREQ("a", table(0, 0));
    EXPECT_STREQ("d", table(0, 1));
    EXPECT_EQ(2, table.colCountByMaxColIndex());

    // Negative index handling
    {
        using namespace DFG_ROOT_NS;
        TableSz<char, int> tableInt;
        tableInt.insertColumnsAt(-1, 0);
        tableInt.insertColumnsAt(0, -1);
        EXPECT_EQ(0, tableInt.colCountByMaxColIndex());
    }
}

/*
    Some figures (all 32-bit builds):
        -2019-09-04: MSVC 2013  , std::vector<std::pair> as TableSz::ColumnIndexPairContainer  : ~1.73 s
        -2019-09-04: MSVC 2019.2, std::vector<std::pair> as TableSz::ColumnIndexPairContainer  : ~0.66 s
        -2019-09-04: MinGW 4.8.0, std::vector<std::pair> as TableSz::ColumnIndexPairContainer  : ~0.66 s
        -2019-09-04: MSVC 2013  , std::vector<TrivialPair> as TableSz::ColumnIndexPairContainer: ~2.07 s
        -2019-09-04: MSVC 2019.2, std::vector<TrivialPair> as TableSz::ColumnIndexPairContainer: ~0.30 s
        -2019-09-04: MinGW 4.8.0, std::vector<TrivialPair> as TableSz::ColumnIndexPairContainer: ~0.63 s
        -2019-09-04: MSVC 2013  , Vector<TrivialPair> as TableSz::ColumnIndexPairContainer     : ~0.64 s
        -2019-09-04: MSVC 2019.2, Vector<TrivialPair> as TableSz::ColumnIndexPairContainer     : ~0.30 s
        -2019-09-04: MinGW 4.8.0, Vector<TrivialPair> as TableSz::ColumnIndexPairContainer     : ~0.63 s
*/
TEST(dfgCont, TableSz_insertToFrontPerformance)
{
#if DFGTEST_ENABLE_BENCHMARKS == 0
    DFGTEST_MESSAGE("TableSz_insertToFrontPerformance skipped due to build settings");
#else
    using namespace DFG_ROOT_NS;

    typedef DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) TimerType;
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TableCsv)<char, uint32> Table;

#ifdef _DEBUG
    const uint32 nInitialBeginIndex = 100;
    const uint32 nEndIndex = 200;
#else
    const uint32 nInitialBeginIndex = 25000;
    const uint32 nEndIndex = 50000;
#endif // _DEBUG
    Table table;
    // Adding some content.
    for (uint32 r = nInitialBeginIndex; r < nEndIndex; ++r)
        table.setElement(r, 0, DFG_UTF8("a"));

    // Doing the actual benchmark by adding items to front.
    TimerType timer;
    for (uint32 r = 1; r <= nInitialBeginIndex; ++r)
        table.setElement(nInitialBeginIndex - r, 0, DFG_UTF8("a"));
    const auto elapsedTime = timer.elapsedWallSeconds();
    DFGTEST_MESSAGE("TableSz_insertToFrontPerformance: front inserts lasted " + DFG_MODULE_NS(str)::floatingPointToStr<DFG_CLASS_NAME(StringAscii)>(elapsedTime, 4).rawStorage());
#endif //DFGTEST_ENABLE_BENCHMARKS
}

TEST(dfgCont, TableSz_indexHandling)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    TableSz<char, int32> table;

    // Making sure that with negative row, item is not added.
    {
        table.addString("", -1, 0);
        table.addString("a", -1, 1);
        EXPECT_EQ(0, table.cellCountNonEmpty());
    }

    // Making sure that with negative column, item is not added.
    {
        table.addString("", 0, -1);
        table.addString("a", 0, -2);
        EXPECT_EQ(0, table.cellCountNonEmpty());
    }
}

TEST(dfgCont, TableCsv)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(io);
    using namespace DFG_MODULE_NS(utf);
    
    std::vector<std::string> paths;
    std::vector<TextEncoding> encodings;
    std::vector<char> separators;
    std::vector<EndOfLineType> eolTypes;
    // TODO: Read these properties from file names once "for-each-file-in-folder"-function is available.
    const TextEncoding contEncodings[] = { encodingUTF8,
                                            encodingUTF16Be,
                                            encodingUTF16Le,
                                            encodingUTF32Be,
                                            encodingUTF32Le };
    const std::string contSeparators[] = { "2C", "09", "3B" }; // ',' '\t', ';'
    const std::string contEols[] = { "n", "rn", "r" };
    const std::array<EndOfLineType, 3> contEolIndexToType = { EndOfLineTypeN, EndOfLineTypeRN, EndOfLineTypeR };
    
    const std::string sPathTemplate = "testfiles/csv_testfiles/csvtest%1%_BOM_sep_%2%_eol_%3%.csv";
    for (size_t iE = 0; iE < count(contEncodings); ++iE)
    {
        for (size_t iS = 0; iS < count(contSeparators); ++iS)
        {
            for (size_t iEol = 0; iEol < count(contEols); ++iEol)
            {
                auto s = DFG_MODULE_NS(str)::replaceSubStrs(sPathTemplate, "%1%", encodingToStrId(contEncodings[iE]));
                DFG_MODULE_NS(str)::replaceSubStrsInplace(s, "%2%", contSeparators[iS]);
                DFG_MODULE_NS(str)::replaceSubStrsInplace(s, "%3%", contEols[iEol]);
                const bool bFileExists = DFG_MODULE_NS(os)::isPathFileAvailable(s.c_str(), DFG_MODULE_NS(os)::FileModeRead);
                if (!bFileExists && iEol != 0)
                    continue; // There are no rn or r -versions of all files so simply skip such.
                EXPECT_TRUE(bFileExists);
                paths.push_back(std::move(s));
                encodings.push_back(contEncodings[iE]);
                eolTypes.push_back(contEolIndexToType[iEol]);
                const auto charValOpt = DFG_MODULE_NS(str)::stringLiteralCharToValue<char>(std::string("\\x") + contSeparators[iS]);
                EXPECT_TRUE(charValOpt.first);
                separators.push_back(charValOpt.second);
            }
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
        auto readFormat = table.defaultReadFormat();
        if (eolTypes[i] == EndOfLineTypeR) // \r is not detected automatically so setting it manually.
            readFormat.eolType(EndOfLineTypeR);
        table.readFromFile(s, readFormat);
        // ...check that read properties separator, separator char and encoding, matches...
        {
            EXPECT_EQ(encodings[i], table.m_readFormat.textEncoding()); // TODO: use access function for format info.
            EXPECT_EQ(separators[i], table.m_readFormat.separatorChar()); // TODO: use access function for format info.
            //EXPECT_EQ(eolTypes[i], table.m_readFormat.eolType()); // Commented out for now as table does not store original eol info. TODO: use access function for format info.
        }

        // ...write to memory using the same encoding as in file...
        std::string bytes;
        DFG_CLASS_NAME(OmcByteStream)<std::string> ostrm(&bytes);
        auto writeFormat = table.m_readFormat;
        writeFormat.eolType(eolTypes[i]);
        {
            auto writePolicy = table.createWritePolicy<decltype(ostrm)>(writeFormat);
            table.writeToStream(ostrm, writePolicy);
            // ...read file bytes...
            const auto fileBytes = fileToByteContainer<std::string>(s);
            // ...check that written bytes end with EOL...
            {
                const auto sEol = eolStrFromEndOfLineType(eolTypes[i]);
                std::string sEolEncoded;
                for (const auto& c : sEol)
                    cpToEncoded(static_cast<uint32>(c), std::back_inserter(sEolEncoded), encodings[i]);
                ASSERT_TRUE(bytes.size() >= sEolEncoded.size());
                EXPECT_TRUE(std::equal(bytes.cend() - sEolEncoded.size(), bytes.cend(), sEolEncoded.cbegin()));
            }
            // ...and compare with the original file excluding EOL
            const auto nEolSizeInBytes = eolStrFromEndOfLineType(eolTypes[i]).size() * baseCharacterSize(encodings[i]);
            EXPECT_EQ(fileBytes, std::string(bytes.cbegin(), bytes.cend() - nEolSizeInBytes));
        }

        // Check also that saving without BOM works.
        {
            std::string nonBomBytes;
            DFG_CLASS_NAME(OmcByteStream)<std::string> ostrmNonBom(&nonBomBytes);
            auto csvFormat = table.m_readFormat;
            csvFormat.bomWriting(false);
            csvFormat.eolType(eolTypes[i]);
            auto writePolicy = table.createWritePolicy<decltype(ostrmNonBom)>(csvFormat);
            table.writeToStream(ostrmNonBom, writePolicy);
            // ...and check that bytes match.
            const auto nBomSize = ::DFG_MODULE_NS(utf)::bomSizeInBytes(csvFormat.textEncoding());
            ASSERT_EQ(bytes.size(), nonBomBytes.size() + nBomSize);
            EXPECT_TRUE(std::equal(bytes.cbegin() + nBomSize, bytes.cend(), nonBomBytes.cbegin()));
        }
    }

    // Test that results are the same as with DelimitedTextReader.
    {
        std::wstring sFromFile;
        DFG_CLASS_NAME(IfStreamWithEncoding) istrm(paths.front());
        DFG_CLASS_NAME(DelimitedTextReader)::read<wchar_t>(istrm, ',', '"', '\n', [&](const size_t nRow, const size_t nCol, const wchar_t* const p, const size_t nSize)
        {
            std::wstring sUtfConverted;
            auto inputRange = DFG_ROOT_NS::makeSzRange((*tables.front())(nRow, nCol).c_str());
            DFG_MODULE_NS(utf)::utf8To16Native(inputRange, std::back_inserter(sUtfConverted));
            EXPECT_EQ(std::wstring(p, nSize), sUtfConverted);
        });
    }

    // Verify that all tables are identical.
    for (size_t i = 1; i < tables.size(); ++i)
    {
        EXPECT_TRUE(tables[0]->isContentAndSizesIdenticalWith(*tables[i]));
    }

    // Test BOM-handling. Makes sure that BOM gets handled correctly whether it is autodetected or told explicitly, and that BOM-less files are read correctly if correct encoding is given to reader.
    {
        std::string utf8Bomless;
        utf8Bomless.push_back('a');
        const uint32 nonAsciiChar = 0x20AC; // Euro-sign
        DFG_MODULE_NS(utf)::cpToEncoded(nonAsciiChar, std::back_inserter(utf8Bomless), encodingUTF8);
        utf8Bomless += ",b";
        std::string utf8WithBom = utf8Bomless;
        const auto bomBytes = encodingToBom(encodingUTF8);
        utf8WithBom.insert(utf8WithBom.begin(), bomBytes.cbegin(), bomBytes.cend());

        DFG_CLASS_NAME(TableCsv)<char, uint32> tableWithBomExplictEncoding;
        DFG_CLASS_NAME(TableCsv)<char, uint32> tableWithBomAutoDetectedEncoding;
        DFG_CLASS_NAME(TableCsv)<char, uint32> tableWithoutBom;

        const DFG_CLASS_NAME(CsvFormatDefinition) formatDef(',', '"', EndOfLineTypeN, encodingUTF8);
        tableWithBomExplictEncoding.readFromMemory(utf8WithBom.data(), utf8WithBom.size(), formatDef);
        tableWithBomAutoDetectedEncoding.readFromMemory(utf8WithBom.data(), utf8WithBom.size());
        tableWithoutBom.readFromMemory(utf8Bomless.data(), utf8Bomless.size(), formatDef);

        EXPECT_TRUE(tableWithBomExplictEncoding.isContentAndSizesIdenticalWith(tableWithBomAutoDetectedEncoding));
        EXPECT_TRUE(tableWithBomExplictEncoding.isContentAndSizesIdenticalWith(tableWithoutBom));
        ASSERT_EQ(1, tableWithBomAutoDetectedEncoding.rowCountByMaxRowIndex());
        ASSERT_EQ(2, tableWithBomAutoDetectedEncoding.colCountByMaxColIndex());

        DFG_CLASS_NAME(StringUtf8) sItem00(tableWithBomExplictEncoding(0, 0));
        DFG_CLASS_NAME(StringUtf8) sItem01(tableWithBomExplictEncoding(0, 1));
        ASSERT_EQ(4, sItem00.sizeInRawUnits());
        ASSERT_EQ(1, sItem01.sizeInRawUnits());
        EXPECT_EQ('a', utf8ToFixedChSizeStr<wchar_t>(sItem00.rawStorage()).front());
        EXPECT_EQ(nonAsciiChar, utf8ToFixedChSizeStr<wchar_t>(sItem00.rawStorage())[1]);
        EXPECT_EQ('b', utf8ToFixedChSizeStr<wchar_t>(sItem01.rawStorage()).front());
    }

    // Test unknown encoding handling, i.e. that bytes are read as specified as Latin-1.
    {
        DFG_CLASS_NAME(TableCsv)<char, uint32> table;
        std::array<unsigned char, 255> data;
        DFG_CLASS_NAME(StringUtf8) dataAsUtf8;
        for (size_t i = 0; i < data.size(); ++i)
        {
            unsigned char val = (i + 1 != DFG_U8_CHAR('\n')) ? static_cast<unsigned char>(i + 1) : DFG_U8_CHAR('a'); // Don't write '\n' to get single cell table.
            data[i] = val;
            cpToEncoded(data[i], std::back_inserter(dataAsUtf8.rawStorage()), encodingUTF8);
        }
        EXPECT_EQ(127 + 2*128, dataAsUtf8.sizeInRawUnits());
        const auto metaCharNone = DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone;
        table.readFromMemory(reinterpret_cast<const char*>(data.data()), data.size(), DFG_CLASS_NAME(CsvFormatDefinition)(metaCharNone,
                                                                                                                          metaCharNone,
                                                                                                                          EndOfLineTypeN,
                                                                                                                          encodingUnknown));
        ASSERT_EQ(1, table.rowCountByMaxRowIndex());
        ASSERT_EQ(1, table.colCountByMaxColIndex());
        EXPECT_EQ(dataAsUtf8, table(0,0));
    }

    // Test enclosement behaviour
    {
        const size_t nExpectedCount = 2;
        // Note: EbEnclose is missing because it would have need more work to implement writing something even for non-existent cells.
        const std::array<EnclosementBehaviour, nExpectedCount> enclosementItems = { EbEncloseIfNeeded, EbEncloseIfNonEmpty };
        const std::array<std::string, nExpectedCount> expected = {  DFG_ASCII("a,b,,\"c,d\",\"e\nf\",g\n,,r1c2,,,\n").c_str(),
                                                                    DFG_ASCII("\"a\",\"b\",,\"c,d\",\"e\nf\",\"g\"\n,,\"r1c2\",,,\n").c_str() };
        DFG_CLASS_NAME(TableCsv)<char, uint32> table;
        table.setElement(0, 0, DFG_UTF8("a"));
        table.setElement(0, 1, DFG_UTF8("b"));
        //table.setElement(0, 2, DFG_UTF8(""));
        table.setElement(0, 3, DFG_UTF8("c,d"));
        table.setElement(0, 4, DFG_UTF8("e\nf"));
        table.setElement(0, 5, DFG_UTF8("g"));
        table.setElement(1, 2, DFG_UTF8("r1c2"));
        for (size_t i = 0; i < nExpectedCount; ++i)
        {
            DFG_CLASS_NAME(OmcByteStream)<> ostrm;
            auto writePolicy = table.createWritePolicy<decltype(ostrm)>();
            writePolicy.m_format.enclosementBehaviour(enclosementItems[i]);
            writePolicy.m_format.bomWriting(false);
            table.writeToStream(ostrm, writePolicy);
            ASSERT_EQ(expected[i].size(), ostrm.container().size());
            EXPECT_TRUE(std::equal(expected[i].cbegin(), expected[i].cend(), ostrm.container().cbegin()));
        }

    }

    // TODO: test auto detection
}

TEST(dfgCont, TableCsv_memStreamTypes)
{
    // Test that writing to different memory streams work (in particular that BasicOmcByteStream works).

    using namespace DFG_ROOT_NS;
    const auto nRowCount = 10;
    const auto nColCount = 5;
    DFG_MODULE_NS(cont)::TableCsv<char, uint32> table;
    for (int c = 0; c < nColCount; ++c)
        for (int r = 0; r < nRowCount; ++r)
            table.setElement(r, c, DFG_UTF8("a"));

    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Vector)<char> VectorT;
    VectorT bytesStd;
    VectorT bytesOmc;
    VectorT bytesBasicOmc;

    // std::ostrstream
    {
        std::ostrstream ostrStream;
        table.writeToStream(ostrStream);
        const auto psz = ostrStream.str();
        bytesStd.assign(psz, psz + static_cast<std::streamoff>(ostrStream.tellp()));
        ostrStream.freeze(false);
    }

    // OmcByteStream
    {
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(OmcByteStream)<> omcStrm;
        table.writeToStream(omcStrm);
        bytesOmc = omcStrm.releaseData();
    }

    // BasicOmcByteStream
    {
        DFG_MODULE_NS(io)::BasicOmcByteStream<> ostrm;
        table.writeToStream(ostrm);
        bytesBasicOmc = ostrm.releaseData();
        EXPECT_TRUE(ostrm.m_internalData.empty()); // Make sure that releasing works.
    }

    EXPECT_EQ(bytesStd, bytesOmc);
    EXPECT_EQ(bytesStd, bytesBasicOmc);
}


namespace
{
    class SimpleStringMatcher
    {
    public:
        SimpleStringMatcher(DFG_ROOT_NS::StringViewSzUtf8 sv)
        {
            m_s = sv.c_str();
        }

        bool isMatch(const int nInputRow, const DFG_ROOT_NS::StringViewUtf8& sv)
        {
            DFG_UNUSED(nInputRow);
            std::string s(sv.dataRaw(), sv.length());
            return s.find(m_s.rawStorage()) != std::string::npos;
        }

        template <class Char_T, class Index_T>
        bool isMatchImpl(const int nInputRow, const typename DFG_MODULE_NS(cont)::TableCsv<Char_T, Index_T>::RowContentFilterBuffer& rowBuffer)
        {
            for (const auto& item : rowBuffer)
            {
                if (isMatch(nInputRow, item.second(rowBuffer)))
                    return true;
            }
            return false;
        }

        bool isMatch(const int nInputRow, const DFG_MODULE_NS(cont)::TableCsv<char, DFG_ROOT_NS::uint32>::RowContentFilterBuffer& rowBuffer)
        {
            return isMatchImpl<char, DFG_ROOT_NS::uint32>(nInputRow, rowBuffer);
        }

        bool isMatch(const int nInputRow, const DFG_MODULE_NS(cont)::TableCsv<char, int>::RowContentFilterBuffer& rowBuffer)
        {
            return isMatchImpl<char, int>(nInputRow, rowBuffer);
        }

        DFG_ROOT_NS::StringUtf8 m_s;
    };

}

TEST(dfgCont, TableCsv_filterCellHandler)
{
    using namespace DFG_ROOT_NS;

    const char szExample0[] = "00,01\n"
                              "10,11\n"
                              "20,21\n"
                              "30,31\n"
                              "40,41\n"
                              "50,51";

    const char szExample1[] = "00,01\n"
                              "\n"
                              "20,21\n"
                              "\n"
                              "\n"
                              "50,51";

    const char szExample2[] = "00,01,02,03\n"
                              "10,11,12,13\n"
                              "20,21,22,23\n"
                              "30,31,32,33\n"
                              "40,41,42,43\n";

    using IndexT = uint32;
    using TableT = DFG_MODULE_NS(cont)::TableCsv<char, IndexT>;

    // Simple test
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler();
        filterCellHandler.setIncludeRows(::DFG_MODULE_NS(cont)::intervalSetFromString<IndexT>("1;3:4"));
        table.readFromMemory(szExample0, DFG_COUNTOF_SZ(szExample0), table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(3, table.rowCountByMaxRowIndex());
        EXPECT_EQ(2, table.colCountByMaxColIndex());
        EXPECT_STREQ("10", table(0, 0).c_str());
        EXPECT_STREQ("11", table(0, 1).c_str());
        EXPECT_STREQ("30", table(1, 0).c_str());
        EXPECT_STREQ("31", table(1, 1).c_str());
        EXPECT_STREQ("40", table(2, 0).c_str());
        EXPECT_STREQ("41", table(2, 1).c_str());
    }

    // Testing handling of empty lines.
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler();
        filterCellHandler.setIncludeRows(::DFG_MODULE_NS(cont)::intervalSetFromString<IndexT>("1;5"));
        table.readFromMemory(szExample1, DFG_COUNTOF_SZ(szExample1), table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(2, table.rowCountByMaxRowIndex());
        EXPECT_EQ(2, table.colCountByMaxColIndex());
        EXPECT_STREQ("", table(0, 0).c_str());
        EXPECT_EQ(nullptr, table(0, 1).c_str());
        EXPECT_STREQ("50", table(1, 0).c_str());
        EXPECT_STREQ("51", table(1, 1).c_str());
    }

    // Testing handling of empty filter
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler();
        table.readFromMemory(szExample1, DFG_COUNTOF_SZ(szExample1), table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(0, table.rowCountByMaxRowIndex());
        EXPECT_EQ(0, table.colCountByMaxColIndex());
    }

    // Simple file read test
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler();
        filterCellHandler.setIncludeRows(::DFG_MODULE_NS(cont)::intervalSetFromString<IndexT>("1"));
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(1, table.rowCountByMaxRowIndex());
        EXPECT_EQ(3, table.colCountByMaxColIndex());
        EXPECT_STREQ("14510", table(0, 0).c_str());
        EXPECT_STREQ("26690", table(0, 1).c_str());
        EXPECT_STREQ("41354", table(0, 2).c_str());
    }

    // Row content filter: basic test
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler(SimpleStringMatcher(DFG_UTF8("1")));
        /*
        matrix_3x3.txt
            8925, 25460, 46586
            14510, 26690, 41354
            17189, 42528, 49812
        */
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(2, table.rowCountByMaxRowIndex());
        EXPECT_EQ(3, table.colCountByMaxColIndex());
        EXPECT_STREQ("14510", table(0, 0).c_str());
        EXPECT_STREQ("26690", table(0, 1).c_str());
        EXPECT_STREQ("41354", table(0, 2).c_str());
        EXPECT_STREQ("17189", table(1, 0).c_str());
        EXPECT_STREQ("42528", table(1, 1).c_str());
        EXPECT_STREQ("49812", table(1, 2).c_str());
    }

    // Index handling with int-index, mostly to check that this doesn't generate warnings.
    {
        DFG_MODULE_NS(cont)::TableCsv<char, int> table;
        auto filterCellHandler = table.createFilterCellHandler(SimpleStringMatcher(DFG_UTF8("1")));
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(2, table.rowCountByMaxRowIndex());
        EXPECT_EQ(3, table.colCountByMaxColIndex());
    }

    // Row content filter: last row handling
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler(SimpleStringMatcher(DFG_UTF8("49812")));
        /*
        matrix_3x3.txt
            8925, 25460, 46586
            14510, 26690, 41354
            17189, 42528, 49812
        */
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(1, table.rowCountByMaxRowIndex());
        EXPECT_EQ(3, table.colCountByMaxColIndex());
        EXPECT_STREQ("17189", table(0, 0).c_str());
        EXPECT_STREQ("42528", table(0, 1).c_str());
        EXPECT_STREQ("49812", table(0, 2).c_str());
    }

    // Row content filter: single column filter
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler(SimpleStringMatcher(DFG_UTF8("9")), 1);
        /*
        matrix_3x3.txt
            8925, 25460, 46586
            14510, 26690, 41354
            17189, 42528, 49812
        */
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(1, table.rowCountByMaxRowIndex());
        EXPECT_EQ(3, table.colCountByMaxColIndex());
        EXPECT_STREQ("14510", table(0, 0).c_str());
        EXPECT_STREQ("26690", table(0, 1).c_str());
        EXPECT_STREQ("41354", table(0, 2).c_str());
    }

    // Column filter
    {
        using namespace DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(cont);
        {
            TableT table;
            auto filterCellHandler = table.createFilterCellHandler();
            filterCellHandler.setIncludeRows(intervalSetFromString<IndexT>("1;3"));
            filterCellHandler.setIncludeColumns(intervalSetFromString<IndexT>(""));
            table.readFromMemory(szExample2, DFG_COUNTOF_SZ(szExample2), table.defaultReadFormat(), filterCellHandler);
            EXPECT_EQ(0, table.cellCountNonEmpty());
        }

        {
            TableT table;
            auto filterCellHandler = table.createFilterCellHandler();
            filterCellHandler.setIncludeRows(intervalSetFromString<IndexT>("1;3"));
            filterCellHandler.setIncludeColumns(intervalSetFromString<IndexT>("3"));
            table.readFromMemory(szExample2, DFG_COUNTOF_SZ(szExample2), table.defaultReadFormat(), filterCellHandler);
            EXPECT_EQ(2, table.rowCountByMaxRowIndex());
            EXPECT_EQ(1, table.colCountByMaxColIndex());
            EXPECT_STREQ("13", table(0, 0).c_str());
            EXPECT_STREQ("33", table(1, 0).c_str());
        }

        {
            TableT table;
            auto filterCellHandler = table.createFilterCellHandler();
            filterCellHandler.setIncludeRows(intervalSetFromString<IndexT>("1;3"));
            filterCellHandler.setIncludeColumns(intervalSetFromString<IndexT>("0:1;3"));
            table.readFromMemory(szExample2, DFG_COUNTOF_SZ(szExample2), table.defaultReadFormat(), filterCellHandler);
            EXPECT_EQ(2, table.rowCountByMaxRowIndex());
            EXPECT_EQ(3, table.colCountByMaxColIndex());
            EXPECT_STREQ("10", table(0, 0).c_str());
            EXPECT_STREQ("11", table(0, 1).c_str());
            EXPECT_STREQ("13", table(0, 2).c_str());
            EXPECT_STREQ("30", table(1, 0).c_str());
            EXPECT_STREQ("31", table(1, 1).c_str());
            EXPECT_STREQ("33", table(1, 2).c_str());
        }
    }

    // Row, column and content filter
    {
        /*
        matrix_3x3.txt
            8925, 25460, 46586
            14510, 26690, 41354
            17189, 42528, 49812
        */
        using namespace DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(cont);
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler(SimpleStringMatcher(DFG_UTF8("8")));
        filterCellHandler.setIncludeRows(intervalSetFromString<IndexT>("0;2"));
        filterCellHandler.setIncludeColumns(intervalSetFromString<IndexT>("1"));
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(1, table.rowCountByMaxRowIndex());
        EXPECT_EQ(1, table.colCountByMaxColIndex());
        EXPECT_EQ(1, table.cellCountNonEmpty());
        EXPECT_STREQ("42528", table(0, 0).c_str());
    }
}

namespace
{
    static void verifyFormatInFile(const ::DFG_ROOT_NS::StringViewSzC svPath,
                                   const char cSep,
                                   const char cEnc,
                                   const ::DFG_MODULE_NS(io)::EndOfLineType eolType,
                                   const ::DFG_MODULE_NS(io)::TextEncoding encoding
                                 )
    {
        using namespace DFG_ROOT_NS;
        const auto format = peekCsvFormatFromFile(svPath);
        EXPECT_EQ(cSep, format.separatorChar());
        DFG_UNUSED(cEnc);
        //EXPECT_EQ(cEnc, format.enclosingChar()); Detection is not supported
        EXPECT_EQ(eolType, format.eolType());
        EXPECT_EQ(encoding, format.textEncoding());
    }
}

TEST(dfgCont, TableCsv_peekCsvFormatFromFile)
{
    using namespace ::DFG_MODULE_NS(io);

    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16BE_BOM_sep_09_eol_n.csv",  '\t',   '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16BE_BOM_sep_2C_eol_n.csv",  ',',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16BE_BOM_sep_3B_eol_n.csv",  ';',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_09_eol_n.csv",  '\t',   '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_09_eol_rn.csv", '\t',   '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_2C_eol_n.csv",  ',',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_2C_eol_rn.csv", ',',    '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_3B_eol_n.csv",  ';',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_3B_eol_rn.csv", ';',    '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32BE_BOM_sep_09_eol_n.csv",  '\t',   '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32BE_BOM_sep_2C_eol_n.csv",  ',',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32BE_BOM_sep_3B_eol_n.csv",  ';',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32LE_BOM_sep_09_eol_n.csv",  '\t',   '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32LE_BOM_sep_2C_eol_n.csv",  ',',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32LE_BOM_sep_3B_eol_n.csv",  ';',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_09_eol_n.csv",     '\t',   '"', EndOfLineType::EndOfLineTypeN,   encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_09_eol_rn.csv",    '\t',   '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_2C_eol_n.csv",     ',',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_2C_eol_rn.csv",    ',',    '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_3B_eol_n.csv",     ';',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_3B_eol_rn.csv",    ';',    '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_1F_eol_n.csv",     '\x1F', '"', EndOfLineType::EndOfLineTypeN,   encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtest_plain_ascii_3x3_sep_2C_eol_n.csv", ',','"', EndOfLineType::EndOfLineTypeN,   encodingUnknown);

    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_2C_eol_r.csv",     ',',    '"', EndOfLineType::EndOfLineTypeR,   encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16BE_BOM_sep_2C_eol_r.csv",  ',',    '"', EndOfLineType::EndOfLineTypeR,   encodingUTF16Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_2C_eol_r.csv",  ',',    '"', EndOfLineType::EndOfLineTypeR,   encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32BE_BOM_sep_2C_eol_r.csv",  ',',    '"', EndOfLineType::EndOfLineTypeR,   encodingUTF32Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32LE_BOM_sep_2C_eol_r.csv",  ',',    '"', EndOfLineType::EndOfLineTypeR,   encodingUTF32Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtest_plain_ascii_3x3_sep_3B_eol_r.csv", ';','"', EndOfLineType::EndOfLineTypeR,   encodingUnknown);
}

TEST(dfgCont, CsvConfig)
{
    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig) config;
    config.loadFromFile("testfiles/csvConfigTest_0.csv");

    EXPECT_EQ(DFG_UTF8("UTF8"), config.value(DFG_UTF8("encoding")));
    EXPECT_EQ(DFG_UTF8("1"), config.value(DFG_UTF8("bom_writing")));
    EXPECT_EQ(DFG_UTF8("\\n"), config.value(DFG_UTF8("end_of_line_type")));
    EXPECT_EQ(DFG_UTF8(","), config.value(DFG_UTF8("separator_char")));
    EXPECT_EQ(DFG_UTF8("\""), config.value(DFG_UTF8("enclosing_char")));
    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("channels")));
    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("channels/0")));
    EXPECT_EQ(DFG_UTF8("integer"), config.value(DFG_UTF8("channels/0/type")));
    EXPECT_EQ(DFG_UTF8("50"), config.value(DFG_UTF8("channels/0/width")));
    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("channels/1")));
    EXPECT_EQ(DFG_UTF8("100"), config.value(DFG_UTF8("channels/1/width")));
    EXPECT_EQ(DFG_UTF8("also section-entries can have a value"), config.value(DFG_UTF8("properties")));
    EXPECT_EQ(DFG_UTF8("abc"), config.value(DFG_UTF8("properties/property_one")));
    EXPECT_EQ(DFG_UTF8("def"), config.value(DFG_UTF8("properties/property_two")));

    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("no_ending_separator"), DFG_UTF8("a")));
    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("no_ending_separator/0"), DFG_UTF8("a")));
    EXPECT_EQ(DFG_UTF8("a"), config.value(DFG_UTF8("no_ending_separator/1"), DFG_UTF8("")));
    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("no_ending_separator/2"), DFG_UTF8("a")));

    const int euroSign[] = { 0x20AC };
    EXPECT_EQ(DFG_ROOT_NS::SzPtrUtf8(DFG_MODULE_NS(utf)::codePointsToUtf8(euroSign).c_str()), config.value(DFG_UTF8("a_non_ascii_value")));

    EXPECT_EQ(DFG_UTF8("default_value"), config.value(DFG_UTF8("a/non_existent_item"), DFG_UTF8("default_value")));
    EXPECT_EQ(nullptr, config.valueStrOrNull(DFG_UTF8("a/non_existent_item")));

    EXPECT_EQ(19, config.entryCount());
}

TEST(dfgCont, CsvConfig_forEachStartingWith)
{
    typedef DFG_ROOT_NS::DFG_CLASS_NAME(StringUtf8) StringUTf8;
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig) ConfigT;
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Vector) < StringUTf8 > StorageT;
    ConfigT config;
    config.loadFromFile("testfiles/csvConfigTest_0.csv");
    StorageT expectedKeys;
    expectedKeys.push_back(StringUTf8(DFG_UTF8("property_one")));
    expectedKeys.push_back(StringUTf8(DFG_UTF8("property_two")));
    StorageT expectedValues;
    expectedValues.push_back(StringUTf8(DFG_UTF8("abc")));
    expectedValues.push_back(StringUTf8(DFG_UTF8("def")));

    StorageT actualKeys;
    StorageT actualValues;
    config.forEachStartingWith(DFG_UTF8("properties/"), [&](const ConfigT::StringViewT& key, const ConfigT::StringViewT& value)
    {
        actualKeys.push_back(key.toString());
        actualValues.push_back(value.toString());
    });
    EXPECT_EQ(expectedKeys, actualKeys);
    EXPECT_EQ(expectedValues, actualValues);
}

TEST(dfgCont, CsvConfig_saving)
{
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig) ConfigT;
    ConfigT config;

    config.loadFromFile("testfiles/csvConfigTest_1.csv");
    config.saveToFile("testfiles/generated/csvConfigTest_1.csv");
    ConfigT config2;
    EXPECT_NE(config, config2);
    config2.loadFromFile("testfiles/generated/csvConfigTest_1.csv");
    EXPECT_EQ(config, config2);

    // Test also that the output is exactly the same as input.
    {
        const auto inputBytes = DFG_MODULE_NS(io)::fileToVector("testfiles/csvConfigTest_1.csv");
        const auto writtenBytes = DFG_MODULE_NS(io)::fileToVector("testfiles/generated/csvConfigTest_1.csv");
        EXPECT_EQ(inputBytes, writtenBytes);
    }
}

TEST(dfgCont, CsvFormatDefinition_FromCsvConfig)
{
    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig) config;
    config.loadFromFile("testfiles/csvConfigTest_0.csv");
    DFG_ROOT_NS::DFG_CLASS_NAME(CsvFormatDefinition) format('a', 'b', DFG_MODULE_NS(io)::EndOfLineTypeRN, DFG_MODULE_NS(io)::encodingUnknown);
    format.bomWriting(false);
    format.fromConfig(config);
    EXPECT_EQ(DFG_MODULE_NS(io)::encodingUTF8, format.textEncoding());
    EXPECT_EQ('"', format.enclosingChar());
    EXPECT_EQ(',', format.separatorChar());
    EXPECT_EQ(DFG_MODULE_NS(io)::EndOfLineTypeN, format.eolType());
    EXPECT_EQ(true, format.bomWriting());
    EXPECT_EQ("abc", format.getProperty("property_one", ""));
    EXPECT_EQ("def", format.getProperty("property_two", ""));
}

TEST(dfgCont, CsvFormatDefinition_ToConfig)
{
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig) CsvConfig;
    typedef DFG_ROOT_NS::DFG_CLASS_NAME(CsvFormatDefinition) CsvFormatDef;
    // Basic test
    {
        CsvConfig inConfig;
        inConfig.loadFromFile("testfiles/csvConfigTest_2.csv");
        CsvFormatDef format('a', 'b', DFG_MODULE_NS(io)::EndOfLineTypeRN, DFG_MODULE_NS(io)::encodingUnknown);
        format.bomWriting(false);
        format.fromConfig(inConfig);
        EXPECT_EQ(DFG_MODULE_NS(io)::encodingUTF8, format.textEncoding());
        EXPECT_EQ('"', format.enclosingChar());
        EXPECT_EQ(',', format.separatorChar());
        EXPECT_EQ(DFG_MODULE_NS(io)::EndOfLineTypeRN, format.eolType());
        EXPECT_EQ(true, format.bomWriting());

        CsvConfig outConfig;
        format.appendToConfig(outConfig);

        EXPECT_EQ(inConfig, outConfig);
    }

    // Test 'no enclosing char'-handling
    {
        CsvFormatDef format('a', 'b', DFG_MODULE_NS(io)::EndOfLineTypeRN, DFG_MODULE_NS(io)::encodingUnknown);
        format.enclosingChar(DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone);
        CsvConfig config;
        format.appendToConfig(config);
        EXPECT_TRUE(DFG_MODULE_NS(str)::isEmptyStr(config.value(DFG_UTF8("enclosing_char"), DFG_UTF8("a"))));
    }
}

namespace
{

template <class T, size_t N>
static void mapBlockIndexTestImpl(::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::MapBlockIndex<T, N>&& m)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont)::DFG_DETAIL_NS;

    {
        EXPECT_TRUE(m.empty());
        EXPECT_EQ(0, m.size());

        m.set(0, "a");
        EXPECT_FALSE(m.empty());
        EXPECT_EQ(1, m.size());
        EXPECT_EQ(0, m.backKey());

        const uint32 nBigIndex = 10 * m.blockSize() + m.blockSize() / 2;
        m.set(nBigIndex - 1, "b");
        EXPECT_EQ(2, m.size());
        EXPECT_FALSE(m.empty());
        EXPECT_EQ(nBigIndex - 1, m.backKey());

        m.set(nBigIndex, "b2");
        EXPECT_EQ(3, m.size());
        EXPECT_FALSE(m.empty());
        EXPECT_EQ(nBigIndex, m.backKey());

        EXPECT_STREQ("a", m.value(0));
        EXPECT_EQ(nullptr, m.value(10));
        EXPECT_EQ(nullptr, m.value(10000));
        EXPECT_STREQ("b", m.value(nBigIndex - 1));
        EXPECT_STREQ("b2", m.value(nBigIndex));
        EXPECT_EQ(nullptr, m.value(nBigIndex - 2));
        EXPECT_EQ(nullptr, m.value(nBigIndex + 1));

        const uint32 nMiddleIndex = nBigIndex / 2;
        m.set(nMiddleIndex, "c");
        EXPECT_EQ(4, m.size());
        EXPECT_FALSE(m.empty());
        EXPECT_EQ(nBigIndex, m.backKey());

        // Inserting
        {
            // Insert to end (effectively a no-op)
            {
                m.insertKeysByPositionAndCount(nBigIndex + m.blockSize() * 2, 1000); // This is not expected to do anything
                EXPECT_EQ(4, m.size());
                EXPECT_FALSE(m.empty());
                EXPECT_EQ(nBigIndex, m.backKey());
            }

            // Insert before last in the same block
            {
                m.insertKeysByPositionAndCount(nBigIndex, 1);
                EXPECT_EQ(4, m.size());
                EXPECT_FALSE(m.empty());
                EXPECT_EQ(nBigIndex + 1, m.backKey());
                m.insertKeysByPositionAndCount(nBigIndex, 10);
                EXPECT_EQ(nBigIndex + 11, m.backKey());
            }

            // Insert before last in the preceeding block
            {
                m.insertKeysByPositionAndCount(nBigIndex - m.blockSize(), 1);
                EXPECT_EQ(4, m.size());
                EXPECT_FALSE(m.empty());
                EXPECT_EQ(nBigIndex + 12, m.backKey());
            }

            EXPECT_EQ(4, m.size());
            EXPECT_STREQ("a", m.value(0));
            EXPECT_STREQ("c", m.value(nMiddleIndex));
            EXPECT_STREQ("b", m.value(nBigIndex));
            EXPECT_STREQ("b2", m.value(nBigIndex + 12));

            // Insert before nMiddleIndex
            m.insertKeysByPositionAndCount(nMiddleIndex, 5);

            EXPECT_EQ(4, m.size());
            EXPECT_STREQ("a", m.value(0));
            EXPECT_STREQ("c", m.value(nMiddleIndex + 5));
            EXPECT_STREQ("b", m.value(nBigIndex + 5));
            EXPECT_STREQ("b2", m.value(nBigIndex + 17));

            // Insert before first
            m.insertKeysByPositionAndCount(0, 7);

            EXPECT_EQ(4, m.size());
            EXPECT_STREQ("a", m.value(7));
            EXPECT_STREQ("c", m.value(nMiddleIndex + 12));
            EXPECT_STREQ("b", m.value(nBigIndex + 12));
            EXPECT_STREQ("b2", m.value(nBigIndex + 24));
        }

        // Removing
        {
            const auto nSizeBeforeRemoves = m.size();
            // Removing from end, expected to do nothing.
            {
                m.removeKeysByPositionAndCount(m.backKey() + 1, 1000);
                m.removeKeysByPositionAndCount(m.backKey(), 0);
                EXPECT_EQ(nSizeBeforeRemoves, m.size());
                EXPECT_FALSE(m.empty());
            }

            // Removing non-mapped indexes from beginning
            {
                m.removeKeysByPositionAndCount(0, 7);
                EXPECT_EQ(nSizeBeforeRemoves, m.size());
                EXPECT_FALSE(m.empty());
                EXPECT_STREQ("a", m.value(0));
                EXPECT_STREQ("c", m.value(nMiddleIndex + 5));
                EXPECT_STREQ("b", m.value(nBigIndex + 5));
                EXPECT_STREQ("b2", m.value(nBigIndex + 17));
            }

            // Removing last
            {
                m.removeKeysByPositionAndCount(m.backKey(), m.maxKey() - 50); // Also a count overflow test by using large count value.
                EXPECT_EQ(nSizeBeforeRemoves - 1, m.size());
                EXPECT_FALSE(m.empty());
                EXPECT_STREQ("a", m.value(0));
                EXPECT_STREQ("c", m.value(nMiddleIndex + 5));
                EXPECT_STREQ("b", m.value(nBigIndex + 5));
            }

            // Removing non-mapped indexes between first and second
            {
                m.removeKeysByPositionAndCount(nMiddleIndex, 5);
                EXPECT_EQ(nSizeBeforeRemoves - 1, m.size());
                EXPECT_FALSE(m.empty());
                EXPECT_STREQ("a", m.value(0));
                EXPECT_STREQ("c", m.value(nMiddleIndex));
                EXPECT_STREQ("b", m.value(nBigIndex));
            }

            // Removing all but last
            {
                m.removeKeysByPositionAndCount(0, nBigIndex);
                EXPECT_EQ(1, m.size());
                EXPECT_FALSE(m.empty());
                EXPECT_STREQ("b", m.value(0));
            }

            // Removing last
            {
                m.removeKeysByPositionAndCount(0, 1);
                EXPECT_EQ(0, m.size());
                EXPECT_TRUE(m.empty());
            }

            // Test block removal from middle (originally this had a bug caught by dfgTestQt test dfgQt.CsvItemModel_removeRows)
            {
                m.set(0, "a");
                m.set(1, "b");
                m.set(2, "c");
                m.set(3, "d");
                m.removeKeysByPositionAndCount(1, 2);
                EXPECT_EQ(2, m.size());
                EXPECT_FALSE(m.empty());
                EXPECT_STREQ("a", m.value(0));
                EXPECT_STREQ("d", m.value(1));
            }
        }
    } // General tests
}

    template <class T, size_t N>
    static void mapBlockIndexOverflowTestImpl(::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::MapBlockIndex<T, N>&& m)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(cont)::DFG_DETAIL_NS;

        // Trying to insert more than what mapping can hold; should only insert as many as possible so that existing mappings do not fall off.
        m.set(1, "a");
        m.set(10, "b");
        m.insertKeysByPositionAndCount(10, m.maxKey());
        EXPECT_EQ(2, m.size());
        EXPECT_FALSE(m.empty());
        EXPECT_EQ(1, m.frontKey());
        EXPECT_EQ(m.maxKey(), m.backKey());
        EXPECT_STREQ("a", m.value(1));
        EXPECT_STREQ("b", m.value(m.backKey()));
    }

template <class T, size_t N>
static void MapBlockIndex_iteratorsImpl(::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::MapBlockIndex<T, N>&& m)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(cont)::DFG_DETAIL_NS;
    EXPECT_EQ(m.begin(), m.end());
    MapToStringViews<uint32, std::string> mExpected;
    mExpected.insert(5, "a");
    mExpected.insert(30, "b");
    mExpected.insert(10, "c");
    mExpected.insert(1, "d");
    mExpected.insert(22, "e");
    for (const auto& item : mExpected)
        m.set(item.first, item.second(mExpected).c_str());

    EXPECT_EQ(mExpected.size(), m.size());

    EXPECT_FALSE(m.begin() == m.end());
    auto iter = m.begin();
    EXPECT_EQ(1, iter->first);
    EXPECT_STREQ("d", iter->second);
    ++iter;
    EXPECT_EQ(5, iter->first);
    EXPECT_STREQ("a", iter->second);

    for (const auto& item : m)
    {
        EXPECT_EQ(mExpected[item.first], item.second);
    }
}

} // unnamed namespace

TEST(dfgCont, MapBlockIndex)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont)::DFG_DETAIL_NS;

    mapBlockIndexTestImpl(MapBlockIndex<const char*, 16>());
    mapBlockIndexTestImpl(MapBlockIndex<const char*, 0>(16));

    mapBlockIndexOverflowTestImpl(MapBlockIndex<const char*, 1048576>());
    mapBlockIndexOverflowTestImpl(MapBlockIndex<const char*, 0>(1048576));
}

TEST(dfgCont, MapBlockIndex_iterators)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont)::DFG_DETAIL_NS;

    MapBlockIndex_iteratorsImpl(MapBlockIndex<const char*, 16>());
    MapBlockIndex_iteratorsImpl(MapBlockIndex<const char*, 0>(16));
}

namespace
{
    template <class T, size_t N>
    static std::tuple<double, size_t, double> mapBlockIndex_benchmarksSingleItem(::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::MapBlockIndex<T, N>&& m)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(cont)::DFG_DETAIL_NS;
        using namespace DFG_MODULE_NS(time);
        using namespace DFG_MODULE_NS(rand);

        std::tuple<double, size_t, double> rv;

        auto randEng = createDefaultRandEngineUnseeded();
        randEng.seed(1234);
        auto distrEng = makeDistributionEngineUniform(&randEng, size_t(0), size_t(100));

        // Inserting
        {
            TimerCpu timer;
            for (uint32 i = 0; i < 30000000; ++i)
                m.set(i, distrEng());
            std::get<0>(rv) = timer.elapsedWallSeconds();
        }

        // Calculating sum
        size_t nSum = 0;
        {
            for (const auto& val : m)
                nSum += val.second;
        }
        std::get<1>(rv) = nSum;

        // Clearing
        {
            TimerCpu timer;
            m.clear();
            std::get<2>(rv) = timer.elapsedWallSeconds();
        }
        return rv;
    }

    template <size_t N>
    static void mapBlockIndex_benchmarksImpl()
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(func);
        using namespace DFG_MODULE_NS(str);
        using namespace DFG_MODULE_NS(cont)::DFG_DETAIL_NS;

        MemFuncAvg<double> insertRatioAvg;
        MemFuncAvg<double> clearRatioAvg;

        char buffer[32];
        char buffer2[32];
        for (int i = 0; i < 3; ++i)
        {
            const auto staticValues = mapBlockIndex_benchmarksSingleItem(MapBlockIndex<size_t, N>());
            const auto dynamicValues = mapBlockIndex_benchmarksSingleItem(MapBlockIndex<size_t, 0>(N));
            EXPECT_EQ(std::get<1>(dynamicValues), std::get<1>(staticValues));
            const auto insertRatio = std::get<0>(dynamicValues) / std::get<0>(staticValues);
            const auto clearRatio = std::get<2>(dynamicValues) / std::get<2>(staticValues);
            insertRatioAvg(insertRatio);
            clearRatioAvg(clearRatio);
            DFGTEST_MESSAGE("Block " << N << ", insert ratio: " << floatingPointToStr(insertRatio, buffer, 4) <<
                            ", static insert time: " << floatingPointToStr(std::get<0>(staticValues), buffer, 4) <<
                            ", clear ratio: " << floatingPointToStr(clearRatio, buffer2, 4) <<
                            ", static clear time: " << floatingPointToStr(std::get<2>(staticValues), buffer, 4));
        }
        
        DFGTEST_MESSAGE("Block " << N << ", insert ratio avg: " << floatingPointToStr(insertRatioAvg.average(), buffer, 4) << ", clear ratio avg: " << floatingPointToStr(clearRatioAvg.average(), buffer2, 4) << '\n');
    }
}

/*

Example run with VC2019 release x64 2021-04-17
 [ RUN      ] dfgCont.MapBlockIndex_benchmarks
    MESSAGE: Block 5, insert ratio: 1.1, static insert time: 1.813, clear ratio: 1.051, static clear time: 0.2217
    MESSAGE: Block 5, insert ratio: 1.104, static insert time: 1.803, clear ratio: 1.031, static clear time: 0.2269
    MESSAGE: Block 5, insert ratio: 1.12, static insert time: 1.811, clear ratio: 1.006, static clear time: 0.2295
    MESSAGE: Block 5, insert ratio avg: 1.108, clear ratio avg: 1.03

    MESSAGE: Block 1000, insert ratio: 1.117, static insert time: 1.378, clear ratio: 1.037, static clear time: 0.03073
    MESSAGE: Block 1000, insert ratio: 1.122, static insert time: 1.371, clear ratio: 1.012, static clear time: 0.03191
    MESSAGE: Block 1000, insert ratio: 1.135, static insert time: 1.36, clear ratio: 1.068, static clear time: 0.03206
    MESSAGE: Block 1000, insert ratio avg: 1.125, clear ratio avg: 1.039

    MESSAGE: Block 4096, insert ratio: 1.139, static insert time: 1.351, clear ratio: 0.869, static clear time: 0.03384
    MESSAGE: Block 4096, insert ratio: 1.222, static insert time: 1.367, clear ratio: 0.7576, static clear time: 0.02905
    MESSAGE: Block 4096, insert ratio: 1.144, static insert time: 1.419, clear ratio: 0.7525, static clear time: 0.03141
    MESSAGE: Block 4096, insert ratio avg: 1.168, clear ratio avg: 0.793

    MESSAGE: Block 32768, insert ratio: 1.141, static insert time: 1.337, clear ratio: 0.6589, static clear time: 0.02573
    MESSAGE: Block 32768, insert ratio: 1.148, static insert time: 1.32, clear ratio: 0.9711, static clear time: 0.01505
    MESSAGE: Block 32768, insert ratio: 1.149, static insert time: 1.314, clear ratio: 0.9274, static clear time: 0.01403
    MESSAGE: Block 32768, insert ratio avg: 1.146, clear ratio avg: 0.8525

    MESSAGE: Block 262144, insert ratio: 1.143, static insert time: 1.333, clear ratio: 0.9125, static clear time: 0.02324
    MESSAGE: Block 262144, insert ratio: 1.133, static insert time: 1.35, clear ratio: 1.008, static clear time: 0.02331
    MESSAGE: Block 262144, insert ratio: 1.134, static insert time: 1.346, clear ratio: 1.005, static clear time: 0.02187
    MESSAGE: Block 262144, insert ratio avg: 1.136, clear ratio avg: 0.9753


Example run with MinGW 7.3 O2 x64 2021-04-17
[ RUN      ] dfgCont.MapBlockIndex_benchmarks
    MESSAGE: Block 5, insert ratio: 1.083, static insert time: 1.159, clear ratio: 1.038, static clear time: 0.2186
    MESSAGE: Block 5, insert ratio: 1.185, static insert time: 1.15, clear ratio: 1.008, static clear time: 0.2361
    MESSAGE: Block 5, insert ratio: 1.045, static insert time: 1.252, clear ratio: 0.9246, static clear time: 0.2471
    MESSAGE: Block 5, insert ratio avg: 1.104, clear ratio avg: 0.9902

    MESSAGE: Block 1000, insert ratio: 1.122, static insert time: 0.814, clear ratio: 1.031, static clear time: 0.02961
    MESSAGE: Block 1000, insert ratio: 1.102, static insert time: 0.8202, clear ratio: 1.047, static clear time: 0.03076
    MESSAGE: Block 1000, insert ratio: 1.101, static insert time: 0.8214, clear ratio: 1.004, static clear time: 0.03187
    MESSAGE: Block 1000, insert ratio avg: 1.108, clear ratio avg: 1.027

    MESSAGE: Block 4096, insert ratio: 1.14, static insert time: 0.7868, clear ratio: 0.8714, static clear time: 0.03378
    MESSAGE: Block 4096, insert ratio: 1.159, static insert time: 0.7849, clear ratio: 0.8289, static clear time: 0.02815
    MESSAGE: Block 4096, insert ratio: 1.172, static insert time: 0.77, clear ratio: 0.7296, static clear time: 0.03075
    MESSAGE: Block 4096, insert ratio avg: 1.157, clear ratio avg: 0.81

    MESSAGE: Block 32768, insert ratio: 1.145, static insert time: 0.7709, clear ratio: 0.9171, static clear time: 0.01971
    MESSAGE: Block 32768, insert ratio: 1.147, static insert time: 0.7598, clear ratio: 0.8682, static clear time: 0.01549
    MESSAGE: Block 32768, insert ratio: 1.148, static insert time: 0.7547, clear ratio: 0.9222, static clear time: 0.01467
    MESSAGE: Block 32768, insert ratio avg: 1.147, clear ratio avg: 0.9025

    MESSAGE: Block 262144, insert ratio: 1.171, static insert time: 0.7753, clear ratio: 1.112, static clear time: 0.02049
    MESSAGE: Block 262144, insert ratio: 1.136, static insert time: 0.7809, clear ratio: 1.057, static clear time: 0.02057
    MESSAGE: Block 262144, insert ratio: 1.136, static insert time: 0.7792, clear ratio: 1.094, static clear time: 0.02228
    MESSAGE: Block 262144, insert ratio avg: 1.148, clear ratio avg: 1.088
*/
TEST(dfgCont, MapBlockIndex_benchmarks)
{
#if defined(_DEBUG) || DFGTEST_ENABLE_BENCHMARKS != 1
    DFGTEST_MESSAGE("MapBlockIndex_benchmarks skipped due to build settings");
#else
    {
        mapBlockIndex_benchmarksImpl<5>();
        mapBlockIndex_benchmarksImpl<1000>();
        mapBlockIndex_benchmarksImpl<4096>();
        mapBlockIndex_benchmarksImpl<32768>();
        mapBlockIndex_benchmarksImpl<262144>();
    }
#endif
}
