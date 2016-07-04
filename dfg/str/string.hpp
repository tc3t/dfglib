#pragma once

#include "../dfgDefs.hpp"
#include "../SzPtr.hpp"
#include <cstring>
#include <string>


DFG_ROOT_NS_BEGIN { // Note: String is included in main namespace on purpose.

/*
template <class Char_T> class String_T : public std::basic_string<Char_T> {};

typedef String_T<char>		StringA;
typedef String_T<wchar_t>	StringW;
*/

typedef std::string		DFG_CLASS_NAME(StringA);
typedef std::wstring	DFG_CLASS_NAME(StringW);

// String whose content encoding is guaranteed in compile-time type.
template <SzPtrType Type_T>
class DFG_CLASS_NAME(StringTyped)
{
public:
    typedef std::string StorageType;
    typedef SzPtrT<const char, Type_T> SzPtrR;

    StringTyped() {}
    explicit StringTyped(SzPtrR psz) : m_s(psz.c_str()) {}
    SzPtrR c_str() const { return SzPtrR(m_s.c_str()); }

    size_t length() const { return m_s.length(); }

    bool operator==(const SzPtrR& psz) const
    {
        return (std::strcmp(toSzPtr_raw(c_str()), toSzPtr_raw(psz)) == 0);
    }

    bool operator!=(const SzPtrR& psz) const
    {
        return !(*this == psz);
    }

    StorageType& rawStorage() { return m_s; }
    const StorageType& rawStorage() const { return m_s; }

    StorageType m_s; // Encoded by the method defined by SzPtrType.
};

typedef DFG_CLASS_NAME(StringTyped)<SzPtrTypeAscii> DFG_CLASS_NAME(StringAscii);
typedef DFG_CLASS_NAME(StringTyped)<SzPtrTypeLatin1> DFG_CLASS_NAME(StringLatin1);
typedef DFG_CLASS_NAME(StringTyped)<SzPtrTypeUtf8> DFG_CLASS_NAME(StringUtf8);

} // root namespace




