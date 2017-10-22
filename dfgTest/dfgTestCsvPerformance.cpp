#include <stdafx.h>

#if 0 // On/off switch for the whole performance test.

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


#if ENABLE_FAST_CPP_CSV_PARSER
    DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
        #include <dfg/io/fast-cpp-csv-parser/csv.h>
    DFG_END_INCLUDE_WITH_DISABLED_WARNINGS
#endif

namespace
{
    #ifndef _DEBUG
        const size_t gnRowCount = 4000000;
    #else
        const size_t gnRowCount = 2000;
    #endif
    const size_t gnColCount = 7;

    const size_t gnRunCount = 5;

    typedef DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) TimerType;

    std::string GenerateTestFile(const size_t nRowCount, const size_t nColCount)
    {
        using namespace DFG_MODULE_NS(io);
        using namespace DFG_MODULE_NS(str);

        std::string sFilePath = "testfiles/generated/generatedCsv_r" + toStrC(nRowCount) + "_c" + toStrC(nColCount) + "_constant_content_abc_test.csv";

        TimerType timer;

        DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStream) ostrm(sFilePath);
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
            DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellStrm(ostrm, "abc", ',', '"', '\n', EbEncloseIfNeeded);
            for (size_t c = 1; c < nColCount; ++c)
            {
#if 1
                ostrm << ",abc";
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

    

    template <class Strm_T> std::string StreamName() { DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(Strm_T, "Template specialization is missing for given type."); }

    std::string fileToMemoryType()
    {
        return (std::is_same<DFG_MODULE_NS(os)::DFG_CLASS_NAME(MemoryMappedFile), FileByteHolder>::value) ? " (memory mapped)" : " (fileToVector)";
    }

    template <> std::string StreamName<std::ifstream>() { return "std::ifstream";}
    template <> std::string StreamName<std::istrstream>() { return "std::istrstream" + fileToMemoryType(); }
    template <> std::string StreamName<std::istringstream>() { return "std::istringstream" + fileToMemoryType(); }
    template <> std::string StreamName<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>() { return "dfg::io::BasicImStream" + fileToMemoryType(); }

    template <class T> std::string prettierTypeName();
    template <> std::string prettierTypeName<const char*>() { return "const char*"; }
    template <> std::string prettierTypeName<std::string>() { return "std::string"; }
    

    void PrintTestCaseRow(std::ostream& output, const std::string& sFilePath, const std::vector<double>& runtimes, const DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlySzParamC)& sReader, const DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlySzParamC)& sProcessingType, const std::string& sStreamType)
    {
        output  << DFG_MODULE_NS(time)::localDate_yyyy_mm_dd_C() << ",," << DFG_COMPILER_NAME_SIMPLE << ',' << sizeof(void*) << ','
                << DFG_BUILD_DEBUG_RELEASE_TYPE << ',' << sFilePath << "," << sReader << "," << sProcessingType << "," << sStreamType << ",";
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

    typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharBuffer<char> DefaultBufferType;
    
    template <class IStrm_T, class CharAppender_T, class Buffer_T, class IStrmInit_T, class ReaderImplementationOption_T>
    void ExecuteTestCaseDelimitedTextReader(std::ostream& output, IStrmInit_T streamInitFunc, ReaderImplementationOption_T readerOption, const std::string& sFilePath, const size_t nCount, const char* const pszReaderType, const char* const pszProcessingType)
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
            typedef DFG_CLASS_NAME(DelimitedTextReader)::FormatDefinitionSingleCharsCompileTime<DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone, '\n', ','> FormatDef;
            typedef DFG_CLASS_NAME(DelimitedTextReader)::CellData<char, char, Buffer_T, CharAppender_T, FormatDef> Cdt;

            FormatDef formatDef;
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
            //EXPECT_TRUE(nCounter > 0);
            //EXPECT_TRUE(i == 0 || nCounter == nPreviousCounter);
            nPreviousCounter = nCounter;

            runtimes.push_back(elapsedTime);
        }
        PrintTestCaseRow(output, sFilePath, runtimes, pszReaderType, pszProcessingType, StreamName<IStrm_T>());
    }

    template <class IStrm_T, class IStrmInit_T>
    void ExecuteTestCase_DelimitedTextReader_NoCharAppend(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount, const bool bBasicReader = false)
    {
        if (bBasicReader)
            ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderNone, DefaultBufferType>(output, streamInitFunc, ReaderCreation_basic(), sFilePath, nCount, "DelimitedTextReader_basic", "CharAppenderNone");
        else
            ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderNone, DefaultBufferType>(output, streamInitFunc, ReaderCreation_default(), sFilePath, nCount, "DelimitedTextReader", "CharAppenderNone");
    }

    template <class IStrm_T, class IStrmInit_T>
    void ExecuteTestCase_DelimitedTextReader_DefaultCharAppend(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount)
    {
        ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderDefault<DefaultBufferType, char>, DefaultBufferType>(output, streamInitFunc, ReaderCreation_default(), sFilePath, nCount, "DelimitedTextReader", "CharAppenderDefault");
    }

    template <class IStrm_T, class IStrmInit_T>
    void ExecuteTestCase_DelimitedTextReader_basicReader(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount)
    {
        ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderDefault<DefaultBufferType, char>, DefaultBufferType>(output, streamInitFunc, ReaderCreation_basic(), sFilePath, nCount, "DelimitedTextReader_basic", "CharAppenderDefault");
    }

    template <class IStrm_T, class IStrmInit_T>
    void ExecuteTestCase_DelimitedTextReader_basicReader_stringViewBuffer(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount)
    {
        typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::StringViewCBuffer BufferType;
        typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderStringViewCBuffer AppenderType;
        ExecuteTestCaseDelimitedTextReader<IStrm_T, AppenderType, BufferType>(output, streamInitFunc, ReaderCreation_basic(), sFilePath, nCount, "DelimitedTextReader_basic", "CharAppenderStringViewCBuffer");
    }

