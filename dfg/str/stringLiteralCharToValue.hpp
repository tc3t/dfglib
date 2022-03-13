#pragma once

#include <utility>
#include "../numericTypeTools.hpp"
#include <cstdlib>
#include <string>
#include "strLen.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(str) {

    namespace DFG_DETAIL_NS
    {
        constexpr bool hasAsciiCompatibleCharLiterals()
        {
            // Testing only a subset, for more detail:  https://stackoverflow.com/questions/63980636/how-can-i-check-if-char-encoding-is-ascii
            return '\a' == 0x07 && '\b' == 0x08 && '\t' == 0x09 && '\n' == 0x0a && '\v' == 0x0b && '\f' == 0x0c &&
                   '\r' == 0x0d && 'a' == 0x61 && '~' == 0x7e;
        }
    }

    // Converts character definition as written in string literal to corresponding character value (e.g. "\\t" -> '\t')
    // Returns: pair (bool, Char_T), where bool tells whether the Char_T value is valid.
    //      If str.size() == 1 && str[0] is non-escape sequence that fits to Char_T -> returns (true, str[0])
    //      else if str.size() >= 2 && str is valid escape sequence (e.g. "\\t"), returns (true, value of character corresponding to the escape sequence)
    //      else returns (false, unspecified value).
    template <class Char_T, class Str_T>
    std::pair<bool, Char_T> stringLiteralCharToValue(const Str_T& str)
    {
        std::pair<bool, Char_T> rv(false, '\0');
        const size_t n = strLen(str);

        if (n == 1)
        {
            if (isValWithinLimitsOfType<Char_T>(str[0]))
            {
                rv.first = true;
                rv.second = static_cast<Char_T>(str[0]);
            }
        }
        else if (n >= 2 && str[0] == '\\' && isValWithinLimitsOfType<Char_T>(str[1]))
        {
            const Char_T c = static_cast<Char_T>(str[1]);
            Char_T rvC = 'a';
            int base = 0;
            switch (c)
            {
                case '\'':  rvC = '\''; break;
                case '\"':  rvC = '\"'; break;
                case '?':   rvC = '?'; break;
                case '\\':  rvC = '\\'; break;
                case 'a':   rvC = '\a'; break;
                case 'b':   rvC = '\b'; break;
                case 'f':   rvC = '\f'; break;
                case 'n':   rvC = '\n'; break;
                case 'r':   rvC = '\r'; break;
                case 't':   rvC = '\t'; break;
                case 'v':   rvC = '\v'; break;
                case '0':   base = 8; break;
                case 'x':   base = 16; break;
            }
            if (base != 0)
            {
                std::string s;
                for (size_t i = 2; i < n; ++i)
                {
                    const auto charVal = str[i];
                    if (!isValWithinLimitsOfType<char>(charVal)) // TODO: more precise checking (i.e. only digits and hex chars should be accepted).
                        return rv;
                    s.push_back(static_cast<char>(charVal));
                }
                if (n == 2 && base == 8)
                    s = "0";
                char* pEnd = nullptr;
                auto val = strtoul(s.c_str(), &pEnd, base);
                if (pEnd != s.c_str() && isValWithinLimitsOfType<Char_T>(val))
                    rv = std::pair<bool, Char_T>(true, static_cast<Char_T>(val));
            }
            else if (rvC != 'a' && n == 2)
                rv = std::pair<bool, Char_T>(true, rvC);
        }
        return rv;
    }

    // Given a char value, returns a string representation of it. For most of printable ASCII-values, string has char as such and
    // for rest, returns escaped representation which can be converted back to char value with stringLiteralCharToValue()
    inline std::string charToPrintable(const int32 c)
    {
        DFG_STATIC_ASSERT(DFG_DETAIL_NS::hasAsciiCompatibleCharLiterals(), "Implementation assumes ascii-compatible characters");
        if (c >= 33 && c <= 126)
            return std::string(1, static_cast<char>(c));
        else if (c >= 0)
        {
            char buffer[16];
            switch (c)
            {
                case '\t': return "\\t";
                case '\a': return "\\a";
                case '\b': return "\\b";
                case '\f': return "\\f";
                case '\n': return "\\n";
                case '\r': return "\\r";
                case '\v': return "\\v";
                default: DFG_DETAIL_NS::sprintf_s(buffer, sizeof(buffer), "\\x%x", c); return buffer;
            }
        }
        else
            return std::string();
    }

} } // module namespace
