#include <stdafx.h>

#if 0 // On/off switch for the whole performance test.

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

#define DFG_COMPILER_NAME_SIMPLE_VC2010          "MSVC_2010"
#define DFG_COMPILER_NAME_SIMPLE_VC2012          "MSVC_2012"
#define DFG_COMPILER_NAME_SIMPLE_VC2013          "MSVC_2013"
#define DFG_COMPILER_NAME_SIMPLE_VC2015_RTM      "MSVC_2015_rtm"
#define DFG_COMPILER_NAME_SIMPLE_VC2015_UPDATE1  "MSVC_2015_u1"
#define DFG_COMPILER_NAME_SIMPLE_VC2015_UPDATE2  "MSVC_2015_u2"

#if DFG_MSVC_VER == DFG_MSVC_VER_2010
    #define DFG_COMPILER_NAME_SIMPLE DFG_COMPILER_NAME_SIMPLE_VC2010
#elif DFG_MSVC_VER == DFG_MSVC_VER_2012
    #define DFG_COMPILER_NAME_SIMPLE DFG_COMPILER_NAME_SIMPLE_VC2012
#elif DFG_MSVC_VER == DFG_MSVC_VER_2013
    #define DFG_COMPILER_NAME_SIMPLE DFG_COMPILER_NAME_SIMPLE_VC2013
#elif defined(_MSC_FULL_VER) && _MSC_FULL_VER == 190023026  // Compiler versions are from http://stackoverflow.com/questions/30760889/unknown-compiler-version-while-compiling-boost-with-msvc-14-0-vs-2015
    #define DFG_COMPILER_NAME_SIMPLE DFG_COMPILER_NAME_SIMPLE_VC2015_RTM
#elif defined(_MSC_FULL_VER) && _MSC_FULL_VER == 190023506
    #define DFG_COMPILER_NAME_SIMPLE DFG_COMPILER_NAME_SIMPLE_VC2015_UPDATE1
#elif defined(_MSC_FULL_VER) && _MSC_FULL_VER == 190023918
    #define DFG_COMPILER_NAME_SIMPLE DFG_COMPILER_NAME_SIMPLE_VC2015_UPDATE2
#elif defined(__MINGW32__)
    #define DFG_COMPILER_NAME_SIMPLE "MinGW_" DFG_STRINGIZE(__GNUC__) "." DFG_STRINGIZE(__GNUC_MINOR__) "." DFG_STRINGIZE(__GNUC_PATCHLEVEL__)
#else
    #error "Compiler name is not defined."
#endif

#ifdef _DEBUG
    #define DFG_BUILD_DEBUG_RELEASE_TYPE    "debug"
#else
    #define DFG_BUILD_DEBUG_RELEASE_TYPE    "release"
#endif

#if DFG_MSVC_VER >= DFG_MSVC_VER_2015
    DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
        #include <dfg/io/fast-cpp-csv-parser/csv.h>
    DFG_END_INCLUDE_WITH_DISABLED_WARNINGS
#endif

namespace
{
    #ifndef _DEBUG
        const size_t nRowCount = 4000000;
    #else
        const size_t nRowCount = 2000;
    #endif
    const size_t nColCount = 7;

    const size_t nRunCount = 5;

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
    

    void PrintTestCaseRow(std::ostream& output, const std::string& sFilePath, const std::vector<double>& runtimes, const char* const pszReader, const char* const pszProcessingType, const std::string& sStreamType)
    {
        output  << DFG_MODULE_NS(time)::localDate_yyyy_mm_dd_C() << ",," << DFG_COMPILER_NAME_SIMPLE << ',' << sizeof(void*) << ','
                << DFG_BUILD_DEBUG_RELEASE_TYPE << ',' << sFilePath << "," << pszReader << "," << pszProcessingType << "," << sStreamType << ",";
        std::for_each(runtimes.begin(), runtimes.end(), [&](const double val) {output << val << ','; });
        output  << DFG_MODULE_NS(numeric)::average(runtimes) << ","
                << DFG_MODULE_NS(numeric)::median(runtimes) << ","
                << DFG_MODULE_NS(numeric)::accumulate(runtimes, double(0)) << '\n';
    }

    template <class IStrm_T, class CharAppender_T, class IStrmInit_T>
    void ExecuteTestCaseDelimitedTextReader(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount, const char* const pszReaderType, const char* const pszProcessingType)
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
            typedef DFG_CLASS_NAME(DelimitedTextReader)::CellData<char, char, std::basic_string<char>, CharAppender_T> Cdt;

            Cdt cellDataHandler(',', -1, '\n');
            auto reader = DFG_CLASS_NAME(DelimitedTextReader)::createReader(istrm, cellDataHandler);

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
    void ExecuteTestCase_DelimitedTextReader_NoCharAppend(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount)
    {
        ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderNone>(output, streamInitFunc, sFilePath, nCount, "DelimitedTextReader", "CharAppenderNone");
    }

