#include "stdafx.h"

#if (defined(DFGTEST_BUILD_MODULE_OS) && DFGTEST_BUILD_MODULE_OS == 1) || (!defined(DFGTEST_BUILD_MODULE_OS) && DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/os.hpp>
#include <dfg/str.hpp>
#include <dfg/io.hpp>
#include <dfg/osAll.hpp>
#include <dfg/alg.hpp>
#include <dfg/dfgBase.hpp>
#include <dfg/io/OfStream.hpp>

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
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(os);
    using namespace DFG_MODULE_NS(io);

    const char szPath[] = "testfiles/matrix_3x3.txt";
    {
        MemoryMappedFile mmf;
        mmf.open(szPath);
        EXPECT_TRUE(mmf.is_open());
        EXPECT_EQ(62, mmf.size());
        std::vector<char> bytes(mmf.begin(), mmf.end());
        const auto vec2 = fileToVector("testfiles/matrix_3x3.txt");
        EXPECT_EQ(bytes, vec2);
    }

    // Testing that can use string view as path
    {
        MemoryMappedFile mmf;
        mmf.open(StringViewC(szPath));
        EXPECT_EQ(62, mmf.size());
    }
}

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
        tf.close();
        EXPECT_FALSE(isPathFileAvailable(sPath.c_str(), FileModeExists));
    }
}

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

TEST(dfgOs, renameFileOrDirectory_cstdio)
{
    using namespace DFG_MODULE_NS(os);
    using namespace DFG_MODULE_NS(io);

    const char szPath0[] =      "testfiles/generated/renameFileOrDirectory_cstdioTest0.file";
    const wchar_t wszPath0[] = L"testfiles/generated/renameFileOrDirectory_cstdioTest0.file";
    const char szPath1[] =      "testfiles/generated/renameFileOrDirectory_cstdioTest1.file";
    const wchar_t wszPath1[] = L"testfiles/generated/renameFileOrDirectory_cstdioTest1.file";

    removeFile(szPath0);
    removeFile(szPath1);

    EXPECT_FALSE(isPathFileAvailable(szPath0, FileModeExists));
    EXPECT_FALSE(isPathFileAvailable(szPath1, FileModeExists));

    // Create file
    DFG_CLASS_NAME(OfStream) strm(szPath0);
    strm.close();

    EXPECT_TRUE(isPathFileAvailable(szPath0, FileModeExists));
    EXPECT_FALSE(isPathFileAvailable(szPath1, FileModeExists));

    EXPECT_EQ(0, renameFileOrDirectory_cstdio(szPath0, szPath1));

    EXPECT_FALSE(isPathFileAvailable(szPath0, FileModeExists));
    EXPECT_TRUE(isPathFileAvailable(szPath1, FileModeExists));

#ifdef _WIN32
    renameFileOrDirectory_cstdio(wszPath1, wszPath0);
#else
    renameFileOrDirectory_cstdio(szPath1, szPath0);
#endif

    EXPECT_TRUE(isPathFileAvailable(szPath0, FileModeExists));
    EXPECT_FALSE(isPathFileAvailable(szPath1, FileModeExists));

    removeFile(szPath0);

    EXPECT_FALSE(isPathFileAvailable(szPath0, FileModeExists));

    // TODO: test directory renaming
}

namespace
{
    template <class OutputFile_T>
    static void testOutputFileMemStream(OutputFile_T& outputFile, const DFG_ROOT_NS::DFG_CLASS_NAME(StringViewSzC)& svPath)
    {
        using namespace DFG_MODULE_NS(os);
        auto& strm = outputFile.intermediateMemoryStream();
        EXPECT_TRUE(strm.good());
        EXPECT_TRUE(outputFile.m_pathIntermediate.empty());
        strm.write("abc", 3);
        // Verify that destination file hasn't been changed.
        EXPECT_EQ(1, fileSize(svPath.c_str()));
        EXPECT_EQ(0, outputFile.writeIntermediateToFinalLocation());
        EXPECT_EQ(3, fileSize(svPath.c_str()));
        EXPECT_EQ("abc", DFG_MODULE_NS(io)::fileToByteContainer<std::string>(svPath.c_str()));
    }
}

