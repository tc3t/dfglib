#pragma once

#include "../dfgBase.hpp"
#include "../dfgassert.hpp"
#include "../ReadOnlySzParam.hpp"
#include <string>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(str) {

namespace DFG_DETAIL_NS
{
    const char gHexCharsUpperCase[] = "0123456789ABCDEF";
    inline unsigned char nibbleToHexChar(const unsigned char c)
    {
        DFG_ASSERT_UB(c < DFG_COUNTOF_SZ(gHexCharsUpperCase));
        return static_cast<unsigned char>(gHexCharsUpperCase[c]);
    }
} // namespace DFG_DETAIL_NS


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
		*iterStr = DFG_DETAIL_NS::nibbleToHexChar(val);
		++iterStr;
		val = *p & 0xF;
		*iterStr = DFG_DETAIL_NS::nibbleToHexChar(val);
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

// Returns true if char is valid hex char in the sense that hexStrToBytes() accepts it.
inline bool isValidHexChar(const unsigned char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// Returns true if sv is empty or valid hex sequence in the sense that hexStrToBytes() accepts it.
inline bool isValidHexStr(const DFG_CLASS_NAME(StringViewC)& sv)
{
    const auto nSize = sv.size();
    if (nSize % 2 == 1) // Valid hex string must have even size.
        return false;

    auto p = sv.data();
    for (size_t i = nSize; i != 0; ++p, --i)
    {
        if (!isValidHexChar(*p))
            return false;
    }
    return true;
}

// Inverse of bytesToHexStr. Does not check the validity of input and works with lower and upper case letters.
// [in] psz : Pointer to null terminated string containing the hex string as ASCII bytes.
// [out] pBuffer : Destination buffer.
// [in] nbSize : Number of bytes that pBuffer can hold at least.
// Return: pBuffer (pointer to written bytes).
// Behaviour on null terminated input that is not valid hex str: GIGO (Garbage In - Garbage Out, i.e. guaranteed to be free on UB, but output content is unspecified)
inline void* hexStrToBytes(const ConstCharPtr psz, const VoidPtr pBuffer, const size_t nbSize)
{
    DFG_ASSERT_CORRECTNESS(isValidHexStr(psz));
    const auto toNibble = [](unsigned char c) -> unsigned char
                        {
                            return ((c & 0x40) == 0) ? c & 0xF : (c & 7) + 9; // '(c & 0x40) == 0' means 'is a digit'? (i.e. assuming that input is valid hex string, character is a digit if and only if 0x40 bit is not set)
                        };

	auto pBuf = reinterpret_cast<unsigned char*>(pBuffer);
	size_t nPos = 0;
	for(auto p = psz; *p != '\0' && nPos < nbSize; p++, pBuf++, nPos++)
	{
		*pBuf = toNibble(*p) << 4;
		p++; 
		if (*p == '\0')
			break;
        *pBuf += toNibble(*p);
	}
	return pBuffer;
}

}} // module str
