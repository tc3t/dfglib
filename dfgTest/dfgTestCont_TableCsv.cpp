#include "stdafx.h"

#if (defined(DFGTEST_BUILD_MODULE_CONT_TABLE_CSV) && DFGTEST_BUILD_MODULE_CONT_TABLE_CSV == 1) || (!defined(DFGTEST_BUILD_MODULE_CONT_TABLE_CSV) && DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/cont.hpp>
#include <dfg/cont/table.hpp>
#include <dfg/cont/arrayWrapper.hpp>
#include <string>
#include <memory>
#include <strstream>
#include <dfg/ptrToContiguousMemory.hpp>
#include <dfg/dfgBase.hpp>
#include <dfg/ReadOnlySzParam.hpp>
#include <dfg/cont/CsvConfig.hpp>
#include <dfg/cont/MapVector.hpp>
#include <dfg/cont/tableCsv.hpp>
#include <dfg/cont/TrivialPair.hpp>
#include <dfg/rand.hpp>
#include <dfg/typeTraits.hpp>
#include <dfg/io/BasicOmcByteStream.hpp>
#include <dfg/io/OmcByteStream.hpp>
#include <dfg/iter/szIterator.hpp>
#include <dfg/cont/contAlg.hpp>
#include <dfg/io/cstdio.hpp>
#include <dfg/time/timerCpu.hpp>
#include <dfg/str.hpp>
#include <dfg/str/stringLiteralCharToValue.hpp>
#include <dfg/cont/IntervalSetSerialization.hpp>

#include <thread>


