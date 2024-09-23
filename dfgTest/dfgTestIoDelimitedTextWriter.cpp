#include "stdafx.h"

#if (DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/buildConfig.hpp> // To get rid of C4996 "Function call with parameters that may be unsafe" in MSVC.
#include <dfg/io/DelimitedTextWriter.hpp>
#include <type_traits>
#include <dfg/cont.hpp>
#include <dfg/io/OmcStreamWithEncoding.hpp>

namespace
{
    static void DelimitedTextCellWriterStr_impl(const char* pszExpected, const wchar_t* pwszExpected, const char* psz, const wchar_t* pwsz, const DFG_MODULE_NS(io)::EnclosementBehaviour eb)
    {
        using DelimitedTextCellWriter = ::DFG_MODULE_NS(io)::DelimitedTextCellWriter;
        using StringViewC   = ::DFG_ROOT_NS::StringViewC;
        using StringViewW   = ::DFG_ROOT_NS::StringViewW;
        using StringViewSzC = ::DFG_ROOT_NS::StringViewSzC;
        using StringViewSzW = ::DFG_ROOT_NS::StringViewSzW;

        std::ostringstream ostrm;
        DelimitedTextCellWriter::writeCellStrm(ostrm,
            std::string(psz),
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pszExpected, ostrm.str());
        std::ostringstream ostrm2;
        DelimitedTextCellWriter::writeCellStrm(ostrm2,
            StringViewC(psz),
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pszExpected, ostrm2.str());
        std::ostringstream ostrm3;
        DelimitedTextCellWriter::writeCellStrm(ostrm3,
            StringViewSzC(psz),
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pszExpected, ostrm3.str());
        std::string sOutput;
        DelimitedTextCellWriter::writeCellIter(std::back_inserter<std::string>(sOutput),
            std::string(psz),
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pszExpected, sOutput);
        std::string sOutput2;
        DelimitedTextCellWriter::writeCellIter(std::back_inserter<std::string>(sOutput2),
            psz,
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pszExpected, sOutput2);
        std::string sOutput3;
        DelimitedTextCellWriter::writeCellIter(std::back_inserter<std::string>(sOutput3),
            StringViewC(psz),
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pszExpected, sOutput3);

        std::wostringstream owstrm;
        DelimitedTextCellWriter::writeCellStrm(owstrm,
            std::wstring(pwsz),
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pwszExpected, owstrm.str());

        std::wostringstream owstrm2;
        DelimitedTextCellWriter::writeCellStrm(owstrm2,
            StringViewW(pwsz),
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pwszExpected, owstrm2.str());

        std::wostringstream owstrm3;
        DelimitedTextCellWriter::writeCellStrm(owstrm3,
            StringViewSzW(pwsz),
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pwszExpected, owstrm3.str());

        std::wstring swOutput;
        DelimitedTextCellWriter::writeCellIter(std::back_inserter<std::wstring>(swOutput),
            std::wstring(pwsz),
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pwszExpected, swOutput);
        std::wstring swOutput2;
        DelimitedTextCellWriter::writeCellIter(std::back_inserter<std::wstring>(swOutput2),
            pwsz,
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pwszExpected, swOutput2);

        std::wstring swOutput3;
        DelimitedTextCellWriter::writeCellIter(std::back_inserter<std::wstring>(swOutput3),
            StringViewW(pwsz),
            ',',
            '"',
            '\n',
            eb);
        EXPECT_EQ(pwszExpected, swOutput3);
    }
} // unnamed namespace

TEST(dfgIo, DelimitedTextCellWriterStr)
{
    #define TEST_STRING(EXPECTED, STR, EB) DelimitedTextCellWriterStr_impl(EXPECTED, L##EXPECTED, STR, L##STR, DFG_MODULE_NS(io)::EB)

    TEST_STRING("\"abc\"", "abc", EbEnclose);
    TEST_STRING("abc", "abc", EbNoEnclose);
    TEST_STRING("abc", "abc", EbEncloseIfNeeded);

    TEST_STRING("\"\"", "", EbEnclose);
    TEST_STRING("", "", EbNoEnclose);
    TEST_STRING("", "", EbEncloseIfNeeded);

    TEST_STRING("\"a,b\"", "a,b", EbEnclose);
    TEST_STRING("a,b", "a,b", EbNoEnclose);
    TEST_STRING("\"a,b\"", "a,b", EbEncloseIfNeeded);

    TEST_STRING("\"a\"\"b\"", "a\"b", EbEnclose);
    TEST_STRING("a\"b", "a\"b", EbNoEnclose);
    TEST_STRING("\"a\"\"b\"", "a\"b", EbEncloseIfNeeded);

    TEST_STRING("\"a\nb\"", "a\nb", EbEnclose);
    TEST_STRING("a\nb", "a\nb", EbNoEnclose);
    TEST_STRING("\"a\nb\"", "a\nb", EbEncloseIfNeeded);

    TEST_STRING("\"a\"\"b\"\"\"\"\"\"c\"", "a\"b\"\"\"c", EbEnclose);
    TEST_STRING("a\"b\"\"\"c", "a\"b\"\"\"c", EbNoEnclose);
    TEST_STRING("\"a\"\"b\"\"\"\"\"\"c\"", "a\"b\"\"\"c", EbEncloseIfNeeded);

    using namespace DFG_MODULE_NS(io);
    std::string s;
    DFG_MODULE_NS(io)::OmcStreamWithEncoding<std::string> ostrmEncodingStream(&s, DFG_MODULE_NS(io)::encodingUTF16Le);
    const char szExample[4] = { 'a', -28, 'b', '\0' };
    //DelimitedTextCellWriter::writeCellStrm(ostrmEncodingStream, std::string(szExample), ',', '"', '\n', EbEncloseIfNeeded);
    DelimitedTextCellWriter::writeCellStrm(ostrmEncodingStream, szExample, ',', '"', '\n', EbEncloseIfNeeded);
    EXPECT_EQ(6, s.size());
    if (s.size() == 6)
    {
        EXPECT_EQ('a', s[0]);
        EXPECT_EQ(0, s[1]);
        EXPECT_EQ(-28, s[2]);
        EXPECT_EQ(0, s[3]);
        EXPECT_EQ('b', s[4]);
        EXPECT_EQ(0, s[5]);
    }

#undef TEST_STRING
}

