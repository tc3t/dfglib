#pragma once

#include <cstring>
#include <string> // std::char_traits
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
template <class T> size_t strLen(T* psz)        { return std::char_traits<T>::length(psz); } // This overload was needed on MSVC2015
template <class T> size_t strLen(const T* psz)  { return std::char_traits<T>::length(psz); }

template <class Char_T, CharPtrType TypeId_T>
inline size_t strLen(const SzPtrT<Char_T, TypeId_T>& tpsz) { return strLen(tpsz.c_str()); }

template <class Str_T>
inline size_t strLen(const Str_T& str)          { return str.length(); }

}} // module namespace