TEST(dfgCont, TableCsv)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(io);
    using namespace DFG_MODULE_NS(utf);

    std::vector<std::string> paths;
    std::vector<TextEncoding> encodings;
    std::vector<char> separators;
    std::vector<EndOfLineType> eolTypes;
    // TODO: Read these properties from file names once "for-each-file-in-folder"-function is available.
    const TextEncoding contEncodings[] = { encodingUTF8,
                                            encodingUTF16Be,
                                            encodingUTF16Le,
                                            encodingUTF32Be,
                                            encodingUTF32Le };
    const std::string contSeparators[] = { "2C", "09", "3B" }; // ',' '\t', ';'
    const std::string contEols[] = { "n", "rn", "r" };
    const std::array<EndOfLineType, 3> contEolIndexToType = { EndOfLineTypeN, EndOfLineTypeRN, EndOfLineTypeR };
    
    const std::string sPathTemplate = "testfiles/csv_testfiles/csvtest%1%_BOM_sep_%2%_eol_%3%.csv";
    for (size_t iE = 0; iE < count(contEncodings); ++iE)
    {
        for (size_t iS = 0; iS < count(contSeparators); ++iS)
        {
            for (size_t iEol = 0; iEol < count(contEols); ++iEol)
            {
                auto s = DFG_MODULE_NS(str)::replaceSubStrs(sPathTemplate, "%1%", encodingToStrId(contEncodings[iE]));
                DFG_MODULE_NS(str)::replaceSubStrsInplace(s, "%2%", contSeparators[iS]);
                DFG_MODULE_NS(str)::replaceSubStrsInplace(s, "%3%", contEols[iEol]);
                const bool bFileExists = DFG_MODULE_NS(os)::isPathFileAvailable(s.c_str(), DFG_MODULE_NS(os)::FileModeRead);
                if (!bFileExists && iEol != 0)
                    continue; // There are no rn or r -versions of all files so simply skip such.
                EXPECT_TRUE(bFileExists);
                paths.push_back(std::move(s));
                encodings.push_back(contEncodings[iE]);
                eolTypes.push_back(contEolIndexToType[iEol]);
                const auto charValOpt = DFG_MODULE_NS(str)::stringLiteralCharToValue<char>(std::string("\\x") + contSeparators[iS]);
                EXPECT_TRUE(charValOpt.first);
                separators.push_back(charValOpt.second);
            }
        }
    }
    typedef DFG_CLASS_NAME(TableCsv)<char, size_t> Table;
    std::vector<std::unique_ptr<Table>> tables;
    tables.resize(paths.size());

    for (size_t i = 0; i < paths.size(); ++i)
    {
        const auto& s = paths[i];
        tables[i].reset(new Table);
        auto& table = *tables[i];
        // Read from file...
        auto readFormat = table.defaultReadFormat();
        if (eolTypes[i] == EndOfLineTypeR) // \r is not detected automatically so setting it manually.
            readFormat.eolType(EndOfLineTypeR);
        table.readFromFile(s, readFormat);
        // ...check that read properties separator, separator char and encoding, matches...
        {
            EXPECT_EQ(encodings[i], table.m_readFormat.textEncoding()); // TODO: use access function for format info.
            EXPECT_EQ(separators[i], table.m_readFormat.separatorChar()); // TODO: use access function for format info.
            //EXPECT_EQ(eolTypes[i], table.m_readFormat.eolType()); // Commented out for now as table does not store original eol info. TODO: use access function for format info.
        }

        // ...write to memory using the same encoding as in file...
        std::string bytes;
        DFG_CLASS_NAME(OmcByteStream)<std::string> ostrm(&bytes);
        auto writeFormat = table.m_readFormat;
        writeFormat.eolType(eolTypes[i]);
        {
            auto writePolicy = table.createWritePolicy<decltype(ostrm)>(writeFormat);
            table.writeToStream(ostrm, writePolicy);
            // ...read file bytes...
            const auto fileBytes = fileToByteContainer<std::string>(s);
            // ...check that written bytes end with EOL...
            {
                const auto sEol = eolStrFromEndOfLineType(eolTypes[i]);
                std::string sEolEncoded;
                for (const auto& c : sEol)
                    cpToEncoded(static_cast<uint32>(c), std::back_inserter(sEolEncoded), encodings[i]);
                ASSERT_TRUE(bytes.size() >= sEolEncoded.size());
                EXPECT_TRUE(std::equal(bytes.cend() - sEolEncoded.size(), bytes.cend(), sEolEncoded.cbegin()));
            }
            // ...and compare with the original file excluding EOL
            const auto nEolSizeInBytes = eolStrFromEndOfLineType(eolTypes[i]).size() * baseCharacterSize(encodings[i]);
            EXPECT_EQ(fileBytes, std::string(bytes.cbegin(), bytes.cend() - nEolSizeInBytes));
        }

        // Check also that saving without BOM works.
        {
            std::string nonBomBytes;
            DFG_CLASS_NAME(OmcByteStream)<std::string> ostrmNonBom(&nonBomBytes);
            auto csvFormat = table.m_readFormat;
            csvFormat.bomWriting(false);
            csvFormat.eolType(eolTypes[i]);
            auto writePolicy = table.createWritePolicy<decltype(ostrmNonBom)>(csvFormat);
            table.writeToStream(ostrmNonBom, writePolicy);
            // ...and check that bytes match.
            const auto nBomSize = ::DFG_MODULE_NS(utf)::bomSizeInBytes(csvFormat.textEncoding());
            ASSERT_EQ(bytes.size(), nonBomBytes.size() + nBomSize);
            EXPECT_TRUE(std::equal(bytes.cbegin() + nBomSize, bytes.cend(), nonBomBytes.cbegin()));
        }
    }

    // Test that results are the same as with DelimitedTextReader.
    {
        std::wstring sFromFile;
        DFG_CLASS_NAME(IfStreamWithEncoding) istrm(paths.front());
        DFG_CLASS_NAME(DelimitedTextReader)::read<wchar_t>(istrm, ',', '"', '\n', [&](const size_t nRow, const size_t nCol, const wchar_t* const p, const size_t nSize)
        {
            std::wstring sUtfConverted;
            auto inputRange = DFG_ROOT_NS::makeSzRange((*tables.front())(nRow, nCol).c_str());
            DFG_MODULE_NS(utf)::utf8To16Native(inputRange, std::back_inserter(sUtfConverted));
            EXPECT_EQ(std::wstring(p, nSize), sUtfConverted);
        });
    }

    // Verify that all tables are identical.
    for (size_t i = 1; i < tables.size(); ++i)
    {
        EXPECT_TRUE(tables[0]->isContentAndSizesIdenticalWith(*tables[i]));
    }

    // Test BOM-handling. Makes sure that BOM gets handled correctly whether it is autodetected or told explicitly, and that BOM-less files are read correctly if correct encoding is given to reader.
    {
        std::string utf8Bomless;
        utf8Bomless.push_back('a');
        const uint32 nonAsciiChar = 0x20AC; // Euro-sign
        DFG_MODULE_NS(utf)::cpToEncoded(nonAsciiChar, std::back_inserter(utf8Bomless), encodingUTF8);
        utf8Bomless += ",b";
        std::string utf8WithBom = utf8Bomless;
        const auto bomBytes = encodingToBom(encodingUTF8);
        utf8WithBom.insert(utf8WithBom.begin(), bomBytes.cbegin(), bomBytes.cend());

        DFG_CLASS_NAME(TableCsv)<char, uint32> tableWithBomExplictEncoding;
        DFG_CLASS_NAME(TableCsv)<char, uint32> tableWithBomAutoDetectedEncoding;
        DFG_CLASS_NAME(TableCsv)<char, uint32> tableWithoutBom;

        const DFG_CLASS_NAME(CsvFormatDefinition) formatDef(',', '"', EndOfLineTypeN, encodingUTF8);
        tableWithBomExplictEncoding.readFromMemory(utf8WithBom.data(), utf8WithBom.size(), formatDef);
        tableWithBomAutoDetectedEncoding.readFromMemory(utf8WithBom.data(), utf8WithBom.size());
        tableWithoutBom.readFromMemory(utf8Bomless.data(), utf8Bomless.size(), formatDef);

        EXPECT_TRUE(tableWithBomExplictEncoding.isContentAndSizesIdenticalWith(tableWithBomAutoDetectedEncoding));
        EXPECT_TRUE(tableWithBomExplictEncoding.isContentAndSizesIdenticalWith(tableWithoutBom));
        ASSERT_EQ(1, tableWithBomAutoDetectedEncoding.rowCountByMaxRowIndex());
        ASSERT_EQ(2, tableWithBomAutoDetectedEncoding.colCountByMaxColIndex());

        DFG_CLASS_NAME(StringUtf8) sItem00(tableWithBomExplictEncoding(0, 0));
        DFG_CLASS_NAME(StringUtf8) sItem01(tableWithBomExplictEncoding(0, 1));
        ASSERT_EQ(4, sItem00.sizeInRawUnits());
        ASSERT_EQ(1, sItem01.sizeInRawUnits());
        EXPECT_EQ('a', utf8ToFixedChSizeStr<wchar_t>(sItem00.rawStorage()).front());
        EXPECT_EQ(nonAsciiChar, utf8ToFixedChSizeStr<wchar_t>(sItem00.rawStorage())[1]);
        EXPECT_EQ('b', utf8ToFixedChSizeStr<wchar_t>(sItem01.rawStorage()).front());
    }

    // Test unknown encoding handling, i.e. that bytes are read as specified as Latin-1.
    {
        DFG_CLASS_NAME(TableCsv)<char, uint32> table;
        std::array<unsigned char, 255> data;
        DFG_CLASS_NAME(StringUtf8) dataAsUtf8;
        for (size_t i = 0; i < data.size(); ++i)
        {
            unsigned char val = (i + 1 != DFG_U8_CHAR('\n')) ? static_cast<unsigned char>(i + 1) : DFG_U8_CHAR('a'); // Don't write '\n' to get single cell table.
            data[i] = val;
            cpToEncoded(data[i], std::back_inserter(dataAsUtf8.rawStorage()), encodingUTF8);
        }
        EXPECT_EQ(127 + 2*128, dataAsUtf8.sizeInRawUnits());
        const auto metaCharNone = DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone;
        table.readFromMemory(reinterpret_cast<const char*>(data.data()), data.size(), DFG_CLASS_NAME(CsvFormatDefinition)(metaCharNone,
                                                                                                                          metaCharNone,
                                                                                                                          EndOfLineTypeN,
                                                                                                                          encodingUnknown));
        ASSERT_EQ(1, table.rowCountByMaxRowIndex());
        ASSERT_EQ(1, table.colCountByMaxColIndex());
        EXPECT_EQ(dataAsUtf8, table(0,0));
    }

    // Test enclosement behaviour
    {
        const size_t nExpectedCount = 2;
        // Note: EbEnclose is missing because it would have need more work to implement writing something even for non-existent cells.
        const std::array<EnclosementBehaviour, nExpectedCount> enclosementItems = { EbEncloseIfNeeded, EbEncloseIfNonEmpty };
        const std::array<std::string, nExpectedCount> expected = {  DFG_ASCII("a,b,,\"c,d\",\"e\nf\",g\n,,r1c2,,,\n").c_str(),
                                                                    DFG_ASCII("\"a\",\"b\",,\"c,d\",\"e\nf\",\"g\"\n,,\"r1c2\",,,\n").c_str() };
        DFG_CLASS_NAME(TableCsv)<char, uint32> table;
        table.setElement(0, 0, DFG_UTF8("a"));
        table.setElement(0, 1, DFG_UTF8("b"));
        //table.setElement(0, 2, DFG_UTF8(""));
        table.setElement(0, 3, DFG_UTF8("c,d"));
        table.setElement(0, 4, DFG_UTF8("e\nf"));
        table.setElement(0, 5, DFG_UTF8("g"));
        table.setElement(1, 2, DFG_UTF8("r1c2"));
        for (size_t i = 0; i < nExpectedCount; ++i)
        {
            DFG_CLASS_NAME(OmcByteStream)<> ostrm;
            auto writePolicy = table.createWritePolicy<decltype(ostrm)>();
            writePolicy.m_format.enclosementBehaviour(enclosementItems[i]);
            writePolicy.m_format.bomWriting(false);
            table.writeToStream(ostrm, writePolicy);
            ASSERT_EQ(expected[i].size(), ostrm.container().size());
            EXPECT_TRUE(std::equal(expected[i].cbegin(), expected[i].cend(), ostrm.container().cbegin()));
        }

    }

    // Correctness tests for reading UTF8 with enclosing items from memory
    {
        using Table = TableCsv<char, size_t>;
        Table t;
        t.readFromString("12,\"\"\"34\"\n\"56\",\"78\r\n\"\n\"1\"\",\n2\"\"\"\"\",\"\"\"\"\na,\"\"", CsvFormatDefinition(',', '"', dfg::io::EndOfLineTypeN, dfg::io::encodingUTF8));
        EXPECT_EQ(4, t.rowCountByMaxRowIndex());
        EXPECT_EQ(2, t.colCountByMaxColIndex());
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("12", t(0, 0));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("\"34", t(0, 1));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("56", t(1, 0));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("78\r\n", t(1, 1));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("1\",\n2\"\"", t(2, 0));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("\"", t(2, 1));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("a", t(3, 0));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("", t(3, 1));
    }

    // TODO: test auto detection
}

