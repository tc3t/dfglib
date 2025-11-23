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

#include <type_traits>

// For now optionally using std::from_chars() on MSVC2017 with version >= 8 and on MSVC2019 with version >= 4
// MSVC2017 note: with default build options _MSVC_LANG < 201703L so from_chars() won't be used by default.
// MSVC2017 references: https://devblogs.microsoft.com/cppblog/stl-features-and-fixes-in-vs-2017-15-8/, https://docs.microsoft.com/en-us/cpp/overview/cpp-conformance-improvements?view=vs-2017#improvements_157
//      -using 2017.8 as limit due to information in the first link.
// MSVC2019 reference: https://docs.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance?view=vs-2019
// Note that using _MSVC_LANG below instead of __cplusplus because of reason listed here: https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
//      -i.e getting correct __cplusplus value needs a non-default build flag.
// This is coarse check that needs to be manually updated when std::from_chars() become available on other compilers.
// TODO: improve std::to_chars() detection, currently enabled only on MSVC and GCC
//      -In MinGW 11.1 there was std::from_chars for integer, but not for double, in GCC 11.1 both compiled.
// Note: std::to_chars() overload for floating point with precision argument doesn't seem to be available MSVC2017.9 (15.9) so using another flag for that.
#if ((DFG_MSVC_VER >= DFG_MSVC_VER_2019_4 || (DFG_MSVC_VER < DFG_MSVC_VER_VC16_0 && DFG_MSVC_VER >= DFG_MSVC_VER_2017_8)) && _MSVC_LANG >= 201703L) || (defined(__GNUG__) && !defined(__MINGW32__) && (__GNUC__ >= 11))
    #define DFG_STRTO_USING_FROM_CHARS 1
    #define DFG_STRTO_RADIX_SUPPORT    1
    #define DFG_TOSTR_USING_TO_CHARS   1
    #if defined(_MSC_VER)
        #define DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG (DFG_MSVC_VER >= DFG_MSVC_VER_2019_4)
    #elif defined(__GNUG__)
        #define DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG (__GNUC__ >= 11)
    #else
        #define DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG 0
    #endif
    #include <charconv>
#else
    #define DFG_STRTO_USING_FROM_CHARS 0
    #define DFG_STRTO_RADIX_SUPPORT    0
    #define DFG_TOSTR_USING_TO_CHARS   0
    #define DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG 0
