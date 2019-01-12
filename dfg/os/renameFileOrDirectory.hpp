#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include <cstdio>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os)
{
    // Returns 0 in success, otherwise non-zero (for details see documentation of std::rename())
    // TODO: test directory renaming.
    inline int renameFileOrDirectory_cstdio(const DFG_CLASS_NAME(StringViewSzC)& svFrom, const DFG_CLASS_NAME(StringViewSzC)& svTo) { return std::rename(svFrom.c_str(), svTo.c_str()); }
#ifdef _WIN32
    inline int renameFileOrDirectory_cstdio(const DFG_CLASS_NAME(StringViewSzW)& svFrom, const DFG_CLASS_NAME(StringViewSzW)& svTo) { return _wrename(svFrom.c_str(), svTo.c_str()); }
#endif

} } // module namespace