TEST(dfgCont, TableCsv_memStreamTypes)
{
    // Test that writing to different memory streams work (in particular that BasicOmcByteStream works).

    using namespace DFG_ROOT_NS;
    const auto nRowCount = 10;
    const auto nColCount = 5;
    DFG_MODULE_NS(cont)::TableCsv<char, uint32> table;
    for (int c = 0; c < nColCount; ++c)
        for (int r = 0; r < nRowCount; ++r)
            table.setElement(r, c, DFG_UTF8("a"));

    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Vector)<char> VectorT;
    VectorT bytesStd;
    VectorT bytesOmc;
    VectorT bytesBasicOmc;

    // std::ostrstream
    {
        std::ostrstream ostrStream;
        table.writeToStream(ostrStream);
        const auto psz = ostrStream.str();
        bytesStd.assign(psz, psz + static_cast<std::streamoff>(ostrStream.tellp()));
        ostrStream.freeze(false);
    }

    // OmcByteStream
    {
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(OmcByteStream)<> omcStrm;
        table.writeToStream(omcStrm);
        bytesOmc = omcStrm.releaseData();
    }

    // BasicOmcByteStream
    {
        DFG_MODULE_NS(io)::BasicOmcByteStream<> ostrm;
        table.writeToStream(ostrm);
        bytesBasicOmc = ostrm.releaseData();
        EXPECT_TRUE(ostrm.m_internalData.empty()); // Make sure that releasing works.
    }

    EXPECT_EQ(bytesStd, bytesOmc);
    EXPECT_EQ(bytesStd, bytesBasicOmc);
}


namespace
{
    class SimpleStringMatcher
    {
    public:
        SimpleStringMatcher(DFG_ROOT_NS::StringViewSzUtf8 sv)
        {
            m_s = sv.c_str();
        }

        bool isMatch(const int nInputRow, const DFG_ROOT_NS::StringViewUtf8& sv)
        {
            DFG_UNUSED(nInputRow);
            std::string s(sv.dataRaw(), sv.length());
            return s.find(m_s.rawStorage()) != std::string::npos;
        }

        template <class Char_T, class Index_T>
        bool isMatchImpl(const int nInputRow, const typename DFG_MODULE_NS(cont)::TableCsv<Char_T, Index_T>::RowContentFilterBuffer& rowBuffer)
        {
            for (const auto& item : rowBuffer)
            {
                if (isMatch(nInputRow, item.second(rowBuffer)))
                    return true;
            }
            return false;
        }

        bool isMatch(const int nInputRow, const DFG_MODULE_NS(cont)::TableCsv<char, DFG_ROOT_NS::uint32>::RowContentFilterBuffer& rowBuffer)
        {
            return isMatchImpl<char, DFG_ROOT_NS::uint32>(nInputRow, rowBuffer);
        }

        bool isMatch(const int nInputRow, const DFG_MODULE_NS(cont)::TableCsv<char, int>::RowContentFilterBuffer& rowBuffer)
        {
            return isMatchImpl<char, int>(nInputRow, rowBuffer);
        }

        DFG_ROOT_NS::StringUtf8 m_s;
    };

}

namespace
{
    class ExceptionReader : public ::DFG_MODULE_NS(cont)::TableCsv<char, ::DFG_ROOT_NS::uint32>::DefaultCellHandler
    {
    public:
        using BaseClass = ::DFG_MODULE_NS(cont)::TableCsv<char, ::DFG_ROOT_NS::uint32>::DefaultCellHandler;
        using BaseClass::BaseClass;
        
        void operator()(const size_t nRow, const size_t nCol, const char* pData, const size_t nCount)
        {
            BaseClass::operator()(nRow, nCol, pData, nCount);
            if (nRow == 1)
                throw std::logic_error(exceptionMessage().c_str());
        }

        static ::DFG_ROOT_NS::StringViewSzC exceptionMessage()
        {
            return "Row 1 encountered";
        }
    }; // ExceptionReader
} // unnamed namespace

TEST(dfgCont, TableCsv_exceptionHandling)
{
    using namespace DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(cont);
    using TableT = TableCsv<char, uint32>;

    const char szCsv[] = "1,2\n3,4\n5,6\n7,8";
    const char szExceptedErrorMsg[] = "Caught exception: 'Row 1 encountered'"; // Format itself is not part of the interface, but testing whole error message string so that unintentional format change is caught by tests.

    // Single-threaded read
    {
        TableT t;
        t.readFromMemory(szCsv, DFG_COUNTOF_SZ(szCsv), CsvFormatDefinition::fromReadTemplate_commaQuoteEolNUtf8(), ExceptionReader(t));
        const auto errorInfo = t.readFormat().getReadStat<TableCsvReadStat::errorInfo>();
        DFGTEST_EXPECT_LEFT(1, errorInfo.entryCount()); // If this fails, might simply be due to added fields
        const auto errorMsg = errorInfo.value(TableCsvErrorInfoFields::errorMsg);
        DFGTEST_EXPECT_EQ_LITERAL_UTF8(szExceptedErrorMsg, errorMsg);
    }

    // Multithreaded read
    {
        TableCsvReadWriteOptions readOptions = TableCsvReadWriteOptions::fromReadTemplate_commaNoEnclosingEolNUtf8();
        readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadCount>(2);
        readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>(0); // Reducing block size to allow threaded reading even for small files.
        TableT t;
        t.readFromMemory(szCsv, DFG_COUNTOF_SZ(szCsv), readOptions, ExceptionReader(t));
        const auto errorInfo = t.readFormat().getReadStat<TableCsvReadStat::errorInfo>();
        const auto errorMsg0 = errorInfo.value(SzPtrUtf8(format_fmt("threads/thread_0/{}", TableCsvErrorInfoFields::errorMsg.c_str()).c_str()));
        const auto errorMsg1 = errorInfo.value(SzPtrUtf8(format_fmt("threads/thread_1/{}", TableCsvErrorInfoFields::errorMsg.c_str()).c_str()));

        DFGTEST_EXPECT_LEFT(szExceptedErrorMsg, errorMsg0.rawStorage()); 
        DFGTEST_EXPECT_LEFT(szExceptedErrorMsg, errorMsg1.rawStorage());
    }
}

