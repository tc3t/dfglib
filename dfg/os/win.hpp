#pragma once

#include "../dfgDefs.hpp"
#include <string>
#include <Windows.h>
#include "../ptrToContiguousMemory.hpp"

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

inline DWORD GetTempPath(DWORD nSize, char* psz)
{
    return ::GetTempPathA(nSize, psz);
}

inline DWORD GetTempPath(DWORD nSize, wchar_t* psz)
{
    return ::GetTempPathW(nSize, psz);
}

template <class Char_T> inline std::basic_string<Char_T> GetTempPathT()
{
    const DWORD nBufferSize = MAX_PATH + 1;
    std::basic_string<Char_T> s(nBufferSize, '\0');
    DFG_STATIC_ASSERT(IsContiguousRange<std::basic_string<Char_T>>::value == true, "Expecting std::basic_string<T> to have contiguous storage.");
    const auto nSize = GetTempPath(nBufferSize, &s[0]);
    s.resize(nSize);
    return s;
}

inline std::string GetTempPathA()
{
    return GetTempPathT<char>();
}

inline std::wstring GetTempPathW()
{
    return GetTempPathT<wchar_t>();
}

}}} // module os::win