#endif

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(str) {

class NumberRadix
{
public:
    // Radix must be within [2, 36].
    explicit NumberRadix(const int nRadix = 10)
        : m_nRadix(nRadix)
    {
        DFG_ASSERT_UB(m_nRadix >= 2 && m_nRadix <= 36);
    }

    operator int() const
    {
        return m_nRadix;
    }

    int m_nRadix;
}; // class NumberRadix

enum class CharsFormat
{
#if (DFG_STRTO_RADIX_SUPPORT == 1)
    fixed       = static_cast<int>(std::chars_format::fixed),
    general     = static_cast<int>(std::chars_format::general),
    hex         = static_cast<int>(std::chars_format::hex),
    scientific  = static_cast<int>(std::chars_format::scientific),
    default_fmt = scientific | fixed | hex
#else
    default_fmt = 123
#endif
}; // class CharsFormat

namespace DFG_DETAIL_NS
{
    namespace StrToConversionClass
    {
        using integer       = std::integral_constant<int, 1>;
        using floatingPoint = std::integral_constant<int, 2>;
        using boolType      = std::integral_constant<int, 3>;
        using general       = std::integral_constant<int, 100>;
    }

    template <class ConversionClass_T>
    class StrToParam_common
    {
    public:
        StrToParam_common(bool* pOk = nullptr)
            : m_pbOk(pOk)
        {}

        static constexpr ConversionClass_T conversionClass()
        {
            return ConversionClass_T();
        }

        void setSuccessFlagIfPresent(const bool b)
        {
            if (m_pbOk)
                *m_pbOk = b;
        }

        bool* m_pbOk;
    }; // class StrToParam_common

    class StrToParam_bool : public StrToParam_common<StrToConversionClass::boolType>
    {
    public:
        using BaseClass = StrToParam_common<StrToConversionClass::boolType>;
        using BaseClass::BaseClass;
    }; // class StrToParam_bool

    template <class Char_T> class StrToParam_integer : public StrToParam_common<StrToConversionClass::integer>
    {
    public:
        using BaseClass = StrToParam_common<StrToConversionClass::integer>;
        using BaseClass::BaseClass;
    }; // class StrToParam_integer for Char_T != char
    
    template <> class StrToParam_integer<char> : public StrToParam_common<StrToConversionClass::integer>
    {
    public:
        using BaseClass = StrToParam_common<StrToConversionClass::integer>;

#if DFG_STRTO_USING_FROM_CHARS == 1
        StrToParam_integer(NumberRadix radix = NumberRadix(10), bool* pOk = nullptr)
            : BaseClass(pOk)
            , m_radix(radix)
        {
        }

        StrToParam_integer(bool* pOk)
            : BaseClass(pOk)
        {}

        constexpr bool hasFromCharsArgument() const
        {
            return true;
        }

        int fromCharsArgument() const
        {
            return this->m_radix;
        }

        NumberRadix radix() const
        {
            return m_radix;
        }

        NumberRadix m_radix;
#else // case: DFG_STRTO_USING_FROM_CHARS == 0
        StrToParam_integer(bool* pOk = nullptr)
            : BaseClass(pOk)
        {}

        NumberRadix radix() const
        {
            return NumberRadix(10);
        }
#endif
    }; // class StrToParam_integer for char

    template <class Char_T>
    class StrToParam_floatingPoint : public StrToParam_common<StrToConversionClass::floatingPoint>
    {
    public:
        using BaseClass = StrToParam_common<StrToConversionClass::floatingPoint>;
        using BaseClass::BaseClass;
    }; // class StrToParam_floatingPoint for Char_T != char

    template <> class StrToParam_floatingPoint<char> : public StrToParam_common<StrToConversionClass::floatingPoint>
    {
    public:
        using BaseClass = StrToParam_common<StrToConversionClass::floatingPoint>;

        StrToParam_floatingPoint(CharsFormat cf = CharsFormat::default_fmt, bool* pOk = nullptr)
            : BaseClass(pOk)
            , m_charsFormat(cf)
        {
        }

        StrToParam_floatingPoint(bool* pOk)
            : BaseClass(pOk)
            , m_charsFormat(CharsFormat::default_fmt)
        {
        }

        bool hasFromCharsArgument() const
        {
            return (m_charsFormat != CharsFormat::default_fmt);
        }

#if DFG_STRTO_USING_FROM_CHARS == 1
        // Precondition: hasFromCharsArgument() == true
        std::chars_format fromCharsArgument() const
        {
            DFG_ASSERT_INVALID_ARGUMENT(hasFromCharsArgument(), "hasFromCharsArgument() must be true when calling fromCharsArgument()");
            return (m_charsFormat != CharsFormat::default_fmt) ? static_cast<std::chars_format>(m_charsFormat) : std::chars_format::general;
        }
#endif // DFG_STRTO_USING_FROM_CHARS == 1

        CharsFormat m_charsFormat;
    }; // class StrToParam_floatingPoint for char

    template <class T, class Char_T> class StrToParamImpl : public StrToParam_common<StrToConversionClass::general> { using BaseClass = StrToParam_common<StrToConversionClass::general>; using BaseClass::BaseClass; };

    // Integer types
    template <class Char_T> class StrToParamImpl<bool, Char_T>               : public DFG_DETAIL_NS::StrToParam_bool    { using BaseClass = StrToParam_bool; using BaseClass::BaseClass; };
    template <class Char_T> class StrToParamImpl<short, Char_T>              : public DFG_DETAIL_NS::StrToParam_integer<Char_T> { using BaseClass = StrToParam_integer<Char_T>; using BaseClass::BaseClass; };
    template <class Char_T> class StrToParamImpl<unsigned short, Char_T>     : public DFG_DETAIL_NS::StrToParam_integer<Char_T> { using BaseClass = StrToParam_integer<Char_T>; using BaseClass::BaseClass; };
    template <class Char_T> class StrToParamImpl<int, Char_T>                : public DFG_DETAIL_NS::StrToParam_integer<Char_T> { using BaseClass = StrToParam_integer<Char_T>; using BaseClass::BaseClass; };
    template <class Char_T> class StrToParamImpl<unsigned int, Char_T>       : public DFG_DETAIL_NS::StrToParam_integer<Char_T> { using BaseClass = StrToParam_integer<Char_T>; using BaseClass::BaseClass; };
    template <class Char_T> class StrToParamImpl<long, Char_T>               : public DFG_DETAIL_NS::StrToParam_integer<Char_T> { using BaseClass = StrToParam_integer<Char_T>; using BaseClass::BaseClass; };
    template <class Char_T> class StrToParamImpl<unsigned long, Char_T>      : public DFG_DETAIL_NS::StrToParam_integer<Char_T> { using BaseClass = StrToParam_integer<Char_T>; using BaseClass::BaseClass; };
    template <class Char_T> class StrToParamImpl<long long, Char_T>          : public DFG_DETAIL_NS::StrToParam_integer<Char_T> { using BaseClass = StrToParam_integer<Char_T>; using BaseClass::BaseClass; };
    template <class Char_T> class StrToParamImpl<unsigned long long, Char_T> : public DFG_DETAIL_NS::StrToParam_integer<Char_T> { using BaseClass = StrToParam_integer<Char_T>; using BaseClass::BaseClass; };

    // Floating point types
    template <class Char_T> class StrToParamImpl<float, Char_T>         : public DFG_DETAIL_NS::StrToParam_floatingPoint<Char_T> { using BaseClass = StrToParam_floatingPoint<Char_T>; using BaseClass::BaseClass; };
    template <class Char_T> class StrToParamImpl<double, Char_T>        : public DFG_DETAIL_NS::StrToParam_floatingPoint<Char_T> { using BaseClass = StrToParam_floatingPoint<Char_T>; using BaseClass::BaseClass; };
    template <class Char_T> class StrToParamImpl<long double, Char_T>   : public DFG_DETAIL_NS::StrToParam_floatingPoint<Char_T> { using BaseClass = StrToParam_floatingPoint<Char_T>; using BaseClass::BaseClass; };

    template <class T, class Char_T> using StrToParam = DFG_DETAIL_NS::StrToParamImpl<typename std::decay<T>::type, typename std::remove_const<Char_T>::type>;

    template <class T, class Char_T>
    T& strToByNoThrowLexCastImpl(const StringView<Char_T>& sv, T& val, StrToParam<T, Char_T> param = StrToParam<T, Char_T>())
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
        param.setSuccessFlagIfPresent(bSuccess);
        return val;
    }
} // namespace DFG_DETAIL_NS
/////////////////////