#if ENABLE_FAST_CPP_CSV_PARSER
    template <class Read_T>
    void ExecuteTestCase_FastCppCsvParser(std::ostream& output, const std::string& sFilePath, const size_t nCount)
    {
        std::vector<double> runtimes;
        size_t nPreviousCounter = DFG_ROOT_NS::NumericTraits<size_t>::maxValue;
        for (size_t i = 0; i < nCount; ++i)
        {
            DFG_STATIC_ASSERT(gnColCount == 7, "Code below assumes 7 columns.");
            ::io::CSVReader<7> fastCsvParser(sFilePath);
            fastCsvParser.read_header(io::ignore_no_column, "Column 0", "Column 1", "Column 2", "Column 3", "Column 4", "Column 5", "Column 6");
            Read_T c0, c1, c2, c3, c4, c5, c6;
            dfg::time::TimerCpu timer1;
            size_t nCounter = 0;
            while (fastCsvParser.read_row(c0, c1, c2, c3, c4, c5, c6))
            {
                nCounter++;
                //EXPECT_TRUE(std::strcmp(c0, "abc") == 0 && std::strcmp(c6, "abc") == 0);
            }
            const auto elapsed1 = timer1.elapsedWallSeconds();
            EXPECT_TRUE(nCounter > 0);
            EXPECT_TRUE(i == 0 || nCounter == nPreviousCounter);
            nPreviousCounter = nCounter;

            runtimes.push_back(elapsed1);
        }
       
        PrintTestCaseRow(output, sFilePath, runtimes, "fast-cpp-csv-parser 2017-02-17", std::string("\"Parse only, read type = ") + prettierTypeName<Read_T>() + std::string("\""), "N/A");
    }
#endif

    void ExecuteTestCase_TableCsv(std::ostream& output, const std::string& sFilePath, const size_t nCount)
    {
        using namespace DFG_ROOT_NS;

        typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TableCsv)<char, uint32> Table;

        std::vector<double> runtimes;

        for (size_t i = 0; i < nCount; ++i)
        {
            TimerType timer;
            Table table;
            table.readFromFile(sFilePath);
            EXPECT_EQ(gnRowCount + 1, table.rowCountByMaxRowIndex());
            EXPECT_EQ(gnColCount, table.colCountByMaxColIndex());
            const auto elapsedTime = timer.elapsedWallSeconds();

            runtimes.push_back(elapsedTime);
        }
        PrintTestCaseRow(output, sFilePath, runtimes, "\"TableCsv<char,uint32>\"", "Read&Store", "N/A");
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
    void ExecuteTestCase_GetThrough(std::ostream& output, IStrmInit_T streamInitFunc, GetThrough_T getThroughFunc, const std::string& sFilePath, const size_t nCount, const std::string additionalDesc = std::string())
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
        PrintTestCaseRow(output, sFilePath, runtimes, "readOne()", "Read through" + additionalDesc, StreamName<IStrm_T>());
    }

} // unnamed namespace


TEST(dfgPerformance, CsvReadPerformance)
{
    std::ofstream ostrmTestResults("testfiles/generated/csvPerformanceResults.csv");

    const auto nRunCount = gnRunCount;

    // TODO: add compiler name & version, build config (debug/release)
    ostrmTestResults << "Date,Test machine,Compiler,Pointer size,Build type,File,Reader,Processing type,Stream type";
    for (size_t i = 0; i < nRunCount; ++i)
    {
        ostrmTestResults << ",time#" << i + 1;
    }
    if (nRunCount > 0)
        ostrmTestResults  << ",time_avg,time_median,time_sum\n";

    const auto sFilePath = GenerateTestFile(gnRowCount, gnColCount);

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

    // BasicImStream
    ExecuteTestCase_GetThrough<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, getThroughOriginal<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>, sFilePath, nRunCount, " with get()");
    ExecuteTestCase_GetThrough<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, getThroughIoGetThroughLambda<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>, sFilePath, nRunCount, " with io::getThrough() and lambda");
    ExecuteTestCase_GetThrough<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, getThroughIoGetThroughFunctor<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>, sFilePath, nRunCount, " with io::getThrough() and functor");
    ExecuteTestCase_DelimitedTextReader_NoCharAppend<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_NoCharAppend<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount, true); //Basic reader
    ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_basicReader<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_basicReader_stringViewBuffer<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount);

    // fast-cpp-csv-parser
#if ENABLE_FAST_CPP_CSV_PARSER
    ExecuteTestCase_FastCppCsvParser<const char*>(ostrmTestResults, sFilePath, nRunCount);
    ExecuteTestCase_FastCppCsvParser<std::string>(ostrmTestResults, sFilePath, nRunCount);
#endif

    // TableCsv
    ExecuteTestCase_TableCsv(ostrmTestResults, sFilePath, nRunCount);

    ostrmTestResults.close();
    //std::system("testfiles\\generated\\csvPerformanceResults.csv");
}

#endif
