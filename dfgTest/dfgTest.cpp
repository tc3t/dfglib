#include "stdafx.h"
#include "../externals/gtest/gtest.h"
#include <dfg/os.hpp>

namespace
{
    // Hack: Sets working directory so that tests using relative paths will find files. In normal Visual Studio project usage the path is correct by default, but 
    // adjustment is needed e.g. for CMake-generated projects and when running tests through Visual Studio Test Explorer.
    static bool setWorkingDirectory()
    {
        using namespace DFG_MODULE_NS(os);
        const auto cwd = getCurrentWorkingDirectoryC();

        std::string modifier;
        if (isPathFileAvailable(cwd + "/testfiles/matrix_10x10_1to100_eol_rn.txt", FileModeExists))
            modifier = ".";
        else if (isPathFileAvailable(cwd + "/dfgTest/testfiles/matrix_10x10_1to100_eol_rn.txt", FileModeExists))
            modifier = "/dfgTest/";
        else if (isPathFileAvailable(cwd + "/../dfgTest/testfiles/matrix_10x10_1to100_eol_rn.txt", FileModeExists))
            modifier = "/../dfgTest/";
        else if (isPathFileAvailable(cwd + "/../../dfgTest/testfiles/matrix_10x10_1to100_eol_rn.txt", FileModeExists))
            modifier = "/../../dfgTest/";
        else if (isPathFileAvailable(cwd + "/../../../dfgTest/testfiles/matrix_10x10_1to100_eol_rn.txt", FileModeExists))
            modifier = "/../../../dfgTest/";
        else if (isPathFileAvailable(cwd + "/../../../../dfgTest/testfiles/matrix_10x10_1to100_eol_rn.txt", FileModeExists))
            modifier = "/../../../../dfgTest/";
        if (!modifier.empty() && modifier != ".")
        {
            const auto newCwd = cwd + modifier;
            setCurrentWorkingDirectory(newCwd);
            GTEST_LOG_(INFO) << "Adjusted current working directory, now: " << getCurrentWorkingDirectoryC();
        }
        return isPathFileAvailable(getCurrentWorkingDirectoryC() + "/testfiles/matrix_10x10_1to100_eol_rn.txt", FileModeExists);
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (!setWorkingDirectory())
    {
        GTEST_LOG_(ERROR) << "Didn't find input file path, tests won't run. Working directory is " << DFG_MODULE_NS(os)::getCurrentWorkingDirectoryC();
        return 1;
    }

    // Code below can be used to run only specific test.
    //::testing::GTEST_FLAG(filter) = "dfg.isEmpty";
    //::testing::GTEST_FLAG(filter) = "dfg.ScopedCaller";
    //::testing::GTEST_FLAG(filter) = "dfgAlg.arrayCopy";
    //::testing::GTEST_FLAG(filter) = "dfgCont.contAlg";
    //::testing::GTEST_FLAG(filter) = "dfgCont.makeVector";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapPerformanceComparisonWithStdStringKeyAndConstCharLookUp";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapVector";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapVectorPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgCont.SetVector";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TrivialPair";
    //::testing::GTEST_FLAG(filter) = "dfgCont.UniqueResourceHolder";
    //::testing::GTEST_FLAG(filter) = "dfgCont.ValueArray"
    //::testing::GTEST_FLAG(filter) = "dfgCont.VectorInsertPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgCont.VectorSso";
    //::testing::GTEST_FLAG(filter) = "dfgDebug.SehExceptionHandler";
    //::testing::GTEST_FLAG(filter) = "dfgIo.BasicImStream";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_basicReader";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_metaCharHandling";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_autoDetectCsvSeparator";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_rnTranslation";
    //::testing::GTEST_FLAG(filter) = "dfgIo.getThrough";
    //::testing::GTEST_FLAG(filter) = "dfgIo.OfStreamWithEncoding";
    //::testing::GTEST_FLAG(filter) = "dfgIo.StreamBufferMem";
    //::testing::GTEST_FLAG(filter) = "dfgIo.writeDelimited";
    //::testing::GTEST_FLAG(filter) = "dfgIter.szIterator";
    //::testing::GTEST_FLAG(filter) = "dfgNumeric.accumulate";
    //::testing::GTEST_FLAG(filter) = "dfgPerformance.CsvReadPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgStr.String";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringView";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringViewSz";
    //::testing::GTEST_FLAG(filter) = "dfgStr.toStr";
    //::testing::GTEST_FLAG(filter) = "dfgStr.TypedCharPtrT";
    
    auto rv = RUN_ALL_TESTS();
    //std::system("pause");
    return rv;
}