TEST(dfgCont, TableCsv_filterCellHandler)
{
    using namespace DFG_ROOT_NS;

    const char szExample0[] = "00,01\n"
                              "10,11\n"
                              "20,21\n"
                              "30,31\n"
                              "40,41\n"
                              "50,51";

    const char szExample1[] = "00,01\n"
                              "\n"
                              "20,21\n"
                              "\n"
                              "\n"
                              "50,51";

    const char szExample2[] = "00,01,02,03\n"
                              "10,11,12,13\n"
                              "20,21,22,23\n"
                              "30,31,32,33\n"
                              "40,41,42,43\n";

    using IndexT = uint32;
    using TableT = DFG_MODULE_NS(cont)::TableCsv<char, IndexT>;

    // Simple test
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler();
        filterCellHandler.setIncludeRows(::DFG_MODULE_NS(cont)::intervalSetFromString<IndexT>("1;3:4"));
        table.readFromMemory(szExample0, DFG_COUNTOF_SZ(szExample0), table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(3, table.rowCountByMaxRowIndex());
        EXPECT_EQ(2, table.colCountByMaxColIndex());
        EXPECT_STREQ("10", table(0, 0).c_str());
        EXPECT_STREQ("11", table(0, 1).c_str());
        EXPECT_STREQ("30", table(1, 0).c_str());
        EXPECT_STREQ("31", table(1, 1).c_str());
        EXPECT_STREQ("40", table(2, 0).c_str());
        EXPECT_STREQ("41", table(2, 1).c_str());
    }

    // Testing handling of empty lines.
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler();
        filterCellHandler.setIncludeRows(::DFG_MODULE_NS(cont)::intervalSetFromString<IndexT>("1;5"));
        table.readFromMemory(szExample1, DFG_COUNTOF_SZ(szExample1), table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(2, table.rowCountByMaxRowIndex());
        EXPECT_EQ(2, table.colCountByMaxColIndex());
        EXPECT_STREQ("", table(0, 0).c_str());
        EXPECT_EQ(nullptr, table(0, 1).c_str());
        EXPECT_STREQ("50", table(1, 0).c_str());
        EXPECT_STREQ("51", table(1, 1).c_str());
    }

    // Testing handling of empty filter
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler();
        table.readFromMemory(szExample1, DFG_COUNTOF_SZ(szExample1), table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(0, table.rowCountByMaxRowIndex());
        EXPECT_EQ(0, table.colCountByMaxColIndex());
    }

    // Simple file read test
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler();
        filterCellHandler.setIncludeRows(::DFG_MODULE_NS(cont)::intervalSetFromString<IndexT>("1"));
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(1, table.rowCountByMaxRowIndex());
        EXPECT_EQ(3, table.colCountByMaxColIndex());
        EXPECT_STREQ("14510", table(0, 0).c_str());
        EXPECT_STREQ(" 26690", table(0, 1).c_str());
        EXPECT_STREQ(" 41354", table(0, 2).c_str());
    }

    // Row content filter: basic test
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler(SimpleStringMatcher(DFG_UTF8("1")));
        /*
        matrix_3x3.txt
            8925, 25460, 46586
            14510, 26690, 41354
            17189, 42528, 49812
        */
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(2, table.rowCountByMaxRowIndex());
        EXPECT_EQ(3, table.colCountByMaxColIndex());
        EXPECT_STREQ("14510", table(0, 0).c_str());
        EXPECT_STREQ(" 26690", table(0, 1).c_str());
        EXPECT_STREQ(" 41354", table(0, 2).c_str());
        EXPECT_STREQ("17189", table(1, 0).c_str());
        EXPECT_STREQ(" 42528", table(1, 1).c_str());
        EXPECT_STREQ(" 49812", table(1, 2).c_str());
    }

    // Index handling with int-index, mostly to check that this doesn't generate warnings.
    {
        DFG_MODULE_NS(cont)::TableCsv<char, int> table;
        auto filterCellHandler = table.createFilterCellHandler(SimpleStringMatcher(DFG_UTF8("1")));
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(2, table.rowCountByMaxRowIndex());
        EXPECT_EQ(3, table.colCountByMaxColIndex());
    }

    // Row content filter: last row handling
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler(SimpleStringMatcher(DFG_UTF8("49812")));
        /*
        matrix_3x3.txt
            8925, 25460, 46586
            14510, 26690, 41354
            17189, 42528, 49812
        */
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(1, table.rowCountByMaxRowIndex());
        EXPECT_EQ(3, table.colCountByMaxColIndex());
        EXPECT_STREQ("17189", table(0, 0).c_str());
        EXPECT_STREQ(" 42528", table(0, 1).c_str());
        EXPECT_STREQ(" 49812", table(0, 2).c_str());
    }

    // Row content filter: single column filter
    {
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler(SimpleStringMatcher(DFG_UTF8("9")), 1);
        /*
        matrix_3x3.txt
            8925, 25460, 46586
            14510, 26690, 41354
            17189, 42528, 49812
        */
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(1, table.rowCountByMaxRowIndex());
        EXPECT_EQ(3, table.colCountByMaxColIndex());
        EXPECT_STREQ("14510", table(0, 0).c_str());
        EXPECT_STREQ(" 26690", table(0, 1).c_str());
        EXPECT_STREQ(" 41354", table(0, 2).c_str());
    }

    // Column filter
    {
        using namespace DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(cont);
        {
            TableT table;
            auto filterCellHandler = table.createFilterCellHandler();
            filterCellHandler.setIncludeRows(intervalSetFromString<IndexT>("1;3"));
            filterCellHandler.setIncludeColumns(intervalSetFromString<IndexT>(""));
            table.readFromMemory(szExample2, DFG_COUNTOF_SZ(szExample2), table.defaultReadFormat(), filterCellHandler);
            EXPECT_EQ(0, table.cellCountNonEmpty());
        }

        {
            TableT table;
            auto filterCellHandler = table.createFilterCellHandler();
            filterCellHandler.setIncludeRows(intervalSetFromString<IndexT>("1;3"));
            filterCellHandler.setIncludeColumns(intervalSetFromString<IndexT>("3"));
            table.readFromMemory(szExample2, DFG_COUNTOF_SZ(szExample2), table.defaultReadFormat(), filterCellHandler);
            EXPECT_EQ(2, table.rowCountByMaxRowIndex());
            EXPECT_EQ(1, table.colCountByMaxColIndex());
            EXPECT_STREQ("13", table(0, 0).c_str());
            EXPECT_STREQ("33", table(1, 0).c_str());
        }

        {
            TableT table;
            auto filterCellHandler = table.createFilterCellHandler();
            filterCellHandler.setIncludeRows(intervalSetFromString<IndexT>("1;3"));
            filterCellHandler.setIncludeColumns(intervalSetFromString<IndexT>("0:1;3"));
            table.readFromMemory(szExample2, DFG_COUNTOF_SZ(szExample2), table.defaultReadFormat(), filterCellHandler);
            EXPECT_EQ(2, table.rowCountByMaxRowIndex());
            EXPECT_EQ(3, table.colCountByMaxColIndex());
            EXPECT_STREQ("10", table(0, 0).c_str());
            EXPECT_STREQ("11", table(0, 1).c_str());
            EXPECT_STREQ("13", table(0, 2).c_str());
            EXPECT_STREQ("30", table(1, 0).c_str());
            EXPECT_STREQ("31", table(1, 1).c_str());
            EXPECT_STREQ("33", table(1, 2).c_str());
        }
    }

    // Row, column and content filter
    {
        /*
        matrix_3x3.txt
            8925, 25460, 46586
            14510, 26690, 41354
            17189, 42528, 49812
        */
        using namespace DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(cont);
        TableT table;
        auto filterCellHandler = table.createFilterCellHandler(SimpleStringMatcher(DFG_UTF8("8")));
        filterCellHandler.setIncludeRows(intervalSetFromString<IndexT>("0;2"));
        filterCellHandler.setIncludeColumns(intervalSetFromString<IndexT>("1"));
        table.readFromFile("testfiles/matrix_3x3.txt", table.defaultReadFormat(), filterCellHandler);
        EXPECT_EQ(1, table.rowCountByMaxRowIndex());
        EXPECT_EQ(1, table.colCountByMaxColIndex());
        EXPECT_EQ(1, table.cellCountNonEmpty());
        EXPECT_STREQ(" 42528", table(0, 0).c_str());
    }
}

namespace
{
    static void verifyFormatInFile(const ::DFG_ROOT_NS::StringViewSzC svPath,
                                   const char cSep,
                                   const char cEnc,
                                   const ::DFG_MODULE_NS(io)::EndOfLineType eolType,
                                   const ::DFG_MODULE_NS(io)::TextEncoding encoding
                                 )
    {
        using namespace DFG_ROOT_NS;
        const auto format = peekCsvFormatFromFile(svPath);
        EXPECT_EQ(cSep, format.separatorChar());
        DFG_UNUSED(cEnc);
        //EXPECT_EQ(cEnc, format.enclosingChar()); Detection is not supported
        EXPECT_EQ(eolType, format.eolType());
        EXPECT_EQ(encoding, format.textEncoding());
    }
}

