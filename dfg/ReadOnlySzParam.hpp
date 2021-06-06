#pragma once

#include "dfgDefs.hpp"
#include "dfgBaseTypedefs.hpp"
#include "dfgAssert.hpp"
#include "numericTypeTools.hpp"
#include "str/strCmp.hpp"
#include "str/strLen.hpp"
#include "build/utils.hpp"
#include <string>
#include <limits>
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
 
NOTE: ReadOnlySzParam is deprecated; StringView and StringViewSz are alternatives.
 
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
class ReadOnlySzParam
{
public:
	typedef const Char_T* iterator;
	typedef const Char_T* const_iterator;

	template <size_t N>
	ReadOnlySzParam(const Char_T(&arr)[N]) :
		m_pSz(arr)
	{
	}

	ReadOnlySzParam(const Char_T* psz) :
		m_pSz(psz)
	{
	}

	ReadOnlySzParam(const std::basic_string<Char_T>& s) :
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

// Like ReadOnlySzParam, but also stores the size. For string classes, uses the
// size returned by related member function, not the null terminated length; these
// may differ in case of embedded null chars.
// Note: if null terminated string is not required, consider using StringView instead of this class.
template <class Char_T>
class ReadOnlySzParamWithSize : public ReadOnlySzParam<Char_T>
{
public:
	typedef ReadOnlySzParam<Char_T> BaseClass;
	using typename BaseClass::iterator;
	using typename BaseClass::const_iterator;

	ReadOnlySzParamWithSize(const std::basic_string<Char_T>& s) :
		BaseClass(readOnlySzParamConverter<Char_T>(s)),
		m_nSize(readOnlySzParamLength(s))
	{

	}

	ReadOnlySzParamWithSize(const Char_T* psz) :
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
    // Base common for both both view and viewSz. Most of the the stuff in StringView and StringViewSz should be here.
    template <class Char_T, class Str_T>
    class StringViewCommonBase
    {
    public:
        using SzPtrT                = decltype(Str_T().c_str());
        using PtrT                  = decltype(toCharPtr(Str_T().c_str()));
        using PtrRawT               = decltype(toCharPtr_raw(Str_T().c_str()));
        using CharT                 = Char_T;
        using CodePointT            = decltype(*PtrT(nullptr));
        using StringT               = Str_T;
        using const_iterator        = PtrT;

    protected:
        struct NonSzConstructorTag {};

    public:

        StringViewCommonBase(PtrT p = PtrT(nullptr)) :
            m_pFirst(p)
        {}

        static constexpr bool isTriviallyIndexable()
        {
            return CharPtrTypeTraits<charPtrTypeByPtr<PtrT>()>::hasTrivialIndexing;
        }

        PtrT data() const { return m_pFirst; }
        PtrRawT dataRaw() const { return toCharPtr_raw(this->data()); }

        const_iterator begin() const
        {
            return data();
        }

        const_iterator cbegin() const
        {
            return begin();
        }

        PtrRawT beginRaw() const
        {
            return toCharPtr_raw(begin());
        }

        PtrT m_pFirst;          // Pointer to first character.
    };

    template <class Char_T, class Str_T>
    class StringViewDefaultBase : public StringViewCommonBase<Char_T, Str_T>
    {
    protected:
        using BaseClass         = StringViewCommonBase<Char_T, Str_T>;
        using PtrT              = typename BaseClass::PtrT;
        using PtrRawT           = typename BaseClass::PtrRawT;

        StringViewDefaultBase() :
            m_nSize(0)
        {}

        StringViewDefaultBase(PtrT p, const size_t nCount) :
            BaseClass(p),
            m_nSize(nCount)
        {}

    public:
        bool empty() const
        {
            return m_nSize == 0;
        }

        // memcmp() causes undefined behaviour if argument is nullptr even when size argument is 0.
        // Thus passing beginRaw() to memcmp() is not valid even if not actually comparing anything
        // so this function is a helper that guarantees to return valid pointer even in case of empty.
        PtrRawT privBeginRawForMemcmp() const
        {
            auto p = toCharPtr_raw(this->begin());
            DFG_ASSERT_UB(p != nullptr || this->empty());
            return (p != nullptr) ? p : reinterpret_cast<const Char_T*>(this);
        }

    protected:
        size_t m_nSize;	        // Length of the string as returned by strLen().
    };

