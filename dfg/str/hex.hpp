#pragma once

#include "../dfgBase.hpp"
#include <string>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(str) {


// Converts arrays of bytes to hex char string.
// [in] pBytes : Pointer to byte array.
// [in] nBytes : Number of bytes in array.
// [out] iterStr  : Output iterator such as pointer to beginning of string that will receive the hex string. Must be valid for 2*nBytes (+ 1 if bAppendNull is true) writes (e.g. the array pointed to must be at least that length(in characters).)
// [in] bAppendNull : True iff. null character should be appended to iterStr. Not wanted e.g. for std::string iterators.
// Note: It's callers responsibility to make sure that given output iterator is accessible for the whole range.
template <class Iter_T>
inline void bytesToHexStr(ConstVoidPtr pBytes, const size_t nBytes, Iter_T iterStr, const bool bAppendNull)
{
	auto p = reinterpret_cast<const unsigned char*>(pBytes);
	for(size_t i = 0; i < nBytes; ++i, ++p)
	{
		unsigned char val = ((*p & 0xF0) >> 4);
		*iterStr = (val < 0xA) ? '0' + val : 'A' + (val - 10);
		++iterStr;
		val = *p & 0xF;
		*iterStr = (val < 0xA) ? '0' + val : 'A' + (val - 10);
		++iterStr;
	}
	if (bAppendNull)
		*iterStr = 0;
}

// Overloads that take string as parameter and handles its correct resizing.
// TODO: test
template <class Char_T>
inline std::basic_string<Char_T>& bytesToHexStr(ConstVoidPtr pBytes, const size_t nBytes, std::basic_string<Char_T>& str)
{
	str.resize(nBytes*2);
	bytesToHexStr(pBytes, nBytes, str.begin(), false);
	return str;
}

// Converts arrays of bytes to to hex char string.
// [in] pBytes : Pointer to byte array.
// [in] nBytes : Number of bytes in array.
// Return: String containing the string.
// TODO: test
inline std::string bytesToHexStr(ConstVoidPtr pBytes, const size_t nBytes)
{
	std::string str;
	bytesToHexStr(pBytes, nBytes, str);
	return str;
}

// Inverse of bytesToHexStr. Does not check the validity of input.
// [in] psz : Pointer null terminated string containing the hex string.
// [out] pBuffer : Number of bytes in array.
// [in] nbSize : Number of bytes that pBuffer can hold at least.
// Return: pBuffer (pointer to bytes).
// TODO: test
inline void* hexStrToBytes(const ConstCharPtr psz, const VoidPtr pBuffer, const size_t nbSize)
{
	auto pBuf = reinterpret_cast<unsigned char*>(pBuffer);
	size_t nPos = 0;
	for(auto p = psz; *p != 0 && nPos < nbSize; p++, pBuf++)
	{
		*pBuf = (*p >= '0' && *p <= '9') ? (*p - '0') << 4 : (*p - 'A' + 10) << 4;
		p++; 
		if (*p == 0)
			break;
		*pBuf += (*p >= '0' && *p <= '9') ? (*p - '0') : (*p - 'A' + 10);
	}
	return pBuffer;
}

}} // module str
