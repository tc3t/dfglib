#include <stdafx.h>
#include <dfg/io.hpp>
#include <dfg/rand.hpp>
#include <dfg/alg.hpp>
#include <dfg/str.hpp>
#include <dfg/cont.hpp>
#include <dfg/str.hpp>
#include <utility>
#include <strstream>
#include <boost/lexical_cast.hpp>
#include <boost/timer.hpp>
#include <strstream>
#include <dfg/io/BasicImStream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <numeric> // std::accumulate
#include <dfg/io/nullOutputStream.hpp>
#include <dfg/time/timerCpu.hpp>
#include <dfg/io/OmcStreamWithEncoding.hpp>
#include <dfg/io/ImStreamWithEncoding.hpp>
#include <dfg/utf.hpp>
#include <dfg/ioAll.hpp>
#include <dfg/build/languageFeatureInfo.hpp>
#include <dfg/io/OfStream.hpp>
#include <dfg/io/seekFwdToLine.hpp>
#include <dfg/str/strCat.hpp>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include <dlib/vectorstream.h>
    #include <dlib/compress_stream.h>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

#if 0
void CreateTestFile(const size_t nDim)
{
    using namespace DFG_ROOT_NS;
    const auto sDim = DFG_SUB_NS_NAME(str)::toStrC(nDim);
    std::ofstream ostrm("testfile_" + sDim + "x" + sDim + ".txt");
    std::ofstream ostrmCells("testfile_" + sDim + "x" + sDim + "_CellTexts.txt");

    const char separator[] = "\t";

    auto randEngine = DFG_SUB_NS_NAME(rand)::createDefaultRandEngineUnseeded();
    randEngine.seed(578964);
    auto cellLengthGenerator = DFG_SUB_NS_NAME(rand)::makeDistributionEngineUniform(&randEngine, size_t(0), size_t(30));
    // Note: Char can't be used in std::uniform_int_distribution, see
    // https://connect.microsoft.com/VisualStudio/feedback/details/856484/std-uniform-int-distribution-does-not-like-char
    //auto cellCharGenerator = DFG_SUB_NS_NAME(rand)::makeDistributionEngineUniform(&randEngine, char(32), char(127));
    auto cellCharGenerator = DFG_SUB_NS_NAME(rand)::makeDistributionEngineUniform(&randEngine, int(32), int(127));

    const auto writeNewCellText = [&](std::string& str)
                                    {
                                        const auto nLength = cellLengthGenerator();
                                        str.resize(nLength);
                                        ::DFG_ROOT_NS::DFG_SUB_NS_NAME(alg)::forEachFwd(str, [&](char& ch)
                                        {
                                            ch = static_cast<char>(cellCharGenerator());
                                        });

                                        replaceSubStrsInplace(str, "\"", "\"\"");
                                        ostrm << str;
                                        ostrmCells << str;
                                    };

    std::string str;
    for(size_t r = 0; r<nDim; ++r)
    {
        writeNewCellText(str);
        for(size_t c = 1; c<nDim; ++c)
        {
            ostrm << separator;
            writeNewCellText(str);
        }
        ostrm << '\n';
    }
}
#endif

void CreateTestMatrixFile(const size_t nDim)
{
    using namespace DFG_ROOT_NS;
    const auto sDim = DFG_SUB_NS_NAME(str)::toStrC(nDim);
    std::ofstream ostrm("matrix_" + sDim + "x" + sDim + ".txt");

    const char separator[] = ", ";

    auto randEngine = DFG_SUB_NS_NAME(rand)::createDefaultRandEngineUnseeded();
    randEngine.seed((unsigned int)std::time(nullptr));
    auto randDistrEng = DFG_SUB_NS_NAME(rand)::makeDistributionEngineUniform(&randEngine, int(0), int(50000));
    for(size_t r = 0; r<nDim; ++r)
    {
        ostrm << randDistrEng();
        for(size_t c = 1; c<nDim; ++c)
        {
            ostrm << separator << randDistrEng();
        }
        ostrm << '\n';
    }
}

