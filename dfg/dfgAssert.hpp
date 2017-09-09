#pragma once

#include "dfgDefs.hpp"
#include <cstdio>
#include <stdexcept>
#include <vector>
#ifdef _MSC_VER
    #include <crtdbg.h>
#endif
#include <cstring>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(debug) {

    class DFG_CLASS_NAME(ExceptionRequire) : public std::runtime_error
    {
    public:
        typedef std::runtime_error BaseClass;
        DFG_CLASS_NAME(ExceptionRequire)(const char* psz) : BaseClass(psz) {}
    };

    inline void onAssertFailure(const char* const pszFile, const int nLine, const char* const pszMsg)
    {
        #if defined(_MSC_VER) && defined(_DEBUG) // _CrtDbgReport seems to be defined only in debug-config.
            const auto rv = _CrtDbgReport(_CRT_ASSERT, pszFile, nLine, NULL, "%s", pszMsg);
            if (rv == 1) // If retry-is pressed (rv == 1), break.
                _CrtDbgBreak();
        #else
            DFG_UNUSED(pszFile);
            DFG_UNUSED(nLine);
            fprintf(stderr, "%s", pszMsg);

            // Show messagebox if it's already available. Don't include it just for this to avoid
            // dragging the whole Windows.h along.
            #if defined(_WIN32) && defined(MessageBox)
                ::MessageBoxA(NULL, pszMsg, "Assert failed", MB_ICONERROR);
            #endif
            throw std::runtime_error(pszMsg);
        #endif
    }

    namespace DFG_DETAIL_NS
    {
        inline void assertImpl(const bool b,
                                const char* const pszFile,
                                const int nLine,
                                const char* const pszFuncName,
                                const char* const pszAssertType,
                                const char* pszExp)
        {
            if (!b)
            {
                const auto nParamLengths = std::strlen(pszAssertType) + std::strlen(pszFile) + std::strlen(pszFuncName) + std::strlen(pszExp);
                std::vector<char> chars(nParamLengths + 100);
#ifdef _MSC_VER
                sprintf_s(chars.data(), chars.size(),
#else
                sprintf(chars.data(),
#endif
                    "%s. File %s, line: %d, function: %s\nAssert expression:\n%s",
                    pszAssertType,
                    pszFile,
                    nLine,
                    pszFuncName,
                    pszExp);
                onAssertFailure(pszFile, nLine, chars.data());
            }
        }

        inline void requireImpl(const bool b, const char* pszExp)
        {
            if (!b)
                throw DFG_CLASS_NAME(ExceptionRequire)(pszExp);
        }
    }




} } // namespace debug

#define DFG_IMPLEMENTATION_ASSERT_DEBUG_IMPL(exp, msgAssertType) \
    DFG_MODULE_NS(debug)::DFG_DETAIL_NS::assertImpl((exp) != 0, __FILE__, __LINE__, DFG_CURRENT_FUNCTION_NAME, msgAssertType, #exp)


#ifdef _DEBUG
#define DFG_IMPLEMENTATION_ASSERT(exp, msgAssertType) \
        DFG_IMPLEMENTATION_ASSERT_DEBUG_IMPL(exp, msgAssertType)
#else
#define DFG_IMPLEMENTATION_ASSERT(exp, msgAssertType)
#endif

// Assert macros. Note that given parameter is not necessarily evaluated so one should not write
// DFG_ASSERT(expr);
// if expr does something that shouldn't be omitted.

// Generic assert whose severity is unspecified.
#define DFG_ASSERT(exp) DFG_IMPLEMENTATION_ASSERT(exp, "Generic assert failed")

// Generic assert that can be given a message parameter.
#define DFG_ASSERT_WITH_MSG(exp, msg) DFG_IMPLEMENTATION_ASSERT(exp, msg)

// Specialization of assert to be used when failure means possible error
// in correctness but not for example UB.
#define DFG_ASSERT_CORRECTNESS(exp) DFG_IMPLEMENTATION_ASSERT(exp, "Correctness-assert failed")

// Specialization of assert to be used when failure means that
// undefined behaviour will result.
#define DFG_ASSERT_UB(exp) DFG_IMPLEMENTATION_ASSERT(exp, "'Undefined behaviour'-assert triggered")

// Specialization of assert to be used when execution reaches a "not implemented"-part.
#define DFG_ASSERT_IMPLEMENTED(exp) DFG_IMPLEMENTATION_ASSERT(exp, "'Not implemented'-assert triggered")

#define DFG_ASSERT_INVALID_ARGUMENT(exp, msg) DFG_IMPLEMENTATION_ASSERT(exp, "'Invalid argument'-assert triggered:" msg)

// Like MFC VERIFY: In debug version evaluates the expression and causes error message if x is not true.
//                  In release version evaluates the expression but does not check the value.
#if defined(_DEBUG)
    #define DFG_VERIFY(x) DFG_ASSERT(x)
    #define DFG_VERIFY_WITH_MSG(x,msg) DFG_ASSERT_WITH_MSG(x,msg)
#else
    #define DFG_VERIFY(x)	x
    #define DFG_VERIFY_WITH_MSG(x,msg) x
#endif



// Checks given expression and if not true, throws expection of type DFG_MODULE_NS(debug)::DFG_CLASS_NAME(ExceptionRequire).
#define DFG_REQUIRE(exp) \
    DFG_MODULE_NS(debug)::DFG_DETAIL_NS::requireImpl((exp), #exp);


