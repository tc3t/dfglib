//#include <stdafx.h>
#include <dfg/qt/CsvItemModel.hpp>
#include <dfg/io.hpp>
#include <dfg/io/OmcByteStream.hpp>
#include <dfg/qt/containerUtils.hpp>
#include <dfg/qt/ConsoleDisplay.hpp>
#include <dfg/qt/CsvTableViewChartDataSource.hpp>
#include <dfg/qt/CsvFileDataSource.hpp>
#include <dfg/qt/TableEditor.hpp>
#include <dfg/qt/SQLiteFileDataSource.hpp>
#include <dfg/qt/NumberGeneratorDataSource.hpp>
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
#include <QSpinBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QGridLayout>
#include <QtGlobal>
#include <QThread>
#include <dfg/qt/qxt/gui/qxtspanslider.h>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS


TEST(dfgQt, SpanSlider)
{
    // Simply test that this compiles and links.
    auto pSpanSlider = std::unique_ptr<QxtSpanSlider>(new QxtSpanSlider(Qt::Horizontal));
#if 0 // Enable to test in a GUI.
    auto pLayOut = new QGridLayout();
    auto pEditMinVal = new QSpinBox;
    auto pEditMaxVal = new QSpinBox;

    QObject::connect(pSpanSlider.get(), SIGNAL(lowerValueChanged(int)), pEditMinVal, SLOT(setValue(int)));
    QObject::connect(pSpanSlider.get(), SIGNAL(upperValueChanged(int)), pEditMaxVal, SLOT(setValue(int)));

    pLayOut->addWidget(pSpanSlider.release(), 0, 0, 1, 2); // Takes ownership
    pLayOut->addWidget(pEditMinVal, 1, 0, 1, 1); // Takes ownership
    pLayOut->addWidget(pEditMaxVal, 1, 1, 1, 1); // Takes ownership
    QDialog w;
    w.setLayout(pLayOut); // Takes ownership
    w.exec();
#endif
}

namespace
{
    class QObjectStorageTestWidget : public QWidget
    {
    public:
        QObjectStorageTestWidget()
        {
            m_threads.push_back(new QThread(this));
            m_threads.push_back(new QThread);
        }
    public:
        std::vector<DFG_MODULE_NS(qt)::QObjectStorage<QThread>> m_threads;
    };
}

TEST(dfgQt, QObjectStorage)
{
    // Testing moving of QObject storages
    {
        DFG_MODULE_NS(qt)::QObjectStorage<QDialog> storage0(new QDialog);
        auto pDialog = storage0.get();
        EXPECT_EQ(pDialog, storage0.get());
        DFG_MODULE_NS(qt)::QObjectStorage<QDialog> storage1 = std::move(storage0);
        EXPECT_FALSE(storage0);
        EXPECT_EQ(pDialog, storage1.get());

        // Testing move assignment
        DFG_MODULE_NS(qt)::QObjectStorage<QDialog> storage2;
        storage2 = std::move(storage1);
        EXPECT_FALSE(storage1);
        EXPECT_EQ(pDialog, storage2.get());
    }

    // Testing actual class that has std::vector<QObjectStorage<T>> as member.
    {
        QObjectStorageTestWidget widgetTest;
    }
}

TEST(dfgQt, ConsoleDisplay)
{
    ::DFG_MODULE_NS(qt)::ConsoleDisplay display;
    const size_t nLengthLimit = 40;
    display.lengthInCharactersLimit(nLengthLimit);
    // Note: display adds prefix to entries so not this many entries are really required to go beyond 40 chars.
    display.addEntry("0123456789");
    display.addEntry("0123456789");
    display.addEntry("0123456789");
    display.addEntry("0123456789");
    display.addEntry("0123456789");
    EXPECT_TRUE(display.lengthInCharacters() < nLengthLimit);
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)) // Filter is available only since Qt 5.12
TEST(dfgQt, TableEditor_filter)
{
    using namespace ::DFG_MODULE_NS(qt);
    TableEditor tableEditor;
    ASSERT_TRUE(tableEditor.m_spTableModel != nullptr);
    ASSERT_TRUE(tableEditor.m_spTableView != nullptr);

    // Generating content
    tableEditor.m_spTableModel->openString(R"(,
a,b
c,d
e,f
g,h
)");

    auto& rView = *tableEditor.m_spTableView;
    QPointer<QAbstractItemModel> spViewModel = rView.model();
    ASSERT_TRUE(spViewModel != nullptr);
    EXPECT_EQ(4, spViewModel->rowCount());
    EXPECT_EQ(2, spViewModel->columnCount());

    const char szFilters[] = R"({ "text": "a|c", "apply_columns":"1", "and_group": "a", "type": "reg_exp" } )"
                             R"({ "text": "d", "apply_rows":"1:3", "and_group": "a" } )"
                             R"({ "text": "h", "and_group": "b"})";
    tableEditor.setFilterJson(szFilters);

    ASSERT_TRUE(spViewModel != nullptr);
    auto& rViewModel = *spViewModel;
    EXPECT_EQ(2, rViewModel.rowCount());
    EXPECT_EQ(2, rViewModel.columnCount());
    EXPECT_EQ("c", rViewModel.data(rViewModel.index(0, 0)).toString());
    EXPECT_EQ("d", rViewModel.data(rViewModel.index(0, 1)).toString());
    EXPECT_EQ("g", rViewModel.data(rViewModel.index(1, 0)).toString());
    EXPECT_EQ("h", rViewModel.data(rViewModel.index(1, 1)).toString());
}
#endif // (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))

