#include "stdafx.h"

#if (defined(DFGTEST_BUILD_MODULE_IO) && DFGTEST_BUILD_MODULE_IO == 1) || (!defined(DFGTEST_BUILD_MODULE_IO) && DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/io.hpp>
#include <dfg/rand.hpp>
#include <dfg/alg.hpp>
#include <dfg/str.hpp>
#include <dfg/cont.hpp>
#include <dfg/str.hpp>
#include <utility>
#include <strstream>
#include <dfg/io/BasicImStream.hpp>
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
#include <dfg/cont/tableCsv.hpp>
#include <dfg/os/memoryMappedFile.hpp>
#include <dfg/stdcpp/stdversion.hpp>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include <boost/lexical_cast.hpp>
    #include <boost/iostreams/device/array.hpp>
    #include <boost/iostreams/device/file.hpp>
    #include <boost/iostreams/stream.hpp>
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

TEST(dfgIo, BasicIfStream)
{
    using namespace ::DFG_ROOT_NS;

    /*
    matrix_3x3has \r\n EOL and content
    8925, 25460, 46586
    14510, 26690, 41354
    17189, 42528, 49812 
    */
    const char szFilePath3x3[] = "testfiles/matrix_3x3.txt";

    {
        using namespace ::DFG_MODULE_NS(cont);
        using namespace ::DFG_MODULE_NS(io);
        BasicIfStream istrm(szFilePath3x3);
        const size_t nBufferSize = 10;
        EXPECT_EQ(nBufferSize, istrm.bufferSize(nBufferSize));
        EXPECT_EQ(nBufferSize, istrm.bufferSize());
        EXPECT_EQ(nBufferSize, istrm.bufferSize(NumericTraits<size_t>::maxValue));

        std::vector<char> storage(100);
        EXPECT_EQ('8', istrm.get());
        EXPECT_EQ(nBufferSize - 1, istrm.bufferedByteCount());
        EXPECT_EQ(1, istrm.tellg());
        EXPECT_TRUE(istrm.seekg(0));
        EXPECT_EQ(nBufferSize, istrm.bufferSize());
        EXPECT_EQ(0, istrm.tellg());
        const auto nWholeRead = istrm.readBytes(storage.data(), storage.size());
        EXPECT_EQ(62, nWholeRead);
        storage.resize(nWholeRead);
        EXPECT_TRUE(isEqualContent(storage, fileToVector(szFilePath3x3)));
        EXPECT_EQ(62, istrm.tellg());
        EXPECT_EQ(istrm.eofVal(), istrm.get());

        // Seeking back to beginning
        std::fill(storage.begin(), storage.end(), '\0');
        EXPECT_TRUE(istrm.seekg(BasicIfStream::SeekOriginCurrent, -62));
        EXPECT_EQ(0, istrm.tellg());
        EXPECT_EQ(3, istrm.readBytes(storage.data(), Min(size_t(3), storage.size())));
        EXPECT_EQ('8', storage[0]);
        EXPECT_EQ('9', storage[1]);
        EXPECT_EQ('2', storage[2]);
        EXPECT_EQ(nBufferSize - 3, istrm.bufferedByteCount());

        // Skipping next
        EXPECT_TRUE(istrm.seekg(BasicIfStream::SeekOriginCurrent, 1));
        EXPECT_EQ(',', istrm.get());
        EXPECT_EQ(nBufferSize - 1, istrm.bufferedByteCount());

        // seekg/tellg tests
        {
            EXPECT_TRUE(istrm.seekg(BasicIfStream::SeekOriginEnd, 0));
            const auto nPosAtEnd = istrm.tellg();
            EXPECT_TRUE(istrm.seekg(BasicIfStream::SeekOriginBegin, 10));
            EXPECT_EQ(10, istrm.tellg());
            EXPECT_TRUE(istrm.seekg(BasicIfStream::SeekOriginEnd, -nPosAtEnd + 10));
            EXPECT_EQ(10, istrm.tellg());
            EXPECT_TRUE(istrm.seekg(BasicIfStream::SeekOriginBegin, 10));
            EXPECT_EQ(10, istrm.tellg());
        }
    }

    // seekg() overflow test
    {
        using namespace ::DFG_MODULE_NS(cont);
        using namespace ::DFG_MODULE_NS(io);
        BasicIfStream istrm(szFilePath3x3);
        char buffer[5];
        istrm.readBytes(buffer, sizeof(buffer));
        EXPECT_FALSE(istrm.seekg(BasicIfStream::SeekOriginCurrent, minValueOfType<BasicIfStream::OffType>()));
    }

    // Move constructor
    {
        using namespace ::DFG_MODULE_NS(cont);
        using namespace ::DFG_MODULE_NS(io);
        BasicIfStream istrm(szFilePath3x3);
        EXPECT_EQ('8', istrm.get());
        const auto nBufferedCount = istrm.bufferedByteCount();
        auto istrm2 = std::move(istrm);
        EXPECT_EQ(0, istrm.bufferedByteCount());
        EXPECT_EQ(BasicIfStream::eofVal(), istrm.get());
        EXPECT_EQ(nBufferedCount, istrm2.bufferedByteCount());
        EXPECT_EQ('9', istrm2.get());
    }

    const size_t nSumExpected = 12997224;
    const char szFilePath[] = "testfiles/matrix_200x200.txt";
    const size_t nFileSize = 271040; // Size of file in szFilePath
    typedef DFG_MODULE_NS(time)::TimerCpu Timer;

    {
    Timer timer;
    size_t sum = 0;
    DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::BasicIfStream istrm(szFilePath);
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
    std::vector<char> buffer(nFileSize);
    istrm.read(buffer.data(), buffer.size());
    DFG_ROOT_NS::DFG_SUB_NS_NAME(alg)::forEachFwd(buffer, [&](char c) {sum += c;});
    std::cout << "std::ifstream to buffer elapsed: " << timer.elapsedWallSeconds() << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    {
    Timer timer;
    size_t sum = 0;
    DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::BasicIfStream istrm(szFilePath);
    std::vector<char> buffer(nFileSize);
    istrm.read(buffer.data(), buffer.size());
    DFG_ROOT_NS::DFG_SUB_NS_NAME(alg)::forEachFwd(buffer, [&](char c) {sum += c;});
    std::cout << "BasicIfStream to buffer elapsed: " << timer.elapsedWallSeconds() << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }
}

TEST(dfgIo, StdIStrStreamPerformance)
{
#ifdef DFG_BUILD_TYPE_DEBUG
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

    ::DFG_MODULE_NS(io)::NullOutputStream nullStream;
    auto& strmSumPrint = nullStream; // std::cout;

    typedef ::DFG_MODULE_NS(time)::TimerCpu Timer;

    // Direct memory read
    {
    Timer timer;
    size_t sum = 0;
    DFG_ROOT_NS::DFG_SUB_NS_NAME(alg)::forEachFwd(buffer, [&](char c) {sum += c;});
    std::cout << "Direct memory read elapsed: " << timer.elapsedWallSeconds() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    // Direct memory read with std::accumulate
    {
    Timer timer;
    size_t sum = 0;
    sum = std::accumulate(buffer.cbegin(), buffer.cend(), sum);
    std::cout << "Direct memory read with std::accumulate elapsed: " << timer.elapsedWallSeconds() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    // std::istrstream with get()
    {
    Timer timer;
    size_t sum = 0;
    std::istrstream istrm(static_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    int ch;
    while((ch = istrm.get()) != EOF)
        sum += ch;
    std::cout << "std::istrstream with get elapsed: " << timer.elapsedWallSeconds() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    // std::istrstream with read()
    {
    Timer timer;
    size_t sum = 0;
    std::istrstream istrm(static_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    char ch;
    while(istrm.read(&ch, 1))
        sum += ch;
    std::cout << "std::istrstream with read elapsed: " << timer.elapsedWallSeconds() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    // BasicImStream
    {
    Timer timer;
    size_t sum = 0;
    DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(BasicImStream) istrm(buffer.data(), buffer.size());
    int ch;
    while((ch = istrm.get()) != istrm.eofVal())
        sum += ch;
    std::cout << "BasicImStream with get elapsed: " << timer.elapsedWallSeconds() << '\n';
    strmSumPrint << "Sum: " << sum << '\n';
    EXPECT_EQ(nSumExpected, sum);
    }

    // boost::iostreams
    {
    Timer timer;
    size_t sum = 0;
    boost::iostreams::stream<boost::iostreams::array_source> istrm(buffer.data(), buffer.size());
    int ch;
    while((ch = istrm.get()) != EOF)
        sum += ch;
    std::cout << "boost::iostreams::stream<boost::iostreams::array_source> with get elapsed: " << timer.elapsedWallSeconds() << '\n';
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
        BasicImStream istrm(szData, DFG_COUNTOF_SZ(szData));

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

    // Testing constructor that takes a lvalue container
    {
        std::string a = "abc";
        const std::vector<char> b = { 'd', 'e' };
        std::vector<char16_t> c = { 'f' };
        {
            // These should fail to compile: constructing from rvalue is not allowed to make it less easy to accidentally cause input go dangling as it would in these cases
            //BasicImStream istrm0(std::string("abc"));
            //BasicImStream istrm1(std::vector<char>({ 'd', 'e', 'f' }));
            //BasicImStream_T<char16_t> istrm2(std::vector<char16_t>({ 'g', 'h', 'i' }));
        }
        {
            BasicImStream istrm0(a);
            BasicImStream istrm1(b);
            BasicImStream_T<char16_t> istrm2(c);
            DFGTEST_EXPECT_LEFT(3, istrm0.countOfRemainingElems());
            DFGTEST_EXPECT_LEFT(2, istrm1.countOfRemainingElems());
            DFGTEST_EXPECT_LEFT(1, istrm2.countOfRemainingElems());
            DFGTEST_EXPECT_LEFT(a.data(), istrm0.beginPtr());
            DFGTEST_EXPECT_LEFT(b.data(), istrm1.beginPtr());
            DFGTEST_EXPECT_LEFT(c.data(), istrm2.beginPtr());
        }
    }

    // get-testing
    {
        getTest<BasicImStream>();
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
    template <class Int_T>
    void ImStreamWithEncoding_unalignedSourceImpl()
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);

        //char buffer[3 * sizeof(Int_T)] = ""; // Test case ImStreamWithEncoding_unalignedSource would crash on Clang 18.1.3 & libc++ ver 180100
        //                                        and on closer look commenting everything else out in this function but this line still caused a crash when both
        //                                        uint16 and uint32 specializations were called in actual test function. Commenting out uint32-call
        //                                        fixed the crash as did workaround of not initializing buffer in declaration line.
        char buffer[3 * sizeof(Int_T)];
        memsetZero(buffer);
        char* p = &buffer[0];
        while (reinterpret_cast<uintptr_t>(p) % sizeof(Int_T) != 1)
            p++;
        const auto pStart = p;
        Int_T c0 = 'a';
        Int_T c1 = 'b';
        memcpy(p, &c0, sizeof(c0));
        p += sizeof(c0);
        memcpy(p, &c1, sizeof(c1));
        p += sizeof(c1);

        {
            DFG_CLASS_NAME(ImStreamWithEncoding) istrm(pStart, p - pStart, hostNativeUtfEncodingFromCharType(sizeof(Int_T)));
            EXPECT_EQ('a', istrm.get());
            EXPECT_EQ('b', istrm.get());
        }

        // Check that reading uneven item count behaves.
        {
            // No full Int_T's
            DFG_CLASS_NAME(ImStreamWithEncoding) istrm(pStart, sizeof(Int_T) / 2, hostNativeUtfEncodingFromCharType(sizeof(Int_T)));
            EXPECT_EQ(std::char_traits<char>::eof(), istrm.get());
            EXPECT_TRUE(istrm.eof());

            // One full codepoint and one non-full Int_T
            DFG_CLASS_NAME(ImStreamWithEncoding) istrm2(pStart, sizeof(Int_T) + sizeof(Int_T) / 2, hostNativeUtfEncodingFromCharType(sizeof(Int_T)));
            EXPECT_EQ('a', istrm2.get());
            EXPECT_EQ(std::char_traits<char>::eof(), istrm2.get());
            EXPECT_TRUE(istrm2.eof());
        }
    }
}

TEST(dfgIo, ImStreamWithEncoding_unalignedSource)
{
    using namespace DFG_ROOT_NS;
    ImStreamWithEncoding_unalignedSourceImpl<uint16>();
    ImStreamWithEncoding_unalignedSourceImpl<uint32>();

    // Test code point that is cut in the middle
    {
        using namespace DFG_MODULE_NS(io);

        std::basic_string<char16_t> s;
        DFG_MODULE_NS(utf)::cpToUtf(70000, std::back_inserter(s), sizeof(char16_t), ByteOrderHost);
        DFG_MODULE_NS(utf)::cpToUtf(80000, std::back_inserter(s), sizeof(char16_t), ByteOrderHost);
        ASSERT_TRUE(s.size() == 4);
        char buffer[7];
        memcpy(buffer, s.data(), 7);
        const char* pStart = reinterpret_cast<const char*>(buffer);
        const char* const pEnd = pStart + 7;
        DFG_CLASS_NAME(ImStreamWithEncoding) istrm(pStart, pEnd - pStart, hostNativeUtfEncodingFromCharType(sizeof(char16_t)));
        EXPECT_EQ(70000, istrm.get()); // First item should read ok
        EXPECT_EQ(DFG_MODULE_NS(utf)::INVALID_CODE_POINT, istrm.get()); // Code point consists of 4 bytes, but only three is in input -> expecting INVALID_CODE_POINT.
        EXPECT_EQ(std::char_traits<char>::eof(), istrm.get());
        EXPECT_TRUE(istrm.eof());
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

#if 0
TEST(dfgIo, OByteStream)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);
}
#endif

TEST(dfgIo, ImcByteStream)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char buffer[] = "ImcByteStream\n\r\n";
    char readBuffer[DFG_COUNTOF_SZ(buffer)];

    ImcByteStream istrm(buffer, DFG_COUNTOF_SZ(buffer));
    readBinary(istrm, readBuffer);
    EXPECT_EQ(DFG_COUNTOF_SZ(buffer), istrm.gcount());
    EXPECT_EQ(0, memcmp(buffer, readBuffer, count(readBuffer)));
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

TEST(dfgIo, IfStreamWithEncoding)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);
 
    const char szNonExistentPath[] = "testfiles/nonExistentFile.invalid";

    // Test handling of non-existent path.
    {
        IfStreamWithEncoding strm(szNonExistentPath);
        std::ifstream strmStd(szNonExistentPath);
        EXPECT_EQ(strmStd.rdstate(), strm.rdstate());
        EXPECT_EQ(strmStd.good(), strm.good());

        IfStreamWithEncoding strm2;
        strm2.open(szNonExistentPath);
        EXPECT_EQ(strmStd.rdstate(), strm2.rdstate());
        EXPECT_EQ(strmStd.good(), strm2.good());
    }

    // Testing seeking
    {
        const auto testSeek = [](const FileReadOpenModeRequest openMode)
        {
            IfStreamWithEncoding istrm("testfiles/matrix_3x3.txt", encodingUnknown, FileReadOpenRequestArgs(openMode, 4));
            char bytes[3];
            EXPECT_TRUE(istrm.good());
            readBytes(istrm, bytes, 3);
            EXPECT_TRUE(istrm.good());
            istrm.seekg(-3, std::ios_base::cur);
            EXPECT_EQ(bytes[0], istrm.get());
            EXPECT_TRUE(istrm.good());
            istrm.seekg(1, std::ios_base::cur);
            EXPECT_EQ(bytes[2], istrm.get());
            istrm.seekg(0);
            EXPECT_EQ(bytes[0], istrm.get());
            EXPECT_TRUE(istrm.good());
        };
        testSeek(FileReadOpenModeRequest::memoryMap);
        testSeek(FileReadOpenModeRequest::file);
    }

    // Testing read results from memory and file reads with UTF-encoded file.
    {
        const char szFilePath[] = "testfiles/utf8_with_BOM.txt";

        const auto computeGetSum = [&](const FileReadOpenModeRequest openMode)
            {
                IfStreamWithEncoding strm;
                strm.open(szFilePath, encodingUnknown, openMode);
                int c = 0;
                uint64 nSum = 0;
                while ((c = strm.get()) != strm.eofValue())
                    nSum += static_cast<uint64>(c);
                return nSum;
            };

        EXPECT_EQ(computeGetSum(FileReadOpenModeRequest::memoryMap), computeGetSum(FileReadOpenModeRequest::file));
    }

    // Testing that reading ascii-file results to identical results with IfStreamWithEncoding() and BasicIfStream
    {
        const char szFilePath[] = "testfiles/matrix_3x3.txt";
        std::string bytesBasic, bytesEncMem, bytesEncFile;

        bytesBasic = fileToByteContainer<decltype(bytesBasic)>(szFilePath);
        {
            IfStreamWithEncoding istrm(szFilePath, encodingUnknown, FileReadOpenModeRequest::memoryMap);
            bytesEncMem = readAllFromStream<decltype(bytesEncMem)>(istrm);
        }
        {
            IfStreamWithEncoding istrm(szFilePath, encodingUnknown, FileReadOpenRequestArgs(FileReadOpenModeRequest::file, 4));
            bytesEncFile = readAllFromStream<decltype(bytesEncMem)>(istrm);
        }
        EXPECT_EQ(bytesBasic, bytesEncMem);
        EXPECT_EQ(bytesBasic, bytesEncFile);
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

    IfStreamWithEncoding strm(szPath);
    std::ifstream strmStd(szPath);
    EXPECT_EQ(strmStd.is_open(), strm.is_open());
    EXPECT_EQ(strmStd.good(), strm.good());
    std::string s;
    strm >> s;
    EXPECT_EQ(szText, s);
}

TEST(dfgIo, IfStreamWithEncoding_rawByteReading)
{
    // Test to verify that it is possible to easily read plain bytes from UTF8-data with IfStreamWithEncoding.

    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char szFilePath[] = "testfiles/utf8_with_BOM.txt";

    auto bytes0 = fileToVector(szFilePath);
    ASSERT_TRUE(bytes0.size() > 3);
    bytes0.erase(bytes0.begin(), bytes0.begin() + 3); // Erase BOM.

    IfStreamWithEncoding istrm;
    istrm.open("testfiles/utf8_with_BOM.txt");

    EXPECT_EQ(encodingUTF8, istrm.encoding());

    istrm.setEncoding(encodingUnknown);
    const auto bytes1 = readAllFromStream<std::vector<char>>(istrm);

    EXPECT_EQ(bytes0, bytes1);
}

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
    // Testing that write() handles all bytes correctly.
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char szFilename[] = "testfiles/generated/OfStreamWithEncoding_writeTest.txt";

    OfStreamWithEncoding ostrm(szFilename, encodingLatin1);
    for (int i = 0; i < 256; ++i)
    {
        auto uc = static_cast<uint8>(i);
        ostrm.write(reinterpret_cast<const char*>(&uc), 1);
    }
    EXPECT_TRUE(ostrm.good());
    ostrm.close();

    const auto bytes = fileToMemory_readOnly(szFilename);
    EXPECT_EQ(256, bytes.size());
    if (bytes.size() == 256)
    {
        const auto uspan = bytes.asSpan<uint8>();
        for (int i = 0; i < 256; ++i)
        {
            EXPECT_EQ(i, uspan[i]);
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
            const char* pInput = (std::is_same_v<Strm_T, std::istrstream>) ? "" : nullptr; // istrstream interprets 0 length as "call strlen() to get length" (https://en.cppreference.com/w/cpp/io/strstreambuf/strstreambuf)
            Strm_T istrm(pInput, std::streamsize(0));
            getThrough(istrm, [&](char ch) { output.push_back(ch); }); 
            EXPECT_EQ(0, output.size());
        }
    }
}

TEST(dfgIo, getThrough)
{
    using namespace DFG_MODULE_NS(io);
    DFGTEST_STATIC(DFG_DETAIL_NS::io_hpp::Has_getThrough<BasicImStream>::value == true);
    DFGTEST_STATIC(DFG_DETAIL_NS::io_hpp::Has_getThrough<std::ifstream>::value == false);
    DFGTEST_STATIC(DFG_DETAIL_NS::io_hpp::Has_getThrough<std::istrstream>::value == false);

    getThroughImpl<BasicImStream>();
    getThroughImpl<std::istrstream>();
}

TEST(dfgIo, endOfLineTypeFromStr)
{
    using namespace DFG_MODULE_NS(io);
    EXPECT_EQ(EndOfLineTypeN, endOfLineTypeFromStr("\\n"));
    EXPECT_EQ(EndOfLineTypeN, endOfLineTypeFromStr("\n"));
    EXPECT_EQ(EndOfLineTypeN, endOfLineTypeFromStr("\"\n\""));

    EXPECT_EQ(EndOfLineTypeRN, endOfLineTypeFromStr("\\r\\n"));
    EXPECT_EQ(EndOfLineTypeRN, endOfLineTypeFromStr("\r\n"));
    EXPECT_EQ(EndOfLineTypeRN, endOfLineTypeFromStr("\"\\r\\n\""));

    EXPECT_EQ(EndOfLineTypeR, endOfLineTypeFromStr("\\r"));
    EXPECT_EQ(EndOfLineTypeR, endOfLineTypeFromStr("\r"));
    EXPECT_EQ(EndOfLineTypeR, endOfLineTypeFromStr("\"\\r\""));

    // Invalid cases below, expected to return \n for these.
    EXPECT_EQ(EndOfLineTypeN, endOfLineTypeFromStr(""));
    EXPECT_EQ(EndOfLineTypeN, endOfLineTypeFromStr("\\r\\n\\r"));
    EXPECT_EQ(EndOfLineTypeN, endOfLineTypeFromStr("\\t"));
}

TEST(dfgIo, BasicOmcByteStream)
{
    using namespace DFG_ROOT_NS;
    {
        DFG_MODULE_NS(io)::BasicOmcByteStream<> ostrm;
        EXPECT_TRUE(ostrm.tryReserve(1000));
    }
    {
        DFG_MODULE_NS(io)::BasicOmcByteStream<> ostrm;
        EXPECT_FALSE(ostrm.tryReserve(NumericTraits<size_t>::maxValue));
    }
}

TEST(dfgIo, fileToByteContainer)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const char szPath[] = "testfiles/matrix_3x3.txt";
    const auto bytes = fileToVector(szPath);
    DFG_MODULE_NS(os)::MemoryMappedFile mmf;
    mmf.open(szPath);
    ASSERT_EQ(mmf.size(), bytes.size());
    EXPECT_TRUE(std::equal(mmf.begin(), mmf.end(), bytes.cbegin()));

    // Testing that can read also as unsigned char, int8, uint8 and std::byte if available
    {
        const auto vecUchar = fileToByteContainer<std::vector<unsigned char>>(szPath);
        const auto vecInt8 = fileToByteContainer<std::vector<int8>>(szPath);
        const auto vecUint8 = fileToByteContainer<std::vector<uint8>>(szPath);
#if defined(__cpp_lib_byte) && (__cpp_lib_byte >= 201603L)
        const auto vecBytes = fileToByteContainer<std::vector<std::byte>>(szPath);
        EXPECT_EQ(bytes.size(), vecBytes.size());
#endif
        EXPECT_EQ(bytes.size(), vecUchar.size());
        EXPECT_EQ(bytes.size(), vecInt8.size());
        EXPECT_EQ(bytes.size(), vecUint8.size());
    }
}

TEST(dfgIo, fileToMemory_readOnly)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(io);
    const char szFilePath[] = "testfiles/matrix_3x3.txt";

    {
        const auto bytes = fileToMemory_readOnly(szFilePath);
        auto bytesNoMmf = fileToMemory_readOnly(szFilePath, FileToMemoryFlags().removeFlag(FileToMemoryFlags::allowFileLocking));
        //const auto danglingSpan = fileToMemory_readOnly(szFilePath).asSpan<char>(); // This should fail to compile
        EXPECT_FALSE(bytes.empty());
        EXPECT_FALSE(bytesNoMmf.empty());
        EXPECT_TRUE(bytes.isMemoryMapped());
        EXPECT_FALSE(bytesNoMmf.isMemoryMapped());
        EXPECT_EQ(62, bytes.size());
        const auto vecChar = fileToByteContainer<std::vector<char>>(szFilePath);
        const auto vecUChar = fileToByteContainer<std::vector<unsigned char>>(szFilePath);
        const auto vecInt8 = fileToByteContainer<std::vector<int8>>(szFilePath);
        const auto vecUint8 = fileToByteContainer<std::vector<uint8>>(szFilePath);
        const auto s = fileToByteContainer<std::string>(szFilePath);
#if defined(__cpp_lib_byte) && (__cpp_lib_byte >= 201603L)
        const auto vecBytes = fileToByteContainer<std::vector<std::byte>>(szFilePath);
        EXPECT_TRUE(isEqualContent(vecBytes, bytes.asSpan<std::byte>()));
        DFGTEST_EXPECT_TRUE(isEqualContent(vecBytes, bytes.asContainer<std::vector<std::byte>>()));
#endif
        EXPECT_TRUE(isEqualContent(vecChar, bytes.asSpan<char>()));
        EXPECT_TRUE(isEqualContent(vecChar, bytesNoMmf.asSpan<char>()));
        EXPECT_TRUE(isEqualContent(vecUChar, bytes.asSpan<unsigned char>()));
        EXPECT_TRUE(isEqualContent(vecInt8, bytes.asSpan<int8>()));
        EXPECT_TRUE(isEqualContent(vecUint8, bytes.asSpan<uint8>()));
        EXPECT_TRUE(isEqualContent(s, bytes.asSpan<char>()));

        // asContainer() tests
        DFGTEST_EXPECT_TRUE(isEqualContent(vecChar, bytes.asContainer<std::string>()));
        DFGTEST_EXPECT_TRUE(isEqualContent(vecChar, bytes.asContainer<std::vector<char>>()));
        DFGTEST_EXPECT_TRUE(isEqualContent(vecUint8, bytes.asContainer<std::vector<uint8>>()));
        DFGTEST_EXPECT_TRUE(isEqualContent(vecChar, fileToMemory_readOnly(szFilePath).asContainer<std::string>())); // Testing that, unlike asSpan(), asContainer() works for rvalue
        //DFGTEST_EXPECT_TRUE(isEqualContent(std::vector<uint16>(), bytes.asContainer<std::vector<uint16>>())); // This should fail to compile since asContainer() requires sizeof(element type) == 1

        bytesNoMmf.releaseStorage(); // Just calling this shouldn't yet clear contents
        EXPECT_EQ(62, bytesNoMmf.size());
        const auto bytesNoMmfSpan = bytesNoMmf.asSpan<char>();
        const auto releasedBytes = bytesNoMmf.releaseStorage();
        EXPECT_TRUE(bytesNoMmf.empty());
        EXPECT_EQ(bytesNoMmfSpan.begin(), releasedBytes.data());
        EXPECT_EQ(bytesNoMmfSpan.size(), releasedBytes.size());
    }
}

TEST(dfgIo, ostreamPerformance)
{
#if DFGTEST_ENABLE_BENCHMARKS == 0
    DFGTEST_MESSAGE("ostreamPerformance skipped due to build settings");
#else
    using namespace DFG_ROOT_NS;
#ifdef DFG_BUILD_TYPE_DEBUG
    const auto nRowCount = 10000;
#else
    const auto nRowCount = 1000000;
#endif
    const auto nColCount = 5;
    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TableCsv)<char, uint32> table;
    for (int c = 0; c < nColCount; ++c)
        for (int r = 0; r < nRowCount; ++r)
            table.setElement(r, c, DFG_UTF8("a"));

    typedef DFG_MODULE_NS(time)::TimerCpu TimerT;
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Vector)<char> VectorT;
    VectorT bytesStd;
    VectorT bytesOmc;
    VectorT bytesBasicOmc;

    // std::ostrstream
    {
        TimerT timer;
        std::ostrstream ostrStream;
        table.writeToStream(ostrStream);
        DFGTEST_MESSAGE("std::ostrstream lasted " << timer.elapsedWallSeconds());
        const auto psz = ostrStream.str();
        bytesStd.assign(psz, psz + static_cast<std::streamoff>(ostrStream.tellp()));
        ostrStream.freeze(false);
    }

    // OmcByteStream
    {
        TimerT timer;
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(OmcByteStream)<> omcStrm;
        table.writeToStream(omcStrm);
        DFGTEST_MESSAGE("OmcByteStream lasted " << timer.elapsedWallSeconds());
        bytesOmc = omcStrm.releaseData();
    }

    // BasicOmcByteStream
    {
        TimerT timer;
        DFG_MODULE_NS(io)::BasicOmcByteStream<> ostrm;
        table.writeToStream(ostrm);
        DFGTEST_MESSAGE("BasicOmcByteStream lasted " << timer.elapsedWallSeconds());
        bytesBasicOmc = ostrm.releaseData();
    }

    EXPECT_EQ(bytesStd, bytesOmc);
    EXPECT_EQ(bytesStd, bytesBasicOmc);
#endif // DFGTEST_ENABLE_BENCHMARKS
}

#ifdef _WIN32
TEST(dfgIo, widePathStrToFstreamFriendlyNonWide)
{
    const wchar_t szNonConvertable[] = L"\x20AD";
    EXPECT_TRUE(DFG_MODULE_NS(io)::widePathStrToFstreamFriendlyNonWide(szNonConvertable).empty());
}
#endif

TEST(dfgIo, isUtfEncoding)
{
    using namespace DFG_MODULE_NS(io);
    DFGTEST_STATIC_TEST(isUtfEncoding(encodingUTF8));
    DFGTEST_STATIC_TEST(isUtfEncoding(encodingUTF16Be));
    DFGTEST_STATIC_TEST(isUtfEncoding(encodingUTF16Le));
    DFGTEST_STATIC_TEST(isUtfEncoding(encodingUTF32Be));
    DFGTEST_STATIC_TEST(isUtfEncoding(encodingUTF32Le));

    DFGTEST_STATIC_TEST(!isUtfEncoding(encodingLatin1));
    DFGTEST_STATIC_TEST(!isUtfEncoding(encodingUCS2Be));
    DFGTEST_STATIC_TEST(!isUtfEncoding(encodingUCS2Le));
    DFGTEST_STATIC_TEST(!isUtfEncoding(encodingUCS4Be));
    DFGTEST_STATIC_TEST(!isUtfEncoding(encodingUCS4Le));
}

TEST(dfgIo, isBigEndianEncoding)
{
    using namespace DFG_MODULE_NS(io);
    DFGTEST_STATIC_TEST_MESSAGE(NumberOfTextCodingItems == 12, "Number of text encoding items has changed, check if test case is up-to-date.");
    DFGTEST_STATIC_TEST(!isBigEndianEncoding(encodingUTF8));
    DFGTEST_STATIC_TEST(isBigEndianEncoding(encodingUTF16Be));
    DFGTEST_STATIC_TEST(!isBigEndianEncoding(encodingUTF16Le));
    DFGTEST_STATIC_TEST(isBigEndianEncoding(encodingUTF32Be));
    DFGTEST_STATIC_TEST(!isBigEndianEncoding(encodingUTF32Le));
    DFGTEST_STATIC_TEST(isBigEndianEncoding(encodingUCS2Be));
    DFGTEST_STATIC_TEST(!isBigEndianEncoding(encodingUCS2Le));
    DFGTEST_STATIC_TEST(isBigEndianEncoding(encodingUCS4Be));
    DFGTEST_STATIC_TEST(!isBigEndianEncoding(encodingUCS4Le));
}

TEST(dfgIo, areAsciiBytesValidContentInEncoding)
{
    using namespace DFG_MODULE_NS(io);
    DFGTEST_STATIC_TEST_MESSAGE(NumberOfTextCodingItems == 12, "Number of text encoding items has changed, check if test case is up-to-date.");
    DFGTEST_EXPECT_TRUE(areAsciiBytesValidContentInEncoding(encodingLatin1));
    DFGTEST_EXPECT_TRUE(areAsciiBytesValidContentInEncoding(encodingWindows1252));
    DFGTEST_EXPECT_TRUE(areAsciiBytesValidContentInEncoding(encodingUTF8));
}

TEST(dfgIo, encodingStrIds)
{
    using namespace DFG_MODULE_NS(io);
    DFGTEST_STATIC_TEST_MESSAGE(NumberOfTextCodingItems == 12, "Number of text encoding items has changed, check if test case is up-to-date.");
    DFGTEST_EXPECT_STREQ("",             encodingToStrId(encodingUnknown));
    DFGTEST_EXPECT_STREQ("UTF8",         encodingToStrId(encodingUTF8));
    DFGTEST_EXPECT_STREQ("UTF16LE",      encodingToStrId(encodingUTF16Le));
    DFGTEST_EXPECT_STREQ("UTF16BE",      encodingToStrId(encodingUTF16Be));
    DFGTEST_EXPECT_STREQ("UTF32LE",      encodingToStrId(encodingUTF32Le));
    DFGTEST_EXPECT_STREQ("UTF32BE",      encodingToStrId(encodingUTF32Be));
    DFGTEST_EXPECT_STREQ("Latin1",       encodingToStrId(encodingLatin1));
    DFGTEST_EXPECT_STREQ("UCS2LE",       encodingToStrId(encodingUCS2Le));
    DFGTEST_EXPECT_STREQ("UCS2BE",       encodingToStrId(encodingUCS2Be));
    DFGTEST_EXPECT_STREQ("UCS4LE",       encodingToStrId(encodingUCS4Le));
    DFGTEST_EXPECT_STREQ("UCS4BE",       encodingToStrId(encodingUCS4Be));
    DFGTEST_EXPECT_STREQ("windows-1252", encodingToStrId(encodingWindows1252));

    DFGTEST_EXPECT_LEFT(encodingUnknown,     strIdToEncoding(""));
    DFGTEST_EXPECT_LEFT(encodingUnknown,     strIdToEncoding("abc"));
    DFGTEST_EXPECT_LEFT(encodingUTF8,        strIdToEncoding("UTF8"));
    DFGTEST_EXPECT_LEFT(encodingUTF16Le,     strIdToEncoding("UTF16LE"));
    DFGTEST_EXPECT_LEFT(encodingUTF16Be,     strIdToEncoding("UTF16BE"));
    DFGTEST_EXPECT_LEFT(encodingUTF32Le,     strIdToEncoding("UTF32LE"));
    DFGTEST_EXPECT_LEFT(encodingUTF32Be,     strIdToEncoding("UTF32BE"));
    DFGTEST_EXPECT_LEFT(encodingLatin1,      strIdToEncoding("Latin1"));
    DFGTEST_EXPECT_LEFT(encodingUCS2Le,      strIdToEncoding("UCS2LE"));
    DFGTEST_EXPECT_LEFT(encodingUCS2Be,      strIdToEncoding("UCS2BE"));
    DFGTEST_EXPECT_LEFT(encodingUCS4Le,      strIdToEncoding("UCS4LE"));
    DFGTEST_EXPECT_LEFT(encodingUCS4Be,      strIdToEncoding("UCS4BE"));
    DFGTEST_EXPECT_LEFT(encodingWindows1252, strIdToEncoding("windows-1252"));
}

TEST(dfgIo, baseCharacterSize)
{
    using namespace DFG_MODULE_NS(io);
    DFGTEST_STATIC_TEST_MESSAGE(NumberOfTextCodingItems == 12, "Number of text encoding items has changed, check if test case is up-to-date.");
    DFGTEST_EXPECT_LEFT(1, baseCharacterSize(encodingUnknown));
    DFGTEST_EXPECT_LEFT(1, baseCharacterSize(encodingUTF8));
    DFGTEST_EXPECT_LEFT(2, baseCharacterSize(encodingUTF16Le));
    DFGTEST_EXPECT_LEFT(2, baseCharacterSize(encodingUTF16Be));
    DFGTEST_EXPECT_LEFT(4, baseCharacterSize(encodingUTF32Le));
    DFGTEST_EXPECT_LEFT(4, baseCharacterSize(encodingUTF32Be));
    DFGTEST_EXPECT_LEFT(1, baseCharacterSize(encodingLatin1));
    DFGTEST_EXPECT_LEFT(2, baseCharacterSize(encodingUCS2Le));
    DFGTEST_EXPECT_LEFT(2, baseCharacterSize(encodingUCS2Be));
    DFGTEST_EXPECT_LEFT(4, baseCharacterSize(encodingUCS4Le));
    DFGTEST_EXPECT_LEFT(4, baseCharacterSize(encodingUCS4Be));
    DFGTEST_EXPECT_LEFT(1, baseCharacterSize(encodingWindows1252));
}

#endif
