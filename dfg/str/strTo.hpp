#ifndef DFG_STR_STRTO_LJIBPTYR
#define DFG_STR_STRTO_LJIBPTYR

#include "../dfgBase.hpp"
#if DFG_BUILD_OPT_USE_BOOST==1
    #include <boost/lexical_cast.hpp>
#else
    #include "../build/utils.hpp"
#endif
#include "../ReadOnlySzParam.hpp"
#include "../preprocessor/compilerInfoMsvc.hpp"

// For now optionally using std::from_chars() on MSVC2017 with version >= 7 and on MSVC2019 with version >= 4
// Note that with default build options at least MSVC2017 has _MSVC_LANG < 201703L so from_chars() won't be used by default.
// https://docs.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance?view=vs-2019
// https://docs.microsoft.com/en-us/cpp/overview/cpp-conformance-improvements?view=vs-2017#improvements_157
// Note that using _MSVC_LANG instead of __cplusplus because of reason listed here: https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
// This is coarse check that needs to be manually updated when std::from_chars() become available on other compilers.
#if ((DFG_MSVC_VER >= DFG_MSVC_VER_2019_4 || (DFG_MSVC_VER < DFG_MSVC_VER_VC16_0 && DFG_MSVC_VER >= DFG_MSVC_VER_2017_7)) && _MSVC_LANG >= 201703L)
    #define DFG_STRTO_USING_FROM_CHARS 1
    #include <charconv>
#else
    #define DFG_STRTO_USING_FROM_CHARS 0