TEST(dfgQt, TableEditor_selectionDetails)
{
    using namespace ::DFG_MODULE_NS(qt);
    TableEditor tableEditor;

    auto pView = tableEditor.tableView();

    DFGTEST_ASSERT_TRUE(pView != nullptr);
    DFGTEST_ASSERT_TRUE(tableEditor.m_spTableModel != nullptr);

    // Generating content
    tableEditor.m_spTableModel->openString(R"(,
1,5
9,6
a,7
2,8
)");

    auto pDetailPanel = qobject_cast<CsvTableViewBasicSelectionAnalyzerPanel*>(tableEditor.selectionDetailPanel());

    DFGTEST_ASSERT_TRUE(pDetailPanel != nullptr);

    // Adding some custom details
    pDetailPanel->addDetail({ {"id", "test_product" }, {"type", "accumulator"}, {"formula", "acc * value"}, {"initial_value", "1"} } );
    pDetailPanel->addDetail({ {"id", "percentile_33" }, {"type", "percentile"}, {"percentage", "33"} } );
    pDetailPanel->addDetail({ {"id", "percentile_34" }, {"type", "percentile"}, {"percentage", "34"} } );
    pDetailPanel->setEnableStatusForAll(true);

    QString sResult;
    const auto handler = [&](const QString& sResultArg) { sResult = sResultArg; };

    DFG_QT_VERIFY_CONNECT(QObject::connect(pDetailPanel, &CsvTableViewBasicSelectionAnalyzerPanel::sigSetValueDisplayString, pDetailPanel, handler));

    pView->selectColumn(0);

    // Manually running event loop in order to trigger handling of signal sigSetValueDisplayString
    for (int i = 0; i < 10 && (sResult.isEmpty() || !sResult.startsWith("Included")); ++i)
    {
        QCoreApplication::processEvents();
        QThread::msleep(10);
    }
    DFGTEST_ASSERT_TRUE(!sResult.isEmpty());

    // Note: numeric precision in results is subject to change.
    const char szExpectedRegExp[] = R"(Included: 3, Excluded: 1, Sum: 12, Avg: 4, Median: 2, Min: 1, Max: 9, Variance: 12\.666666666666668, StdDev \(pop\): 3\.559026084010437.*, StdDev \(smp\): 4\.358898943540674, Is sorted \(num\): no \(asc for 2 first\), test_product: 18, percentile_33: 1, percentile_34: 2)";
    DFGTEST_EXPECT_LEFT(0, sResult.indexOf(QRegularExpression(szExpectedRegExp)));
}

TEST(dfgQt, TableEditor_filterPanelSettingsFromConfFile)
{
    using namespace ::DFG_MODULE_NS(qt);
    {
        TableEditor tableEditor;
        auto& view = *tableEditor.tableView();
        DFGTEST_EXPECT_TRUE(view.openFile("testfiles/TableEditor_confFile.csv"));

        const auto settings = tableEditor.getFilterPanelSettings();
        DFGTEST_EXPECT_LEFT(R"({"apply_columns":"1","text":"c","type":"reg_exp"})", settings[CsvOptionProperty_filterText].toString());
        DFGTEST_EXPECT_LEFT("0",    settings[CsvOptionProperty_filterCaseSensitive].toString());
        DFGTEST_EXPECT_LEFT("Json", settings[CsvOptionProperty_filterSyntaxType].toString());
        DFGTEST_EXPECT_LEFT("0",    settings[CsvOptionProperty_filterColumn].toString());
    }
}

TEST(dfgQt, TableEditor_findPanelSettingsFromConfFile)
{
    using namespace ::DFG_MODULE_NS(qt);
    TableEditor tableEditor;
    auto& view = *tableEditor.tableView();

    DFGTEST_EXPECT_TRUE(view.openFile("testfiles/TableEditor_confFile.csv"));

    const auto settings = tableEditor.getFindPanelSettings();
    DFGTEST_EXPECT_LEFT("abc", settings[CsvOptionProperty_findText].toString());
    DFGTEST_EXPECT_LEFT("1", settings[CsvOptionProperty_findCaseSensitive].toString());
    DFGTEST_EXPECT_LEFT("Regular expression", settings[CsvOptionProperty_findSyntaxType].toString());
    DFGTEST_EXPECT_LEFT("1", settings[CsvOptionProperty_findColumn].toString());
}

TEST(dfgQt, TableEditor_fileInfo)
{
    using namespace ::DFG_MODULE_NS(qt);
    TableEditor tableEditor;
    auto& view = *tableEditor.tableView();

    {
        DFGTEST_EXPECT_TRUE(view.openFile("testfiles/TableEditor_confFile.csv"));

        const auto fields = tableEditor.getToolTipFileInfoFields();
        DFGTEST_EXPECT_LEFT(12, fields.size()); // If this fails after adding fields, simply adjusts expected count.

        QMap<QString, QString> expected;
        //expected["File path"] = ;
        expected["File size"] = "13 bytes (13 bytes)";
        //expected["Created"] = ;
        //expected["Last modified"] = ;
        expected["Hidden"] = "no";
        expected["Writable"] = "yes";
        expected["File encoding"] = "UTF8";
        expected["Save separator char"] = ",";
        expected["Save enclosing behaviour"] = "If needed";
        expected["Save enclosing char"] = "\"";
        expected["Save EOL"] = "\\n";
        expected["Save encoding"] = "UTF8";

        for (const auto field : fields)
        {
            const auto expectedIter = expected.find(field.first);
            if (expectedIter == expected.end())
                continue;
            DFGTEST_EXPECT_LEFT(expectedIter.value(), field.second);
            expected.erase(expectedIter);
        }
        DFGTEST_EXPECT_TRUE(expected.empty()); // If this fails, not all expected fields were found.
    }

    // TODO: add more cases (symbolic links, disabled enclosing etc.)
}

