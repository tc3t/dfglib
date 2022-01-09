#include <dfg/qt/CsvItemModel.hpp>
#include <dfg/io.hpp>
#include <dfg/io/OmcByteStream.hpp>
#include <dfg/qt/containerUtils.hpp>
#include <dfg/qt/connectHelper.hpp>
#include <dfg/math.hpp>
#include <dfg/qt/sqlTools.hpp>
#include <dfg/qt/PatternMatcher.hpp>
#include <dfg/iter/FunctionValueIterator.hpp>
#include <dfg/os/TemporaryFileStream.hpp>
#include "../dfgTest/dfgTest.hpp"
#include "dfgTestQt_gtestPrinters.hpp"

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include <gtest/gtest.h>
    #include <QAction>
    #include <QCoreApplication>
    #include <QDateTime>
    #include <QFile>
    #include <QFileInfo>
    #include <QDialog>
    #include <QThread>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

TEST(dfgQt, CsvItemModel)
{

    for (size_t i = 0;; ++i)
    {
        const QString sInputPath = QString("testfiles/example%1.csv").arg(i);
        const QString sOutputPath = QString("testfiles/generated/example%1.testOutput.csv").arg(i);
        if (!QFileInfo::exists(sInputPath))
        {
            const bool bAtLeastOneFileFound = (i != 0);
            EXPECT_TRUE(bAtLeastOneFileFound);
            break;
        }

        DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel) model;

        DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::LoadOptions loadOptions;
        if (i == 3) // Example 3 needs non-native eol-settings.
            loadOptions.separatorChar('\t');
        if (i == 4)
            loadOptions.textEncoding(DFG_MODULE_NS(io)::encodingLatin1);

        const auto bOpenSuccess = model.openFile(sInputPath, loadOptions);
        EXPECT_EQ(true, bOpenSuccess);

        DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::SaveOptions saveOptions(&model);
        if (i == 3)
            saveOptions = loadOptions;
        if (i == 4)
            saveOptions.textEncoding(loadOptions.textEncoding());

        saveOptions.eolType((i == 1 || i == 4) ? DFG_MODULE_NS(io)::EndOfLineTypeN : DFG_MODULE_NS(io)::EndOfLineTypeRN);

        // Note: EOL-setting does not affect in-cell EOL-items, i.e. saving of eol-chars inside cell content
        // is not affected by eol-setting.
        const auto bSaveSuccess = model.saveToFile(sOutputPath, saveOptions);
        EXPECT_EQ(true, bSaveSuccess);

        auto inputBytesWithAddedEol = DFG_MODULE_NS(io)::fileToVector(sInputPath.toLatin1().data());
        const auto sEol = ::DFG_MODULE_NS(io)::eolStrFromEndOfLineType(saveOptions.eolType());
        inputBytesWithAddedEol.insert(inputBytesWithAddedEol.end(), sEol.cbegin(), sEol.cend());
        const auto outputBytes = DFG_MODULE_NS(io)::fileToVector(sOutputPath.toLatin1().data());
        EXPECT_EQ(inputBytesWithAddedEol, outputBytes);

        // Test in-memory saving
        {
            DFG_MODULE_NS(io)::DFG_CLASS_NAME(OmcByteStream)<std::vector<char>> strm;
            model.save(strm, saveOptions);
            const auto& outputBytesMc = strm.container();
            EXPECT_EQ(inputBytesWithAddedEol, outputBytesMc);
        }
    }

    // Testing insertRows/Columns
    {
        using namespace DFG_MODULE_NS(qt);
        CsvItemModel model;
        // Making sure that bad insert counts are handled correctly.
        EXPECT_FALSE(model.insertRows(0, 0));
        EXPECT_FALSE(model.insertColumns(0, 0));
        EXPECT_FALSE(model.insertRows(0, -5));
        EXPECT_FALSE(model.insertColumns(0, -5));
        EXPECT_EQ(0, model.rowCount());
        EXPECT_EQ(0, model.columnCount());
    }
}

