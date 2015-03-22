#pragma once

#include "../dfgDefs.hpp"
#include <algorithm>
#include "../build/languageFeatureInfo.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(alg) {

namespace DFG_DETAIL_NS
{
	template <bool bMemcpy>
	struct ArrayCopyImpl {};

	template <> struct ArrayCopyImpl<true>
	{
		template <class T>
		static void copy(T* pDst, const T* pSrc, const size_t n) {memcpy(pDst, pSrc, sizeof(T) * n);}
	};

	template <> struct ArrayCopyImpl<false>
	{
		template <class T>
		static void copy(T* pDst, const T* pSrc, const size_t n) {std::copy(pSrc, pSrc + n, pDst);}
	};
} // namespace Impl


// Copies n elements from array pSrc to array pDst.
// If the source and destination arrays overlap, the behaviour is undefined.
// TODO: test
template <class T>
void arrayCopy(T* pDst, const T* pSrc, const size_t n)
{
#if DFG_LANGFEAT_TYPETRAITS_11
	DFG_DETAIL_NS::ArrayCopyImpl<std::has_trivial_assign<T>::value>::copy(pDst, pSrc, n);
#else
	DFG_DETAIL_NS::ArrayCopyImpl<false>::copy(pDst, pSrc, n);
#endif
}

// Overload for copying arrays of equal size.
// TODO: test
template <class T, size_t N>
void arrayCopy(T (&dst)[N], const T (&src)[N])
{
#if DFG_LANGFEAT_TYPETRAITS_11
	DFG_DETAIL_NS::ArrayCopyImpl<std::has_trivial_assign<T>::value>::copy(dst, src, N);
#else
	DFG_DETAIL_NS::ArrayCopyImpl<false>::copy(dst, src, N);
#endif
}

}} // module alg
