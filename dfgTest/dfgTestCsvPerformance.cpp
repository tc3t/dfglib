#include "stdafx.h"

#if (defined(DFGTEST_BUILD_MODULE_CSV_BENCHMARK) && DFGTEST_BUILD_MODULE_CSV_BENCHMARK == 1) || (!defined(DFGTEST_BUILD_MODULE_CSV_BENCHMARK) && DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/build/compilerDetails.hpp>
#include <dfg/cont/tableCsv.hpp>
#include <dfg/preprocessor/compilerInfoMsvc.hpp>
#include <dfg/time/timerCpu.hpp>
#include <dfg/numeric/average.hpp>
#include <dfg/numeric/median.hpp>
#include <dfg/numeric/accumulate.hpp>
#include <dfg/build/utils.hpp>
#include <dfg/io/fileToByteContainer.hpp>
#include <dfg/io/OfStream.hpp>
#include <dfg/time.hpp>
#include <dfg/os/memoryMappedFile.hpp>
#include <iostream>
#include <strstream>
#include <sstream>

#if DFG_MSVC_VER == 0 || DFG_MSVC_VER >= DFG_MSVC_VER_2015
    #define ENABLE_FAST_CPP_CSV_PARSER 1
#else
    #define ENABLE_FAST_CPP_CSV_PARSER 0
#endif

#define ENABLE_CSV_PARSER_A      (0)
#define ENABLE_CSV_PARSER_V   (0 && DFG_MSVC_VER > DFG_MSVC_VER_2015) // Didn't seem to build on VC2015 as of 2020-07

#if ENABLE_FAST_CPP_CSV_PARSER
    DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
        #include <dfg/io/fast-cpp-csv-parser/csv.h>
    DFG_END_INCLUDE_WITH_DISABLED_WARNINGS
#endif

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include <dfg/io/cppcsv_ph/csvparser.hpp>
    #if (ENABLE_CSV_PARSER_A == 1)
        #include <dfg/io/csv-parser_a/parser.hpp>
    #endif
    #if (ENABLE_CSV_PARSER_V == 1)
        #include <dfg/io/csv-parser_v/csv.hpp>
    #endif
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

namespace
{
    #ifndef DFG_BUILD_TYPE_DEBUG
        // Release
        const size_t gnRowCount = 4000000;
    #else
        // Debug
        const size_t gnRowCount = 1000;
    #endif
    const size_t gnColCount = 7;
    const size_t gnExpectedCellCounterValueWithHeaderCells = (gnRowCount + 1) * gnColCount; // + 1 from header

    const size_t gnRunCount = 5;

    typedef DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) TimerType;

    std::string GenerateTestFile(const size_t nRowCount, const size_t nColCount, const bool bEnclose)
    {
        using namespace DFG_MODULE_NS(io);
        using namespace DFG_MODULE_NS(str);

        std::string sFilePath = "testfiles/generated/generatedCsv_r" + toStrC(nRowCount) + "_c" + toStrC(nColCount) + "_constant_content_abc_test" + ((bEnclose) ? "_enclosed": "") + ".csv";

        TimerType timer;

        DFG_MODULE_NS(io)::OfStream ostrm(sFilePath);
        if (nRowCount == 0 || nColCount == 0)
            return sFilePath;
        for (size_t c = 0; c < nColCount; ++c)
        {
            if (c > 0)
                ostrm << ',';
            ostrm << "Column " << c;
        }
        ostrm << '\n';
        for (size_t r = 0; r < nRowCount; ++r)
        {
            DelimitedTextCellWriter::writeCellStrm(ostrm, "abc", ',', '"', '\n', (bEnclose) ? EbEnclose : EbEncloseIfNeeded);
            for (size_t c = 1; c < nColCount; ++c)
            {
#if 1
                ostrm << ((bEnclose) ? ",\"abc\"" : ",abc");
#else
                writeBinary(ostrm, ',');
                //DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellStrm(ostrm, "abc", ',', '"', '\n', EbEncloseIfNeeded); // This was about 2.5 x slower than direct writing in a run instance.
                //DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellStrm(ostrm, "abc", ',', '"', '\n', EbNoEnclose); // About as slow as with EbEncloseIfNeeded in the same run instance.
#endif
            }
            writeBinary(ostrm, '\n');
        }

        //std::cout << "Writing lasted " << timer.elapsedWallSeconds() << " seconds";
        //std::system("pause");

        return sFilePath;
    }
}

namespace
{
    //typedef std::vector<char> FileByteHolder;
    typedef DFG_MODULE_NS(os)::DFG_CLASS_NAME(MemoryMappedFile) FileByteHolder;

