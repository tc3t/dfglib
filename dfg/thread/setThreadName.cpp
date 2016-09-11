#pragma once

#include "setThreadName.hpp"

#ifdef _MSC_VER
#include <windows.h>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(thread)
{
    // https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
    namespace DFG_DETAIL_NS
    {
        const DWORD msVcExceptionId = 0x406D1388;
    #pragma pack(push,8)
        struct THREADNAME_INFO
        {
            DWORD dwType; // Must be 0x1000.
            LPCSTR szName; // Pointer to name (in user addr space).
            DWORD dwThreadID; // Thread ID (-1=caller thread).
            DWORD dwFlags; // Reserved for future use, must be zero.
        };
    #pragma pack(pop)
    } // detail namespace

} } // module namespace

void DFG_MODULE_NS(thread)::setThreadName(const unsigned long dwThreadID, const char* pszThreadName)
{
    DFG_DETAIL_NS::THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = pszThreadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try {
        RaiseException(DFG_DETAIL_NS::msVcExceptionId, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
#pragma warning(pop)
}

void DFG_MODULE_NS(thread)::setThreadName(const char* pszThreadName)
{
    setThreadName(GetCurrentThreadId(), pszThreadName);
}

#endif // _MSC_VER
