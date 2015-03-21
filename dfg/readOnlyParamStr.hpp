#pragma once

#include "dfgDefs.hpp"
#include "dfgBaseTypedefs.hpp"
#include "str/strlen.hpp"
#include "buildConfig.hpp"
#include "build/utils.hpp"
#include <string>

DFG_ROOT_NS_BEGIN {

template <class Char_T, class String_T>
const Char_T* readOnlyParamStrConverter(const String_T&)
{ 
	DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(String_T, "No conversion to ReadOnlyParamStr exists for given type");
}

// Returns length in chars.
template <class Char_T, class String_T>
size_t readOnlyParamStrLength(const String_T& str)
{
	return DFG_SUB_NS_NAME(str)::strLen(str);
}

template <> inline ConstCharPtr readOnlyParamStrConverter<char, ConstCharPtr>(const ConstCharPtr& psz) { return psz; }
template <> inline ConstWCharPtr readOnlyParamStrConverter<wchar_t, ConstWCharPtr>(const ConstWCharPtr& psz) { return psz; }
template <> inline ConstCharPtr readOnlyParamStrConverter<char, std::string>(const std::string& s) { return s.c_str(); }
template <> inline ConstWCharPtr readOnlyParamStrConverter<wchar_t, std::wstring>(const std::wstring& s) { return s.c_str(); }

/*
Class to be used as a convenient replacement of any concrete string type
in such function parameter that is essentially a c-string compatible, read only string parameter.

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
class DFG_CLASS_NAME(ReadOnlyParamStr)
{
public:
	typedef const Char_T* iterator;
	typedef const Char_T* const_iterator;

	template <size_t N>
	DFG_CLASS_NAME(ReadOnlyParamStr)(const Char_T(&arr)[N]) :
		m_pSz(arr)
	{
	}

	DFG_CLASS_NAME(ReadOnlyParamStr)(const Char_T* psz) :
		m_pSz(psz)
	{
	}

	DFG_CLASS_NAME(ReadOnlyParamStr)(const std::basic_string<Char_T>& s) :
		m_pSz(readOnlyParamStrConverter<Char_T>(s))
	{
	}
	
	// Return c-string length of string.
	size_t length() const
	{
		return strLen(m_pSz);
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

// Like DFG_CLASS_NAME(ReadOnlyParamStr), but also stores the size. For string classes, uses the
// size returned by related member function, not the null terminated length; these
// may differ in case of embedded null chars.
template <class Char_T>
class DFG_CLASS_NAME(ReadOnlyParamStrWithSize) : public DFG_CLASS_NAME(ReadOnlyParamStr)<Char_T>
{
public:
	typedef DFG_CLASS_NAME(ReadOnlyParamStr)<Char_T> BaseClass;
	using typename BaseClass::iterator;
	using typename BaseClass::const_iterator;

	DFG_CLASS_NAME(ReadOnlyParamStrWithSize)(const std::basic_string<Char_T>& s) :
		BaseClass(readOnlyParamStrConverter<Char_T>(s)),
		m_nSize(readOnlyParamStrLength<Char_T>(s))
	{

	}

	template <size_t N>
	DFG_CLASS_NAME(ReadOnlyParamStrWithSize)(const Char_T(&arr)[N]) :
		BaseClass(readOnlyParamStrConverter<Char_T, const Char_T*>(arr)),
		m_nSize(readOnlyParamStrLength<Char_T>(arr))
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

typedef DFG_CLASS_NAME(ReadOnlyParamStr)<char>				DFG_CLASS_NAME(ReadOnlyParamStrC);
typedef DFG_CLASS_NAME(ReadOnlyParamStr)<wchar_t>			DFG_CLASS_NAME(ReadOnlyParamStrW);

typedef DFG_CLASS_NAME(ReadOnlyParamStrWithSize)<char>		DFG_CLASS_NAME(ReadOnlyParamStrWithSizeC);
typedef DFG_CLASS_NAME(ReadOnlyParamStrWithSize)<wchar_t>	DFG_CLASS_NAME(ReadOnlyParamStrWithSizeW);

} // namespace
