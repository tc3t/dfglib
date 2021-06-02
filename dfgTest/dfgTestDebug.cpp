#include "stdafx.h"

#if (DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/debugAll.hpp>
#include <dfg/dfgAssert.hpp>
#include <dfg/thread/setThreadName.hpp>

TEST(dfgDebug, AssertImplementation)
{
	const int vals[] = { 1, 2 };
	for (int i = 1; i < DFG_COUNTOF(vals); ++i)
	{
		// This failed in debug build in earlier version due to use of loop variable named i in the assert implementation.
		// Also affected DFG_VERIFY.
		DFG_ASSERT(vals[i] == 2);
#ifndef _DEBUG // Generated warning in debug-build; just disable it.
		int j = 0;
		DFG_VERIFY((j = vals[i]));
		EXPECT_EQ(2, j);
#endif

	}

	int dummy = 1;

	// Test for usage in if-else structure (failed in some versions due to additional ;)
	if (dummy == 2)
		DFG_ASSERT_IMPLEMENTED(false);
	else if (dummy == 1)
		DFG_ASSERT_UB(true);
	else
		DFG_ASSERT(false);


	// This can be used to verify that DFG_ASSERT doesn't evaluate expression more than once if assert fails.
	{
		/*
		int val = 0;
		DFG_ASSERT(++val < 1);
		EXPECT_EQ(val, 1);
		*/
	}

	// Check evaluation counts.
	{
		int val = 0;
		DFG_ASSERT(++val);
#ifdef _DEBUG
		const auto expected = 1;
#else
		const auto expected = 0;
#endif
		EXPECT_EQ(expected, val);
		DFG_VERIFY(++val);
		EXPECT_EQ(expected + 1, val);
	}

	int i = 0; // Use variable to avoid "conditional expression is constant" warning

	if (i)
		;
	else
		DFG_ASSERT(true); // Intented to test potential syntax error.
}


TEST(dfgDebug, printToDebugDisplay)
{
	using namespace DFG_MODULE_NS(debug);
#ifdef _MSC_VER
	const char szTextC[] = "Test of printToDebugDisplay(char). Should appear in output window in Visual studio";
	const wchar_t szTextW[] = L"Test of printToDebugDisplay(wchar_t). Should appear in output window in Visual studio";
	printToDebugDisplay(szTextC);
	printToDebugDisplay(std::string(szTextC));
	printToDebugDisplay(szTextW);
	printToDebugDisplay(std::wstring(szTextW));
#endif
}

TEST(dfgDebug, setThreadName)
{
#ifdef _MSC_VER
    DFG_MODULE_NS(thread)::setThreadName("NewThreadName");
#endif
}

TEST(dfgDebug, require)
{
	using namespace DFG_MODULE_NS(debug);
	int s = 0;
	EXPECT_THROW(DFG_REQUIRE(++s == 2), DFG_CLASS_NAME(ExceptionRequire));
	EXPECT_NO_THROW(DFG_REQUIRE(++s == 2));
}

#endif

