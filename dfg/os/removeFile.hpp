#pragma once

#include "../dfgDefs.hpp"
#include <cstdio>

#ifndef _WIN32
    #include "../io/widePathStrToFstreamFriendlyNonWide.hpp"
#endif

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os)
{
    // Returns 0 in success, otherwise non-zero (for details see documentation of std::remove())
    inline int removeFile(const char* psz) { return std::remove(psz); }
#ifdef _WIN32
    inline int removeFile(const wchar_t* psz) { return _wremove(psz); }
#else
    inline int removeFile(const wchar_t* psz) { return removeFile(DFG_MODULE_NS(io)::pathStrToFileApiFriendlyPath(psz).c_str()); }
#endif

} } // module namespace
