#include <stdafx.h>

#if 0

#include <dfg/cont/tableCsv.hpp>
#include <dfg/preprocessor/compilerInfoMsvc.hpp>
#include <dfg/time/timerCpu.hpp>
#include <dfg/numeric/average.hpp>
#include <dfg/numeric/median.hpp>
#include <dfg/build/utils.hpp>
#include <dfg/io/fileToByteContainer.hpp>
#include <dfg/io/OfStream.hpp>
#include <iostream>
#include <strstream>
#include <sstream>



#if DFG_MSVC_VER >= DFG_MSVC_VER_2015
    DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
        #include <dfg/io/fast-cpp-csv-parser/csv.h>
    DFG_END_INCLUDE_WITH_DISABLED_WARNINGS
#endif

namespace
{
    typedef DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) TimerType;

    std::string GenerateTestFile(const size_t nRowCount, const size_t nColCount)
    {
        using namespace DFG_MODULE_NS(io);
        using namespace DFG_MODULE_NS(str);

        std::string sFilePath = "testfiles/generated/generatedCsv_r" + toStrC(nRowCount) + "_c" + toStrC(nColCount) + ".csv";

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
    std::unique_ptr<std::ifstream> InitIfstream(const std::string& sFilePath, std::vector<char>&)
    {
        return std::unique_ptr<std::ifstream>(new std::ifstream(sFilePath));
    }

    std::unique_ptr<std::istrstream> InitIStrStream(const std::string& sFilePath, std::vector<char>& byteContainer)
    {
        byteContainer = DFG_MODULE_NS(io)::fileToVector(sFilePath);
        return std::unique_ptr<std::istrstream>(new std::istrstream(byteContainer.data(), byteContainer.size()));
    }

    std::unique_ptr<std::istringstream> InitIStringStream(const std::string& sFilePath, std::vector<char>& byteContainer)
    {
        byteContainer = DFG_MODULE_NS(io)::fileToVector(sFilePath);
        return std::unique_ptr<std::istringstream>(new std::istringstream(std::string(byteContainer.data(), byteContainer.size())));
    }

    std::unique_ptr<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)> InitIBasicImStream(const std::string& sFilePath, std::vector<char>& byteContainer)
    {
        byteContainer = DFG_MODULE_NS(io)::fileToVector(sFilePath);
        return std::unique_ptr<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(new DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)(byteContainer.data(), byteContainer.size()));
    }

    

    template <class Strm_T> std::string StreamName() { DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(Strm_T, "Template specialization is missing for given type."); }

    template <> std::string StreamName<std::ifstream>() { return "std::ifstream";}
    template <> std::string StreamName<std::istrstream>() { return "std::istrstream"; }
    template <> std::string StreamName<std::istringstream>() { return "std::istringstream"; }
    template <> std::string StreamName<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>() { return "dfg::io::BasicImStream"; }

    template <class IStrm_T, class IStrmInit_T>
    void ExeceuteTestCase(std::ostream& output, IStrmInit_T streamInitFunc, const std::string& sFilePath, const size_t nCount)
    {
        using namespace DFG_MODULE_NS(io);

        std::vector<double> runtimes;

        for (size_t i = 0; i < nCount; ++i)
        {
            TimerType timer;
            std::vector<char> bytes;
            auto spStrm = streamInitFunc(sFilePath, bytes);
            EXPECT_TRUE(spStrm != nullptr);
            auto& istrm = *spStrm;
            typedef DFG_CLASS_NAME(DelimitedTextReader)::CellData<char, char, std::basic_string<char>, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderNone> Cdt;

            Cdt cellDataHandler(',', -1, '\n');
            auto reader = DFG_CLASS_NAME(DelimitedTextReader)::createReader(istrm, cellDataHandler);
            size_t nCounter = 0;
            auto cellHandler = [&](const size_t, const size_t, const decltype(cellDataHandler)&)
            {
                nCounter++;
            };
            DFG_CLASS_NAME(DelimitedTextReader)::read(reader, cellHandler);
            const auto elapsedTime = timer.elapsedWallSeconds();
            runtimes.push_back(elapsedTime);
        }
        output << sFilePath << ",Parse only,DelimitedTextReader," << StreamName<IStrm_T>() << ",";
        std::for_each(runtimes.begin(), runtimes.end(), [&](const double val) {output << val << ','; });
        output << DFG_MODULE_NS(numeric)::average(runtimes) << "," << DFG_MODULE_NS(numeric)::median(runtimes) << '\n';
    }

    void ExeceuteTestCaseFastCppCsvParser(std::ostream& output, const std::string& sFilePath, const size_t nCount)
    {
        std::vector<double> runtimes;
        for (size_t i = 0; i < nCount; ++i)
        {
            ::io::CSVReader<7> fastCsvParser(sFilePath);
            std::string c0, c1, c2, c3, c4, c5, c6;
            dfg::time::TimerCpu timer1;
            {
                size_t i = 0;
                while (fastCsvParser.read_row(c0, c1, c2, c3, c4, c5, c6))
                {
                }
            }
            const auto elapsed1 = timer1.elapsedWallSeconds();
            runtimes.push_back(elapsed1);
        }
       
        output << sFilePath << ",Parse only,fast-cpp-csv-parser," << "N/A" << ",";
        std::for_each(runtimes.begin(), runtimes.end(), [&](const double val) {output << val << ','; });
        output << DFG_MODULE_NS(numeric)::average(runtimes) << "," << DFG_MODULE_NS(numeric)::median(runtimes) << '\n';
    }
}

TEST(dfgCont, CsvReadPerformance)
{
    const size_t nRunCount = 3;

    std::ofstream ostrmTestResults("testfiles/generated/csvPerformanceResults.csv");

    // TODO: add compiler name & version, build config (debug/release)
    ostrmTestResults << "File,Processing type,reader,Stream type";
    for (size_t i = 0; i < nRunCount; ++i)
    {
        ostrmTestResults << ",time#" << i + 1;
    }
    if (nRunCount > 0)
        ostrmTestResults  << ",time_avg,time_median\n";

#ifndef _DEBUG
    const size_t nRowCount = 2000000;
#else
    const size_t nRowCount = 2000;
#endif
    const auto sFilePath = GenerateTestFile(nRowCount, 7);

    ExeceuteTestCase<std::ifstream>(ostrmTestResults, InitIfstream, sFilePath, nRunCount);
    ExeceuteTestCase<std::istrstream>(ostrmTestResults, InitIStrStream, sFilePath, nRunCount);
    ExeceuteTestCase<std::istringstream>(ostrmTestResults, InitIStringStream, sFilePath, nRunCount);
    ExeceuteTestCase<DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)>(ostrmTestResults, InitIBasicImStream, sFilePath, nRunCount);
    ExeceuteTestCaseFastCppCsvParser(ostrmTestResults, sFilePath, nRunCount);

    ostrmTestResults.close();
    //std::system("testfiles\\generated\\csvPerformanceResults.csv");
}

#endif