    // Base class for string views that have raw characters or for which every character can be treated as a code point.
    // -> functionality such as front(), back(), operator[] etc. are well defined.
    template <class Char_T, class Str_T>
    class StringViewIndexAccessBase : public StringViewDefaultBase<Char_T, Str_T>
    {
    public:
        using BaseClass             = StringViewDefaultBase<Char_T, Str_T>;
        using PtrT                  = typename BaseClass::PtrT;
        using CodePointT            = typename BaseClass::CodePointT;
        
        StringViewIndexAccessBase()
        {}

        StringViewIndexAccessBase(PtrT psz, const size_t nCount) :
            BaseClass(psz, nCount)
        {}

        CodePointT back() const
        {
            DFG_ASSERT_UB(!this->empty());
            return CodePointT(*(toCharPtr_raw(this->m_pFirst) + this->m_nSize - 1));
        }

        void clear()
        {
            this->m_pFirst = PtrT(nullptr);
            this->m_nSize = 0;
        }

        CodePointT operator[](const size_t n) const
        {
            DFG_ASSERT_UB(n < this->m_nSize);
            return CodePointT(*(toCharPtr_raw(this->m_pFirst) + n));
        }

        CodePointT front() const
        {
            DFG_ASSERT_UB(!this->empty());
            return *this->m_pFirst;
        }

        // Precondition: empty() == false
        void pop_front()
        {
            DFG_ASSERT_UB(!this->empty());
            ++this->m_pFirst;
            this->m_nSize--;
        }

        // Precondition: empty() == false
        void pop_back()
        {
            DFG_ASSERT_UB(!this->empty());
            this->m_nSize--;
        }

        void cutTail(const PtrT iter)
        {
        #if DFG_LANGFEAT_EXPLICIT_OPERATOR_BOOL
            const auto nCount = (this->m_pFirst + this->m_nSize) - iter;
        #else
            const auto nCount = (this->m_pFirst + ptrdiff_t(this->m_nSize)) - iter;
        #endif
            this->m_nSize -= nCount;
        }
    }; // StringViewIndexAccessBase

    template <class Char_T, class Str_T> struct StringViewBase;
    template <> struct StringViewBase<char,    std::string>             { typedef StringViewIndexAccessBase<char,    std::string>    type; };
    template <> struct StringViewBase<wchar_t, std::wstring>            { typedef StringViewIndexAccessBase<wchar_t, std::wstring>   type; };
    template <> struct StringViewBase<char,    StringAscii>             { typedef StringViewIndexAccessBase<char,    StringAscii>    type; };
    template <> struct StringViewBase<char,    StringLatin1>            { typedef StringViewIndexAccessBase<char,    StringLatin1>   type; };
    template <> struct StringViewBase<char,    StringUtf8>              { typedef StringViewIndexAccessBase<char,    StringUtf8>     type; };
    DFG_STATIC_ASSERT(DFG_DETAIL_NS::gnNumberOfCharPtrTypesWithEncoding == 4, "Is a typed string view missing?"); // TODO: utf16

    template <class View_T, class Str_T>
    View_T substr_startCount(size_t nStart, size_t nCount, const Str_T& str)
    {
        const auto iterBegin = str.begin();
        const auto iterEnd = str.end();
        //const auto nSize = static_cast<size_t>(std::distance(iterBegin, iterEnd));
        const auto nSize = static_cast<size_t>(iterEnd - iterBegin);
        if (nStart > nSize)
            nStart = nSize;
        if (nCount > nSize - nStart)
            nCount = nSize - nStart;
        const auto iterSubStart = iterBegin + nStart;
        return View_T(iterSubStart, iterSubStart + nCount);
    }

} // namespace DFG_DETAIL_NS

// Note: Unlike ReadOnlySzParam, the string view stored here can't guarantee access to null terminated string.
// TODO: keep compatible with std::string_view or even typedef when available.
template <class Char_T, class Str_T = std::basic_string<Char_T>>
class StringView : public DFG_DETAIL_NS::StringViewBase<Char_T, Str_T>::type
{
public:
    using BaseClass      = typename DFG_DETAIL_NS::StringViewBase<Char_T, Str_T>::type;
    using SzPtrT         = typename BaseClass::SzPtrT;
    using PtrT           = typename BaseClass::PtrT;
    using PtrRawT        = typename BaseClass::PtrRawT;
    using CharT          = typename BaseClass::CharT;
    using StringT        = typename BaseClass::StringT;
    using const_iterator = typename BaseClass::const_iterator;
    
public:
    StringView()
    {
    }