TEST(dfgIo, readBytes)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char szFilePath[] = "testfiles/matrix_3x3.txt";

    // Test reading from BasicImStream
    {
        char arrDest[DFG_COUNTOF(szFilePath)] = "";
        DFG_CLASS_NAME(BasicImStream) imStrm(szFilePath, DFG_COUNTOF_SZ(szFilePath));
        const auto nRead = readBytes(imStrm, arrDest, sizeof(arrDest));
        EXPECT_EQ(DFG_COUNTOF_SZ(szFilePath), nRead);
        lastOf(arrDest) = '\0';
        EXPECT_STREQ(szFilePath, arrDest);
    }

    // Test reading from BasicIfStream
    {
        char arrDest[DFG_COUNTOF(szFilePath)] = "";
        DFG_CLASS_NAME(BasicIfStream) ifStrm(szFilePath);
        const auto nRead = readBytes(ifStrm, arrDest, 5);
        EXPECT_EQ(5, nRead);
        arrDest[5] = '\0';
        EXPECT_STREQ("8925,", arrDest);
    }

    // Test reading from std::istrstream
    {
        char arrDest[DFG_COUNTOF(szFilePath)] = "";
        std::istrstream imStrm(szFilePath, DFG_COUNTOF_SZ(szFilePath));
        const auto nRead = readBytes(imStrm, arrDest, sizeof(arrDest));
        EXPECT_EQ(DFG_COUNTOF_SZ(szFilePath), nRead);
        lastOf(arrDest) = '\0';
        EXPECT_STREQ(szFilePath, arrDest);
    }

    // Test reading from std::ifstream
    {
        char arrDest[DFG_COUNTOF(szFilePath)] = "";
        std::ifstream ifStrm(szFilePath);
        const auto nRead = readBytes(ifStrm, arrDest, 5);
        EXPECT_EQ(5, nRead);
        arrDest[5] = '\0';
        EXPECT_STREQ("8925,", arrDest);
    }

    // Test reading bytes from wchar_t buffer with BasicImStream_T
    {
        char arrDest[DFG_COUNTOF(szFilePath)] = "";
        const wchar_t szData[4] = L"dfg";
        DFG_CLASS_NAME(BasicImStream_T)<wchar_t> imStrm(szData, DFG_COUNTOF_SZ(szData));
        const auto nRead = readBytes(imStrm, arrDest, 2 * sizeof(wchar_t));
        EXPECT_EQ(2 * sizeof(wchar_t), nRead);
        arrDest[3] = '\0';
        EXPECT_EQ(L'd', *reinterpret_cast<const wchar_t*>(arrDest));
        EXPECT_EQ(L'f', *reinterpret_cast<const wchar_t*>(arrDest + sizeof(wchar_t)));
    }

    // The same as above for std::wistream
    {
        char arrDest[DFG_COUNTOF(szFilePath)] = "";
        const wchar_t szData[4] = L"dfg";
        std::wistringstream imStrm(szData);
        const auto nRead = readBytes(imStrm, arrDest, 2 * sizeof(wchar_t));
        EXPECT_EQ(2 * sizeof(wchar_t), nRead);
        arrDest[3] = '\0';
        EXPECT_EQ(L'd', *reinterpret_cast<const wchar_t*>(arrDest));
        EXPECT_EQ(L'f', *reinterpret_cast<const wchar_t*>(arrDest + sizeof(wchar_t)));
    }

    // Test reading bytes from wchar_t buffer with uneven byte count.
    {
        char arrDest[DFG_COUNTOF(szFilePath)] = "";
        const wchar_t szData[4] = L"dfg";
        DFG_CLASS_NAME(BasicImStream_T)<wchar_t> imStrm(szData, DFG_COUNTOF_SZ(szData));
        const auto nRead = readBytes(imStrm, arrDest, sizeof(wchar_t) + 1);
        EXPECT_EQ(sizeof(wchar_t), nRead);
        EXPECT_EQ(L'd', *reinterpret_cast<const wchar_t*>(arrDest));
    }

    // The same as above for std::wistream.
    {
        char arrDest[DFG_COUNTOF(szFilePath)] = "";
        const wchar_t szData[4] = L"dfg";
        std::wistringstream imStrm(szData);
        const auto nRead = readBytes(imStrm, arrDest, sizeof(wchar_t) + 1);
        EXPECT_EQ(sizeof(wchar_t), nRead);
        EXPECT_EQ(L'd', *reinterpret_cast<const wchar_t*>(arrDest));
    }
}

