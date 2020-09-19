cmake_minimum_required (VERSION 2.6)

project (dfgTest)

include_directories ("${PROJECT_SOURCE_DIR}/"
	"${PROJECT_SOURCE_DIR}/../externals/"
	"${PROJECT_SOURCE_DIR}/../")

# TODO: Set VC warning level to W4

add_definitions(-std=c++11) 

SET(HEADERS
    stdafx.h
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
    dfgTestCont.cpp
    dfgTestCont_MapSetVector.cpp
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
    dlibSource.cpp
    stdafx.cpp
   ../externals/gtest/gtest-all.cc
)

add_executable(dfgTest ${SOURCE} ${HEADERS})

if(UNIX)
    TARGET_LINK_LIBRARIES(dfgTest pthread)
endif()
