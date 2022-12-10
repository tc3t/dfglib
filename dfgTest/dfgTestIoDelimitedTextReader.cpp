#include "stdafx.h"

#if (defined(DFGTEST_BUILD_MODULE_IO_DELIM_READER) && DFGTEST_BUILD_MODULE_IO_DELIM_READER == 1) || (!defined(DFGTEST_BUILD_MODULE_IO_DELIM_READER) && DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/io/DelimitedTextReader.hpp>
#include <dfg/alg.hpp>
#include <dfg/str/format_fmt.hpp>
#include <dfg/cont.hpp>
#include <strstream>
#include <dfg/io/BasicImStream.hpp>
#include <dfg/rand.hpp>
#include <dfg/io/DelimitedTextWriter.hpp>
#include <dfg/str/string.hpp>
#include <dfg/baseConstructorDelegate.hpp>
#include <dfg/iter/szIterator.hpp>
#include <dfg/time/timerCpu.hpp>
#include <dfg/cont/tableCsv.hpp>
#include <dfg/str.hpp>

namespace
{

const size_t g_nTestCsv0ColumnCount = 3;
const size_t g_nTestCsv0RowCount	= 2;

typedef wchar_t CsvCellChar; // Can't use char16_t while there's no way to mark string literals as such (won't work with L)

const std::basic_string<CsvCellChar> g_arrTestCsv0FileCells[g_nTestCsv0ColumnCount * g_nTestCsv0RowCount] =
{
    L"\"Col1\"",		L"Col2",				L"\"_Col\"\"3\"\" \"",
    L"\"abc_\xe4\"",	L"\"a\"\"bc_\x20AC_\"",	L"\"abc_\xf6\""
};

const std::basic_string<CsvCellChar> g_arrTestCsv0ExpectedFromReadCells[DFG_COUNTOF(g_arrTestCsv0FileCells)] =
{
    L"Col1",		L"Col2",			L"_Col\"3\" ",
    L"abc_\xe4",	L"a\"bc_\x20AC_",	L"abc_\xf6"
};

std::tuple<const std::basic_string<CsvCellChar>*, const std::basic_string<CsvCellChar>*, size_t, size_t> csvTestSets[] =
{
    std::tuple<const std::basic_string<CsvCellChar>*, const std::basic_string<CsvCellChar>*, size_t, size_t>(g_arrTestCsv0FileCells, g_arrTestCsv0ExpectedFromReadCells, DFG_COUNTOF(g_arrTestCsv0FileCells), g_nTestCsv0ColumnCount)
};

const std::basic_string<CsvCellChar>* getCsvTestWriteArray(size_t nIndex)
{
    if (DFG_ROOT_NS::isValidIndex(csvTestSets, nIndex))
        return std::get<0>(csvTestSets[nIndex]);
    else
        throw std::out_of_range("Bad index");
}

size_t getCsvTestCellCount(size_t nIndex)
{
    if (DFG_ROOT_NS::isValidIndex(csvTestSets, nIndex))
        return std::get<2>(csvTestSets[nIndex]);
    else
        throw std::out_of_range("Bad index");
}

const std::basic_string<CsvCellChar>& getExpectedCellData(size_t nFileIndex, size_t nRow, size_t nCol)
{
    if (!DFG_ROOT_NS::isValidIndex(csvTestSets, nFileIndex))
        throw std::out_of_range("Bad index");
    const size_t index = DFG_ROOT_NS::pairIndexToLinear(nRow, nCol, std::get<3>(csvTestSets[nFileIndex]));
    if (index < std::get<2>(csvTestSets[nFileIndex]))
        return std::get<1>(csvTestSets[nFileIndex])[index];
    else
        throw std::out_of_range("Bad index");
}

void createTestFile(const std::string& sFile, const DFG_MODULE_NS(io)::TextEncoding encoding, const bool bWriteBom, const std::basic_string<CsvCellChar>* pArr, const size_t nCount, const size_t nColCount)
{
    using namespace DFG_MODULE_NS(io);

    DFG_CLASS_NAME(OfStreamWithEncoding) ostrm(sFile, encoding, bWriteBom);

    for (size_t i = 0; i < nCount; ++i)
    {
        DFG_MODULE_NS(alg)::forEachFwd(pArr[i], [&](CsvCellChar ch)
        {
            ostrm.writeUnicodeChar(static_cast<DFG_ROOT_NS::uint32>(ch));
        });
        if ((i + 1) % nColCount != 0)
            ostrm << ", ";
        else if (i + 1 < nCount)
            ostrm << '\n';
    }
}

void createTestFileCsvRandomData(const std::string& sFile)
{
    using namespace DFG_ROOT_NS;

    std::basic_ofstream<char> ostrm(sFile, std::ios::out | std::ios::binary);
    std::basic_ofstream<char> ostrmExpected(sFile + "_expected.txt", std::ios::out | std::ios::binary);
    const size_t nColSizes[] = {5, 2, 3, 4, 15, 67, 2, 1, 3, 55, 12, 3};

    auto randEng = DFG_SUB_NS_NAME(rand)::createDefaultRandEngineUnseeded();
    randEng.seed(4657);
    auto distrEng = DFG_SUB_NS_NAME(rand)::makeDistributionEngineUniform(&randEng, 29, 127);
    auto distrEng01 = DFG_SUB_NS_NAME(rand)::makeDistributionEngineUniform(&randEng, 0.0, 1.0);

    const char cSep = ',';
    const char cEnc = '"';
    const char cEol = '\n';

#ifdef _DEBUG
    const size_t nRowCount = DFG_COUNTOF(nColSizes) + 1;
#else
    const size_t nRowCount = 600;
#endif

    for(size_t r = 0; r<nRowCount; ++r)
    {
        const auto nColCount = nColSizes[r % count(nColSizes)];
        for(size_t c = 0; c<nColCount; ++c)
        {
            std::string s;
            while(distrEng01() > 0.1)
            {
                const auto val = distrEng();
                if (val >= 32)
                    s.push_back(static_cast<char>(val));
                else if (val == 29)
                    s.push_back(cSep);
                else if (val == 30)
                    s.push_back(cEnc);
                else
                    s.push_back(cEol);
            }
            DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellStrm(ostrm,
                                                                        s,
                                                                        cSep,
                                                                        cEnc,
                                                                        cEol,
                                                                        DFG_SUB_NS_NAME(io)::EbEncloseIfNeeded);
            ostrmExpected << r << c;
            ostrmExpected.write(s.data(), s.size());
            if (c + 1 < nColCount)
                ostrm << cSep;
            else
                ostrm << cEol;
        }
    }
}

std::string createCsvTestFileName(const char* const pszId, const size_t nIndex)
{
    return ::DFG_ROOT_NS::format_fmt("testfiles/generated/csvtest{0}_{1}.csv", pszId, nIndex);
}

} // unnamed namespace

#define CREATE_TESTFILE(PATH, ENCODING, WRITE_BOM, ARRAYINDEX, COLCOUNT) \
    { \
    createTestFile(createCsvTestFileName(PATH, 0), ENCODING, WRITE_BOM, getCsvTestWriteArray(ARRAYINDEX), getCsvTestCellCount(ARRAYINDEX), COLCOUNT); \
    } \



TEST(DfgIo, DelimitedTextReader_createTestFiles)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    CREATE_TESTFILE("UTF16BE_BOM",	encodingUTF16Be, true,  0, g_nTestCsv0ColumnCount);
    CREATE_TESTFILE("UTF16BE",      encodingUTF16Be, false, 0, g_nTestCsv0ColumnCount);
    CREATE_TESTFILE("UTF16LE_BOM",  encodingUTF16Le, true,  0, g_nTestCsv0ColumnCount);
    CREATE_TESTFILE("UTF16LE",      encodingUTF16Le, false, 0, g_nTestCsv0ColumnCount);
    CREATE_TESTFILE("UTF8_BOM",		encodingUTF8,    true,  0, g_nTestCsv0ColumnCount);
    CREATE_TESTFILE("UTF8",			encodingUTF8,    false, 0, g_nTestCsv0ColumnCount);

    // Create UTF32-files by hand.
    const auto utf8LeFile = DFG_MODULE_NS(io)::fileToVector(createCsvTestFileName("UTF8", 0));
    const auto sPathUtf32Le = createCsvTestFileName("UTF32LE_BOM", 0);
    const auto sPathUtf32Be = createCsvTestFileName("UTF32BE_BOM", 0);
    const auto sPathUtf32LeNoBom = createCsvTestFileName("UTF32LE", 0);
    const auto sPathUtf32BeNoBom = createCsvTestFileName("UTF32BE", 0);
    std::vector<uint32> le32Data;
    utf8::utf8to32(utf8LeFile.cbegin(), utf8LeFile.cend(), std::back_inserter(le32Data));
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStream) strmLe32(sPathUtf32Le);
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStream) strmBe32(sPathUtf32Be);
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStream) strmLe32NoBom(sPathUtf32LeNoBom);
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStream) strmBe32NoBom(sPathUtf32BeNoBom);
    writeBinary(strmLe32, DFG_MODULE_NS(utf)::bomUTF32Le);
    writeBinary(strmBe32, DFG_MODULE_NS(utf)::bomUTF32Be);
    DFG_MODULE_NS(alg)::forEachFwd(le32Data, [&](const uint32 val)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);
        writeBinary(strmLe32, byteSwapHostToLittleEndian(val));
        writeBinary(strmLe32NoBom, byteSwapHostToLittleEndian(val));
        writeBinary(strmBe32, byteSwapHostToBigEndian(val));
        writeBinary(strmBe32NoBom, byteSwapHostToBigEndian(val));
    });
}

