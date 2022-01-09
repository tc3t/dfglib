#ifndef DFG_MEMSETZEROSECURE_LGHYKMKD
#define DFG_MEMSETZEROSECURE_LGHYKMKD

#include "dfgBase.hpp"

#ifdef _WIN32

#if DFG_MSVC_VER > 0
#pragma warning(push, 1)	// Saves warning state and sets warning level 1 to try to minimize the number of warnings from Windows.h
#endif

#include <Windows.h>

#if DFG_MSVC_VER > 0
#pragma warning(pop)		// Restores the warning state.
#endif


DFG_ROOT_NS_BEGIN
{

inline void memsetZeroSecure(const VoidPtr pData, const size_t nSize)
{
	#if defined(_WIN32)
		SecureZeroMemory(pData, nSize);
	#else
		#error TODO: memsetZeroSecure implementation for non-Windows environment.
	#endif
}

template <class T>
inline void memsetZeroSecure(T& a)
{
	DFG_STATIC_ASSERT(std::is_trivially_copyable<T>::value == true && std::is_pointer<T>::value == false, "Won't memset pointers or types that are not trivially copyable");
	memsetZeroSecure(&a, sizeof(a));
}

} // module namespace

#endif

#endif