#endif

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(str) {

template <class Str_T, class T>
T& strToByNoThrowLexCast(const Str_T& s, T& obj, bool* pSuccess = nullptr)
{
#if DFG_BUILD_OPT_USE_BOOST==1
    try
    {
        obj = boost::lexical_cast<T>(s);
        if (pSuccess)
            *pSuccess = true;
    }
    catch(const boost::bad_lexical_cast&)
    {
        if (pSuccess)
            *pSuccess = false;
        obj = T();
    }
    return obj;
#else
    DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(T, "strToByNoThrowLexCast: implementation is not available when building without Boost");
#endif
}

template <class T, class Char_T>
T& strToByNoThrowLexCastImpl(const DFG_CLASS_NAME(StringView)<Char_T>& sv, T& val, bool* pSuccess = nullptr)
{
    bool bSuccess = true;
    try
    {
        val = boost::lexical_cast<T>(sv.dataRaw(), sv.length());
    }
    catch (const boost::bad_lexical_cast&)
    {
        bSuccess = false;
        val = T();
    }
    if (pSuccess)
        *pSuccess = bSuccess;
    return val;
}

template <class T>
T& strToByNoThrowLexCast(const DFG_CLASS_NAME(StringViewC)& sv, T& val, bool* pSuccess = nullptr) { return strToByNoThrowLexCastImpl(sv, val, pSuccess); }

template <class T>
T& strToByNoThrowLexCast(const DFG_CLASS_NAME(StringViewW)& sv, T& val, bool* pSuccess = nullptr) { return strToByNoThrowLexCastImpl(sv, val, pSuccess); }

template <class Char_T>
bool& strToBoolNoThrowLexCast(const DFG_CLASS_NAME(StringView)<Char_T>& sv, bool& val, bool* pSuccess = nullptr)
{ 
    if (sv == DFG_STRING_LITERAL_BY_CHARTYPE(Char_T, "true"))
        val = true;
    else if (sv == DFG_STRING_LITERAL_BY_CHARTYPE(Char_T, "false"))
        val = false;
    else
        val = strToByNoThrowLexCastImpl(sv, val, pSuccess);
    return val;
}

template <> inline bool& strToByNoThrowLexCast(const DFG_CLASS_NAME(StringViewC)& sv, bool& val, bool* pSuccess) { return strToBoolNoThrowLexCast(sv, val, pSuccess); }
template <> inline bool& strToByNoThrowLexCast(const DFG_CLASS_NAME(StringViewW)& sv, bool& val, bool* pSuccess) { return strToBoolNoThrowLexCast(sv, val, pSuccess); }

template <class T>
T strToByNoThrowLexCast(const DFG_CLASS_NAME(ReadOnlySzParamC)& s, bool* pSuccess = nullptr)
{
    T val;
    return strToByNoThrowLexCast(s, val, pSuccess);
}

namespace DFG_DETAIL_NS
{
#if (DFG_STRTO_USING_FROM_CHARS != 1)
    class Locale
    {
    public:
        enum LocaleCategory
        {
    #ifdef _WIN32
            LocaleCategoryNumeric = LC_NUMERIC
    #else
            LocaleCategoryNumeric = LC_NUMERIC_MASK
    #endif
        };
    #ifdef _WIN32
        using LocaleImplT = _locale_t;
    #else
        using LocaleImplT = locale_t;
    #endif

        static LocaleImplT createLocaleImpl(const int category, const char* pszLocale)
        {
    #ifdef _WIN32
            return _create_locale(category, pszLocale);
    #else
            return newlocale(category, pszLocale, nullptr);
    #endif
        }

        static Locale createNumericClocale()
        {
            return Locale(LocaleCategoryNumeric, "C");
        }

        static void freeLocale(LocaleImplT locale)
        {
#if defined(_WIN32)
            _free_locale(locale);
#else
            freelocale(locale);
#endif
        }

        Locale(LocaleCategory category, const char* locale)
        {
            m_locale = createLocaleImpl(category, locale);
        }

        ~Locale()
        {
            freeLocale(m_locale);
        }

        LocaleImplT m_locale;
    };

    static const Locale::LocaleImplT& plainNumericLocale()
    {
        static Locale locale = Locale::createNumericClocale();
        return locale.m_locale;
    }
#endif // (DFG_STRTO_USING_FROM_CHARS != 1)


#if 0
    inline int wtoiImpl(const wchar_t* psz)
    {
        return static_cast<int>(wcstol(psz, nullptr, 10));
    }

    inline double wtofImpl(const wchar_t* psz)
    {
        #ifdef _MSC_VER
            return _wtof(psz);
        #else
            #error Not implemented
        #endif
    }

    inline int64 strtoi64Impl(const char* psz)
    {
        #ifdef _MSC_VER
            return _strtoi64(psz, nullptr, 10);
        #else
            #error Not implemented
        #endif
    }

    inline int64 wcstoi64Impl(const wchar_t* psz)
    {
        #ifdef _MSC_VER
            return _wcstoi64(psz, nullptr, 10);
        #else
            #error Not implemented
        #endif
    }

    inline uint64 strtoui64Impl(const char* psz)
    {
        #ifdef _MSC_VER
            return _strtoui64(psz, nullptr, 10);
        #else
            #error Not implemented
        #endif
    }

    inline uint64 wcstoui64Impl(const wchar_t* psz)
    {
        #ifdef _MSC_VER
            return _wcstoui64(psz, nullptr, 10);
        #else
            #error Not implemented
        #endif
    }
#endif

    template <class Char_T, class T>
    inline void convertImpl(DFG_CLASS_NAME(StringView)<Char_T> sv, T& t, bool* pSuccess = nullptr)
    {
        strToByNoThrowLexCast(sv, t, pSuccess);
    }

    inline void convertImpl(DFG_CLASS_NAME(StringView)<char> sv, double& t, bool* pSuccess = nullptr)
    {
        // While view itself is not necessarily null-terminated, the underlying string is and since
        // trailing spaces seem to be no problem for strtod(), passing the start pointer as such.
        
#if DFG_STRTO_USING_FROM_CHARS == 1
        t = std::numeric_limits<double>::quiet_NaN();
        const auto rv = std::from_chars(sv.data(), sv.endRaw(), t);
        if (pSuccess)
            *pSuccess = (rv.ptr == sv.endRaw() && rv.ec == std::errc());
#else
        char* pEnd;
    #if defined(_MSC_VER)
            t = _strtod_l(sv.data(), &pEnd, plainNumericLocale());
    #elif defined(__MINGW32__)
            // On MinGW 7.3 _strtod_l() failed to parse inf/nan, so for now just using std::strtod()
            t = std::strtod(sv.data(), &pEnd);
    #else
            t = strtod_l(sv.data(), &pEnd, plainNumericLocale());
    #endif
            if (pSuccess)
                *pSuccess = (pEnd == sv.endRaw());
#endif
    }

    template <class T, class Char_T>
    T genericImpl(const Char_T* psz, bool* pSuccess = nullptr)
    {
        auto t = T();
        if (pSuccess)
            *pSuccess = false;
        if (!psz)
            return t;

        // Trimming front.
        while (*psz == ' ')
            ++psz;

        if (*psz == '\0')
            return t;

        // Trimming end
        auto pEnd = psz + strLen(psz);
        while (*(pEnd - 1) == ' ')
            --pEnd;

        DFG_CLASS_NAME(StringView)<Char_T> sv(psz, pEnd - psz);
        convertImpl(sv, t, pSuccess);
        return t;
    }

    #define CONVERT_STR_TO_IMPL(typeName, funcA, funcW) \
        template <> inline typeName convertStrToImpl(const NonNullCStr psz) {return static_cast<typeName>(funcA(psz));} \
        template <> inline typeName convertStrToImpl(const NonNullCStrW psz) {return static_cast<typeName>(funcW(psz));}
    #define CONVERT_STR_TO_IMPL_PARAMS(typeName, funcA, funcW, param1, param2) \
        template <> inline typeName convertStrToImpl(const NonNullCStr psz) {return static_cast<typeName>(funcA(psz, param1, param2));} \
        template <> inline typeName convertStrToImpl(const NonNullCStrW psz) {return static_cast<typeName>(funcW(psz, param1, param2));}

    template <class T> T convertStrToImpl(const NonNullCStr psz, bool* pSuccess = nullptr)
    {
        return genericImpl<T>(psz, pSuccess);
    }

    template <class T> T convertStrToImpl(const NonNullCStrW psz, bool* pSuccess = nullptr)
    {
        return genericImpl<T>(psz, pSuccess);
    }

    /*
    CONVERT_STR_TO_IMPL(int8, atoi, wtoiImpl);
    CONVERT_STR_TO_IMPL(uint8, atoi, wtoiImpl);
    CONVERT_STR_TO_IMPL(int16, atoi, wtoiImpl);
    CONVERT_STR_TO_IMPL(uint16, atoi, wtoiImpl);
    CONVERT_STR_TO_IMPL(int32, atoi, wtoiImpl);
    CONVERT_STR_TO_IMPL_PARAMS(uint32, strtoul, wcstoul, nullptr, 10);
    CONVERT_STR_TO_IMPL(int64, strtoi64Impl, wcstoi64Impl);
    CONVERT_STR_TO_IMPL(uint64, strtoui64Impl, wcstoui64Impl);
    CONVERT_STR_TO_IMPL(float, atof, wtofImpl);
    CONVERT_STR_TO_IMPL(double, atof, wtofImpl);
    */


    // Long double not implemented.
    #undef CONVERT_STR_TO_IMPL
    #undef CONVERT_STR_TO_IMPL_PARAMS
} // detail namespace

template <class T> inline typename std::remove_cv<T>::type convertStrTo(const NonNullCStr psz, bool* pSuccess = nullptr)
    {return DFG_DETAIL_NS::convertStrToImpl<typename std::remove_cv<T>::type>(psz, pSuccess);}
template <class T> inline typename std::remove_cv<T>::type convertStrTo(const NonNullCStrW psz, bool* pSuccess = nullptr)
    {return DFG_DETAIL_NS::convertStrToImpl<typename std::remove_cv<T>::type>(psz, pSuccess);}

// Single parameter strTo()
template <class T> inline typename std::remove_cv<T>::type strTo(const NonNullCStr psz, bool* pSuccess = nullptr)
    {return convertStrTo<T>(psz, pSuccess);}

template <class T> inline typename std::remove_cv<T>::type strTo(const NonNullCStrW psz, bool* pSuccess = nullptr)
    {return convertStrTo<T>(psz, pSuccess);}

// Single parameter strTo()
template <class T> inline typename std::remove_cv<T>::type strTo(const std::string& s, bool* pSuccess = nullptr)
{
    return convertStrTo<T>(s.c_str(), pSuccess);
}

template <class T> inline typename std::remove_cv<T>::type strTo(const std::wstring& s, bool* pSuccess = nullptr)
{
    return convertStrTo<T>(s.c_str(), pSuccess);
}

// strTo(x) for StringView
template <class T, class Char_T, class Str_T>
inline typename std::remove_cv<T>::type strTo(const DFG_CLASS_NAME(StringView)<Char_T, Str_T>& sv, bool* pSuccess = nullptr)
{
    // TODO: convert directly from string view instead of creating redundant temporary.
    return convertStrTo<T>(toCharPtr_raw(sv.toString().c_str()), pSuccess);
}

// strTo(x) for StringViewSz
template <class T, class Char_T, class Str_T>
inline typename std::remove_cv<T>::type strTo(const DFG_CLASS_NAME(StringViewSz)<Char_T, Str_T>& sv, bool* pSuccess = nullptr)
{
    return convertStrTo<T>(toCharPtr_raw(sv.c_str()), pSuccess);
}

// strTo(x) for SzPtrT
template <class T, class Char_T, CharPtrType Type_T>
inline typename std::remove_cv<T>::type strTo(const SzPtrT<Char_T, Type_T>& sv, bool* pSuccess = nullptr)
{
    return convertStrTo<T>(toCharPtr_raw(sv.c_str()), pSuccess);
}

// Overloads that take the destination object as parameter.
template<class T> inline T& convertStrTo(const NonNullCStr psz, T& obj)  {return (obj = convertStrTo<T>(psz));}
template<class T> inline T& convertStrTo(const NonNullCStrW psz, T& obj) {return (obj = convertStrTo<T>(psz));}

// Overloads for std::string and std::wstring.
template<class T> inline T& convertStrTo(const std::string& s, T& obj)  {return (obj = convertStrTo<T>(s.c_str()));}
template<class T> inline T& convertStrTo(const std::wstring& s, T& obj) {return (obj = convertStrTo<T>(s.c_str()));}

template <class T> inline T& strTo(const NonNullCStr psz, T& obj)	{return convertStrTo(psz, obj);}
template <class T> inline T& strTo(const NonNullCStrW psz, T& obj)	{return convertStrTo(psz, obj);}
template <class T> inline T& strTo(const std::string& s, T& obj)	{return convertStrTo(s, obj);}
template <class T> inline T& strTo(const std::wstring& s, T& obj)	{return convertStrTo(s, obj);}

}} // module str

#endif