static void DelimitedTextReader_CsvReadCommon(const char* const pszFileId, DFG_MODULE_NS(io)::TextEncoding encoding = DFG_MODULE_NS(io)::encodingUnknown)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    const auto sFilePath = createCsvTestFileName(pszFileId, 0);

    size_t nCellCount = 0;
    DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::CellData<CsvCellChar> cellData(',', '"', '\n');

    const auto cellHandler = [&](const size_t nRow, const size_t nCol, const decltype(cellData)& data)
                            {
                                const auto& sExpected = getExpectedCellData(0, nRow, nCol);
                                EXPECT_EQ(sExpected, data.getBuffer());
                                ++nCellCount;
                            };

    
    DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::readFromFile(sFilePath, cellData, cellHandler, encoding);
    EXPECT_EQ(getCsvTestCellCount(0), nCellCount);

    // Now the same but from in-memory stream.
    nCellCount = 0;
    const auto fileData = DFG_MODULE_NS(io)::fileToVector(sFilePath);
    DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::readFromMemory(fileData.data(), fileData.size(), cellData, cellHandler, encoding);
}

TEST(DfgIo, DelimitedTextReader_CsvReadUTF8_withBOM)
{
    DelimitedTextReader_CsvReadCommon("UTF8_BOM");
}

TEST(DfgIo, DelimitedTextReader_CsvReadUTF8_noBOM)
{
    DelimitedTextReader_CsvReadCommon("UTF8", DFG_MODULE_NS(io)::encodingUTF8);
}

TEST(DfgIo, DelimitedTextReader_CsvReadUTF16LE_withBOM)
{
    DelimitedTextReader_CsvReadCommon("UTF16LE_BOM");
}

TEST(DfgIo, DelimitedTextReader_CsvReadUTF16LE_noBOM)
{
    DelimitedTextReader_CsvReadCommon("UTF16LE", DFG_MODULE_NS(io)::encodingUTF16Le);
}

TEST(DfgIo, DelimitedTextReader_CsvReadUTF16BE_withBOM)
{
    DelimitedTextReader_CsvReadCommon("UTF16BE_BOM");
}

TEST(DfgIo, DelimitedTextReader_CsvReadUTF16BE_noBOM)
{
    DelimitedTextReader_CsvReadCommon("UTF16BE", DFG_MODULE_NS(io)::encodingUTF16Be);
}

TEST(DfgIo, DelimitedTextReader_CsvReadUTF32LE_withBOM)
{
    DelimitedTextReader_CsvReadCommon("UTF32LE_BOM");
}

TEST(DfgIo, DelimitedTextReader_CsvReadUTF32LE_noBOM)
{
    DelimitedTextReader_CsvReadCommon("UTF32LE", DFG_MODULE_NS(io)::encodingUTF32Le);
}

TEST(DfgIo, DelimitedTextReader_CsvReadUTF32BE_withBOM)
{
    DelimitedTextReader_CsvReadCommon("UTF32BE_BOM");
}

TEST(DfgIo, DelimitedTextReader_CsvReadUTF32BE_noBOM)
{
    DelimitedTextReader_CsvReadCommon("UTF32BE", DFG_MODULE_NS(io)::encodingUTF32Be);
}

TEST(DfgIo, DelimitedTextReader_CsvRead_plainBytes)
{
}

TEST(DfgIo, DelimitedTextReader_CharsetWindows1250_Read)
{
    using namespace DFG_ROOT_NS;

    //TODO

    const std::string sFilePath = "testfiles/csvWindows1250.csv";
}

TEST(DfgIo, DelimitedTextReader_CharsetANSI_Read)
{
    using namespace DFG_ROOT_NS;

    //TODO

    const std::string sFilePath = "testfiles/csvANSI.csv";
}

namespace
{
    template <class CharBuffer_T, class CharAppender_T, size_t N>
    void DelimitedTextReader_readCellImpl(const std::tuple<std::string, std::string, int> (&arrCellExpected)[N])
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);
        for (size_t i = 0; i < count(arrCellExpected); ++i)
        {
            const auto& expected = arrCellExpected[i];
            BasicImStream strm(std::get<0>(expected).data(), std::get<0>(expected).size());

            DelimitedTextReader::CellData<char, char, CharBuffer_T, CharAppender_T> cellDataHandler(',', '"', '\n');
            auto reader = DelimitedTextReader::createReader(strm, cellDataHandler);

            DelimitedTextReader::readCell(reader);
            const auto chNext = strm.get();

            std::string sDest;
            sDest.assign(cellDataHandler.getBuffer().begin(), cellDataHandler.getBuffer().end());
            EXPECT_EQ(std::get<1>(expected), sDest);
            EXPECT_EQ(std::get<2>(expected), chNext);
        }
    }
}

TEST(DfgIo, DelimitedTextReader_readCell)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    //CreateTestMatrixFile(3);
    //CreateTestFile(200);
    //CreateTestMatrixFile(200);
    //CreateTestMatrixFile(1000);
    //CreateTestMatrixFile(2000);

    // text, expected parsed text, expected next char from stream.
    typedef std::tuple<std::string, std::string, int> ExpectedResultsT;
    const auto eofGetVal = std::istream::traits_type::eof();
    const ExpectedResultsT arrCellExpected[] =
    {
        ExpectedResultsT("abcdefg", "abcdefg", eofGetVal),
        ExpectedResultsT("a b c d e f g", "a b c d e f g", eofGetVal),
        ExpectedResultsT("asd\tasd", "asd\tasd", eofGetVal),
        ExpectedResultsT("asd\"\"asd", "asd\"\"asd", eofGetVal),
        ExpectedResultsT("\"asd\"\"asd\"", "asd\"asd", eofGetVal),
        ExpectedResultsT("\"asd\"\"\"\"asd\"", "asd\"\"asd", eofGetVal),
        ExpectedResultsT("\"asd\"\"\"\"asd\"\"\"", "asd\"\"asd\"", eofGetVal),
        ExpectedResultsT("\"  asd\"\"asd\"", "  asd\"asd", eofGetVal),
        ExpectedResultsT("  \" asd\"\"asd\"  ", " asd\"asd", eofGetVal),
        ExpectedResultsT("  \"as,d,\"\",asd\"  ", "as,d,\",asd", eofGetVal),
        ExpectedResultsT("  \"as,d,\"\",a s  d\",", "as,d,\",a s  d", eofGetVal),
        ExpectedResultsT("  \"as,d,\"\",asd\"  ,", "as,d,\",asd", eofGetVal),
        ExpectedResultsT("a b,c", "a b", 'c'),

        ExpectedResultsT("a b,    c", "a b", ' '),
        ExpectedResultsT("\"a b\",c", "a b", 'c'),

        ExpectedResultsT("  \"a b\"   , c", "a b", ' '),
        ExpectedResultsT("  \"a b\"   ,  c", "a b", ' '),
        ExpectedResultsT("  \"a b  \"   ,  c", "a b  ", ' '),
        ExpectedResultsT("  \"a,\"\",b\"   ,tc", "a,\",b", 't'),
        ExpectedResultsT("  \"a,\"\",\r,\n,\r\n,b\"   ,tc", "a,\",\r,\n,\r\n,b", 't'),
        ExpectedResultsT("\"a \r\" \na", "a \r", 'a'), // Testing that \r\n translation doesn't get confused if last in enclosed cell is \r and there are skipped items before cell ending \n.

        ExpectedResultsT("abc\ndef", "abc", 'd'),
        ExpectedResultsT("abc\r\ndef", "abc", 'd'),
        ExpectedResultsT("\"abc\ndef\"", "abc\ndef", eofGetVal),
        ExpectedResultsT("\"abc\r\ndef\"", "abc\r\ndef", eofGetVal),

        ExpectedResultsT("\"\"\"\"", "\"", eofGetVal), // Test reading of consecutive enclosing chars """" -> "
        ExpectedResultsT("\"\"\"\"\"\"", "\"\"", eofGetVal), // Test reading of consecutive enclosing chars """""" -> ""
        ExpectedResultsT("\"\"\",\"\"\"\"\"", "\",\"\"", eofGetVal), // Test reading of consecutive enclosing chars """,""""" -> ",""

        // Test empty cell reading.
        ExpectedResultsT("\"\"", "", eofGetVal),
        ExpectedResultsT("\"\", ", "", ' '),
        ExpectedResultsT("   ", "", eofGetVal),

        ExpectedResultsT("\" \"", " ", eofGetVal),
        ExpectedResultsT("\" \"ab,b", " ", 'b'), // Malformed cell " "ab  Simply ignore trailing chars.
        ExpectedResultsT("\"\"ab,b", "", 'b'), // Malformed cell ""ab     Simply ignore trailing chars.
    };

    DelimitedTextReader_readCellImpl<DelimitedTextReader::CharBuffer<char>, DelimitedTextReader::CharAppenderDefault<DelimitedTextReader::CharBuffer<char>, char>>(arrCellExpected);
    DelimitedTextReader_readCellImpl<DelimitedTextReader::StringViewCBufferWithEnclosedCellSupport, DelimitedTextReader::CharAppenderStringViewCBufferWithEnclosedCellSupport>(arrCellExpected);
}

