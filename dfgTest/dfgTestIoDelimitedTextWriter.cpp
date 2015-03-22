#include <stdafx.h>
#include <dfg/buildConfig.hpp> // To get rid of C4996 "Function call with parameters that may be unsafe" in MSVC.
#include <dfg/io/DelimitedTextWriter.hpp>
#include <type_traits>
#include <dfg/cont.hpp>
#include <dfg/io/OmcStreamWithEncoding.hpp>

TEST(DfgIo, DelimitedTextCellWriterStr)
{
#define TEST_STRING(EXPECTED, STR, EB) \
        { \
        std::ostringstream ostrm; \
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellStrm(ostrm, \
                                                                            std::string(STR), \
                                                                            ',', \
                                                                            '"', \
                                                                            '\n', \
                                                                            DFG_MODULE_NS(io)::EB); \
        EXPECT_EQ(EXPECTED, ostrm.str()); \
        std::string sOutput; \
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellIter(std::back_inserter<std::string>(sOutput), \
                                                                            std::string(STR), \
                                                                            ',', \
                                                                            '"', \
                                                                            '\n', \
                                                                            DFG_MODULE_NS(io)::EB); \
        EXPECT_EQ(EXPECTED, sOutput); \
        std::string sOutput2; \
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellIter(std::back_inserter<std::string>(sOutput2), \
                                                                            STR, \
                                                                            ',', \
                                                                            '"', \
                                                                            '\n', \
                                                                            DFG_MODULE_NS(io)::EB); \
        EXPECT_EQ(EXPECTED, sOutput2); \
        \
        std::wostringstream owstrm; \
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellStrm(owstrm, \
                                                                            std::wstring(L##STR), \
                                                                            ',', \
                                                                            '"', \
                                                                            '\n', \
                                                                            DFG_MODULE_NS(io)::EB); \
        EXPECT_EQ(L##EXPECTED, owstrm.str()); \
        \
        std::wstring swOutput; \
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellIter(std::back_inserter<std::wstring>(swOutput), \
                                                                            std::wstring(L##STR), \
                                                                            ',', \
                                                                            '"', \
                                                                            '\n', \
                                                                            DFG_MODULE_NS(io)::EB); \
        EXPECT_EQ(L##EXPECTED, swOutput); \
        std::wstring swOutput2; \
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellIter(std::back_inserter<std::wstring>(swOutput2), \
                                                                            L##STR, \
                                                                            ',', \
                                                                            '"', \
                                                                            '\n', \
                                                                            DFG_MODULE_NS(io)::EB); \
        EXPECT_EQ(L##EXPECTED, swOutput2); \
        }

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
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(OmcStreamWithEncoding)<std::string> ostrmEncodingStream(&s, DFG_MODULE_NS(io)::encodingUTF16Le);
    const char szExample[4] = { 'a', -28, 'b', '\0' };
    //DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellStrm(ostrmEncodingStream, std::string(szExample), ',', '"', '\n', EbEncloseIfNeeded);
    DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellStrm(ostrmEncodingStream, szExample, ',', '"', '\n', EbEncloseIfNeeded);
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

TEST(DfgIo, DelimitedTextCellWriterWriteLine)
{
    using namespace DFG_MODULE_NS(io);

#define TEST_STRING(EXPECTED, CONT, EB) \
        { \
        std::basic_ostringstream<char /*TODO: use char type of EXPECTED */> ostrm; \
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextWriter)::writeMultiple(ostrm, \
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