    void readBytes(const std::string& sFilePath, std::vector<char>& bytes)
    {
        bytes = DFG_MODULE_NS(io)::fileToVector(sFilePath);
    }

    void readBytes(const std::string& sFilePath, DFG_MODULE_NS(os)::DFG_CLASS_NAME(MemoryMappedFile)& bytes)
    {
        bytes.open(sFilePath);
    }

    std::unique_ptr<std::ifstream> InitIfstream(const std::string& sFilePath, FileByteHolder&)
    {
        return std::unique_ptr<std::ifstream>(new std::ifstream(sFilePath));
    }

    std::unique_ptr<::DFG_MODULE_NS(io)::BasicIfStream> InitBasicIfstream(const std::string& sFilePath, FileByteHolder&)
    {
        return std::unique_ptr<::DFG_MODULE_NS(io)::BasicIfStream>(new ::DFG_MODULE_NS(io)::BasicIfStream(sFilePath));
    }

    std::unique_ptr<::DFG_MODULE_NS(io)::IfStreamWithEncoding> InitIfStreamWithEncoding(const std::string& sFilePath, FileByteHolder&)
    {
        return std::unique_ptr<::DFG_MODULE_NS(io)::IfStreamWithEncoding>(new ::DFG_MODULE_NS(io)::IfStreamWithEncoding(sFilePath));
    }

    std::unique_ptr<std::istrstream> InitIStrStream(const std::string& sFilePath, FileByteHolder& byteContainer)
    {
        readBytes(sFilePath, byteContainer);
        return std::unique_ptr<std::istrstream>(new std::istrstream(byteContainer.data(), byteContainer.size()));
    }

    std::unique_ptr<std::istringstream> InitIStringStream(const std::string& sFilePath, FileByteHolder& byteContainer)
    {
        readBytes(sFilePath, byteContainer);
        return std::unique_ptr<std::istringstream>(new std::istringstream(std::string(byteContainer.data(), byteContainer.size())));
    }

