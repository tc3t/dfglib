#pragma once

#include "dfgDefs.hpp"
#include <type_traits>
#include "build/languageFeatureInfo.hpp"
#include "preprocessor/compilerInfoMsvc.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(TypeTraits)
{
    struct UnknownAnswerType {};

    struct ConstructibleFromAnyType { template <class T> ConstructibleFromAnyType(const T&) {} };

    // VC2010 does not have is_trivially_copy_assignable.
#if defined(_MSC_VER)
    template <class B_T>
#if DFG_MSVC_VER <= DFG_MSVC_VER_2010
    struct IsTriviallyCopyAssignable : public std::has_trivial_assign<B_T> {};
#else
    struct IsTriviallyCopyAssignable : public std::is_trivially_copy_assignable<B_T> {};
#endif
#endif // defined(_MSC_VER)

        template <class T>
    #if DFG_LANGFEAT_HAS_IS_TRIVIALLY_COPYABLE
        struct IsTriviallyCopyable : public std::is_trivially_copyable<T> { };
    #else
        struct IsTriviallyCopyable : public UnknownAnswerType { };
    #endif
}} // Module namespace
