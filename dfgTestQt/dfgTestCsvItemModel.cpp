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
#include <dfg/charts/commonChartTools.hpp>
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
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    #include <QStringConverter>
#else
    #include <QTextCodec>
#endif // QT_VERSION
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
        EXPECT_EQ(QString("2,1"), QString::fromUtf8(model.rawStringPtrAt(0, 0).c_str()));
        EXPECT_EQ(QString("2,2"), QString::fromUtf8(model.rawStringPtrAt(0, 1).c_str()));
        EXPECT_EQ(QString("2,4"), QString::fromUtf8(model.rawStringPtrAt(0, 2).c_str()));
        EXPECT_EQ(QString("4,1"), QString::fromUtf8(model.rawStringPtrAt(1, 0).c_str()));
        EXPECT_EQ(QString("4,2"), QString::fromUtf8(model.rawStringPtrAt(1, 1).c_str()));
        EXPECT_EQ(QString("4,4"), QString::fromUtf8(model.rawStringPtrAt(1, 2).c_str()));
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
        EXPECT_EQ(QString("1,4"), QString::fromUtf8(model.rawStringPtrAt(0, 4).c_str()));
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
        EXPECT_EQ(QString("1,4"), QString::fromUtf8(model.rawStringPtrAt(0, 4).c_str()));
        EXPECT_EQ(QString("3,0"), QString::fromUtf8(model.rawStringPtrAt(1, 0).c_str()));
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
        EXPECT_EQ(QString("3,1"), QString::fromUtf8(model.rawStringPtrAt(0, 0).c_str()));
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
        EXPECT_EQ(QString("b"), QString::fromUtf8(model.rawStringPtrAt(0, 0).c_str()));
        EXPECT_EQ(QString("ab"), QString::fromUtf8(model.rawStringPtrAt(0, 1).c_str()));
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
        EXPECT_EQ(QString("b"), QString::fromUtf8(model.rawStringPtrAt(0, 0).c_str()));
        EXPECT_EQ(QString("ab"), QString::fromUtf8(model.rawStringPtrAt(0, 1).c_str()));
        EXPECT_EQ(QString("ab"), QString::fromUtf8(model.rawStringPtrAt(1, 0).c_str()));
        EXPECT_EQ(QString("c"), QString::fromUtf8(model.rawStringPtrAt(1, 1).c_str()));
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
    typedef DFG_MODULE_NS(qt)::CsvItemModel ModelT;
    typedef DFG_MODULE_NS(io)::OmcByteStream<std::string> StrmT;
    typedef DFG_MODULE_NS(qt)::CsvItemModel::LoadOptions LoadOptionsT;
    typedef DFG_ROOT_NS::CsvFormatDefinition CsvFormatDef;
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

        DFGTEST_EXPECT_EQ(iter->m_pszInput, ostrm.container());
        DFGTEST_EXPECT_EQ(iter->m_pszInput, model.saveToByteString()); // Making sure that also saveToByteString() uses correct save options
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

TEST(dfgQt, CsvItemModel_setColumnType)
{
    using namespace DFG_MODULE_NS(qt);
    CsvItemModel model;
    model.setSize(1, 2);
    model.setModifiedStatus(false);
    DFGTEST_EXPECT_FALSE(model.isModified());
    model.setColumnType(0, CsvItemModel::ColTypeText); // Should do nothing since type already is text type
    DFGTEST_EXPECT_FALSE(model.isModified());
    model.setColumnType(0, ""); // Should do nothing since type argument is empty.
    DFGTEST_EXPECT_FALSE(model.isModified());
    model.setColumnType(0, "text"); // Should do nothing since type already is text type
    DFGTEST_EXPECT_FALSE(model.isModified());
    model.setColumnType(0, CsvItemModel::ColTypeNumber);
    DFGTEST_EXPECT_TRUE(model.isModified());
    DFGTEST_EXPECT_LEFT(CsvItemModel::ColTypeNumber, model.getColType(0));
    DFGTEST_EXPECT_LEFT(CsvItemModel::ColTypeText, model.getColType(1));
    model.setColumnType(0, "text");
    model.setColumnType(1, "number");
    DFGTEST_EXPECT_LEFT(CsvItemModel::ColTypeText, model.getColType(0));
    DFGTEST_EXPECT_LEFT(CsvItemModel::ColTypeNumber, model.getColType(1));
}

