#pragma once

#include "dfgDefs.hpp"
#include <type_traits>
#include "preprocessor/compilerInfoMsvc.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(TypeTraits)
{
    // VC2010 does not have is_trivially_copy_assignable.
#if defined(_MSC_VER)
    template <class B_T>
#if DFG_MSVC_VER <= DFG_MSVC_VER_2010
    struct IsTriviallyCopyAssignable : public std::has_trivial_assign<B_T>
#else
    struct IsTriviallyCopyAssignable : public std::is_trivially_copy_assignable<B_T>
#endif
    {
    };
#endif // defined(_MSC_VER)
}} // Module namespace
