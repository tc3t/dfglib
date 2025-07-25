#include "stdafx.h"
#include "../externals/gtest/gtest.h"
#include <dfg/os.hpp>
#include <dfg/build/buildTimeDetails.hpp>
#include <iostream>

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

#if 0

#include <dfg/utf.hpp>

void natvisTesting()
{
    using namespace ::DFG_ROOT_NS;

    const auto ptrAscii  = TypedCharPtrAsciiR("abc");
    const auto ptrLatin1 = TypedCharPtrLatin1R("abc");
    const auto ptrU8     = TypedCharPtrUtf8R("abc\xe2\x82\xac"); // Byte sequence is euro-sign in UTF8
    const auto ptrU16    = TypedCharPtrUtf16R(u"abc\x20AC");     // // Byte sequence is euro-sign in UTF16

    const auto szptrAscii  = SzPtrAscii("abc");
    const auto szptrLatin1 = SzPtrLatin1("abc");
    const auto szptrU8     = SzPtrUtf8("abc\xe2\x82\xac");
    const auto szptrU16    = SzPtrUtf16(u"abc\x20AC");

    StringViewC svCnull(nullptr);
    StringViewC svC("abc");
    StringViewC svCNonSz("abc", 2);
    StringViewC svEmbeddedNull("ab\0c", 4);
    StringViewSzC svSzCnull(nullptr);
    StringViewSzC svSzC("abc");
    StringViewSzC svSzCEmbeddedNull("ab\0c", 4); // SzView with embedded null
    StringViewUtf8 svUtf8(SzPtrUtf8("ab\xe2\x82\xac" "d")); // Byte sequence is euro-sign
    StringViewSzUtf8 svSzUtf8(SzPtrUtf8("ab\xe2\x82\xac")); // Byte sequence is euro-sign
    StringViewW sCW(L"abc");
    StringView16 sv16(u"abc\x20AC");
    StringView32 sv32(U"abc");

    StringUtf8 sUtf8(szptrU8);
}

#endif // natvis testing

