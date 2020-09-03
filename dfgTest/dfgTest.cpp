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

    // Code below can be used to run only specific test by uncommenting the one test that should be run (if for some reason this seems more convenient than other options).
    // Tests can also be run and controlled from command line and possibly from test features in IDE (e.g. in Visual Studio).
    // Examples of some command line options:
    //      -Run one test:                dfgTest --gtest_filter=dfgCont.CsvConfig
    //      -List tests:                  dfgTest --gtest_list_tests
    //      -Output test results to json: dfgTest --gtest_output=json
    //      -Output test results to xml:  dfgTest --gtest_output=xml
    // For more details, see              dfgTest --help

    //::testing::GTEST_FLAG(filter) = "dfg.saturateCast";
    //::testing::GTEST_FLAG(filter) = "dfg.isEmpty";
    //::testing::GTEST_FLAG(filter) = "dfg.isValidIndex";
    //::testing::GTEST_FLAG(filter) = "dfg.ScopedCaller";
    //::testing::GTEST_FLAG(filter) = "dfgAlg.arrayCopy";
    //::testing::GTEST_FLAG(filter) = "dfgAlg.floatIndexInSorted";
    //::testing::GTEST_FLAG(filter) = "dfgAlg.nearestRangeInSorted";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.operations";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.operations_passWindow";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.ChartEntryOperationManager";
    //::testing::GTEST_FLAG(filter) = "dfgCont.contAlg";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvConfig";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvConfig_saving";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvConfig_forEachStartingWith";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvFormatDefinition_FromCsvConfig";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvFormatDefinition_ToConfig";
    //::testing::GTEST_FLAG(filter) = "dfgCont.IntervalSet";
    //::testing::GTEST_FLAG(filter) = "dfgCont.intervalSetFromString";
    //::testing::GTEST_FLAG(filter) = "dfgCont.makeVector";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapBlockIndex";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapBlockIndex_iterators";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapPerformanceComparisonWithStdStringKeyAndConstCharLookUp";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapVector";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapVectorPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgCont.SetVector";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapToStringViews";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv_filterCellHandler";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv_memStreamTypes";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv_peekCsvFormatFromFile";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableSz_insertToFrontPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableSz_contentStorageSizeInBytes";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableSz_addRemoveColumns";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableSz_insertRowsAt";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableSz_indexHandling";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableSz_maxLimits";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableSzUntypedInterface";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TrivialPair";
    //::testing::GTEST_FLAG(filter) = "dfgCont.UniqueResourceHolder";
    //::testing::GTEST_FLAG(filter) = "dfgCont.ValueArray"
    //::testing::GTEST_FLAG(filter) = "dfgCont.VectorInsertPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgCont.VectorSso";
    //::testing::GTEST_FLAG(filter) = "dfgCont.ViewableSharedPtr";
    //::testing::GTEST_FLAG(filter) = "dfgCont.ViewableSharedPtr_editing";
    //::testing::GTEST_FLAG(filter) = "dfgCont.ViewableSharedPtr_editingWithThreads";
    //::testing::GTEST_FLAG(filter) = "dfgDebug.SehExceptionHandler";
    //::testing::GTEST_FLAG(filter) = "dfgIo.BasicImStream";
    //::testing::GTEST_FLAG(filter) = "dfgIo.BasicOmcByteStream";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_basicReader";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_CharAppenderUtf";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_metaCharHandling";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_autoDetectCsvSeparator";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_readCell";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_readRowConsecutiveSeparators";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_readTableToContainer";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_rnTranslation";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextCellWriterStr";
    //::testing::GTEST_FLAG(filter) = "dfgIo.endOfLineTypeFromStr";
    //::testing::GTEST_FLAG(filter) = "dfgIo.getThrough";
    //::testing::GTEST_FLAG(filter) = "dfgIo.ImcByteStream";
    //::testing::GTEST_FLAG(filter) = "dfgIo.ImStreamWithEncoding_UCS";
    //::testing::GTEST_FLAG(filter) = "dfgIo.IStreamWithEncoding_Windows1252";
    //::testing::GTEST_FLAG(filter) = "dfgIo.OfStreamWithEncoding";
    //::testing::GTEST_FLAG(filter) = "dfgIo.OfStreamWithEncoding_write";
    //::testing::GTEST_FLAG(filter) = "dfgIo.ostreamPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgIo.StreamBufferMem";
    //::testing::GTEST_FLAG(filter) = "dfgIo.writeDelimited";
    //::testing::GTEST_FLAG(filter) = "dfgIter.RangeIterator";
    //::testing::GTEST_FLAG(filter) = "dfgIter.RawStorageIterator";
    //::testing::GTEST_FLAG(filter) = "dfgIter.szIterator";
    //::testing::GTEST_FLAG(filter) = "dfgIter.szIterator_operatorLt";
    //::testing::GTEST_FLAG(filter) = "dfgMath.absAsUnsigned";
    //::testing::GTEST_FLAG(filter) = "dfgMath.isFloatConvertibleTo";
    //::testing::GTEST_FLAG(filter) = "dfgMath.isIntegerValued";
    //::testing::GTEST_FLAG(filter) = "dfgMath.logOfBase";
    //::testing::GTEST_FLAG(filter) = "dfgMath.numericDistance";
    //::testing::GTEST_FLAG(filter) = "dfgNumeric.accumulate";
    //::testing::GTEST_FLAG(filter) = "dfgNumeric.minmaxElement_withNanHandling";
    //::testing::GTEST_FLAG(filter) = "dfgNumeric.percentileRange_and_percentile_ceilElem";
    //::testing::GTEST_FLAG(filter) = "dfgOs.OutputFile_completeOrNone";
    //::testing::GTEST_FLAG(filter) = "dfgOs.TemporaryFile";
    //::testing::GTEST_FLAG(filter) = "dfgOs.renameFileOrDirectory_cstdio";
    //::testing::GTEST_FLAG(filter) = "dfgPerformance.CsvReadPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgStr.beginsWith";
    //::testing::GTEST_FLAG(filter) = "dfgStr.beginsWith_TypedStrings";
    //::testing::GTEST_FLAG(filter) = "dfgStr.beginsWith_StringViews";
    //::testing::GTEST_FLAG(filter) = "dfgStr.floatingPointTypeToSprintfType";
    //::testing::GTEST_FLAG(filter) = "dfgStr.intToRadixRepresentation";
    //::testing::GTEST_FLAG(filter) = "dfgStr.String";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringView";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringView_autoConvToUntyped";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringViewOrOwner";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringViewSz";
    //::testing::GTEST_FLAG(filter) = "dfgStr.strTo";
    //::testing::GTEST_FLAG(filter) = "dfgStr.strTo_bool";
    //::testing::GTEST_FLAG(filter) = "dfgStr.strToByLexCast";
    //::testing::GTEST_FLAG(filter) = "dfgStr.toStr";
    //::testing::GTEST_FLAG(filter) = "dfgStr.TypedCharPtrT";
    //::testing::GTEST_FLAG(filter) = "DfgUtf.utfGeneral";
    //::testing::GTEST_FLAG(filter) = "DfgUtf.cpToEncoded";
    //::testing::GTEST_FLAG(filter) = "DfgUtf.windows1252charToCp";
    
    auto rv = RUN_ALL_TESTS();
#ifdef __MINGW32__
    // Pause with MinGW because when run from Visual Studio, without pause the console closes immediately after running tests.
    // TODO: add check to pause only if tests seems to be run from Visual Studio (=parent process is devenv.exe)
    std::system("pause");
#endif
    return rv;
}
