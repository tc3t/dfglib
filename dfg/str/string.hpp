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
template <CharPtrType Type_T>
class DFG_CLASS_NAME(StringTyped)
{
public:
    typedef std::string StorageType;
    typedef TypedCharPtrT<const char, Type_T> TypedPtrT;
    typedef SzPtrT<const char, Type_T> SzPtrR;

    StringTyped() {}
    explicit StringTyped(SzPtrR psz) : m_s(psz.c_str()) {}
    SzPtrR c_str() const { return SzPtrR(m_s.c_str()); }

    void append(const TypedPtrT& first, const TypedPtrT& end)
    {
        m_s.append(first.rawPtr(), end.rawPtr());
    }

    bool empty() const
    {
        return m_s.empty();
    }

    size_t length() const { return m_s.length(); }
    size_t size() const { return m_s.size(); }

    bool operator==(const SzPtrR& psz) const
    {
        return (std::strcmp(toCharPtr_raw(c_str()), toCharPtr_raw(psz)) == 0);
    }

    bool operator!=(const SzPtrR& psz) const
    {
        return !(*this == psz);
    }

    void reserve(const size_t nCount)
    {
        m_s.reserve(nCount);
    }

    void resize(const size_t nNewSize)
    {
        m_s.resize(nNewSize, '\0');
    }

    StorageType& rawStorage() { return m_s; }
    const StorageType& rawStorage() const { return m_s; }

    StorageType m_s; // Encoded by the method defined by SzPtrType.
}; // class StringTyped

template<class SzPtr_T, CharPtrType Type_T>
inline bool operator==(const SzPtr_T& psz, const StringTyped<Type_T>& right)
{
    return right == psz;
}

typedef DFG_CLASS_NAME(StringTyped)<CharPtrTypeAscii> DFG_CLASS_NAME(StringAscii);
typedef DFG_CLASS_NAME(StringTyped)<CharPtrTypeLatin1> DFG_CLASS_NAME(StringLatin1);
typedef DFG_CLASS_NAME(StringTyped)<CharPtrTypeUtf8> DFG_CLASS_NAME(StringUtf8);

inline ConstCharPtr toSzPtr_raw(const DFG_CLASS_NAME(StringAscii)& str) { return str.c_str().c_str(); }
inline ConstCharPtr toSzPtr_raw(const DFG_CLASS_NAME(StringLatin1)& str) { return str.c_str().c_str(); }
inline ConstCharPtr toSzPtr_raw(const DFG_CLASS_NAME(StringUtf8)& str) { return str.c_str().c_str(); }

} // root namespace