TEST(dfgQt, CsvItemModel_removeRows)
{
    DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel) model;

    model.insertColumns(0, 1);
    model.insertRows(0, 10);
    EXPECT_EQ(10, model.rowCount());
    EXPECT_EQ(1, model.columnCount());
    for (int r = 0, nRowCount = model.rowCount(); r < nRowCount; ++r)
    {
        char c[2] = { static_cast<char>('a' + r), '\0' };
        model.setDataNoUndo(r, 0, DFG_ROOT_NS::SzPtrAscii(c));
    }
    model.removeRows(0, 2);
    EXPECT_EQ(8, model.rowCount());
    EXPECT_EQ(QString("c"), model.data(model.index(0, 0)).toString());

    model.removeRows(0, 1);
    EXPECT_EQ(7, model.rowCount());
    EXPECT_EQ(QString("d"), model.data(model.index(0, 0)).toString());

    const int removeRows0[] = { -1 };
    model.removeRows(removeRows0);
    EXPECT_EQ(7, model.rowCount()); // Passing bogus row index shouldn't do anything.

    const int removeRows1[] = { 1, 5, 6 };
    model.removeRows(removeRows1);
    EXPECT_EQ(4, model.rowCount());
    EXPECT_EQ(QString("d"), model.data(model.index(0, 0)).toString());
    EXPECT_EQ(QString("f"), model.data(model.index(1, 0)).toString());

    const int removeRows2[] = { 1, 2 };
    model.removeRows(removeRows2);
    EXPECT_EQ(2, model.rowCount());
    EXPECT_EQ(QString("d"), model.data(model.index(0, 0)).toString());
    EXPECT_EQ(QString("h"), model.data(model.index(1, 0)).toString());

    // Removing remaining rows and testing that bogus entries in the input list gets handled correctly also in this case.
    const int removeRows3[] = { -5, -2, -1, 0, 1, 2, 3, 50 };
    model.removeRows(removeRows3);
    EXPECT_EQ(0, model.rowCount());
}