    StringView(const Str_T& s) :
        BaseClass(s.c_str(), readOnlySzParamLength(s))
    {
    }

    StringView(SzPtrT pszOrNullptr) :
        BaseClass(pszOrNullptr, (pszOrNullptr) ? readOnlySzParamLength(pszOrNullptr) : 0)
    {
    }

    // Allows contruction from compatible typed SzPtr (e.g. construction of const StringViewUtf8& sv = SzPtrAscii("a"))
    template <class Char2_T, CharPtrType Type_T>
    StringView(::DFG_ROOT_NS::SzPtrT<Char2_T, Type_T> psz) :
        BaseClass(psz, readOnlySzParamLength(psz))
    {
    }

    StringView(PtrT psz, const size_t nCountOfBaseChars) :
        BaseClass(psz, nCountOfBaseChars)
    {
    }

    StringView(PtrT pBegin, PtrT pEnd) :
        BaseClass(pBegin, toCharPtr_raw(pEnd) - toCharPtr_raw(pBegin))
    {
    }

protected:
    // For API-compatibility with StringViewSz
    StringView(typename BaseClass::NonSzConstructorTag)
        : StringView()
    {}

public:

    // Returns view as untyped.
    StringView<CharT> asUntypedView() const
    {
        return StringView<CharT>(this->beginRaw(), this->endRaw());
    }

    size_t length() const
    {
        return this->m_nSize;
    }

    size_t size() const
    {
        return length();
    }

    // Returns size() as int through saturateCast.
    int sizeAsInt() const
    {
        return saturateCast<int>(size());
    }

    // Returns sub string by start position and count. Available only for string types that have trivial indexing.
    StringView substr_startCount(const size_t nStart, const size_t nCount) const
    {
        DFG_STATIC_ASSERT(BaseClass::isTriviallyIndexable(), "substr_startCount() is available only for string types that have trivial indexing. Possible workaround: asUntypedView().substr_startCount()");
        return DFG_DETAIL_NS::substr_startCount<StringView>(nStart, nCount, *this);
    }

    // Returns substring starting at nStart
    StringView substr_start(const size_t nStart) const
    {
        return substr_startCount(nStart, (std::numeric_limits<size_t>::max)());
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
        return Str_T(this->cbegin(), this->cend());
    }

    bool operator==(const StringView& other) const
    {
        return (this->m_nSize == other.m_nSize) && std::equal(toCharPtr_raw(this->m_pFirst), toCharPtr_raw(this->m_pFirst) + this->m_nSize, toCharPtr_raw(other.m_pFirst));
    }

    bool operator==(const Str_T& str) const
    {
        return operator==(StringView(str));
    }

    bool operator==(const SzPtrT& tpsz) const
    {
        // TODO: this is potentially suboptimal: makes redundant strlen() call due to construction of StringView.
        return operator==(StringView(tpsz));
    }

    // Conversion to untyped StringView.
    operator StringView<Char_T>() const
    {
        return asUntypedView();
    }

}; // class StringView

namespace DFG_DETAIL_NS
{
    static const size_t gnStringViewSzSizeNotCalculated = NumericTraits<size_t>::maxValue;
}

// Like StringView, but guarantees that view is null terminated.
// Also the string length is not computed on constructor but on demand removing some of the const's.
template <class Char_T, class Str_T = std::basic_string<Char_T>>
class StringViewSz : public DFG_DETAIL_NS::StringViewCommonBase<Char_T, Str_T>
{
public:
    using BaseClass      = typename DFG_DETAIL_NS::StringViewCommonBase<Char_T, Str_T>;
    using StringViewT    = StringView<Char_T, Str_T>;
    using SzPtrT         = typename BaseClass::SzPtrT;
    using PtrT           = typename BaseClass::PtrT;
    using PtrRawT        = typename BaseClass::PtrRawT;
    using CharT          = typename BaseClass::CharT;
    using StringT        = typename BaseClass::StringT;
    using CodePointT     = typename BaseClass::CodePointT;
    using const_iterator = typename BaseClass::const_iterator;
    
    StringViewSz(const Str_T& s) :
        BaseClass(s.c_str()),
        m_nSize(readOnlySzParamLength(s))
    {
        DFG_ASSERT_CORRECTNESS(this->m_pFirst != nullptr);
    }

    StringViewSz(SzPtrT psz) :
        BaseClass(psz),
        m_nSize(DFG_DETAIL_NS::gnStringViewSzSizeNotCalculated)
    {
        DFG_ASSERT_CORRECTNESS(this->m_pFirst != nullptr);
    }

