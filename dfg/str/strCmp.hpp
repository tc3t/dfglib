#pragma once

#include <cstring>
#include "../dfgBaseTypedefs.hpp"
#include "../SzPtrTypes.hpp"
#include <cwchar> // For std::wcscmp
#include <string> // For std::char_traits

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(str) {

    // Wrappers for strcmp and wsccmp.
    // Lexicographic("dictionary-like") comparison. For example: "a" < "b", "111" < "15").
    // Note thought that for example "B" < "a".
    // [return] : If psz1 < psz2: negative value
    //			  If psz1 == psz2 : 0
    //			  If psz1 > psz2 : positive value
    inline int strCmp(const NonNullCStr psz1, const NonNullCStr psz2)   { return std::strcmp(psz1, psz2); }
    inline int strCmp(const NonNullCStrW psz1, const NonNullCStrW psz2) { return std::wcscmp(psz1, psz2); }
    inline int strCmp(const char16_t* psz1, const char16_t* psz2)
    {
        // TODO: improve, lengths shouldn't need to be precalculated.
        const auto nSize1 = std::char_traits<char16_t>::length(psz1);
        const auto nSize2 = std::char_traits<char16_t>::length(psz2);
        const auto nMinSize = Min(nSize1, nSize2);
        const auto compare = std::char_traits<char16_t>::compare(psz1, psz2, nMinSize);
        if (compare != 0)
            return compare;
        else if (nSize1 != nSize2)
            return (nSize1 < nSize2) ? -1 : 1;
        else
            return 0;
    }

    template <class Char_T, CharPtrType Type_T> inline int strCmp(const SzPtrT<Char_T, Type_T>& tpsz1, const SzPtrT<Char_T, Type_T>& tpsz2)
    {
        return strCmp(tpsz1.c_str(), tpsz2.c_str());
    }

} } // module namespace
