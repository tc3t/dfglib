#pragma once

#include "../dfgBase.hpp"
#include "../str/strlen.hpp"
#include <cctype>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(io) {

template <class Stream_T>
void skipRestOfLine(Stream_T& strm, const char cEol)
{
    strm.ignore(NumericTraits<std::streamsize>::maxValue, cEol);
}

// From current position forward, seeks to first line which begins with sText. If not found, after the call istrm will be eof.
// TODO: Test
template <class Strm_T, class Str_T>
void seekFwdToLineBeginningWith(Strm_T& istrm, const Str_T& sText, const CaseSensitivity cs, const char cEol)
{
    const size_t nLength = DFG_SUB_NS_NAME(str)::strLen(sText);
    if (nLength <= 0)
        return;
    typedef typename std::decay<decltype(sText[0])>::type CharT;
    std::vector<CharT> vecBuf(nLength+1, 0);
    size_t nLine = 0;
    while(istrm)
    {
        size_t nPos = 0;
        decltype(istrm.get()) ch = 0;
        const auto posAtBeginning = istrm.tellg();
        for(; nPos < nLength; ++nPos)
        {
            ch = istrm.get();
            if (cs == CaseSensitivityNo)
            {
                if (std::tolower(ch) != tolower(sText[nPos]))
                    break;
            }
            else // Case: case sensitive
            {
                if (ch != sText[nPos])
                    break;
            }
                    
        }
        if (nPos == nLength) // Found match
        {
            istrm.seekg(posAtBeginning);
            return;
        }
        else if (ch != cEol) // No match -> skip rest of line making sure that if last char read was eol, next line won't be skipped.
            skipRestOfLine(istrm, cEol);

        nLine++;
    }
}

}} // module io