    std::unique_ptr<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)> InitIBasicImStream(const std::string& sFilePath, FileByteHolder& byteContainer)
    {
        readBytes(sFilePath, byteContainer);
        return std::unique_ptr<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(new DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)(byteContainer.data(), byteContainer.size()));
    }

    

    template <class Strm_T> std::string StreamName() { DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(Strm_T, "Template specialization for StreamName() is missing for given type."); }

    std::string fileToMemoryType()
    {
        return (std::is_same<DFG_MODULE_NS(os)::DFG_CLASS_NAME(MemoryMappedFile), FileByteHolder>::value) ? " (memory mapped)" : " (fileToVector)";
    }

    template <> std::string StreamName<std::ifstream>() { return "std::ifstream";}
    template <> std::string StreamName<std::istrstream>() { return "std::istrstream" + fileToMemoryType(); }
    template <> std::string StreamName<std::istringstream>() { return "std::istringstream" + fileToMemoryType(); }
    template <> std::string StreamName<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>() { return "dfg::io::BasicImStream" + fileToMemoryType(); }
    template <> std::string StreamName<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicIfStream)>() { return "dfg::io::BasicIfStream"; }
    template <> std::string StreamName<DFG_MODULE_NS(io)::DFG_CLASS_NAME(IfStreamWithEncoding)>() { return "dfg::io::IfStreamWithEncoding"; }
    

    template <class T> std::string prettierTypeName();
    template <> std::string prettierTypeName<const char*>() { return "const char*"; }
    template <> std::string prettierTypeName<std::string>() { return "std::string"; }
    

    DFG_NOINLINE void PrintTestCaseRow(std::ostream& output, const std::string& sFilePath, const std::vector<double>& runtimes, const DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlySzParamC)& sReader, const DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlySzParamC)& formatDefinition, const DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlySzParamC)& sProcessingType, const std::string& sStreamType)
    {
        output  << DFG_MODULE_NS(time)::localDate_yyyy_mm_dd_hh_mm_ss_C() << ",," << DFG_COMPILER_NAME_SIMPLE << ',' << sizeof(void*) << ','
                << DFG_BUILD_DEBUG_RELEASE_TYPE << ',' << sFilePath << "," << sReader << "," << formatDefinition << "," << sProcessingType << "," << sStreamType << ",";
        std::for_each(runtimes.begin(), runtimes.end(), [&](const double val) {output << val << ','; });
        output  << DFG_MODULE_NS(numeric)::average(runtimes) << ","
                << DFG_MODULE_NS(numeric)::median(runtimes) << ","
                << DFG_MODULE_NS(numeric)::accumulate(runtimes, double(0)) << '\n';
    }

    struct ReaderCreation_default {};
    struct ReaderCreation_basic {};

    template <class Strm_T, class CellData_T>
    auto createReader(Strm_T& strm, CellData_T& cd, ReaderCreation_default) -> decltype(DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cd))
    {
        return DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cd);
    }

    template <class Strm_T, class CellData_T>
    auto createReader(Strm_T& strm, CellData_T& cd, ReaderCreation_basic) -> decltype(DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::createReader_basic(strm, cd))
    {
        return DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::createReader_basic(strm, cd);
    }

    struct FormatDefTag_compileTime         {};
    struct FormatDefTag_compileTime_noRn    {}; // Compile time format without \r\n -> \n translation
    struct FormatDefTag_runtime             {};

    std::string formatDefTagToOutputDescription(FormatDefTag_compileTime)         { return "compile time"; }
    std::string formatDefTagToOutputDescription(FormatDefTag_compileTime_noRn)    { return "compile time & no \\r\\n translation"; }
    std::string formatDefTagToOutputDescription(FormatDefTag_runtime)             { return "runtime"; }

    auto createFormatDef(FormatDefTag_compileTime) -> DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::FormatDefinitionSingleCharsCompileTime<DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone, '\n', ','>
    {
        using namespace DFG_MODULE_NS(io);
        return DFG_CLASS_NAME(DelimitedTextReader)::FormatDefinitionSingleCharsCompileTime<DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone, '\n', ','>();
    }

    auto createFormatDef(FormatDefTag_compileTime_noRn)->DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::FormatDefinitionSingleCharsCompileTime<DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone, '\n', ',', DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CsvFormatFlagNoRnTranslation>
    {
        using namespace DFG_MODULE_NS(io);
        return DFG_CLASS_NAME(DelimitedTextReader)::FormatDefinitionSingleCharsCompileTime<DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone, '\n', ',', DFG_CLASS_NAME(DelimitedTextReader)::CsvFormatFlagNoRnTranslation>();
    }

    auto createFormatDef(FormatDefTag_runtime) -> DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::FormatDefinitionSingleChars
    {
        using namespace DFG_MODULE_NS(io);
        return DFG_CLASS_NAME(DelimitedTextReader)::FormatDefinitionSingleChars(DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone, '\n', ',');
    }

    typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharBuffer<char> DefaultBufferType;
    
    template <class IStrm_T, class CharAppender_T, class Buffer_T, class IStrmInit_T, class ReaderImplementationOption_T, class FormatDefTag_T>
    DFG_NOINLINE void ExecuteTestCaseDelimitedTextReader(std::ostream& output,
                                            IStrmInit_T streamInitFunc,
                                            ReaderImplementationOption_T readerOption,
                                            const FormatDefTag_T formatDefTag,
                                            const std::string& sFilePath,
                                            const size_t nCount,
                                            const char* const pszReaderType,
                                            const char* const pszProcessingType)
    {
        using namespace DFG_MODULE_NS(io);

        std::vector<double> runtimes;

        for (size_t i = 0; i < nCount; ++i)
        {
            TimerType timer;
            FileByteHolder bytes;
            auto spStrm = streamInitFunc(sFilePath, bytes);
            EXPECT_TRUE(spStrm != nullptr);
            auto& istrm = *spStrm;

            const auto formatDef = createFormatDef(formatDefTag);

            typedef DFG_CLASS_NAME(DelimitedTextReader)::CellData<char, char, Buffer_T, CharAppender_T, typename std::remove_const<decltype(formatDef)>::type> Cdt;
            Cdt cellDataHandler(formatDef);
            auto reader = createReader(istrm, cellDataHandler, readerOption);

            size_t nCounter = 0;

            auto cellHandler = [&](const size_t, const size_t, const decltype(cellDataHandler)&)
            {
                nCounter++;
            };

            DFG_CLASS_NAME(DelimitedTextReader)::read(reader, cellHandler);
            const auto elapsedTime = timer.elapsedWallSeconds();

            // Note: with CharAppenderNone nothing gets added to the read buffer so the cell handler does not get called.
            const auto nExpectedCount = std::is_same<CharAppender_T, ::DFG_MODULE_NS(io)::DelimitedTextReader::CharAppenderNone>::value ? 0 : gnExpectedCellCounterValueWithHeaderCells;
            EXPECT_EQ(nExpectedCount, nCounter);
            runtimes.push_back(elapsedTime);
        }
        const std::string sFormatDefType = formatDefTagToOutputDescription(FormatDefTag_T());
        
        PrintTestCaseRow(output, sFilePath, runtimes, pszReaderType, sFormatDefType, pszProcessingType, StreamName<IStrm_T>());
    }

    template <class IStrm_T, class IStrmInit_T>
    DFG_NOINLINE void ExecuteTestCase_DelimitedTextReader_NoCharAppend(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount, const bool bBasicReader = false)
    {
        if (bBasicReader)
            ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderNone, DefaultBufferType>(output, streamInitFunc, ReaderCreation_basic(), FormatDefTag_compileTime(), sFilePath, nCount, "DelimitedTextReader_basic", "CharAppenderNone");
        else
            ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderNone, DefaultBufferType>(output, streamInitFunc, ReaderCreation_default(), FormatDefTag_compileTime(), sFilePath, nCount, "DelimitedTextReader", "CharAppenderNone");
    }

    template <class IStrm_T, class IStrmInit_T>
    DFG_NOINLINE void ExecuteTestCase_DelimitedTextReader_DefaultCharAppend(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount, const bool compiletimeFormatDef = true)
    {
        if (compiletimeFormatDef)
            ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DelimitedTextReader::CharAppenderDefault<DefaultBufferType, char>, DefaultBufferType>(output, streamInitFunc, ReaderCreation_default(), FormatDefTag_compileTime(), sFilePath, nCount, "DelimitedTextReader", "CharAppenderDefault");
        else
            ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DelimitedTextReader::CharAppenderDefault<DefaultBufferType, char>, DefaultBufferType>(output, streamInitFunc, ReaderCreation_default(), FormatDefTag_runtime(), sFilePath, nCount, "DelimitedTextReader", "CharAppenderDefault");
    }

    template <class IStrm_T, class IStrmInit_T>
    DFG_NOINLINE void ExecuteTestCase_DelimitedTextReader_StringViewCBufferWithEnclosedCellSupport(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount, const bool compiletimeFormatDef = true)
    {
        typedef DFG_MODULE_NS(io)::DelimitedTextReader::StringViewCBufferWithEnclosedCellSupport BufferType;
        typedef DFG_MODULE_NS(io)::DelimitedTextReader::CharAppenderStringViewCBufferWithEnclosedCellSupport AppenderType;
        if (compiletimeFormatDef)
            ExecuteTestCaseDelimitedTextReader<IStrm_T, AppenderType, BufferType>(output, streamInitFunc, ReaderCreation_default(), FormatDefTag_compileTime(), sFilePath, nCount, "DelimitedTextReader", "CharAppenderStringViewCBufferWithEnclosedCellSupport");
        else
            ExecuteTestCaseDelimitedTextReader<IStrm_T, AppenderType, BufferType>(output, streamInitFunc, ReaderCreation_default(), FormatDefTag_runtime(), sFilePath, nCount, "DelimitedTextReader", "CharAppenderStringViewCBufferWithEnclosedCellSupport");
    }

    template <class IStrm_T, class IStrmInit_T>
    DFG_NOINLINE void ExecuteTestCase_DelimitedTextReader_basicReader(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount, const bool compiletimeFormatDef)
    {
        if (compiletimeFormatDef)
            ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderDefault<DefaultBufferType, char>, DefaultBufferType>(output, streamInitFunc, ReaderCreation_basic(), FormatDefTag_compileTime(), sFilePath, nCount, "DelimitedTextReader_basic", "CharAppenderDefault");
        else
            ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderDefault<DefaultBufferType, char>, DefaultBufferType>(output, streamInitFunc, ReaderCreation_basic(), FormatDefTag_runtime(), sFilePath, nCount, "DelimitedTextReader_basic", "CharAppenderDefault");
    }

    template <class IStrm_T, class IStrmInit_T>
    DFG_NOINLINE void ExecuteTestCase_DelimitedTextReader_basicReader_stringViewBuffer(std::ostream& output,
                                                                          IStrmInit_T streamInitFunc,
                                                                          const std::string& sFilePath,
                                                                          const size_t nCount,
                                                                          const bool compiletimeFormatDef,
                                                                          const bool rnTranslation)
    {
        typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::StringViewCBuffer BufferType;
        typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderStringViewCBuffer AppenderType;
        if (compiletimeFormatDef)
        {
            if (rnTranslation)
                ExecuteTestCaseDelimitedTextReader<IStrm_T, AppenderType, BufferType>(output, streamInitFunc, ReaderCreation_basic(), FormatDefTag_compileTime(), sFilePath, nCount, "DelimitedTextReader_basic", "CharAppenderStringViewCBuffer");
            else
                ExecuteTestCaseDelimitedTextReader<IStrm_T, AppenderType, BufferType>(output, streamInitFunc, ReaderCreation_basic(), FormatDefTag_compileTime_noRn(), sFilePath, nCount, "DelimitedTextReader_basic", "CharAppenderStringViewCBuffer");
        }
        else
            ExecuteTestCaseDelimitedTextReader<IStrm_T, AppenderType, BufferType>(output, streamInitFunc, ReaderCreation_basic(), FormatDefTag_runtime(), sFilePath, nCount, "DelimitedTextReader_basic", "CharAppenderStringViewCBuffer");
    }