TEST(dfgCont, TableCsv_peekCsvFormatFromFile)
{
    using namespace ::DFG_MODULE_NS(io);

    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16BE_BOM_sep_09_eol_n.csv",  '\t',   '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16BE_BOM_sep_2C_eol_n.csv",  ',',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16BE_BOM_sep_3B_eol_n.csv",  ';',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_09_eol_n.csv",  '\t',   '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_09_eol_rn.csv", '\t',   '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_2C_eol_n.csv",  ',',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_2C_eol_rn.csv", ',',    '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_3B_eol_n.csv",  ';',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_3B_eol_rn.csv", ';',    '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32BE_BOM_sep_09_eol_n.csv",  '\t',   '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32BE_BOM_sep_2C_eol_n.csv",  ',',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32BE_BOM_sep_3B_eol_n.csv",  ';',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32LE_BOM_sep_09_eol_n.csv",  '\t',   '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32LE_BOM_sep_2C_eol_n.csv",  ',',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32LE_BOM_sep_3B_eol_n.csv",  ';',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF32Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_09_eol_n.csv",     '\t',   '"', EndOfLineType::EndOfLineTypeN,   encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_09_eol_rn.csv",    '\t',   '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_2C_eol_n.csv",     ',',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_2C_eol_rn.csv",    ',',    '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_3B_eol_n.csv",     ';',    '"', EndOfLineType::EndOfLineTypeN,   encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_3B_eol_rn.csv",    ';',    '"', EndOfLineType::EndOfLineTypeRN,  encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_1F_eol_n.csv",     '\x1F', '"', EndOfLineType::EndOfLineTypeN,   encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtest_plain_ascii_3x3_sep_2C_eol_n.csv", ',','"', EndOfLineType::EndOfLineTypeN,   encodingUnknown);

    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF8_BOM_sep_2C_eol_r.csv",     ',',    '"', EndOfLineType::EndOfLineTypeR,   encodingUTF8);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16BE_BOM_sep_2C_eol_r.csv",  ',',    '"', EndOfLineType::EndOfLineTypeR,   encodingUTF16Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF16LE_BOM_sep_2C_eol_r.csv",  ',',    '"', EndOfLineType::EndOfLineTypeR,   encodingUTF16Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32BE_BOM_sep_2C_eol_r.csv",  ',',    '"', EndOfLineType::EndOfLineTypeR,   encodingUTF32Be);
    verifyFormatInFile("testfiles/csv_testfiles/csvtestUTF32LE_BOM_sep_2C_eol_r.csv",  ',',    '"', EndOfLineType::EndOfLineTypeR,   encodingUTF32Le);
    verifyFormatInFile("testfiles/csv_testfiles/csvtest_plain_ascii_3x3_sep_3B_eol_r.csv", ';','"', EndOfLineType::EndOfLineTypeR,   encodingUnknown);
}

TEST(dfgCont, TableCsv_multiThreadedRead)
{
    using namespace DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(cont);
    using TableT = TableCsv<char, uint32>;

    const auto nThreadCountRequest = limited(std::thread::hardware_concurrency(), 2u, 4u);
    const TableCsvReadWriteOptions basicReadOptionsForMt(',', ::DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharNone, ::DFG_MODULE_NS(io)::EndOfLineTypeN, ::DFG_MODULE_NS(io)::encodingUTF8);
    const char szPathMatrix200x200[] = "testfiles/matrix_200x200.txt"; // Note: this file has \r\n EOL
    const char szPathMatrix10x10_eol_n[] = "testfiles/matrix_10x10_1to100_eol_n.txt";
    
    const auto nFileSizeMatrix200x200 = ::DFG_MODULE_NS(os)::fileSize(szPathMatrix200x200);
    
    // Basic correctness test
    {
        TableT tSingleThreaded;
        TableT tMultiThreaded;
        const char* path = szPathMatrix200x200;

        // Single-threaded read
        {
            TableCsvReadWriteOptions readOptions = basicReadOptionsForMt;
            // No need to set threadCount as TableCsv should use single-threaded read by default.
            //readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::threadCount>(1);
            DFGTEST_EXPECT_FALSE(readOptions.hasPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>());
            readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>(0); // This should have no effect as multhreaded read is not enabled.
            DFGTEST_EXPECT_TRUE(readOptions.hasPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>());
            tSingleThreaded.readFromFile(path, readOptions);
            const auto nUsedThreadCount = tSingleThreaded.readFormat().getReadStat<TableCsvReadStat::threadCount>();
            DFGTEST_EXPECT_LE(nUsedThreadCount, 1);
        }

        // Multithreaded read
        {
            TableCsvReadWriteOptions readOptions = basicReadOptionsForMt;
            readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadCount>(nThreadCountRequest);
            readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>(0); // Reducing block size to allow threaded reading even for small files.
            tMultiThreaded.readFromFile(path, readOptions);
            const auto nUsedThreadCount = tMultiThreaded.readFormat().getReadStat<TableCsvReadStat::threadCount>();
            DFGTEST_EXPECT_LEFT(nThreadCountRequest, nUsedThreadCount);
        }

        DFGTEST_EXPECT_TRUE(tSingleThreaded.isContentAndSizesIdenticalWith(tMultiThreaded));
        DFGTEST_EXPECT_TRUE(tSingleThreaded.readFormat().isFormatMatchingWith(tMultiThreaded.readFormat()));
        
    }

    // Checking that files having encoding that is compatible with current blocking are read multithreaded.
    {
        const char* path = szPathMatrix10x10_eol_n;
        TableCsvReadWriteOptions readOptionTemplate = basicReadOptionsForMt;
        readOptionTemplate.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadCount>(nThreadCountRequest);
        readOptionTemplate.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>(0); // Reducing block size to allow threaded reading even for small files.
        readOptionTemplate.separatorChar('\t');

        TableT tExpected;
        tExpected.readFromFile(path);

        const auto testEncoding = [&](const ::DFG_MODULE_NS(io)::TextEncoding encoding, const StringViewSzC svExpectedStreamType, const StringViewSzC svExpectedAppendType)
        {
            TableCsvReadWriteOptions readOptions = readOptionTemplate;
            readOptions.textEncoding(encoding);
            TableT t;
            t.readFromFile(path, readOptions);
            const auto nUsedThreadCount = t.readFormat().getReadStat<TableCsvReadStat::threadCount>();
            DFGTEST_EXPECT_LEFT(nThreadCountRequest, nUsedThreadCount);
            DFGTEST_EXPECT_LEFT(tExpected.rowCountByMaxRowIndex(), t.rowCountByMaxRowIndex());
            DFGTEST_EXPECT_LEFT(tExpected.colCountByMaxColIndex(), t.colCountByMaxColIndex());
            DFGTEST_EXPECT_TRUE(t.isContentAndSizesIdenticalWith(tExpected));
            DFGTEST_EXPECT_LEFT(StringViewAscii(SzPtrAscii(svExpectedAppendType.c_str())), t.readFormat().getReadStat<TableCsvReadStat::appenderType>());
            DFGTEST_EXPECT_LEFT(StringViewAscii(SzPtrAscii(svExpectedStreamType.c_str())), t.readFormat().getReadStat<TableCsvReadStat::streamType>());

            DFGTEST_EXPECT_NON_NAN(t.readFormat().getReadStat<TableCsvReadStat::timeBlockReads>());
            DFGTEST_EXPECT_NON_NAN(t.readFormat().getReadStat<TableCsvReadStat::timeBlockMerge>());
            DFGTEST_EXPECT_NON_NAN(t.readFormat().getReadStat<TableCsvReadStat::timeTotal>());

#if 0 // Some diagnostic messages below
            DFGTEST_MESSAGE("Encoding         = " << dfg::io::encodingToStrId(encoding));
            DFGTEST_MESSAGE("Total Read time  = " << t.readFormat().getReadStat<TableCsvReadStat::timeTotal>());
            DFGTEST_MESSAGE("Block read time  = " << t.readFormat().getReadStat<TableCsvReadStat::timeBlockReads>());
            DFGTEST_MESSAGE("Block merge time = " << t.readFormat().getReadStat<TableCsvReadStat::timeBlockMerge>());
            DFGTEST_MESSAGE("Appender type    = " << t.readFormat().getReadStat<TableCsvReadStat::appenderType>().rawStorage());
            DFGTEST_MESSAGE("Stream type      = " << t.readFormat().getReadStat<TableCsvReadStat::streamType>().rawStorage());
            DFGTEST_MESSAGE("");
#endif
        };

        testEncoding(::DFG_MODULE_NS(io)::encodingLatin1, "memory_basic", "utf_charbufc");
        testEncoding(::DFG_MODULE_NS(io)::encodingWindows1252, "memory_encoding", "utf_charbufc");
        testEncoding(::DFG_MODULE_NS(io)::encodingUTF8, "memory_basic", "viewc_basic");
    }

    // Checking that multithreaded reading branches won't even be compiled if handler is not multithread-compatible.
    {
        class NonMtSafeHandler : public TableT::DefaultCellHandler
        {
        public:
            using BaseClass = TableT::DefaultCellHandler;
            using ConcurrencySafeCellHandlerNo = TableT::ConcurrencySafeCellHandlerNo;

            NonMtSafeHandler(TableT& rTable, int dummy) // Having constructor with different signature than base class constructor is here 
                : BaseClass(rTable)                     // to test that isConcurrencySafeT() actually prevents call to default clone implementation
            {
                DFG_UNUSED(dummy);
            }

            static constexpr ConcurrencySafeCellHandlerNo isConcurrencySafeT() { return ConcurrencySafeCellHandlerNo(); }
        };

        TableT table;
        TableCsvReadWriteOptions readOptions = basicReadOptionsForMt;
        readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadCount>(nThreadCountRequest);
        table.readFromFile(szPathMatrix200x200, readOptions, NonMtSafeHandler(table, 1));
        DFGTEST_EXPECT_LEFT(200, table.rowCountByMaxRowIndex());
        DFGTEST_EXPECT_LEFT(200, table.colCountByMaxColIndex());
        const auto nUsedThreadCount = table.readFormat().getReadStat<TableCsvReadStat::threadCount>();
        DFGTEST_EXPECT_TRUE(nUsedThreadCount <= 1);
    }

    // Testing \r handling
    {
        auto bytes = ::DFG_MODULE_NS(io)::fileToByteContainer<std::string>(szPathMatrix200x200);
        ::DFG_MODULE_NS(str)::replaceSubStrsInplaceImpl(bytes, "\r\n", "\r");
        TableCsvReadWriteOptions readOptions = basicReadOptionsForMt;
        readOptions.eolType(::DFG_MODULE_NS(io)::EndOfLineTypeR);
        readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadCount>(nThreadCountRequest);
        readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>(0); // Reducing block size to allow threaded reading even for small files.
        TableT table;
        table.readFromMemory(bytes.data(), bytes.size(), readOptions);
        DFGTEST_EXPECT_LEFT(200, table.rowCountByMaxRowIndex());
        DFGTEST_EXPECT_LEFT(200, table.colCountByMaxColIndex());
        const auto nUsedThreadCount = table.readFormat().getReadStat<TableCsvReadStat::threadCount>();
        DFGTEST_EXPECT_LEFT(nThreadCountRequest, nUsedThreadCount);
    }

    // Checking read block size handling, e.g. that tiny file with 32 lines won't be spread to be read with multiple threads.
    {
        const char* path = szPathMatrix200x200;
        const auto nInputSize = nFileSizeMatrix200x200;

        // By default, expecting single-threaded read since the input file is small
        {
            TableT table;
            TableCsvReadWriteOptions readOptions = basicReadOptionsForMt;
            readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadCount>(2);
            table.readFromFile(path, readOptions);
            const auto nUsedThreadCount = table.readFormat().getReadStat<TableCsvReadStat::threadCount>();
            DFGTEST_EXPECT_LE(nUsedThreadCount, 1);
        }

        // With reduced block size minimum, expecting to see 2 threads
        {
            TableT table;
            TableCsvReadWriteOptions readOptions = basicReadOptionsForMt;
            readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadCount>(4);
            readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>(nInputSize / 2);
            table.readFromFile(path, readOptions);
            const auto nUsedThreadCount = table.readFormat().getReadStat<TableCsvReadStat::threadCount>();
            DFGTEST_EXPECT_LEFT(2, nUsedThreadCount);
        }

        // Testing that can determine intermediate counts correctly (e.g. using 5 threads out of requested 8)
        {
            TableT table;
            TableCsvReadWriteOptions readOptions = basicReadOptionsForMt;
            readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadCount>(8);
            readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>(nInputSize * 2 / 11);
            table.readFromFile(path, readOptions);
            const auto nUsedThreadCount = table.readFormat().getReadStat<TableCsvReadStat::threadCount>();
            DFGTEST_EXPECT_LEFT(5, nUsedThreadCount);
        }

        // With minimal block size, expecting to see maximal thread count being used.
        {
            TableT table;
            TableCsvReadWriteOptions readOptions = basicReadOptionsForMt;
            readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadCount>(8);
            readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>(0);
            table.readFromFile(path, readOptions);
            const auto nUsedThreadCount = table.readFormat().getReadStat<TableCsvReadStat::threadCount>();
            DFGTEST_EXPECT_LEFT(8, nUsedThreadCount);
        }
    }
}

