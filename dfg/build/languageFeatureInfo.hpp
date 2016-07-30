#pragma once

// languageFeatureInfo.hpp
// Purpose: Define constants that can be used to do conditional compilation based on availability of different
// language features.

#include "../preprocessor/compilerInfoMsvc.hpp"

#ifndef __MINGW32__ // TODO: make this less coarse.
    #define DFG_LANGFEAT_MOVABLE_STREAMS	1
#else
    #define DFG_LANGFEAT_MOVABLE_STREAMS	0
#endif

#if defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2013)
    #define DFG_LANGFEAT_HAS_ISNAN	0
#else
    #define DFG_LANGFEAT_HAS_ISNAN	1
#endif

#if defined(_MSC_VER) && (_MSC_VER >= DFG_MSVC_VER_2010)
    #define DFG_LANGFEAT_TYPETRAITS_11  1
#else
    #define DFG_LANGFEAT_TYPETRAITS_11  0
#endif

#if defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2012)
    #define DFG_LANGFEAT_MUTEX_11  0
#else
    #define DFG_LANGFEAT_MUTEX_11  1
#endif

#if (defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2013)) || defined(__MINGW32__) // TODO: make MinGW32 test less coarse.
    #define DFG_LANGFEAT_EXPLICIT_OPERATOR_BOOL  0
    #define DFG_EXPLICIT_OPERATOR_BOOL_IF_SUPPORTED
#else
    #define DFG_LANGFEAT_EXPLICIT_OPERATOR_BOOL  1
    #define DFG_EXPLICIT_OPERATOR_BOOL_IF_SUPPORTED explicit
#endif

#if defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2012)
    #define DFG_LANGFEAT_CHRONO_11  0
#else
    #define DFG_LANGFEAT_CHRONO_11  1
#endif