TEST(dfgQt, CsvTableViewBasicSelectionAnalyzerPanel_basicCustomDetailHandling)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvTableViewBasicSelectionAnalyzerPanel panel;

    DFGTEST_EXPECT_FALSE(panel.addDetail( { {"type", "accumulator"}, {"formula", "acc + value"} } )); // Missing initial value
    DFGTEST_EXPECT_FALSE(panel.addDetail( { {"type", "accumulator"}, {"initial_value", 0} } )); // Missing formula string
    DFGTEST_EXPECT_FALSE(panel.addDetail( { {"formula", "acc + value"}, {"initial_value", 0} } )); // Missing type
    DFGTEST_EXPECT_FALSE(panel.addDetail( { {"type", "unknown_type"}, {"formula", "acc + value"}, {"initial_value", 0} } )); // Bad type

    DFGTEST_EXPECT_TRUE(panel.addDetail( { {"id", "test_sum" }, {"type", "accumulator"}, {"formula", "acc + value"}, {"initial_value", "0"} } ));
    DFGTEST_EXPECT_TRUE(panel.addDetail( { {"id", "test_product" }, {"type", "accumulator"}, {"formula", "acc * value"}, {"initial_value", "1"} } ));
    DFGTEST_EXPECT_TRUE(panel.addDetail( { {"type", "accumulator"}, {"formula", "acc + value"}, {"initial_value", 0} } )); // Tests that can add without id

    auto spCollectors = panel.collectors();
    DFGTEST_ASSERT_TRUE(spCollectors != nullptr);

    spCollectors->updateAll(2);
    spCollectors->updateAll(3);

    const auto getCollector = [&](const char* psz)
    {
        auto p = spCollectors->find(psz);
        DFGTEST_EXPECT_TRUE(p != nullptr);
        return p;
    };

    auto pSumCollector = getCollector("test_sum");
    auto pProductCollector = getCollector("test_product");

    if (pSumCollector)
    {
        DFGTEST_EXPECT_EQ(5, pSumCollector->value());
    }

    if (pProductCollector)
    {
        DFGTEST_EXPECT_EQ(6, pProductCollector->value());
    }
}

TEST(dfgQt, qstringUtils)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(qt);
    EXPECT_EQ(QString("abc"), viewToQString(DFG_UTF8("abc")));
    EXPECT_EQ(QString("abc"), untypedViewToQStringAsUtf8("abc"));
    EXPECT_EQ(DFG_UTF8("abc"), qStringToStringUtf8(QString("abc")));
}

TEST(dfgQt, StringMatchDefinition)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(qt);
#define DFGTEST_TEMP_VERIFY2(JSON_STR, EXPECTED_STR, EXPECTED_CASE, EXPECTED_SYNTAX, EXPECTED_NEGATE) \
    { \
        const auto smd = StringMatchDefinition::fromJson(DFG_UTF8(JSON_STR)).first; \
        DFGTEST_EXPECT_LEFT(EXPECTED_STR, smd.matchString()); \
        DFGTEST_EXPECT_LEFT(EXPECTED_CASE, smd.caseSensitivity()); \
        DFGTEST_EXPECT_LEFT(EXPECTED_SYNTAX, smd.patternSyntax()); \
        DFGTEST_EXPECT_LEFT(EXPECTED_NEGATE, smd.isNegated()); \
    }

#define DFGTEST_TEMP_VERIFY(JSON_STR, EXPECTED_STR, EXPECTED_CASE, EXPECTED_SYNTAX) \
    DFGTEST_TEMP_VERIFY2(JSON_STR, EXPECTED_STR, EXPECTED_CASE, EXPECTED_SYNTAX, false)

    DFGTEST_TEMP_VERIFY("non_json", "*", Qt::CaseInsensitive, PatternMatcher::Wildcard);
    DFGTEST_TEMP_VERIFY("{}", "", Qt::CaseInsensitive, PatternMatcher::Wildcard);
    DFGTEST_TEMP_VERIFY(R"( { "text": "abc" } )", "abc", Qt::CaseInsensitive, PatternMatcher::Wildcard);
    DFGTEST_TEMP_VERIFY(R"( { "case_sensitive": true } )", "", Qt::CaseSensitive, PatternMatcher::Wildcard);
    DFGTEST_TEMP_VERIFY(R"( { "type": "wildcard" } )", "", Qt::CaseInsensitive, PatternMatcher::Wildcard);
    DFGTEST_TEMP_VERIFY(R"( { "type": "fixed" } )", "", Qt::CaseInsensitive, PatternMatcher::FixedString);
    DFGTEST_TEMP_VERIFY(R"( { "type": "reg_exp" } )", "", Qt::CaseInsensitive, PatternMatcher::RegExp);
    DFGTEST_TEMP_VERIFY(R"( { "text": "a|B", "case_sensitive":true, "type": "reg_exp" } )", "a|B", Qt::CaseSensitive, PatternMatcher::RegExp);
    DFGTEST_TEMP_VERIFY2(R"( { "negate": true } )", "", Qt::CaseInsensitive, PatternMatcher::Wildcard, true);

