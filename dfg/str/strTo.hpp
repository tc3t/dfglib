#ifndef DFG_STR_STRTO_LJIBPTYR
#define DFG_STR_STRTO_LJIBPTYR

#include "../dfgBase.hpp"
#if DFG_BUILD_OPT_USE_BOOST==1
    #include <boost/lexical_cast.hpp>
#else
    #include "../build/utils.hpp"
#endif
#include "../ReadOnlySzParam.hpp"

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
    }
    return obj;
#else
    DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(T, "strToByNoThrowLexCast: implementation is not available when building without Boost");
#endif
}

template <class T>
T strToByNoThrowLexCast(const DFG_CLASS_NAME(ReadOnlySzParamC)& s, bool* pSuccess = nullptr)
{
    T val;
    return strToByNoThrowLexCast(s, val, pSuccess);
}

namespace DFG_DETAIL_NS
{
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

    template <class T, class Char_T>
    T genericImpl(const Char_T* psz)
    {
        T t;
        strToByNoThrowLexCast(psz, t);
        return t;
    }

    #define CONVERT_STR_TO_IMPL(typeName, funcA, funcW) \
        template <> inline typeName convertStrToImpl(const NonNullCStr psz) {return static_cast<typeName>(funcA(psz));} \
        template <> inline typeName convertStrToImpl(const NonNullCStrW psz) {return static_cast<typeName>(funcW(psz));}
    #define CONVERT_STR_TO_IMPL_PARAMS(typeName, funcA, funcW, param1, param2) \
        template <> inline typeName convertStrToImpl(const NonNullCStr psz) {return static_cast<typeName>(funcA(psz, param1, param2));} \
        template <> inline typeName convertStrToImpl(const NonNullCStrW psz) {return static_cast<typeName>(funcW(psz, param1, param2));}

    template <class T> T convertStrToImpl(const NonNullCStr psz)
    {
        return genericImpl<T>(psz);
    }

    template <class T> T convertStrToImpl(const NonNullCStrW psz)
    {
        return genericImpl<T>(psz);
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

template <class T> inline typename std::remove_cv<T>::type convertStrTo(const NonNullCStr psz)
    {return DFG_DETAIL_NS::convertStrToImpl<typename std::remove_cv<T>::type>(psz);}
template <class T> inline typename std::remove_cv<T>::type strTo(const NonNullCStr psz)
    {return convertStrTo<T>(psz);}
template <class T> inline typename std::remove_cv<T>::type convertStrTo(const NonNullCStrW psz)
    {return DFG_DETAIL_NS::convertStrToImpl<typename std::remove_cv<T>::type>(psz);}
template <class T> inline typename std::remove_cv<T>::type strTo(const NonNullCStrW psz)
    {return convertStrTo<T>(psz);}

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
