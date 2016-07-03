#pragma once

#include "dfgDefs.hpp"
#include "SzPtrTypes.hpp"
#include <string>

DFG_ROOT_NS_BEGIN
{

inline ConstCharPtr toSzPtr_raw(const ConstCharPtr psz) { return psz; }
inline ConstWCharPtr toSzPtr_raw(const ConstWCharPtr psz) { return psz; }
inline ConstCharPtr toSzPtr_raw(const std::string& str) { return str.c_str(); }
inline ConstWCharPtr toSzPtr_raw(const std::wstring& str) { return str.c_str(); }

inline ConstCharPtr toSzPtr_raw(const SzPtrAsciiR tpsz) { return tpsz.c_str(); }
inline ConstCharPtr toSzPtr_raw(const SzPtrLatin1R tpsz) { return tpsz.c_str(); }
inline ConstCharPtr toSzPtr_raw(const SzPtrUtf8R tpsz) { return tpsz.c_str(); }

inline SzPtrAsciiR toSzPtr_typed(const SzPtrAsciiR tpsz) { return tpsz; }
inline SzPtrLatin1R toSzPtr_typed(const SzPtrLatin1R tpsz) { return tpsz; }
inline SzPtrUtf8R toSzPtr_typed(const SzPtrUtf8R tpsz) { return tpsz; }

} // root namespace