#if ENABLE_FAST_CPP_CSV_PARSER
    template <class Read_T>
    DFG_NOINLINE void ExecuteTestCase_FastCppCsvParser(std::ostream& output, const std::string& sFilePath, const size_t nCount)
    {
        std::vector<double> runtimes;
        for (size_t i = 0; i < nCount; ++i)
        {
            DFG_STATIC_ASSERT(gnColCount == 7, "Code below assumes 7 columns.");
            ::io::CSVReader<7> fastCsvParser(sFilePath);
            fastCsvParser.read_header(io::ignore_no_column, "Column 0", "Column 1", "Column 2", "Column 3", "Column 4", "Column 5", "Column 6");
            Read_T c0, c1, c2, c3, c4, c5, c6;
            TimerType timer1;
            size_t nCounter = 0;
            while (fastCsvParser.read_row(c0, c1, c2, c3, c4, c5, c6))
            {
                nCounter += gnColCount;
                //EXPECT_TRUE(std::strcmp(c0, "abc") == 0 && std::strcmp(c6, "abc") == 0);
            }
            const auto elapsed1 = timer1.elapsedWallSeconds();
            EXPECT_EQ(gnExpectedCellCounterValueWithHeaderCells, nCounter + 7);
            runtimes.push_back(elapsed1);
        }
       
        PrintTestCaseRow(output, sFilePath, runtimes, "fast-cpp-csv-parser 2020-05-19", "compile time", std::string("\"Parse only, read type = ") + prettierTypeName<Read_T>() + std::string("\""), "N/A");
    }