TEST(dfgOs, OutputFile_completeOrNone)
{
    using namespace DFG_MODULE_NS(os);
    typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStream) OfStream;

    // Basic file intermediate test
    {
        const char szFilePath[] = "testfiles/generated/OutputFileTest_0.txt";
        OfStream::dumpBytesToFile_overwriting(szFilePath, "a", 1);
        OutputFile_completeOrNone<> outputFile(szFilePath);
        auto& strm = outputFile.intermediateFileStream();
        EXPECT_TRUE(strm.good());
        // Check that intermediate file got created.
        EXPECT_TRUE(isPathFileAvailable(outputFile.m_pathIntermediate, FileModeExists));
        strm.write("abc", 3);
        // Verify that destination file hasn't been changed.
        EXPECT_EQ(1, fileSize(szFilePath));
        EXPECT_EQ(0, outputFile.writeIntermediateToFinalLocation());
        // Intermediate file should have been removed now.
        EXPECT_FALSE(isPathFileAvailable(outputFile.m_pathIntermediate, FileModeExists));
        EXPECT_EQ(3, fileSize(szFilePath));
    }

    // Basic memory intermediate test
    {
        const char szFilePath[] = "testfiles/generated/OutputFileTest_1.txt";
        OfStream::dumpBytesToFile_overwriting(szFilePath, "a", 1);
        OutputFile_completeOrNone<> outputFile(szFilePath);
        testOutputFileMemStream(outputFile, szFilePath);
    }

    // Memory intermediate test with user-given stream type.
    {
        const char szFilePath[] = "testfiles/generated/OutputFileTest_2.txt";
        OfStream::dumpBytesToFile_overwriting(szFilePath, "a", 1);
        OutputFile_completeOrNone<OfStream, DFG_MODULE_NS(io)::DFG_CLASS_NAME(OmcByteStream)<>> outputFile(szFilePath);
        testOutputFileMemStream(outputFile, szFilePath);
    }

    // Test that intermediate file name generation works if default file happens to exists.
    {
        const char szFilePath[] = "testfiles/generated/OutputFileTest_3.txt";
        OfStream::dumpBytesToFile_overwriting(szFilePath, "a", 1);
        OutputFile_completeOrNone<> outputFile0(szFilePath);
        auto& strm0 = outputFile0.intermediateFileStream();
        EXPECT_TRUE(strm0.good());
        EXPECT_TRUE(isPathFileAvailable(outputFile0.m_pathIntermediate, FileModeExists));

        OutputFile_completeOrNone<> outputFile1(szFilePath);
        auto& strm1 = outputFile1.intermediateFileStream();
        EXPECT_TRUE(strm1.good());
        EXPECT_TRUE(isPathFileAvailable(outputFile1.m_pathIntermediate, FileModeExists));

        EXPECT_NE(outputFile0.m_pathIntermediate, outputFile1.m_pathIntermediate);

        strm0.write("abc", 3);
        strm1.write("abcd", 4);

        // Verify that destination file hasn't been changed.
        EXPECT_EQ(1, fileSize(szFilePath));
        EXPECT_EQ(0, outputFile0.writeIntermediateToFinalLocation());

        // Intermediate file should have been removed now.
        EXPECT_FALSE(isPathFileAvailable(outputFile0.m_pathIntermediate, FileModeExists));
        EXPECT_EQ(3, fileSize(szFilePath));
        EXPECT_EQ("abc", DFG_MODULE_NS(io)::fileToByteContainer<std::string>(szFilePath));

        // Finalize second stream.
        EXPECT_EQ(0, outputFile1.writeIntermediateToFinalLocation());
        EXPECT_FALSE(isPathFileAvailable(outputFile1.m_pathIntermediate, FileModeExists));
        EXPECT_EQ(4, fileSize(szFilePath));
        EXPECT_EQ("abcd", DFG_MODULE_NS(io)::fileToByteContainer<std::string>(szFilePath));
    }

    // Test that empty output overwrites existing file with file intermediate stream.
    {
        const char szFilePath[] = "testfiles/generated/OutputFileTest_4.txt";
        OfStream::dumpBytesToFile_overwriting(szFilePath, "a", 1);
        {
            OutputFile_completeOrNone<> outputFile(szFilePath);
            auto& strm = outputFile.intermediateFileStream();
            DFG_UNUSED(strm);
            EXPECT_EQ(1, fileSize(szFilePath));
        }
        EXPECT_TRUE(isPathFileAvailable(szFilePath, FileModeExists));
        EXPECT_EQ(0, fileSize(szFilePath));
    }

    // Test that empty output overwrites existing file with memory intermediate stream.
    {
        const char szFilePath[] = "testfiles/generated/OutputFileTest_5.txt";
        OfStream::dumpBytesToFile_overwriting(szFilePath, "a", 1);
        {
            OutputFile_completeOrNone<> outputFile(szFilePath);
            auto& strm = outputFile.intermediateMemoryStream();
            DFG_UNUSED(strm);
            EXPECT_EQ(1, fileSize(szFilePath));
        }
        EXPECT_TRUE(isPathFileAvailable(szFilePath, FileModeExists));
        EXPECT_EQ(0, fileSize(szFilePath));
    }
}

#ifdef _WIN32 // TODO: remove #ifdef once available on other platforms
    TEST(dfgOs, pathFilename)
    {
        EXPECT_STREQ("", DFG_MODULE_NS(os)::pathFilename(static_cast<char*>(nullptr)));
        EXPECT_STREQ(L"", DFG_MODULE_NS(os)::pathFilename(static_cast<wchar_t*>(nullptr)));
        EXPECT_STREQ("b.txt", DFG_MODULE_NS(os)::pathFilename("c:\\a\\b.txt"));
        EXPECT_STREQ("b.txt", DFG_MODULE_NS(os)::pathFilename("c:/a/b.txt"));
        EXPECT_STREQ(L"a b c.txt.txt2", DFG_MODULE_NS(os)::pathFilename(L"c:/a/a b c.txt.txt2"));
        EXPECT_STREQ("a b c.txt.txt2", DFG_MODULE_NS(os)::pathFilename("a b c.txt.txt2"));
        EXPECT_STREQ("", DFG_MODULE_NS(os)::pathFilename("c:/a/"));
    }

    TEST(dfgOs, getExecutableFilePath)
    {
        const auto sPathC = DFG_MODULE_NS(os)::getExecutableFilePath<char>();
        const auto sPathW = DFG_MODULE_NS(os)::getExecutableFilePath<wchar_t>();
        EXPECT_STREQ("dfgTest.exe", DFG_MODULE_NS(os)::pathFilename(sPathC.c_str()));
        EXPECT_STREQ(L"dfgTest.exe", DFG_MODULE_NS(os)::pathFilename(sPathW.c_str()));
    }
#endif // _WIN32

#endif
