#pragma once

#include <string>
#include <cstring>
#include "../dfgBaseTypedefs.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(str) {

// strLen returns the size of the string with following convention:
// strlen() for null terminated strings.
// size() or equivalent for std::string, std::wstring and similar string objects.
// Note that this means that for std::string s containing null characters,
// strLen(s) and strLen(s.c_str()) may return different value.
// Note also that complexity may be different for different types:
// linear for null terminated strings, constant for objects that store their size like std::string.
// TODO: test
inline size_t strLen(NonNullCStr psz)			{return strlen(psz);}
inline size_t strLen(NonNullCStrW psz)			{return wcslen(psz);}
inline size_t strLen(const std::string& str)	{return str.length();}
inline size_t strLen(const std::wstring& str)	{return str.length();}

}} // module namespace