template <class T, class Char_T> using StrToParam = DFG_DETAIL_NS::StrToParam<T, Char_T>;

template <class Str_T, class T>
T& strToByNoThrowLexCast(const Str_T& s, T& obj, StrToParam<T, typename std::decay<decltype(*std::declval<Str_T>().begin())>::type> param = StrToParam<T, typename std::decay<decltype(*std::declval<Str_T>().begin())>::type>())
{
#if DFG_BUILD_OPT_USE_BOOST==1
    try
    {
        obj = boost::lexical_cast<T>(s);
        param.setSuccessFlagIfPresent(true);
    }
    catch(const boost::bad_lexical_cast&)
    {
        param.setSuccessFlagIfPresent(false);
        obj = T();
    }
    return obj;
#else
    DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(T, "strToByNoThrowLexCast: implementation is not available when building without Boost");
#endif
}

template <class T>
T& strToByNoThrowLexCast(const StringViewC& sv, T& val, StrToParam<T, char> param = StrToParam<T, char>()) { return DFG_DETAIL_NS::strToByNoThrowLexCastImpl(sv, val, param); }

template <class T>
T& strToByNoThrowLexCast(const StringViewW& sv, T& val, StrToParam<T, wchar_t> param = StrToParam<T, wchar_t>()) { return DFG_DETAIL_NS::strToByNoThrowLexCastImpl(sv, val, param); }

