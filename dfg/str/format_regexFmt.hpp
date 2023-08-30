#pragma once

#include "format_fmt.hpp"
#include <regex>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(str) {

enum class FormatTo_regexFmt_returnValue
{
    formatApplied,              // Regex match was found and format function was applied.
                                // Note: with this return value it is unspecified whether format-call actually worked.
    noRegexMatch,               // No regex match found
    matchCountOverflow          // There were more than supported number of matches, formatting was not applied.
}; // enum class formatTo_regexFmt_returnValue

/*
*  Formats string to output param 'rsOutput' so that regex subexpressions are used as arguments for fmt-style format string.
*  @param rsOutput std::string -compatible string receives the output which has the following content depending return value:
*       - formatApplied:
*           - Format successful : formatted output
*           - Format unsuccesful: unspecified content
*       - formatApplied         : empty
*       - matchCountOverflow    : empty
*  @param svInput Input string against which regex is matched
*  @param re Regular expression to use against svInput
*  @param svFormat fmt-style format string where {i} corresponds to subexpression $i.
*       Note: Currently maximum supported index is 10, having more matches causes formatting to fail.
*       Note: Indexing is essentially 1-based, i.e. first parenthesis item has index 1, at index 0 is the full match like in https://en.cppreference.com/w/cpp/regex/match_results
*  @return Enum FormatTo_regexFmt_returnValue, see it's documentation for details
*/
template <class Str_T>
inline FormatTo_regexFmt_returnValue formatTo_regexFmt(Str_T& rsOutput, const StringViewC& svInput, const std::regex& re, const StringViewSzC& svFormat)
{
    std::cmatch bm;
    const auto bRegExMatch = std::regex_match(svInput.beginRaw(), svInput.endRaw(), bm, re);
    const auto toArg = [](const auto& match) { return StringViewC(match.first, match.second); };
    rsOutput.clear();
    if (!bRegExMatch)
        return FormatTo_regexFmt_returnValue::noRegexMatch;
    const auto nMatchCount = bm.size();

    switch (nMatchCount)
    {
    case  1: formatTo_fmt(rsOutput, svFormat, toArg(bm[0])); break;
    case  2: formatTo_fmt(rsOutput, svFormat, toArg(bm[0]), toArg(bm[1])); break;
    case  3: formatTo_fmt(rsOutput, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2])); break;
    case  4: formatTo_fmt(rsOutput, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3])); break;
    case  5: formatTo_fmt(rsOutput, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4])); break;
    case  6: formatTo_fmt(rsOutput, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5])); break;
    case  7: formatTo_fmt(rsOutput, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5]), toArg(bm[6])); break;
    case  8: formatTo_fmt(rsOutput, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5]), toArg(bm[6]), toArg(bm[7])); break;
    case  9: formatTo_fmt(rsOutput, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5]), toArg(bm[6]), toArg(bm[7]), toArg(bm[8])); break;
    case 10: formatTo_fmt(rsOutput, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5]), toArg(bm[6]), toArg(bm[7]), toArg(bm[8]), toArg(bm[9])); break;
    case 11: formatTo_fmt(rsOutput, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5]), toArg(bm[6]), toArg(bm[7]), toArg(bm[8]), toArg(bm[9]), toArg(bm[10])); break;
    default: return FormatTo_regexFmt_returnValue::matchCountOverflow;
    }
    return FormatTo_regexFmt_returnValue::formatApplied;
}

// Like formatTo_regexFmt(), but writes output to a new string and returns it along with return value of formatTo_regexFmt()
inline std::pair<std::string, FormatTo_regexFmt_returnValue> format_regexFmt(StringViewC svInput, const std::regex & re, StringViewSzC svFormat)
{
    std::string s;
    const auto rv = formatTo_regexFmt(s, svInput, re, svFormat);
    return std::pair(s, rv);
    }

} } // Namespace dfg::str