#undef DFGTEST_TEMP_VERIFY
#undef DFGTEST_TEMP_VERIFY2

    // Tests with Wildcard
    {
        EXPECT_FALSE(StringMatchDefinition("A", Qt::CaseSensitive, PatternMatcher::Wildcard).isMatchWith(DFG_UTF8("abc")));
        EXPECT_FALSE(StringMatchDefinition("A", Qt::CaseSensitive, PatternMatcher::Wildcard).isMatchWith(QString("abc")));
        EXPECT_TRUE(StringMatchDefinition("A", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(DFG_UTF8("abc")));
        EXPECT_TRUE(StringMatchDefinition("A", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(QString("abc")));

        // Testing that characters that are special in regular expression but not in wildcard are handled correctly.
        EXPECT_TRUE(StringMatchDefinition("+{()}", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(QString("a+{()}b")));
        EXPECT_FALSE(StringMatchDefinition("+{()}", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(QString("a+{(}b")));

        EXPECT_TRUE(StringMatchDefinition("a*b[cde]f?g", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(QString("a1234qwbef+ghi")));
        EXPECT_FALSE(StringMatchDefinition("a*b[cde]f?g", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(QString("a1234qwbff+ghi")));
    }

    // Tests with FixedString
    {
        EXPECT_FALSE(StringMatchDefinition("A", Qt::CaseSensitive, PatternMatcher::FixedString).isMatchWith(DFG_UTF8("abc")));
        EXPECT_FALSE(StringMatchDefinition("A", Qt::CaseSensitive, PatternMatcher::FixedString).isMatchWith(QString("abc")));
        EXPECT_TRUE(StringMatchDefinition("A", Qt::CaseInsensitive, PatternMatcher::FixedString).isMatchWith(DFG_UTF8("abc")));
        EXPECT_TRUE(StringMatchDefinition("A", Qt::CaseInsensitive, PatternMatcher::FixedString).isMatchWith(QString("abc")));

        // Testing that characters that are special in wildcard and regular expression are handled correctly.
        EXPECT_TRUE(StringMatchDefinition("*", Qt::CaseInsensitive, PatternMatcher::FixedString).isMatchWith(QString("a?*b")));
        EXPECT_FALSE(StringMatchDefinition("*", Qt::CaseInsensitive, PatternMatcher::FixedString).isMatchWith(QString("a?b")));
    }

    // negate-tests
    {
        DFGTEST_EXPECT_TRUE( StringMatchDefinition("abc", Qt::CaseInsensitive, PatternMatcher::FixedString, false).isMatchWith(QString("abc")));
        DFGTEST_EXPECT_FALSE(StringMatchDefinition("abc", Qt::CaseInsensitive, PatternMatcher::FixedString, true).isMatchWith(QString("abc")));
    }
}

namespace
{
template <class Source_T>
static void testFileDataSource(const QString& sExtension,
                               std::function<std::unique_ptr<Source_T> (QString, QString)> sourceCreator,
                               std::function<bool (QString, ::DFG_MODULE_NS(qt)::CsvItemModel&)> fileCreator,
                               std::function<bool (QString, ::DFG_MODULE_NS(qt)::CsvItemModel&)> editer)
{
    using namespace ::DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(qt);

    const QString sTestFilePath("testfiles/generated/tempFileDataSourceTest." + sExtension);
    QFile::remove(sTestFilePath);

    {
        CsvItemModel model;
        model.openString("Col0,Col1,123.456,0.125\nab,b,18.9.2020,1.2\nb,a\xE2\x82\xAC" "b,2020-09-18,-2.25\nab,c,2020-09-18 12:00:00,3.5");
        ASSERT_TRUE(fileCreator(sTestFilePath, model));
    }

    const size_t nColCount = 4;
    std::array<std::vector<double>, nColCount> expectedRows = { std::vector<double>({0, 1, 2, 3}), {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3} };
    std::array<std::vector<std::string>, nColCount> expectedStrings = { std::vector<std::string>(
                                                                {"Col0", "ab", "b", "ab"}),
                                                                {"Col1", "b", "a\xE2\x82\xAC" "b", "c"}, // \xE2\x82\xAC is eurosign in UTF-8
                                                                {"123.456", "18.9.2020", "2020-09-18", "2020-09-18 12:00:00"},
                                                                {"0.125", "1.2", "-2.25", "3.5"}
                                                                };

    // csv source includes header on row 0, SQLiteSource doesn't.
    const bool bIncludeHeaderInComparisons = (sExtension == "csv");
    
    if (!bIncludeHeaderInComparisons)
    {
        ::DFG_MODULE_NS(alg)::forEachFwd(expectedRows,    [](std::vector<double>& v)      { v.erase(v.begin()); });
        ::DFG_MODULE_NS(alg)::forEachFwd(expectedStrings, [](std::vector<std::string>& v) { v.erase(v.begin()); });
    }
    const std::array<ChartDataType, nColCount> expectedColumnDataTypes = {ChartDataType::unknown, ChartDataType::unknown, ChartDataType::dateAndTime };
    std::array<std::vector<double>, nColCount> expectedValues;
    std::transform(expectedStrings[2].begin(),
                   expectedStrings[2].end(),
                   std::back_inserter(expectedValues[2]), [](const std::string& s) { return GraphDataSource::cellStringToDouble(SzPtrUtf8(s.c_str())); } );

    for (DataSourceIndex c = 0; c < nColCount + 1; ++c)
    {
        std::vector<double> rows;
        std::vector<std::string> strings;
        std::vector<double> values;
        auto spSource = sourceCreator(sTestFilePath, "csvSource");
        auto& source = *spSource;
        source.forEachElement_byColumn(c, DataQueryDetails(DataQueryDetails::DataMaskAll), [&](const SourceDataSpan& sourceSpan)
        {
            rows.insert(rows.end(), sourceSpan.rows().asSpan().begin(), sourceSpan.rows().asSpan().end());
            const auto nOldSize = static_cast<uint16>(strings.size());
            strings.resize(nOldSize + sourceSpan.stringViews().size());
            std::transform(sourceSpan.stringViews().begin(), sourceSpan.stringViews().end(), strings.begin() + nOldSize, [](const StringViewUtf8& sv) { return sv.toString().rawStorage(); });
            values.insert(values.end(), sourceSpan.doubles().begin(), sourceSpan.doubles().end());
        });
        if (c < expectedRows.size())
        {
            ASSERT_EQ(expectedRows.size(), expectedStrings.size());
            EXPECT_EQ(expectedRows[c], rows);
            EXPECT_EQ(expectedStrings[c], strings);
            if (!expectedValues[c].empty())
            {
                EXPECT_EQ(expectedValues[c].size(), values.size());
                if (expectedValues[c].size() == values.size())
                {
                    EXPECT_TRUE(std::equal(values.begin(), values.end(), expectedValues[c].begin()));
                }
            }
        }
        else
        {
            EXPECT_TRUE(rows.empty());
            EXPECT_TRUE(strings.empty());
        }
        if (isValidIndex(expectedColumnDataTypes, c))
        {
            EXPECT_EQ(expectedColumnDataTypes[c], source.columnDataType(c));
        }
    }

    // Testing that data mask is taken into account
    const DataQueryDetails::DataMask masks[] = { DataQueryDetails::DataMaskRows, DataQueryDetails::DataMaskNumerics, DataQueryDetails::DataMaskStrings, DataQueryDetails::DataMaskRowsAndNumerics, DataQueryDetails::DataMaskRowsAndStrings };
    for (const auto& mask : masks)
    {
        const DataSourceIndex nCol = 2;
        std::vector<double> rows;
        std::vector<std::string> strings;
        std::vector<double> values;
        auto spSource = sourceCreator(sTestFilePath, "csvSource");
        auto& source = *spSource;
        source.forEachElement_byColumn(nCol, DataQueryDetails(mask), [&](const SourceDataSpan& sourceSpan)
        {
            rows.insert(rows.end(), sourceSpan.rows().asSpan().begin(), sourceSpan.rows().asSpan().end());
            const auto nOldSize = static_cast<uint16>(strings.size());
            strings.resize(nOldSize + sourceSpan.stringViews().size());
            std::transform(sourceSpan.stringViews().begin(), sourceSpan.stringViews().end(), strings.begin() + nOldSize, [](const StringViewUtf8& sv) { return sv.toString().rawStorage(); });
            values.insert(values.end(), sourceSpan.doubles().begin(), sourceSpan.doubles().end());
        });
        if (mask & DataQueryDetails::DataMaskRows)
            EXPECT_EQ(expectedRows[nCol], rows);
        else
            EXPECT_TRUE(rows.empty());

        if (mask & DataQueryDetails::DataMaskNumerics)
            EXPECT_EQ(expectedValues[nCol], values);
        else
            EXPECT_TRUE(values.empty());

        if (mask & DataQueryDetails::DataMaskStrings)
            EXPECT_EQ(expectedStrings[nCol], strings);
        else
            EXPECT_TRUE(strings.empty());
    }

    // Testing fetchColumnNumberData
    {
        auto spSource = sourceCreator(sTestFilePath, "csvSource");
        using DataPipe = GraphDataSourceDataPipe_MapVectorSoADoubleValueVector;
        DataPipe::RowToValueMap valueMapFetched;
        DataPipe::RowToValueMap valueMapForEach;
        valueMapForEach.setSorting(false);
        const auto queryDetails = DataQueryDetails(DataQueryDetails::DataMaskRowsAndNumerics);
        const auto nColumn = 3;
        DataPipe dataPipe(&valueMapFetched);
        spSource->fetchColumnNumberData(dataPipe, nColumn, queryDetails);
        spSource->forEachElement_byColumn(nColumn, queryDetails, [&](const SourceDataSpan& data)
        {
            valueMapForEach.pushBackToUnsorted(data.rows().asSpan(), data.doubles());
        });
        valueMapForEach.setSorting(true);
        EXPECT_EQ(valueMapFetched.m_keyStorage, valueMapForEach.m_keyStorage);
        EXPECT_EQ(valueMapFetched.m_valueStorage, valueMapForEach.m_valueStorage);
        EXPECT_TRUE(std::equal(expectedStrings[3].begin(), expectedStrings[3].end(), valueMapFetched.beginValue(),
                               [](const std::string& s, const double d) { return ::DFG_MODULE_NS(str)::strTo<double>(s) == d; }));
    }

    // Testing that change signaling works
    {
        auto spSource = sourceCreator(sTestFilePath, "csvSource");
        auto& source = *spSource;
        bool bChangeNotificationReceived = false;
        DFG_QT_VERIFY_CONNECT(QObject::connect(&source, &CsvFileDataSource::sigChanged, [&]() { bChangeNotificationReceived = true;}));
        {
            CsvItemModel model;
            model.openString("a,b\n1,2");
            EXPECT_TRUE(editer(sTestFilePath, model));
        }

        for (int i = 0; i < 10 && !bChangeNotificationReceived; ++i)
        {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
        // Checking that change notification was received and that new columns are effective.
        EXPECT_TRUE(bChangeNotificationReceived);
        ASSERT_EQ(2u, source.columnNames().size());
        EXPECT_EQ("a", source.columnNames()[0u]);
        EXPECT_EQ("b", source.columnNames()[1u]);
        std::vector<std::string> elems;
        source.forEachElement_byColumn(0, DataQueryDetails(DataQueryDetails::DataMaskAll), [&](const SourceDataSpan& sourceSpan)
        {
            std::transform(sourceSpan.stringViews().begin(), sourceSpan.stringViews().end(), std::back_inserter(elems), [](const StringViewUtf8& sv) { return sv.toString().rawStorage(); });
        });
        source.forEachElement_byColumn(1, DataQueryDetails(DataQueryDetails::DataMaskAll), [&](const SourceDataSpan& sourceSpan)
        {
            std::transform(sourceSpan.stringViews().begin(), sourceSpan.stringViews().end(), std::back_inserter(elems), [](const StringViewUtf8& sv) { return sv.toString().rawStorage(); });
        });
        if (bIncludeHeaderInComparisons)
            EXPECT_EQ(std::vector<std::string>({"a", "1", "b", "2"}), elems);
        else
            EXPECT_EQ(std::vector<std::string>({ "1", "2" }), elems);
    }

    QFile::remove(sTestFilePath);
}
}

TEST(dfgQt, CsvFileDataSource)
{
    using namespace ::DFG_MODULE_NS(qt);
    auto sourceCreator = [](const QString& sPath, const QString& sId) { return std::unique_ptr<CsvFileDataSource>(new CsvFileDataSource(sPath, sId)); };
    auto fileCreator = [](QString sPath, CsvItemModel& model) { return model.saveToFile(sPath); };
    testFileDataSource<CsvFileDataSource>("csv", sourceCreator, fileCreator, fileCreator);
}

TEST(dfgQt, SQLiteFileDataSource)
{
    using namespace ::DFG_MODULE_NS(qt);
    auto sourceCreator = [](const QString& sPath, const QString& sId) { return std::unique_ptr<SQLiteFileDataSource>(new SQLiteFileDataSource("SELECT * FROM table_from_csv", sPath, sId)); };
    auto fileCreator = [](QString sPath, CsvItemModel& model) { QFile::remove(sPath); return model.exportAsSQLiteFile(sPath); };
    testFileDataSource<SQLiteFileDataSource>("sqlite3", sourceCreator, fileCreator, fileCreator);
}

TEST(dfgQt, NumberGeneratorDataSource)
{
    using namespace ::DFG_MODULE_NS(qt);
    using namespace ::DFG_MODULE_NS(cont);

    // Basic test
    {
        NumberGeneratorDataSource ds("test", 5);
        ValueVector<double> vals;
        ds.forEachElement_byColumn(0, DataQueryDetails(DataQueryDetails::DataMaskAll), [&](const SourceDataSpan& dataSpan)
        {
            EXPECT_TRUE(dataSpan.rows().empty()); // Rows are not expected to be present.
            auto doubleSpan = dataSpan.doubles();
            vals.insert(vals.end(), doubleSpan.begin(), doubleSpan.end());
        });
        EXPECT_EQ(ValueVector<double>({0, 1, 2, 3, 4}), vals);
    }

    const auto storeValuesFromSource = [](NumberGeneratorDataSource& ds)
    {
        ValueVector<double> vals;
        ds.forEachElement_byColumn(0, DataQueryDetails(DataQueryDetails::DataMaskAll), [&](const SourceDataSpan& dataSpan)
        {
            auto doubleSpan = dataSpan.doubles();
            vals.insert(vals.end(), doubleSpan.begin(), doubleSpan.end());
        });
        return vals;
    };

    // Testing data more than block size.
    {
        const DataSourceIndex nRowCount = 5;
        NumberGeneratorDataSource ds("test", nRowCount, 3);
        const auto vals = storeValuesFromSource(ds);
        EXPECT_EQ(ValueVector<double>({0, 1, 2, 3, 4}), vals);
    }

    // createByCountFirstStep
    {
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstStep("test", 0, 1, 2);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_TRUE(storeValuesFromSource(*spDs).empty());
        }
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstStep("test", 1, 3, 5);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({3}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstStep("test", 3, 10, -20);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({10, -10, -30}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstStep("test", 3, 10, 0);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({10, 10, 10}), storeValuesFromSource(*spDs));
        }
    }

    // createByCountFirstLast
    {
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstLast("test", 0, 1, 2);
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstLast("test", 1, 3, 3);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({3}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstLast("test", 1, 3, 5);
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstLast("test", 2, 3, 5);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({3, 5}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstLast("test", 5, 6, 8);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({6, 6.5, 7, 7.5, 8}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstLast("test", 5, 8, 6);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({8, 7.5, 7, 6.5, 6}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstLast("test", 1, 3, std::numeric_limits<double>::quiet_NaN());
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumberGeneratorDataSource::createByCountFirstLast("test", 1, std::numeric_limits<double>::quiet_NaN(), 3);
            ASSERT_TRUE(spDs == nullptr);
        }
    }

    // createByfirstLastStep
    {
        {
            auto spDs = NumberGeneratorDataSource::createByfirstLastStep("test", 0, 1, 0);
            ASSERT_TRUE(spDs == nullptr);
        }

        {
            auto spDs = NumberGeneratorDataSource::createByfirstLastStep("test", 0, 1, 1);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({0, 1}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumberGeneratorDataSource::createByfirstLastStep("test", 1, 3, 4);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({1}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumberGeneratorDataSource::createByfirstLastStep("test", 1, 3, 0.75);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({1, 1.75, 2.5}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumberGeneratorDataSource::createByfirstLastStep("test", 1, 3, -1);
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumberGeneratorDataSource::createByfirstLastStep("test", 3, 1, 1);
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumberGeneratorDataSource::createByfirstLastStep("test", 1, 3, std::numeric_limits<double>::quiet_NaN());
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumberGeneratorDataSource::createByfirstLastStep("test", 1, std::numeric_limits<double>::quiet_NaN(), 3);
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumberGeneratorDataSource::createByfirstLastStep("test", 1, 2, std::numeric_limits<double>::quiet_NaN());
            ASSERT_TRUE(spDs == nullptr);
        }
    }

    // Testing query mask handling: handler is not expected to get called if numbers are not requested.
    {
        NumberGeneratorDataSource ds("test", 10);
        bool bCallbackCalled = false;
        ds.forEachElement_byColumn(0, DataQueryDetails(DataQueryDetails::DataMaskRowsAndStrings), [&](const SourceDataSpan&)
        {
            bCallbackCalled = true;
        });
        EXPECT_FALSE(bCallbackCalled);
    }

    // Checking that forEachElement_byColumn() and fetchColumnNumberData() produce the same data.
    {
        using namespace DFG_ROOT_NS;
        using DataPipe = GraphDataSourceDataPipe_MapVectorSoADoubleValueVector;
        DataPipe::RowToValueMap valueMapFetch;
        DataPipe::RowToValueMap valueMapForEach;
        valueMapForEach.setSorting(false);

        NumberGeneratorDataSource ds("test", 10, 2);
        DataPipe dataPipe(&valueMapFetch);
        ds.fetchColumnNumberData(dataPipe, 0, DataQueryDetails(DataQueryDetails::DataMaskRowsAndNumerics));
        ds.forEachElement_byColumn(0, DataQueryDetails(DataQueryDetails::DataMaskRowsAndNumerics), [&](const SourceDataSpan& data)
        {
            const auto nStartPos = static_cast<uint32>(valueMapForEach.size());
            valueMapForEach.pushBackToUnsorted(indexRangeIE(nStartPos, static_cast<uint32>(nStartPos + data.doubles().size())), data.doubles());
        });
        valueMapForEach.setSorting(true);
        valueMapFetch.sort();
        EXPECT_EQ(valueMapFetch.m_valueStorage, valueMapForEach.m_valueStorage);
    }
}

TEST(dfgQt, SQLiteDatabase)
{
    using namespace ::DFG_MODULE_NS(sql);

    EXPECT_TRUE(SQLiteDatabase::defaultConnectOptionsForWriting().isEmpty()); // Implementation detail, feel free to change is implementation changes.

    const QString sTestFilePath = "testfiles/generated/sqliteDatabase_test_file_63585.sqlite3";
    EXPECT_TRUE(!QFile::exists(sTestFilePath) || QFile::remove(sTestFilePath));

    // Testing that non-existing file flag won't be created by constructor when using read-only connect option.
    {
        SQLiteDatabase db(sTestFilePath);
        EXPECT_FALSE(db.isOpen());
    }

    // Testing custom connect options
    {
        SQLiteDatabase db(sTestFilePath, "testing");
        EXPECT_EQ("testing", db.connectOptions());
    }

    QMap<QString, QStringList> tableNameToColumnNames;
    tableNameToColumnNames["first_test_table"] = QStringList() << "Column0" << "Column1";
    tableNameToColumnNames["second_test_table"] = QStringList() << "id";
    const int nFirstTestTableRowCount = 10;

    const auto createTestContent = [](int r, int c) { return QString("%1,%2").arg(r).arg(c); };

    // Creating tables to test database
    {
        SQLiteDatabase db(sTestFilePath, SQLiteDatabase::defaultConnectOptionsForWriting());
        EXPECT_TRUE(db.isOpen());
        EXPECT_EQ(SQLiteDatabase::defaultConnectOptionsForWriting(), db.connectOptions());

        EXPECT_TRUE(db.transaction());
        auto statement = db.createQuery();
        const QString sCreateStatement = QString("CREATE TABLE %1 ('%2' TEXT, '%3' TEXT)").arg(tableNameToColumnNames.firstKey(), tableNameToColumnNames.first()[0], tableNameToColumnNames.first()[1]);
        EXPECT_FALSE(SQLiteDatabase::isSelectQuery(sCreateStatement));
        EXPECT_TRUE(statement.prepare(sCreateStatement));
        EXPECT_TRUE(statement.exec());
        EXPECT_TRUE(statement.exec(QString("CREATE TABLE %1 (id TEXT)").arg(tableNameToColumnNames.keys()[1])));

        const QString sInsertStatement = QString("INSERT INTO %1 VALUES(?, ?)").arg(tableNameToColumnNames.firstKey());
        EXPECT_TRUE(statement.prepare(sInsertStatement));
        EXPECT_FALSE(SQLiteDatabase::isSelectQuery(sInsertStatement));
        for (int r = 0; r < nFirstTestTableRowCount; ++r)
        {
            statement.addBindValue(createTestContent(r, 0));
            statement.addBindValue(createTestContent(r, 1));
            EXPECT_TRUE(statement.exec());
        }
        EXPECT_TRUE(db.commit());
        EXPECT_TRUE(db.isOpen());
        db.close();
        EXPECT_FALSE(db.isOpen());
    }

    // Reading from new test database
    {
        SQLiteDatabase db(sTestFilePath);
        EXPECT_TRUE(db.isOpen());
        EXPECT_EQ("QSQLITE_OPEN_READONLY", db.connectOptions()); // Implementation detail, feel free to change is implementation changes.

        EXPECT_EQ(tableNameToColumnNames.keys(), db.tableNames());

        auto query = db.createQuery();
        const QString sQuery = QString("SELECT * FROM %1;").arg(tableNameToColumnNames.firstKey());
        EXPECT_TRUE(db.isSelectQuery(sQuery));
        EXPECT_TRUE(query.prepare(sQuery));
        EXPECT_TRUE(query.exec());

        auto rec = query.record();
        const auto nColCount = rec.count();
        EXPECT_EQ(2, nColCount);

        EXPECT_EQ(tableNameToColumnNames.first()[0], rec.fieldName(0));
        EXPECT_EQ(tableNameToColumnNames.first()[1], rec.fieldName(1));

        // Reading records and filling table.
        size_t nRowCount = 0;
        for (int r = 0; query.next(); ++r, ++nRowCount)
        {
            for (int c = 0; c < nColCount; ++c)
                EXPECT_EQ(createTestContent(r, c), query.value(c).toString());
        }
        EXPECT_EQ(static_cast<size_t>(nFirstTestTableRowCount), nRowCount);
    }

    // Testing static getters
    {
        // isSQLiteFileExtension()
        EXPECT_TRUE(SQLiteDatabase::isSQLiteFileExtension("db"));
        EXPECT_TRUE(SQLiteDatabase::isSQLiteFileExtension("sqlite"));
        EXPECT_TRUE(SQLiteDatabase::isSQLiteFileExtension("sqlite3"));
        EXPECT_FALSE(SQLiteDatabase::isSQLiteFileExtension("sqlitee"));

        // isSQLiteFile()
        {
            const QString sCustomExtensionPath = sTestFilePath + "custom_ext";
            EXPECT_TRUE(QFile::copy(sTestFilePath, sCustomExtensionPath));
            EXPECT_TRUE(QFileInfo::exists(sCustomExtensionPath));
            EXPECT_TRUE(SQLiteDatabase::isSQLiteFile(sTestFilePath));
            EXPECT_TRUE(SQLiteDatabase::isSQLiteFile(sCustomExtensionPath));
            EXPECT_FALSE(SQLiteDatabase::isSQLiteFile(sCustomExtensionPath, true));
            EXPECT_FALSE(SQLiteDatabase::isSQLiteFile("testfiles/test_5x5_sep1F_content_row_comma_column.csv"));
            EXPECT_FALSE(SQLiteDatabase::isSQLiteFile("testfiles/test_5x5_sep1F_content_row_comma_column.csv", true));
            EXPECT_TRUE(QFile::remove(sCustomExtensionPath));
        }

        // getSQLiteFileTableNames()
        EXPECT_EQ(tableNameToColumnNames.keys(), SQLiteDatabase::getSQLiteFileTableNames(sTestFilePath));
        EXPECT_TRUE(SQLiteDatabase::getSQLiteFileTableNames("testfiles/test_5x5_sep1F_content_row_comma_column.csv").isEmpty());

        // getSQLiteFileTableColumnNames
        EXPECT_EQ(tableNameToColumnNames.values()[0], SQLiteDatabase::getSQLiteFileTableColumnNames(sTestFilePath, tableNameToColumnNames.keys()[0]));
        EXPECT_EQ(tableNameToColumnNames.values()[1], SQLiteDatabase::getSQLiteFileTableColumnNames(sTestFilePath, tableNameToColumnNames.keys()[1]));
    }

    EXPECT_TRUE(QFile::remove(sTestFilePath));
}

TEST(dfgQt, qStringToStringUtf8)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(qt);
    QString s;
    for (uint16 i = 32; i < 260; ++i)
        s.push_back(QChar(i));
    s.push_back(QChar(0x20AC)); // Euro-sign
    const char32_t u32 = 123456; // Arbitrary codepoint that requires surrogates.
    s.append(QString::fromUcs4(&u32, 1));
    const auto s8 = qStringToStringUtf8(s);
    const QString sRoundTrip = QString::fromUtf8(s8.beginRaw(), saturateCast<int>(s8.sizeInRawUnits()));
    EXPECT_EQ(s, sRoundTrip);
}

TEST(dfgQt, DataQueryDetails_maskHandling)
{
    using namespace DFG_MODULE_NS(qt);
    {
        DFGTEST_EXPECT_TRUE(DataQueryDetails(DataQueryDetails::DataMaskRows).areOnlyRowsOrNumbersRequested());
        DFGTEST_EXPECT_TRUE(DataQueryDetails(DataQueryDetails::DataMaskNumerics).areOnlyRowsOrNumbersRequested());
        DFGTEST_EXPECT_TRUE(DataQueryDetails(DataQueryDetails::DataMaskRows | DataQueryDetails::DataMaskNumerics).areOnlyRowsOrNumbersRequested());

        DFGTEST_EXPECT_FALSE(DataQueryDetails(DataQueryDetails::DataMaskAll).areOnlyRowsOrNumbersRequested());
        DFGTEST_EXPECT_FALSE(DataQueryDetails(DataQueryDetails::DataMaskStrings).areOnlyRowsOrNumbersRequested());
        DFGTEST_EXPECT_FALSE(DataQueryDetails(DataQueryDetails::DataMaskRows | DataQueryDetails::DataMaskStrings).areOnlyRowsOrNumbersRequested());
        DFGTEST_EXPECT_FALSE(DataQueryDetails(DataQueryDetails::DataMaskNumerics | DataQueryDetails::DataMaskStrings).areOnlyRowsOrNumbersRequested());
    }
}