#endif

    class CppCsvCellCounter : public cppcsv::per_cell_tag
    {
    public:
        CppCsvCellCounter() :
            m_nCellCounter(0)
        {}

        void begin_row()
        {
        }

        void cell(const char* /*buf*/, size_t /*len*/)
        {
            m_nCellCounter++;
        }

        void end_row()
        {
        }

        size_t m_nCellCounter;
    };

    template <class CommentsChar_T, class Quote_T, class Separator_T>
    DFG_NOINLINE void ExecuteTestCase_cppCsvImpl(std::ostream& output,
                                    const std::string& sFilePath,
                                    const size_t nCount,
                                    const Quote_T quote,
                                    const Separator_T sep,
                                    const char* pszFormatDef,
                                    const char* pszParseStyle)
    {
        std::vector<double> runtimes;
        for (size_t i = 0; i < nCount; ++i)
        {
            const auto bytes = dfg::io::fileToVector(sFilePath);
            CppCsvCellCounter cc;
            cppcsv::csv_parser<decltype(cc), Quote_T, Separator_T, CommentsChar_T> parser(cc, quote, sep, false, false);

            auto pData = bytes.data();
            TimerType timer1;
            parser.process(pData, bytes.size());

            const auto elapsed1 = timer1.elapsedWallSeconds();

            EXPECT_EQ(gnExpectedCellCounterValueWithHeaderCells, cc.m_nCellCounter);
            runtimes.push_back(elapsed1);
        }
        PrintTestCaseRow(output, sFilePath, runtimes, "cppcsv_ph_2020-01-10", pszFormatDef, pszParseStyle, "Contiguous memory");
    }

    DFG_NOINLINE void ExecuteTestCase_cppCsv(std::ostream& output, const std::string& sFilePath, const size_t nCount)
    {
        // Generic configuration
        ExecuteTestCase_cppCsvImpl<char>(output, sFilePath, nCount, '\0', ',', "runtime", "Parse only (cell counter)");

        // Disabled comments and quotes support, runtime separator
        ExecuteTestCase_cppCsvImpl<cppcsv::Disable>(output, sFilePath, nCount, cppcsv::Disable(), ',', "mixed", "Parse only (cell counter) + no(comments quotes)");

        // Disabled comments and quotes support, compile-time separator
        ExecuteTestCase_cppCsvImpl<cppcsv::Disable>(output, sFilePath, nCount, cppcsv::Disable(), cppcsv::Separator_Comma(), "compile time", "Parse only (cell counter) + no(comments quotes) + comma only");
    }