TEST(dfgCont, TableCsv_multiThreadedReadPerformance)
{
#if DFGTEST_ENABLE_BENCHMARKS == 0
    DFGTEST_MESSAGE("TableCsv_multiThreadedReadPerformance skipped due to build settings");
#else
    using namespace DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(cont);
    using TableT = TableCsv<char, uint32>;
    using TimerT = ::DFG_MODULE_NS(time)::TimerCpu;
    const char path[] = "testfiles/matrix_1000x1000.txt"; // Note: this probably too small to actually see benefit from multithreading.

    const auto hwConcurrency = std::thread::hardware_concurrency();
    DFGTEST_MESSAGE("hardware_concurrency() is " << hwConcurrency);
    if (hwConcurrency <= 1)
    {
        DFGTEST_MESSAGE("Not running test since hardware_concurrency() is <= 1");
        return;
    }
    
    const auto baseFormatDef = CsvFormatDefinition(',', ::DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharNone, ::DFG_MODULE_NS(io)::EndOfLineTypeN, ::DFG_MODULE_NS(io)::encodingUTF8);

    const auto PropThreadCount = TableCsvReadWriteOptions::PropertyId::readOpt_threadCount;

    DFGTEST_MESSAGE("Reading path" << path);
    {
        {
            TableT tSingleThreaded;
            TableCsvReadWriteOptions readOptions = baseFormatDef;
            readOptions.setPropertyT<PropThreadCount>(1);
            TimerT timerSingleThread;
            tSingleThreaded.readFromFile(path, readOptions);
            const auto elapsedSingleThread = timerSingleThread.elapsedWallSeconds();
            DFGTEST_MESSAGE("Single-threaded time: " << elapsedSingleThread);
            DFGTEST_EXPECT_LE(tSingleThreaded.readFormat().getReadStat<TableCsvReadStat::threadCount>(), 1);
        }

        TableT tMultiThreaded;
        {
            TimerT timer;
            TableCsvReadWriteOptions readOptions = baseFormatDef;
            readOptions.setPropertyT<PropThreadCount>(hwConcurrency);
            readOptions.setPropertyT<TableCsvReadWriteOptions::PropertyId::readOpt_threadBlockSizeMinimum>(0);
            tMultiThreaded.readFromFile(path, readOptions);

            const auto elapsed = timer.elapsedWallSeconds();
            DFGTEST_MESSAGE("Multi-threaded time : " << elapsed << " (used " << readOptions.getPropertyT<PropThreadCount>(0) << " threads)");
        }

        TableT tSingleThreaded;
        {
            TableCsvReadWriteOptions readOptions = baseFormatDef;
            readOptions.setPropertyT<PropThreadCount>(1);
            TimerT timerSingleThread;
            tSingleThreaded.readFromFile(path, readOptions);
            const auto elapsedSingleThread = timerSingleThread.elapsedWallSeconds();
            DFGTEST_MESSAGE("Single-threaded time: " << elapsedSingleThread);
            DFGTEST_EXPECT_LE(tSingleThreaded.readFormat().getReadStat<TableCsvReadStat::threadCount>(), 1);
        }

        DFGTEST_EXPECT_TRUE(tSingleThreaded.isContentAndSizesIdenticalWith(tMultiThreaded));
        DFGTEST_EXPECT_TRUE(tSingleThreaded.readFormat().isFormatMatchingWith(tMultiThreaded.readFormat()));
    }
#endif // DFGTEST_ENABLE_BENCHMARKS
}