TEST(dfgQt, CsvItemModel_filteredRead)
{
    using namespace DFG_MODULE_NS(qt);
    const char szFilePath5x5[] = "testfiles/test_5x5_sep1F_content_row_comma_column.csv";
    const char szFilePathAb[] = "testfiles/example_forFilterRead.csv";

    CsvItemModel model;

    // Row and column filter.
    {
        auto options = model.getLoadOptionsForFile(szFilePath5x5);
        options.setProperty(CsvOptionProperty_includeRows, "0;2;4");
        options.setProperty(CsvOptionProperty_includeColumns, "2;3;5");
        model.openFile(szFilePath5x5, options);
        ASSERT_EQ(2, model.rowCount());
        ASSERT_EQ(3, model.columnCount());
        EXPECT_EQ(QString("Col1"), model.getHeaderName(0));
        EXPECT_EQ(QString("Col2"), model.getHeaderName(1));
        EXPECT_EQ(QString("Col4"), model.getHeaderName(2));
        EXPECT_EQ(QString("2,1"), QString::fromUtf8(model.RawStringPtrAt(0, 0).c_str()));
        EXPECT_EQ(QString("2,2"), QString::fromUtf8(model.RawStringPtrAt(0, 1).c_str()));
        EXPECT_EQ(QString("2,4"), QString::fromUtf8(model.RawStringPtrAt(0, 2).c_str()));
        EXPECT_EQ(QString("4,1"), QString::fromUtf8(model.RawStringPtrAt(1, 0).c_str()));
        EXPECT_EQ(QString("4,2"), QString::fromUtf8(model.RawStringPtrAt(1, 1).c_str()));
        EXPECT_EQ(QString("4,4"), QString::fromUtf8(model.RawStringPtrAt(1, 2).c_str()));
    }

    // Content filter, basic test
    {
        auto options = model.getLoadOptionsForFile(szFilePath5x5);
        const char szFilters[] = R"({ "text": "1,4" })";
        options.setProperty(CsvOptionProperty_readFilters, szFilters);
        model.openFile(szFilePath5x5, options);
        ASSERT_EQ(1, model.rowCount());
        ASSERT_EQ(5, model.columnCount());
        EXPECT_EQ(QString("Col4"), model.getHeaderName(4));
        EXPECT_EQ(QString("1,4"), QString::fromUtf8(model.RawStringPtrAt(0, 4).c_str()));
        EXPECT_EQ(5, model.m_table.cellCountNonEmpty());
    }

    // Content filter, both OR and AND filters
    {
        auto options = model.getLoadOptionsForFile(szFilePath5x5);
        const char szFilters[] = R"({ "text": "3,*", "and_group": "a" }
                                    { "text": "*,0", "and_group": "a" }
                                    {"text": "1,4", "and_group": "b"} )";
        options.setProperty(CsvOptionProperty_readFilters, szFilters);
        model.openFile(szFilePath5x5, options);
        ASSERT_EQ(2, model.rowCount());
        ASSERT_EQ(5, model.columnCount());
        EXPECT_EQ(QString("Col0"), model.getHeaderName(0));
        EXPECT_EQ(QString("Col4"), model.getHeaderName(4));
        EXPECT_EQ(QString("1,4"), QString::fromUtf8(model.RawStringPtrAt(0, 4).c_str()));
        EXPECT_EQ(QString("3,0"), QString::fromUtf8(model.RawStringPtrAt(1, 0).c_str()));
        EXPECT_EQ(10, model.m_table.cellCountNonEmpty());
    }

    // Row, column and content filters
    {
        auto options = model.getLoadOptionsForFile(szFilePath5x5);
        const char szFilters[] = R"({ "text": "3,*", "and_group": "a" }
                                    { "text": "*,1", "and_group": "a" }
                                    { "text": "4,1", "and_group": "b"} )";
        options.setProperty(CsvOptionProperty_includeRows, "0;3;5");
        options.setProperty(CsvOptionProperty_includeColumns, "2");
        options.setProperty(CsvOptionProperty_readFilters, szFilters);
        model.openFile(szFilePath5x5, options);
        ASSERT_EQ(1, model.rowCount());
        ASSERT_EQ(1, model.columnCount());
        EXPECT_EQ(QString("Col1"), model.getHeaderName(0));
        EXPECT_EQ(QString("3,1"), QString::fromUtf8(model.RawStringPtrAt(0, 0).c_str()));
        EXPECT_EQ(1, model.m_table.cellCountNonEmpty());
    }

    // Case sensitivity in content filter
    {
        {
            auto options = model.getLoadOptionsForFile(szFilePathAb);
            const char szFilters[] = R"({ "text": "A", "case_sensitive": true } )";
            options.setProperty(CsvOptionProperty_readFilters, szFilters);
            model.openFile(szFilePathAb, options);
            EXPECT_EQ(0, model.m_table.cellCountNonEmpty());
        }

        {
            auto options = model.getLoadOptionsForFile(szFilePathAb);
            const char szFilters[] = R"({ "text": "A", "case_sensitive": false } )";
            options.setProperty(CsvOptionProperty_readFilters, szFilters);
            model.openFile(szFilePathAb, options);
            EXPECT_EQ(6, model.m_table.cellCountNonEmpty());
        }
    }

    // Column apply-filters in content filter
    {
        auto options = model.getLoadOptionsForFile(szFilePathAb);
        const char szFilters[] = R"({ "text": "a", "apply_columns": "2" } )";
        options.setProperty(CsvOptionProperty_readFilters, szFilters);
        model.openFile(szFilePathAb, options);
        ASSERT_EQ(1, model.rowCount());
        ASSERT_EQ(2, model.columnCount());
        EXPECT_EQ(QString("Col0"), model.getHeaderName(0));
        EXPECT_EQ(QString("Col1"), model.getHeaderName(1));
        EXPECT_EQ(QString("b"), QString::fromUtf8(model.RawStringPtrAt(0, 0).c_str()));
        EXPECT_EQ(QString("ab"), QString::fromUtf8(model.RawStringPtrAt(0, 1).c_str()));
        EXPECT_EQ(2, model.m_table.cellCountNonEmpty());
    }

    // Row apply-filters in content filter
    {
        auto options = model.getLoadOptionsForFile(szFilePathAb);
        const char szFilters[] = R"({ "text": "c", "apply_rows": "1;3" } )";
        options.setProperty(CsvOptionProperty_readFilters, szFilters);
        model.openFile(szFilePathAb, options);
        ASSERT_EQ(2, model.rowCount());
        ASSERT_EQ(2, model.columnCount());
        EXPECT_EQ(QString("Col0"), model.getHeaderName(0));
        EXPECT_EQ(QString("Col1"), model.getHeaderName(1));
        EXPECT_EQ(QString("b"), QString::fromUtf8(model.RawStringPtrAt(0, 0).c_str()));
        EXPECT_EQ(QString("ab"), QString::fromUtf8(model.RawStringPtrAt(0, 1).c_str()));
        EXPECT_EQ(QString("ab"), QString::fromUtf8(model.RawStringPtrAt(1, 0).c_str()));
        EXPECT_EQ(QString("c"), QString::fromUtf8(model.RawStringPtrAt(1, 1).c_str()));
        EXPECT_EQ(4, model.m_table.cellCountNonEmpty());
    }
}

