#pragma once

#include "../dfgDefs.hpp"
#include "fmtlib/format.h"
#include <string>

DFG_ROOT_NS_BEGIN // Note: format-items are root-namespace on purpose.
{ 

    /*
    The code for formatAppend_fmt() overloads has been generated using the following parameters:
        ForwardingFuncDefinition param;
        param.m_nMinParamCount = 1;
        param.m_nMaxParamCount = 10;
        param.m_sIndent = "    ";
        param.m_sAdditionalTemplateParams = "class Str_T, class Char_T";
        param.m_sLeadingReturnType = "Str_T&";
        param.m_sFuncName = "formatAppend_fmt";
        param.m_sAdditionalCallParams = "Str_T& s, const Char_T* pszFormat";
        param.m_sCode = "try {\n"
        "    s += ::fmt::format(pszFormat, #);\n"
        "}\n"
        "catch(...)\n"
        "{\n"
        "    DFG_ASSERT(false);\n"
        "}\n"
        "return s;";
    */
// Appends formatted string to given string using formatting language as in fmtlib.
// TODO:  write directly to given string to avoid possible unneeded memory allocation.
// Remarks: Behaviour is unspecified if the format string and parameters are not compatible.
template <class Str_T, class Char_T, class T0>
Str_T& formatAppend_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0)
{
    try {
        s += ::fmt::format(pszFormat, std::forward<T0>(t0));
    }
    catch (...)
    {
        DFG_ASSERT(false);
    }
    return s;
}

template <class Str_T, class Char_T, class T0, class T1>
Str_T& formatAppend_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1)
{
    try {
        s += ::fmt::format(pszFormat, std::forward<T0>(t0), std::forward<T1>(t1));
    }
    catch (...)
    {
        DFG_ASSERT(false);
    }
    return s;
}

template <class Str_T, class Char_T, class T0, class T1, class T2>
Str_T& formatAppend_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2)
{
    try {
        s += ::fmt::format(pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2));
    }
    catch (...)
    {
        DFG_ASSERT(false);
    }
    return s;
}

template <class Str_T, class Char_T, class T0, class T1, class T2, class T3>
Str_T& formatAppend_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3)
{
    try {
        s += ::fmt::format(pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3));
    }
    catch (...)
    {
        DFG_ASSERT(false);
    }
    return s;
}

template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4>
Str_T& formatAppend_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4)
{
    try {
        s += ::fmt::format(pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4));
    }
    catch (...)
    {
        DFG_ASSERT(false);
    }
    return s;
}

template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5>
Str_T& formatAppend_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5)
{
    try {
        s += ::fmt::format(pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5));
    }
    catch (...)
    {
        DFG_ASSERT(false);
    }
    return s;
}

template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
Str_T& formatAppend_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6)
{
    try {
        s += ::fmt::format(pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6));
    }
    catch (...)
    {
        DFG_ASSERT(false);
    }
    return s;
}

template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
Str_T& formatAppend_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7)
{
    try {
        s += ::fmt::format(pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7));
    }
    catch (...)
    {
        DFG_ASSERT(false);
    }
    return s;
}

template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
Str_T& formatAppend_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, T8&& t8)
{
    try {
        s += ::fmt::format(pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7), std::forward<T8>(t8));
    }
    catch (...)
    {
        DFG_ASSERT(false);
    }
    return s;
}

