#pragma once

#include "../buildConfig.hpp"
#include "../io.hpp"
#include "../RangeIterator.hpp"
#include "../iter/OstreamIterator.hpp"
#include <algorithm>
#include <iterator>
#include "../iter/szIterator.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(io) {

enum EnclosementBehaviour
{
    EbEnclose,
    EbNoEnclose,
    EbEncloseIfNeeded
};

class DFG_CLASS_NAME(DelimitedTextCellWriter)
{
public:
    template <class OutputIter_T, class InputRange_T, class Char_T>
    static void writeCellFromStrIter(OutputIter_T iterOut,
                                    const InputRange_T& input,
                                    const Char_T cSep,
                                    const Char_T cEnc,
                                    const Char_T cEol,
                                    const EnclosementBehaviour eb)
    {
        bool bEnclose = (eb == EbEnclose);
        if (!bEnclose && eb == EbEncloseIfNeeded)
        {
            // TODO: use any_of-like algorithm.
            for (auto iter = input.begin(), iterEnd = input.end(); !isAtEnd(iter, iterEnd); ++iter)
            {
                const auto c = *iter;
                // TODO: enclose if has leading whitespaces?
                if (c == cEol || c == cEnc || c == cSep)
                {
                    bEnclose = true;
                    break;
                }
            }
        }
        if (!bEnclose)
        {
            //*iterOut = input; // TODO: Use this when possible (notably with ostream_iterators).
            // Can't be used as such because it will fail if iterator is element-wise such as std::back_inserter.
            std::copy(input.begin(), input.end(), iterOut);
        }
        else
        {
            *iterOut++ = cEnc;
            for (auto iter = input.begin(), iterEnd = input.end(); !isAtEnd(iter, iterEnd); ++iter)
            {
                const auto c = *iter;
                if (c == cEnc)
                {
                    *iterOut++ = c;
                    *iterOut++ = c;
                }
                else
                    *iterOut++ = c;
            }
            *iterOut++ = cEnc;
        }
    }

    template <class Stream_T, class InputRange_T, class Char_T>
    static void writeCellFromStrStrm(Stream_T& strm,
        const InputRange_T& input,
        const Char_T cSep,
        const Char_T cEnc,
        const Char_T cEol,
        const EnclosementBehaviour eb)
    {
        writeCellFromStrIter(DFG_MODULE_NS(iter)::makeOstreamIterator(strm), input, cSep, cEnc, cEol, eb);
    }

    template <class OutputIter_T, class Elem_T>
    static void writeItemImpl(OutputIter_T iterOut,
        const Elem_T& item,
        const char cSep,
        const char cEnc,
        const char cEol,
        const EnclosementBehaviour eb)
    {
        DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(Elem_T, "No implementation exists for writeItemImpl with given type");
    }

    template <class OutputIter_T>
    static void writeItemImpl(OutputIter_T iterOut,
        const std::string& str,
        const char cSep,
        const char cEnc,
        const char cEol,
        const EnclosementBehaviour eb)
    {
        writeCellFromStrIter(iterOut, str, cSep, cEnc, cEol, eb);
    }

    template <class OutputIter_T>
    static void writeItemImpl(OutputIter_T iterOut,
        const char* psz,
        const char cSep,
        const char cEnc,
        const char cEol,
        const EnclosementBehaviour eb)
    {
        writeCellFromStrIter(iterOut, makeSzRange(psz), cSep, cEnc, cEol, eb);
    }

    template <class OutputIter_T>
    static void writeItemImpl(OutputIter_T iterOut,
        const std::wstring& str,
        const char cSep,
        const char cEnc,
        const char cEol,
        const EnclosementBehaviour eb)
    {
        writeCellFromStrIter(iterOut, str, cSep, cEnc, cEol, eb);
    }

    template <class OutputIter_T>
    static void writeItemImpl(OutputIter_T iterOut,
        const wchar_t* psz,
        const char cSep,
        const char cEnc,
        const char cEol,
        const EnclosementBehaviour eb)
    {
        writeCellFromStrIter(iterOut, makeSzRange(psz), cSep, cEnc, cEol, eb);
    }

    template <class OutputIter_T, class Elem_T>
    static void writeCellIter(OutputIter_T iterOut,
                                    const Elem_T& elem,
                                    const char cSep,
                                    const char cEnc,
                                    const char cEol,
                                    const EnclosementBehaviour eb)
    {
        writeItemImpl(iterOut, elem, cSep, cEnc, cEol, eb);
    }

    template <class Stream_T, class T> static void writeCellStrm(Stream_T& strm,
        const T& item,
        const char cSep,
        const char cEnc,
        const char cEol,
        const EnclosementBehaviour eb)
    {
        writeItemImpl(DFG_MODULE_NS(iter)::makeOstreamIterator(strm), item, cSep, cEnc, cEol, eb);
    }

};

class DFG_CLASS_NAME(DelimitedTextWriter)
{
public:
    // TODO: test
    template <class Stream_T, class Cont_T>
    static void writeMultiple(Stream_T& strm,
                            const Cont_T& cont,
                            const char cSep = ',',
                            const char cEnc = '"',
                            const char cEol = '\n',
                            const EnclosementBehaviour eb = EbEncloseIfNeeded)
    {
        writeDelimited(strm,
                        cont,
                        cSep,
                        [&](Stream_T& strm, const typename Cont_T::value_type& item)
                        {
                            DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellStrm(strm,
                                                                 item,
                                                                 cSep,
                                                                 cEnc,
                                                                 cEol,
                                                                 eb);
                        });
        //strm << cEol;
    }

}; // DFG_CLASS_NAME(DelimitedTextWriter)




}} // module namespace