int main(int argc, char **argv)
{
    //natvisTesting();
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

    //::testing::GTEST_FLAG(filter) = "dfg.integerDigitAtPos";
    //::testing::GTEST_FLAG(filter) = "dfg.isEmpty";
    //::testing::GTEST_FLAG(filter) = "dfg.isValidIndex";
    //::testing::GTEST_FLAG(filter) = "dfg.OpaquePtr";
    //::testing::GTEST_FLAG(filter) = "dfg.saturateAdd";
    //::testing::GTEST_FLAG(filter) = "dfg.saturateCast";
    //::testing::GTEST_FLAG(filter) = "dfg.ScopedCaller";
    //::testing::GTEST_FLAG(filter) = "dfg.Span";
    //::testing::GTEST_FLAG(filter) = "dfgAlg.arrayCopy";
    //::testing::GTEST_FLAG(filter) = "dfgAlg.detail_areContiguousByTypeAndOverlappingRanges";
    //::testing::GTEST_FLAG(filter) = "dfgAlg.floatIndexInSorted";
    //::testing::GTEST_FLAG(filter) = "dfgAlg.nearestRangeInSorted";
    //::testing::GTEST_FLAG(filter) = "dfgAlg.replaceSubarrays";
    //::testing::GTEST_FLAG(filter) = "dfgBuild.buildTimeDetails";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.AbstractChartControlItem";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.ChartOperationPipeData";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.operations";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.operations_blockWindow";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.operations_formula";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.operations_passWindow";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.operations_regexFormat";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.operations_smoothing_indexNb";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.operations_textFilter";
    //::testing::GTEST_FLAG(filter) = "dfgCharts.ChartEntryOperationManager";
    //::testing::GTEST_FLAG(filter) = "dfgConcurrency.ConditionCounter";
    //::testing::GTEST_FLAG(filter) = "dfgConcurrency.ConditionCounter_waitFor";
    //::testing::GTEST_FLAG(filter) = "dfgCont.contAlg";
    //::testing::GTEST_FLAG(filter) = "dfgCont.contAlg_equal";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvConfig";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvConfig_saving";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvConfig_forEach";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvConfig_uriPart";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvFormatDefinition";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvFormatDefinition_FromCsvConfig";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvFormatDefinition_isFormatMatchingWith";
    //::testing::GTEST_FLAG(filter) = "dfgCont.CsvFormatDefinition_ToConfig";
    //::testing::GTEST_FLAG(filter) = "dfgCont.Flags";
    //::testing::GTEST_FLAG(filter) = "dfgCont.IntervalSet";
    //::testing::GTEST_FLAG(filter) = "dfgCont.IntervalSet_shift_raw";
    //::testing::GTEST_FLAG(filter) = "dfgCont.IntervalSet_wrapNegatives";
    //::testing::GTEST_FLAG(filter) = "dfgCont.intervalSetFromString";
    //::testing::GTEST_FLAG(filter) = "dfgCont.makeVector";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapBlockIndex";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapBlockIndex_benchmarks";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapBlockIndex_iterators";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapPerformanceComparisonWithStdStringKeyAndConstCharLookUp";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapVector";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapVectorPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapVector_keyValueRanges";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapVector_makeIndexMapped";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapVector_valueCopyOr";
    //::testing::GTEST_FLAG(filter) = "dfgCont.SetVector";
    //::testing::GTEST_FLAG(filter) = "dfgCont.MapToStringViews";
    //::testing::GTEST_FLAG(filter) = "dfgCont.SortedSequence";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv_exceptionHandling";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv_filterCellHandler";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv_invalidUtfWrite";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv_memStreamTypes";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv_multiThreadedRead";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv_multiThreadedReadPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableCsv_peekCsvFormatFromFile";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableSz_appendTablesWithMove";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableSz_insertToFrontPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgCont.TableSz_cellCountNonEmpty";
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
    //::testing::GTEST_FLAG(filter) = "dfgDataAnalysis.smoothWithNeighbourAverages";
    //::testing::GTEST_FLAG(filter) = "dfgDataAnalysis.smoothWithNeighbourMedians";
    //::testing::GTEST_FLAG(filter) = "dfgDebug.SehExceptionHandler";
    //::testing::GTEST_FLAG(filter) = "dfgIo.areAsciiBytesValidContentInEncoding";
    //::testing::GTEST_FLAG(filter) = "dfgIo.baseCharacterSize";
    //::testing::GTEST_FLAG(filter) = "dfgIo.BasicIfStream";
    //::testing::GTEST_FLAG(filter) = "dfgIo.BasicImStream";
    //::testing::GTEST_FLAG(filter) = "dfgIo.BasicOmcByteStream";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_basicReader";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_CharAppenderUtf";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_metaCharHandling";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_autoDetectCsvSeparator";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_read";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_readCell";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_readCellPastEnclosedCell";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_readRow";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_readRowConsecutiveSeparators";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_readTableToContainer";
    //::testing::GTEST_FLAG(filter) = "DfgIo.DelimitedTextReader_rnTranslation";
    //::testing::GTEST_FLAG(filter) = "dfgIo.DelimitedTextCellWriter_isEnclosementNeeded";
    //::testing::GTEST_FLAG(filter) = "dfgIo.DelimitedTextCellWriterStr";
    //::testing::GTEST_FLAG(filter) = "dfgIo.encodingStrIds";
    //::testing::GTEST_FLAG(filter) = "dfgIo.endOfLineTypeFromStr";
    //::testing::GTEST_FLAG(filter) = "dfgIo.fileToByteContainer";
    //::testing::GTEST_FLAG(filter) = "dfgIo.fileToMemory_readOnly";
    //::testing::GTEST_FLAG(filter) = "dfgIo.getThrough";
    //::testing::GTEST_FLAG(filter) = "dfgIo.IfStreamWithEncoding";
    //::testing::GTEST_FLAG(filter) = "dfgIo.IfStreamWithEncoding_rawByteReading";
    //::testing::GTEST_FLAG(filter) = "dfgIo.IfStreamWithEncodingReadOnlyFile";
    //::testing::GTEST_FLAG(filter) = "dfgIo.ImcByteStream";
    //::testing::GTEST_FLAG(filter) = "dfgIo.ImStreamWithEncoding";
    //::testing::GTEST_FLAG(filter) = "dfgIo.ImStreamWithEncoding_UCS";
    //::testing::GTEST_FLAG(filter) = "dfgIo.isBigEndianEncoding";
    //::testing::GTEST_FLAG(filter) = "dfgIo.IStreamWithEncoding_Windows1252";
    //::testing::GTEST_FLAG(filter) = "dfgIo.isUtfEncoding";
    //::testing::GTEST_FLAG(filter) = "dfgIo.OfStreamWithEncoding";
    //::testing::GTEST_FLAG(filter) = "dfgIo.OfStreamWithEncoding_write";
    //::testing::GTEST_FLAG(filter) = "dfgIo.ostreamPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgIo.StreamBufferMem";
    //::testing::GTEST_FLAG(filter) = "dfgIo.writeDelimited";
    //::testing::GTEST_FLAG(filter) = "dfgIter.CustomAccessIterator";
    //::testing::GTEST_FLAG(filter) = "dfgIter.FunctionValueIterator";
    //::testing::GTEST_FLAG(filter) = "dfgIter.RangeIterator";
    //::testing::GTEST_FLAG(filter) = "dfgIter.RangeIterator_isSizeOne";
    //::testing::GTEST_FLAG(filter) = "dfgIter.RawStorageIterator";
    //::testing::GTEST_FLAG(filter) = "dfgIter.szIterator";
    //::testing::GTEST_FLAG(filter) = "dfgIter.szIterator_operatorLt";
    //::testing::GTEST_FLAG(filter) = "dfgLog.basicFmtTests";
    //::testing::GTEST_FLAG(filter) = "dfgMath.absAsUnsigned";
    //::testing::GTEST_FLAG(filter) = "dfgMath.FormulaParser";
    //::testing::GTEST_FLAG(filter) = "dfgMath.FormulaParser_cmath";
    //::testing::GTEST_FLAG(filter) = "dfgMath.FormulaParser_forEachDefinedFunctionNameWhile";
    //::testing::GTEST_FLAG(filter) = "dfgMath.FormulaParser_functors";
    //::testing::GTEST_FLAG(filter) = "dfgMath.FormulaParser_randomFunctions";
    //::testing::GTEST_FLAG(filter) = "dfgMath.FormulaParser_time_epochMsec";
    //::testing::GTEST_FLAG(filter) = "dfgMath.FormulaParser_time_ISOdateTo";
    //::testing::GTEST_FLAG(filter) = "dfgMath.isFloatConvertibleTo";
    //::testing::GTEST_FLAG(filter) = "dfgMath.isNumberConvertibleTo";
    //::testing::GTEST_FLAG(filter) = "dfgMath.isIntegerValued";
    //::testing::GTEST_FLAG(filter) = "dfgMath.largestContiguousFloatInteger";
    //::testing::GTEST_FLAG(filter) = "dfgMath.logOfBase";
    //::testing::GTEST_FLAG(filter) = "dfgMath.numericDistance";
    //::testing::GTEST_FLAG(filter) = "dfgNumeric.accumulate";
    //::testing::GTEST_FLAG(filter) = "dfgNumeric.minmaxElement_withNanHandling";
    //::testing::GTEST_FLAG(filter) = "dfgNumeric.percentileRange_and_percentile_ceilElem";
    //::testing::GTEST_FLAG(filter) = "dfgOs.getMemoryUsage_process";
    //::testing::GTEST_FLAG(filter) = "dfgOs.getMemoryUsage_system";
    //::testing::GTEST_FLAG(filter) = "dfgOs.MemoryMappedFile";
    //::testing::GTEST_FLAG(filter) = "dfgOs.OutputFile_completeOrNone";
    //::testing::GTEST_FLAG(filter) = "dfgOs.TemporaryFile";
    //::testing::GTEST_FLAG(filter) = "dfgOs.renameFileOrDirectory_cstdio";
    //::testing::GTEST_FLAG(filter) = "dfgPerformance.CsvReadPerformance";
    //::testing::GTEST_FLAG(filter) = "dfgRand.distributionCreation";
    //::testing::GTEST_FLAG(filter) = "dfgRand.DistributionFunctor";
    //::testing::GTEST_FLAG(filter) = "dfgRand.forEachDistributionType";
    //::testing::GTEST_FLAG(filter) = "dfgStr.beginsWith";
    //::testing::GTEST_FLAG(filter) = "dfgStr.beginsWith_TypedStrings";
    //::testing::GTEST_FLAG(filter) = "dfgStr.beginsWith_StringViews";
    //::testing::GTEST_FLAG(filter) = "dfgStr.ByteCountFormatter_metric";
    //::testing::GTEST_FLAG(filter) = "dfgStr.charToPrintable";
    //::testing::GTEST_FLAG(filter) = "dfgStr.floatingPointTypeToSprintfType";
    //::testing::GTEST_FLAG(filter) = "dfgStr.format_fmt";
    //::testing::GTEST_FLAG(filter) = "dfgStr.format_regexFmt";
    //::testing::GTEST_FLAG(filter) = "dfgStr.intToRadixRepresentation";
    //::testing::GTEST_FLAG(filter) = "dfgStr.replaceSubStrsInplace";
    //::testing::GTEST_FLAG(filter) = "dfgStr.String";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringView";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringView_autoConvToUntyped";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringView_lessThan";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringViewOrOwner";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringView_trimFront";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringView_trimmed_tail";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringViewSz";
    //::testing::GTEST_FLAG(filter) = "dfgStr.StringViewSz_popFrontBaseChar";
    //::testing::GTEST_FLAG(filter) = "dfgStr.strTo";
    //::testing::GTEST_FLAG(filter) = "dfgStr.strTo_bool";
    //::testing::GTEST_FLAG(filter) = "dfgStr.strToByLexCast";
    //::testing::GTEST_FLAG(filter) = "dfgStr.toStr";
    //::testing::GTEST_FLAG(filter) = "dfgStr.TypedCharPtrT";
    //::testing::GTEST_FLAG(filter) = "dfgStr.utf16";
    //::testing::GTEST_FLAG(filter) = "dfgTest.googleTestStringViewPrinter";
    //::testing::GTEST_FLAG(filter) = "dfgTime.DateTime_dayOfWeek";
    //::testing::GTEST_FLAG(filter) = "dfgTime.DateTime_fromString";
    //::testing::GTEST_FLAG(filter) = "dfgTime.DateTime_toSecondsSinceEpoch";
    //::testing::GTEST_FLAG(filter) = "dfgTime.DateTime_fromStdTm";
    //::testing::GTEST_FLAG(filter) = "dfgTime.DateTime_fromTime_t";
    //::testing::GTEST_FLAG(filter) = "dfgTime.DateTime_secondsTo";
    //::testing::GTEST_FLAG(filter) = "dfgTime.DateTime_toSYSTEMTIME";
    //::testing::GTEST_FLAG(filter) = "DfgUtf.utfGeneral";
    //::testing::GTEST_FLAG(filter) = "DfgUtf.cpToEncoded";
    //::testing::GTEST_FLAG(filter) = "DfgUtf.cpToUtfToCp";
    //::testing::GTEST_FLAG(filter) = "DfgUtf.readUtfCharAndAdvance";
    //::testing::GTEST_FLAG(filter) = "DfgUtf.windows1252charToCp";

    auto rv = RUN_ALL_TESTS();
    dfgtest::printPostTestDetailSummary(std::cout);
#ifdef __MINGW32__
    // Pause with MinGW because when run from Visual Studio, without pause the console closes immediately after running tests.
    // TODO: add check to pause only if tests seems to be run from Visual Studio (=parent process is devenv.exe)
    std::system("pause");
#endif
    return rv;
}

