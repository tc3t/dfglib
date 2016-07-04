#pragma once

#include "dfgDefs.hpp"
#include "dfgBaseTypedefs.hpp"
#include "str/strlen.hpp"
#include "buildConfig.hpp"
#include "build/utils.hpp"
#include <string>
#include "str/string.hpp"

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
};

// Note: Unlike previous, the string view stored here can't guarantee access to null terminated string.
// TODO: keep compatible with std::string_view or even typedef when available.
template <class Char_T, class Str_T = std::basic_string<Char_T>>
class DFG_CLASS_NAME(StringView)
{
public:
    typedef decltype(Str_T().c_str()) PtrT;
    typedef PtrT const_iterator;

    DFG_CLASS_NAME(StringView)(const Str_T& s) :
        m_pFirst(s.c_str()),
        m_nSize(readOnlySzParamLength(s))
    {
    }

    DFG_CLASS_NAME(StringView)(PtrT psz) :
        m_pFirst(psz),
        m_nSize(readOnlySzParamLength(psz))
    {
    }

    DFG_CLASS_NAME(StringView)(PtrT psz, const size_t nCount) :
        m_pFirst(psz),
        m_nSize(nCount)
    {
    }

    size_t length() const
    {
        return m_nSize;
    }

    const_iterator begin() const
    {
        return m_pFirst;
    }

    const_iterator end() const
    {
        return m_pFirst + m_nSize;
    }

protected:
    PtrT m_pFirst;          // Pointer to first character.
    const size_t m_nSize;	// Length of the string as returned by strLen().
};

typedef DFG_CLASS_NAME(ReadOnlySzParam)<char>				DFG_CLASS_NAME(ReadOnlySzParamC);
typedef DFG_CLASS_NAME(ReadOnlySzParam)<wchar_t>			DFG_CLASS_NAME(ReadOnlySzParamW);

typedef DFG_CLASS_NAME(ReadOnlySzParamWithSize)<char>		DFG_CLASS_NAME(ReadOnlySzParamWithSizeC);
typedef DFG_CLASS_NAME(ReadOnlySzParamWithSize)<wchar_t>	DFG_CLASS_NAME(ReadOnlySzParamWithSizeW);

typedef DFG_CLASS_NAME(StringView)<char>		            DFG_CLASS_NAME(StringViewC);
typedef DFG_CLASS_NAME(StringView)<wchar_t>	                DFG_CLASS_NAME(StringViewW);
typedef DFG_CLASS_NAME(StringView)<char, StringAscii>	    DFG_CLASS_NAME(StringViewAscii);
typedef DFG_CLASS_NAME(StringView)<char, StringLatin1>	    DFG_CLASS_NAME(StringViewLatin1);
typedef DFG_CLASS_NAME(StringView)<char, StringUtf8>	    DFG_CLASS_NAME(StringViewUtf8);

} // namespace
