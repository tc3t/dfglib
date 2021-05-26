#ifndef DFG_STR_MROMU4TC
#define DFG_STR_MROMU4TC

#include "dfgBase.hpp"
#include <cstring>
#include "math.hpp"
#include <string>
#include "math/roundedUpToMultiple.hpp"
#include "str/strCmp.hpp"
#include "str/strLen.hpp"
#include "ReadOnlySzParam.hpp"
#include "numericTypeTools.hpp"
#include <algorithm>
#include "preprocessor/compilerInfoMsvc.hpp"

#if DFG_BUILD_OPT_USE_BOOST==1
    DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
        #include <boost/lexical_cast.hpp>
    DFG_END_INCLUDE_WITH_DISABLED_WARNINGS
#endif
#include <cstdio>
#include <limits>
#include <cstdarg>

#define DFG_TEXT_ANSI(str)	str	// Intended as marker for string literals that are promised
                                // (by the programmer) to be ANSI-encoded.
#define DFG_DEFINE_STRING_LITERAL_C(name, str) const char name[] = str;
#define DFG_DEFINE_STRING_LITERAL_W(name, str) const wchar_t name[] = L##str;

// TODO: improve std::to_chars() detection, currently enabled only on MSVC
// Note: std::to_chars() overload for floating point with precision argument doesn't seem to be available MSVC2017.9 (15.9) so using another flag for that.
#if ((DFG_MSVC_VER >= DFG_MSVC_VER_2019_4 || (DFG_MSVC_VER < DFG_MSVC_VER_VC16_0 && DFG_MSVC_VER >= DFG_MSVC_VER_2017_8)) && _MSVC_LANG >= 201703L)
    #define DFG_TOSTR_USING_TO_CHARS 1
    #define DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG (DFG_MSVC_VER >= DFG_MSVC_VER_2019_4)
    #include <charconv>
#else
    #define DFG_TOSTR_USING_TO_CHARS 0
    #define DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG 0
#endif

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
Char_T* strCpyAllThatFit(Char_T(&szDest)[N], const Char_T* pszSrc)
{
    return strCpyAllThatFit(szDest, N, pszSrc);
}

namespace DFG_DETAIL_NS
{
    template <class FloatingPoint_T> inline const char* floatingPointTypeToSprintfType()
    {
        DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(FloatingPoint_T, "floatingPointTypeToSprintfType() not implement for given type");
        return nullptr; // To avoid warnings in some compilers.
    }
    template <> inline const char* floatingPointTypeToSprintfType<float>()       { return "g"; }
    template <> inline const char* floatingPointTypeToSprintfType<double>()      { return "g"; }
    template <> inline const char* floatingPointTypeToSprintfType<long double>() { return "Lg"; }

    // Effectively the same as std::snprintf(), i.e. guarantees that buffer is null terminated (given sizeInCharacters > 0)
    inline int sprintf_s(char* buffer, size_t sizeInCharacters, const char* pszFormat, ...)
    {
        va_list args;
        va_start(args, pszFormat);
#if defined(__MINGW32__)
        // Handling long double's is buggy in MinGW (at least in 4.8.1)
        // https://stackoverflow.com/questions/30080618/long-double-is-printed-incorrectly-with-iostreams-on-mingw
        // https://stackoverflow.com/questions/4089174/printf-and-long-double
        // As a workaround using special MinGW version.
        auto rv = __mingw_vsnprintf(buffer, sizeInCharacters, pszFormat, args);
#else
        auto rv = vsnprintf(buffer, sizeInCharacters, pszFormat, args);
#endif
        va_end(args);
        return rv;
    }

    template <class T> inline T strToFloatingPoint(const char* /*psz*/)
    { 
        DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(T, "strToFloatingPoint: implementation is not available for given type");
    }

    template <> inline float strToFloatingPoint<float>(const char* psz)
    { 
    #if defined(_MSC_VER) && (DFG_MSVC_VER < DFG_MSVC_VER_2013)
        return static_cast<float>(std::strtod(psz, nullptr));
    #else
        return std::strtof(psz, nullptr);
    #endif
    }

    template <> inline double strToFloatingPoint<double>(const char* psz)
    { 
        return std::strtod(psz, nullptr);
    }