    template <class IStrm_T, class IStrmInit_T>
    void ExecuteTestCase_DelimitedTextReader_DefaultCharAppend(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount)
    {
        ExecuteTestCaseDelimitedTextReader<IStrm_T, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderDefault<std::string, char>>(output, streamInitFunc, sFilePath, nCount, "DelimitedTextReader", "CharAppenderDefault");
    }

#if DFG_MSVC_VER >= DFG_MSVC_VER_2015
    void ExecuteTestCase_FastCppCsvParser(std::ostream& output, const std::string& sFilePath, const size_t nCount)
    {
        std::vector<double> runtimes;
        size_t nPreviousCounter = DFG_ROOT_NS::NumericTraits<size_t>::maxValue;
        for (size_t i = 0; i < nCount; ++i)
        {
            ::io::CSVReader<7> fastCsvParser(sFilePath);
            std::string c0, c1, c2, c3, c4, c5, c6;
            dfg::time::TimerCpu timer1;
            size_t nCounter = 0;
            while (fastCsvParser.read_row(c0, c1, c2, c3, c4, c5, c6))
            {
                nCounter++;
            }
            const auto elapsed1 = timer1.elapsedWallSeconds();
            EXPECT_TRUE(nCounter > 0);
            EXPECT_TRUE(i == 0 || nCounter == nPreviousCounter);
            nPreviousCounter = nCounter;

            runtimes.push_back(elapsed1);
        }
       
        PrintTestCaseRow(output, sFilePath, runtimes, "fast-cpp-csv-parser", "Parse only", "N/A");
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
            EXPECT_EQ(nRowCount + 1, table.rowCountByMaxRowIndex());
            EXPECT_EQ(nColCount, table.colCountByMaxColIndex());
            const auto elapsedTime = timer.elapsedWallSeconds();

            runtimes.push_back(elapsedTime);
        }
        PrintTestCaseRow(output, sFilePath, runtimes, "\"TableCsv<char,uint32>\"", "Read&Store", "N/A");
    }

    template <class IStrm_T, class IStrmInit_T>
    void ExecuteTestCase_GetThrough(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount)
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

            size_t nCounter = 0;

            const auto cEofChar = eofChar(istrm);
            while (readOne(istrm) != cEofChar)
                nCounter++;

            const auto elapsedTime = timer.elapsedWallSeconds();

            EXPECT_TRUE(nCounter > 0);
            EXPECT_TRUE(i == 0 || nCounter == nPreviousCounter);
            nPreviousCounter = nCounter;

            runtimes.push_back(elapsedTime);
        }
        PrintTestCaseRow(output, sFilePath, runtimes, "readOne()", "Read through", StreamName<IStrm_T>());
    }

} // unnamed namespace


TEST(dfgPerformance, CsvReadPerformance)
{
    std::ofstream ostrmTestResults("testfiles/generated/csvPerformanceResults.csv");

    // TODO: add compiler name & version, build config (debug/release)
    ostrmTestResults << "Date,Test machine,Compiler,Pointer size,Build type,File,Reader,Processing type,Stream type";
    for (size_t i = 0; i < nRunCount; ++i)
    {
        ostrmTestResults << ",time#" << i + 1;
    }
    if (nRunCount > 0)
        ostrmTestResults  << ",time_avg,time_median,time_sum\n";

    const auto sFilePath = GenerateTestFile(nRowCount, nColCount);

    // std::ifstream
    ExecuteTestCase_GetThrough<std::ifstream>(ostrmTestResults, InitIfstream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_NoCharAppend<std::ifstream>(ostrmTestResults, InitIfstream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<std::ifstream>(ostrmTestResults, InitIfstream, sFilePath, nRunCount);

    // std::istrstream
    ExecuteTestCase_GetThrough<std::istrstream>(ostrmTestResults, InitIStrStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_NoCharAppend<std::istrstream>(ostrmTestResults, InitIStrStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<std::istrstream>(ostrmTestResults, InitIStrStream, sFilePath, nRunCount);

    // std::istringstream
    ExecuteTestCase_GetThrough<std::istringstream>(ostrmTestResults, InitIStringStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_NoCharAppend<std::istringstream>(ostrmTestResults, InitIStringStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<std::istringstream>(ostrmTestResults, InitIStringStream, sFilePath, nRunCount);

    // BasicImStream
    ExecuteTestCase_GetThrough<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_NoCharAppend<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount);
    ExecuteTestCase_DelimitedTextReader_DefaultCharAppend<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount);

    // fast-cpp-csv-parser
#if DFG_MSVC_VER >= DFG_MSVC_VER_2015
    ExecuteTestCase_FastCppCsvParser(ostrmTestResults, sFilePath, nRunCount);
#endif

    // TableCsv
    ExecuteTestCase_TableCsv(ostrmTestResults, sFilePath, nRunCount);

    ostrmTestResults.close();
    //std::system("testfiles\\generated\\csvPerformanceResults.csv");
}

#endif