TEST(DfgIo, DelimitedTextReader_readRow)
{
    using namespace DFG_ROOT_NS;
    using DelimitedTextReader = ::DFG_MODULE_NS(io)::DelimitedTextReader;

#define MAKE_VECTOR DFG_SUB_NS_NAME(cont)::makeVector<std::string>

    // text, expected cell texts.
    typedef std::tuple<std::string, std::vector<std::string>> ExpectedResultsT;
    const ExpectedResultsT arrCellExpected[] =
    {
        ExpectedResultsT("asd, asd2, \"asd\"\",3\"\nabc, def", MAKE_VECTOR("asd", "asd2", "asd\",3")),
    };

    for(size_t i = 0; i<count(arrCellExpected); ++i)
    {
        const auto& expected = arrCellExpected[i];
        const auto& sInput = std::get<0>(expected);
        const auto& exceptedCells = std::get<1>(expected);
        ::DFG_MODULE_NS(io)::BasicImStream strm(sInput);
        ::DFG_MODULE_NS(io)::BasicImStream strm2(sInput);
        ::DFG_MODULE_NS(io)::BasicImStream strm3(sInput.c_str(), sInput.size());

        DelimitedTextReader::CellData<char> cellDataHandler(',', '"', '\n');
        auto reader = DelimitedTextReader::createReader(strm, cellDataHandler);

        std::vector<std::string> vecStrings;
        auto cellHandler = [&](const size_t nCol, const char* const p, const size_t nSize)
            {
                EXPECT_EQ(nCol, vecStrings.size());
                vecStrings.push_back(std::string(p, p + nSize));
            };
        auto cellHandler2 = [&](const size_t nCol, const decltype(cellDataHandler)& cdh) { cellHandler(nCol, cdh.getBuffer().data(), cdh.getBuffer().size()); };

        DelimitedTextReader::readRow(reader, cellHandler2);
        EXPECT_EQ(exceptedCells, vecStrings);
        vecStrings.clear();
        DelimitedTextReader::readRow<char>(strm2, ',', '"', '\n', cellHandler);
        EXPECT_EQ(exceptedCells, vecStrings);
        vecStrings.clear();
        DelimitedTextReader::readRow<char>(strm3, CsvFormatDefinition(',', '"', ::DFG_MODULE_NS(io)::EndOfLineTypeN, ::DFG_MODULE_NS(io)::encodingUnknown), cellHandler);
        EXPECT_EQ(exceptedCells, vecStrings);

        // Testing that readRow() accepts rvalue stream
        vecStrings.clear();
        DelimitedTextReader::readRow<char>(::DFG_MODULE_NS(io)::BasicImStream(std::get<0>(expected)), ',', '"', '\n', cellHandler);
        EXPECT_EQ(exceptedCells, vecStrings);
    }

    // Test 'skip rest of row'-behaviour
    {
        const auto& expected = ExpectedResultsT("a,b,c,d,e,f", MAKE_VECTOR("c"));
        ::DFG_MODULE_NS(io)::BasicImStream strm(std::get<0>(expected).c_str(), std::get<0>(expected).size());

        DelimitedTextReader::CellData<char> cellDataHandler(',', '"', '\n');
        auto reader = DelimitedTextReader::createReader(strm, cellDataHandler);

        std::vector<std::string> vecStrings;
        size_t nCellHandlerCallCount = 0;
        auto cellHandler = [&](const size_t nCol, decltype(cellDataHandler)& cdh)
                            {
                                ++nCellHandlerCallCount;
                                if (nCol == 2)
                                {
                                    vecStrings.push_back(std::string(cdh.getBuffer().begin(), cdh.getBuffer().end()));
                                    cdh.setReadStatus(DelimitedTextReader::cellHrvSkipRestOfLine);
                                }

                            };

        DelimitedTextReader::readRow(reader, cellHandler);

        EXPECT_EQ(3, nCellHandlerCallCount);
        EXPECT_EQ(std::get<1>(expected), vecStrings);
    }

#undef MAKE_VECTOR
}

TEST(DfgIo, DelimitedTextReader_autoDetectCsvSeparator)
{
    const char szNoSep[] = "abc";
    const char szCommaSep[] = "a,b";
    const char szTabSep[] = "a\tb";
    const char szSemicolonSep[] = "a;b";
    const char szEnclosedSep[] = "\"a;b\",c";
    const char szEnclosedWithEolSep[] = "\"a\nb\"\tc";
    const char szUnitSeparatorSep[] = "a\x1f" "b";

    using namespace DFG_MODULE_NS(io);
    const auto noSep = DFG_CLASS_NAME(DelimitedTextReader)::autoDetectCsvSeparatorFromString(szNoSep, '"', '\n');
    const auto sepComma = DFG_CLASS_NAME(DelimitedTextReader)::autoDetectCsvSeparatorFromString(szCommaSep, '"', '\n');
    const auto sepTab = DFG_CLASS_NAME(DelimitedTextReader)::autoDetectCsvSeparatorFromString(szTabSep);
    const auto sepSemicolon = DFG_CLASS_NAME(DelimitedTextReader)::autoDetectCsvSeparatorFromString(szSemicolonSep);
    const auto sepEnclosedDefault = DFG_CLASS_NAME(DelimitedTextReader)::autoDetectCsvSeparatorFromString(szEnclosedSep);
    const auto sepEnclosedNoEnc = DFG_CLASS_NAME(DelimitedTextReader)::autoDetectCsvSeparatorFromString(szEnclosedSep, 'Q', '\n');
    const auto sepEnclosedWithEolSepDefault = DFG_CLASS_NAME(DelimitedTextReader)::autoDetectCsvSeparatorFromString(szEnclosedWithEolSep);
    const auto sepEnclosedWithEolSepNoEnc = DFG_CLASS_NAME(DelimitedTextReader)::autoDetectCsvSeparatorFromString(szEnclosedWithEolSep, 'Q', '\n');
    const auto sepUnitSeparator = DFG_CLASS_NAME(DelimitedTextReader)::autoDetectCsvSeparatorFromString(szUnitSeparatorSep, 'Q', '\n');

    EXPECT_EQ(DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone  , noSep);
    EXPECT_EQ(','                                                   , sepComma);
    EXPECT_EQ('\t'                                                  , sepTab);
    EXPECT_EQ(';'                                                   , sepSemicolon);
    EXPECT_EQ(','                                                   , sepEnclosedDefault);
    EXPECT_EQ(';'                                                   , sepEnclosedNoEnc);
    EXPECT_EQ('\t'                                                  , sepEnclosedWithEolSepDefault);
    EXPECT_EQ(DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone  , sepEnclosedWithEolSepNoEnc);
    EXPECT_EQ('\x1f'                                                , sepUnitSeparator);

    // Test basic usage with enclosed content (test case for issue#2).
    {
        using namespace DFG_MODULE_NS(io);
        const char bytes[] = "\"a\",b";
        DFG_CLASS_NAME(BasicImStream) istrm(bytes, DFG_COUNTOF_SZ(bytes));
        std::vector<std::string> items;
        const auto formatDef = DFG_CLASS_NAME(DelimitedTextReader)::read<char>(istrm, DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect, '"', '\n', [&](const size_t, const size_t, const char* p, const size_t dataSize)
        {
            items.push_back(std::string(p, dataSize));
        });
        EXPECT_EQ(',', formatDef.getSep());
        ASSERT_EQ(2, items.size());
        EXPECT_EQ("a", items[0]);
        EXPECT_EQ("b", items[1]);
    }
}

TEST(DfgIo, DelimitedTextReader_readRowSkipWhiteSpaces)
{
    using namespace DFG_MODULE_NS(io);
    const char* arrSources[] =
    {
        " a\t  b\t  \t\tc\t  ",
        " a,\t  b,\t  ,\t,\t,c\t,  "
    };
    const char arrSeparators[] = {'\t', ','};
    std::vector<std::string> vecRead;
    std::array<std::string, 6> arrExpected0NotSkipped = { " a", "  b", "  ", "", "c", "  " };
    std::array<std::string, 6> arrExpected0Skipped = { "a", "b", "", "", "c", "" };
    std::array<std::string, 7> arrExpected1NotSkipped = { " a", "\t  b", "\t  ", "\t", "\t", "c\t", "  " };
    std::array<std::string, 7> arrExpected1Skipped = { "a", "b", "", "", "", "c\t", "" };
    std::vector<std::string> expectedNotSkipped[] = 
    {
        std::vector<std::string>(arrExpected0NotSkipped.begin(), arrExpected0NotSkipped.end()),
        std::vector<std::string>(arrExpected1NotSkipped.begin(), arrExpected1NotSkipped.end())
    };
    std::vector<std::string> expectedSkipped[] =
    {
        std::vector<std::string>(arrExpected0Skipped.begin(), arrExpected0Skipped.end()),
        std::vector<std::string>(arrExpected1Skipped.begin(), arrExpected1Skipped.end())
    };

    DFG_STATIC_ASSERT(DFG_COUNTOF(arrSources) == DFG_COUNTOF(arrSeparators), "");
    DFG_STATIC_ASSERT(DFG_COUNTOF(arrSeparators) == DFG_COUNTOF(expectedNotSkipped), "");
    DFG_STATIC_ASSERT(DFG_COUNTOF(expectedNotSkipped) == DFG_COUNTOF(expectedSkipped), "");
    

    for (size_t i = 0; i < DFG_ROOT_NS::count(arrSources); ++i)
    {
        const auto impl = [&](const bool bSkip)
                        {
                            using namespace DFG_MODULE_NS(io);
                            BasicImStream strm(arrSources[i], std::strlen(arrSources[i]));
                            DFG_CLASS_NAME(DelimitedTextReader)::CellData<char> cellDataHandler(arrSeparators[i], '"', '\n');
                            cellDataHandler.getFormatDefInfo().setFlag(DFG_CLASS_NAME(DelimitedTextReader)::rfSkipLeadingWhitespaces, bSkip);
                            auto reader = DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cellDataHandler);
                            std::vector<std::string> vecRead;
                            DFG_CLASS_NAME(DelimitedTextReader)::readRow(reader, [&](size_t /*nCol*/, const decltype(cellDataHandler)& cd)
                            {
                                vecRead.push_back(cd.getBuffer());
                            });
                            if (bSkip)
                                EXPECT_EQ(expectedSkipped[i], vecRead);
                            else
                                EXPECT_EQ(expectedNotSkipped[i], vecRead);
                        };

        impl(true);
        impl(false);
    }

}

