#include "stdafx.h"

#if (defined(DFGTEST_BUILD_MODULE_CONT_TABLES) && DFGTEST_BUILD_MODULE_CONT_TABLES == 1) || (!defined(DFGTEST_BUILD_MODULE_CONT_TABLES) && DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/cont.hpp>
#include <dfg/cont/table.hpp>
#include <dfg/cont/arrayWrapper.hpp>
#include <string>
#include <memory>
#include <dfg/ptrToContiguousMemory.hpp>
#include <dfg/dfgBase.hpp>
#include <dfg/ReadOnlySzParam.hpp>
#include <dfg/cont/CsvConfig.hpp>
#include <dfg/cont/MapToStringViews.hpp>
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
#include <dfg/cont/detail/MapBlockIndex.hpp>
#include <dfg/cont/tableUtils.hpp>

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

TEST(dfgCont, TableSz_swapCellContent)
{
    using namespace DFG_MODULE_NS(cont);
    TableSz<char> table;

    table.setElement(1, 3, "abc");
    table.setElement(4, 2, "def");
    DFGTEST_EXPECT_LEFT(5, table.rowCountByMaxRowIndex());
    DFGTEST_EXPECT_LEFT(4, table.colCountByMaxColIndex());
    DFGTEST_EXPECT_LEFT("abc", table.viewAt(1, 3));
    DFGTEST_EXPECT_LEFT("def", table.viewAt(4, 2));
    table.swapCellContent(50, 0, 0, 50);
    // Making sure that swapping non-existing cells does not cause effective size to change.
    DFGTEST_EXPECT_LEFT(5, table.rowCountByMaxRowIndex());
    DFGTEST_EXPECT_LEFT(4, table.colCountByMaxColIndex());

    table.swapCellContent(4, 2, 1, 3);
    DFGTEST_EXPECT_LEFT("abc", table.viewAt(4, 2));
    DFGTEST_EXPECT_LEFT("def", table.viewAt(1, 3));
    DFGTEST_EXPECT_LEFT(5, table.rowCountByMaxRowIndex());
    DFGTEST_EXPECT_LEFT(4, table.colCountByMaxColIndex());

    table.swapCellContent(4, 2, 0, 1);
    DFGTEST_EXPECT_LEFT("", table.viewAt(4, 2));
    DFGTEST_EXPECT_LEFT("abc", table.viewAt(0, 1));
    DFGTEST_EXPECT_LEFT(5, table.rowCountByMaxRowIndex());
    DFGTEST_EXPECT_LEFT(4, table.colCountByMaxColIndex());

    table.swapCellContent(0, 1, 10, 1);
    DFGTEST_EXPECT_LEFT("", table.viewAt(0, 1));
    DFGTEST_EXPECT_LEFT("abc", table.viewAt(10, 1));
    DFGTEST_EXPECT_LEFT(11, table.rowCountByMaxRowIndex());
    DFGTEST_EXPECT_LEFT(4, table.colCountByMaxColIndex());

    table.swapCellContent(1, 3, 4, 1);
    table.swapCellContent(4, 1, 10, 1);
    DFGTEST_EXPECT_LEFT("abc", table.viewAt(4, 1));
    DFGTEST_EXPECT_LEFT("def", table.viewAt(10, 1));
    DFGTEST_EXPECT_LEFT(11, table.rowCountByMaxRowIndex());
    DFGTEST_EXPECT_LEFT(4, table.colCountByMaxColIndex());

    table.swapCellContent(10, 1, 2, 15);
    DFGTEST_EXPECT_LEFT("", table.viewAt(10, 1));
    DFGTEST_EXPECT_LEFT("def", table.viewAt(2, 15));
    DFGTEST_EXPECT_LEFT(11, table.rowCountByMaxRowIndex());
    DFGTEST_EXPECT_LEFT(16, table.colCountByMaxColIndex());
}

