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

#include "dfgTest.hpp"

#if !defined(_DEBUG) && !defined(__MINGW32__)
    #define DFGTEST_ENABLE_BENCHMARKS	0
#else
    #define DFGTEST_ENABLE_BENCHMARKS	0
#endif

// Flags for controlling what gets included in the build.

// Default value to use if build flag is not set explicitly 
#define DFGTEST_BUILD_MODULE_DEFAULT                  1

// Per-module build flags taking precedence over default value if defined.
//#define DFGTEST_BUILD_MODULE_CONT                         1
//#define DFGTEST_BUILD_MODULE_BASE                         1
//#define DFGTEST_BUILD_MODULE_CONT_MAPSETVECTOR            1
//#define DFGTEST_BUILD_MODULE_CONT_TABLES                  1
#define DFGTEST_BUILD_MODULE_CSV_BENCHMARK                0
//#define DFGTEST_BUILD_MODULE_IO                           1
//#define DFGTEST_BUILD_MODULE_IO_DELIM_READER              1
//#define DFGTEST_BUILD_MODULE_MATH                         1
//#define DFGTEST_BUILD_MODULE_OS                           1
//#define DFGTEST_BUILD_MODULE_STR                          1
// 
//#define DFGTEST_BUILD_MODULE_