#if (ENABLE_CSV_PARSER_A == 1)
    // TODO: revise is this a correct way to use the library in this context
    DFG_NOINLINE void ExecuteTestCase_csvParserAria(std::ostream& output, const std::string& sFilePath, const size_t nCount)
    {
        std::vector<double> runtimes;
        for (size_t i = 0; i < nCount; ++i)
        {
            using namespace aria::csv;
            std::ifstream strm(sFilePath);

            size_t nCounter = 0;
            CsvParser parser(strm);

            TimerType timer1;
            for (;;)
            {
                auto field = parser.next_field();
                if (field.type == FieldType::DATA)
                    ++nCounter;
                else if (field.type == FieldType::CSV_END)
                    break;
            }
            const auto elapsed1 = timer1.elapsedWallSeconds();

            EXPECT_EQ(gnExpectedCellCounterValueWithHeaderCells, nCounter);
            runtimes.push_back(elapsed1);
        }
        PrintTestCaseRow(output, sFilePath, runtimes, "csv-parser_aria_2020-07-05", "runtime", "Parse only (cell counter)", "N/A");
    }
#endif // ENABLE_CSV_PARSER_A

 #if ENABLE_CSV_PARSER_V
    // TODO: revise is this a correct way to use the library in this context
    DFG_NOINLINE void ExecuteTestCase_csvParserVincent(std::ostream& output, const std::string& sFilePath, const size_t nCount)
    {
        std::vector<double> runtimes;
        for (size_t i = 0; i < nCount; ++i)
        {
            csv::CSVFormat format;
            format.delimiter(',').quote(false);
            csv::CSVReader reader(sFilePath, format);

            TimerType timer1;
            size_t nCounter = 0;
#if 1
            for (csv::CSVRow& row : reader)
            {
                for (csv::CSVField& field: row)
                {
                    DFG_UNUSED(field);
                    ++nCounter;
                }
            }
#else
            csv::CSVRow row;
            while (reader.read_row(row))
            {
                ++nCounter;
            }
#endif
            const auto elapsed1 = timer1.elapsedWallSeconds();

            EXPECT_EQ(gnExpectedCellCounterValueWithHeaderCells, nCounter + 7);
            runtimes.push_back(elapsed1);
        }
        PrintTestCaseRow(output, sFilePath, runtimes, "csv-parser_vincent_2020-07-05", "runtime", "Parse only (cell counter)", "N/A");
    }
