#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../dfgAssert.hpp"
#include <algorithm>
#include <type_traits>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(str) {

    namespace DFG_DETAIL_NS
    {
        template <class StringView_T, class Str_T>
        void strCatImpl(Str_T&& dest, const StringView_T* arr, const size_t nArrSize)
        {
            // Count the length of the resulting string.
            size_t nParamLengthSum = 0;
            for (size_t i = 0; i < nArrSize; ++i)
                nParamLengthSum += arr[i].length();

            // Store old size and resize to new size.
            auto nInsertAt = dest.size();
            dest.resize(dest.size() + nParamLengthSum, '\0');

            // Append each parameter to string.
            for (size_t i = 0; i < nArrSize; ++i)
            {
                auto& s = arr[i];
                std::copy(s.begin(), s.end(), dest.begin() + nInsertAt);
                nInsertAt += s.length();
            }
            DFG_ASSERT_UB(nInsertAt == dest.size());
        }
    } // namespace DFG_DETAIL_NS

    // Note: Before starting to think about using variadic templates, revise whether it is supported by compilers where strCat() should compile. 

    // Defines return type: Str_T for rvalues and Str_T& for lvalues.
#define DFG_TEMP_RETURN_TYPE typename std::conditional<std::is_rvalue_reference<Str_T&&>::value, Str_T, Str_T&>::type

    // Defines the actual functions for given char type. These are just boilerplate interfaces passing the actual work to strCatImpl().
#define DFG_TEMP_MACRO_DEFINE_STRCAT(PARAMTYPE, STR) \
    template <class Str_T> \
    auto strCat(Str_T&& dest, const PARAMTYPE& s0) -> DFG_TEMP_RETURN_TYPE \
    { \
        PARAMTYPE arrParams[] = { s0 }; \
        DFG_DETAIL_NS::strCatImpl(std::forward<Str_T>(dest), arrParams, DFG_COUNTOF(arrParams)); \
        return std::forward<Str_T>(dest); \
    } \
    template <class Str_T> \
    auto strCat(Str_T&& dest, const PARAMTYPE& s0, const PARAMTYPE& s1) -> DFG_TEMP_RETURN_TYPE \
    { \
        PARAMTYPE arrParams[] = { s0, s1 }; \
        DFG_DETAIL_NS::strCatImpl(dest, arrParams, DFG_COUNTOF(arrParams)); \
        return dest; \
    } \
    template <class Str_T> \
    auto strCat(Str_T&& dest, const PARAMTYPE& s0, const PARAMTYPE& s1, const PARAMTYPE& s2) -> DFG_TEMP_RETURN_TYPE \
    { \
        PARAMTYPE arrParams[] = { s0, s1, s2 }; \
        DFG_DETAIL_NS::strCatImpl(dest, arrParams, DFG_COUNTOF(arrParams)); \
        return dest; \
    } \
    template <class Str_T> \
    auto strCat(Str_T&& dest, const PARAMTYPE& s0, const PARAMTYPE& s1, const PARAMTYPE& s2, const PARAMTYPE& s3) -> DFG_TEMP_RETURN_TYPE \
    { \
        PARAMTYPE arrParams[] = { s0, s1, s2, s3 }; \
        DFG_DETAIL_NS::strCatImpl(dest, arrParams, DFG_COUNTOF(arrParams)); \
        return dest; \
    } \
    template <class Str_T> \
    auto strCat(Str_T&& dest, const PARAMTYPE& s0, const PARAMTYPE& s1, const PARAMTYPE& s2, const PARAMTYPE& s3, const PARAMTYPE& s4) -> DFG_TEMP_RETURN_TYPE \
    { \
        PARAMTYPE arrParams[] = { s0, s1, s2, s3, s4 }; \
        DFG_DETAIL_NS::strCatImpl(dest, arrParams, DFG_COUNTOF(arrParams)); \
        return dest; \
    } \
    template <class Str_T> \
    auto strCat(Str_T&& dest, const PARAMTYPE& s0, const PARAMTYPE& s1, const PARAMTYPE& s2, const PARAMTYPE& s3, const PARAMTYPE& s4, const PARAMTYPE& s5, \
                             const PARAMTYPE& s6 = PARAMTYPE(STR), const PARAMTYPE& s7 = PARAMTYPE(STR), const PARAMTYPE& s8 = PARAMTYPE(STR), const PARAMTYPE& s9 = PARAMTYPE(STR) \
            ) -> DFG_TEMP_RETURN_TYPE \
    { \
        PARAMTYPE arrParams[] = { s0, s1, s2, s3, s4, s5, s6, s7, s8, s9 }; \
        DFG_DETAIL_NS::strCatImpl(dest, arrParams, DFG_COUNTOF(arrParams)); \
        return dest; \
    } 

    // Define strCat for char wchar_t parameters.
    DFG_TEMP_MACRO_DEFINE_STRCAT(DFG_CLASS_NAME(StringViewC), "")
    DFG_TEMP_MACRO_DEFINE_STRCAT(DFG_CLASS_NAME(StringViewW), L"");

#undef DFG_TEMP_MACRO_DEFINE_STRCAT
#undef DFG_TEMP_RETURN_TYPE

} } // module namespace