TEST(DfgIo, DelimitedTextReader_readRowConsecutiveSeparators)
{
    using namespace DFG_MODULE_NS(io);

    const auto testImpl = [](const char* const psz, const int cDelim)
                            {
                                using namespace DFG_MODULE_NS(io);
                                std::vector<std::string> vecRead;
                                std::array<std::string, 7> arrExpected = { "", "", "a", "", "", "b", "" };
                                std::vector<std::string> vecExpected(arrExpected.begin(), arrExpected.end());
                                BasicImStream strm(psz, std::strlen(psz));
                                DFG_CLASS_NAME(DelimitedTextReader)::readRow<char>(strm, cDelim, '"', '\n', [&](const size_t /*nCol*/, const char* const p, const size_t nSize)
                                {
                                    vecRead.push_back(std::string(p, nSize));
                                });

                                EXPECT_EQ(vecExpected, vecRead);
                            };

    testImpl(",,a,,,b,", ',');
    testImpl(",,a,,,b,", DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect);
    testImpl("\t\ta\t\t\tb\t", '\t');
    testImpl("\t\ta\t\t\tb\t", DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect);
    testImpl(";;a;;;b;", ';');
    testImpl(";;a;;;b;", DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect);
    testImpl("  a   b ", ' ');
}



TEST(DfgIo, DelimitedTextReader_readWithSkipRestOfRowBehaviourHasEnclosingItem)
{
    using namespace DFG_ROOT_NS;

    const std::string s =	"a,b,c\n"
                            "d,e,f\n"
                            "h,i,j,k\n"
                            "l,\"m\",n\n"
                            "o,p";
    const std::array<std::string, 8> arrExpected = { "a", "b", "d", "l", "m", "n", "o", "p" };
    const std::vector<std::string> contExpected(DFG_ROOT_NS::cbegin(arrExpected), DFG_ROOT_NS::cend(arrExpected));
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strm(s.c_str(), s.size());

    DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::CellData<char> cellDataHandler(',', '"', '\n');
    auto reader = DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cellDataHandler);

    std::vector<std::string> vecStrings;
    size_t nCellHandlerCallCount = 0;
    auto cellHandler = [&](const size_t nRow, const size_t nCol, decltype(cellDataHandler)& cdh)
    {
        ++nCellHandlerCallCount;
        if (nRow == 2)
        {
            cdh.setReadStatus(::DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::cellHrvSkipRestOfLine);
            return;
        }
        vecStrings.push_back(cdh.getBuffer());
        if ((nRow == 0 && nCol == 1) ||
            nRow == 1)
            cdh.setReadStatus(::DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::cellHrvSkipRestOfLine);
    };

    DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::read(reader, cellHandler);

    EXPECT_EQ(9, nCellHandlerCallCount);
    EXPECT_EQ(contExpected, vecStrings);
}

TEST(DfgIo, DelimitedTextReader_readWithSkipRestOfRowBehaviourNoEnclosingItem)
{
    using namespace DFG_ROOT_NS;

    const std::string s = "a,b,c\n"
        "d,e,f\n"
        "h,i,j,k\n"
        "l,m,n\n"
        "o,p";
    const std::array<std::string, 8> arrExpected = { "a", "b", "d", "l", "m", "n", "o", "p" };
    const std::vector<std::string> contExpected(DFG_ROOT_NS::cbegin(arrExpected), DFG_ROOT_NS::cend(arrExpected));
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strm(s.c_str(), s.size());

    DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::CellData<char> cellDataHandler(',', -1, '\n');
    auto reader = DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cellDataHandler);

    std::vector<std::string> vecStrings;
    size_t nCellHandlerCallCount = 0;
    auto cellHandler = [&](const size_t nRow, const size_t nCol, decltype(cellDataHandler)& cdh)
    {
        ++nCellHandlerCallCount;
        if (nRow == 2)
        {
            cdh.setReadStatus(::DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::cellHrvSkipRestOfLine);
            return;
        }
        vecStrings.push_back(cdh.getBuffer());
        if ((nRow == 0 && nCol == 1) ||
            nRow == 1)
            cdh.setReadStatus(::DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::cellHrvSkipRestOfLine);
    };

    DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::read(reader, cellHandler);

    EXPECT_EQ(9, nCellHandlerCallCount);
    EXPECT_EQ(contExpected, vecStrings);
}

TEST(DfgIo, DelimitedTextReader_readSkipRestOfRowOnLastEmptyItem)
{
    using namespace DFG_MODULE_NS(io);
    const std::string s = "a,b,\n"
                        "c,d,\n"
                        "\n"
                        "e,f";
    DFG_CLASS_NAME(BasicImStream) strm(s.c_str(), s.size());
    std::array<std::string, 6> contExpected = {"a", "b", "c", "d", "e", "f"};

    DFG_CLASS_NAME(DelimitedTextReader)::CellData<char> cellDataHandler(',', -1, '\n');
    auto reader = DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cellDataHandler);

    std::vector<std::string> contReadValues;
    size_t nHandlerCallCount = 0;
    DFG_CLASS_NAME(DelimitedTextReader)::read(reader, [&](const size_t nRow, const size_t nCol, decltype(cellDataHandler)& cdh)
    {
        DFG_UNUSED(nRow);
        DFG_UNUSED(nCol);
        nHandlerCallCount++;
        if (cdh.getBuffer().empty())
        {
            cdh.setReadStatus(::DFG_ROOT_NS::DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::cellHrvSkipRestOfLine);
            return;
        }
        contReadValues.push_back(cdh.getBuffer());
    });

    EXPECT_EQ(9, nHandlerCallCount);
    ASSERT_EQ(contExpected.size(), contReadValues.size());
    for(size_t i = 0; i<contReadValues.size(); ++i)
        EXPECT_EQ(contExpected[i], contReadValues[i]);
}

TEST(DfgIo, DelimitedTextReader_readTerminateRead)
{
    using namespace DFG_MODULE_NS(io);

    const char szTest[] =   "ab\n"
                            "cd,ef,gh\n"
                            "ij\n"
                            "kl,,\n"
                            "\n"
                            "mn";

    const DFG_CLASS_NAME(DelimitedTextReader)::cellHandlerRv skipTypes[] = {
        DFG_CLASS_NAME(DelimitedTextReader)::cellHrvTerminateRead,
        DFG_CLASS_NAME(DelimitedTextReader)::cellHrvSkipRestOfLineAndTerminate,
    };
    std::array<size_t, 6> arrExpectedTellgs[DFG_COUNTOF(skipTypes)] = 
    {
        {  6,  9, 12, 15, 18, 21 },
        { 12, 12, 12, 15, 20, 21 }
    };
    for (size_t nSkipType = 0; nSkipType < DFG_ROOT_NS::count(skipTypes); ++nSkipType)
    {
        const auto& expectedTellgs = arrExpectedTellgs[nSkipType];
        const auto skipType = skipTypes[nSkipType];
        for (size_t i = 0; i < 6; ++i)
        {
            DFG_CLASS_NAME(DelimitedTextReader)::CellData<char> cellData(',', '"', '\n');

            DFG_CLASS_NAME(BasicImStream) strm(szTest, DFG_COUNTOF_SZ(szTest));
            auto reader = DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cellData);
            DFG_CLASS_NAME(DelimitedTextReader)::read(reader, [&](size_t r, size_t c, decltype(cellData)& cd)
            {
                if (i == 0 && r == 1 && c == 0) // Test termination on first item on row.
                    cd.setReadStatus(skipType);
                else if (i == 1 && r == 1 && c == 1) // Test termination in middle of row.
                    cd.setReadStatus(skipType);
                else if (i == 2 && r == 1 && c == 2) // Test termination on last on row.
                    cd.setReadStatus(skipType);
                else if (i == 3 && r == 2 && c == 0) // Test termination on first item on single item row.
                    cd.setReadStatus(skipType);
                else if (i == 4 && r == 3 && c == 0) // Test termination on first item on only non-empty item.
                    cd.setReadStatus(skipType);
                else if (i == 5 && r == 4) // Test termination on empty row.
                    cd.setReadStatus(skipType);
            });
            EXPECT_EQ(expectedTellgs[i], strm.tellg());
        }
    }
}