template <class Char_T>
bool& strToBoolNoThrowLexCast(const StringView<Char_T>& sv, bool& val, StrToParam<bool, Char_T> param = StrToParam<bool, Char_T>())
{ 
    if (sv == DFG_STRING_LITERAL_BY_CHARTYPE(Char_T, "true"))
    {
        val = true;
        param.setSuccessFlagIfPresent(true);
    }
    else if (sv == DFG_STRING_LITERAL_BY_CHARTYPE(Char_T, "false"))
    {
        val = false;
        param.setSuccessFlagIfPresent(true);
    }
    else
        val = strToByNoThrowLexCastImpl(sv, val, param);
    return val;
}

template <class T>
T strToByNoThrowLexCast(const ReadOnlySzParamC& s, StrToParam<T, char> param = StrToParam<T, char>())
{
    T val;
    return strToByNoThrowLexCast(s, val, param);
}

namespace DFG_DETAIL_NS
{
#if (DFG_STRTO_USING_FROM_CHARS != 1) && !defined(__MINGW32__)
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

#if DFG_STRTO_USING_FROM_CHARS == 1
    template <class T>
    inline void fromChars(StringView<char> sv, T& t, const T defaultValue, StrToParam<T, char> param)
    {
        t = defaultValue;
        const auto rv = (param.hasFromCharsArgument()) ? std::from_chars(sv.data(), sv.endRaw(), t, param.fromCharsArgument()) : std::from_chars(sv.data(), sv.endRaw(), t);
        param.setSuccessFlagIfPresent(rv.ptr == sv.endRaw() && rv.ec == std::errc());
    }
#endif

    // Specialization for case that Char_T != char or T is bool or T is non-integer.
    template <class Char_T, class T>
    inline void convertImpl2(StringView<Char_T> sv, T& t, StrToParam<T, Char_T> param, std::false_type)
    {
        DFG_DETAIL_NS::strToByNoThrowLexCastImpl(sv, t, param);
    }

    // Specialization for case that Char_T == char and T is non-bool integer.
    template <class Char_T, class T>
    inline void convertImpl2(StringView<Char_T> sv, T& t, StrToParam<T, Char_T> param, std::true_type)
    {
        DFG_STATIC_ASSERT(std::is_integral<T>::value, "Internal error: integer implementation called with non-integer");
#if DFG_STRTO_USING_FROM_CHARS == 1
        fromChars(sv, t, T(), param);
#else
        convertImpl2(sv, t, param, std::false_type());
#endif
    }

    template <class Char_T, class T>
    inline void convertImpl(StringView<Char_T> sv, T& t, StrToParam<T, Char_T> param)
    {
        constexpr bool isCharAndNonBoolInteger = std::is_integral<T>::value && std::is_same<Char_T, char>::value && !std::is_same<T, bool>::value;
        convertImpl2(sv, t, param, std::integral_constant<bool, isCharAndNonBoolInteger>());
    }

    template <class T>
    inline void convertImplFloatOrLongDouble(StringView<char> sv, T& t, StrToParam<T, char> param)
    {
#if DFG_STRTO_USING_FROM_CHARS == 1
        fromChars(sv, t, std::numeric_limits<T>::quiet_NaN(), param);
#else
        convertImpl2(sv, t, param, std::false_type());
#endif
    }

