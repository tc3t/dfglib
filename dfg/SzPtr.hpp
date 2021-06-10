#pragma once

#include "dfgDefs.hpp"
#include "dfgBaseTypedefs.hpp"
#include "SzPtrTypes.hpp"
#include <string>

DFG_ROOT_NS_BEGIN
{

inline ConstCharPtr     toCharPtr_raw(const std::string& str)       { return str.c_str(); }
inline ConstWCharPtr    toCharPtr_raw(const std::wstring& str)      { return str.c_str(); }

template <class T>
inline const T* toCharPtr_raw(const T* psz) { return psz; }

/////////////////
// toCharPtr_raw
template <class Char_T, CharPtrType Type_T>
inline auto toCharPtr_raw(const TypedCharPtrT<Char_T, Type_T>& tpsz) -> decltype(tpsz.rawPtr()) { return tpsz.rawPtr(); }

/////////////////
// toCharPtr_typed
template <class Char_T, CharPtrType Type_T>
inline auto toCharPtr_typed(const TypedCharPtrT<Char_T, Type_T>& tpsz) -> TypedCharPtrT<Char_T, Type_T> { return tpsz; }

/////////////////
// toSzPtr_typed
template <class Char_T, CharPtrType Type_T>
inline auto toSzPtr_typed(const SzPtrT<Char_T, Type_T>& tpsz) -> SzPtrT<Char_T, Type_T> { return tpsz; }

/////////////////
// toCharPtr
template <class T>
inline const T* toCharPtr(const T* psz) { return psz; }

template <class Char_T, CharPtrType Type_T>
TypedCharPtrT<Char_T, Type_T> toCharPtr(const TypedCharPtrT<Char_T, Type_T>& tp) { return tp; }

} // root namespace
