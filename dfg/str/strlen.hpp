#pragma once

#include <cstring>
#include "../dfgBaseTypedefs.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(str) {

// strLen() returns the size of the string with following convention:
//     -std::strlen() for non-typed null terminated strings.
//     -strLen(typedPsz.c_str()) for typed null terminated strings.
//     -size() or equivalent for std::string, std::wstring and similar string objects.
// 
// Note that for std::(w)string containing null characters,
// strLen(s) and strLen(s.c_str()) may return different value.
// Note also that complexity may be different for different types:
// linear for null terminated strings, constant for objects that store their size like std::string.
inline size_t strLen(NonNullCStr psz)           { return std::strlen(psz); }
inline size_t strLen(char* psz)                 { return std::strlen(psz); }
inline size_t strLen(NonNullCStrW psz)          { return std::wcslen(psz); }
inline size_t strLen(wchar_t* psz)              { return std::wcslen(psz); }

inline size_t strLen(const SzPtrAsciiR& tpsz)   { return strLen(tpsz.c_str()); }
inline size_t strLen(const SzPtrLatin1R& tpsz)  { return strLen(tpsz.c_str()); }
inline size_t strLen(const SzPtrUtf8R& tpsz)    { return strLen(tpsz.c_str()); }

template <class Str_T>
inline size_t strLen(const Str_T& str)          { return str.length(); }


}} // module namespace
