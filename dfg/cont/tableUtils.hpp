#pragma once

#include "table.hpp"
#include "../io/DelimitedTextReader.hpp"
#include <string>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

// Reads table from stream to Table-container.
template <class Stream_T, class Char_T>
Table<std::basic_string<Char_T>> readStreamToTable(Stream_T& strm, const Char_T cSeparator, const Char_T cEnc, const Char_T eol)
{
    using DelimitedTextReader = ::DFG_MODULE_NS(io)::DelimitedTextReader;
    DelimitedTextReader::CellData<Char_T> cellDataHandler(cSeparator, cEnc, eol);
    auto reader = DelimitedTextReader::createReader(strm, cellDataHandler);
    Table<std::basic_string<Char_T>> tableStrings;
    auto cellHandler = [&](const size_t nRow, const size_t /*nCol*/, const decltype(cellDataHandler)& cdh)
    {
        tableStrings.pushBackOnRow(nRow, cdh.getBuffer());
    };
    DelimitedTextReader::read(reader, cellHandler);
    return tableStrings;
}

// Overload enabling the use of user-defined container of type Table<UserStringType>.
template <class Char_T, class Stream_T, class StrCont_T>
void readStreamToTable(Stream_T& strm, const Char_T cSeparator, const Char_T cEnc, const Char_T eol, StrCont_T& dest)
{
    using DelimitedTextReader = ::DFG_MODULE_NS(io)::DelimitedTextReader;
    DelimitedTextReader::CellData<Char_T> cellDataHandler(cSeparator, cEnc, eol);
    auto reader = DelimitedTextReader::createReader(strm, cellDataHandler);
    auto cellHandler = [&](const size_t nRow, const size_t nCol, const decltype(cellDataHandler)& cdh)
    {
        dest.setElement(nRow, nCol, cdh.getBuffer());
    };
    DelimitedTextReader::read(reader, cellHandler);
}

}} // namespace dfg::cont