TEST(DfgIo, DelimitedTextReader_readRowMatrix)
{
    using namespace DFG_ROOT_NS;

    const std::string str = "3458, 35547, 32826, 49881, 19705, 5588, 26629, 15554, 23371, 14798, 16389, 32219, 1597, 4237, 7371, 35113, 6337, 30421, 27418, 45975, 10175, 25688, 35184, 24650, 18118, 4410, 3314, 37347, 7753, 701, 34731, 1458, 47878, 12676, 14701, 19977, 9002, 25315, 49158, 31194, 19565, 34928, 8525, 28942, 34585, 1060, 26717, 5763, 36309, 32569, 16920, 5385, 1659, 2962, 19056, 45954, 39281, 43274, 36897, 24266, 26694, 25324, 32292, 8665, 41941, 16524, 14944, 16252, 15404, 37787, 45642, 44001, 39683, 37467, 16568, 12273, 39375, 38445, 46893, 30967, 27129, 34115, 26165, 16020, 34171, 1883, 36962, 33783, 27005, 16101, 13532, 38846, 43845, 46157, 38657, 11253, 21659, 49276, 44295, 7730, 49015, 46028, 39694, 44884, 242, 15533, 33432, 23564, 30127, 13979, 28309, 23487, 46252, 39920, 18475, 7780, 19479, 49252, 37224, 26976, 12371, 45144, 16376, 3484, 12075, 24606, 17309, 48700, 33971, 23256, 1104, 4577, 21782, 47305, 33544, 27605, 2446, 27744, 19090, 8321, 27032, 31698, 40057, 36490, 48643, 33357, 13949, 21742, 36909, 20985, 5061, 5401, 46390, 47173, 48065, 49346, 35196, 23988, 18280, 9172, 37733, 3369, 23680, 11271, 27599, 27256, 13211, 12636, 34565, 24350, 9747, 1922, 41901, 49479, 21853, 33281, 48952, 19712, 49903, 43663, 6407, 10709, 21651, 15334, 15338, 25452, 30473, 44977, 42953, 17820, 34483, 10964, 47584, 4579, 15883, 44372, 11590, 18323, 43645, 16126";
    const int vals[] = {3458, 35547, 32826, 49881, 19705, 5588, 26629, 15554, 23371, 14798, 16389, 32219, 1597, 4237, 7371, 35113, 6337, 30421, 27418, 45975, 10175, 25688, 35184, 24650, 18118, 4410, 3314, 37347, 7753, 701, 34731, 1458, 47878, 12676, 14701, 19977, 9002, 25315, 49158, 31194, 19565, 34928, 8525, 28942, 34585, 1060, 26717, 5763, 36309, 32569, 16920, 5385, 1659, 2962, 19056, 45954, 39281, 43274, 36897, 24266, 26694, 25324, 32292, 8665, 41941, 16524, 14944, 16252, 15404, 37787, 45642, 44001, 39683, 37467, 16568, 12273, 39375, 38445, 46893, 30967, 27129, 34115, 26165, 16020, 34171, 1883, 36962, 33783, 27005, 16101, 13532, 38846, 43845, 46157, 38657, 11253, 21659, 49276, 44295, 7730, 49015, 46028, 39694, 44884, 242, 15533, 33432, 23564, 30127, 13979, 28309, 23487, 46252, 39920, 18475, 7780, 19479, 49252, 37224, 26976, 12371, 45144, 16376, 3484, 12075, 24606, 17309, 48700, 33971, 23256, 1104, 4577, 21782, 47305, 33544, 27605, 2446, 27744, 19090, 8321, 27032, 31698, 40057, 36490, 48643, 33357, 13949, 21742, 36909, 20985, 5061, 5401, 46390, 47173, 48065, 49346, 35196, 23988, 18280, 9172, 37733, 3369, 23680, 11271, 27599, 27256, 13211, 12636, 34565, 24350, 9747, 1922, 41901, 49479, 21853, 33281, 48952, 19712, 49903, 43663, 6407, 10709, 21651, 15334, 15338, 25452, 30473, 44977, 42953, 17820, 34483, 10964, 47584, 4579, 15883, 44372, 11590, 18323, 43645, 16126};
    std::vector<std::string> expectedStrs;
    expectedStrs.reserve(count(vals));
    std::transform(cbegin(vals), cend(vals), std::back_inserter(expectedStrs), [](int a) {return ::DFG_ROOT_NS::DFG_SUB_NS_NAME(str)::toStrC(a);});

    // text, expected cell texts.
    typedef std::tuple<std::string, std::vector<std::string>> ExpectedResultsT;
    const ExpectedResultsT arrCellExpected[] =
    {
        ExpectedResultsT(std::move(str), std::move(expectedStrs)),
    };

    for(size_t i = 0; i<count(arrCellExpected); ++i)
    {
        const auto& expected = arrCellExpected[i];
        ::DFG_MODULE_NS(io)::BasicImStream strm(std::get<0>(expected).data(), std::get<0>(expected).size());

        DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::CellData<char> cellDataHandler(',', '"', '\n');
        auto reader = DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cellDataHandler);

        std::vector<std::string> vecStrings;
        auto cellHandler = [&](const size_t nCol, decltype(cellDataHandler)& cdh)
                            {
                                EXPECT_EQ(nCol, vecStrings.size());
                                vecStrings.push_back(std::string(cdh.getBuffer().begin(), cdh.getBuffer().end()));
                            };


        DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::readRow(reader, cellHandler);

        EXPECT_EQ(std::get<1>(expected), vecStrings);
    }

}

TEST(DfgIo, DelimitedTextReader_read)
{
    using namespace DFG_ROOT_NS;
    using DelimitedTextReader = ::DFG_MODULE_NS(io)::DelimitedTextReader;

    /*
    8925, 25460, 46586
    14510, 26690, 41354
    17189, 42528, 49812
    */
    auto istrm = DFG_SUB_NS_NAME(io)::createInputStreamBinaryFile("testfiles/matrix_3x3.txt");
    //auto istrm = DFG_SUB_NS_NAME(io)::createInputStreamBinaryFile("testfiles/matrix_200x200.csv");
    //auto istrm = DFG_SUB_NS_NAME(io)::createInputStreamBinaryFile("testfiles/matrix_1000x1000.txt");

    EXPECT_EQ(istrm.good(), true);

    const int arrExpectedValues[] = {8925, 25460, 46586, 14510, 26690, 41354, 17189, 42528, 49812};
    const int arrExpectedRows[] = {0, 0, 0, 1, 1, 1, 2, 2, 2};
    const int arrExpectedCols[] = {0, 1, 2, 0, 1, 2, 0, 1, 2};

    const std::vector<int> contExpectedValues(cbegin(arrExpectedValues), cend(arrExpectedValues));
    const std::vector<size_t> contExpectedRows(cbegin(arrExpectedRows), cend(arrExpectedRows));
    const std::vector<size_t> contExpectedCols(cbegin(arrExpectedCols), cend(arrExpectedCols));
    std::vector<int> readValues;
    std::vector<size_t> readRows;
    std::vector<size_t> readCols;

    int sum = 0;

    DelimitedTextReader::CellData<char> cellDataHandler(',', '"', '\n');
    auto reader = DelimitedTextReader::createReader(istrm, cellDataHandler);

    auto cellHandler = [&](const size_t nRow, const size_t nCol, const char* p, const size_t nCount)
                            {
                                const auto val = ::DFG_MODULE_NS(str)::strTo<int>(StringViewC(p, nCount));
                                readRows.push_back(nRow);
                                readCols.push_back(nCol);
                                readValues.push_back(val);
                                if (nRow == nCol)
                                    sum += val;
                            };
    auto cellHandler2 = [&](const size_t nRow, const size_t nCol, const decltype(cellDataHandler)& rCd) { cellHandler(nRow, nCol, rCd.getBuffer().data(), rCd.getBuffer().size()); };

    // Testing reader overload
    {
        DelimitedTextReader::read(reader, cellHandler2);
        EXPECT_EQ(contExpectedValues, readValues);
        EXPECT_EQ(contExpectedRows, readRows);
        EXPECT_EQ(contExpectedCols, readCols);
    }

    // Testing convenience overload with individual format items
    {
        readValues.clear();
        readRows.clear();
        readCols.clear();
        istrm.seekg(decltype(istrm)::SeekOriginBegin, 0);
        DelimitedTextReader::read<char>(istrm, ',', '"', '\n', cellHandler);
        EXPECT_EQ(contExpectedValues, readValues);
        EXPECT_EQ(contExpectedRows, readRows);
        EXPECT_EQ(contExpectedCols, readCols);
    }

    // Testing convenience overload with format object
    {
        readValues.clear();
        readRows.clear();
        readCols.clear();
        istrm.seekg(decltype(istrm)::SeekOriginBegin, 0);
        const CsvFormatDefinition csvFormat(',', '"', ::DFG_MODULE_NS(io)::EndOfLineTypeN, ::DFG_MODULE_NS(io)::encodingUnknown);
        DelimitedTextReader::read<char>(istrm, csvFormat, cellHandler);
        EXPECT_EQ(contExpectedValues, readValues);
        EXPECT_EQ(contExpectedRows, readRows);
        EXPECT_EQ(contExpectedCols, readCols);
    }

    // Testing handling of rfSkipLeadingWhitespaces-flag
    {
        const char szPath[] = "testfiles/matrix_10x10_1to100_eol_n.txt"; // This file has leading whitespaces
        const auto bytes = ::DFG_MODULE_NS(io)::fileToMemory_readOnly(szPath);
        const auto byteSpan = bytes.asSpan<char>();

        // With default reader
        {
            ::DFG_MODULE_NS(io)::BasicImStream strm(byteSpan);
            DelimitedTextReader::read<char>(strm, '\t', '"', '\n', [](const size_t nRow, const size_t nCol, const char* const pData, const size_t nSize)
            {
                const char* pszLead = (nRow < 1) ? " " : "";
                DFGTEST_EXPECT_LEFT(pszLead + ::DFG_MODULE_NS(str)::toStrC(nRow * 10 + nCol), StringViewC(pData, nSize));
            });
        }
        // With basic reader
        {
            ::DFG_MODULE_NS(io)::BasicImStream strm(byteSpan);
            DelimitedTextReader::read<char>(strm, '\t', DelimitedTextReader::s_nMetaCharNone, '\n', [](const size_t nRow, const size_t nCol, const char* const pData, const size_t nSize)
            {
                const char* pszLead = (nRow < 1) ? " " : "";
                DFGTEST_EXPECT_LEFT(pszLead + ::DFG_MODULE_NS(str)::toStrC(nRow * 10 + nCol), StringViewC(pData, nSize));
            });
        }
    }

    //std::cout << "sum: " << sum << std::endl;
}