TEST(dfgIo, DFG_CLASS_NAME(BasicIfStream))
{
    const size_t nSumExpected = 12997224;
    const char szFilePath[] = "testfiles/matrix_200x200.txt";
    typedef DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) Timer;

    {
    Timer timer;
    size_t sum = 0;
    DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(BasicIfStream) istrm(szFilePath);
    int ch;
    while((ch = istrm.get()) != istrm.eofVal())
        sum += ch;
    std::cout << "BasicIfStream elapsed: " << timer.elapsedWallSeconds() << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    {
    Timer timer;
    size_t sum = 0;
    std::ifstream istrm(szFilePath, std::ios_base::in | std::ios_base::binary);
    int ch;
    while((ch = istrm.get()) != std::ifstream::traits_type::eof())
        sum += ch;
    std::cout << "std::ifstream elapsed: " << timer.elapsedWallSeconds() << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    {
    Timer timer;
    size_t sum = 0;
    boost::iostreams::stream<boost::iostreams::file_source> istrm(szFilePath, std::ios_base::in | std::ios_base::binary);
    int ch;
    while((ch = istrm.get()) != EOF)
        sum += ch;
    std::cout << "boost::iostreams::stream<file_source> elapsed: " << timer.elapsedWallSeconds() << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    {
    Timer timer;
    size_t sum = 0;
    std::ifstream istrm(szFilePath, std::ios_base::in | std::ios_base::binary);
    std::vector<char> buffer(6778362);
    istrm.read(buffer.data(), buffer.size());
    DFG_ROOT_NS::DFG_SUB_NS_NAME(alg)::forEachFwd(buffer, [&](char c) {sum += c;});
    std::cout << "std::ifstream to buffer elapsed: " << timer.elapsedWallSeconds() << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    {
    Timer timer;
    size_t sum = 0;
    DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(BasicIfStream) istrm(szFilePath);
    std::vector<char> buffer(6778362);
    istrm.read(buffer.data(), buffer.size());
    DFG_ROOT_NS::DFG_SUB_NS_NAME(alg)::forEachFwd(buffer, [&](char c) {sum += c;});
    std::cout << "BasicIfStream to buffer elapsed: " << timer.elapsedWallSeconds() << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }
}

TEST(dfgIo, StdIStrStreamPerformance)
{
#ifdef _DEBUG
    auto buffer = DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::fileToVector("testfiles/matrix_200x200.txt", 271040);
    const size_t nSumExpected = 12997224;
#else
    #if 1
        auto buffer = DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::fileToVector("testfiles/matrix_1000x1000.txt", 6778362);
        EXPECT_FALSE(buffer.empty());
        const size_t nSumExpected = 325307649;
    #else
        auto buffer = DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::fileToVector("testfiles/matrix_2000x2000.txt", 27111152);
        EXPECT_FALSE(buffer.empty());
        const size_t nSumExpected = 1301215741;
    #endif
#endif

    DFG_CLASS_NAME(NullOutputStream) nullStream;
    auto& strmSumPrint = nullStream; // std::cout;

    // Direct memory read
    {
    boost::timer timer;
    size_t sum = 0;
    DFG_ROOT_NS::DFG_SUB_NS_NAME(alg)::forEachFwd(buffer, [&](char c) {sum += c;});
    std::cout << "Direct memory read elapsed: " << timer.elapsed() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    // Direct memory read with std::accumulate
    {
    boost::timer timer;
    size_t sum = 0;
    sum = std::accumulate(buffer.cbegin(), buffer.cend(), sum);
    std::cout << "Direct memory read with std::accumulate elapsed: " << timer.elapsed() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    // std::istrstream with get()
    {
    boost::timer timer;
    size_t sum = 0;
    std::istrstream istrm(static_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    int ch;
    while((ch = istrm.get()) != EOF)
        sum += ch;
    std::cout << "std::istrstream with get elapsed: " << timer.elapsed() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    // std::istrstream with read()
    {
    boost::timer timer;
    size_t sum = 0;
    std::istrstream istrm(static_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    char ch;
    while(istrm.read(&ch, 1))
        sum += ch;
    std::cout << "std::istrstream with read elapsed: " << timer.elapsed() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    // BasicImStream
    {
    boost::timer timer;
    size_t sum = 0;
    DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(BasicImStream) istrm(buffer.data(), buffer.size());
    int ch;
    while((ch = istrm.get()) != istrm.eofVal())
        sum += ch;
    std::cout << "BasicImStream with get elapsed: " << timer.elapsed() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    // boost::iostreams
    {
    boost::timer timer;
    size_t sum = 0;
    boost::iostreams::stream<boost::iostreams::array_source> istrm(buffer.data(), buffer.size());
    int ch;
    while((ch = istrm.get()) != EOF)
        sum += ch;
    std::cout << "boost::iostreams::stream<boost::iostreams::array_source> with get elapsed: " << timer.elapsed() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    // dlib vectorstream
    {
    boost::timer timer;
    size_t sum = 0;
    dlib::vectorstream istrm(buffer);
    int ch;
    while((ch = istrm.get()) != EOF)
        sum += ch;
    std::cout << "dlib::vectorstream with get elapsed: " << timer.elapsed() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }
}

namespace
{
    template <class Strm_T>
    void getTest()
    {
        using namespace DFG_MODULE_NS(io);

        const char input[] = "abcdef";
        std::string output;
        Strm_T istrm(input, DFG_COUNTOF_SZ(input));
        char ch;
        while (istrm.get(ch))
            output.push_back(ch);

        std::string output2;
        Strm_T istrm2(input, DFG_COUNTOF_SZ(input));
        decltype(istrm.get()) ch2;
        while ((ch2 = istrm2.get()) != eofChar(istrm2))
            output2.push_back(static_cast<char>(ch2));

        EXPECT_EQ(DFG_COUNTOF_SZ(input), output.size());
        EXPECT_EQ(input, output);
        EXPECT_EQ(output, output2);
    }
}

TEST(dfgIo, BasicImStream)
{
    const char szData[] = "012345";

    using namespace DFG_MODULE_NS(io);

    {
        DFG_CLASS_NAME(BasicImStream) istrm(szData, DFG_COUNTOF_CSL(szData));

        char c;
        readBinary(istrm, c);
        EXPECT_EQ('0', c);
        char c2[6];
        readBinary(istrm, c2);
        EXPECT_EQ('1', c2[0]);
        EXPECT_EQ('2', c2[1]);
        EXPECT_EQ('3', c2[2]);
        EXPECT_EQ('4', c2[3]);
        EXPECT_EQ('5', c2[4]);
        EXPECT_FALSE(istrm.good());
    }

    // get-testing
    {
        getTest<DFG_CLASS_NAME(BasicImStream)>();
        getTest<std::istrstream>();
    }
}

TEST(dfgIo, BasicImStream_T_wcharT)
{
    const wchar_t szData[] = L"012345";

    using namespace DFG_MODULE_NS(io);

    DFG_CLASS_NAME(BasicImStream_T)<wchar_t> istrm(szData, DFG_COUNTOF_CSL(szData));

    wchar_t c;
    c = istrm.get();
    EXPECT_EQ('0', c);
    wchar_t c2[6];
    istrm.read(c2, 6);
    EXPECT_EQ('1', c2[0]);
    EXPECT_EQ('2', c2[1]);
    EXPECT_EQ('3', c2[2]);
    EXPECT_EQ('4', c2[3]);
    EXPECT_EQ('5', c2[4]);
    EXPECT_FALSE(istrm.good());
}

TEST(dfgIo, OmcStreamWithEncoding)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);
    using namespace DFG_MODULE_NS(utf);

    const char szAnsi[] = "abc";
    const std::string sAnsiLatin1(szAnsi, &szAnsi[0] + DFG_COUNTOF_CSL(szAnsi));
    const std::string sAnsiUtf8 = latin1ToUtf8(sAnsiLatin1);

    const char szTestLatin1[] = { 'u', 't', 'f', -28, '_', 0 };
    const std::string sLatin1(szTestLatin1, &szTestLatin1[0] + DFG_COUNTOF_CSL(szTestLatin1));
    const std::string sUtf8 = latin1ToUtf8(sLatin1);
    const auto sUtf16Le = utf8ToFixedChSizeStr<char16_t>(sUtf8, ByteOrderLittleEndian);
    const std::string sUtf16LeBytes(reinterpret_cast<const char*>(sUtf16Le.c_str()), 2 * sUtf16Le.size());
    const auto sUtf16Be = utf8ToFixedChSizeStr<char16_t>(sUtf8, ByteOrderBigEndian);
    const std::string sUtf16BeBytes(reinterpret_cast<const char*>(sUtf16Be.c_str()), 2 * sUtf16Be.size());
    const auto sUtf32Le = utf8ToFixedChSizeStr<char32_t>(sUtf8, ByteOrderLittleEndian);
    const std::string sUtf32LeBytes(reinterpret_cast<const char*>(sUtf32Le.c_str()), 4 * sUtf32Le.size());
    const auto sUtf32Be = utf8ToFixedChSizeStr<char32_t>(sUtf8, ByteOrderBigEndian);
    const std::string sUtf32BeBytes(reinterpret_cast<const char*>(sUtf32Be.c_str()), 4 * sUtf32Be.size());

    const auto testCaseImpl = [&](TextEncoding encoding, const size_t nExpectedSize0, const std::string& sExp0)
    {
        std::string s;
#if DFG_LANGFEAT_MOVABLE_STREAMS
        auto ostrm = createOmcStreamWithEncoding(&s, encoding);
#else
        DFG_CLASS_NAME(OmcStreamWithEncoding)<std::string> ostrm(&s, encoding);
#endif

        ostrm << szTestLatin1;
        EXPECT_EQ(nExpectedSize0, s.size());
        EXPECT_EQ(nExpectedSize0, ostrm.size());
        EXPECT_EQ(ostrm.size(), ostrm.container().size());
        //EXPECT_EQ(nExpectedSize0, ostrm.tellp()); // tellp() is not updated in ostrm
        EXPECT_EQ(sExp0, s);
        EXPECT_TRUE(memcmp(s.data(), ostrm.data(), s.size()) == 0);
        EXPECT_EQ(s.data(), ostrm.data());
        const auto s2 = ostrm.releaseData();
        EXPECT_EQ(true, s.empty());
        EXPECT_EQ(sExp0, s2);
        EXPECT_EQ(&s, &ostrm.container()); // Verify that releaseData() did not release the buffer reference.

        ostrm.write(szAnsi, DFG_COUNTOF_CSL(szAnsi));
        if (s.size() == DFG_COUNTOF_CSL(szAnsi))
        {
            for (size_t i = 0; i < DFG_COUNTOF_CSL(szAnsi); ++i)
            {
                EXPECT_EQ(szAnsi[i], ostrm.container()[i]);
                EXPECT_EQ(szAnsi[i], s[i]);
            }
        }

        ostrm.clearBufferWithoutDeallocAndSeekToBegin();
        EXPECT_TRUE(s.empty());
        ostrm << szTestLatin1;
        EXPECT_EQ(sExp0, s);
        const auto s3 = ostrm.releaseDataAndBuffer();
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(sExp0, s3);
        EXPECT_NE(&s, &ostrm.container()); // Verify that releaseDataAndBuffer() changed the buffer.

        // Test writeUnicodeChar()
        {
            using namespace DFG_ROOT_NS;
            using namespace DFG_MODULE_NS(io);
            using namespace DFG_MODULE_NS(utf);

            const auto nWritten = ostrm.writeUnicodeChar(0x20AC);
            EXPECT_TRUE(nWritten >= 2);
            const auto& cont = ostrm.container();
            const auto nSize = cont.size();
            const auto bytes = makeRange(cont.data() + nSize - nWritten, cont.data() + nSize);
            std::vector<uint32> dest;
            if (encoding == encodingUTF8)
            {
                utfToFixedChSizeStrIter<uint32>(bytes, std::back_inserter(dest), dfg::ByteOrderHost, dfg::ByteOrderHost, [](const UnconvertableCpHandlerParam&){return 0; });
                EXPECT_TRUE(!dest.empty());
                EXPECT_EQ(0x20AC, dest.front());
            }
        }
    };

    testCaseImpl(encodingUTF8, 6, sUtf8);
    testCaseImpl(encodingUTF16Le, 10, sUtf16LeBytes);
    testCaseImpl(encodingUTF16Be, 10, sUtf16BeBytes);
    testCaseImpl(encodingUTF32Le, 20, sUtf32LeBytes);
    testCaseImpl(encodingUTF32Be, 20, sUtf32BeBytes);

    // TODO: write UCS data
}

namespace
{
    template <class Strm_T>
    void ImStreamWithEncodingImpl(Strm_T& istrm, const int nCharSize)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);
        
        EXPECT_EQ('a', istrm.get());
        const auto pos = istrm.tellg();
        EXPECT_EQ('b', istrm.get());
        EXPECT_EQ('c', istrm.get());
        istrm.seekg(-1 * nCharSize, std::ios_base::cur);
        EXPECT_TRUE(istrm.good());
        EXPECT_EQ('c', istrm.get());
        istrm.seekg(pos);
        EXPECT_TRUE(istrm.good());
        EXPECT_EQ('b', istrm.get());
    }
} // unnamed namespace

TEST(dfgIo, ImStreamWithEncoding)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);
    const char dataC[] = "abcd";
    const wchar_t dataW[] = L"abcd";
    
    {
        DFG_CLASS_NAME(ImStreamWithEncoding) istrmC(dataC, DFG_MODULE_NS(str)::strLen(dataC), encodingUnknown);
        DFG_CLASS_NAME(ImStreamWithEncoding) istrmW(dataW, DFG_MODULE_NS(str)::strLen(dataW));
        ImStreamWithEncodingImpl(istrmC, 1);
        ImStreamWithEncodingImpl(istrmW, sizeof(wchar_t));
    }

    // Test how read()-function works.
    {
        const char dataC2[2] = { -84, 0x20 }; // Euro-sign in UTF16Le
        DFG_CLASS_NAME(ImStreamWithEncoding) istrmC(dataC2, 2, encodingUTF16Le);
        char bufferC[2];
        //wchar_t bufferW[2];
        istrmC.read(bufferC, 2);
        const auto nRead = istrmC.gcount();
        EXPECT_EQ(1, nRead);
        istrmC.clear();
        istrmC.seekg(0);
        const wchar_t val = static_cast<wchar_t>(istrmC.get());
        EXPECT_EQ(0x20AC, val);
        //istrmC.read(bufferW, 2); // TODO: implement read for wchar_t argument.
    }
}

TEST(dfgIo, ImStreamWithEncoding_UCS)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char dataCLe2[2] = { -84, 0x20 }; // Euro-sign in UTF16Le
    const char dataCBe2[2] = { 0x20, -84 }; // Euro-sign in UTF16Be
    const char dataCLe4[4] = { -84, 0x20, 0, 0 }; // Euro-sign in UTF32Le
    const char dataCBe4[4] = { 0, 0, 0x20, -84 }; // Euro-sign in UTF32Be

    // UCS2Le
    {
        DFG_CLASS_NAME(ImStreamWithEncoding) istrm(dataCLe2, 2, encodingUCS2Le);
        auto val = istrm.get();
        EXPECT_EQ(0x20AC, val);
    }

    // UCS2Be
    {
        DFG_CLASS_NAME(ImStreamWithEncoding) istrm(dataCBe2, 2, encodingUCS2Be);
        auto val = istrm.get();
        EXPECT_EQ(0x20AC, val);
    }

    // UCS4Le
    {
        DFG_CLASS_NAME(ImStreamWithEncoding) istrm(dataCLe4, 4, encodingUCS4Le);
        auto val = istrm.get();
        EXPECT_EQ(0x20AC, val);
    }

    // UCS4Be
    {
        DFG_CLASS_NAME(ImStreamWithEncoding) istrm(dataCBe4, 4, encodingUCS4Be);
        auto val = istrm.get();
        EXPECT_EQ(0x20AC, val);
    }
}

namespace
{
    template <class IStrm_T>
    void IStreamWithEncoding_Windows1252_impl(IStrm_T&& istrm)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);
        int val = istrm.get();
        size_t nGetCount = 0;
        do
        {
            EXPECT_EQ(DFG_MODULE_NS(utf)::windows1252charToCp(static_cast<uint8>(nGetCount)), static_cast<size_t>(val));
            nGetCount++;
            val = istrm.get();
        } while (istrm);
        EXPECT_EQ(256, nGetCount);
    }
}

TEST(dfgIo, IStreamWithEncoding_Windows1252)
{
    using namespace DFG_MODULE_NS(io);

    std::vector<char> bytes(256);
    DFG_MODULE_NS(alg)::generateAdjacent(bytes, char(0), char(1));
    {
        DFG_CLASS_NAME(OfStream) ostrm("testfiles/generated/all_bytes_from_0_to_255.bin");
        ostrm.write(bytes.data(), bytes.size());
    }

    // ImStreamWithEncoding
    {
        
        DFG_CLASS_NAME(ImStreamWithEncoding) istrm(bytes.data(), bytes.size(), encodingWindows1252);
        IStreamWithEncoding_Windows1252_impl(istrm);
    }

    // IfStreamWithEncoding
    {
        DFG_CLASS_NAME(IfStreamWithEncoding) istrm("testfiles/generated/all_bytes_from_0_to_255.bin", encodingWindows1252);
        IStreamWithEncoding_Windows1252_impl(istrm);
    }
}

TEST(dfgIo, OByteStream)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();
    auto distrEng = DFG_MODULE_NS(rand)::makeDistributionEngineUniform(&randEng, -128, 127);

    std::vector<char> bytes;
    for (size_t i = 0; i < 200*sizeof(double); ++i)
        bytes.push_back(static_cast<int8>(distrEng()));

    dlib::compress_stream::kernel_1ea compressorEa;

    DFG_CLASS_NAME(ImcByteStream) istrm(bytes.data(), bytes.size());
    DFG_CLASS_NAME(OmcByteStream)<> ostrmCompressed;

    compressorEa.compress(istrm, ostrmCompressed);

    std::vector<double> doubles;

#if DFG_LANGFEAT_MOVABLE_STREAMS
    auto ostrmToDoubleArray = makeOmcByteStream(doubles);
#else
    DFG_CLASS_NAME(OmcByteStream)<std::vector<double>> ostrmToDoubleArray(&doubles);
#endif

    DFG_CLASS_NAME(ImcByteStream) istrm2(ostrmCompressed.data(), ostrmCompressed.size());
    compressorEa.decompress(istrm2, ostrmToDoubleArray);

    EXPECT_EQ(bytes.size(), sizeInBytes(doubles));
    EXPECT_FALSE(std::memcmp(bytes.data(), doubles.data(), bytes.size()));
}

TEST(dfgIo, ImcByteStream)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char buffer[] = "ImcByteStream\n\r\n";
    char readBuffer[DFG_COUNTOF_CSL(buffer)];

    DFG_CLASS_NAME(ImcByteStream) istrm(buffer, DFG_COUNTOF_CSL(buffer));
    readBinary(istrm, readBuffer);
    EXPECT_FALSE(memcmp(buffer, readBuffer, count(readBuffer)));
}