TEST(dfgQt, CsvItemModel_cellReadOnly)
{
    using namespace DFG_MODULE_NS(qt);
    {
        CsvItemModel model;
        // Setting size to 2x4
        model.setSize(2, 4);
        // Setting cell (1, 0) to "a"
        DFGTEST_EXPECT_TRUE(model.setDataNoUndo(1, 0, DFG_UTF8("a")));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("a", model.rawStringViewAt(1, 0));
        // Setting read-only to (1, 0)
        model.setCellReadOnlyStatus(1, 0, true);
        // Making sure that can't edit (1, 0)
        DFGTEST_EXPECT_FALSE(model.setDataNoUndo(1, 0, DFG_UTF8("b")));
        // Inserting row to beginning
        model.insertRow(0);
        DFGTEST_EXPECT_LEFT(3, model.rowCount());
        // Removing columns (for testing that implementation is not relying on columnCount()-dependent LinearIndex)
        model.removeColumns(2, 2);
        DFGTEST_EXPECT_LEFT(2, model.columnCount());
        // Making sure that (1, 0) is still read-only
        DFGTEST_EXPECT_FALSE(model.setDataNoUndo(1, 0, DFG_UTF8("b")));
        // Removing read-only status
        model.setCellReadOnlyStatus(1, 0, false);
        // Writing new content to (1, 0).
        DFGTEST_EXPECT_TRUE(model.setDataNoUndo(1, 0, DFG_UTF8("b")));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("b", model.rawStringViewAt(1, 0));
    }
}

TEST(dfgQt, CsvItemModel_cellDataAsDouble)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(qt);
    using namespace DFG_MODULE_NS(str);

    {
        CsvItemModel model;
        model.setSize(6, 1);
        constexpr auto doubleMax = (std::numeric_limits<double>::max)();
        constexpr auto doubleInf = std::numeric_limits<double>::infinity();

        // Setting values
        DFGTEST_EXPECT_TRUE(model.setDataNoUndo(0, 0, DFG_UTF8("1.25")));
        DFGTEST_EXPECT_TRUE(model.setDataNoUndo(1, 0, DFG_UTF8("inf")));
        DFGTEST_EXPECT_TRUE(model.setDataNoUndo(2, 0, DFG_UTF8("-inf")));
        DFGTEST_EXPECT_TRUE(model.setDataNoUndo(3, 0, DFG_UTF8("nan")));
        DFGTEST_EXPECT_TRUE(model.setDataNoUndo(4, 0, DFG_UTF8("non-value")));
        DFGTEST_EXPECT_TRUE(model.setDataNoUndo(5, 0, StringUtf8::fromRawString(toStrC(doubleMax))));

        // Testing that get expected results
        DFGTEST_EXPECT_LEFT(1.25, model.cellDataAsDouble(0, 0));
        DFGTEST_EXPECT_FALSE(doubleInf > model.cellDataAsDouble(1, 0));
        DFGTEST_EXPECT_FALSE(-doubleInf < model.cellDataAsDouble(2, 0));
        DFGTEST_EXPECT_NAN(model.cellDataAsDouble(3, 0));
        DFGTEST_EXPECT_NAN(model.cellDataAsDouble(3, 0, nullptr, 1));
        DFGTEST_EXPECT_NAN(model.cellDataAsDouble(4, 0));
        DFGTEST_EXPECT_LEFT(1, model.cellDataAsDouble(4, 0, nullptr, 1));
        DFGTEST_EXPECT_LEFT(doubleMax, model.cellDataAsDouble(5, 0));

        using ChartDataType = ::DFG_MODULE_NS(charts)::ChartDataType;
        // Installing custom parser
        model.setColumnStringToDoubleParser(0, CsvItemModel::ColInfo::StringToDoubleParser(QString(), [](CsvItemModel::ColInfo::StringToDoubleParserParam param, const QString&)
            {
                const auto sv = param.view();
                if (sv == DFG_UTF8("1.25"))
                {
                    param.setInterpretedChartType(ChartDataType::DataType::dateAndTimeMillisecondTz);
                    return 10.0;
                }
                else if (sv == DFG_UTF8("inf"))
                {
                    param.setInterpretedChartType(ChartDataType::DataType::dayTimeMillisecond);
                    return 20.0;
                }
                else if (sv == DFG_UTF8("-inf"))
                {
                    param.setInterpretedChartType(ChartDataType::DataType::dayTime);
                    return 30.0;
                }
                else
                {
                    param.setInterpretedChartType(ChartDataType::DataType::unknown);
                    return param.conversionErrorReturnValue();
                }
            }));
        ChartDataType dataType;
        DFGTEST_EXPECT_LEFT(10, model.cellDataAsDouble(0, 0, &dataType));
        DFGTEST_EXPECT_LEFT(ChartDataType::dateAndTimeMillisecondTz, dataType);

        DFGTEST_EXPECT_LEFT(20, model.cellDataAsDouble(1, 0, &dataType));
        DFGTEST_EXPECT_LEFT(ChartDataType::dayTimeMillisecond, dataType);

        DFGTEST_EXPECT_LEFT(30, model.cellDataAsDouble(2, 0, &dataType));
        DFGTEST_EXPECT_LEFT(ChartDataType::dayTime, dataType);

        DFGTEST_EXPECT_LEFT(123, model.cellDataAsDouble(3, 0, &dataType, 123));
        DFGTEST_EXPECT_LEFT(ChartDataType::unknown, dataType);
    }
}