TEST(DfgIo, DelimitedTextReader_readMatrix10x10_1to100)
{
    using namespace DFG_ROOT_NS;
    const char eol[3] = {'\n', '\r', '\n'};
    const char* pszFileNames[] = {	"testfiles/matrix_10x10_1to100_eol_n.txt",
                                    "testfiles/matrix_10x10_1to100_eol_r.txt",
                                    "testfiles/matrix_10x10_1to100_eol_rn.txt"};


    for(size_t i = 0; i<count(eol); ++i)
    {
        std::vector<size_t> contExpected(100);
        DFG_SUB_NS_NAME(alg)::generateAdjacent(contExpected, 0, 1); // generate values 0,1...,98,99
        std::array<uint32, 10> rowSums = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        std::array<uint32, 10> colSums = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::CellData<char> cellDataHandler('\t', -1, eol[i]);
        DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::readFromFile(	pszFileNames[i],
                                                    cellDataHandler,
                                                    [&](const size_t nRow, const size_t nCol, const decltype(cellDataHandler)& rCd)
                                                    {
                                                        const auto idx = ::DFG_ROOT_NS::pairIndexToLinear(nRow, nCol, size_t(10));
                                                        const auto val = std::stoul(rCd.getBuffer());
                                                        EXPECT_EQ(contExpected[idx], val);
                                                        rowSums[nRow] += val;
                                                        colSums[nCol] += val;
                                                    }
                                                 );

        for(size_t r = 0; r<10; ++r)
        {
            EXPECT_EQ(rowSums[r], 45 + 10 * 10 * r);
            EXPECT_EQ(colSums[r], 450 + 10 * r);
        }


    }

}

TEST(DfgIo, DelimitedTextReader_csvReaderTestIntMatrixes)
{
#if 1
    using namespace DFG_ROOT_NS;

    const size_t matSizes[] = {3, 200, 1000};

#ifdef _DEBUG
    const bool matActive[] = {true, false, false};
#else
    const bool matActive[] = {true, true, true};
#endif

    const size_t expectedDiagSums[] = {85427, 5109601, 25037701};
    const size_t expectedBackDiagSums[] = {90465, 5023154, 25140301};

    typedef DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) Timer;

    for(int i = 0; i < static_cast<int>(DFG_COUNTOF(matSizes)); ++i)
    {
        if (!matActive[i])
            continue;
        const auto matSize = matSizes[i];
        std::ostringstream fnStrm;
        fnStrm << "testfiles/matrix_" << matSize << "x" << matSize << ".txt";
        //fnStrm << "testfiles/matrix_" << matSize << "x" << matSize << "_UTF8.txt";

        // Note: With release-version reading to memory dropped runtime ~6.5 s -> ~1.8 s.
#if 1
        Timer timer;
        auto vec = DFG_SUB_NS_NAME(io)::fileToByteContainer<std::vector<char>>(	fnStrm.str().c_str(),
                                                                        512);
        std::cout << "Matrix size " << matSize << " fileToByteContainer time: " << timer.elapsedWallSeconds() << '\n';
        EXPECT_EQ(false, vec.empty());
        // Runtime difference between std::istrstream ja BasicImStream was about 2.3 s vs. 0.75 s
        //std::istrstream istrm(vec.data(), vec.size());
        DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(BasicImStream) istrm(vec.data(), vec.size());
        // imbue didn't seem to work on istrstream (based on one test).
        //istrm.imbue(std::locale(istrm.getloc(), new std::codecvt_utf8<char, 0xffff, std::consume_header>));
#else
        // createInputStreamBinaryFile-function does something unexpected:
        // With the upper line runtime is about 10 s, while with the lower line takes about 2.5 s.
        auto istrm = DFG_SUB_NS_NAME(io)::createInputStreamBinaryFile(fnStrm.str());
        //std::ifstream istrm(fnStrm.str(), std::ios::binary | std::ios::in);
        //istrm.imbue(std::locale(istrm.getloc(), new std::codecvt_utf8<char, 0xffff, std::consume_header>));
#endif

        EXPECT_EQ(true, istrm.good());
        size_t diag = 0;
        size_t backDiag = 0;

        DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::CellData<char> cellDataHandler(',', '"', '\n');
        auto reader = DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::createReader(istrm, cellDataHandler);
        DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::read(reader, [&](const size_t nRow, const size_t nCol, const decltype(cellDataHandler)& rCell)
        {
            if (nRow == nCol) // Diagonal
                diag += ::DFG_MODULE_NS(str)::strTo<size_t>(rCell.getBuffer().toView());
            if (nRow + nCol == matSize - 1) // Back diagonal
                backDiag += ::DFG_MODULE_NS(str)::strTo<size_t>(rCell.getBuffer().toView());
        });

        if (expectedDiagSums[i] != 0)
            EXPECT_EQ(expectedDiagSums[i], diag);

        if (expectedBackDiagSums[i] != 0)
            EXPECT_EQ(expectedBackDiagSums[i], backDiag);
    }
#endif
}


TEST(DfgIo, DelimitedTextReader_csvReaderTestTextCells)
{
    using namespace DFG_ROOT_NS;

#ifndef _DEBUG // Takes too much time in debug-build.
    const auto sFile = "testfiles/testfile_200x200.txt";

    auto vec = DFG_SUB_NS_NAME(io)::fileToByteContainer<std::vector<char>>(sFile,
                                                                    512);
    EXPECT_EQ(false, vec.empty());
    ::DFG_MODULE_NS(io)::BasicImStream istrm(vec.data(), vec.size());

#ifdef _MSC_VER
    auto ostrm = DFG_SUB_NS_NAME(io)::createOutputStreamBinaryFile("testfiles/generated/read_testfile_200x200.txt");
#else
    std::ofstream ostrm("testfiles/generated/read_testfile_200x200.txt");
#endif

    EXPECT_EQ(ostrm.good(), true);

    // Note: won't work if enclosed item is " since the file is not well formed.
    DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::CellData<char> cellDataHandler('\t', -1, '\n');
    cellDataHandler.getFormatDefInfo().setFlag(DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::rfSkipLeadingWhitespaces, false);
    auto reader = DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::createReader(istrm, cellDataHandler);

    DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::read(reader,
                                        [&](const size_t /*nRow*/, const size_t /*nCol*/, const decltype(cellDataHandler)& cellData)
                                        {
                                            ostrm << cellData.getBuffer();
                                        });

    ostrm.close(); // To make sure that all data is written to output file.

    const auto sExpected = "testfiles/testfile_200x200_CellTexts.txt";
    const auto sGot = "testfiles/generated/read_testfile_200x200.txt";

    auto vecExpected = DFG_SUB_NS_NAME(io)::fileToByteContainer<std::vector<char>>(sExpected, 512);

    auto vecGot = DFG_SUB_NS_NAME(io)::fileToByteContainer<std::vector<char>>(sGot, 512);

    EXPECT_EQ(false, vecExpected.empty());
    EXPECT_EQ(false, vecGot.empty());

    EXPECT_EQ(vecExpected.size(), vecGot.size());
    if (vecExpected.size() == vecGot.size())
    {
        for(size_t i = 0; i<Min(vecExpected.size(), vecGot.size()); ++i)
        {
            EXPECT_EQ(vecExpected[i], vecGot[i]);
        }
    }
#endif
}

TEST(DfgIo, DelimitedTextReader_csvReaderTestRandomTextRead)
{
    using namespace DFG_ROOT_NS;

    const char szFileName[] = "testfiles/generated/randomCsvData.csv";
    const char szFileNameExpected[] = "testfiles/generated/randomCsvData.csv_expected.txt";

    std::vector<char> readCells;

    createTestFileCsvRandomData(szFileName);

    DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::CellData<char> cellData(',', '"', '\n');
    cellData.getFormatDefInfo().setFlag(DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::rfSkipLeadingWhitespaces, false);
    DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::readFromFile(szFileName, cellData, [&](uint64 r, uint64 c, const decltype(cellData)& rCd)
                                                {
                                                    auto s = std::to_string(r);
                                                    readCells.insert(readCells.end(), s.begin(), s.end());
                                                    s = std::to_string(c);
                                                    readCells.insert(readCells.end(), s.begin(), s.end());
                                                    readCells.insert(readCells.end(),
                                                                     rCd.getBuffer().begin(),
                                                                     rCd.getBuffer().end());
                                                });

    EXPECT_EQ(false, readCells.empty());

    {
        std::ofstream ostrm("testfiles/generated/randomCsvData.csv_read.txt", std::ios::binary);
        ostrm.write(readCells.data(), readCells.size());
        EXPECT_EQ(true, ostrm.is_open());
    }

    auto vecExpected = DFG_SUB_NS_NAME(io)::fileToVector(szFileNameExpected);

    EXPECT_EQ(false, vecExpected.empty());

    EXPECT_EQ(vecExpected.size(), readCells.size());
    if (vecExpected.size() == readCells.size())
    {
        for(size_t i = 0; i<Min(vecExpected.size(), readCells.size()); ++i)
        {
            EXPECT_EQ(vecExpected[i], readCells[i]);
        }
    }
}