TEST(dfgCont, CsvConfig)
{
    DFG_MODULE_NS(cont)::CsvConfig config;
    config.loadFromFile("testfiles/csvConfigTest_0.csv");

    EXPECT_EQ(DFG_UTF8("UTF8"), config.value(DFG_UTF8("encoding")));
    EXPECT_EQ(DFG_UTF8("1"), config.value(DFG_UTF8("bom_writing")));
    EXPECT_EQ(DFG_UTF8("\\n"), config.value(DFG_UTF8("end_of_line_type")));
    EXPECT_EQ(DFG_UTF8(","), config.value(DFG_UTF8("separator_char")));
    EXPECT_EQ(DFG_UTF8("\""), config.value(DFG_UTF8("enclosing_char")));
    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("channels")));
    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("channels/0")));
    EXPECT_EQ(DFG_UTF8("integer"), config.value(DFG_UTF8("channels/0/type")));
    EXPECT_EQ(DFG_UTF8("50"), config.value(DFG_UTF8("channels/0/width")));
    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("channels/1")));
    EXPECT_EQ(DFG_UTF8("100"), config.value(DFG_UTF8("channels/1/width")));
    EXPECT_EQ(DFG_UTF8("also section-entries can have a value"), config.value(DFG_UTF8("properties")));
    EXPECT_EQ(DFG_UTF8("abc"), config.value(DFG_UTF8("properties/property_one")));
    EXPECT_EQ(DFG_UTF8("def"), config.value(DFG_UTF8("properties/property_two")));

    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("no_ending_separator"), DFG_UTF8("a")));
    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("no_ending_separator/0"), DFG_UTF8("a")));
    EXPECT_EQ(DFG_UTF8("a"), config.value(DFG_UTF8("no_ending_separator/1"), DFG_UTF8("")));
    EXPECT_EQ(DFG_UTF8(""), config.value(DFG_UTF8("no_ending_separator/2"), DFG_UTF8("a")));

    const int euroSign[] = { 0x20AC };
    EXPECT_EQ(DFG_ROOT_NS::SzPtrUtf8(DFG_MODULE_NS(utf)::codePointsToUtf8(euroSign).c_str()), config.value(DFG_UTF8("a_non_ascii_value")));

    EXPECT_EQ(DFG_UTF8("default_value"), config.value(DFG_UTF8("a/non_existent_item"), DFG_UTF8("default_value")));
    EXPECT_EQ(nullptr, config.valueStrOrNull(DFG_UTF8("a/non_existent_item")));

    EXPECT_EQ(20, config.entryCount());

    // tests for contains()
    {
        DFGTEST_EXPECT_TRUE(config.contains(DFG_UTF8("encoding")));
        DFGTEST_EXPECT_TRUE(config.contains(DFG_UTF8("channels")));
        DFGTEST_EXPECT_TRUE(config.contains(DFG_UTF8("channels/0")));
        DFGTEST_EXPECT_TRUE(config.contains(DFG_UTF8("channels/0/width")));
        DFGTEST_EXPECT_FALSE(config.contains(DFG_UTF8("non-existing")));
    }
}

// Both forEachKeyValue and forEachStartingWith
TEST(dfgCont, CsvConfig_forEach)
{
    typedef DFG_ROOT_NS::StringUtf8 StringUtf8;
    typedef DFG_ROOT_NS::StringViewUtf8 StringViewUtf8;
    typedef DFG_MODULE_NS(cont)::CsvConfig ConfigT;
    typedef DFG_MODULE_NS(cont)::Vector<StringUtf8> StorageT;
    typedef DFG_MODULE_NS(cont)::MapVectorSoA<StringUtf8, StringUtf8> MapStrToStr;
    ConfigT config;
    config.loadFromFile("testfiles/csvConfigTest_0.csv");
    StorageT expectedKeys;
    expectedKeys.push_back(StringUtf8(DFG_UTF8("property_one")));
    expectedKeys.push_back(StringUtf8(DFG_UTF8("property_three")));
    expectedKeys.push_back(StringUtf8(DFG_UTF8("property_two")));
    StorageT expectedValues;
    expectedValues.push_back(StringUtf8(DFG_UTF8("abc")));
    expectedValues.push_back(StringUtf8(DFG_UTF8("123")));
    expectedValues.push_back(StringUtf8(DFG_UTF8("def")));

    StorageT actualKeys;
    StorageT actualValues;
    config.forEachStartingWith(DFG_UTF8("properties/"), [&](const ConfigT::StringViewT& key, const ConfigT::StringViewT& value)
    {
        actualKeys.push_back(key.toString());
        actualValues.push_back(value.toString());
    });
    EXPECT_EQ(expectedKeys, actualKeys);
    EXPECT_EQ(expectedValues, actualValues);

    // forEachKeyValue
    MapStrToStr keyValues;
    config.forEachKeyValue([&](const StringViewUtf8& svKey, const StringViewUtf8& svValue)
    {
        keyValues[svKey.toString()] = svValue.toString();
    });
    DFGTEST_EXPECT_LEFT(20, keyValues.size());
    DFGTEST_EXPECT_LEFT(DFG_UTF8("integer"), keyValues[DFG_UTF8("channels/0/type")]);
    DFGTEST_EXPECT_LEFT(DFG_UTF8("UTF8"), keyValues[DFG_UTF8("encoding")]);
}

TEST(dfgCont, CsvConfig_saving)
{
    using ConfigT = ::DFG_MODULE_NS(cont)::CsvConfig;
    ConfigT config;

    const char szPathConfigTest1[] = "testfiles/csvConfigTest_1.csv";
    config.loadFromFile(szPathConfigTest1);
    config.saveToFile("testfiles/generated/csvConfigTest_1.csv");
    ConfigT config2;
    DFGTEST_EXPECT_NE(config, config2);
    config2.loadFromFile("testfiles/generated/csvConfigTest_1.csv");
    DFGTEST_EXPECT_EQ(config, config2);

    const auto config1fileBytes = ::DFG_MODULE_NS(io)::fileToMemory_readOnly(szPathConfigTest1);

    // Test also that the output is exactly the same as input.
    {
        const auto writtenBytes = ::DFG_MODULE_NS(io)::fileToVector("testfiles/generated/csvConfigTest_1.csv");
        DFGTEST_EXPECT_TRUE(::DFG_MODULE_NS(cont)::isEqualContent(config1fileBytes.asSpan<char>(), writtenBytes));
    }

    // Testing memory versions
    {
        auto configMem = ConfigT::fromMemory(::DFG_MODULE_NS(io)::fileToMemory_readOnly(szPathConfigTest1).asSpanFromRValue<char>());
        DFGTEST_EXPECT_EQ(config, configMem);
        const auto fromMemSavedBytes = configMem.saveToMemory();
        DFGTEST_EXPECT_TRUE(::DFG_MODULE_NS(cont)::isEqualContent(config1fileBytes.asSpan<char>(), fromMemSavedBytes.rawStorage()));
    }
}

