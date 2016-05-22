#pragma once

#include "dfgDefs.hpp"
#include <stdint.h>

DFG_ROOT_NS_BEGIN
{

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

/*
#ifndef _WIN32
	typedef int BOOL;
	typedef unsigned char BYTE;
	typedef uint16_t WORD;
	typedef uint32_t DWORD;
#endif // _WIN32
*/

const int8  int8_min	= INT8_MIN;  // -128
const int16 int16_min   = INT16_MIN; // -32768
const int32 int32_min   = INT32_MIN; // -2147483648
const int64 int64_min   = INT64_MIN; // -9223372036854775808

const int8  int8_max    = INT8_MAX;  // 127
const int16 int16_max   = INT16_MAX; // 32767
const int32 int32_max   = INT32_MAX; // 2147483647
const int64 int64_max   = INT64_MAX; // 9223372036854775807

const uint8  uint8_max  = UINT8_MAX;  // 255
const uint16 uint16_max = UINT16_MAX; // 65535
const uint32 uint32_max = UINT32_MAX; // 4294967295
const uint64 uint64_max = UINT64_MAX; // 18446744073709551615;

typedef char*			CharPtr;
typedef wchar_t*		WCharPtr;
typedef const char*		ConstCharPtr;
typedef const wchar_t*	ConstWCharPtr;
typedef void*			VoidPtr;
typedef const void*		ConstVoidPtr;
typedef CharPtr			NonNullStr;		// Pointer to non-null C string
typedef WCharPtr		NonNullStrW;	// Pointer to non-null wide char C string
typedef ConstCharPtr	NonNullCStr;	// Pointer to non-null const C string
typedef ConstWCharPtr	NonNullCStrW;	// Pointer to non-null const wide char C string

} // root namespace

#include "SzPtr.hpp"
