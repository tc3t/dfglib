#ifndef DFG_STR_MROMU4TC
#define DFG_STR_MROMU4TC

#include "dfgBase.hpp"
#include <cstring>
#include "math.hpp"
#include <string>
#include "math/roundedUpToMultiple.hpp"
#include "str/strCmp.hpp"
#include "str/strlen.hpp"
#include "ReadOnlySzParam.hpp"
#include <boost/lexical_cast.hpp>
#include <cstdio>
#include <limits>

#define DFG_TEXT_ANSI(str)	str	// Intended as marker for string literals that are promised
                                // (by the programmer) to be ANSI-encoded.
#define DFG_DEFINE_STRING_LITERAL_C(name, str) const char name[] = str;
#define DFG_DEFINE_STRING_LITERAL_W(name, str) const wchar_t name[] = L##str;

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(str) {

const char nullCh = '\0';
template <class Char> bool IsNullCh(const Char ch) {return (ch == nullCh);}

// Typedefs character type of given string type as 'type'.
template <class T> struct CharType {};
template <> struct CharType<std::string> {typedef std::string::value_type type;};
template <> struct CharType<std::wstring> {typedef std::wstring::value_type type;};

// Has sizeof(CharType<StrintT>) of static const member value.
template <class T> struct CharSize {};
template <> struct CharSize<std::string>	{DFG_STATIC_ASSERT(sizeof(std::string::value_type) == sizeof(char), ""); static const size_t value = sizeof(CharType<std::string>);};
template <> struct CharSize<std::wstring>	{DFG_STATIC_ASSERT(sizeof(std::wstring::value_type) == sizeof(wchar_t), ""); static const size_t value = sizeof(CharType<std::wstring>);};

// Returns char size for given string type.
template <class T> inline size_t charSize(const T&) {return CharSize<T>::value;}

// Returns param.c_str() or the parameter unmodified in case of raw pointer type or typed SzPtr.
// [in] : Pointer to null terminated string or string object.
// return : Pointer to corresponding const C string.
template <class T> inline auto toCstr(const T& s) -> decltype(s.c_str()) { return s.c_str(); }
template <class T> inline const T* toCstr(const T* psz) {return psz;}
template <class Char_T, CharPtrType Type_T> SzPtrT<Char_T, Type_T> toCstr(const SzPtrT<Char_T, Type_T>& tpsz) { return tpsz; }

// Copies pszSrc to pszDest. If pszSrc does not fit pszDest, copies as many characters as fit. In all cases expect when pszDest is nullptr or zero-sized, pszDest will be null terminated.
template <class Char_T>
Char_T* strCpyAllThatFit(Char_T* const pDstBegin, const size_t nSizeInChars, const Char_T* pszSrc)
{
    if (nSizeInChars < 1)
        return pDstBegin;
    if (pDstBegin == nullptr)
        return nullptr;
    if (pszSrc == nullptr)
    {
        *pDstBegin = '\0';
        return pDstBegin;
    }

    auto pDest = pDstBegin;
    const auto pNullPosIfDoesntFit = pDstBegin + (nSizeInChars - 1);
    for (; pDest != pNullPosIfDoesntFit && *pszSrc != '\0'; ++pDest, ++pszSrc)
        *pDest = *pszSrc;
    *pDest = '\0';
    return pDstBegin;
}

template <class Char_T, size_t N>
Char_T* strCpyAllThatFit(Char_T (&szDest)[N], const Char_T* pszSrc)
{
    return strCpyAllThatFit(szDest, N, pszSrc);
}

// Converts a double to string so that std::atof(toStr(val,...)) == val for all non-NaN, finite numbers.
// The returned string representation is, roughly speaking, intended to be shortest possible fulfilling the above condition expect for integers, 
// for which formats such as 1000000 may be preferred to scientific format 1e6.
// For +- infinity, return value is inf/-inf.
// For NaN's, return value begins with "nan"
template <class T>
inline char* floatingPointToStr(const T val, char* psz, const size_t nDstSize, const int nPrecParam = -1)
{
    if (nDstSize < 1)
        return psz;
    if (DFG_MODULE_NS(math)::isInf(val))
        return strCpyAllThatFit(psz, nDstSize, (val > 0) ? "inf" : "-inf");
    else if (DFG_MODULE_NS(math)::isNan(val))
        return strCpyAllThatFit(psz, nDstSize, "nan");
    auto nPrec = nPrecParam;
    if (nPrec < 0)
        nPrec = static_cast<decltype(nPrec)>(DFG_MODULE_NS(math)::DFG_CLASS_NAME(RoundedUpToMultipleT)<std::numeric_limits<T>::digits * 30103, 100000>::value / 100000 + 1); // 30103 == round(log10(2) * 100000)
        //nPrec = std::numeric_limits<T>::max_digits10; // This seems to be too-little-by-one in VC2010 so use the manual version above.
    else if (nPrec >= 1000) // Limit precision to 3 digits.
        nPrec = 999;
    char szFormat[8] = "";
    std::sprintf(szFormat, "%%.%ug", nPrec);
    sprintf_s(psz, nDstSize, szFormat, val);

    // Manual tweak: if using default precision and string is suspiciously long, try if shorter precision is enough in the sense that
    //               std::atof(pszLonger) == std::atof(pszShorter).
    if (nPrecParam == -1 && std::strlen(psz) > std::numeric_limits<T>::digits10)
    {
        char szShortFormat[8] = "";
        std::sprintf(szShortFormat, "%%.%ug", std::numeric_limits<T>::digits10);
        sprintf_s(psz, nDstSize, szShortFormat, val);
        if (static_cast<T>(std::atof(psz)) == val)
            return psz;
        sprintf_s(psz, nDstSize, szFormat, val); // Shorter was too short, reconvert.
    }
    return psz;
}

template <class T, size_t N>
inline char* floatingPointToStr(const T val, char (&sz)[N], const int nPrecParam = -1)
{
    return floatingPointToStr(val, sz, N, nPrecParam);
}

template <class Str_T>
inline Str_T floatingPointToStr(const double val, const int nPrecParam = -1)
{
    char szBuf[32] = "";
    floatingPointToStr(val, szBuf, nPrecParam);
    Str_T s(SzPtrAscii(szBuf));
    return s;
}

#define DFG_INTERNAL_DEFINE_TOSTR(CHAR, TYPE, FUNC, PARAM) \
    inline              CHAR* toStr(TYPE val, CHAR* buf, const size_t nBufCount,    const int param = PARAM)  { FUNC(val, buf, nBufCount, param); return buf; } \
    template <size_t N> CHAR* toStr(TYPE val, CHAR (&buf)[N],                       const int param = PARAM)  { return toStr(val, buf, N, param); }

DFG_INTERNAL_DEFINE_TOSTR(char,     int32,      _itoa_s,    10);
DFG_INTERNAL_DEFINE_TOSTR(wchar_t,  int32,      _itow_s,    10);
DFG_INTERNAL_DEFINE_TOSTR(char,     uint32,     _ultoa_s,   10);
DFG_INTERNAL_DEFINE_TOSTR(wchar_t,  uint32,     _ultow_s,   10);
DFG_INTERNAL_DEFINE_TOSTR(char,     int64,      _i64toa_s,  10);
DFG_INTERNAL_DEFINE_TOSTR(wchar_t,  int64,      _i64tow_s,  10);
DFG_INTERNAL_DEFINE_TOSTR(char,     uint64,     _ui64toa_s, 10);
DFG_INTERNAL_DEFINE_TOSTR(wchar_t,  uint64,     _ui64tow_s, 10);
DFG_INTERNAL_DEFINE_TOSTR(char,     float,          floatingPointToStr<float>,          -1);
DFG_INTERNAL_DEFINE_TOSTR(char,     double,         floatingPointToStr<double>,         -1);
DFG_INTERNAL_DEFINE_TOSTR(char,     long double,    floatingPointToStr<long double>,    -1);

#undef DFG_INTERNAL_DEFINE_TOSTR

template <size_t N> char* toStr(const double val, char(&buf)[N], const char* pszFormat) // TODO: test
{
    sprintf_s(buf, N, pszFormat, val);
    return buf;
}

template <class T, class Str_T>
Str_T& toStr(const T& obj, Str_T& str)
{
    str = boost::lexical_cast<Str_T>(obj);
    return str;
}

template <class Str_T, class T>
Str_T toStrT(const T& obj)
{
    return boost::lexical_cast<Str_T>(obj);
}

template <class T> std::string toStrCImpl(const T& d, const std::true_type) // Type is floating_point type.
{ 
    char sz[32] = "";
    toStr(d, sz);
    return sz;
}

template <class T> std::string toStrCImpl(const T& obj, const std::false_type) // Type is not floating point.
{
    return boost::lexical_cast<std::string>(obj);
}

template <class T> std::string toStrC(const T& obj) { return toStrCImpl(obj, std::is_floating_point<T>()); }

template <class T>
std::wstring toStrW(const T& obj)
{
    return boost::lexical_cast<std::wstring>(obj);
}

template <size_t N> inline char* strCpy(char (&dest)[N], NonNullCStr pszSrc) {return strcpy(dest, pszSrc);}
template <size_t N> inline wchar_t* strCpy(wchar_t (&dest)[N], NonNullCStrW pszSrc) {return wcscpy(dest, pszSrc);}

// Replaces occurrences of 'sOldSub' with 'sNewSub' in given string.
// Note: Behaviour is undefined if substrings overlap with 'str'.
template <class StrT, class StrTOld, class StrTNew>
void replaceSubStrsInplaceImpl(StrT& str, const StrTOld& sOldSub, const StrTNew& sNewSub)
{
    for(size_t i = 0; i<str.length();)
    {
        i = str.find(sOldSub, i);
        if (isValidIndex(str, i))
        {
            str.replace(i, strLen(sOldSub), sNewSub);
            i += strLen(sNewSub);
        }
        else
            break;
    }
}

// See replaceSubStrsInplaceImpl
template <class StrTOld, class StrTNew>
void replaceSubStrsInplace(std::string& str, const StrTOld& sOldSub, const StrTNew& sNewSub)
{
    replaceSubStrsInplaceImpl(str, sOldSub, sNewSub);
}

// See replaceSubStrsInplaceImpl
template <class StrTOld, class StrTNew>
void replaceSubStrsInplace(std::wstring& str, const StrTOld& sOldSub, const StrTNew& sNewSub)
{
    replaceSubStrsInplaceImpl(str, sOldSub, sNewSub);
}

// Replaces occurrences of 'sOldSub' with 'sNewSub' and returns the new string.
template <class StrT, class StrTOld, class StrTNew>
StrT replaceSubStrs(StrT str, const StrTOld& sOldSub, const StrTNew& sNewSub)
{
    replaceSubStrsInplace(str, sOldSub, sNewSub);
    return str;
}

// Converts given string enclosed in 'cEnclosing'-characters and if 'str' contains any
// enclosing characters, they are escaped as double.
// For example: if cEnclosing = ' and str "a'b", str will become "'a''b'"
template <class StrT> void toEnclosedTextInplace(StrT& str, const char cEnclosing)
{
    typename CharType<StrT>::type sOld[2] = {cEnclosing, '\0'};
    typename CharType<StrT>::type sNew[3] = {cEnclosing, cEnclosing, '\0'};
    replaceSubStrsInplace(str, sOld, sNew);
}

// Determines whether 'pszSearchFrom' starts with 'pszSearchFor'.
// If 'pszSearchFor' is empty, returns true.
// TODO: test
template <class Char_T> bool beginsWith(const Char_T* pszSearchFrom, const Char_T* pszSearchFor)
{
    // Note: There's no need to check whether pszSearchFrom reaches the end:
    //       If pszSearchFrom is shorter, at some point comparison (*pszSearchFrom != *pszSearchFor)
    //		 will be
    //       nullCh != nonNullCh
    //       and the loop will end.
    for(; *pszSearchFor != nullCh; ++pszSearchFor, ++pszSearchFrom)
    {
        if (*pszSearchFrom != *pszSearchFor)
            return false;
    }
    return true;
}

template <class Str0_T, class Str1_T> bool beginsWith(const Str0_T& sSearchFrom, const Str1_T& sSearchFor)
{
    return beginsWith(toCstr(sSearchFrom), toCstr(sSearchFor));
}

// Tests whether string is empty.
// [in] : Pointer to null terminated string or string object.
// return : True iff. string is empty(i.e. if it's length is zero).
inline bool isEmptyStr(const NonNullCStr pszNonNull) {return pszNonNull[0] == 0;}
inline bool isEmptyStr(const NonNullCStrW pszNonNull) {return pszNonNull[0] == 0;}

template <class Str_T>
inline bool isEmptyStr(const Str_T& str) { return str.empty(); }

template <class Char_T, CharPtrType Type_T> inline bool isEmptyStr(const SzPtrT<Char_T, Type_T>& tpsz) { return isEmptyStr(tpsz.c_str()); }


namespace detail
{
    template <class Char_T>
    Char_T* toLowerImpl(Char_T* const psz)
    {
        for(auto p = psz; *p != nullCh; ++p)
        {
            if (isupper(*p))
                *p = static_cast<Char_T>(tolower(*p));
        }
        return psz;
    }
}

// Converts upper case characters of given null terminated string to lower case characters.
// Behaviour is undefined for non-ascii characters.
inline ConstCharPtr  toLower(const NonNullStr psz)  {return detail::toLowerImpl(psz);}
inline ConstWCharPtr toLower(const NonNullStrW psz) {return detail::toLowerImpl(psz);}

// Like strcmp, but does lowercase comparison.
// Order of comparison involving non-alphabet letters is undefined.
// TODO: Test.
#ifdef _MSC_VER
    inline int strCmpLowerCase(const NonNullCStr psz1, const NonNullCStr psz2) {return _stricmp(psz1, psz2);}
    inline int strCmpLowerCase(const NonNullCStrW psz1, const NonNullCStrW psz2) {return _wcsicmp(psz1, psz2);}

    // TODO: Implement comparison based on size() instead of c_str(),
    // NOTE: Behaviour for strings with embedded null is subject to change.
    inline int strCmpLowerCase(const std::string& s1, const std::string& s2) {return _stricmp(s1.c_str(), s2.c_str());}
    inline int strCmpLowerCase(const std::wstring& s1, const std::wstring& s2) {return _wcsicmp(s1.c_str(), s2.c_str());}
#endif // _MSC_VER



/// Clears string so that it contains string of length zero after the call.
template <class String>
inline void clear(String& str) {str.clear();}

inline char* strChr(char* str, int ch) { return strchr(str, ch); }
inline const char* strChr(const char* str, int ch) { return strchr(str, ch); }
inline wchar_t* strChr(wchar_t* str, wchar_t ch) { return wcschr(str, ch); }
inline const wchar_t* strChr(const wchar_t* str, wchar_t ch) { return wcschr(str, ch); }

// Returns pointer to the first character in @p pszSearch not in string @p pszNotOf.
// If not found, returns pointer to ending null (not nullptr).
// TODO: test
template <class Char_T>
const Char_T* firstNotOf(const Char_T* pszSearch, const Char_T* const pszNotOf)
{
    for(; !isNullCh(*pszSearch); ++pszSearch)
    {
        if (strChr(pszNotOf, *pszSearch) == nullptr)
            return pszSearch;
    }
    return pszSearch;
}

// Overload that takes and returns modifiable char pointer.
// TODO: test
template <class Char_T>
Char_T* firstNotOf(Char_T* pszSearch, const Char_T* const pszNotOf)
{
    return const_cast<Char_T*>(firstNotOf(static_cast<const Char_T*>(pszSearch), pszNotOf));
}

// Replaces first occurence of given string 'pszWhat' with 'pszWith'
// TODO: Test
template <class Str_T, class Char_T>
Str_T& replaceFirst(Str_T& str, const Char_T* const pszWhat, const Char_T* const pszWith)
{
    const auto nPos = str.find(pszWhat);
    if (nPos != str.npos)
        str.replace(str.begin() + nPos, str.begin() + nPos + strLen(pszWhat), pszWith);
    return str;
}

// For given pointer to null terminated string, advances the pointer as long as current character is any of 
// characters in pszWhitespaces.
// Note: second parameter is template so that type resolution won't be ambiguous in case of char*, const char*
// Return: 
//			-If psz is nullptr, nullptr
//			-otherwise pointer character not in pszWhitespaces or pointer to terminating nullptr. In this case the
//			 return value is dereferencable.
//
template <class Char_T, class Char2_T>
Char_T* skipWhitespacesSz(Char_T* psz, const Char2_T* const pszWhitespaces)
{
    if (!psz)
        return psz;
    while (*psz != '\0' && strChr(pszWhitespaces, *psz) != nullptr)
        ++psz;
    return psz;
}

// Overload using " " as whitespaces.
template <class Char_T>
Char_T* skipWhitespacesSz(Char_T* psz)
{
    const Char_T sz[2] = { ' ', '\0' };
    return skipWhitespacesSz(psz, sz);
}

}} // module str

#include "str/strTo.hpp"

#endif // include guard
