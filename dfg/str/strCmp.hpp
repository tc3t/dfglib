#pragma once

#include <cstring>
#include "../dfgBaseTypedefs.hpp"
#include "../SzPtrTypes.hpp"
#include <cwchar> // For std::wcscmp

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(str) {

    // Wrappers for strcmp and wsccmp.
    // Lexicographic("dictionary-like") comparison. For example: "a" < "b", "111" < "15").
    // Note thought that for example "B" < "a".
    // [return] : If psz1 < psz2: negative value
    //			  If psz1 == psz2 : 0
    //			  If psz1 > psz2 : positive value
    inline int strCmp(const NonNullCStr psz1, const NonNullCStr psz2)   { return std::strcmp(psz1, psz2); }
    inline int strCmp(const NonNullCStrW psz1, const NonNullCStrW psz2) { return std::wcscmp(psz1, psz2); }
    template <class Char_T, CharPtrType Type_T> inline int strCmp(const SzPtrT<Char_T, Type_T>& tpsz1, const SzPtrT<Char_T, Type_T>& tpsz2)
    {
        return strCmp(tpsz1.c_str(), tpsz2.c_str());
    }

} } // module namespace
