#include <stdafx.h>
#include <dfg/os.hpp>
#include <dfg/str.hpp>
#include <dfg/io.hpp>
#include <dfg/os/memoryMappedFile.hpp>
#include <dfg/os/TemporaryFileStream.hpp>
#include <dfg/os/fileSize.hpp>
#include <dfg/alg.hpp>
#include <dfg/dfgBase.hpp>

#ifdef _WIN32
TEST(dfgOs, pathFindExtension)
{
    using namespace DFG_MODULE_NS(os);
    using namespace DFG_MODULE_NS(str);
    EXPECT_EQ(strCmp(".txt", pathFindExtension("c:/testFolder/test.txt")), 0);
    EXPECT_EQ(strCmp(".txt", pathFindExtension("c:\\testFolder\\test.txt")), 0);
    EXPECT_EQ(strCmp(".gz", pathFindExtension("test.tar.gz")), 0);
    EXPECT_EQ(strCmp(".gz", pathFindExtension("c:\\testFolder\\test.tar.gz")), 0);
    EXPECT_EQ(strCmp(L".", pathFindExtension(L"c:/testFolder/test.")), 0);
    EXPECT_EQ(strCmp(L".", pathFindExtension(L"c:\\testFolder\\test.")), 0);
    EXPECT_EQ(strCmp(L"", pathFindExtension(L"c:/testFolder/test")), 0);
    EXPECT_EQ(strCmp(L".extension", pathFindExtension(L"z:/testFolder/test/dfg/dfgvcbcv/bcvbb/.extension")), 0);
    EXPECT_EQ(strCmp(L".extension", pathFindExtension(L"z:\\testFolder\\test\\dfg\\dfgvcbcv\\bcvbb\\.extension")), 0);
    EXPECT_NE(strCmp(L".extension", pathFindExtension(L"z:\\testFolder\\test\\dfg\\dfgvcbcv\\bcvbb\\.extensionA")), 0);
}
#endif

TEST(dfgOs, MemoryMappedFile)
{
    using namespace DFG_MODULE_NS(os);
    using namespace DFG_MODULE_NS(io);
    DFG_CLASS_NAME(MemoryMappedFile) mmf;
    mmf.open("testfiles/matrix_3x3.txt");
    EXPECT_TRUE(mmf.is_open());
    EXPECT_TRUE(mmf.size() == 62);
    std::vector<char> bytes(mmf.begin(), mmf.end());
    const auto vec2 = fileToVector("testfiles/matrix_3x3.txt");
    EXPECT_EQ(bytes, vec2);
}

#ifdef _WIN32
TEST(dfgOs, TemporaryFile)
{
    using namespace DFG_MODULE_NS(os);

    std::string sTfPath;
    std::string sTfWPath;

    {
        DFG_CLASS_NAME(TemporaryFileStream) tf(nullptr, nullptr, nullptr, "dfgTestTemp", 16);
        DFG_CLASS_NAME(TemporaryFileStream) tfW(nullptr, nullptr, nullptr, L"dfgTestTemp", 16);
        EXPECT_TRUE(tf.isOpen());
        EXPECT_TRUE(tfW.isOpen());

        sTfPath = tf.pathLatin1();
        sTfWPath = tfW.pathLatin1();
        EXPECT_EQ(sTfPath, tf.pathU8()); // Hack: assume that path is ANSI, probably true in most cases, but not guaranteed.
        EXPECT_EQ(sTfWPath, tfW.pathU8()); // Hack: assume that path is ANSI, probably true in most cases, but not guaranteed.

        EXPECT_TRUE(isPathFileAvailable(sTfPath.c_str(), FileModeExists));
        EXPECT_TRUE(isPathFileAvailable(sTfWPath.c_str(), FileModeExists));

        const char szTestText[] = "test_text_to_temp_file";

        {
            tf << szTestText;
            tf.flush();
            std::ifstream istrm(sTfPath);
            EXPECT_TRUE(istrm.is_open());
            std::string sRead;
            istrm >> sRead;
            istrm.close();
            EXPECT_EQ(szTestText, sRead);
        }

        {
            tfW << szTestText;
            tfW.flush();
            std::ifstream istrmW(sTfWPath);
            EXPECT_TRUE(istrmW.is_open());
            std::string sRead;
            istrmW >> sRead;
            istrmW.close();
            EXPECT_EQ(szTestText, sRead);
        }
    }

    EXPECT_FALSE(isPathFileAvailable(sTfPath.c_str(), FileModeExists));
    EXPECT_FALSE(isPathFileAvailable(sTfWPath.c_str(), FileModeExists));

#ifdef _MSC_VER
    // Test temp file path which has character outside latin1. 
    {
        const wchar_t cEuroSign[2] = { 0x20AC , 0 };
        DFG_CLASS_NAME(TemporaryFileStream) tf(nullptr, cEuroSign, nullptr, L"dfgTestTemp");
        const auto sPath = tf.path();
        const auto sPathU8 = tf.pathU8();
        const auto sPathLatin1 = tf.pathLatin1();
        const auto i = DFG_MODULE_NS(alg)::indexOf(sPath, 0x20AC);
        EXPECT_TRUE(DFG_ROOT_NS::isValidIndex(sPath, i));
        { // Hack: assume that the path is otherwise ANSI.
            EXPECT_EQ(sPath.size() + 2, sPathU8.size());
            EXPECT_EQ(sPath.size(), sPathLatin1.size());
            auto utf8EuroSignRange = DFG_ROOT_NS::makeRange(sPathU8.begin() + i, sPathU8.begin() + i + 3);

            const auto sEuro = DFG_MODULE_NS(utf)::utf8ToFixedChSizeStr<wchar_t>(utf8EuroSignRange);
            EXPECT_EQ(1, sEuro.size());
            EXPECT_EQ(0x20AC, sEuro.front());
        }
        EXPECT_TRUE(isPathFileAvailable(sPath.c_str(), FileModeExists));
    }
#endif // _MSC_VER
}
#endif // _WIN32

TEST(dfgOs, fileSize)
{
    using namespace DFG_MODULE_NS(os);
    const char szPath[] = "testfiles/matrix_10x10_1to100_eol_n.txt";
    const wchar_t wszPath[] = L"testfiles/matrix_10x10_1to100_eol_n.txt";
    const std::string sPath(szPath);
    const std::wstring swPath(wszPath);
    const auto nKnownFileSize = 310;
    EXPECT_EQ(nKnownFileSize, fileSize(szPath));
    EXPECT_EQ(nKnownFileSize, fileSize(wszPath));
    EXPECT_EQ(nKnownFileSize, fileSize(sPath));
    EXPECT_EQ(nKnownFileSize, fileSize(swPath));
    EXPECT_EQ(0, fileSize("testFiles/aNonExistentFile.invalidExtension"));
}