TEST(dfgCont, CsvConfig_uriPart)
{
    using ConfigT = ::DFG_MODULE_NS(cont)::CsvConfig;

    DFGTEST_EXPECT_EQ_LITERAL_UTF8("abc", ConfigT::uriPart(DFG_UTF8("abc/def/ghi"), 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("def", ConfigT::uriPart(DFG_UTF8("abc/def/ghi"), 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("ghi", ConfigT::uriPart(DFG_UTF8("abc/def/ghi"), 2));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("",    ConfigT::uriPart(DFG_UTF8("abc/def/ghi"), 3));

    DFGTEST_EXPECT_EQ_LITERAL_UTF8("",    ConfigT::uriPart(DFG_UTF8(""), 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("",    ConfigT::uriPart(DFG_UTF8(""), 123456));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("abc", ConfigT::uriPart(DFG_UTF8("abc"), 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("",    ConfigT::uriPart(DFG_UTF8("/12"), 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("12",  ConfigT::uriPart(DFG_UTF8("/12"), 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("12",  ConfigT::uriPart(DFG_UTF8("/12/"), 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("",    ConfigT::uriPart(DFG_UTF8("/12"), 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("",    ConfigT::uriPart(DFG_UTF8("/12/"), 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("",    ConfigT::uriPart(DFG_UTF8("//ab"), 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("",    ConfigT::uriPart(DFG_UTF8("//ab"), 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("ab",  ConfigT::uriPart(DFG_UTF8("//ab"), 2));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("",    ConfigT::uriPart(DFG_UTF8("/ab////cd"), 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("ab",  ConfigT::uriPart(DFG_UTF8("/ab////cd"), 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("cd",  ConfigT::uriPart(DFG_UTF8("/ab////cd///ef"), 5));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("",    ConfigT::uriPart(DFG_UTF8("/ab////cd///ef"), 7));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("ef",  ConfigT::uriPart(DFG_UTF8("/ab////cd///ef"), 8));
}

TEST(dfgCont, CsvFormatDefinition)
{
    using namespace ::DFG_ROOT_NS;
    {
        const auto csvf = CsvFormatDefinition::fromReadTemplate_commaQuoteEolNUtf8();
        DFGTEST_EXPECT_LEFT(',', csvf.separatorChar());
        DFGTEST_EXPECT_LEFT('"', csvf.enclosingChar());
        DFGTEST_EXPECT_LEFT(::DFG_MODULE_NS(io)::EndOfLineTypeN, csvf.eolType());
        DFGTEST_EXPECT_LEFT(::DFG_MODULE_NS(io)::encodingUTF8, csvf.textEncoding());
    }

    {
        const auto csvf = CsvFormatDefinition::fromReadTemplate_commaNoEnclosingEolNUtf8();
        DFGTEST_EXPECT_LEFT(',', csvf.separatorChar());
        DFGTEST_EXPECT_LEFT(CsvFormatDefinition::metaCharNone(), csvf.enclosingChar());
        DFGTEST_EXPECT_LEFT(::DFG_MODULE_NS(io)::EndOfLineTypeN, csvf.eolType());
        DFGTEST_EXPECT_LEFT(::DFG_MODULE_NS(io)::encodingUTF8, csvf.textEncoding());
    }
}

TEST(dfgCont, CsvFormatDefinition_FromCsvConfig)
{
    DFG_MODULE_NS(cont)::CsvConfig config;
    config.loadFromFile("testfiles/csvConfigTest_0.csv");
    DFG_ROOT_NS::CsvFormatDefinition format('a', 'b', DFG_MODULE_NS(io)::EndOfLineTypeRN, DFG_MODULE_NS(io)::encodingUnknown);
    format.bomWriting(false);
    format.fromConfig(config);
    EXPECT_EQ(DFG_MODULE_NS(io)::encodingUTF8, format.textEncoding());
    EXPECT_EQ('"', format.enclosingChar());
    EXPECT_EQ(',', format.separatorChar());
    EXPECT_EQ(DFG_MODULE_NS(io)::EndOfLineTypeN, format.eolType());
    EXPECT_EQ(true, format.bomWriting());
    EXPECT_EQ("abc", format.getProperty("property_one", ""));
    EXPECT_EQ("def", format.getProperty("property_two", ""));

    DFGTEST_EXPECT_LEFT(123, format.getPropertyThroughStrTo<int>("property_three", -1));
    DFGTEST_EXPECT_LEFT(-1, format.getPropertyThroughStrTo<int>("non-existent property", -1));
}

TEST(dfgCont, CsvFormatDefinition_ToConfig)
{
    typedef ::DFG_MODULE_NS(cont)::CsvConfig CsvConfig;
    typedef ::DFG_ROOT_NS::CsvFormatDefinition CsvFormatDef;
    // Basic test
    {
        CsvConfig inConfig;
        inConfig.loadFromFile("testfiles/csvConfigTest_2.csv");
        CsvFormatDef format('a', 'b', DFG_MODULE_NS(io)::EndOfLineTypeRN, DFG_MODULE_NS(io)::encodingUnknown);
        format.bomWriting(false);
        format.fromConfig(inConfig);
        EXPECT_EQ(DFG_MODULE_NS(io)::encodingUTF8, format.textEncoding());
        EXPECT_EQ('"', format.enclosingChar());
        EXPECT_EQ(',', format.separatorChar());
        EXPECT_EQ(DFG_MODULE_NS(io)::EndOfLineTypeRN, format.eolType());
        EXPECT_EQ(true, format.bomWriting());

        CsvConfig outConfig;
        format.appendToConfig(outConfig);

        EXPECT_EQ(inConfig, outConfig);
    }

    // Test 'no enclosing char'-handling
    {
        CsvFormatDef format('a', 'b', DFG_MODULE_NS(io)::EndOfLineTypeRN, DFG_MODULE_NS(io)::encodingUnknown);
        format.enclosingChar(DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharNone);
        CsvConfig config;
        format.appendToConfig(config);
        EXPECT_TRUE(DFG_MODULE_NS(str)::isEmptyStr(config.value(DFG_UTF8("enclosing_char"), DFG_UTF8("a"))));
    }

    // Test handling of printable/unprintable chars
    {
        CsvFormatDef format('\x1f', 0x7e, DFG_MODULE_NS(io)::EndOfLineTypeRN, DFG_MODULE_NS(io)::encodingUnknown);
        CsvFormatDef format2(' ', 33, DFG_MODULE_NS(io)::EndOfLineTypeRN, DFG_MODULE_NS(io)::encodingUnknown);
        CsvConfig config;
        CsvConfig config2;
        format.appendToConfig(config);
        format2.appendToConfig(config2);
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("\\x1f", config.value(DFG_UTF8("separator_char")));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("~", config.value(DFG_UTF8("enclosing_char")));

        DFGTEST_EXPECT_EQ_LITERAL_UTF8("\\x20", config2.value(DFG_UTF8("separator_char")));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("!", config2.value(DFG_UTF8("enclosing_char")));
    }
}

TEST(dfgCont, CsvFormatDefinition_isFormatMatchingWith)
{
    typedef ::DFG_ROOT_NS::CsvFormatDefinition CsvFormatDef;
    const CsvFormatDef baseFormat(',', '"', ::DFG_MODULE_NS(io)::EndOfLineTypeN, ::DFG_MODULE_NS(io)::encodingUTF8);
    DFGTEST_EXPECT_TRUE(baseFormat.isFormatMatchingWith(baseFormat));
    DFGTEST_EXPECT_FALSE(baseFormat.isFormatMatchingWith(CsvFormatDef(';', '"', ::DFG_MODULE_NS(io)::EndOfLineTypeN,  ::DFG_MODULE_NS(io)::encodingUTF8)));
    DFGTEST_EXPECT_FALSE(baseFormat.isFormatMatchingWith(CsvFormatDef(',', '|', ::DFG_MODULE_NS(io)::EndOfLineTypeN,  ::DFG_MODULE_NS(io)::encodingUTF8)));
    DFGTEST_EXPECT_FALSE(baseFormat.isFormatMatchingWith(CsvFormatDef(',', '"', ::DFG_MODULE_NS(io)::EndOfLineTypeRN, ::DFG_MODULE_NS(io)::encodingUTF8)));
    DFGTEST_EXPECT_FALSE(baseFormat.isFormatMatchingWith(CsvFormatDef(',', '"', ::DFG_MODULE_NS(io)::EndOfLineTypeN,  ::DFG_MODULE_NS(io)::encodingUTF16Le)));

    auto formatWithDifferentProps = baseFormat;
    formatWithDifferentProps.setProperty("test", "a");
    DFGTEST_EXPECT_TRUE(baseFormat.isFormatMatchingWith(formatWithDifferentProps));
    DFGTEST_EXPECT_TRUE(formatWithDifferentProps.isFormatMatchingWith(baseFormat));
}

#endif // Build on/off switch