#if 0
TEST(dfgTest, googleTestEqualComparison)
{
    EXPECT_EQ(-1, (std::numeric_limits<size_t>::max)()); // This falsely passes with original gtest 1.11.0, fails (as it should) with adjusted version.
}
#endif

// This can be used to test GoogleTest printers
#if 0
TEST(dfgTest, googleTestStringViewPrinter)
{
    using namespace DFG_ROOT_NS;
    DFGTEST_EXPECT_LEFT(StringViewC("abcde"), StringViewC("fghij"));
    DFGTEST_EXPECT_LEFT(StringViewSzC("abcde"), StringViewSzC("fghij"));
    DFGTEST_EXPECT_LEFT(StringViewW(L"abc\xe4" L"de"), StringViewW(L"fghij"));
    DFGTEST_EXPECT_LEFT(StringViewSzW(L"abcde"), StringViewSzW(L"fghij"));
    DFGTEST_EXPECT_LEFT(StringViewAscii(DFG_ASCII("abcde")), StringViewAscii(DFG_ASCII("fghij")));
    DFGTEST_EXPECT_LEFT(StringViewSzAscii(DFG_ASCII("abcde")), StringViewSzAscii(DFG_ASCII("fghij")));
    DFGTEST_EXPECT_LEFT(StringViewLatin1(DFG_ASCII("abcde")), StringViewLatin1(DFG_ASCII("fghij")));
    DFGTEST_EXPECT_LEFT(StringViewSzLatin1(DFG_ASCII("abcde")), StringViewSzLatin1(DFG_ASCII("fghij")));
    DFGTEST_EXPECT_LEFT(StringViewUtf8(DFG_UTF8("abcde")), StringViewUtf8(DFG_UTF8("fghij")));
    DFGTEST_EXPECT_LEFT(StringViewSzUtf8(DFG_UTF8("abcde")), StringViewSzUtf8(DFG_UTF8("fghij")));
    DFGTEST_EXPECT_LEFT(std::string("abcde"), std::string("fghij"));
    DFGTEST_EXPECT_LEFT(std::wstring(L"abcde"), std::wstring(L"fghij"));

    DFGTEST_EXPECT_LEFT(DFG_UTF8("abcde"), DFG_UTF8("fghij"));
    DFGTEST_EXPECT_LEFT(DFG_ASCII("abcde"), DFG_ASCII("fghij"));

    // StringTyped
    DFGTEST_EXPECT_LEFT(DFG_ASCII("abcde"), StringAscii(DFG_ASCII("fghij")));
    DFGTEST_EXPECT_LEFT(DFG_UTF8("abcde"),  StringUtf8(DFG_UTF8("fghij")));
    DFGTEST_EXPECT_LEFT(StringViewUtf16(DFG_UTF16("abcde")), StringUtf16(DFG_UTF16("fghij")));
}

#endif // GoogleTest printer tests