TEST(dfgIo, FileMemoryMapped)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    DFG_CLASS_NAME(FileMemoryMapped) istrm("testfiles/matrix_3x3.txt");
    EXPECT_TRUE(istrm.is_open());
    EXPECT_TRUE(istrm.size() == 62);

    std::vector<char> bytes(istrm.begin(), istrm.end());
    const auto vec2 = fileToVector(L"testfiles/matrix_3x3.txt");
    EXPECT_EQ(bytes, vec2);

    {
        DFG_CLASS_NAME(IfmmStream) istrm3("testfiles/matrix_3x3.txt");
        std::vector<char> vec3;
        char ch;
        while (istrm3.get(ch))
            vec3.push_back(ch);
        EXPECT_EQ(vec2, vec3);
    }

    // Test that open() will reset istream bits.
    {
        DFG_CLASS_NAME(IfmmStream) istrm4;
        istrm4.open("testfiles/matrix_3x3.txt");
        EXPECT_TRUE(istrm4.is_open());
        EXPECT_TRUE(istrm4.good());
        std::vector<char> vec4;
        char ch;
        while (istrm4.get(ch))
            vec4.push_back(ch);
        EXPECT_EQ(vec2, vec4);
    }
}

TEST(dfgIo, StreamBufferMem)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);
    using namespace DFG_MODULE_NS(cont);

    wchar_t wbuffer[] = L"abc";

    {
        DFG_CLASS_NAME(StreamBufferMem)<wchar_t> strmBuf(wbuffer, 3);

        std::basic_istream<wchar_t> strm(&strmBuf);
        std::vector<wchar_t> vecRead;
        const auto vecExpected = makeArray<wchar_t>('a', 'b', 'c');
        wchar_t ch;
        while (strm.get(ch))
            vecRead.push_back(ch);
        EXPECT_EQ(vecExpected.size(), vecRead.size());
        EXPECT_TRUE(std::equal(vecExpected.begin(), vecExpected.end(), vecRead.begin()));
    }

    //
    {
        DFG_CLASS_NAME(StreamBufferMem)<char>    sbC(nullptr, 0);
        DFG_CLASS_NAME(StreamBufferMem)<wchar_t> sbW(nullptr, 0);
        EXPECT_EQ(std::char_traits<char>::eof(), sbC.uflow());
        EXPECT_EQ(std::char_traits<char>::eof(), sbC.underflow());
        EXPECT_EQ(std::char_traits<wchar_t>::eof(), sbW.uflow());
        EXPECT_EQ(std::char_traits<wchar_t>::eof(), sbW.underflow());

    }
}