    // Constructs StringViewSz and optionally sets size requiring psz[nCount] == '\0'.
    // Precondition: psz != nullptr && (nCount == DFG_DETAIL_NS::gnStringViewSzSizeNotCalculated || psz[nCount] == '\0')
    StringViewSz(SzPtrT psz, const size_t nCount) :
        BaseClass(psz),
        m_nSize(nCount)
    {
        DFG_ASSERT_CORRECTNESS(this->m_pFirst != nullptr);
        DFG_ASSERT_CORRECTNESS(m_nSize == DFG_DETAIL_NS::gnStringViewSzSizeNotCalculated || toCharPtr_raw(this->m_pFirst)[nCount] == '\0');
    }

protected:
    // StringViewOrOwner has well defined default constructor since it can always set view to internal owned member, but it can't do it yet in baseclass constructor.
    // This protected constructor allows creating a temporary non-sz view that the derived class will make a proper sz-view in it's own constructor.
    StringViewSz(typename BaseClass::NonSzConstructorTag) :
        BaseClass(nullptr),
        m_nSize(0)
    {}

public:

    // Note: returning StringViewT as substring can't be of type StringViewSz: only tail parts can be Sz-substrings (e.g. substring "a" from "abc" is not sz)
    StringViewT substr_startCount(const size_t nStart, const size_t nCount) const
    {
        return toStringView().substr_startCount(nStart, nCount);
    }

    // Returns substring starting at nStart. Note that unlike substr_startCount(), this can be guaranteed to be null-terminated so return type is StringViewSz.
    StringViewSz substr_start(const size_t nStart) const
    {
        auto sv = substr_startCount(nStart, (std::numeric_limits<size_t>::max)());
        return StringViewSz(SzPtrT(sv.beginRaw()));
    }

    // Returns view as untyped.
    StringViewSz<CharT> asUntypedView() const
    {
        return StringViewSz<CharT>(this->dataRaw(), this->m_nSize);
    }

    bool empty() const
    {
        return *toCharPtr_raw(this->m_pFirst) == '\0';
    }

    bool isLengthCalculated() const
    {
        return m_nSize != DFG_DETAIL_NS::gnStringViewSzSizeNotCalculated;
    }