TEST(dfgCont, TableSz_appendTablesWithMove)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);

    using TableT = TableSz<char>;
    using TableCsv = TableCsv<char, uint32>;

    // Basic test with csv-file
    {
        TableCsv table;
        table.readFromFile("testfiles/matrix_3x3.txt");
        std::vector<StringUtf8> cells;
        table.forEachNonNullCell([&](const uint32 r, const uint32 c, StringViewUtf8 sv)
            {
                const auto n = pairIndexToLinear(r, c, uint32(3));
                if (cells.size() <= n)
                    cells.resize(n + 1);
                cells[n] = sv.toString();
            });
        DFGTEST_ASSERT_EQ(9, cells.size());
        TableCsv table2;
        table2.readFromFile("testfiles/matrix_3x3.txt");
        DFGTEST_EXPECT_TRUE(table.appendTablesWithMove(makeRange(&table2, &table2 + 1)));
        // Checking that moving cleared source tables.
        DFGTEST_EXPECT_LEFT(0, table2.rowCountByMaxRowIndex());
        DFGTEST_EXPECT_LEFT(0, table2.colCountByMaxColIndex());

        DFGTEST_EXPECT_LEFT(6, table.rowCountByMaxRowIndex());
        DFGTEST_EXPECT_LEFT(3, table.colCountByMaxColIndex());
        size_t nIndex = 0;
        table.forEachNonNullCell([&](const uint32 r, const uint32 c, StringViewUtf8 sv)
        {
            const auto n = pairIndexToLinear(r, c, uint32(3));
            DFGTEST_EXPECT_LEFT(cells[n % cells.size()], sv);
            ++nIndex;
        });
    }

    // Testing destination table within source tables.
    {
        TableT t0;
        t0.setElement(0, 0, "a");
        t0.setElement(1, 0, "");
        TableT t1;
        t1.setElement(0, 0, "b");
        std::vector<TableT*> addItems;
        addItems.push_back(&t0);
        addItems.push_back(&t1);
        addItems.push_back(&t0);
        DFGTEST_EXPECT_TRUE(t0.appendTablesWithMove(addItems, [](TableT* p) { return p; }));
        DFGTEST_EXPECT_LEFT(7, t0.rowCountByMaxRowIndex());
        DFGTEST_EXPECT_LEFT(1, t0.colCountByMaxColIndex());
        DFGTEST_EXPECT_STREQ("a", t0(0, 0));
        DFGTEST_EXPECT_STREQ("", t0(1, 0));
        DFGTEST_EXPECT_EQ(t0(0, 0), t0(2, 0));
        DFGTEST_EXPECT_EQ(t0(1, 0), t0(3, 0));
        DFGTEST_EXPECT_STREQ("b", t0(4, 0));
        DFGTEST_EXPECT_STREQ("a", t0(5, 0));
        DFGTEST_EXPECT_STREQ("", t0(6, 0));
    }

    // Tests for uneven column sizes, i.e. that tables get appended correctly even if tables have differently sized columns
    {
        TableT table;
        table.insertColumnsAt(0, 3);
        table.setElement(0, 0, "t0(0,0)");
        // Note: (0, 1) is intentionally left empty to test that appending for every column starts at rowCountByMaxRowIndex() instead of column-specific size.
        {
            std::vector<TableT> newTables(2);
            newTables[0].setElement(0, 0, "");
            newTables[0].setElement(1, 0, "t1(1,0)");
            newTables[0].setElement(0, 1, "t1(0,1)");
            // nothing in (1, 1)

            newTables[1].setElement(0, 0, "t2(0,0)");
            // nothing in (0, 1)
            newTables[1].setElement(0, 2, "t2(0,2)");

            // nothing in (1, 1)
            newTables[1].setElement(1, 1, "");
            // nothing in (1, 2)

            DFGTEST_EXPECT_TRUE(table.appendTablesWithMove(newTables));
        }
        DFGTEST_EXPECT_LEFT(5, table.rowCountByMaxRowIndex());
        DFGTEST_EXPECT_LEFT(3, table.colCountByMaxColIndex());
        DFGTEST_EXPECT_STREQ("t0(0,0)", table(0, 0));
        DFGTEST_EXPECT_LEFT(nullptr,    table(0, 1));
        DFGTEST_EXPECT_LEFT(nullptr,    table(0, 2));

        DFGTEST_EXPECT_STREQ("",        table(1, 0));
        DFGTEST_EXPECT_LEFT(&table.m_emptyString, toCharPtr_raw(table(1, 0))); // Tests implementation detail of adjusting empty string optimization references.
        DFGTEST_EXPECT_STREQ("t1(0,1)", table(1, 1));
        DFGTEST_EXPECT_LEFT(nullptr,    table(1, 2));

        DFGTEST_EXPECT_STREQ("t1(1,0)", table(2, 0));
        DFGTEST_EXPECT_LEFT(nullptr,    table(2, 1));
        DFGTEST_EXPECT_LEFT(nullptr,    table(2, 2));

        DFGTEST_EXPECT_STREQ("t2(0,0)", table(3, 0));
        DFGTEST_EXPECT_LEFT(nullptr,    table(3, 1));
        DFGTEST_EXPECT_STREQ("t2(0,2)", table(3, 2));

        DFGTEST_EXPECT_LEFT(nullptr,    table(4, 0));
        DFGTEST_EXPECT_STREQ("",        table(4, 1));
        DFGTEST_EXPECT_LEFT(&table.m_emptyString, toCharPtr_raw(table(4, 1))); // The same comment as in earlier m_emptyString-case
        DFGTEST_EXPECT_LEFT(nullptr,    table(4, 2));
    }

    // Testing that final table has column count of maximum column count of tables
    {
        std::vector<TableT> tables(3);
        tables[0].setElement(0, 0, "a");
        tables[1].setElement(0, 2, "b");
        tables[2].setElement(0, 3, "c");
        TableT t;
        DFGTEST_EXPECT_TRUE(t.appendTablesWithMove(tables));
        DFGTEST_EXPECT_LEFT(3, t.rowCountByMaxRowIndex());
        DFGTEST_EXPECT_LEFT(4, t.colCountByMaxColIndex());
        DFGTEST_EXPECT_STREQ("a", t(0, 0));
        DFGTEST_EXPECT_STREQ("b", t(1, 2));
        DFGTEST_EXPECT_STREQ("c", t(2, 3));
        DFGTEST_EXPECT_LEFT(3, t.cellCountNonEmpty());
    }

    // Checking that append fails if final row count would be more than maxRowCount()
    {
        std::vector<TableSz<char, uint16>> tables(3);
        tables[0].setElement(25000, 0, "a");
        tables[1].setElement(25000, 0, "b");
        tables[2].setElement(25000, 0, "c");
        DFGTEST_EXPECT_FALSE(tables[0].appendTablesWithMove(makeRange(tables.begin() + 1, tables.end())));
        DFGTEST_EXPECT_LEFT(25001, tables[0].rowCountByMaxRowIndex());
        DFGTEST_EXPECT_STREQ("b", tables[1](25000, 0)); // Checking that sources were not cleared given that appending failed.
        DFGTEST_EXPECT_STREQ("c", tables[2](25000, 0)); // Checking that sources were not cleared given that appending failed.
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

namespace
{
    class CustomString : public std::string
    {
    public:
        CustomString() {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_1(CustomString, std::string) {}
    };
};

TEST(dfgCont, tableUtils_readStreamToTable)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(io);
    std::string s = "a,b,c,d\n"
        "e,f,g,h\n"
        "i,j,k,l,m,n";
    BasicImStream strm(s.c_str(), s.size());
    std::array<std::string, 14> contExpected = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n" };
    const auto cont = readStreamToTable(strm, ',', char(-1), '\n');

    Table<CustomString> table2;
    {
        BasicImStream strm2(s.c_str(), s.size());
        readStreamToTable(strm2, ',', char(-1), '\n', table2);
    }
    TableSz<char> table2Sz;
    {
        BasicImStream strm2(s.c_str(), s.size());
        readStreamToTable(strm2, ',', char(-1), '\n', table2Sz);
    }
    EXPECT_EQ(cont.getCellCount(), table2.getCellCount());
    if (cont.getCellCount() == table2.getCellCount())
        EXPECT_TRUE((std::equal(cont.cbegin(), cont.cend(), table2.cbegin())));

    EXPECT_EQ(cont.getRowCount(), 3);
    if (cont.getRowCount() >= 3)
    {
        EXPECT_EQ(cont.getColumnCountOnRow(0), 4);
        EXPECT_EQ(cont.getColumnCountOnRow(1), 4);
        EXPECT_EQ(cont.getColumnCountOnRow(2), 6);
    }

    size_t nCounter = 0;
    for (size_t r = 0; r < cont.getRowCount(); ++r)
    {
        for (size_t c = 0; c < cont.getColumnCountOnRow(r); ++c)
        {
            if (!isValidIndex(contExpected, nCounter))
            {
                ADD_FAILURE();
                break;
            }
            EXPECT_EQ(cont(r, c), contExpected[nCounter++]);
        }
    }
}

#endif // Build on/off switch
