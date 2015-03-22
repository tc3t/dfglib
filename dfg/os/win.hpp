#pragma once

#include "../dfgDefs.hpp"
#include <string>
#include <Windows.h>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(os) { DFG_SUB_NS(win) {

// Returns error message corresponding to error code returned by GetLastError().
// TODO: test
inline std::string getErrorMessageA(const DWORD nErrorCode)
{
	const size_t nBufferSize = 256;
	char buffer[nBufferSize];
	FormatMessageA(  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					nErrorCode,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					buffer,
					nBufferSize,
					NULL );

	return buffer;
}

// TODO: test
inline std::string getErrorMessageA()
{
	const auto err = GetLastError();
	return getErrorMessageA(err);
}

}}} // module os::win