TEST(dfgIo, StreamBufferMemSeeking)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char buffer[] = { 'a', 'b', 'c' };
    std::istrstream strmStd(buffer, DFG_COUNTOF(buffer));
    DFG_CLASS_NAME(StreamBufferMem)<char> strmBuf(buffer, DFG_COUNTOF(buffer));
    std::istream strmStrmBuf(&strmBuf);
    strmStd.seekg(DFG_COUNTOF(buffer), std::ios::cur);
    strmStrmBuf.seekg(DFG_COUNTOF(buffer), std::ios::cur);
    EXPECT_EQ(strmStd.rdstate(), strmStrmBuf.rdstate());
    EXPECT_EQ(strmStd.eof(), strmStrmBuf.eof());
    strmStd.seekg(1, std::ios::cur);
    strmStrmBuf.seekg(1, std::ios::cur);
    EXPECT_EQ(strmStd.rdstate(), strmStrmBuf.rdstate());
    EXPECT_EQ(strmStd.eof(), strmStrmBuf.eof());
}

TEST(dfgIo, StreamBufferMemWithEncoding)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);
    //const char szTest[] = "abc";
}

TEST(dfgIo, IfStreamBufferWithEncoding)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);



}

#ifndef __MINGW32__ // IfStreamWithEncoding crashes in line m_memoryMappedFile.open(sPath); with MinGW for unknown reason.
TEST(dfgIo, IfStreamWithEncoding)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);
 
    const char szNonExistentPath[] = "testfiles/nonExistentFile.invalid";

    // Test handling of non-existent path.
    {
        DFG_CLASS_NAME(IfStreamWithEncoding) strm(szNonExistentPath);
        std::ifstream strmStd(szNonExistentPath);
        EXPECT_EQ(strmStd.rdstate(), strm.rdstate());
        EXPECT_EQ(strmStd.good(), strm.good());
    }

    // Test seeking
    {
        DFG_CLASS_NAME(IfStreamWithEncoding) istrm("testfiles/matrix_3x3.txt");
        char bytes[3];
        EXPECT_TRUE(istrm.good());
        readBytes(istrm, bytes, 3);
        EXPECT_TRUE(istrm.good());
        istrm.seekg(-3, std::ios_base::cur);
        EXPECT_EQ(bytes[0], istrm.get());
        EXPECT_TRUE(istrm.good());
        istrm.seekg(1, std::ios_base::cur);
        EXPECT_EQ(bytes[2], istrm.get());
        EXPECT_TRUE(istrm.good());
    }
}

