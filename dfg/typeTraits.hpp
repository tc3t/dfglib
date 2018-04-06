#pragma once

#include "dfgDefs.hpp"
#include <type_traits>
#include "build/languageFeatureInfo.hpp"
#include "preprocessor/compilerInfoMsvc.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(TypeTraits)
{
    // Helper for determining if given trait is true.
    // Problem with simple value-constant is that it might be unknown and it would be misleading to say either true or false.
    // For example if IsTriviallyCopyable<T> is not known, it should be neither true or false.
    template <class T>
    struct IsTrueTrait
    {
        enum { value = std::is_same<std::true_type, T>::value || std::is_base_of<std::true_type, T>::value };
    };

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
    struct IsTriviallyCopyable : public std::conditional<std::is_scalar<T>::value, std::true_type, UnknownAnswerType>::type { };
#endif
}} // Module namespace
