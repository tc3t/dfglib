#pragma once

#include "../dfgDefs.hpp"
#include "../SzPtr.hpp"
#include <cstring>
#include <string>
#include "../build/languageFeatureInfo.hpp"


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
    typedef std::string                         StorageType;
    typedef TypedCharPtrT<const char, Type_T>   TypedPtrT;
    typedef SzPtrT<const char, Type_T>          SzPtrR;
    typedef SzPtrT<char, Type_T>                SzPtrW;
    typedef StorageType::size_type              size_type;

    DFG_CLASS_NAME(StringTyped)() {}
    explicit DFG_CLASS_NAME(StringTyped)(SzPtrR psz) : m_s(psz.c_str()) {}
    explicit DFG_CLASS_NAME(StringTyped)(TypedPtrT iterBegin, TypedPtrT iterEnd) : m_s(iterBegin.rawPtr(), iterEnd.rawPtr()) {}

#if (DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT == 0)
    DFG_CLASS_NAME(StringTyped)(DFG_CLASS_NAME(StringTyped)&& other)
    {
        operator=(std::move(other));
    }

    DFG_CLASS_NAME(StringTyped)& operator=(DFG_CLASS_NAME(StringTyped)&& other)
    {
        this->m_s = std::move(other.m_s);
        return *this;
    }
#endif // DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT

    // Precondition: sRaw must be correctly encoded.
    // TODO: test
    static DFG_CLASS_NAME(StringTyped) fromRawString(StorageType sRaw)
    {
        DFG_CLASS_NAME(StringTyped) s;
        s.m_s = std::move(sRaw);
        return s;
    }

    SzPtrR c_str() const { return SzPtrR(m_s.c_str()); }

    void append(const TypedPtrT& first, const TypedPtrT& end)
    {
        m_s.append(first.rawPtr(), end.rawPtr());
    }

    int compare(const size_type nThisStart, const size_t nThisCount, const TypedPtrT& psz, const size_type& nCount) const
    {
        return m_s.compare(nThisStart, nThisCount, toCharPtr_raw(psz), nCount);
    }

	// TODO: test
	DFG_CLASS_NAME(StringTyped)& operator=(const SzPtrR& psz)
	{
		rawStorage() = psz.c_str();
		return *this;
	}

    bool empty() const
    {
        return m_s.empty();
    }

    // Note: returns the number of char-elements, not the number of codepoints (for example if UTF8-string has a single non-ascii characters that needs two bytes, size() is 2 instead of 1).
    // TODO: Revise how this should behave. These are inherently ambiguous with encodings like UTF8 so avoid using these and evaluate if they should be removed from the interface.
    size_t length() const { return m_s.length(); }
    size_t size() const { return m_s.size(); }

    // Returns the size() of the raw storage string.
    size_t sizeInRawUnits() const { return m_s.size(); }

    //size_t sizeInCodePoints() const { TODO: implement, perhaps through a strLenCodepoints() or similar; }

    // TODO: implement using StringView
    bool operator==(const DFG_CLASS_NAME(StringTyped)& other) const
    {
        return (m_s.size() == other.m_s.size()) && m_s == other.m_s;
    }

    bool operator!=(const DFG_CLASS_NAME(StringTyped)& other) const
    {
        return !(*this == other);
    }

    // TODO: implement using StringView
    bool operator==(const SzPtrR& psz) const
    {
        return (std::strcmp(toCharPtr_raw(c_str()), toCharPtr_raw(psz)) == 0);
    }

    bool operator!=(const SzPtrR& psz) const
    {
        return !(*this == psz);
    }

    size_t capacity() const
    {
        return m_s.capacity();
    }

    void reserve(const size_t nCount)
    {
        m_s.reserve(nCount);
    }

    void resize(const size_t nNewSize)
    {
        m_s.resize(nNewSize, '\0');
    }

    // TODO: test
    void clear()
    {
        m_s.clear();
    }

	// TODO: test
    // TODO: implement using StringView
	bool operator<(const DFG_CLASS_NAME(StringTyped)& other) const
	{
		return rawStorage() < other.rawStorage();
	}

    // TODO: test
    // TODO: implement using StringView
    bool operator<(const SzPtrR& right) const
    {
        return std::strcmp(toCharPtr_raw(c_str()), toCharPtr_raw(right)) < 0;
    }

    // Returns reference to raw storage container (guarantees that the call is cheap O(1) operation)
    StorageType& rawStorage() { return m_s; }
    const StorageType& rawStorage() const { return m_s; }

    StorageType m_s; // Encoded by the method defined by SzPtrType.
}; // class StringTyped

template <CharPtrType Type_T>
inline bool operator==(const SzPtrT<const char, Type_T>& psz, const DFG_CLASS_NAME(StringTyped)<Type_T>& right)
{
    return right == psz;
}

template <CharPtrType Type_T>
inline bool operator<(const SzPtrT<const char, Type_T>& tpsz, const DFG_CLASS_NAME(StringTyped)<Type_T>& right)
{
    return right < tpsz;
}

typedef DFG_CLASS_NAME(StringTyped)<CharPtrTypeAscii> DFG_CLASS_NAME(StringAscii);
typedef DFG_CLASS_NAME(StringTyped)<CharPtrTypeLatin1> DFG_CLASS_NAME(StringLatin1);
typedef DFG_CLASS_NAME(StringTyped)<CharPtrTypeUtf8> DFG_CLASS_NAME(StringUtf8);

inline ConstCharPtr toSzPtr_raw(const DFG_CLASS_NAME(StringAscii)& str) { return str.c_str().c_str(); }
inline ConstCharPtr toSzPtr_raw(const DFG_CLASS_NAME(StringLatin1)& str) { return str.c_str().c_str(); }
inline ConstCharPtr toSzPtr_raw(const DFG_CLASS_NAME(StringUtf8)& str) { return str.c_str().c_str(); }

} // root namespace




