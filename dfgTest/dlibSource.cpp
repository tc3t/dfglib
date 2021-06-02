#include "stdafx.h"

#if (DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/dfgDefs.hpp>

#ifdef _MSC_VER
	#pragma warning(disable : 4505) // Needs to be silenced separately
#endif // _MSC_VER

#define DLIB_ISO_CPP_ONLY

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #if (DFGTEST_BUILD_OPT_USE_DLIB==1)
        #include <dlib/all/source.cpp>
    #endif
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

#endif
