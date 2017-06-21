#pragma once

// languageFeatureInfo.hpp
// Purpose: Define constants that can be used to do conditional compilation based on availability of different
// language features.
// For more robust solution, see
//    -<boost/config.hpp>
//    -http://www.boost.org/doc/libs/1_64_0/libs/config/doc/html/boost_config/boost_macro_reference.html
// Some shortcuts:
//    #include <boost/config.hpp>
//    #include <boost/config/compiler/gcc.hpp>
//    #include <boost/config/compiler/visualc.hpp>


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

// DFG_LANGFEAT_CHRONO_11
#if defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2012)
    #define DFG_LANGFEAT_CHRONO_11  0
#else
    #define DFG_LANGFEAT_CHRONO_11  1
#endif

// DFG_LANGFEAT_HAS_IS_TRIVIALLY_COPYABLE
#if (defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2012)) || defined(__MINGW32__) // TODO: make this less coarse especially for mingw.
    #define DFG_LANGFEAT_HAS_IS_TRIVIALLY_COPYABLE	0
#else
    #define DFG_LANGFEAT_HAS_IS_TRIVIALLY_COPYABLE	1
#endif

// DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT
#if defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2015)
    #define DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT 0
#else
    #define DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT 1
#endif

// DFG_LANGFEAT_U8_CHAR_LITERALS
#if (defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2015)) || (defined(__GNUC__) && __GNUC__ < 6)
    #define DFG_LANGFEAT_U8_CHAR_LITERALS 0
#else
    #define DFG_LANGFEAT_U8_CHAR_LITERALS 1
#endif

// DFG_LANGFEAT_UNICODE_STRING_LITERALS
#if (defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2015)) || (defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5)))
    #define DFG_LANGFEAT_UNICODE_STRING_LITERALS 0
#else
    #define DFG_LANGFEAT_UNICODE_STRING_LITERALS 1
#endif
