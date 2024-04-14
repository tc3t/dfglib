cmake_minimum_required (VERSION 3.8) # 3.8 includes support for C++17 in CXX_STANDARD (https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html#prop_tgt:CXX_STANDARD)

project (dfgTest)

# Example: line below can be used to set clang++ as compiler e.g. on Ubuntu
# SET(CMAKE_CXX_COMPILER    "/usr/bin/clang++")

# Example: using libc++ as standard library e.g. on Ubuntu
#     May need to install these first: sudo apt install libc++-dev libc++abi-dev
# add_compile_options("-stdlib=libc++")
#     https://stackoverflow.com/questions/51904938/linker-error-with-cmake-and-libcxx
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")

include_directories ("${PROJECT_SOURCE_DIR}/"
	"${PROJECT_SOURCE_DIR}/../externals/"
	"${PROJECT_SOURCE_DIR}/../")

# TODO: Set VC warning level to W4

# Note: if changing the C++ standard requirement, see if also
#       dfg/qt/qmakeTools/dfgQmakeUtil.pri
#       and settings in .vcxproj-files need to be changed.
set(CMAKE_CXX_STANDARD 17)          # Sets c++17 as minimum standard requirement
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Prevents 'decaying' to earlier standard if requested standard is not available
set(CMAKE_CXX_EXTENSIONS OFF)       # Prevents using e.g. -std=gnu++17 instead of -std=c++17
if(MSVC)
    # CMAKE_VERBOSE_MAKEFILE doesn't work on MSVC and didn't seem to have similar alternative as of 2023-03, related links:
    #   https://stackoverflow.com/questions/71353348/how-to-perform-verbose-builds-of-a-cmake-project-with-visual-studio-2019-generat
    #   https://stackoverflow.com/questions/57168248/show-compiler-command-line-of-cmake-with-visual-studio-2019
else()
    set(CMAKE_VERBOSE_MAKEFILE ON)      # Sets more verbose compiler output during build
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    message("CMAKE_BUILD_TYPE not defined, defaulting to 'Debug'")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# In debug-builds, using throwing asserts to be able to test expected assert failures.
message("CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("    Debug-build -> adding definition DFG_DEBUG_ASSERT_WITH_THROW=1")
    add_definitions(-DDFG_DEBUG_ASSERT_WITH_THROW=1)
endif()

SET(HEADERS
    stdafx.h
    dfgTest.hpp
)

set(SOURCE
    ${SOURCE}
    ../dfg/dfgCppAll.cpp
    dfgTest.cpp
    dfgTestAlg.cpp
    dfgTestBase.cpp
    dfgTestBits.cpp
    dfgTestCharts.cpp
    dfgTestColour.cpp
    dfgTestConcurrency.cpp
    dfgTestCont.cpp
    dfgTestCont_MapSetVector.cpp
    dfgTestCont_TableCsv.cpp
    dfgTestCont_tables.cpp
    dfgTestContMapVectorPerformance.cpp
    dfgTestCrypt.cpp
    dfgTestCsvPerformance.cpp
    dfgTestDataAnalysis.cpp
    dfgTestDebug.cpp
    dfgTestFunc.cpp
    dfgTestGuid.cpp
    dfgTestHash.cpp
    dfgTestIncludeAll.cpp
    dfgTestIo.cpp
    dfgTestIoDelimitedTextReader.cpp
    dfgTestIoDelimitedTextWriter.cpp
    dfgTestIter.cpp
    dfgTestMath.cpp
    dfgTestMemory.cpp
    dfgTestMisc.cpp
    dfgTestNumeric.cpp
    dfgTestOs.cpp
    dfgTestPhysics.cpp
    dfgTestRand.cpp
    dfgTestStr.cpp
    dfgTestStr_StringViews.cpp
    dfgTestTime.cpp
    dfgTestUtf.cpp
    stdafx.cpp
   ../externals/gtest/gtest-all.cc
)

add_executable(dfgTestCmake ${SOURCE} ${HEADERS})

if(MINGW)
    TARGET_LINK_LIBRARIES(dfgTestCmake shlwapi)
endif()

if(UNIX)
    TARGET_LINK_LIBRARIES(dfgTestCmake pthread)
endif()