    template <> inline long double strToFloatingPoint<long double>(const char* psz)
    {
    #if defined(_MSC_VER) && (DFG_MSVC_VER < DFG_MSVC_VER_2013)
        DFG_STATIC_ASSERT(sizeof(double) == sizeof(long double), "In MSVC2010 and MSVC2012 long double is expected to be identical to double");
        return std::strtod(psz, nullptr);
    #else
        return std::strtold(psz, nullptr);
    #endif
    }

    // floatingPointToStr() to use when std::to_chars() is not available: converting using brittle locale-dependent sprintf().
    template <class T>
    inline char* floatingPointToStrFallback(const T val, char* psz, const size_t nDstSize, const int nPrecParam = -1)
    {
        if (DFG_MODULE_NS(math)::isInf(val))
            return strCpyAllThatFit(psz, nDstSize, (val > 0) ? "inf" : "-inf");
        else if (DFG_MODULE_NS(math)::isNan(val))
            return strCpyAllThatFit(psz, nDstSize, "nan");
        auto nPrec = nPrecParam;
        if (nPrec < 0)
            nPrec = static_cast<decltype(nPrec)>(DFG_MODULE_NS(math)::DFG_CLASS_NAME(RoundedUpToMultipleT) < std::numeric_limits<T>::digits * 30103, 100000 > ::value / 100000 + 1); // 30103 == round(log10(2) * 100000)
            //nPrec = std::numeric_limits<T>::max_digits10; // This seems to be too-little-by-one in VC2010 so use the manual version above.
        else if (nPrec >= 1000) // Limit precision to 3 digits.
            nPrec = 999;
        char szFormat[8] = "";
        DFG_DETAIL_NS::sprintf_s(szFormat, sizeof(szFormat), "%%.%u%s", nPrec, DFG_DETAIL_NS::floatingPointTypeToSprintfType<T>());
        if (static_cast<size_t>(DFG_DETAIL_NS::sprintf_s(psz, nDstSize, szFormat, val)) >= nDstSize)
        {
            psz[0] = '\0'; // Result did't fit -> returning empty string.
            return psz;
        }
        std::replace(psz, psz + std::strlen(psz), ',', '.'); // Hack: fix for locales where decimal separator is comma. Locales using other than dot or comma remain broken.

        // Manual tweak: if using default precision and string is suspiciously long, try if shorter precision is enough in the sense that
        //               std::atof(pszLonger) == std::atof(pszShorter).
        if (nPrecParam == -1 && std::strlen(psz) > std::numeric_limits<T>::digits10)
        {
            char szShortFormat[8] = "";
            DFG_DETAIL_NS::sprintf_s(szShortFormat, sizeof(szShortFormat), "%%.%u%s", std::numeric_limits<T>::digits10, DFG_DETAIL_NS::floatingPointTypeToSprintfType<T>());
            DFG_DETAIL_NS::sprintf_s(psz, nDstSize, szShortFormat, val);
            if (DFG_DETAIL_NS::strToFloatingPoint<T>(psz) == val)
                return psz;
            DFG_DETAIL_NS::sprintf_s(psz, nDstSize, szFormat, val); // Shorter was too short, reconvert.
        }
        return psz;
    }

    template <class T, class Str_T>
    Str_T& toStrTImpl(const T& obj, Str_T& str)
    {
#if DFG_BUILD_OPT_USE_BOOST==1
        str = boost::lexical_cast<Str_T>(obj);
        return str;
#else
        DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(T, "toStrTImpl: implementation is not available when building without Boost");
#endif
    }

} // DFG_DETAIL_NS

template <class T> std::string toStrC(const T& obj);
template <class T> std::wstring toStrW(const T& obj);

template <class T, class Str_T>
Str_T& toStr(const T& obj, Str_T& str)
{
    return DFG_DETAIL_NS::toStrTImpl(obj, str);
}

template <class T>
std::string& toStr(const T& obj, std::string& s)
{
    s = toStrC(obj);
    return s;
}

template <class T>
std::wstring& toStr(const T& obj, std::wstring& s)
{
    s = toStrW(obj);
    return s;
}