TEST(dfgIo, IfStreamWithEncodingReadOnlyFile)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char szPath[] = "testfiles/IfStreamWithEncodingReadOnlyFile.temp";

    OfStreamWithEncoding ostrm(szPath, encodingUTF32Be);
    const char szText[] = "text";
    ostrm << SzPtrUtf8(szText);
    ostrm.flush(); // Note: this also tests OfStreamWithEncoding::flush().

    DFG_CLASS_NAME(IfStreamWithEncoding) strm(szPath);
    std::ifstream strmStd(szPath);
    EXPECT_EQ(strmStd.is_open(), strm.is_open());
    EXPECT_EQ(strmStd.good(), strm.good());
    std::string s;
    strm >> s;
    EXPECT_EQ(szText, s);
}

TEST(DfgIo, IfStreamWithEncoding_rawByteReading)
{
    // Test to verify that it is possible to easily read plain bytes from UTF8-data with IfStreamWithEncoding.

    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char szFilePath[] = "testfiles/utf8_with_BOM.txt";

    auto bytes0 = fileToVector(szFilePath);
    ASSERT_TRUE(bytes0.size() > 3);
    bytes0.erase(bytes0.begin(), bytes0.begin() + 3); // Erase BOM.

    DFG_CLASS_NAME(IfStreamWithEncoding) istrm;
    istrm.open("testfiles/utf8_with_BOM.txt");

    EXPECT_EQ(encodingUTF8, istrm.encoding());

    istrm.setEncoding(encodingNone);
    const auto bytes1 = readAllFromStream<std::vector<char>>(istrm);

    EXPECT_EQ(bytes0, bytes1);
}
#endif // __MINGW32__

