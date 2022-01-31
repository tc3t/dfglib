#pragma once

#define DFGTEST_STATIC_TEST(expr)	DFG_STATIC_ASSERT(expr, "Static test case failed")
#define DFGTEST_STATIC(expr)        DFGTEST_STATIC_TEST(expr)  //DFG_STATIC is deprecated, use DFGTEST_STATIC_TEST
#define DFGTEST_MESSAGE(expr)       std::cout << "    MESSAGE: " << expr << '\n';

#define DFGTEST_EXPECT_EQ(a, b)             EXPECT_EQ(a, b)             // Expect equality without knowing which one is expected value
#define DFGTEST_EXPECT_NE(a, b)             EXPECT_NE(a, b)
#define DFGTEST_EXPECT_LEFT(expected, val)  EXPECT_EQ(expected, val)    // Expect that 'val' equals 'expected'
#define DFGTEST_EXPECT_TRUE(a)              EXPECT_TRUE(a)
#define DFGTEST_EXPECT_FALSE(a)             EXPECT_FALSE(a)
#define DFGTEST_EXPECT_GE(a, b)             EXPECT_GE(a, b)
#define DFGTEST_EXPECT_GT(a, b)             EXPECT_GT(a, b)
#define DFGTEST_EXPECT_LE(a, b)             EXPECT_LE(a, b)
#define DFGTEST_EXPECT_LT(a, b)             EXPECT_LT(a, b)
#define DFGTEST_EXPECT_DOUBLE_EQ(a,b)       EXPECT_DOUBLE_EQ(a,b)
#define DFGTEST_EXPECT_STREQ(a, b)          EXPECT_STREQ(a, b)

#define DFGTEST_ASSERT_TRUE(a)              ASSERT_TRUE(a)
#define DFGTEST_ASSERT_FALSE(a)             ASSERT_FALSE(a)
#define DFGTEST_ASSERT_EQ(a, b)             ASSERT_EQ(a, b)
#define DFGTEST_ASSERT_NE(a, b)             ASSERT_NE(a, b)
#define DFGTEST_ASSERT_DOUBLE_EQ(a, b)      ASSERT_DOUBLE_EQ(a, b)

// Tests whether val is within [lower, upper].
#define DFGTEST_EXPECT_WITHIN_GE_LE(val, lower, upper) \
    { \
    const auto dfgTestTemporaryValue = val; \
    DFGTEST_EXPECT_GE(dfgTestTemporaryValue, lower); \
    DFGTEST_EXPECT_LE(dfgTestTemporaryValue, upper); \
    }

// These expect that math header has been included; don't want to include in this file to avoid it getting included everywhere.
#define DFGTEST_EXPECT_NAN(val)             DFGTEST_EXPECT_TRUE(::DFG_MODULE_NS(math)::isNan(val))
#define DFGTEST_EXPECT_NON_NAN(val)         DFGTEST_EXPECT_FALSE(::DFG_MODULE_NS(math)::isNan(val))

// For testing cases where left is string literal and rigth utf8, for example: DFGTEST_EXPECT_EQ_LITERAL_UTF8("abc", functionThatReturnsUtf8())
#define DFGTEST_EXPECT_EQ_LITERAL_UTF8(x, y) EXPECT_EQ(x, ::DFG_ROOT_NS::StringViewUtf8(y).asUntypedView())


// Defining StringView printers for GoogleTest. Note that while these also provide clearer output to test console,
// it is not just about convenience, e.g. comparison of StringViewUtf8 fails to compile without these.

#include <ostream>
#include <dfg/ReadOnlySzParam.hpp>

namespace DFG_ROOT_NS
{
    namespace dfgtest_googletest
    {
        inline void gtestPrintToImpl(const char* psz, const size_t nSize, ::std::ostream* pOstrm)
        {
            using namespace DFG_ROOT_NS;
            if (!pOstrm)
                return;
            // Google Test (1.11) prints e.g. std::string with quotes so using for views as well.
            *pOstrm << "\"";
            pOstrm->write(psz, static_cast<std::streamsize>(nSize));
            *pOstrm << "\"";
        }

        template <class Char_T>
        inline void gtestPrintToImpl(const Char_T* psz, const size_t nSize, ::std::ostream* pOstrm, const char* pszTypePrefix)
        {
            using namespace DFG_ROOT_NS;
            if (!pOstrm)
                return;
            *pOstrm << pszTypePrefix << "\"";
            // Printing ASCII as regular chars and non-ascii as \x{hex-value}
            for (size_t i = 0; i < nSize; ++i)
            {
                const auto c = psz[i];
                if (c < 128)
                    *pOstrm << static_cast<char>(c);
                else
                    *pOstrm << "\\x{" << std::hex << uint32(c) << "}";
            }
            *pOstrm << "\"";
        }

        // for wchar_t
        inline void gtestPrintToImpl(const wchar_t* psz, const size_t nSize, ::std::ostream* pOstrm)
        {
            gtestPrintToImpl(psz, nSize, pOstrm, "L");
        }

        // For char16_t
        inline void gtestPrintToImpl(const char16_t* psz, const size_t nSize, ::std::ostream* pOstrm)
        {
            gtestPrintToImpl(psz, nSize, pOstrm, "c16");
        }

        // For char32_t
        inline void gtestPrintToImpl(const char32_t* psz, const size_t nSize, ::std::ostream* pOstrm)
        {
            gtestPrintToImpl(psz, nSize, pOstrm, "c32");
        }
    }

    // Note: printer implementation needs care to avoid ODR-pitfalls etc.
    //          https://stackoverflow.com/questions/24673515/googletest-printto-not-getting-called-for-a-class
    //          https://stackoverflow.com/questions/42597726/how-to-pretty-print-qstring-with-googletest-framework

    // StringView
    template <class T, class T2>
    void PrintTo(const StringView<T, T2>& sv, ::std::ostream* pOstrm)
    {
        dfgtest_googletest::gtestPrintToImpl(sv.asUntypedView().data(), sv.size(), pOstrm);
    }

    // StringViewSz
    template <class T, class T2>
    void PrintTo(const StringViewSz<T, T2>& sv, ::std::ostream* pOstrm)
    {
        dfgtest_googletest::gtestPrintToImpl(sv.asUntypedView().data(), sv.size(), pOstrm);
    }
}