template <class Str_T, class T>
Str_T toStrT(const T& obj)
{
    Str_T str;
    return toStrT(obj, str);
}

// Converts double to string using sprintf() and given format string and writes to result to given buffer.
// Note: Result format is dependent on locale so the output string might not be convertible back to double with strTo().
template <size_t N> char* toStr(const double val, char(&buf)[N], const char* pszSprintfFormat) // TODO: test
{
    DFG_DETAIL_NS::sprintf_s(buf, N, pszSprintfFormat, val);
    return buf;
}

// Converts a double to string so that std::atof(toStr(val,...)) == val for all non-NaN, finite numbers.
// The returned string representation is, roughly speaking, intended to be shortest possible fulfilling the above condition expect for integers, 
// for which formats such as 1000000 may be preferred to scientific format 1e6.
// For +- infinity, return value is inf/-inf.
// For NaN's, return value begins with "nan"
// Note: If string representation does not fit to buffer, resulting string will be empty (given nDstSize > 0).
template <class T>
inline char* floatingPointToStr(const T val, char* psz, const size_t nDstSize, const int nPrecParam = -1)
{
    if (nDstSize < 1 || !psz)
        return psz;
#if DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG == 1
    const auto tcResult = (nPrecParam == -1) ? std::to_chars(psz, psz + nDstSize, val) : std::to_chars(psz, psz + nDstSize, val, std::chars_format::general, nPrecParam);
    if (tcResult.ec == std::errc() && static_cast<size_t>(std::distance(psz, tcResult.ptr)) < nDstSize)
        *tcResult.ptr = '\0';
    else
        *psz = '\0';
    return psz;
#elif DFG_TOSTR_USING_TO_CHARS == 1
    // In this branch only default-precision overload is available.
    if (nPrecParam == -1)
    {
        const auto tcResult = (nPrecParam == -1) ? std::to_chars(psz, psz + nDstSize, val) : std::to_chars(psz, psz + nDstSize, val);
        if (tcResult.ec == std::errc() && static_cast<size_t>(std::distance(psz, tcResult.ptr)) < nDstSize)
            *tcResult.ptr = '\0';
        else
            *psz = '\0';
        return psz;
    }
    else
        return DFG_DETAIL_NS::floatingPointToStrFallback(val, psz, nDstSize, nPrecParam);
#else
    return DFG_DETAIL_NS::floatingPointToStrFallback(val, psz, nDstSize, nPrecParam);
#endif
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

typedef int ItoaReturnValue;
enum ItoaError
{
    ItoaError_badRadix = -1,
    ItoaError_emptyOutputBuffer = -2,
    ItoaError_bufferTooSmall = -3
};

// Converts given integer value to string-like representation using given digit characters.
// @param value Value to convert.
// @radix Radix (base)
// @param digitIndexToRepresentation Function that returns digit-representation for values in range [0, radix - 1] (e.g. for base 10 could be [](const size_t i) { return static_cast<char>('0' + i); })
// @param sign Sign (minus) character (in common cases normally '-')
// @param outBegin Output iterator to beginning of write sequence.
// @param outEnd End iterator for output.
// @return On success, number of digits written to outBuffer, otherwise ItoaError
template <class OutIter_T, class Int_T, class Func_T, class Sign_T>
inline ItoaReturnValue intToRadixRepresentation(const Int_T value, const size_t radix, const Func_T digitIndexToRepresentation, const Sign_T sign, const OutIter_T outBegin, const OutIter_T outEnd)
{
    if (radix < 2)
        return ItoaError_badRadix;
    if (isAtEnd(outBegin, outEnd))
        return ItoaError_emptyOutputBuffer;
    if (value == 0)
    {
        *outBegin = digitIndexToRepresentation(0);
        return 1;
    }

    auto p = outBegin;
    int rv = 0;
    typedef typename std::make_unsigned<Int_T>::type UnsignedType;
    UnsignedType uvalue = ::DFG_ROOT_NS::absAsUnsigned(value);
    while (uvalue != 0)
    {
        if (isAtEnd(p, outEnd))
        {
            rv = ItoaError_bufferTooSmall;
            break;
        }
        const size_t rem = uvalue % radix;
        *p++ = digitIndexToRepresentation(rem);
        uvalue = static_cast<UnsignedType>(uvalue / radix);
    }
    if (value < 0)
    {
        if (!isAtEnd(p, outEnd))
            *p++ = sign;
        else
            rv = ItoaError_bufferTooSmall;
    }
    std::reverse(outBegin, p); // buffer[0] has least significant digit so reverse to get correct representation.
    return (rv >= 0) ? static_cast<int32>(std::distance(outBegin, p)) : rv;
}

// Convenience version returning conversion return value and automatically allocated string (instead of relying on caller given output).
template <class Char_T, class Int_T, class Func_T, class Sign_T>
inline std::pair<ItoaReturnValue, std::basic_string<Char_T>> intToRadixRepresentation(const Int_T value, const size_t radix, const Func_T digitIndexToRepresentation, const Sign_T sign)
{
    typedef std::basic_string<Char_T> StringType;
    Char_T buffer[8 * sizeof(Int_T) + 1]; // Simply reserve maximum length that we can get, i.e. that of base2, +1 for sign.
    auto rv = intToRadixRepresentation(value, radix, digitIndexToRepresentation, sign, std::begin(buffer), std::end(buffer));
    return std::pair<ItoaReturnValue, StringType>(rv, (rv > 0) ? StringType(buffer, buffer + rv) : StringType());
}

// Convenience version for intToRadixRepresentation() limiting radix to 2-36 and writing result as null terminated string (always when given non-empty buffer).
template <class Int_T, class Char_T>
inline ItoaReturnValue itoaSz(const Int_T value, const int radix, Char_T* buffer, const size_t bufCharCount)
{
    if (radix < 2 || radix > 36)
        return ItoaError_badRadix;
    if (bufCharCount == 0)
        return ItoaError_bufferTooSmall;
    const auto rv = intToRadixRepresentation(value, static_cast<size_t>(radix), [](const size_t i) { return "0123456789abcdefghijklmnopqrstuvwxyz"[i]; }, '-', buffer, buffer + bufCharCount);
    if (rv < 0)
        return rv;
    if (static_cast<size_t>(rv) == bufCharCount)
    {
        buffer[0] = '\0';
        return ItoaError_bufferTooSmall;
    }
    buffer[rv] = '\0';
    return rv;
}

// Mimics _itoa_s -family functions.
template <class Int_T, class Char_T>
int itoa(const Int_T value, Char_T* buffer, const size_t bufCharCount, const int radix)
{
    const auto rv = itoaSz(value, radix, buffer, bufCharCount);
    return (rv >= 0) ? 0 : EINVAL;
}

namespace DFG_DETAIL_NS
{
#define DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTION(INPUTTYPE, CHARTYPE, FUNCNAME, IMPLFUNC) \
    inline int FUNCNAME(const INPUTTYPE value, CHARTYPE* buffer, size_t sizeInCharacters, int radix) \
    { \
        return IMPLFUNC(value, buffer, sizeInCharacters, radix); \
    }

#define DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTIONS(f1, f2, f3, f4, f5, f6, f7, f8) \
    DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTION(int32,   char,         i32toa_s,   f1); \
    DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTION(int32,   wchar_t,      i32tow_s,   f2); \
    DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTION(uint32,  char,         ui32toa_s,  f3); \
    DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTION(uint32,  wchar_t,      ui32tow_s,  f4); \
    DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTION(int64,   char,         i64toa_s,   f5); \
    DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTION(int64,   wchar_t,      i64tow_s,   f6); \
    DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTION(uint64,  char,         ui64toa_s,  f7); \
    DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTION(uint64,  wchar_t,      ui64tow_s,  f8);

#ifdef _WIN32
    DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTIONS(_itoa_s, _itow_s, _ultoa_s, _ultow_s, _i64toa_s, _i64tow_s, _ui64toa_s, _ui64tow_s)
#else
    DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTIONS((itoa<int32, char>), (itoa<int32, wchar_t>), (itoa<uint32, char>), (itoa<uint32, wchar_t>), (itoa<int64, char>), (itoa<int64, wchar_t>), (itoa<uint64, char>), (itoa<uint64, wchar_t>))
#endif

#undef DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTIONS
#undef DFG_TEMP_DEFINE_ITOA_LIKE_FUNCTION

template <size_t IntSize_T, bool IsSigned_T> inline char*    intToStr(const typename IntegerTypeBySizeAndSign<IntSize_T, IsSigned_T>::type val, char*    buf, const size_t nBufCount, const int param);
template <size_t IntSize_T, bool IsSigned_T> inline wchar_t* intToStr(const typename IntegerTypeBySizeAndSign<IntSize_T, IsSigned_T>::type val, wchar_t* buf, const size_t nBufCount, const int param);

#define DFG_TEMP_CREATE_INTTOSTR_IMPL(INTSIZE, ISSIGNED, FUNC_C, FUNC_W) \
    template <> \
    inline char* intToStr<INTSIZE, ISSIGNED>(const typename IntegerTypeBySizeAndSign<INTSIZE, ISSIGNED>::type val, char* buf, const size_t nBufCount, const int param) \
    { \
        FUNC_C(val, buf, nBufCount, param); return buf; \
    } \
    template <> \
    inline wchar_t* intToStr<INTSIZE, ISSIGNED>(const typename IntegerTypeBySizeAndSign<INTSIZE, ISSIGNED>::type val, wchar_t* buf, const size_t nBufCount, const int param) \
    { \
        FUNC_W(val, buf, nBufCount, param); return buf; \
    }

DFG_TEMP_CREATE_INTTOSTR_IMPL(2, true,  DFG_DETAIL_NS::i32toa_s,  DFG_DETAIL_NS::i32tow_s)
DFG_TEMP_CREATE_INTTOSTR_IMPL(2, false, DFG_DETAIL_NS::ui32toa_s, DFG_DETAIL_NS::ui32tow_s)
DFG_TEMP_CREATE_INTTOSTR_IMPL(4, true,  DFG_DETAIL_NS::i32toa_s,  DFG_DETAIL_NS::i32tow_s)
DFG_TEMP_CREATE_INTTOSTR_IMPL(4, false, DFG_DETAIL_NS::ui32toa_s, DFG_DETAIL_NS::ui32tow_s)
DFG_TEMP_CREATE_INTTOSTR_IMPL(8, true,  DFG_DETAIL_NS::i64toa_s,  DFG_DETAIL_NS::i64tow_s)
DFG_TEMP_CREATE_INTTOSTR_IMPL(8, false, DFG_DETAIL_NS::ui64toa_s, DFG_DETAIL_NS::ui64tow_s)

#undef DFG_TEMP_CREATE_INTTOSTR_IMPL

// Converts an integer to string representation.
// Note: If string representation does not fit to buffer, behaviour is undefined.
// Base can be range [2, 36]. If wider base is needed, see intToRadixRepresentation()
template <class T>
inline char* integerToStr(const T val, char* pDst, const size_t nDstSize, const int nBase = 10)
{
    if (nDstSize < 1 || !pDst)
        return pDst;
#if DFG_TOSTR_USING_TO_CHARS == 1
    const auto tcResult = std::to_chars(pDst, pDst + nDstSize, val, nBase);
    if (tcResult.ec == std::errc() && static_cast<size_t>(std::distance(pDst, tcResult.ptr)) < nDstSize)
        *tcResult.ptr = '\0';
    else
        *pDst = '\0';
    return pDst;
#else // Case: not using std::to_chars()
    return DFG_DETAIL_NS::intToStr<sizeof(T), std::is_signed<T>::value>(val, pDst, nDstSize, nBase);
#endif
}

} // namespace DFG_DETAIL_NS 

#define DFG_INTERNAL_DEFINE_TOSTR(CHAR, TYPE, FUNC, PARAM) \
    inline              CHAR* toStr(TYPE val, CHAR* buf, const size_t nBufCount,    const int param = PARAM)  { FUNC(val, buf, nBufCount, param); return buf; } \
    template <size_t N> CHAR* toStr(TYPE val, CHAR (&buf)[N],                       const int param = PARAM)  { return toStr(val, buf, N, param); }

#define DFG_INTERNAL_DEFINE_TOSTR_INT(TYPE) \
    DFG_INTERNAL_DEFINE_TOSTR(char,    TYPE, DFG_DETAIL_NS::integerToStr, 10) \
    DFG_INTERNAL_DEFINE_TOSTR(wchar_t, TYPE, (DFG_DETAIL_NS::intToStr<sizeof(TYPE), std::is_signed<TYPE>::value>), 10)

DFG_INTERNAL_DEFINE_TOSTR_INT(short);
DFG_INTERNAL_DEFINE_TOSTR_INT(unsigned short);
DFG_INTERNAL_DEFINE_TOSTR_INT(int);
DFG_INTERNAL_DEFINE_TOSTR_INT(unsigned int);
DFG_INTERNAL_DEFINE_TOSTR_INT(long);
DFG_INTERNAL_DEFINE_TOSTR_INT(unsigned long);
DFG_INTERNAL_DEFINE_TOSTR_INT(long long);
DFG_INTERNAL_DEFINE_TOSTR_INT(unsigned long long);
DFG_INTERNAL_DEFINE_TOSTR(char,     float,              floatingPointToStr<float>,          -1);
DFG_INTERNAL_DEFINE_TOSTR(char,     double,             floatingPointToStr<double>,         -1);
DFG_INTERNAL_DEFINE_TOSTR(char,     long double,        floatingPointToStr<long double>,    -1);

#undef DFG_INTERNAL_DEFINE_TOSTR_INT
#undef DFG_INTERNAL_DEFINE_TOSTR

namespace DFG_DETAIL_NS
{
    struct ToStrConversionClass_floatingPoint {};
    struct ToStrConversionClass_integer {};
    struct ToStrConversionClass_generic {};

    template <class T> struct ToStrConversionClass
    {
        using type = typename std::conditional<
            std::is_floating_point<T>::value,
            ToStrConversionClass_floatingPoint,
            typename std::conditional<std::is_integral<T>::value, ToStrConversionClass_integer, ToStrConversionClass_generic>::type
        >::type;
    };

    template <class T> std::string toStrCImpl(const T& d, const ToStrConversionClass_floatingPoint)
    {
        char sz[32] = "";
        toStr(d, sz);
        return sz;
    }

    template <class T> std::string toStrCImpl(const T& i, const ToStrConversionClass_integer)
    {
        char sz[32] = "";
        toStr(i, sz);
        return sz;
    }

    template <class T> std::string toStrCImpl(const T& obj, const ToStrConversionClass_generic)
    {
        std::string s;
        return DFG_DETAIL_NS::toStrTImpl(obj, s);
    }
} // namespace DFG_DETAIL_NS

template <class T> std::string toStrC(const T& obj) { return DFG_DETAIL_NS::toStrCImpl(obj, typename DFG_DETAIL_NS::ToStrConversionClass<T>::type()); }

template <class T>
std::wstring toStrW(const T& obj)
{
    std::wstring s;
    return DFG_DETAIL_NS::toStrTImpl(obj, s);
}

template <size_t N> inline char*    strCpy(char     (&dest)[N], NonNullCStr pszSrc)     { return strcpy(dest, pszSrc); }
template <size_t N> inline wchar_t* strCpy(wchar_t  (&dest)[N], NonNullCStrW pszSrc)    { return wcscpy(dest, pszSrc); }

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
    return beginsWith(toCharPtr_raw(toCstr(sSearchFrom)), toCharPtr_raw(toCstr(sSearchFor)));
}

template <class Char_T, class Str_T>
bool beginsWith(const DFG_CLASS_NAME(StringView)<Char_T, Str_T>& sSearchFrom, const DFG_CLASS_NAME(StringView)<Char_T, Str_T>& sSearchFor)
{
    auto iterForRaw = sSearchFor.beginRaw();
    auto iterEndForRaw = sSearchFor.endRaw();
    const auto nForSize = std::distance(iterForRaw, iterEndForRaw);
    if (nForSize == 0)
        return true;
    auto iterFromRaw = sSearchFrom.beginRaw();
    auto iterEndFromRaw = sSearchFrom.endRaw();
    const auto nFromSize = std::distance(iterFromRaw, iterEndFromRaw);
    return nFromSize >= nForSize && std::equal(iterForRaw, iterEndForRaw, iterFromRaw);
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
