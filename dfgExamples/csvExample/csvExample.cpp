// csvExample
// This example demonstrates csv-file handling in dfglib including use of various encodings and miscellaneous parts such as sortMultiple.

#include <dfg/buildConfig.hpp>
#include <vector>
#include <iostream>
#include <dfg/rand.hpp>
#include <dfg/str/hex.hpp>
#include <dfg/alg/sortMultiple.hpp>
#include <dfg/io/OmcStreamWithEncoding.hpp>
#include <dfg/io/OfStream.hpp>
#include <dfg/io/DelimitedTextReader.hpp>
#include <dfg/io/DelimitedTextWriter.hpp>
#include <dfg/utf/utfBom.hpp>
#include <dfg/time/timerCpu.hpp>

int main(int argc, const char* argv[])
{
    using namespace DFG_ROOT_NS;

    const char szUtf8Path[] = "csvUtf8.csv";
    const char szUtf32BePath[] = "csvUtf32Be.csv";
    const char cUtf8Sep = ',';
    const char cUtf8Enc = '"';
    const char cUtf8Eol = '\n';
    const char cUtf32Sep = '\t';
    const char cUtf32Enc = '"';
    const char cUtf32Eol = '\n';

    const size_t nRowCount = 1000;

    ////////////////////////////////////////////////////////////////
    // Generate random data for csv
    std::vector<std::string> vecStrs;
    std::vector<double> vecDoubles;
    std::vector<int> vecInts;
    std::vector<size_t> vecUids;
    auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();

    for (size_t i = 0; i < nRowCount; ++i)
    {
        const auto bytes = DFG_MODULE_NS(rand)::rand(randEng, uint32(1), NumericTraits<uint32>::maxValue);
        vecStrs.push_back(DFG_MODULE_NS(str)::bytesToHexStr(&bytes, sizeof(bytes)));
        vecDoubles.push_back(DFG_MODULE_NS(rand)::rand(randEng));
        vecInts.push_back(DFG_MODULE_NS(rand)::rand(randEng, -10, 20));
        vecUids.push_back(i);
    }

    ////////////////////////////////////////////////////////////////
    // Sort data by string data
    DFG_MODULE_NS(alg)::sortMultiple(vecStrs, vecDoubles, vecInts, vecUids);

    ////////////////////////////////////////////////////////////////
    // Write to file using UTF8 encoding and , as separator
    {
        DFG_MODULE_NS(io)::OfStreamWithEncoding strmUtf8(szUtf8Path, DFG_MODULE_NS(io)::encodingUTF8);
        for (size_t i = 0, nCount = vecStrs.size(); i < nCount; ++i)
        {
            DFG_MODULE_NS(io)::DelimitedTextCellWriter::writeCellStrm(strmUtf8, vecStrs[i], cUtf8Enc, cUtf8Enc, cUtf8Eol, DFG_MODULE_NS(io)::EbNoEnclose);
            strmUtf8.writeUnicodeChar(cUtf8Sep);
            DFG_MODULE_NS(io)::DelimitedTextCellWriter::writeCellStrm(strmUtf8, DFG_MODULE_NS(str)::toStrC(vecDoubles[i]), cUtf8Enc, cUtf8Enc, cUtf8Eol, DFG_MODULE_NS(io)::EbNoEnclose);
            strmUtf8.writeUnicodeChar(cUtf8Sep);
            DFG_MODULE_NS(io)::DelimitedTextCellWriter::writeCellStrm(strmUtf8, DFG_MODULE_NS(str)::toStrC(vecInts[i]), cUtf8Enc, cUtf8Enc, cUtf8Eol, DFG_MODULE_NS(io)::EbNoEnclose);
            strmUtf8.writeUnicodeChar(cUtf8Sep);
            strmUtf8 << vecUids[i];
            strmUtf8.writeUnicodeChar(cUtf8Eol);
        }
    }

    ////////////////////////////////////////////////////////////////
    // Sort data by double data
    DFG_MODULE_NS(alg)::sortMultiple(vecDoubles, vecStrs, vecInts, vecUids);

    ////////////////////////////////////////////////////////////////
    // Write to file using UTF32BE encoding and \t as separator
    {
        DFG_MODULE_NS(io)::OfStreamWithEncoding strmUtf32Be(szUtf32BePath, DFG_MODULE_NS(io)::encodingUTF32Be);
        for (size_t i = 0, nCount = vecStrs.size(); i < nCount; ++i)
        {
            DFG_MODULE_NS(io)::DelimitedTextCellWriter::writeCellStrm(strmUtf32Be, vecStrs[i], cUtf32Sep, cUtf32Sep, cUtf32Eol, DFG_MODULE_NS(io)::EbNoEnclose);
            strmUtf32Be.writeUnicodeChar(cUtf32Sep);
            DFG_MODULE_NS(io)::DelimitedTextCellWriter::writeCellStrm(strmUtf32Be, DFG_MODULE_NS(str)::toStrC(vecDoubles[i]), cUtf32Sep, cUtf32Enc, cUtf32Eol, DFG_MODULE_NS(io)::EbNoEnclose);
            strmUtf32Be.writeUnicodeChar(cUtf32Sep);
            DFG_MODULE_NS(io)::DelimitedTextCellWriter::writeCellStrm(strmUtf32Be, DFG_MODULE_NS(str)::toStrC(vecInts[i]), cUtf32Sep, cUtf32Enc, cUtf32Eol, DFG_MODULE_NS(io)::EbNoEnclose);
            strmUtf32Be.writeUnicodeChar(cUtf32Sep);
            strmUtf32Be << vecUids[i];
            strmUtf32Be.writeUnicodeChar(cUtf32Eol);
        }
    }

    ////////////////////////////////////////////////////////////////
    // Check file encoding from the created files.
    const auto utf8Bom = DFG_MODULE_NS(io)::checkBOMFromFile(szUtf8Path);
    const auto utf32BeBom = DFG_MODULE_NS(io)::checkBOMFromFile(szUtf32BePath);
    DFG_ASSERT(utf8Bom == DFG_MODULE_NS(io)::encodingUTF8);
    DFG_ASSERT(utf32BeBom == DFG_MODULE_NS(io)::encodingUTF32Be);
    std::cout << "Encoding in file '" << szUtf8Path << "': " << encodingToStrId(utf8Bom) << std::endl;
    std::cout << "Encoding in file '" << szUtf32BePath << "': " << encodingToStrId(utf32BeBom) << std::endl;

    ////////////////////////////////////////////////////////////////
    // Read files using auto dectection for separator char.
    DFG_MODULE_NS(io)::IfStreamWithEncoding istrmUtf8(szUtf8Path);
    DFG_MODULE_NS(io)::IfStreamWithEncoding istrmUtf32Be(szUtf32BePath);

    // TableSz is data structure optimized for storing lots of small null-terminated strings.
    DFG_MODULE_NS(cont)::TableSz<char> tableUtf8;
    DFG_MODULE_NS(cont)::TableSz<char> tableUtf32Be;
    DFG_MODULE_NS(time)::TimerCpu timer;
    DFG_MODULE_NS(io)::DelimitedTextReader::readTableToStringContainer<char>(istrmUtf8, DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharAutoDetect, cUtf8Enc, cUtf8Eol, tableUtf8);
    DFG_MODULE_NS(io)::DelimitedTextReader::readTableToStringContainer<char>(istrmUtf32Be, DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharAutoDetect, cUtf32Enc, cUtf32Eol, tableUtf32Be);

    const auto elapsed = timer.elapsedWallSeconds();
    std::cout << "Reading data took " << elapsed << " seconds" << std::endl;
    std::cout << "Number of cells read from utf8-file: " << tableUtf8.cellCountNonEmpty() << std::endl;
    std::cout << "Number of cells read from utf32Be-file: " << tableUtf32Be.cellCountNonEmpty() << std::endl;

    ////////////////////////////////////////////////////////////////
    // Convert read data to original format.
    std::vector<std::string> vecStrsReadUtf8, vecStrsReadUtf32Be;
    std::vector<double> vecDoublesReadUtf8, vecDoublesReadUtf32Be;
    std::vector<int> vecIntsReadUtf8, vecIntsReadUtf32Be;
    std::vector<size_t> vecUidsReadUtf8, vecUidsReadUtf32Be;

    tableUtf8.forEachFwdRowInColumn(0, [&](const Dummy&, const char* const psz)
    {
        vecStrsReadUtf8.push_back(psz);
    });
    tableUtf8.forEachFwdRowInColumn(1, [&](const Dummy&, const char* const psz)
    {
        vecDoublesReadUtf8.push_back(DFG_MODULE_NS(str)::strTo<double>(psz));
    });
    tableUtf8.forEachFwdRowInColumn(2, [&](const Dummy&, const char* const psz)
    {
        vecIntsReadUtf8.push_back(DFG_MODULE_NS(str)::strTo<int>(psz));
    });
    tableUtf8.forEachFwdRowInColumn(3, [&](const Dummy&, const char* const psz)
    {
        vecUidsReadUtf8.push_back(DFG_MODULE_NS(str)::strTo<size_t>(psz));
    });

    tableUtf32Be.forEachFwdRowInColumn(0, [&](const Dummy&, const char* const psz)
    {
        vecStrsReadUtf32Be.push_back(psz);
    });
    tableUtf32Be.forEachFwdRowInColumn(1, [&](const Dummy&, const char* const psz)
    {
        vecDoublesReadUtf32Be.push_back(DFG_MODULE_NS(str)::strTo<double>(psz));
    });
    tableUtf32Be.forEachFwdRowInColumn(2, [&](const Dummy&, const char* const psz)
    {
        vecIntsReadUtf32Be.push_back(DFG_MODULE_NS(str)::strTo<int>(psz));
    });
    tableUtf32Be.forEachFwdRowInColumn(3, [&](const Dummy&, const char* const psz)
    {
        vecUidsReadUtf32Be.push_back(DFG_MODULE_NS(str)::strTo<size_t>(psz));
    });


    ////////////////////////////////////////////////////////////////
    // Verify equality by sorting by uid and then doing equality check.
    DFG_MODULE_NS(alg)::sortMultiple(vecUids, vecStrs, vecDoubles, vecInts);
    DFG_MODULE_NS(alg)::sortMultiple(vecUidsReadUtf8, vecStrsReadUtf8, vecDoublesReadUtf8, vecIntsReadUtf8);
    DFG_MODULE_NS(alg)::sortMultiple(vecUidsReadUtf32Be, vecStrsReadUtf32Be, vecDoublesReadUtf32Be, vecIntsReadUtf32Be);
    
    if (vecUidsReadUtf8 == vecUids && vecUidsReadUtf8 == vecUidsReadUtf32Be &&
        vecStrs == vecStrsReadUtf8 && vecStrs == vecStrsReadUtf32Be &&
        vecDoubles == vecDoublesReadUtf8 && vecDoubles == vecDoublesReadUtf32Be &&
        vecInts == vecIntsReadUtf8 && vecInts == vecIntsReadUtf32Be)
        std::cout << "Ok: read data match the original data.\n";
    else
        std::cout << "Bug: read items did not match\n";

    return 0;
}