    size_t computeLength() const
    {
        return readOnlySzParamLength(c_str());
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

    size_t length() const
    {
        return (isLengthCalculated()) ? m_nSize : computeLength();
    }

    size_t size()       { return length(); }
    size_t size() const { return length(); }

    SzPtrT c_str() const { return SzPtrT(toCharPtr_raw(this->m_pFirst)); }

    const_iterator end()
    {
        return PtrT(toCharPtr_raw(this->m_pFirst) + length());
    }

    const_iterator end() const
    {
        return PtrT(toCharPtr_raw(this->m_pFirst) + lengthNonCaching());
    }

    PtrRawT endRaw() const
    {
        return toCharPtr_raw(end());
    }

    const_iterator cend() const
    {
        return end();
    }

    Str_T toString() const
    {
        return StringViewSz(*this).toStringView().toString();
    }

    bool operator==(StringViewSz other)
    {
        return toStringView() == other.toStringView();
    }

    bool operator==(StringViewSz other) const
    {
        return StringViewSz(*this) == other;
    }

    StringViewT toStringView()                       { return StringViewT(this->m_pFirst, length()); }
    StringViewT toStringView() const                 { return StringViewT(this->m_pFirst, lengthNonCaching()); }
    StringViewT toStringViewFromCachedSize() const   { return StringViewT(this->m_pFirst, m_nSize); }

    bool operator==(const Str_T& str)
    {
        return str == toStringView();
    }

    bool operator==(const SzPtrT& tpsz)
    {
        const auto& rThis = *this;
        return rThis == tpsz;
    }

    bool operator==(const SzPtrT& tpsz) const
    {
        return (isLengthCalculated()) ? toStringViewFromCachedSize() == tpsz : DFG_MODULE_NS(str)::strCmp(SzPtrT(toCharPtr_raw(this->m_pFirst)), tpsz) == 0;
    }

    bool operator==(const StringViewT& sv) const
    {
        return sv == this->c_str();
    }

    CodePointT operator[](const size_t n) const
    {
        DFG_STATIC_ASSERT(BaseClass::isTriviallyIndexable(), "operator[] is available only for string types that have trivial indexing. Possible workaround: asUntypedView().operator[]");
        DFG_ASSERT_UB(n < this->lengthNonCaching());
        return *(this->begin() + n);
    }

    // Conversion to untyped StringViewSz.
    operator StringViewSz<Char_T>() const
    {
        return this->asUntypedView();
    }

    // Conversion to untyped StringView.
    operator StringView<Char_T>() const
    {
        return StringView<Char_T>(toCharPtr_raw(this->m_pFirst));
    }

    // Conversion to ReadOnlySzParam for compatibility.
    operator ReadOnlySzParam<Char_T>() const
    {
        return ReadOnlySzParam<Char_T>(toCharPtr_raw(this->m_pFirst));
    }

//protected:
    size_t m_nSize;	    // Length of the string or DFG_DETAIL_NS::gnStringViewSzSizeNotCalculated
}; // StringViewSz

template <CharPtrType Type_T, class Char_T, class Str_T>
inline bool operator==(const SzPtrT<const Char_T, Type_T>& psz, const StringView<Char_T, Str_T>& right)
{
    return right == psz;
}

template <class Char_T>
inline bool operator==(const Char_T* psz, const StringView<Char_T, std::basic_string<Char_T>>& right) { return right == psz; }

template<class Char_T, class Str_T>
inline bool operator==(const Str_T& s, const StringView<Char_T, Str_T>& right)
{
    return right == s;
}

template<class Char_T, class Str_T>
inline bool operator<(const Str_T& s, const StringView<Char_T, Str_T>& right)
{
    return s.compare(0, s.size(), right.begin(), right.length()) < 0;
}

template<class Char_T, class Str_T>
inline bool operator<(const StringView<Char_T, Str_T>& left, const StringView<Char_T, Str_T>& right)
{
    const auto nMinSize = Min(left.size(), right.size());
    // memcmp() need non-null arguments even if nMinSize is zero (https://stackoverflow.com/a/16363034), but views can return nullptr when empty.
    // -> using special helper that guarantees to give non-null pointer.
    const auto cmp = std::memcmp(left.privBeginRawForMemcmp(), right.privBeginRawForMemcmp(), nMinSize);
    return (cmp < 0 || (cmp == 0 && left.size() < right.size()));
}

template<class Char_T, class Str_T>
inline bool operator!=(const StringView<Char_T, Str_T>& left, const StringView<Char_T, Str_T>& right)
{
    return !(left == right);
}

template <CharPtrType Type_T, class Char_T, class Str_T>
inline bool operator==(const SzPtrT<const Char_T, Type_T>& psz, StringViewSz<Char_T, Str_T> right)
{
    return right == psz;
}

template <class Char_T, class Str_T>
inline bool operator==(const StringView<Char_T, Str_T>& left, const StringViewSz<Char_T, Str_T>& right)
{
    return right == left;
}

template <class Char_T>
inline bool operator==(const Char_T* psz, const StringViewSz<Char_T, std::basic_string<Char_T>>& right) { return right == StringViewSz<Char_T, std::basic_string<Char_T>>(psz); }

template<class Char_T, class Str_T>
inline bool operator==(const Str_T& s, StringViewSz<Char_T, Str_T> right)
{
    return right == s;
}

template<class Char_T, class Str_T>
inline bool operator!=(const StringView<Char_T, Str_T>& left, const Str_T& s)
{
    return !(left == s);
}

template<class Char_T, class Str_T>
inline bool operator!=(const Str_T& s, const StringView<Char_T, Str_T>& right)
{
    return !(s == right);
}

template<class Char_T, class Str_T, class SzPtr_T>
inline bool operator!=(StringView<Char_T, Str_T> left, const SzPtr_T& right)
{
    return !(left == right);
}

template<class Char_T, class Str_T, class SzPtr_T>
inline bool operator!=(const SzPtr_T& left, StringView<Char_T, Str_T> right)
{
    return !(left == right);
}

template<class Char_T, class Str_T>
inline bool operator<(const Str_T& s, const StringViewSz<Char_T, Str_T>& right)
{
    return DFG_MODULE_NS(str)::strCmp(s.c_str(), right.c_str()) < 0;
}

template<class Char_T, class Str_T>
inline bool operator<(const StringViewSz<Char_T, Str_T>& left, const StringViewSz<Char_T, Str_T> & right)
{
    return DFG_MODULE_NS(str)::strCmp(left.c_str(), right.c_str()) < 0;
}

template<class Char_T, class Str_T>
inline bool operator!=(StringViewSz<Char_T, Str_T> left, StringViewSz<Char_T, Str_T> right)
{
    return !(left == right);
}

template<class Char_T, class Str_T, class SzPtr_T>
inline bool operator!=(StringViewSz<Char_T, Str_T> left, const SzPtr_T& right)
{
    return !(left == right);
}

template<class Char_T, class Str_T, class SzPtr_T>
inline bool operator!=(const SzPtr_T& left, StringViewSz<Char_T, Str_T> right)
{
    return !(left == right);
}

template<class Char_T, class Str_T>
inline bool operator!=(const Str_T& s, const StringViewSz<Char_T, Str_T>& right)
{
    return !(s == right);
}

typedef ReadOnlySzParam<char>               ReadOnlySzParamC;
typedef ReadOnlySzParam<wchar_t>            ReadOnlySzParamW;

typedef ReadOnlySzParamWithSize<char>       ReadOnlySzParamWithSizeC;
typedef ReadOnlySzParamWithSize<wchar_t>    ReadOnlySzParamWithSizeW;

typedef StringView<char>                  StringViewC;
typedef StringView<wchar_t>               StringViewW;
typedef StringView<char, StringAscii>     StringViewAscii;
typedef StringView<char, StringLatin1>    StringViewLatin1;
typedef StringView<char, StringUtf8>      StringViewUtf8;

typedef StringViewSz<char>                StringViewSzC;
typedef StringViewSz<wchar_t>             StringViewSzW;
typedef StringViewSz<char, StringAscii>   StringViewSzAscii;
typedef StringViewSz<char, StringLatin1>  StringViewSzLatin1;
typedef StringViewSz<char, StringUtf8>    StringViewSzUtf8;


// StringView wrapper that can optionally own the content. To be used e.g. in cases where return value from a function
// can be a view to a string literal or constructed temporary.
// Note: Beware slicing:
//      -Creating dangling by not using auto:
//          const auto func = []() { return StringViewOrOwner<StringViewC, std::string>::makeOwned("abc"); };
//          StringViewC svBAD = func(); // view will dangle after returned temporary StringViewOrOwner() is destroyed.
//          auto svOK = func(); // OK
//      -Note, though, that in case of function arguments as follows slicing is not a problem:
//          void func(StringViewC sv) { ... }
//          func(StringViewOrOwner<StringViewC, std::string>::makeOwned("abc"));
//              func() will get sliced copy of StringViewOrOwner-object, but the original object and thus the viewed string is alive during call of func()
template <class View_T, class Owned_T>
class StringViewOrOwner : public View_T
{
public:
    typedef View_T BaseClass;