namespace
{
    struct CsvItemModelReadFormatTestCase
    {
        const char* m_pszInput;
        DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::LoadOptions m_loadOptions;
    };
}

// Test that saving uses the same settings as loading
TEST(dfgQt, CsvItemModel_readFormatUsageOnWrite)
{
    typedef DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel) ModelT;
    typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(OmcByteStream)<std::string> StrmT;
    typedef DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::LoadOptions LoadOptionsT;
    typedef DFG_ROOT_NS::DFG_CLASS_NAME(CsvFormatDefinition) CsvFormatDef;
#define DFG_TEMP_BOM DFG_UTF_BOM_STR_UTF8
    CsvItemModelReadFormatTestCase testCases[] =
    {
        // Auto-detected separators with default enclosing char.
        { DFG_TEMP_BOM "a,b,\",c\"\n",                          LoadOptionsT() },
        { DFG_TEMP_BOM "a;b;\";c\"\n",                          LoadOptionsT() },
        { DFG_TEMP_BOM "a\tb\t\"\tc\"\n",                       LoadOptionsT() },
        { DFG_TEMP_BOM "a\x1f" "b\x1f" "\"\x1f" "c\"\n",        LoadOptionsT() },
        // Custom formats
        { DFG_TEMP_BOM "a2b2\"2c\"\n",                          CsvFormatDef('2', '"', DFG_MODULE_NS(io)::EndOfLineTypeN, DFG_MODULE_NS(io)::encodingUTF8)  },      // Separator '2', enc '"', eol n
        { DFG_TEMP_BOM "a|b|^|c^\n",                            CsvFormatDef('|', '^', DFG_MODULE_NS(io)::EndOfLineTypeN, DFG_MODULE_NS(io)::encodingUTF8) },       // Separator '|', enc '^', eol n
        { DFG_TEMP_BOM "a|b|^|c^\r\n",                          CsvFormatDef('|', '^', DFG_MODULE_NS(io)::EndOfLineTypeRN, DFG_MODULE_NS(io)::encodingUTF8) },      // Separator '|', enc '^', eol rn
        { "\xE4" "|\xF6" "|^|c^\n",                             CsvFormatDef('|', '^', DFG_MODULE_NS(io)::EndOfLineTypeN, DFG_MODULE_NS(io)::encodingLatin1) },     // Separator '|', enc '^', eol n, encoding Latin1
        // Note: EOL-handling is not tested thoroughly as parsing does not detect EOL (e.g. reading with \n will read \r\n fine, but does not automatically set read EOL as \r\n).
    };

    for (auto iter = std::begin(testCases); iter != std::end(testCases); ++iter)
    {
        ModelT model;
        model.openFromMemory(iter->m_pszInput, DFG_MODULE_NS(str)::strLen(iter->m_pszInput), iter->m_loadOptions);
        EXPECT_EQ(3, model.columnCount());
        StrmT ostrm;
        model.save(ostrm);

        EXPECT_EQ(iter->m_pszInput, ostrm.container());
    }