#endif // ENABLE_CSV_PARSER_V    

    DFG_NOINLINE void ExecuteTestCase_TableCsv(std::ostream& output, const std::string& sFilePath, const size_t nRepeatCount, const bool bMultiThreaded = false)
    {
        using namespace DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(io);

        typedef ::DFG_MODULE_NS(cont)::TableCsv<char, uint32> Table;

        std::vector<double> runtimes;

        const auto nHwConcurrency = std::thread::hardware_concurrency();

        for (size_t i = 0; i < nRepeatCount; ++i)
        {
            TimerType timer;
            Table table;
            ::DFG_MODULE_NS(cont)::TableCsvReadWriteOptions options(',', DelimitedTextReader::s_nMetaCharNone, EndOfLineTypeN, encodingUTF8);
            if (bMultiThreaded)
            {
                options.setPropertyT<::DFG_MODULE_NS(cont)::TableCsvReadWriteOptions::PropertyId::readOpt_threadCount>(nHwConcurrency);
                options.setPropertyT< ::DFG_MODULE_NS(cont)::TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>(0);
            }
            table.readFromFile(sFilePath, options);
            const auto elapsedTime = timer.elapsedWallSeconds();
            EXPECT_EQ(gnRowCount + 1, table.rowCountByMaxRowIndex());
            EXPECT_EQ(gnColCount, table.colCountByMaxColIndex());
            if (bMultiThreaded)
                DFGTEST_EXPECT_LEFT(nHwConcurrency, table.readFormat().getReadStat<::DFG_MODULE_NS(cont)::TableCsvReadStat::threadCount>());
            else
                DFGTEST_EXPECT_LE(table.readFormat().getReadStat<::DFG_MODULE_NS(cont)::TableCsvReadStat::threadCount>(), 1u);
            
            runtimes.push_back(elapsedTime);
        }
        PrintTestCaseRow(output, sFilePath, runtimes, (bMultiThreaded) ? format_fmt("\"TableCsv<char,uint32> (with {} thread(s))\"", nHwConcurrency) : "\"TableCsv<char,uint32> (single-threaded)\"", "runtime", "Read&Store", "N/A");
    }

    template <class IStrm_T>
    size_t getThroughOriginal(IStrm_T& istrm)
    {
        using namespace DFG_MODULE_NS(io);
        size_t nCounter = 0;
        const auto cEofChar = eofChar(istrm);
        while (readOne(istrm) != cEofChar)
            nCounter++;
        return nCounter;
    }

    template <class IStrm_T>
    size_t getThroughIoGetThroughLambda(IStrm_T& istrm)
    {
        size_t nCounter = 0;
        DFG_MODULE_NS(io)::getThrough(istrm, [&](char) { nCounter++; });
        return nCounter;
    }

    struct GetThroughFunctor
    {
        GetThroughFunctor() : m_nCounter(0) {}
        size_t m_nCounter;
        void operator()(char) { m_nCounter++; }
    };

    template <class IStrm_T>
    size_t getThroughIoGetThroughFunctor(IStrm_T& istrm)
    {
        return DFG_MODULE_NS(io)::getThrough(istrm, GetThroughFunctor()).m_nCounter;
    }

    template <class IStrm_T, class IStrmInit_T, class GetThrough_T>
    DFG_NOINLINE void ExecuteTestCase_GetThrough(std::ostream& output, IStrmInit_T streamInitFunc, GetThrough_T getThroughFunc, const std::string& sFilePath, const size_t nCount, const std::string additionalDesc = std::string())
    {
        using namespace DFG_MODULE_NS(io);

        std::vector<double> runtimes;
        size_t nPreviousCounter = DFG_ROOT_NS::NumericTraits<size_t>::maxValue;

        for (size_t i = 0; i < nCount; ++i)
        {
            TimerType timer;
            FileByteHolder bytes;
            auto spStrm = streamInitFunc(sFilePath, bytes);
            EXPECT_TRUE(spStrm != nullptr);
            auto& istrm = *spStrm;

            const auto nCounter = getThroughFunc(istrm);

            const auto elapsedTime = timer.elapsedWallSeconds();

            EXPECT_TRUE(nCounter > 0);
            EXPECT_TRUE(i == 0 || nCounter == nPreviousCounter);
            nPreviousCounter = nCounter;

            runtimes.push_back(elapsedTime);
        }
        PrintTestCaseRow(output, sFilePath, runtimes, "readOne()", "", "Read through" + additionalDesc, StreamName<IStrm_T>());
    }

} // unnamed namespace


