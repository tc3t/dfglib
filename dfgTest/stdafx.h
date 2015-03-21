// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <dfg/buildConfig.hpp>

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#pragma warning(push, 1)
    #include "../externals/gtest/gtest.h"
#pragma warning(pop)

#define DFGTEST_STATIC(expr)	DFG_STATIC_ASSERT(expr, "Static test case failed")

#if !defined(_DEBUG) && !defined(__MINGW32__)
    #define ENABLE_RUNTIME_COMPARISONS	0
#else
    #define ENABLE_RUNTIME_COMPARISONS	0
#endif
