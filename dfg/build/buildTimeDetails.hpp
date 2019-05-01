#pragma once

/*
    This file defines interfaces to access information available at build time.

    -Available items are defined in BuildTimeDetail
    -To access values:
        -Single value: getBuildTimeDetailStr<id>()
        -All values: getBuildTimeDetailStrs(func); (func is callable that is given (BuildTimeDetail, NonNullCStr))
    -Note: If detail is not available, getBuildTimeDetailStr() returns empty string (e.g. querying qtVersion when Qt is not available).
    -Note: All returned NonNullCStr-items have static lifetime so they can be stored and must not be deallocated.

    Related code:
        -Boost.Predef
        -QSysInfo
*/

#include "../dfgDefs.hpp"
#include "../dfgBaseTypedefs.hpp"
#include "compilerDetails.hpp"

#if (DFG_BUILD_OPT_USE_BOOST == 1)
    #include <boost/version.hpp>
    #include <boost/predef.h>
#endif

DFG_ROOT_NS_BEGIN
{

enum BuildTimeDetail
{
    BuildTimeDetail_dateTime,                // Build time as date time in unspecified format.
    BuildTimeDetail_compilerAndShortVersion, // Compiler
    BuildTimeDetail_compilerFullVersion,     // Full compiler version
    BuildTimeDetail_buildDebugReleaseType,   // Debug/release type. TODO: more precise definition especially on Non-Windows
    BuildTimeDetail_architecture,            // Build architecture
    BuildTimeDetail_endianness,              // Architecture endianess
    BuildTimeDetail_boostVersion,            // Boost version
    BuildTimeDetail_qtVersion                // Qt (build time) version
};

namespace DFG_DETAIL_NS
{
    const char* buildTimeDetailStrs[] =
    {
        "Build date",
        "Compiler",
        "Compiler (full version)",
        "Debug/release type",
        "Architecture",
        "Endianness",
        "Boost version",
        "Qt version (build time)"
    };
} // namespace DFG_DETAIL_NS

const char* buildTimeDetailIdToStr(const BuildTimeDetail d)
{
    const auto i = static_cast<int>(d);
    return (i >= 0 && static_cast<size_t>(i) < DFG_COUNTOF(DFG_DETAIL_NS::buildTimeDetailStrs)) ? DFG_DETAIL_NS::buildTimeDetailStrs[i] : "";
}

template <BuildTimeDetail detail> NonNullCStr getBuildTimeDetailStr();

#define DFG_TEMP_DEFINE_BUILD_TIME_DETAIL_FUNC(DET) template <> NonNullCStr getBuildTimeDetailStr<BuildTimeDetail_##DET>()

DFG_TEMP_DEFINE_BUILD_TIME_DETAIL_FUNC(dateTime)                { return __DATE__ " " __TIME__; }
DFG_TEMP_DEFINE_BUILD_TIME_DETAIL_FUNC(compilerAndShortVersion) { return DFG_COMPILER_NAME_SIMPLE; }
DFG_TEMP_DEFINE_BUILD_TIME_DETAIL_FUNC(compilerFullVersion)     { return DFG_COMPILER_FULL_VERSION; }
DFG_TEMP_DEFINE_BUILD_TIME_DETAIL_FUNC(buildDebugReleaseType)   { return DFG_BUILD_DEBUG_RELEASE_TYPE; }
DFG_TEMP_DEFINE_BUILD_TIME_DETAIL_FUNC(boostVersion)
{
#ifdef BOOST_LIB_VERSION
    return BOOST_LIB_VERSION;
#else
    return "";
#endif
}

DFG_TEMP_DEFINE_BUILD_TIME_DETAIL_FUNC(qtVersion)
{
#ifdef QT_VERSION_STR
    return QT_VERSION_STR;
#else
    return "";
#endif
}

DFG_TEMP_DEFINE_BUILD_TIME_DETAIL_FUNC(architecture)
{
#if (DFG_BUILD_OPT_USE_BOOST == 1)
    #if (BOOST_ARCH_X86_64 != BOOST_VERSION_NUMBER_NOT_AVAILABLE)
        return BOOST_ARCH_X86_64_NAME;
    #elif (BOOST_ARCH_X86 != BOOST_VERSION_NUMBER_NOT_AVAILABLE)
        return BOOST_ARCH_X86_NAME;
    #else
        return "";
    #endif
#else
    return "";
#endif
}

DFG_TEMP_DEFINE_BUILD_TIME_DETAIL_FUNC(endianness)
{
#if (DFG_BUILD_OPT_USE_BOOST == 1)
    #if (BOOST_ENDIAN_BIG_BYTE != BOOST_VERSION_NUMBER_NOT_AVAILABLE)
        return BOOST_ENDIAN_BIG_BYTE_NAME;
    #elif (BOOST_ENDIAN_BIG_WORD != BOOST_VERSION_NUMBER_NOT_AVAILABLE)
        return BOOST_ENDIAN_BIG_WORD_NAME;
    #elif (BOOST_ENDIAN_LITTLE_BYTE != BOOST_VERSION_NUMBER_NOT_AVAILABLE)
        return BOOST_ENDIAN_LITTLE_BYTE_NAME;
    #elif (BOOST_ENDIAN_LITTLE_WORD != BOOST_VERSION_NUMBER_NOT_AVAILABLE)
        return BOOST_ENDIAN_LITTLE_WORD_NAME;
    #else
        return "";
    #endif
#else
    return "";
#endif
}

#undef DFG_TEMP_DEFINE_BUILD_TIME_DETAIL_FUNC

template <class Func>
void getBuildTimeDetailStrs(Func&& func)
{
#define DFG_TEMP_DO_FOR_BUILD_DETAIL(DET) func(BuildTimeDetail_##DET, getBuildTimeDetailStr<BuildTimeDetail_##DET>())
    DFG_TEMP_DO_FOR_BUILD_DETAIL(dateTime);
    DFG_TEMP_DO_FOR_BUILD_DETAIL(compilerAndShortVersion);
    DFG_TEMP_DO_FOR_BUILD_DETAIL(compilerFullVersion);
    DFG_TEMP_DO_FOR_BUILD_DETAIL(buildDebugReleaseType);
    DFG_TEMP_DO_FOR_BUILD_DETAIL(architecture);
    DFG_TEMP_DO_FOR_BUILD_DETAIL(endianness);
    DFG_TEMP_DO_FOR_BUILD_DETAIL(boostVersion);
    DFG_TEMP_DO_FOR_BUILD_DETAIL(qtVersion);
#undef DFG_TEMP_DO_FOR_BUILD_DETAIL
}

} // Module namespace