TEST(dfgQt, CsvItemModel_defaultInputEncoding)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(qt);
    {
        CsvItemModel modelUtf8Bom;
        CsvItemModel modelUtf8NoBom;
        CsvItemModel modelWin1252;
        modelUtf8Bom.openFile("testfiles/example5_UTF8_BOM.csv");
        modelUtf8NoBom.openFile("testfiles/example5_UTF8_no_BOM.csv");
        modelWin1252.openFile("testfiles/example5_Windows1252.csv");
        DFGTEST_EXPECT_LEFT(QString(QChar(0x20AC)), viewToQString(modelUtf8Bom.rawStringViewAt(0, 0)));
        DFGTEST_EXPECT_LEFT(QString(QChar(0xB5)), viewToQString(modelUtf8Bom.rawStringViewAt(0, 1)));

        DFGTEST_EXPECT_LEFT(QString(QChar(0x20AC)), viewToQString(modelUtf8NoBom.rawStringViewAt(0, 0)));
        DFGTEST_EXPECT_LEFT(QString(QChar(0xB5)), viewToQString(modelUtf8NoBom.rawStringViewAt(0, 1)));

        // When reading Windows-1252 as UTF8, it will contain bytes that are not valid UTF8 so QString is expected to get
        // 0xFFFD == Replacement character (https://en.wikipedia.org/wiki/Specials_(Unicode_block)#Replacement_character)
        DFGTEST_EXPECT_LEFT(QString(QChar(0xFFFD)), viewToQString(modelWin1252.rawStringViewAt(0, 0)));
        DFGTEST_EXPECT_LEFT(QString(QChar(0xFFFD)), viewToQString(modelWin1252.rawStringViewAt(0, 1)));
        DFGTEST_ASSERT_LEFT(1, modelWin1252.rawStringViewAt(0, 0).size());
        DFGTEST_ASSERT_LEFT(1, modelWin1252.rawStringViewAt(0, 0).size());
        DFGTEST_EXPECT_LEFT(0x80, static_cast<unsigned char>(modelWin1252.rawStringViewAt(0, 0).asUntypedView()[0]));
        DFGTEST_EXPECT_LEFT(0xb5, static_cast<unsigned char>(modelWin1252.rawStringViewAt(0, 1).asUntypedView()[0]));

        // Testing that openFromMemory() reads BOM-less UTF8 in the same way as openFile()
        {
            using namespace ::DFG_MODULE_NS(io);
            const auto noBomUtf8Bytes = fileToMemory_readOnly(qStringToFileApi8Bit(modelUtf8NoBom.getFilePath())).asContainer<std::string>();
            CsvItemModel modelFromMemory;
            modelFromMemory.openFromMemory(noBomUtf8Bytes, modelFromMemory.getLoadOptionsForFile(modelUtf8NoBom.getFilePath()));
            DFGTEST_ASSERT_LEFT(modelUtf8NoBom.saveToByteString(), modelFromMemory.saveToByteString());
        }

        // Invalid UTF8 bytes are lost in load-save roundtrip.
        // Below making sure that saving file that had invalid UTF-8 results
        // to valid UTF8, how invalid UTF gets written is unspecified.
        {
            using namespace ::DFG_MODULE_NS(io);
            DFGTEST_EXPECT_LEFT(0, modelWin1252.m_messagesFromLatestSave.size());
            const auto sSavedWin1252Bytes = modelWin1252.saveToByteString();
            DFGTEST_EXPECT_LEFT(2, modelWin1252.m_messagesFromLatestSave.size());
            const auto sSavePath = "testfiles/generated/example5_Windows1252_saved.csv";
            modelWin1252.saveToFile(sSavePath);
            DFGTEST_EXPECT_LEFT(2, modelWin1252.m_messagesFromLatestSave.size());
            const auto sSavedFileBytes = fileToMemory_readOnly(sSavePath).asContainer<std::string>();
            // First checking that saving to file and saveToByteString() produce the same result.
            DFGTEST_EXPECT_EQ(sSavedWin1252Bytes, sSavedFileBytes);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            auto decoder = QStringDecoder(QStringDecoder::Utf8);
            const QString asQString = decoder(sSavedWin1252Bytes.c_str()).operator QString(); // Note: just calling decoder is not enough to get possible errors, needs conversion to QString.
            DFGTEST_ASSERT_FALSE(decoder.hasError());
#else
            QTextCodec::ConverterState state;
            QTextCodec* codec = QTextCodec::codecForName("UTF-8");
            DFGTEST_ASSERT_TRUE(codec != nullptr);
            QString text = codec->toUnicode(sSavedWin1252Bytes.c_str(), saturateCast<int>(sSavedWin1252Bytes.size()), &state);
            DFGTEST_EXPECT_LEFT(0, state.invalidChars);
#endif
        }
    }
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))

#include <QAbstractItemModelTester>

TEST(dfgQt, CsvItemModel_QAbstractItemModelTester)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(qt);

    CsvItemModel model;
    model.openFile("testfiles/example4.csv"); // Arbitrary choice of example file
    QAbstractItemModelTester modelTester(&model, QAbstractItemModelTester::FailureReportingMode::Warning);
    model.setSize(model.rowCount() + 1, model.columnCount() + 1);
    model.setItem(2, 2, "abc");
    model.removeColumn(1);
    model.removeRow(1);
    model.transpose();
    model.insertColumns(1, 2);
    model.insertRows(2, 3);
}

#endif // Qt >= 5.11.0