    StringViewOrOwner()
        : BaseClass(typename BaseClass::NonSzConstructorTag())
    {
        BaseClass::operator=(m_owned);
    }

    ~StringViewOrOwner()
    {
    }

    static StringViewOrOwner makeView(const View_T& view)
    {
        return StringViewOrOwner(view);
    }

    // Move constructor is needed because with default implementation view in 'this' could be pointing to
    // SSO buffer of moved from -object.
    StringViewOrOwner(StringViewOrOwner&& other) noexcept
        : BaseClass(std::move(other)) // Note: this moves only the view-part.
    {
        const bool bLocal = (other.m_owned.data() == this->data());
        m_owned = std::move(other.m_owned);
        if (bLocal)
            BaseClass::operator=(m_owned); // Reinitializes view to make sure that view won't point to SSO-buffer of moved from -object.
    }

    static StringViewOrOwner makeOwned(Owned_T other = Owned_T())
    {
        auto rv = StringViewOrOwner(other);
        rv.m_owned = std::move(other);
        rv.BaseClass::operator=(rv.m_owned);
        return rv;
    }

    bool isOwner() const
    {
        return this->data() == m_owned.data();
    }

    // Sets this as empty view and returns owned resource by moving.
    Owned_T release()
    {
        auto rv = isOwner() ? std::move(m_owned) : this->toString();
        m_owned.clear();
        BaseClass::operator=(m_owned); // Note: setting view to owned because in case of SzView, view must be null terminated.
        return rv;
    }

    const Owned_T& owned() const
    {
        return m_owned;
    }

private:
    StringViewOrOwner(const View_T& view)
        : BaseClass(view)
    {}

    Owned_T m_owned;
}; // StringViewOrOwner



} // namespace