    inline void convertImpl(StringView<char> sv, float& t, StrToParam<float, char> param)
    {
        convertImplFloatOrLongDouble(sv, t, param);
    }

    inline void convertImpl(StringView<char> sv, long double& t, StrToParam<long double, char> param)
    {
        convertImplFloatOrLongDouble(sv, t, param);
    }

    inline void convertImpl(StringView<char> sv, double& t, StrToParam<double, char> param)
    {
        // While view itself is not necessarily null-terminated, the underlying string is and since
        // trailing spaces seem to be no problem for strtod(), passing the start pointer as such.
#if DFG_STRTO_USING_FROM_CHARS == 1
        fromChars(sv, t, std::numeric_limits<double>::quiet_NaN(), param);
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
            param.setSuccessFlagIfPresent(pEnd == sv.endRaw());
#endif
    }

    // Specialization for integers
    template <class Char_T, class T>
    inline void convertImpl(StringView<Char_T> sv, T& val, StrToParam<T, Char_T> param, StrToConversionClass::integer)
    {
        convertImpl(sv, val, param);
    }

    // Specialization for floating points
    template <class Char_T, class T>
    inline void convertImpl(StringView<Char_T> sv, T& val, StrToParam<T, Char_T> param, StrToConversionClass::floatingPoint)
    {
        convertImpl(sv, val, param);
    }

    // Specialization for bool
    template <class Char_T>
    inline void convertImpl(StringView<Char_T> sv, bool& val, StrToParam<bool, Char_T> param, StrToConversionClass::boolType)
    {
        strToBoolNoThrowLexCast(sv, val, param);
    }

    template <class T>
    T defaultStrToReturnValue() { return std::numeric_limits<T>::quiet_NaN(); }

#if DFG_STRTO_RADIX_SUPPORT == 1
    template <class T>
    const char* skipPrefix(const char* psz, StrToParam<T, char> param, StrToConversionClass::integer)
    {
        if (param.radix() == 16 && psz[0] == '0' && (psz[1] == 'x' || psz[1] == 'X'))
            return psz + 2;
        else
            return psz;
    }
#endif // DFG_STRTO_RADIX_SUPPORT == 1

    template <class T, class Char_T>
    const Char_T* skipPrefix(const Char_T* psz, StrToParam<T, Char_T> param, Dummy)
    {
        DFG_UNUSED(param);
        return psz;
    }

    template <class T, class Char_T>
    T genericImpl(const Char_T* psz, StrToParam<T, Char_T> param)
    {
        auto t = defaultStrToReturnValue<T>();
        param.setSuccessFlagIfPresent(false);
        if (!psz)
            return t;

        // Trimming front.
        while (*psz == ' ')
            ++psz;

        psz = skipPrefix<T>(psz, param, param.conversionClass());

        if (*psz == '\0')
            return t;

        // Trimming end
        auto pEnd = psz + strLen(psz);
        while (*(pEnd - 1) == ' ')
            --pEnd;

        StringView<Char_T> sv(psz, pEnd - psz);
        convertImpl(sv, t, param, param.conversionClass());
        return t;
    }

    template <class T, class Char_T> typename std::remove_cv<T>::type convertStrToImpl(const Char_T* psz, StrToParam<T, Char_T> param)
    {
        using CvRemovedT = typename std::remove_cv<T>::type;
        return genericImpl<CvRemovedT>(psz, param);
    }

    /*
    #define CONVERT_STR_TO_IMPL(typeName, funcA, funcW) \
        template <> inline typeName convertStrToImpl(const NonNullCStr psz) {return static_cast<typeName>(funcA(psz));} \
        template <> inline typeName convertStrToImpl(const NonNullCStrW psz) {return static_cast<typeName>(funcW(psz));}
    #define CONVERT_STR_TO_IMPL_PARAMS(typeName, funcA, funcW, param1, param2) \
        template <> inline typeName convertStrToImpl(const NonNullCStr psz) {return static_cast<typeName>(funcA(psz, param1, param2));} \
        template <> inline typeName convertStrToImpl(const NonNullCStrW psz) {return static_cast<typeName>(funcW(psz, param1, param2));}

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
    // Long double not implemented.
    #undef CONVERT_STR_TO_IMPL
    #undef CONVERT_STR_TO_IMPL_PARAMS
    */

} // detail namespace
/////////////////////