template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
Str_T& formatAppend_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, T8&& t8, T9&& t9)
{
    try {
        s += ::fmt::format(pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7), std::forward<T8>(t8), std::forward<T9>(t9));
    }
    catch (...)
    {
        DFG_ASSERT(false);
    }
    return s;
}

    /*
    The code for formatTo_fmt() overloads has been generated using the following parameters:
        ForwardingFuncDefinition param;
        param.m_nMinParamCount = 1;
        param.m_nMaxParamCount = 10;
        param.m_sIndent = "    ";
        param.m_sAdditionalTemplateParams = "class Str_T, class Char_T";
        param.m_sLeadingReturnType = "Str_T&";
        param.m_sFuncName = "formatTo_fmt";
        param.m_sAdditionalCallParams = "Str_T& s, const Char_T* pszFormat";
        param.m_sCode = "s.clear();\n"
        "return formatAppend_fmt(s, pszFormat, #);";
    */

    // Placeholder for writing the formatted string to an existing string object.
    template <class Str_T, class Char_T, class T0>
    Str_T& formatTo_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0)
    {
        s.clear();
        return formatAppend_fmt(s, pszFormat, std::forward<T0>(t0));
    }

    template <class Str_T, class Char_T, class T0, class T1>
    Str_T& formatTo_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1)
    {
        s.clear();
        return formatAppend_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2>
    Str_T& formatTo_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2)
    {
        s.clear();
        return formatAppend_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3>
    Str_T& formatTo_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3)
    {
        s.clear();
        return formatAppend_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4>
    Str_T& formatTo_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4)
    {
        s.clear();
        return formatAppend_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5>
    Str_T& formatTo_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5)
    {
        s.clear();
        return formatAppend_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    Str_T& formatTo_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6)
    {
        s.clear();
        return formatAppend_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    Str_T& formatTo_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7)
    {
        s.clear();
        return formatAppend_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
    Str_T& formatTo_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, T8&& t8)
    {
        s.clear();
        return formatAppend_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7), std::forward<T8>(t8));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
    Str_T& formatTo_fmt(Str_T& s, const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, T8&& t8, T9&& t9)
    {
        s.clear();
        return formatAppend_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7), std::forward<T8>(t8), std::forward<T9>(t9));
    }

    /*
    The code for format_fmtT() overloads has been generated using the following parameters:
        ForwardingFuncDefinition param;
        param.m_nMinParamCount = 1;
        param.m_nMaxParamCount = 10;
        param.m_sIndent = "    ";
        param.m_sAdditionalTemplateParams = "class Str_T, class Char_T";
        param.m_sLeadingReturnType = "Str_T";
        param.m_sFuncName = "format_fmtT";
        param.m_sAdditionalCallParams = "const Char_T* pszFormat";
        param.m_sCode = "Str_T s;\n"
                        "return formatTo_fmt(s, pszFormat, #);";
    */
    template <class Str_T, class Char_T, class T0>
    Str_T format_fmtT(const Char_T* pszFormat, T0&& t0)
    {
        Str_T s;
        return formatTo_fmt(s, pszFormat, std::forward<T0>(t0));
    }

    template <class Str_T, class Char_T, class T0, class T1>
    Str_T format_fmtT(const Char_T* pszFormat, T0&& t0, T1&& t1)
    {
        Str_T s;
        return formatTo_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2>
    Str_T format_fmtT(const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2)
    {
        Str_T s;
        return formatTo_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3>
    Str_T format_fmtT(const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3)
    {
        Str_T s;
        return formatTo_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4>
    Str_T format_fmtT(const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4)
    {
        Str_T s;
        return formatTo_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5>
    Str_T format_fmtT(const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5)
    {
        Str_T s;
        return formatTo_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    Str_T format_fmtT(const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6)
    {
        Str_T s;
        return formatTo_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    Str_T format_fmtT(const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7)
    {
        Str_T s;
        return formatTo_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
    Str_T format_fmtT(const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, T8&& t8)
    {
        Str_T s;
        return formatTo_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7), std::forward<T8>(t8));
    }

    template <class Str_T, class Char_T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
    Str_T format_fmtT(const Char_T* pszFormat, T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, T8&& t8, T9&& t9)
    {
        Str_T s;
        return formatTo_fmt(s, pszFormat, std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7), std::forward<T8>(t8), std::forward<T9>(t9));
    }
    
    /*
    The code for format_fmt() overloads has been generated using the following parameters:
        ForwardingFuncDefinition param;
        param.m_nMinParamCount = 1;
        param.m_nMaxParamCount = 10;
        param.m_sIndent = "    ";
        param.m_sLeadingReturnType = "std::string";
        param.m_sFuncName = "format_fmt";
        param.m_sCode = "return format_fmtT<std::string>(#);";
    */
    template <class T0>
    std::string format_fmt(T0&& t0)
    {
        return format_fmtT<std::string>(std::forward<T0>(t0));
    }

    template <class T0, class T1>
    std::string format_fmt(T0&& t0, T1&& t1)
    {
        return format_fmtT<std::string>(std::forward<T0>(t0), std::forward<T1>(t1));
    }

    template <class T0, class T1, class T2>
    std::string format_fmt(T0&& t0, T1&& t1, T2&& t2)
    {
        return format_fmtT<std::string>(std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2));
    }

    template <class T0, class T1, class T2, class T3>
    std::string format_fmt(T0&& t0, T1&& t1, T2&& t2, T3&& t3)
    {
        return format_fmtT<std::string>(std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3));
    }

    template <class T0, class T1, class T2, class T3, class T4>
    std::string format_fmt(T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4)
    {
        return format_fmtT<std::string>(std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4));
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5>
    std::string format_fmt(T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5)
    {
        return format_fmtT<std::string>(std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5));
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    std::string format_fmt(T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6)
    {
        return format_fmtT<std::string>(std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6));
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    std::string format_fmt(T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7)
    {
        return format_fmtT<std::string>(std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7));
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
    std::string format_fmt(T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, T8&& t8)
    {
        return format_fmtT<std::string>(std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7), std::forward<T8>(t8));
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
    std::string format_fmt(T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, T8&& t8, T9&& t9)
    {
        return format_fmtT<std::string>(std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), std::forward<T5>(t5), std::forward<T6>(t6), std::forward<T7>(t7), std::forward<T8>(t8), std::forward<T9>(t9));
    }

} // namespace