TEST(dfgIo, DelimitedTextCellWriterWriteLine)
{
    using namespace DFG_MODULE_NS(io);

#define TEST_STRING(EXPECTED, CONT, EB) \
        { \
        std::basic_ostringstream<char /*TODO: use char type of EXPECTED */> ostrm; \
        DFG_MODULE_NS(io)::DelimitedTextWriter::writeMultiple(ostrm, \
                                                                            CONT, \
                                                                            ',', \
                                                                            '"', \
                                                                            '\n', \
                                                                            DFG_MODULE_NS(io)::EB); \
        \
        EXPECT_EQ(EXPECTED, ostrm.str()); \
        \
        }

    std::array<std::string, 3> vecStrings = { "a", "b", "c" };
    TEST_STRING("a,b,c", vecStrings, EbNoEnclose);
    TEST_STRING("\"a\",\"b\",\"c\"", vecStrings, EbEnclose);
    TEST_STRING("a,b,c", vecStrings, EbEncloseIfNeeded);

    std::array<std::string, 4> vecStrings2 = { "a,", "b\"", "\"c\",\n,,", "\n" };
    TEST_STRING("a,,b\",\"c\",\n,,,\n", vecStrings2, EbNoEnclose);
    TEST_STRING("\"a,\",\"b\"\"\",\"\"\"c\"\",\n,,\",\"\n\"", vecStrings2, EbEnclose);
    TEST_STRING("\"a,\",\"b\"\"\",\"\"\"c\"\",\n,,\",\"\n\"", vecStrings2, EbEncloseIfNeeded);
    

#undef TEST_STRING
}

TEST(dfgIo, DelimitedTextCellWriter_isEnclosementNeeded)
{
    using DelimitedTextCellWriter = ::DFG_MODULE_NS(io)::DelimitedTextCellWriter;
    using Sv = ::DFG_ROOT_NS::StringViewC;
    DFGTEST_EXPECT_FALSE(DelimitedTextCellWriter::isEnclosementNeeded(Sv("")  , ',', '"' , '\n'));
    DFGTEST_EXPECT_FALSE(DelimitedTextCellWriter::isEnclosementNeeded(Sv(" ") , ',', '"' , '\n'));
    DFGTEST_EXPECT_FALSE(DelimitedTextCellWriter::isEnclosementNeeded(Sv(" a"), ',', '"' , '\n'));
    DFGTEST_EXPECT_TRUE (DelimitedTextCellWriter::isEnclosementNeeded(Sv(",") , ',', '"' , '\n'));
    DFGTEST_EXPECT_FALSE(DelimitedTextCellWriter::isEnclosementNeeded(Sv(",") , ';', '"' , '\n'));
    DFGTEST_EXPECT_TRUE (DelimitedTextCellWriter::isEnclosementNeeded(Sv("\""), ',', '"' , '\n'));
    DFGTEST_EXPECT_FALSE(DelimitedTextCellWriter::isEnclosementNeeded(Sv("\""), ';', '\'', '\n'));
    DFGTEST_EXPECT_TRUE (DelimitedTextCellWriter::isEnclosementNeeded(Sv("\n"), ',', '"' , '\n'));
    DFGTEST_EXPECT_FALSE(DelimitedTextCellWriter::isEnclosementNeeded(Sv("\n"), ';', '\'', '\r'));

    DFGTEST_EXPECT_TRUE(DelimitedTextCellWriter::isEnclosementNeeded(Sv("a,"),  ',', '"', '\n'));
    DFGTEST_EXPECT_TRUE(DelimitedTextCellWriter::isEnclosementNeeded(Sv(",a"),  ',', '"', '\n'));
    DFGTEST_EXPECT_TRUE(DelimitedTextCellWriter::isEnclosementNeeded(Sv("a,b"), ',', '"', '\n'));

    DFGTEST_EXPECT_TRUE(DelimitedTextCellWriter::isEnclosementNeeded(Sv("a\""),  ',', '"', '\n'));
    DFGTEST_EXPECT_TRUE(DelimitedTextCellWriter::isEnclosementNeeded(Sv("\"a"),  ',', '"', '\n'));
    DFGTEST_EXPECT_TRUE(DelimitedTextCellWriter::isEnclosementNeeded(Sv("a\"b"), ',', '"', '\n'));

    DFGTEST_EXPECT_TRUE(DelimitedTextCellWriter::isEnclosementNeeded(Sv("a\n"),  ',', '"', '\n'));
    DFGTEST_EXPECT_TRUE(DelimitedTextCellWriter::isEnclosementNeeded(Sv("\na"),  ',', '"', '\n'));
    DFGTEST_EXPECT_TRUE(DelimitedTextCellWriter::isEnclosementNeeded(Sv("a\nb"), ',', '"', '\n'));
}

#endif
