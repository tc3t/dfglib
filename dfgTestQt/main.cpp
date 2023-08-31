#include <dfg/dfgDefs.hpp>
#include <dfg/os.hpp>
#include <dfgTest/dfgTest.hpp>
#include <iostream>
#include "dfgTestQt_gtestPrinters.hpp"

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include <gtest/gtest.h>
    #include <QApplication>
    #include <QModelIndex>
    #include <QtMessageHandler>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

#include <dfg/build/buildTimeDetails.hpp> // Note: this must be included after Qt-header in order to have BuildTimeDetail_qtVersion available.


void PrintTo(const QString& s, ::std::ostream* pOstrm)
{
    if (pOstrm)
        *pOstrm << '"' << qUtf8Printable(s) << '"';
}

void PrintTo(const QModelIndex& index, ::std::ostream* pOstrm)
{
    if (pOstrm)
        *pOstrm << QString("QModelIndex(%1, %2, 0x%3)").arg(index.row()).arg(index.column()).arg(reinterpret_cast<uintptr_t>(index.model()), 0, 16).toStdString();
}

namespace
{
    // Sets working directory so that tests using relative paths will find files.
    bool setWorkingDirectory()
    {
        using namespace DFG_MODULE_NS(os);
        const auto cwd = getCurrentWorkingDirectoryC();

        std::string modifier;
        if (isPathFileAvailable(cwd + "/testfiles/example4.csv", FileModeExists))
            modifier = ".";
        else if (isPathFileAvailable(cwd + "/dfgTestQt/testfiles/example4.csv", FileModeExists))
            modifier = "/dfgTestQt/";
        else if (isPathFileAvailable(cwd + "/../dfgTestQt/testfiles/example4.csv", FileModeExists))
            modifier = "/../dfgTestQt/";
        else if (isPathFileAvailable(cwd + "/../../dfgTestQt/testfiles/example4.csv", FileModeExists))
            modifier = "/../../dfgTestQt/";
        else if (isPathFileAvailable(cwd + "/../../../dfgTestQt/testfiles/example4.csv", FileModeExists))
            modifier = "/../../../dfgTestQt/";
        else if (isPathFileAvailable(cwd + "/../../../../dfgTestQt/testfiles/example4.csv", FileModeExists))
            modifier = "/../../../../dfgTestQt/";
        if (!modifier.empty() && modifier != ".")
        {
            const auto newCwd = cwd + modifier;
            setCurrentWorkingDirectory(newCwd);
            GTEST_LOG_(INFO) << "Adjusted current working directory, now: " << getCurrentWorkingDirectoryC();
        }
        return isPathFileAvailable(getCurrentWorkingDirectoryC() + "/testfiles/example4.csv", FileModeExists);
    }
}

void qtMessageHandler(const QtMsgType msgType, const QMessageLogContext& logContext, const QString& msg)
{
    using namespace DFG_ROOT_NS;
    if (msgType == QtMsgType::QtWarningMsg && logContext.category == StringViewC("qt.modeltest"))
        DFGTEST_EXPECT_LEFT("", QString("Qt message handler: failure '%1'").arg(msg).toStdString());
    else
        DFGTEST_MESSAGE("Qt message handler: " << msg.toStdString());
}


int main(int argc, char **argv)
{
    if (!setWorkingDirectory())
    {
        GTEST_LOG_(ERROR) << "Didn't find input file path, tests won't run. Working directory is " << DFG_MODULE_NS(os)::getCurrentWorkingDirectoryC();
        return 1;
    }

	QApplication app(argc, argv);

    // Setting up custom message handler to convert failures reported by
    // testcase dfgQt.CsvItemModel_QAbstractItemModelTester into Google Test failures.
    qInstallMessageHandler(qtMessageHandler);

	::testing::InitGoogleTest(&argc, argv);

    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvItemModel";
    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvTableView_generateContentByFormula_cellValue";
    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvTableView_generateContentByFormula_cellValue_dateHandling";
    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvItemModel_QAbstractItemModelTester";
    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvItemModel_readFormatUsageOnWrite";
    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvTableView_evaluateSelectionAsFormula";
    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvTableView_filterFromSelection";
    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvTableView_regexFormat";
    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvTableView_saveAsShown";
    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvTableView_stringToDouble";
    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvTableView_trimCells";
    //::testing::GTEST_FLAG(filter) = "dfgQt.NumericGeneratorDataSource";
    //::testing::GTEST_FLAG(filter) = "dfgQt.TableEditor_filter";
    //::testing::GTEST_FLAG(filter) = "dfgQt.TableView_setSelectedIndexes";

    const auto rv = RUN_ALL_TESTS();
    {
        using namespace ::DFG_ROOT_NS;
        std::cout << "Done running tests build " << getBuildTimeDetailStr<BuildTimeDetail_dateTime>()
            << " on " << getBuildTimeDetailStr<BuildTimeDetail_compilerAndShortVersion>() << " (" << getBuildTimeDetailStr<BuildTimeDetail_compilerFullVersion>() << "), "
            <<         getBuildTimeDetailStr<BuildTimeDetail_cppStandardVersion>()
            << ", " << getBuildTimeDetailStr<BuildTimeDetail_standardLibrary>()
            << ", " << getBuildTimeDetailStr<BuildTimeDetail_architecture>()
#if defined(_MSC_VER)
            << ", " << getBuildTimeDetailStr<BuildTimeDetail_buildDebugReleaseType>()
#endif
            << ", Qt version (build time) " << getBuildTimeDetailStr<BuildTimeDetail_qtVersion>()
            << ", Boost version " << getBuildTimeDetailStr<BuildTimeDetail_boostVersion>()
            << "\n";
    }
    return rv;
}
