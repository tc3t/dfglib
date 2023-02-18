#pragma once

#include "../dfgDefs.hpp"
#include "fmtlib/format.h"
#include "../ReadOnlySzParam.hpp"
#include "../cont/elementType.hpp"
#include <string>

DFG_ROOT_NS_BEGIN // Note: format-items are root-namespace on purpose.
{

namespace DFG_DETAIL_NS
{
    template <class T> const T& adjustFmtArg(const T& a) { return a; }
    inline const std::string_view adjustFmtArg(const StringViewC& sv)   { return sv; }
    inline const std::string_view adjustFmtArg(const StringViewSzC& sv) { return sv; }
}

// Appends formatted string to given string using formatting language as in fmtlib.
// TODO:  write directly to given string to avoid possible unneeded memory allocation.
// Remarks:
//      -Behaviour is unspecified if the format string and parameters are not compatible.
//      -Taking StringViewSz instead of StringView as fmt expected null-terminated string
template <class Str_T, class ... Args_T>
Str_T& formatAppend_fmt(Str_T& s, const StringViewSz<typename ::DFG_MODULE_NS(cont)::template ElementType<Str_T>::type>& sFormat, Args_T&&... t)
{
    try {
        s += ::fmt::format(sFormat.c_str(), DFG_DETAIL_NS::adjustFmtArg(t)...);
    }
    catch (const ::fmt::FormatError& formatError)
    {
        DFG_UNUSED(formatError);
    }
    catch (...)
    {
        DFG_ASSERT(false);
    }
    return s;
}

// Writes formatted output to given string
template <class Str_T, class FormatStr_T, class ... Args_T>
Str_T& formatTo_fmt(Str_T& s, const FormatStr_T& sFormat, Args_T&&... t)
{
    s.clear();
    return formatAppend_fmt(s, sFormat, std::forward<Args_T>(t)...);
}

// Formats string and returns templated string type.
template <class Str_T, class FormatStr_T, class ... Args_T>
Str_T format_fmtT(const FormatStr_T& sFormat, Args_T&&... t)
{
    Str_T s;
    return formatTo_fmt(s, sFormat, std::forward<Args_T>(t)...);
}
    
// Specialization of format_fmtT for std::string
template <class ... Args_T>
std::string format_fmt(Args_T&& ... t)
{
    return format_fmtT<std::string>(std::forward<Args_T>(t)...);
}

} // DFG_ROOT_NS namespace
