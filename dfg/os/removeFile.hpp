#pragma once

#include "../dfgDefs.hpp"
#include <cstdio>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os)
{
    int removeFile(const char* psz) { return std::remove(psz); }
#ifdef _WIN32
    int removeFile(const wchar_t* psz) { return _wremove(psz); }
#endif

} } // module namespace
