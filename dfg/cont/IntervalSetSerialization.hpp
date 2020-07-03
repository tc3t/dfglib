#pragma once

#include "../dfgDefs.hpp"
#include "IntervalSet.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../alg.hpp"
#include "../str/strTo.hpp"
#include "../io/BasicImStream.hpp"
#include "../io/DelimitedTextReader.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

namespace DFG_DETAIL_NS
{
    // TODO: Consider moving to str-module
    template <class Iter_T>
    Iter_T skipWhitespaces(Iter_T iterStart, const Iter_T iterEnd)
    {
        while (iterStart != iterEnd && ::DFG_MODULE_NS(alg)::contains(" ", *iterStart))
            ++iterStart;
        return iterStart;
    }

    template <class T>
    void readSingleIntervalSection(IntervalSet<T>& is, const StringViewC& sv)
    {
        if (sv.empty())
            return;
        const auto iterEnd = sv.endRaw();
        auto iterStart = skipWhitespaces(sv.beginRaw(), iterEnd);
        if (iterStart == iterEnd)
            return;
        auto iterSep = std::find(iterStart, sv.end(), ':');
        if (iterSep == iterEnd)
        {
            // Did't find ':' -> looks like single value
            // No error checking here, using GIGO.
            is.insert(::DFG_MODULE_NS(str)::strTo<T>(StringViewC(iterStart, iterEnd)));
            return;
        }
        const auto firstVal = ::DFG_MODULE_NS(str)::strTo<T>(StringViewC(iterStart, iterSep));
        ++iterSep;
        if (iterSep == iterEnd)
            return; // Invalid input; nothing after dash
        const auto secondVal = ::DFG_MODULE_NS(str)::strTo<T>(StringViewC(iterSep, iterEnd));
        if (secondVal < firstVal)
            return;
        is.insertClosed(firstVal, secondVal);
    }
} // DFG_DETAIL_NS namespace


template <class T>
IntervalSet<T> intervalSetFromString(const StringViewC& sv)
{
    IntervalSet<T> rv;
    DFG_MODULE_NS(io)::BasicImStream istrm(sv.beginRaw(), sv.endRaw() - sv.beginRaw());
    const auto metaNone = DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharNone;
    DFG_MODULE_NS(io)::DelimitedTextReader::read<char>(istrm, ';', metaNone, metaNone, [&](size_t, size_t, const char* p, const size_t nSize)
    {
        DFG_DETAIL_NS::readSingleIntervalSection(rv, StringViewC(p, p + nSize));
    });
    return rv;
}

} } // Module namespace
