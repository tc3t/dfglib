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

typedef std::string     StringA;
typedef std::wstring    StringW;

// String whose content encoding is guaranteed in compile-time type.
// Note, though, that it is not guaranteed at runtime that the content is validly encoded (i.e. one can construct StringUtf8 that has invalid UTF8)
template <CharPtrType Type_T>
class StringTyped
{
public:
    using CharType = typename CharPtrTypeToBaseCharType<Type_T>::type;
    using value_type = CharType;
    typedef std::basic_string<CharType>             StorageType;
    typedef TypedCharPtrT<const CharType, Type_T>   TypedPtrT;
    typedef const CharType*                         PtrRawT;
    typedef SzPtrT<const CharType, Type_T>          SzPtrR;
    typedef SzPtrT<CharType, Type_T>                SzPtrW;
    typedef typename StorageType::size_type         size_type;

    StringTyped() {}
    explicit StringTyped(SzPtrR psz) : m_s(psz.c_str()) {}
    explicit StringTyped(TypedPtrT iterBegin, TypedPtrT iterEnd) : m_s(iterBegin.rawPtr(), iterEnd.rawPtr()) {}

#if (DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT == 0)
    StringTyped(StringTyped&& other)
    {
        operator=(std::move(other));
    }

    StringTyped& operator=(StringTyped&& other)
    {
        this->m_s = std::move(other.m_s);
        return *this;
    }
#endif // DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT

    // Precondition: sRaw must be correctly encoded.
    static StringTyped fromRawString(StorageType sRaw)
    {
        StringTyped s;
        s.m_s = std::move(sRaw);
        return s;
    }

    // Convenience overload
    static StringTyped fromRawString(const CharType* pBegin, const CharType* pEnd)
    {
        return fromRawString(StorageType(pBegin, pEnd));
    }

    SzPtrR c_str() const { return SzPtrR(m_s.c_str()); }

    SzPtrR data() const { return c_str(); }

    // Note: begin() and end() are omitted on purpose because TypedPtrT might not have increment/decrement operators.
    PtrRawT beginRaw() const { return m_s.c_str(); }
    PtrRawT endRaw()   const { return beginRaw() + sizeInRawUnits(); }

    void append(const TypedPtrT& first, const TypedPtrT& end)
    {
        m_s.append(first.rawPtr(), end.rawPtr());
    }

    int compare(const size_type nThisStart, const size_t nThisCount, const TypedPtrT& psz, const size_type& nCount) const
    {
        return m_s.compare(nThisStart, nThisCount, toCharPtr_raw(psz), nCount);
    }

	// TODO: test
	StringTyped& operator=(const SzPtrR& psz)
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
    bool operator==(const StringTyped& other) const
    {
        return (m_s.size() == other.m_s.size()) && m_s == other.m_s;
    }

    bool operator!=(const StringTyped& other) const
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
	bool operator<(const StringTyped& other) const
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
inline bool operator==(const typename StringTyped<Type_T>::SzPtrR& tpsz, const StringTyped<Type_T>& right)
{
    return right == tpsz;
}

template <CharPtrType Type_T>
inline bool operator<(const typename StringTyped<Type_T>::SzPtrR& tpsz, const StringTyped<Type_T>& right)
{
    return right < tpsz;
}

typedef StringTyped<CharPtrTypeAscii>    StringAscii;
typedef StringTyped<CharPtrTypeLatin1>   StringLatin1;
typedef StringTyped<CharPtrTypeUtf8>     StringUtf8;
typedef StringTyped<CharPtrTypeUtf16>    StringUtf16;

template <CharPtrType PtrType_T>
inline auto toSzPtr_raw(const StringTyped<PtrType_T>& str) -> decltype(str.c_str().c_str()) { return str.c_str().c_str(); }

} // root namespace




