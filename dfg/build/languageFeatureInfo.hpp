#pragma once

/*
 languageFeatureInfo.hpp
 Purpose: Define constants that can be used to do conditional compilation based on availability of different
 language features.
 For more robust solution, see
    -<boost/config.hpp>
    -http://www.boost.org/doc/libs/1_64_0/libs/config/doc/html/boost_config/boost_macro_reference.html
Some shortcuts:
    #include <boost/config.hpp>
    #include <boost/config/compiler/gcc.hpp>
    #include <boost/config/compiler/visualc.hpp>
Notes:
    -__GNUG__ is defined in GCC, MINGW and Clang
        -Version can be accessed through __GNUC__  __GNUC_MINOR__  __GNUC_PATCHLEVEL__ (e.g. 4.8.0)
    -__clang__ is defined in Clang, version through __clang_major__  __clang_minor__  __clang_patchlevel__
*/

#include "../preprocessor/compilerInfoMsvc.hpp"

// https://gcc.gnu.org/gcc-5/changes.html#libstdcxx, https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54316
#if (defined(__GNUG__) && (__GNUC__ < 5) && !defined(__clang__))
    #define DFG_LANGFEAT_MOVABLE_STREAMS	0
#else
    #define DFG_LANGFEAT_MOVABLE_STREAMS	1
#endif

#if defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2013)
    #define DFG_LANGFEAT_HAS_ISNAN	0
#else
    #define DFG_LANGFEAT_HAS_ISNAN	1
#endif

#if defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2010)
    #define DFG_LANGFEAT_TYPETRAITS_11  0
#else
    #define DFG_LANGFEAT_TYPETRAITS_11  1
#endif

#if defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2012)
    #define DFG_LANGFEAT_MUTEX_11  0
#else
    #define DFG_LANGFEAT_MUTEX_11  1
#endif

#if (defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2013))
    #define DFG_LANGFEAT_EXPLICIT_OPERATOR_BOOL  0
    #define DFG_EXPLICIT_OPERATOR_BOOL_IF_SUPPORTED
#else // According to cppreference.com 'Explicit conversion operators' is in GCC 4.5.
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
// GCC: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64195
#if (defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2012)) || (defined(__GNUG__) && (__GNUC__ < 5))
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
#if (defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2015)) || (defined(__GNUC__) && (__GNUC__ < 6)) || (defined(__GNUC__) && (!defined(__cpp_unicode_characters) || __cpp_unicode_characters < 201411))
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

// DFG_LANGFEAT_CONSTEXPR
#if (defined(_MSC_VER) && (_MSC_VER < DFG_MSVC_VER_2015)) || (defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6)))
    #define DFG_LANGFEAT_CONSTEXPR 0
    #define DFG_CONSTEXPR
#else
    #define DFG_LANGFEAT_CONSTEXPR  1
    #define DFG_CONSTEXPR           constexpr
#endif

// DFG_OVERRIDE_DESTRUCTOR
#if (defined(_MSC_VER) && (_MSC_VER <= DFG_MSVC_VER_2010))
	// Using override on destructor generates a compiler error in MSVC2010
	#define DFG_OVERRIDE_DESTRUCTOR
#else
	#define DFG_OVERRIDE_DESTRUCTOR override
#endif

// DFG_LANGFEAT_VECTOR_INSERT_ITERATOR_RETURN (whether vector::insert(pos, iterBegin, iterEnd) returns void or iterator)
#if (defined(_MSC_VER) && (_MSC_VER <= DFG_MSVC_VER_2010)) || (defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <= 8)))
    #define DFG_LANGFEAT_VECTOR_INSERT_ITERATOR_RETURN 0
#else
    #define DFG_LANGFEAT_VECTOR_INSERT_ITERATOR_RETURN 1
#endif

// DFG_NOEXCEPT
#if (!defined(_MSC_VER) || (_MSC_VER >= DFG_MSVC_VER_2015))
    #define DFG_LANGFEAT_NOEXCEPT   1
    #define DFG_NOEXCEPT(expr) noexcept(expr)
    #define DFG_NOEXCEPT_TRUE noexcept
#else
    #define DFG_LANGFEAT_NOEXCEPT   0
    #define DFG_NOEXCEPT(expr)
    #define DFG_NOEXCEPT_TRUE throw()
#endif

// DFG_LANGFEAT_HAS_DEFAULTED_AND_DELETED_FUNCTIONS (i.e. availability of '= default' and '= delete')
#if (!defined(_MSC_VER) || (_MSC_VER >= DFG_MSVC_VER_2013))
    #define DFG_LANGFEAT_HAS_DEFAULTED_AND_DELETED_FUNCTIONS 1
#else
    #define DFG_LANGFEAT_HAS_DEFAULTED_AND_DELETED_FUNCTIONS 0
#endif

// DFG_MAYBE_UNUSED (https://en.cppreference.com/w/cpp/language/attributes/maybe_unused)
#if (DFG_CPLUSPLUS >= DFG_CPLUSPLUS_17)
    #define DFG_MAYBE_UNUSED [[maybe_unused]]
#else
    #define DFG_MAYBE_UNUSED
#endif

// DFG_NODISCARD (https://en.cppreference.com/w/cpp/language/attributes/nodiscard)
#if (DFG_CPLUSPLUS >= DFG_CPLUSPLUS_17)
    #define DFG_NODISCARD [[nodiscard]]
    #if (DFG_CPLUSPLUS >= DFG_CPLUSPLUS_20)
        #define DFG_NODISCARD_MSG(MSG) [[nodiscard(MSG)]]
    #else
        #define DFG_NODISCARD_MSG(MSG) [[nodiscard]]
    #endif
#else
    #define DFG_NODISCARD
    #define DFG_NODISCARD_MSG(MSG)
#endif
//

