cmake_minimum_required (VERSION 2.6)

project (dfgTest)

# Example: line below can be used to set clang++ as compiler e.g. on Ubuntu
# SET(CMAKE_CXX_COMPILER    "/usr/bin/clang++")

include_directories ("${PROJECT_SOURCE_DIR}/"
	"${PROJECT_SOURCE_DIR}/../externals/"
	"${PROJECT_SOURCE_DIR}/../")

# TODO: Set VC warning level to W4

add_definitions(-std=c++17)

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
    dfgTestTime.cpp
    dfgTestUtf.cpp
    stdafx.cpp
   ../externals/gtest/gtest-all.cc
)

add_executable(dfgTest ${SOURCE} ${HEADERS})

if(MINGW)
    TARGET_LINK_LIBRARIES(dfgTest shlwapi)
endif()

if(UNIX)
    TARGET_LINK_LIBRARIES(dfgTest pthread)
endif()