TEST(DfgIo, DelimitedTextReader_tokenizeLine)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(io);

    {
        std::string sA = "\"a\",, , \"b\" , \"c,d\", \"e \", f+62533b tr454f, \"\"\"a,b\"\"\",";
        std::string expectedTokens[] = { "a", "", "", "b", "c,d", "e ", "f+62533b tr454f", "\"a,b\"", "" };
        std::vector<std::string> vA;
        DFG_CLASS_NAME(DelimitedTextReader)::tokenizeLineToContainer<char>(sA, ',', '"', vA);
        EXPECT_TRUE(std::equal(vA.cbegin(), vA.cend(), expectedTokens));

        // Test tokenizing with szRange
        {
            vA.clear();
            auto psz = sA.c_str();
            auto range = makeSzRange(psz);
            DFG_CLASS_NAME(DelimitedTextReader)::tokenizeLineToContainer<char>(range, ',', '"', vA);
            EXPECT_TRUE(std::equal(vA.cbegin(), vA.cend(), expectedTokens));
        }
    }
        
    {
        std::string sA = "a\t\tb\t\t\t";
        std::string expectedTokens[] = { "a", "", "b", "", "", "" };
        std::vector<std::string> vA;
        DFG_CLASS_NAME(DelimitedTextReader)::tokenizeLineToContainer<char>(sA, '\t', '\0', vA);
        EXPECT_TRUE(std::equal(vA.begin(), vA.end(), expectedTokens));
    }

    {
        dfg::StringA sA = "a\t\tb\t\t\t";
        dfg::StringA expectedTokens[] = { "a", "", "b", "", "", "" };
        std::vector<dfg::StringA> vA;
        DFG_CLASS_NAME(DelimitedTextReader)::tokenizeLineToContainer<char>(sA, '\t', '\0', vA);
        EXPECT_TRUE(std::equal(vA.begin(), vA.end(), expectedTokens));
    }


    {
        std::wstring sW = L"\"a\", \"b\" , \"c,d\", \"e \"";
        std::wstring expectedTokens[] = { L"a", L"b", L"c,d", L"e " };
        std::vector<std::wstring> vW;
        DFG_CLASS_NAME(DelimitedTextReader)::tokenizeLineToContainer<wchar_t>(sW, wchar_t(','), wchar_t('"'), vW);
        EXPECT_TRUE(std::equal(vW.begin(), vW.end(), expectedTokens));
    }

    {
        const wchar_t s[] = L"\"a\", \"b\" , \"c,d\", \"e \", \"\"\"\", 1+2+3+";
        std::wstring expectedTokens[] = { L"a", L"b", L"c,d", L"e ", L"\"", L"1+2+3+" };
        std::vector<std::wstring> v;
        DFG_CLASS_NAME(DelimitedTextReader)::tokenizeLineToContainer<wchar_t>(makeRange(s, s + DFG_COUNTOF_CSL(s)), wchar_t(','), wchar_t('"'), v);
        EXPECT_TRUE(std::equal(v.begin(), v.end(), expectedTokens));
    }
#if 0
    // Use TokenizeLine to stream
    {
        std::istringstream istrm("ab,c,\"d\"");
        std::string expectedTokens[] = { "ab", "c", "d" };
        std::vector<std::string> vecTokens;
        DFG_CLASS_NAME(DelimitedTextReader)::tokenizeLineToContainer(
            makeRange(std::istream_iterator<char>(istrm), std::istream_iterator<char>()),
            ',',
            '"',
            vecTokens);
        EXPECT_TRUE(std::equal(vecTokens.begin(), vecTokens.end(), expectedTokens));
    }
#endif

}

namespace
{
    template <class Strm_T, class ReaderCreator_T>
    void DelimitedTextReaderBasicTests(ReaderCreator_T readerCreator)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);
        using namespace DFG_MODULE_NS(cont);

        typedef std::pair<std::string, std::vector<std::string>> InputExpected;

        const InputExpected inputsAndExpected[] =
            {   InputExpected("",                  std::vector<std::string>()),
                InputExpected(" ",                 makeVector<std::string>(" ")),
                InputExpected(",",                 makeVector<std::string>("", "")),
                InputExpected("\n",                makeVector<std::string>("")),
                InputExpected("\r,",               makeVector<std::string>("\r", "")),
                InputExpected("\r\n",              makeVector<std::string>("")),
                InputExpected("abc",               makeVector<std::string>("abc")),
                InputExpected(",\n,",              makeVector<std::string>("", "", "", "")),
                InputExpected("1\n2",              makeVector<std::string>("1", "2")),
                InputExpected("1, 2",              makeVector<std::string>("1", " 2")),
                InputExpected("1,2\n3, 4 ",        makeVector<std::string>("1", "2", "3", " 4 ")),
                InputExpected("1,2\r\n3,4",        makeVector<std::string>("1", "2", "3", "4")),
                InputExpected("\"1\",\"2,3\",4",   makeVector<std::string>("\"1\"", "\"2", "3\"", "4")),
                InputExpected(",,\"\",\n",         makeVector<std::string>("", "", "\"\"", ""))
            };

        for (size_t i = 0; i < DFG_COUNTOF(inputsAndExpected); ++i)
        {
            Strm_T strm(inputsAndExpected[i].first.c_str(), DFG_MODULE_NS(str)::strLen(inputsAndExpected[i].first));

            std::vector<std::string> readCells;

            auto reader = readerCreator(strm);

            typedef decltype(reader) ReaderType;
            typedef typename ReaderType::CellBuffer BufferType;
            DFG_CLASS_NAME(DelimitedTextReader)::read(reader, [&](const size_t /*r*/, const size_t /*c*/, const BufferType& cellData)
            {
                readCells.push_back(std::string(cellData.getBuffer().cbegin(), cellData.getBuffer().cend()));
            });

            EXPECT_EQ(inputsAndExpected[i].second, readCells);
        }
    }

    template <class Strm_T, class BufferType_T, class AppenderType_T, class FormatDef_T>
    void DelimitedTextReaderBasicTests(const FormatDef_T ft)
    {
        using namespace DFG_MODULE_NS(io);
        DelimitedTextReader::CellData<char, char, BufferType_T, AppenderType_T, FormatDef_T> cd(ft);
        DelimitedTextReaderBasicTests<Strm_T>([&](Strm_T& strm)
        {
            return DelimitedTextReader::createReader_basic(strm, cd);
        });
    }

    typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader) DelimReader;
    typedef DelimReader::FormatDefinitionSingleCharsCompileTime<DelimReader::s_nMetaCharNone, '\n', ','> CompileTimeFormatDef;

    template <class Strm_T>
    void StringViewBufferTest() {}

    template <>
    void StringViewBufferTest<DFG_MODULE_NS(io)::BasicImStream>() // This test requires BasicImStream.
    {
        typedef DelimReader::StringViewCBuffer BufferType;
        typedef DelimReader::CharAppenderStringViewCBuffer AppenderType;
        typedef DelimReader::StringViewCBufferWithEnclosedCellSupport BufferType2;
        typedef DelimReader::CharAppenderStringViewCBufferWithEnclosedCellSupport AppenderType2;
        DelimitedTextReaderBasicTests<DFG_MODULE_NS(io)::BasicImStream, BufferType, AppenderType>(CompileTimeFormatDef());
        DelimitedTextReaderBasicTests<DFG_MODULE_NS(io)::BasicImStream, BufferType2, AppenderType2>(CompileTimeFormatDef());
    }

    template <class Strm_T>
    void DelimitedTextReader_basicReaderImpl()
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);
        using namespace DFG_MODULE_NS(cont);

        // Basic reader with default buffer.
        {
            typedef DelimReader::CharBuffer<char> BufferType;
            typedef DelimReader::CharAppenderDefault<BufferType, char> AppenderType;
            DelimitedTextReaderBasicTests<Strm_T, BufferType, AppenderType>(DelimReader::FormatDefinitionSingleChars(DelimReader::s_nMetaCharNone, '\n', ','));
        }

        // Basic reader with string view buffer and compile time format def.
        {
            StringViewBufferTest<Strm_T>();
        }

        // Default reader
        {
            DelimReader::CellData<char> cellData(',', DelimReader::s_nMetaCharNone, '\n');
            cellData.getFormatDefInfo().setFlag(DelimReader::rfSkipLeadingWhitespaces, false);
            DelimitedTextReaderBasicTests<Strm_T>([&](Strm_T& strm) { return DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cellData); });
        }

        // Default reader with compile time format definition.
        {
            using CellData = DelimReader::CellData<char,
                char,
                DelimReader::CharBuffer<char>,
                DelimReader::CharAppenderDefault<DelimReader::CharBuffer<char>, char>,
                CompileTimeFormatDef>;
            CellData cellData{ CompileTimeFormatDef() };
            cellData.getFormatDefInfo().setFlag(DelimReader::rfSkipLeadingWhitespaces, false);
            DelimitedTextReaderBasicTests<Strm_T>([&](Strm_T& strm) { return DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cellData); });
        }
    }
}

