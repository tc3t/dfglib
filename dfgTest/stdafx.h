// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <dfg/buildConfig.hpp>

#include "targetver.h"

#include <cstdio>
#include <iostream>

#ifdef _WIN32
    #include <tchar.h>
#endif

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include "../externals/gtest/gtest.h"
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

// Older dlib won't build on VC2019 std:C++17 with conformance mode so simply exclude dlib when building with VC2019 
#if defined(_MSC_VER) && DFG_MSVC_VER >= DFG_MSVC_VER_2019_0
    #define DFGTEST_BUILD_OPT_USE_DLIB  0
#else
    #define DFGTEST_BUILD_OPT_USE_DLIB  0
#endif // dlib

#define DFGTEST_STATIC_TEST(expr)	DFG_STATIC_ASSERT(expr, "Static test case failed")
#define DFGTEST_STATIC(expr)        DFGTEST_STATIC_TEST(expr)  //DFG_STATIC is deprecated, use DFGTEST_STATIC_TEST
#define DFGTEST_MESSAGE(expr)       std::cout << "    MESSAGE: " << expr << '\n';

#define DFGTEST_EXPECT_WITHIN(val, lower, upper) \
    { \
    const auto dfgTestTemporaryValue = val; \
    EXPECT_GE(dfgTestTemporaryValue, lower); \
    EXPECT_LE(dfgTestTemporaryValue, upper); \
    }

// These expect that math header has been included; don't want to include in this file to avoid it getting included everywhere.
#define DFGTEST_EXPECT_NAN(val)     EXPECT_TRUE(::DFG_MODULE_NS(math)::isNan(val))
#define DFGTEST_EXPECT_NON_NAN(val) EXPECT_FALSE(::DFG_MODULE_NS(math)::isNan(val))

#if !defined(_DEBUG) && !defined(__MINGW32__)
    #define DFGTEST_ENABLE_BENCHMARKS	0
#else
    #define DFGTEST_ENABLE_BENCHMARKS	0
#endif