TEST(dfgIo, OfStreamWithEncoding)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char szFileUtf8[] = "testfiles/generated/OfStreamWithEncodingUtf8.txt";
    const char szFileUtf16Le[] = "testfiles/generated/OfStreamWithEncodingUtf16Le.txt";

    {
        DFG_CLASS_NAME(OfStreamWithEncoding) ostrm(szFileUtf8, encodingUTF8);
        std::string s = "abc_\xe4 def";
        ostrm << DFG_CLASS_NAME(StringViewLatin1)(SzPtrLatin1(s.c_str()), s.size());

        ostrm.close();

        const auto readBytes = fileToByteContainer<std::string>(szFileUtf8);
        const char expectedUtf8[] = { -17, -69, -65, 'a', 'b', 'c', '_', -61, -92, ' ', 'd', 'e', 'f', '\0' };
        EXPECT_EQ(expectedUtf8, readBytes);
        DFG_CLASS_NAME(ImStreamWithEncoding) istrm(readBytes.data(), readBytes.size(), encodingUnknown);
        std::string sRead;
        char ch;
        while (istrm.get(ch))
            sRead.push_back(ch);
        EXPECT_EQ(s, sRead);

        DFG_CLASS_NAME(IfStreamWithEncoding) ifstreamWithEncoding(szFileUtf8);
        std::string sRead2;
        while (ifstreamWithEncoding.get(ch))
            sRead2.push_back(ch);
        EXPECT_EQ(s, sRead2);

    }

    {
        DFG_CLASS_NAME(OfStreamWithEncoding) ostrm(szFileUtf16Le, encodingUTF16Le);
        std::string s = "abc_\xe4 def";
        ostrm << DFG_CLASS_NAME(StringViewLatin1)(SzPtrLatin1(s.c_str()), s.size());
        ostrm.close();

        const auto readBytes = fileToByteContainer<std::string>(szFileUtf16Le);
        const std::array<char, 20> expectedUtf16Le = { -1, -2, 'a', 0, 'b', 0, 'c', 0, '_', 0, -28, 0, ' ', 0, 'd', 0, 'e', 0, 'f', 0 };
        EXPECT_EQ(expectedUtf16Le.size(), readBytes.size());
        EXPECT_TRUE(std::equal(expectedUtf16Le.begin(), expectedUtf16Le.end(), readBytes.begin()));
        DFG_CLASS_NAME(ImStreamWithEncoding) istrm(readBytes.data(), readBytes.size(), encodingUnknown);
        std::string sRead;
        char ch;
        while (istrm.get(ch))
            sRead.push_back(ch);
        EXPECT_EQ(s, sRead);

        DFG_CLASS_NAME(IfStreamWithEncoding) ifstreamWithEncoding(szFileUtf16Le);
        std::string sRead2;
        while (ifstreamWithEncoding.get(ch))
            sRead2.push_back(ch);
        EXPECT_EQ(s, sRead2);
    }

    {
        const char szFileName[] = "testfiles/generated/OfStreamWithEncodingMiscTests.txt";
        const auto encoding = encodingUTF8;
        DFG_CLASS_NAME(OfStreamWithEncoding) ostrm(szFileName, encoding);
        const char szBuf[] = "asd";
        ostrm.writeBytes(std::string(4, 'a'));
        ostrm.writeBytes(szBuf);
        ostrm.writeBytes(szBuf, DFG_COUNTOF_SZ(szBuf));
        ostrm << DFG_UTF8("e");
        ostrm.close();
        const auto readBytes = fileToByteContainer<std::string>(szFileName);
        const auto bomBytes = DFG_MODULE_NS(utf)::encodingToBom(encoding);
        EXPECT_EQ(std::string(bomBytes.begin(), bomBytes.end()) + std::string("aaaaasd\0asde", DFG_COUNTOF_SZ("aaaaasd\0asde")), readBytes);
    }

    // Test writing numeric types.
    {
        const char szFileName[] = "testfiles/generated/OfStreamWithEncodingNumericTests.txt";
        const auto encoding = encodingUTF8;
        DFG_CLASS_NAME(OfStreamWithEncoding) ostrm(szFileName, encoding);
        ostrm << int64_min;
        ostrm << uint64_max;
        ostrm << std::numeric_limits<float>::lowest();
        ostrm << std::numeric_limits<double>::lowest();
        ostrm << 0.3;
        ostrm.close();
        const auto readBytes = fileToByteContainer<std::string>(szFileName);
        const auto bomBytes = DFG_MODULE_NS(utf)::encodingToBom(encoding);
        EXPECT_EQ(std::string(bomBytes.begin(), bomBytes.end()) + 
                    DFG_MODULE_NS(str)::toStrC(int64_min) + 
                    DFG_MODULE_NS(str)::toStrC(uint64_max) +
                    DFG_MODULE_NS(str)::toStrC(std::numeric_limits<float>::lowest()) +
                    DFG_MODULE_NS(str)::toStrC(std::numeric_limits<double>::lowest()) +
                    "0.3"
                , readBytes);
    }
}

TEST(dfgIo, OfStreamWithEncoding_write)
{
    // Test that write() handles all bytes correctly.
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char szFilename[] = "testfiles/generated/OfStreamWithEncoding_writeTest.txt";

    DFG_CLASS_NAME(OfStreamWithEncoding) ostrm(szFilename, encodingLatin1);
    for (int i = 0; i < 256; ++i)
    {
        auto uc = static_cast<uint8>(i);
        ostrm.write(reinterpret_cast<const char*>(&uc), 1);
    }
    EXPECT_TRUE(ostrm.good());
    ostrm.close();

    const auto bytes = fileToVector(szFilename);
    EXPECT_EQ(256, bytes.size());
    if (bytes.size() == 256)
    {
        for (int i = 0; i < 256; ++i)
        {
            EXPECT_EQ(i, static_cast<uint8>(bytes[i]));
        }
    }
}