TEST(DfgIo, DelimitedTextReader_basicReader)
{
    DelimitedTextReader_basicReaderImpl<DFG_MODULE_NS(io)::BasicImStream>();
    DelimitedTextReader_basicReaderImpl<std::istrstream>();
}

namespace
{
    template <class ReadImpl_T>
    static void metaCharHandlingImpl(ReadImpl_T readImpl)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);
        using namespace DFG_MODULE_NS(cont);

        const auto metaNone = DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone;
        const auto metaAuto = DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect;

        // Test negative chars
        {
            std::vector<char> bytes;
            for (char i = std::numeric_limits<char>::min(); i <= 0; ++i)
                bytes.push_back(i);

            DFG_CLASS_NAME(BasicImStream) istrm(bytes.data(), bytes.size());
            
            DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Table)<std::vector<char>> readBytes;

            readImpl(istrm, metaNone, metaNone, '\n', [&](size_t r, size_t c, const char* p, const size_t count)
            {
                readBytes.setElement(r, c, std::vector<char>(p, p + count));
            });

            ASSERT_EQ(1, readBytes.getCellCount());
            EXPECT_EQ(bytes, readBytes(0, 0));
        }

        // Test having numerical value of meta chars as control chars
        {
            const auto charToInternal = [](const char c) { return DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::bufferCharToInternal(c); };

            DFGTEST_STATIC(metaNone == -1 && metaAuto == -2); // "This test is meaningful only if metachar is with in range of char.");
            const char input[] = { 'a', metaNone, 'b', metaAuto, 'c' };
            DFG_CLASS_NAME(BasicImStream) istrm(input, DFG_COUNTOF(input));

            DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Table)<std::string> readBytes;

            readImpl(istrm, charToInternal(metaNone), metaNone, charToInternal(metaAuto), [&](size_t r, size_t c, const char* p, const size_t count)
            {
                readBytes.setElement(r, c, std::string(p, p + count));
            });

            ASSERT_EQ(3, readBytes.getCellCount());
            EXPECT_EQ("a", readBytes(0, 0));
            EXPECT_EQ("b", readBytes(0, 1));
            EXPECT_EQ("c", readBytes(1, 0));
        }
    }
}

TEST(DfgIo, DelimitedTextReader_metaCharHandling)
{
    // Simple read()-overload.
    metaCharHandlingImpl([=](DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)& istrm, const int sep, const int enc, const int eol, std::function<void(size_t, size_t, const char*, size_t)> func)
    {
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::read<char>(istrm, sep, enc, eol, func);
    });

    // default_reader, runtime formatdef and default buffer
    metaCharHandlingImpl([=](DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)& istrm, const int sep, const int enc, const int eol, std::function<void(size_t, size_t, const char*, size_t)> func)
    {
        using namespace DFG_MODULE_NS(io);
        typedef DFG_CLASS_NAME(DelimitedTextReader) Dtr;
        typedef Dtr::CellData<char, char> Cdt;
        Cdt cdt(sep, enc, eol);
        auto reader = Dtr::createReader(istrm, cdt);
        Dtr::read(reader, [&](const size_t r, const size_t c, const Cdt& cellData)
        {
            func(r, c, cellData.getBuffer().data(), cellData.getBuffer().size());
        });
    });

    // default_reader, runtime formatdef and StringViewBuffer
    metaCharHandlingImpl([=](DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)& istrm, const int sep, const int enc, const int eol, std::function<void(size_t, size_t, const char*, size_t)> func)
    {
        using namespace DFG_MODULE_NS(io);
        typedef DFG_CLASS_NAME(DelimitedTextReader) Dtr;
        typedef Dtr::CellData<char, char, Dtr::StringViewCBuffer, Dtr::CharAppenderStringViewCBuffer> Cdt;
        Cdt cdt(sep, enc, eol);
        auto reader = Dtr::createReader(istrm, cdt);
        Dtr::read(reader, [&](const size_t r, const size_t c, const Cdt& cellData)
        {
            func(r, c, cellData.getBuffer().data(), cellData.getBuffer().size());
        });
    });

    // basic_reader, runtime formatdef and default buffer
    metaCharHandlingImpl([=](DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)& istrm, const int sep, const int enc, const int eol, std::function<void(size_t, size_t, const char*, size_t)> func)
    {
        using namespace DFG_MODULE_NS(io);
        typedef DFG_CLASS_NAME(DelimitedTextReader) Dtr;
        typedef Dtr::CellData<char, char> Cdt;
        Cdt cdt(sep, enc, eol);
        auto reader = Dtr::createReader_basic(istrm, cdt);
        Dtr::read(reader, [&](const size_t r, const size_t c, const Cdt& cellData)
        {
            func(r, c, cellData.getBuffer().data(), cellData.getBuffer().size());
        });
    });

    // basic_reader runtime formatdef and StringViewBuffer
    metaCharHandlingImpl([=](DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream)& istrm, const int sep, const int enc, const int eol, std::function<void(size_t, size_t, const char*, size_t)> func)
    {
        using namespace DFG_MODULE_NS(io);
        typedef DFG_CLASS_NAME(DelimitedTextReader) Dtr;
        typedef Dtr::CellData<char, char, Dtr::StringViewCBuffer, Dtr::CharAppenderStringViewCBuffer> Cdt;
        Cdt cdt(sep, enc, eol);
        auto reader = Dtr::createReader_basic(istrm, cdt);
        Dtr::read(reader, [&](const size_t r, const size_t c, const Cdt& cellData)
        {
            func(r, c, cellData.getBuffer().data(), cellData.getBuffer().size());
        });
    });

    // TODO: basic_reader compiletime formatdef
    // TODO: default reader compiletime formatdef
}

namespace
{
    typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader) DelimReader;

    template <class T0, class T1>
    auto createReader(std::true_type, T0& strm, T1& cd) -> decltype(DelimReader::createReader(strm, cd)) { return DelimReader::createReader(strm, cd); }

    template <class T0, class T1>
    auto createReader(std::false_type, T0& strm, T1& cd) -> decltype(DelimReader::createReader_basic(strm, cd)) { return DelimReader::createReader_basic(strm, cd); }

    typedef DelimReader::FormatDefinitionSingleCharsCompileTime<DelimReader::s_nMetaCharNone, '\n', ',', DelimReader::CsvFormatFlagNoRnTranslation> CompileTimeFormatDefNoRn;

    template <class IsNormalReader_T, class Appender_T>
    static void DelimitedTextReader_rnTranslationImpl()
    {
        const CompileTimeFormatDefNoRn formatDef;
        using HandlerT = DelimReader::CellData<char, char, typename Appender_T::BufferType, Appender_T, CompileTimeFormatDefNoRn>;
        HandlerT cd(formatDef);
        const char input[] = "a\r\nb\r \nc\nd";
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strm(input, DFG_COUNTOF_SZ(input));
        auto reader = createReader(IsNormalReader_T(), strm, cd);
        const std::string expectedCells[] = { "a\r", "b\r ", "c", "d" };
        std::vector<std::string> readCells;
        
        DelimReader::read(reader, [&](const size_t /*r*/, const size_t /*c*/, const HandlerT& cd)
        {
            readCells.push_back(std::string(cd.getBuffer().data(), cd.getBuffer().size()));
        });
        ASSERT_EQ(DFG_COUNTOF(expectedCells), readCells.size());
        EXPECT_TRUE(std::equal(readCells.cbegin(), readCells.cend(), expectedCells));
    }
}

TEST(DfgIo, DelimitedTextReader_rnTranslation)
{
    typedef DelimReader::CharAppenderDefault<DelimReader::CharBuffer<char>, char> DefaultAppenderT;
    typedef DelimReader::CharAppenderStringViewCBuffer StringViewCAppenderT;
    DelimitedTextReader_rnTranslationImpl<std::true_type, DefaultAppenderT>();
    DelimitedTextReader_rnTranslationImpl<std::true_type, StringViewCAppenderT>();
    DelimitedTextReader_rnTranslationImpl<std::false_type, DefaultAppenderT>();
    DelimitedTextReader_rnTranslationImpl<std::false_type, StringViewCAppenderT>();
}

TEST(DfgIo, DelimitedTextReader_CharAppenderUtf)
{
    using namespace DFG_ROOT_NS;
    typedef DelimReader::CharAppenderUtf<DelimReader::CharBuffer<char>> CharAppenderUtfT;
    
    DelimReader::FormatDefinitionSingleChars formatDef('"', '\n', '"');
    DelimReader::CellData<char, char, DelimReader::CharBuffer<char>, CharAppenderUtfT> cd(formatDef);
    const char input[] = { static_cast<char>(uint8(199)), ' ', 'a', '\0' };
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strm(input, DFG_COUNTOF_SZ(input));
    
    auto reader = DelimReader::createReader(strm, cd);
    DelimReader::read(reader, [](const size_t, const size_t, const decltype(cd)&) {});

    const auto& buffer = cd.getBuffer();
    ASSERT_EQ(4, buffer.size());
    EXPECT_EQ(-61, buffer[0]);  // 195
    EXPECT_EQ(-121, buffer[1]); // 135
    EXPECT_EQ(' ', buffer[2]);
    EXPECT_EQ('a', buffer[3]);
}

#endif
