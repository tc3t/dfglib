#pragma once

#include "dfgDefs.hpp"
#include "dfgBaseTypedefs.hpp"
#include "dfgAssert.hpp"
#include "numericTypeTools.hpp"
#include "str/strCmp.hpp"
#include "str/strLen.hpp"
#include "buildConfig.hpp"
#include "build/utils.hpp"
#include <string>
#include "str/string.hpp"
#include "build/languageFeatureInfo.hpp"

DFG_ROOT_NS_BEGIN {

template <class Char_T, class String_T>
const Char_T* readOnlySzParamConverter(const String_T&)
{ 
	DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(String_T, "No conversion to ReadOnlySzParam exists for given type");
}

// Returns length in chars.
template <class String_T>
size_t readOnlySzParamLength(const String_T& str)
{
	return DFG_SUB_NS_NAME(str)::strLen(str);
}

template <> inline ConstCharPtr readOnlySzParamConverter<char, ConstCharPtr>(const ConstCharPtr& psz) { return psz; }
template <> inline ConstWCharPtr readOnlySzParamConverter<wchar_t, ConstWCharPtr>(const ConstWCharPtr& psz) { return psz; }
template <> inline ConstCharPtr readOnlySzParamConverter<char, std::string>(const std::string& s) { return s.c_str(); }
template <> inline ConstWCharPtr readOnlySzParamConverter<wchar_t, std::wstring>(const std::wstring& s) { return s.c_str(); }

/*
Class to be used as a convenient replacement of any concrete string type
in such function parameter that is essentially a c-string compatible, read only string parameter.

/---------------------------------------------------------------------------------------------------\
| Note: if null terminated string is not required, consider using StringView instead of this class. |
\---------------------------------------------------------------------------------------------------/

Definition: c-string = null terminated, contiguous array of integer types. Length of the string
						is determined by the number of character types before null character.

Problem with parameter type of
void func(const std::string& s);
is that if func is called with string literal,
char array or with some custom string types, this interface forces construction of
std::string temporary, which may involve dynamic memory allocation.

A fix could be to define parameter as const char*. This, however, introduces inconveniences related
to passing string objects such as std::string to the function as caller would have to use something like
func(s.c_str()). Also if the parameter was ever changed from const char* to const std::string&, this would
trigger the problems described earlier even if the callers would have proper std::string-objects that could
be passed.

Using this class as parameter type instead of const std::string& or const char* hides the implementation
details from caller and allows std::strings or char arrays to be passed to the function without performance
concerns.

Related implementations:
StringPiece (http://src.chromium.org/viewvc/chrome/trunk/src/base/string_piece.h?view=markup)
StringRef (http://llvm.org/docs/ProgrammersManual.html#passing-strings-the-stringref-and-twine-classes)
string_ref (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3442.html)
*/
template <class Char_T>
class DFG_CLASS_NAME(ReadOnlySzParam)
{
public:
	typedef const Char_T* iterator;
	typedef const Char_T* const_iterator;

	template <size_t N>
	DFG_CLASS_NAME(ReadOnlySzParam)(const Char_T(&arr)[N]) :
		m_pSz(arr)
	{
	}

	DFG_CLASS_NAME(ReadOnlySzParam)(const Char_T* psz) :
		m_pSz(psz)
	{
	}

	DFG_CLASS_NAME(ReadOnlySzParam)(const std::basic_string<Char_T>& s) :
		m_pSz(readOnlySzParamConverter<Char_T>(s))
	{
	}
	
	// Return c-string length of string.
	size_t length() const
	{
		return DFG_MODULE_NS(str)::strLen(m_pSz);
	}

	const_iterator begin() const
	{
		return m_pSz;
	}

	operator const Char_T*() const
	{
		return m_pSz;
	}

	const Char_T* c_str() const
	{
		return m_pSz;
	}

	// Note: Having both
	//       operator const CharT* and
	//		 operator[]
	//		 will lead to ambiguity in []-operators.
	/*
	CharT operator[](const size_t nIndex) const
	{
		return m_pSz[nIndex];
	}
	*/

	/* // Commented since calculating Length() may be costly.
	const_iterator end() const
	{
		return m_pSz + Length();
	}
	*/

	const Char_T* const m_pSz; // Pointer to null terminated string.
};

// Like DFG_CLASS_NAME(ReadOnlySzParam), but also stores the size. For string classes, uses the
// size returned by related member function, not the null terminated length; these
// may differ in case of embedded null chars.
// Note: if null terminated string is not required, consider using StringView instead of this class.
template <class Char_T>
class DFG_CLASS_NAME(ReadOnlySzParamWithSize) : public DFG_CLASS_NAME(ReadOnlySzParam)<Char_T>
{
public:
	typedef DFG_CLASS_NAME(ReadOnlySzParam)<Char_T> BaseClass;
	using typename BaseClass::iterator;
	using typename BaseClass::const_iterator;

	DFG_CLASS_NAME(ReadOnlySzParamWithSize)(const std::basic_string<Char_T>& s) :
		BaseClass(readOnlySzParamConverter<Char_T>(s)),
		m_nSize(readOnlySzParamLength(s))
	{

	}

	DFG_CLASS_NAME(ReadOnlySzParamWithSize)(const Char_T* psz) :
		BaseClass(readOnlySzParamConverter<Char_T, const Char_T*>(psz)),
		m_nSize(readOnlySzParamLength(psz))
	{

	}

	size_t length() const
	{
		return m_nSize;
	}

	const_iterator end() const
	{
		return typename BaseClass::begin() + length();
	}

protected:
	const size_t m_nSize;		// Length of the string. In case of raw pointers, this will be strlen().
								// For string classes this may be the value returned by length()-method.
}; // Class ReadOnlySzParamWithSize

namespace DFG_DETAIL_NS
{
    template <class Ptr_T>
    class StringViewDefaultBase
    {
    protected:
        StringViewDefaultBase() :
            m_pFirst(nullptr),
            m_nSize(0)
        {}

        StringViewDefaultBase(Ptr_T psz, const size_t nCount) :
            m_pFirst(psz),
            m_nSize(nCount)
        {}

    public:
        bool empty() const
        {
            return m_nSize == 0;
        }

        Ptr_T data() const { return m_pFirst; }

    protected:
        Ptr_T m_pFirst;          // Pointer to first character.
        size_t m_nSize;	        // Length of the string as returned by strLen().
    };

    // Base class for string views that have raw characters or for which every character can be treated as a code point.
    // -> functionality such as front(), back(), operator[] etc. are well defined.
    template <class Ptr_T>
    class StringViewIndexAccessBase : public StringViewDefaultBase<Ptr_T>
    {
        typedef decltype(*Ptr_T(nullptr)) CodePointT;
        typedef StringViewDefaultBase<Ptr_T> BaseClass;
    public:
        StringViewIndexAccessBase()
        {}

        StringViewIndexAccessBase(Ptr_T psz, const size_t nCount) :
            BaseClass(psz, nCount)
        {}

        CodePointT back() const
        {
            DFG_ASSERT_UB(!empty());
            return CodePointT(*(toCharPtr_raw(this->m_pFirst) + this->m_nSize - 1));
        }

        void clear()
        {
            this->m_pFirst = Ptr_T(nullptr);
            this->m_nSize = 0;
        }

        CodePointT operator[](const size_t n) const
        {
            DFG_ASSERT_UB(n < this->m_nSize);
            return CodePointT(*(toCharPtr_raw(this->m_pFirst) + n));
        }

        CodePointT front() const
        {
            DFG_ASSERT_UB(!empty());
            return *this->m_pFirst;
        }

        // Precondition: empty() == false
        void pop_front()
        {
            DFG_ASSERT_UB(!empty());
            ++this->m_pFirst;
            this->m_nSize--;
        }

        // Precondition: empty() == false
        void pop_back()
        {
            DFG_ASSERT_UB(!empty());
            this->m_nSize--;
        }

        void cutTail(const Ptr_T iter)
        {
        #if DFG_LANGFEAT_EXPLICIT_OPERATOR_BOOL
            const auto nCount = (this->m_pFirst + this->m_nSize) - iter;
        #else
            const auto nCount = (this->m_pFirst + ptrdiff_t(this->m_nSize)) - iter;
        #endif
            this->m_nSize -= nCount;
        }
    };

    template <class Str_T> struct StringViewBase;
    template <> struct StringViewBase<std::string>                     { typedef StringViewIndexAccessBase<decltype(toCharPtr(std::string().c_str()))> type; };
    template <> struct StringViewBase<std::wstring>                    { typedef StringViewIndexAccessBase<decltype(toCharPtr(std::wstring().c_str()))> type; };
    template <> struct StringViewBase<DFG_CLASS_NAME(StringAscii)>     { typedef StringViewIndexAccessBase<decltype(toCharPtr(DFG_CLASS_NAME(StringAscii)().c_str()))> type; };
    template <> struct StringViewBase<DFG_CLASS_NAME(StringLatin1)>    { typedef StringViewIndexAccessBase<decltype(toCharPtr(DFG_CLASS_NAME(StringLatin1)().c_str()))> type; };
    template <> struct StringViewBase<DFG_CLASS_NAME(StringUtf8)>      { typedef StringViewDefaultBase<decltype(toCharPtr(DFG_CLASS_NAME(StringUtf8)().c_str()))> type; };
    DFG_STATIC_ASSERT(DFG_DETAIL_NS::gnNumberOfCharPtrTypesWithEncoding == 3, "Is a typed string view missing?");

} // namespace DFG_DETAIL_NS

// Note: Unlike ReadOnlySzParam, the string view stored here can't guarantee access to null terminated string.
// TODO: keep compatible with std::string_view or even typedef when available.
template <class Char_T, class Str_T = std::basic_string<Char_T>>
class DFG_CLASS_NAME(StringView) : public DFG_DETAIL_NS::StringViewBase<Str_T>::type
{
public:
    typedef decltype(Str_T().c_str())                   SzPtrT;
    typedef decltype(toCharPtr(Str_T().c_str()))        PtrT;
    typedef decltype(toCharPtr_raw(Str_T().c_str()))    PtrRawT;
    typedef typename DFG_DETAIL_NS::StringViewBase<Str_T>::type BaseClass;
    typedef Char_T                                      CharT;
    
    typedef PtrT const_iterator;

    DFG_CLASS_NAME(StringView)()
    {
    }

    DFG_CLASS_NAME(StringView)(const Str_T& s) :
        BaseClass(s.c_str(), readOnlySzParamLength(s))
    {
    }

    DFG_CLASS_NAME(StringView)(SzPtrT pszOrNullptr) :
        BaseClass(pszOrNullptr, (pszOrNullptr) ? readOnlySzParamLength(pszOrNullptr) : 0)
    {
    }

    // Allows contruction from compatible typed SzPtr (e.g. construction of const StringViewUtf8& sv = SzPtrAscii("a"))
    template <class Char2_T, CharPtrType Type_T>
    DFG_CLASS_NAME(StringView)(::DFG_ROOT_NS::SzPtrT<Char2_T, Type_T> psz) :
        BaseClass(psz, readOnlySzParamLength(psz))
    {
    }

    DFG_CLASS_NAME(StringView)(PtrT psz, const size_t nCount) :
        BaseClass(psz, nCount)
    {
    }

    size_t length() const
    {
        return this->m_nSize;
    }

    size_t size() const
    {
        return length();
    }

    PtrRawT dataRaw() const
    {
        return toCharPtr_raw(this->data());
    }

    const_iterator begin() const
    {
        return this->m_pFirst;
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    PtrRawT beginRaw() const
    {
        return toCharPtr_raw(begin());
    }

    const_iterator end() const
    {
        return PtrT(toCharPtr_raw(this->m_pFirst) + this->m_nSize);
    }

    const_iterator cend() const
    {
        return end();
    }

    PtrRawT endRaw() const
    {
        return toCharPtr_raw(end());
    }

    Str_T toString() const
    {
        return Str_T(cbegin(), cend());
    }

    bool operator==(const DFG_CLASS_NAME(StringView)& other) const
    {
        return (this->m_nSize == other.m_nSize) && std::equal(toCharPtr_raw(this->m_pFirst), toCharPtr_raw(this->m_pFirst) + this->m_nSize, toCharPtr_raw(other.m_pFirst));
    }

    bool operator==(const Str_T& str) const
    {
        return operator==(DFG_CLASS_NAME(StringView)(str));
    }

    bool operator==(const SzPtrT& tpsz) const
    {
        return operator==(DFG_CLASS_NAME(StringView)(tpsz));
    }

    // Conversion to untyped StringView.
    operator DFG_CLASS_NAME(StringView)<Char_T>() const
    {
        return DFG_CLASS_NAME(StringView)<Char_T>(beginRaw(), this->m_nSize);
    }

}; // class StringView

namespace DFG_DETAIL_NS
{
    static const size_t gnStringViewSzSizeNotCalculated = NumericTraits<size_t>::maxValue;
}

// Like StringView, but guarantees that view is null terminated.
// Also the string length is not computed on constructor but on demand removing some of the const's.
template <class Char_T, class Str_T = std::basic_string<Char_T>>
class DFG_CLASS_NAME(StringViewSz)
{
public:
    typedef DFG_CLASS_NAME(StringView)<Char_T, Str_T> StringViewT;

    typedef decltype(Str_T().c_str())               SzPtrT;
    typedef decltype(toCharPtr(Str_T().c_str()))    PtrT;
    typedef Char_T                                  CharT;

    typedef PtrT const_iterator;

    DFG_CLASS_NAME(StringViewSz)(const Str_T& s) :
        m_psz(s.c_str()),
        m_nSize(readOnlySzParamLength(s))
    {
        DFG_ASSERT_CORRECTNESS(m_psz != nullptr);
    }

    DFG_CLASS_NAME(StringViewSz)(SzPtrT psz) :
        m_psz(psz),
        m_nSize(DFG_DETAIL_NS::gnStringViewSzSizeNotCalculated)
    {
        DFG_ASSERT_CORRECTNESS(m_psz != nullptr);
    }

    // Careful with this: this must be null terminated view (i.e. is not enough that psz is null terminated).
    DFG_CLASS_NAME(StringViewSz)(SzPtrT psz, const size_t nCount) :
        m_psz(psz),
        m_nSize(nCount)
    {
        DFG_ASSERT_CORRECTNESS(toCharPtr_raw(m_psz)[nCount] == '\0');
    }

    bool empty() const
    {
        return *toCharPtr_raw(m_psz) == '\0';
    }

    bool isLengthCalculated() const
    {
        return m_nSize != DFG_DETAIL_NS::gnStringViewSzSizeNotCalculated;
    }

    size_t computeLength() const
    {
        return readOnlySzParamLength(m_psz);
    }

    size_t lengthNonCaching() const
    {
        if (!isLengthCalculated())
            return computeLength();
        else
            return m_nSize;
    }

    size_t length()
    {
        if (!isLengthCalculated())
            m_nSize = computeLength();
        return m_nSize;
    }

    size_t size()
    {
        return length();
    }

    PtrT data() const { return m_psz; }

    SzPtrT c_str() const { return m_psz; }

    const_iterator begin() const
    {
        return m_psz;
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    const_iterator end()
    {
        return PtrT(toCharPtr_raw(m_psz) + length());
    }

    const_iterator end() const
    {
        return PtrT(toCharPtr_raw(m_psz) + lengthNonCaching());
    }

    const_iterator cend() const
    {
        return end();
    }

    Str_T toString() const
    {
        return DFG_CLASS_NAME(StringViewSz)(*this).toStringView().toString();
    }

    bool operator==(DFG_CLASS_NAME(StringViewSz) other)
    {
        return toStringView() == other.toStringView();
    }

    bool operator==(DFG_CLASS_NAME(StringViewSz) other) const
    {
        return DFG_CLASS_NAME(StringViewSz)(*this) == other;
    }

    StringViewT toStringView()                       { return StringViewT(m_psz, length()); }
    StringViewT toStringViewFromCachedSize() const   { return StringViewT(m_psz, m_nSize); }

    bool operator==(const Str_T& str)
    {
        return str == toStringView();
    }

    bool operator==(const SzPtrT& tpsz)
    {
        return (isLengthCalculated()) ? toStringViewFromCachedSize() == tpsz : DFG_MODULE_NS(str)::strCmp(m_psz, tpsz) == 0;
    }

    // Conversion to untyped StringViewSz.
    operator DFG_CLASS_NAME(StringViewSz)<Char_T>() const
    {
        return DFG_CLASS_NAME(StringViewSz)<Char_T>(toCharPtr_raw(m_psz));
    }

    // Conversion to untyped StringView.
    operator DFG_CLASS_NAME(StringView)<Char_T>() const
    {
        return DFG_CLASS_NAME(StringView)<Char_T>(toCharPtr_raw(m_psz));
    }

//protected:
    SzPtrT m_psz;       // Pointer to first character.
    size_t m_nSize;	    // Length of the string or DFG_DETAIL_NS::gnStringViewSzSizeNotCalculated
}; // StringViewSz

template <CharPtrType Type_T, class Char_T, class Str_T>
inline bool operator==(const SzPtrT<const Char_T, Type_T>& psz, const DFG_CLASS_NAME(StringView)<Char_T, Str_T>& right)
{
    return right == psz;
}

template <class Char_T>
inline bool operator==(const Char_T* psz, const DFG_CLASS_NAME(StringView)<Char_T, std::basic_string<Char_T>>& right) { return right == psz; }

template<class Char_T, class Str_T>
inline bool operator==(const Str_T& s, const DFG_CLASS_NAME(StringView)<Char_T, Str_T>& right)
{
    return right == s;
}

template<class Char_T, class Str_T>
inline bool operator<(const Str_T& s, const DFG_CLASS_NAME(StringView)<Char_T, Str_T>& right)
{
    return s.compare(0, s.size(), right.begin(), right.length()) < 0;
}

template<class Char_T, class Str_T>
inline bool operator!=(const DFG_CLASS_NAME(StringView)<Char_T, Str_T>& left, const DFG_CLASS_NAME(StringView)<Char_T, Str_T>& right)
{
    return !(left == right);
}

template <CharPtrType Type_T, class Char_T, class Str_T>
inline bool operator==(const SzPtrT<const Char_T, Type_T>& psz, DFG_CLASS_NAME(StringViewSz)<Char_T, Str_T> right)
{
    return right == psz;
}

template <class Char_T>
inline bool operator==(const Char_T* psz, const DFG_CLASS_NAME(StringViewSz)<Char_T, std::basic_string<Char_T>>& right) { return right == psz; }

template<class Char_T, class Str_T>
inline bool operator==(const Str_T& s, DFG_CLASS_NAME(StringViewSz)<Char_T, Str_T> right)
{
    return right == s;
}

template<class Char_T, class Str_T>
inline bool operator<(const Str_T& s, const DFG_CLASS_NAME(StringViewSz)<Char_T, Str_T>& right)
{
    return DFG_MODULE_NS(str)::strCmp(s.c_str(), right.c_str()) < 0;
}

template<class Char_T, class Str_T>
inline bool operator!=(DFG_CLASS_NAME(StringViewSz)<Char_T, Str_T> left, DFG_CLASS_NAME(StringViewSz)<Char_T, Str_T> right)
{
    return !(left == right);
}

template<class Char_T, class Str_T, class SzPtr_T>
inline bool operator!=(DFG_CLASS_NAME(StringViewSz)<Char_T, Str_T> left, const SzPtr_T& right)
{
    return !(left == right);
}

typedef DFG_CLASS_NAME(ReadOnlySzParam)<char>				DFG_CLASS_NAME(ReadOnlySzParamC);
typedef DFG_CLASS_NAME(ReadOnlySzParam)<wchar_t>			DFG_CLASS_NAME(ReadOnlySzParamW);

typedef DFG_CLASS_NAME(ReadOnlySzParamWithSize)<char>		DFG_CLASS_NAME(ReadOnlySzParamWithSizeC);
typedef DFG_CLASS_NAME(ReadOnlySzParamWithSize)<wchar_t>	DFG_CLASS_NAME(ReadOnlySzParamWithSizeW);

typedef DFG_CLASS_NAME(StringView)<char>    	                        DFG_CLASS_NAME(StringViewC);
typedef DFG_CLASS_NAME(StringView)<wchar_t>                             DFG_CLASS_NAME(StringViewW);
typedef DFG_CLASS_NAME(StringView)<char, DFG_CLASS_NAME(StringAscii)>   DFG_CLASS_NAME(StringViewAscii);
typedef DFG_CLASS_NAME(StringView)<char, DFG_CLASS_NAME(StringLatin1)>  DFG_CLASS_NAME(StringViewLatin1);
typedef DFG_CLASS_NAME(StringView)<char, DFG_CLASS_NAME(StringUtf8)>    DFG_CLASS_NAME(StringViewUtf8);

typedef DFG_CLASS_NAME(StringViewSz)<char>    	                          DFG_CLASS_NAME(StringViewSzC);
typedef DFG_CLASS_NAME(StringViewSz)<wchar_t>                             DFG_CLASS_NAME(StringViewSzW);
typedef DFG_CLASS_NAME(StringViewSz)<char, DFG_CLASS_NAME(StringAscii)>   DFG_CLASS_NAME(StringViewSzAscii);
typedef DFG_CLASS_NAME(StringViewSz)<char, DFG_CLASS_NAME(StringLatin1)>  DFG_CLASS_NAME(StringViewSzLatin1);
typedef DFG_CLASS_NAME(StringViewSz)<char, DFG_CLASS_NAME(StringUtf8)>    DFG_CLASS_NAME(StringViewSzUtf8);

} // namespace
