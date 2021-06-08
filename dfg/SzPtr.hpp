#pragma once

#include "dfgDefs.hpp"
#include "dfgBaseTypedefs.hpp"
#include "SzPtrTypes.hpp"
#include <string>

DFG_ROOT_NS_BEGIN
{

inline ConstCharPtr     toCharPtr_raw(const ConstCharPtr psz)       { return psz; }
inline ConstWCharPtr    toCharPtr_raw(const ConstWCharPtr psz)      { return psz; }
inline ConstCharPtr     toCharPtr_raw(const std::string& str)       { return str.c_str(); }
inline ConstWCharPtr    toCharPtr_raw(const std::wstring& str)      { return str.c_str(); }

inline CharPtr toCharPtr_raw(const TypedCharPtrAsciiW& tpsz)        { return tpsz.rawPtr(); }
inline CharPtr toCharPtr_raw(const TypedCharPtrLatin1W& tpsz)       { return tpsz.rawPtr(); }
inline CharPtr toCharPtr_raw(const TypedCharPtrUtf8W& tpsz)         { return tpsz.rawPtr(); }
inline auto toCharPtr_raw(const TypedCharPtrUtf16W& tpsz) -> decltype(tpsz.rawPtr()) { return tpsz.rawPtr(); }

inline ConstCharPtr toCharPtr_raw(const TypedCharPtrAsciiR& tpsz)   { return tpsz.rawPtr(); }
inline ConstCharPtr toCharPtr_raw(const TypedCharPtrLatin1R& tpsz)  { return tpsz.rawPtr(); }
inline ConstCharPtr toCharPtr_raw(const TypedCharPtrUtf8R& tpsz)    { return tpsz.rawPtr(); }
inline auto toCharPtr_raw(const TypedCharPtrUtf16R& tpsz) -> decltype(tpsz.rawPtr()) { return tpsz.rawPtr(); }

inline TypedCharPtrAsciiR   toCharPtr_typed(const TypedCharPtrAsciiR& tpsz)  { return tpsz; }
inline TypedCharPtrLatin1R  toCharPtr_typed(const TypedCharPtrLatin1R& tpsz) { return tpsz; }
inline TypedCharPtrUtf8R    toCharPtr_typed(const TypedCharPtrUtf8R& tpsz)   { return tpsz; }
inline TypedCharPtrUtf16R   toCharPtr_typed(const TypedCharPtrUtf16R& tpsz)  { return tpsz; }

inline SzPtrAsciiW  toSzPtr_typed(const SzPtrAsciiW& tpsz)          { return tpsz; }
inline SzPtrLatin1W toSzPtr_typed(const SzPtrLatin1W& tpsz)         { return tpsz; }
inline SzPtrUtf8W   toSzPtr_typed(const SzPtrUtf8W& tpsz)           { return tpsz; }
inline SzPtrUtf16W  toSzPtr_typed(const SzPtrUtf16W& tpsz)          { return tpsz; }

inline SzPtrAsciiR  toSzPtr_typed(const SzPtrAsciiR& tpsz)          { return tpsz; }
inline SzPtrLatin1R toSzPtr_typed(const SzPtrLatin1R& tpsz)         { return tpsz; }
inline SzPtrUtf8R   toSzPtr_typed(const SzPtrUtf8R& tpsz)           { return tpsz; }
inline SzPtrUtf16R  toSzPtr_typed(const SzPtrUtf16R& tpsz)          { return tpsz; }

inline ConstCharPtr    toCharPtr(const ConstCharPtr psz)            { return psz; }
inline ConstWCharPtr   toCharPtr(const ConstWCharPtr psz)           { return psz; }
inline const char16_t* toCharPtr(const char16_t* psz)               { return psz; }

template <class Char_T, CharPtrType Type_T>
TypedCharPtrT<Char_T, Type_T> toCharPtr(const TypedCharPtrT<Char_T, Type_T>& tp) { return tp; }

} // root namespace
