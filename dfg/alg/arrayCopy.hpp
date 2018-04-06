#pragma once

#include "../dfgDefs.hpp"
#include <algorithm>
#include "../build/languageFeatureInfo.hpp"
#include "../typeTraits.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(alg) {

namespace DFG_DETAIL_NS
{
    template <class T>
    struct IsArrayCopyMemCpy
    {
        enum { value = DFG_MODULE_NS(TypeTraits)::IsTrueTrait<DFG_MODULE_NS(TypeTraits)::IsTriviallyCopyable<T>>::value };
    };

	template <bool bMemcpy>
	struct ArrayCopyImpl {};

    // Note: In modern compilers/standard libraries this memcpy() version is probably useless or perhaps even pessimization compared to std::copy
    //       but for example in MSVC2013 using std::copy() for arrays of type
    //       struct SimpleStruct { int a; };
    //       does not use memmove() as SimpleStruct is not scalar-type.
    //       With arrayCopy() and SimpleStruct-arrays memcpy() is used as IsTriviallyCopyable is available in MSVC2013.
	template <> struct ArrayCopyImpl<true>
	{
		template <class T>
		static void copy(T* pDst, const T* pSrc, const size_t n) { memcpy(pDst, pSrc, sizeof(T) * n); }
	};

	template <> struct ArrayCopyImpl<false>
	{
		template <class T>
		static void copy(T* pDst, const T* pSrc, const size_t n) { std::copy(pSrc, pSrc + n, pDst); }
	};
} // namespace Impl


// Copies n elements from array pSrc to array pDst.
// If the source and destination arrays overlap, the behaviour is undefined.
template <class T>
void arrayCopy(T* pDst, const T* pSrc, const size_t n)
{
#if DFG_LANGFEAT_TYPETRAITS_11
    using namespace DFG_MODULE_NS(TypeTraits);
	DFG_DETAIL_NS::ArrayCopyImpl<DFG_DETAIL_NS::IsArrayCopyMemCpy<T>::value>::copy(pDst, pSrc, n);
#else
	DFG_DETAIL_NS::ArrayCopyImpl<false>::copy(pDst, pSrc, n);
#endif
}

// Overload for copying arrays of equal size.
template <class T, size_t N>
void arrayCopy(T (&dst)[N], const T (&src)[N])
{
    arrayCopy(dst, src, N);
}

}} // module alg
