#pragma once

#include "../dfgBase.hpp"
#include "BasicIStreamCRTP.hpp"
#include "../readOnlyParamStr.hpp"
#include <cstdio>
#include "../typeTraits.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

// Reads bytes to buffer of given size from given binary stream.
// To get the number of bytes read, with std::istream call istrm.gcount() after calling this function.
// Note: Stream must be opened in binary mode.
template <class StrmT> StrmT& readBinary(StrmT& istrm, void* pBuf, const size_t nBufSize)
{
    // TODO: Check read mode status.
    // TODO: Check for size_t -> std::streamsize overflow.
    return static_cast<StrmT&>(istrm.read(reinterpret_cast<char*>(pBuf), static_cast<std::streamsize>(nBufSize)));
}

// Provided for convenience: reads bytes to object of type T from binary stream.
template <class Strm_T, class T>
Strm_T& readBinary(Strm_T& istrm, T& obj)
{
#ifdef _MSC_VER // TODO: Add proper check, this was added for GCC 4.8.1
    // is_pod was added for VC2013, where, unlike with VC2010 and VC2012, array of pods returned false for has_trivial_assign<T>.
    DFG_STATIC_ASSERT(DFG_MODULE_NS(TypeTraits)::IsTriviallyCopyAssignable<T>::value == true || std::is_pod<T>::value == true, "readBinary(Strm_T, T&) only accepts trivially assignable or pod T's");
#endif
    return readBinary(istrm, &obj, sizeof(obj));
}

} } // module io
