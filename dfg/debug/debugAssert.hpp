#pragma once

#include "../dfgDefs.hpp"

#ifdef _MSC_VER

#include <intrin.h>


DFG_ROOT_NS_BEGIN { DFG_SUB_NS(debug) {

// TODO: Test
inline void breakToDebuggerInDebugBuild(int expr)
{
	#ifdef DFG_BUILD_TYPE_DEBUG
		if (expr == 0) __debugbreak();
	#else
		DFG_UNUSED(expr);
	#endif
}

}} // module debug

#endif


