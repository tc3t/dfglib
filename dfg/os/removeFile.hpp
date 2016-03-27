#pragma once

#include "../dfgDefs.hpp"
#include <cstdio>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os)
{
    inline int removeFile(const char* psz) { return std::remove(psz); }
#ifdef _WIN32
    inline int removeFile(const wchar_t* psz) { return _wremove(psz); }
#endif

} } // module namespace