TEST(dfgPerformance, CsvReadPerformance)
{
    const char szOutputPath[] = "testfiles/generated/csvPerformanceResults.csv";
    const auto nOldOutputSize = ::DFG_MODULE_NS(os)::fileSize(szOutputPath);
    std::ofstream ostrmTestResults(szOutputPath, std::ios::app);

    const auto nRunCount = gnRunCount;

    // Printing header only if output file has no existing content
    if (nOldOutputSize == 0)
    {
        ostrmTestResults << "Date,Test machine,Compiler,Pointer size,Build type,File,Reader,Format definition,Processing type,Stream type";
        for (size_t i = 0; i < nRunCount; ++i)
        {
            ostrmTestResults << ",time#" << i + 1;
        }
        ostrmTestResults << ",time_avg,time_median,time_sum\n";
    }

    const auto sFilePath = GenerateTestFile(gnRowCount, gnColCount, false);
    const auto sFilePathEnclosed = GenerateTestFile(gnRowCount, gnColCount, true);

#if 0 // std:: stream tests disabled by default as they have been very slow at least in MSVC
    // std::ifstream
    ExecuteTestCase_GetThrough<std::ifstream>(ostrmTestResults, InitIfstream, getThroughOriginal<std::ifstream>, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_NoCharAppend<std::ifstream>(ostrmTestResults, InitIfstream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<std::ifstream>(ostrmTestResults, InitIfstream, sFilePath, nRunCount);

    // std::istrstream
    ExecuteTestCase_GetThrough<std::istrstream>(ostrmTestResults, InitIStrStream, getThroughOriginal<std::istrstream>, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_NoCharAppend<std::istrstream>(ostrmTestResults, InitIStrStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<std::istrstream>(ostrmTestResults, InitIStrStream, sFilePath, nRunCount);

    // std::istringstream
    ExecuteTestCase_GetThrough<std::istringstream>(ostrmTestResults, InitIStringStream, getThroughOriginal<std::istringstream>, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_NoCharAppend<std::istringstream>(ostrmTestResults, InitIStringStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<std::istringstream>(ostrmTestResults, InitIStringStream, sFilePath, nRunCount);
#endif

    // BasicIfStream
    ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<::DFG_MODULE_NS(io)::BasicIfStream>(ostrmTestResults, InitBasicIfstream, sFilePath, nRunCount, false);

    // IfStreamWithEncoding
    //ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<::DFG_MODULE_NS(io)::IfStreamWithEncoding>(ostrmTestResults, InitIfStreamWithEncoding, sFilePath, nRunCount, false);

    using BasicImStreamT = ::DFG_MODULE_NS(io)::BasicImStream;

    // BasicImStream, getThrough
    ExecuteTestCase_GetThrough<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, getThroughOriginal<BasicImStreamT>, sFilePath, nRunCount, " with get()");
    ExecuteTestCase_GetThrough<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, getThroughIoGetThroughLambda<BasicImStreamT>, sFilePath, nRunCount, " with io::getThrough() and lambda");
    ExecuteTestCase_GetThrough<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, getThroughIoGetThroughFunctor<BasicImStreamT>, sFilePath, nRunCount, " with io::getThrough() and functor");
    // BasicImStream, noCharAppend
    ExecuteTestCase_DelimitedTextReader_NoCharAppend<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_NoCharAppend<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount, true); //Basic reader
    // BasicImStream, default reader
    ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount, false); // Runtime format def
    ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount, true); // Compile time format def
    // BasicImStream, default reader with StringViewCBufferWithEnclosedCellSupport
    ExecuteTestCase_DelimitedTextReader_StringViewCBufferWithEnclosedCellSupport<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePath        , nRunCount, false); // Runtime format def
    ExecuteTestCase_DelimitedTextReader_StringViewCBufferWithEnclosedCellSupport<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePath        , nRunCount, true);  // Compile time format def
    ExecuteTestCase_DelimitedTextReader_StringViewCBufferWithEnclosedCellSupport<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePathEnclosed, nRunCount, false); // Runtime format def
    ExecuteTestCase_DelimitedTextReader_StringViewCBufferWithEnclosedCellSupport<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePathEnclosed, nRunCount, true);  // Compile time format def

    // BasicImStream, basicReader, default read buffer
    ExecuteTestCase_DelimitedTextReader_basicReader<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount, false); // Runtime format def
    ExecuteTestCase_DelimitedTextReader_basicReader<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount, true); // Compile time format def
    // BasicImStream, basicReader, stringViewBuffer
    ExecuteTestCase_DelimitedTextReader_basicReader_stringViewBuffer<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount, false, true);  // Runtime format def , \r\n translation
    ExecuteTestCase_DelimitedTextReader_basicReader_stringViewBuffer<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount, true,  true);  // Compile time format, \r\n translation
    ExecuteTestCase_DelimitedTextReader_basicReader_stringViewBuffer<BasicImStreamT>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount, true,  false); // Compile time format, no \r\n translation

    /////////////////////////////////////////////////
    // 3rd party libraries --->
    {
        // fast-cpp-csv-parser, https://github.com/ben-strasser/fast-cpp-csv-parser
#if ENABLE_FAST_CPP_CSV_PARSER
        ExecuteTestCase_FastCppCsvParser<const char*>(ostrmTestResults, sFilePath, nRunCount);
        ExecuteTestCase_FastCppCsvParser<std::string>(ostrmTestResults, sFilePath, nRunCount);
#endif

        // cppcsv, https://github.com/paulharris/cppcsv
        ExecuteTestCase_cppCsv(ostrmTestResults, sFilePath, nRunCount);

        // csv-parser, https://github.com/AriaFallah/csv-parser
#if ENABLE_CSV_PARSER_A
        ExecuteTestCase_csvParserAria(ostrmTestResults, sFilePath, nRunCount);
#endif // ENABLE_CSV_PARSER_A

        // csv-parser, https://github.com/vincentlaucsb/csv-parser
#if ENABLE_CSV_PARSER_V
        ExecuteTestCase_csvParserVincent(ostrmTestResults, sFilePath, nRunCount);
#endif // ENABLE_CSV_PARSER_V
    }
    // <---- 3rd party libraries
    /////////////////////////////////////////////////

    // TableCsv
    ExecuteTestCase_TableCsv(ostrmTestResults, sFilePath, nRunCount);
    ExecuteTestCase_TableCsv(ostrmTestResults, sFilePath, nRunCount, true /* multithreaded */);
    ExecuteTestCase_TableCsv(ostrmTestResults, sFilePathEnclosed, nRunCount);

    ostrmTestResults.close();
    //std::system("testfiles\\generated\\csvPerformanceResults.csv");
}

#endif
