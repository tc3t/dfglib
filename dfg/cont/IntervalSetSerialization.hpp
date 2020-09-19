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
    T parseIntervalValue(const StringViewC& sv, const T nonNegativeWrapPivot)
    {
        auto val = ::DFG_MODULE_NS(str)::strTo<T>(sv);
        if (val < 0)
        {
            DFG_ASSERT_UB(nonNegativeWrapPivot >= 0);
            val = nonNegativeWrapPivot + val;
        }
        return val;
    }

    template <class T>
    void readSingleIntervalSection(IntervalSet<T>& is, const StringViewC& sv, const T nonNegativeWrapPivot)
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
            is.insert(parseIntervalValue(StringViewC(iterStart, iterEnd), nonNegativeWrapPivot));
            return;
        }
        const auto firstVal = parseIntervalValue(StringViewC(iterStart, iterSep), nonNegativeWrapPivot);
        ++iterSep;
        if (iterSep == iterEnd)
            return; // Invalid input; nothing after colon
        const auto secondVal = parseIntervalValue(StringViewC(iterSep, iterEnd), nonNegativeWrapPivot);
        if (secondVal < firstVal)
            return;
        is.insertClosed(firstVal, secondVal);
    }
} // DFG_DETAIL_NS namespace


// Creates interval set from string. If nonNegativeWrapPivot is given, negative values are interpreted as relative to wrapPivot, e.g. 5:-1 with wrap pivot 10 is 5:(10-1) = 5:9
template <class T>
IntervalSet<T> intervalSetFromString(const StringViewC& sv, const T nonNegativeWrapPivot = 0)
{
    IntervalSet<T> rv;
    DFG_ASSERT_INVALID_ARGUMENT(nonNegativeWrapPivot >= 0, "wrap pivot should be non-negative");
    if (nonNegativeWrapPivot < 0)
        return rv;
    DFG_MODULE_NS(io)::BasicImStream istrm(sv.beginRaw(), sv.endRaw() - sv.beginRaw());
    const auto metaNone = DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharNone;
    DFG_MODULE_NS(io)::DelimitedTextReader::read<char>(istrm, ';', metaNone, metaNone, [&](size_t, size_t, const char* p, const size_t nSize)
    {
        DFG_DETAIL_NS::readSingleIntervalSection(rv, StringViewC(p, p + nSize), nonNegativeWrapPivot);
    });
    return rv;
}

} } // Module namespace
