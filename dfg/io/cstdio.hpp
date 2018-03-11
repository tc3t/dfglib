#pragma once

#include "../dfgDefs.hpp"
#include <stdio.h>
#include <cstdio>

// Note: using 'cstdio'-namespace instead of 'io' to make it less likely that 'using namespace io;' would cause ambiguity with global namespace definitions.
DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cstdio) {

    // Wrapper for std::fopen() avoiding 'This function or variable may be unsafe' warnings in MSVC.
    inline FILE* fopen(const char *filename, const char *mode)
    {
#ifdef _MSC_VER
        FILE* file;
        if (::fopen_s(&file, filename, mode) == 0)
            return file;
        else
            return nullptr;
#else
        if (!filename || !mode)
            return nullptr;
        return std::fopen(filename, mode);
#endif
    }

} } // module namespace