namespace
{
    template <class Stream_T, size_t N>
    void istreamNegativeCharsImpl(Stream_T& strm, int(&arr)[N], const int(&arrExpected)[N])
    {
        using namespace DFG_MODULE_NS(io);
        arr[0] = strm.get();
        arr[1] = strm.get();
        arr[2] = strm.get();
        EXPECT_TRUE(eofChar(strm) != arr[2]);
        char ch;
        strm.get(ch);
        arr[3] = ch;
        EXPECT_TRUE(strm.good());
        arr[4] = strm.get();
        if (&arr != &arrExpected)
        {
            EXPECT_EQ(arrExpected[0], arr[0]);
            EXPECT_EQ(arrExpected[1], arr[1]);
            EXPECT_EQ(arrExpected[2], arr[2]);
            EXPECT_EQ(arrExpected[3], arr[3]);
            EXPECT_EQ(arrExpected[4], arr[4]);
        }
    }
}

TEST(dfgIo, istreamNegativeChars)
{
    using namespace DFG_MODULE_NS(io);
    const char sz[] = { 'a', -28, -1, -1, 'b', '\0' };

    int readStd[DFG_COUNTOF_SZ(sz)];
    std::istrstream istrmStd(sz, DFG_COUNTOF_SZ(sz));
    istreamNegativeCharsImpl(istrmStd, readStd, readStd);

    DFG_CLASS_NAME(BasicImStream) istrmBasic(sz, DFG_COUNTOF_SZ(sz));
    int readBasic[DFG_COUNTOF_SZ(sz)];
    istreamNegativeCharsImpl(istrmBasic, readBasic, readStd);

    DFG_CLASS_NAME(ImStreamWithEncoding) istrmEncoded(sz, DFG_COUNTOF_SZ(sz), encodingLatin1);
    int readEncoded[DFG_COUNTOF_SZ(sz)];
    istreamNegativeCharsImpl(istrmEncoded, readEncoded, readStd);

    DFG_CLASS_NAME(StreamBufferMem)<char> strmBuf(sz, DFG_COUNTOF_SZ(sz));
    int readStreamBufferMem[DFG_COUNTOF_SZ(sz)];
    std::istream strmStreamBufMem(&strmBuf);
    istreamNegativeCharsImpl(strmStreamBufMem, readStreamBufferMem, readStd);

    DFG_CLASS_NAME(StreamBufferMemWithEncoding) strmBufEncoding(sz, DFG_COUNTOF_SZ(sz), encodingUnknown);
    int readStreamBufferMemWithEncoding[DFG_COUNTOF_SZ(sz)];
    std::istream strmStreamBufMemWithEncoding(&strmBufEncoding);
    istreamNegativeCharsImpl(strmStreamBufMemWithEncoding, readStreamBufferMemWithEncoding, readStd);
}

TEST(dfgIo, IfmmStream)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    DFG_CLASS_NAME(IfmmStream) istrmBin("testfiles/smallBinary.binary");
    std::array<unsigned char, 10> arrExpected = { 0x41, 0x10, 0x09, 0x0d, 0x0a, 0xff, 0xef, 0x0d, 0x55, 0x46 };
    std::vector<char> vecFromData(istrmBin.data(), istrmBin.data() + istrmBin.size());
    std::vector<char> vecFromGet;
    char ch;
    while (istrmBin.get(ch))
    {
        vecFromGet.push_back(ch);
    }
    EXPECT_EQ(arrExpected.size(), vecFromData.size());
    EXPECT_EQ(arrExpected.size(), vecFromGet.size());
    EXPECT_TRUE(memcmp(arrExpected.data(), vecFromData.data(), arrExpected.size()) == 0);
    EXPECT_EQ(vecFromData, vecFromGet);
}

TEST(dfgIo, writeDelimited)
{
    using namespace DFG_MODULE_NS(io);
    std::array<char, 5> arr = { 'a', 'b', 'c', 'd', 'e' };

    DFG_CLASS_NAME(OmcByteStream)<std::string> strm;

    writeDelimited(strm, arr, ',');
    EXPECT_EQ("a,b,c,d,e", strm.container());
}

TEST(dfgIo, seekFwdToLine)
{
    // TODO: add tests.
    using namespace DFG_MODULE_NS(io);
    DFG_CLASS_NAME(IStringStream) istrm;
    seekFwdToLineBeginningWith(istrm, "a", dfg::CaseSensitivityYes, '\n'); // Just a dummy line to test building.
}

namespace
{
    template <class Strm_T>
    static void getThroughImpl()
    {
        using namespace DFG_MODULE_NS(io);
        
        {
            const char input[] = "abcdef";
            std::string output;
            Strm_T istrm(input, DFG_COUNTOF_SZ(input));

            getThrough(istrm, [&](char ch) { output.push_back(ch); });

            EXPECT_EQ(DFG_COUNTOF_SZ(input), output.size());
            EXPECT_EQ(input, output);
        }
        
        // Test reading empty sequence.
        {
            std::string output;
            Strm_T istrm((const char*)nullptr, 0);
            output.clear();
            getThrough(istrm, [&](char ch) { output.push_back(ch); }); 
            EXPECT_EQ(0, output.size());
        }
    }
}

TEST(dfgIo, getThrough)
{
    using namespace DFG_MODULE_NS(io);
    DFGTEST_STATIC(DFG_DETAIL_NS::io_hpp::Has_getThrough<DFG_CLASS_NAME(BasicImStream)>::value == true);
    DFGTEST_STATIC(DFG_DETAIL_NS::io_hpp::Has_getThrough<std::ifstream>::value == false);
    DFGTEST_STATIC(DFG_DETAIL_NS::io_hpp::Has_getThrough<std::istrstream>::value == false);

    getThroughImpl<DFG_CLASS_NAME(BasicImStream)>();
    getThroughImpl<std::istrstream>();
}