#undef DFG_TEMP_BOM
}

// Tests for SQLite handling (open, save)
TEST(dfgQt, CsvItemModel_SQLite)
{
    // Testing that csv -> SQLite -> csv roundtrip is non-modifying given that sqlite -> csv is done with correct save options.
    using namespace DFG_MODULE_NS(qt);
    const char szFilePath[] = "testfiles/example3.csv";
    const QString sSqlitePath = QString("testfiles/generated/csvItemModel_sqliteTest.sqlite3");
    const QString sCsvFromSqlitePath = QString("testfiles/generated/csvItemModel_sqliteTest.csv");
    const auto initialBytes = ::DFG_MODULE_NS(io)::fileToByteContainer<std::string>(szFilePath) + "\r\n";
    QStringList columnNamesInCsv;
    auto originalSaveOptions = CsvItemModel().getSaveOptions();

    // Removing existing temp files
    {
        QFile::remove(sSqlitePath);
        QFile::remove(sCsvFromSqlitePath);
    }

    // Reading from csv and exporting as SQLite
    {
        CsvItemModel model;
        EXPECT_TRUE(model.openFile(szFilePath));
        columnNamesInCsv = model.getColumnNames();
        originalSaveOptions = model.getSaveOptions();
        EXPECT_TRUE(model.exportAsSQLiteFile(sSqlitePath));
    }

    // Reading from SQLite and writing as csv
    {
        CsvItemModel model;
        const auto tableNames = ::DFG_MODULE_NS(sql)::SQLiteDatabase::getSQLiteFileTableNames(sSqlitePath);
        ASSERT_EQ(1, tableNames.size());
        const auto columnNames = ::DFG_MODULE_NS(sql)::SQLiteDatabase::getSQLiteFileTableColumnNames(sSqlitePath, tableNames.front());
        EXPECT_EQ(columnNamesInCsv, columnNames);
        EXPECT_TRUE(model.openFromSqlite(sSqlitePath, QString("SELECT * FROM '%1';").arg(tableNames.front())));
        EXPECT_TRUE(model.saveToFile(sCsvFromSqlitePath, originalSaveOptions));
    }

    const auto finalBytes = ::DFG_MODULE_NS(io)::fileToByteContainer<std::string>(qStringToFileApi8Bit(sCsvFromSqlitePath));
    EXPECT_EQ(initialBytes, finalBytes);

    EXPECT_TRUE(QFile::remove(sSqlitePath));
    EXPECT_TRUE(QFile::remove(sCsvFromSqlitePath));
}

TEST(dfgQt, CsvItemModel_setSize)
{
    using namespace DFG_MODULE_NS(qt);
    CsvItemModel model;
    EXPECT_FALSE(model.setSize(-1, -1));
    EXPECT_TRUE(model.setSize(1, -1));
    EXPECT_EQ(1, model.rowCount());
    EXPECT_EQ(0, model.columnCount());
    EXPECT_TRUE(model.setSize(-1, 1));
    EXPECT_EQ(1, model.rowCount());
    EXPECT_EQ(1, model.columnCount());
    model.setDataNoUndo(0, 0, DFG_UTF8("abc"));
    EXPECT_TRUE(model.setSize(10, 500));
    EXPECT_EQ(10, model.rowCount());
    EXPECT_EQ(500, model.columnCount());
    EXPECT_TRUE(model.setSize(200, 30));
    EXPECT_EQ(200, model.rowCount());
    EXPECT_EQ(30, model.columnCount());
    EXPECT_TRUE(model.setSize(1, 1));
    EXPECT_FALSE(model.setSize(1, 1));
    EXPECT_EQ("abc", model.rawStringViewAt(0, 0).asUntypedView());
    EXPECT_TRUE(model.setSize(0, 0));
    EXPECT_EQ(0, model.rowCount());
    EXPECT_EQ(0, model.columnCount());
}