// Single parameter strTo()
template <class T> inline typename std::remove_cv<T>::type strTo(const char* psz, StrToParam<T, char> param = StrToParam<T, char>())
{
    return DFG_DETAIL_NS::convertStrToImpl<T>(psz, param);
}

template <class T> inline typename std::remove_cv<T>::type strTo(const wchar_t* psz, StrToParam<T, wchar_t> param = StrToParam<T, wchar_t>())
{
    return DFG_DETAIL_NS::convertStrToImpl<T>(psz, param);
}

// Single parameter strTo()
template <class T> inline typename std::remove_cv<T>::type strTo(const std::string& s, StrToParam<T, char> param = StrToParam<T, char>())
{
    return strTo<T>(s.c_str(), param);
}

template <class T> inline typename std::remove_cv<T>::type strTo(const std::wstring& s, StrToParam<T, wchar_t> param = StrToParam<T, wchar_t>())
{
    return strTo<T>(s.c_str(), param);
}

// strTo(x) for StringViewSz
template <class T, class Char_T, class Str_T>
inline typename std::remove_cv<T>::type strTo(const StringViewSz<Char_T, Str_T>& sv, StrToParam<T, Char_T> param = StrToParam<T, Char_T>())
{
    return strTo<T>(sv.asUntypedView().c_str(), param);
}

// strTo(x) for StringView
template <class T, class Char_T, class Str_T>
inline typename std::remove_cv<T>::type strTo(const StringView<Char_T, Str_T>& sv, StrToParam<T, Char_T> param = StrToParam<T, Char_T>())
{
    // TODO: convert directly from string view instead of creating redundant temporary.
    return strTo<T>(toCharPtr_raw(sv.toString().c_str()), param);
}

// strTo(x) for SzPtrT
template <class T, class Char_T, CharPtrType Type_T>
inline typename std::remove_cv<T>::type strTo(const SzPtrT<Char_T, Type_T>& sv, StrToParam<T, Char_T> param = StrToParam<T, Char_T>())
{
    const Char_T* pszRaw = toCharPtr_raw(sv.c_str());
    return strTo<T>(pszRaw, param);
}

// Overloads that take the destination object as parameter.
template <class T> inline T& strTo(const StringViewSzC sv, T& obj, StrToParam<T, char> param)    { return (obj = strTo<T>(sv, param)); }
template <class T> inline T& strTo(const wchar_t* psz,     T& obj, StrToParam<T, wchar_t> param) { return (obj = strTo<T>(psz, param)); }
template <class T> inline T& strTo(const std::wstring& s,  T& obj, StrToParam<T, wchar_t> param) { return (obj = strTo<T>(s, param)); }

// Helper interfaces to allow calling strTo() without the need to use StrToParam<T> explicitly, e.g. strTo("10", someIntRef, NumberRadix(16), someBoolPtr);

template <class T, class... ArgTypes_T>
T& strTo(const StringViewSzC sv, T& obj, ArgTypes_T... args) { return strTo<T>(sv, obj, StrToParam<T, char>(args...)); }

template <class T, class... ArgTypes_T>
T& strTo(const wchar_t* sv, T& obj, ArgTypes_T... args) { return strTo<T>(sv, obj, StrToParam<T, wchar_t>(args...)); }

template <class T, class... ArgTypes_T>
T& strTo(const std::wstring& s, T& obj, ArgTypes_T... args) { return strTo<T>(s, obj, StrToParam<T, wchar_t>(args...)); }

}} // module str

#endif
