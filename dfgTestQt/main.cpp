#include <dfg/dfgDefs.hpp>
#include <dfg/os.hpp>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
#include <gtest/gtest.h>
#include <QApplication>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

// Including .cc-file due possible Qt Creator bug (for details, see dfgQtTableEditor.pro)
#include <dfg/str/fmtlib/format.cc>

namespace
{
    // Sets working directory so that tests using relative paths will find files.
    static bool setWorkingDirectory()
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


int main(int argc, char **argv)
{
    if (!setWorkingDirectory())
    {
        GTEST_LOG_(ERROR) << "Didn't find input file path, tests won't run. Working directory is " << DFG_MODULE_NS(os)::getCurrentWorkingDirectoryC();
        return 1;
    }

	QApplication app(argc, argv);
	::testing::InitGoogleTest(&argc, argv);

    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvItemModel";
    //::testing::GTEST_FLAG(filter) = "dfgQt.CsvItemModel_readFormatUsageOnWrite";

	return RUN_ALL_TESTS();
}
