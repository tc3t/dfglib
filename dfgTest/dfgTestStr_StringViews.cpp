#include "stdafx.h"

#if (defined(DFGTEST_BUILD_MODULE_STR) && DFGTEST_BUILD_MODULE_STR == 1) || (!defined(DFGTEST_BUILD_MODULE_STR) && DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/str.hpp>
#include <dfg/rand.hpp>
#include <dfg/alg.hpp>
#include <dfg/str/hex.hpp>
#include <dfg/str/strCat.hpp>
#include <dfg/str/string.hpp>
#include <dfg/dfgBaseTypedefs.hpp>
#include <dfg/str/stringLiteralCharToValue.hpp>
#include <dfg/str/format_fmt.hpp>
#include <dfg/preprocessor/compilerInfoMsvc.hpp>
#include <dfg/cont.hpp>
#include <dfg/utf.hpp>
#include <dfg/iter/szIterator.hpp>
#if DFGTEST_ENABLE_BENCHMARKS == 1
    #include <dfg/time/timerCpu.hpp>
#endif // DFGTEST_ENABLE_BENCHMARKS == 1


namespace
{
    void ReadOnlySzParamOverloadTest(DFG_ROOT_NS::ReadOnlySzParamC sC)
    {
        std::string s = sC.c_str();
    }
    void ReadOnlySzParamOverloadTest(DFG_ROOT_NS::ReadOnlySzParamW sW)
    {
        std::wstring s = sW.c_str(); 
    }

    void ReadOnlySzParamWithSizeOverloadTest(DFG_ROOT_NS::ReadOnlySzParamWithSizeC sC)
    {
        std::string s = sC.c_str();
    }
    void ReadOnlySzParamWithSizeOverloadTest(DFG_ROOT_NS::ReadOnlySzParamWithSizeW sW)
    {
        std::wstring s = sW.c_str();
    }
}

TEST(dfgStr, ReadOnlySzParam)
{
    using namespace DFG_ROOT_NS;
    const auto func0 = [](ReadOnlySzParamC s)
    {
        return ::DFG_MODULE_NS(str)::strTo<size_t>(s);
    };
    const auto func1 = [](ReadOnlySzParamW s)
    {
        return ::DFG_MODULE_NS(str)::strTo<size_t>(s);
    };
    const auto func2 = [](ReadOnlySzParamWithSizeC s)
    {
        return ::DFG_MODULE_NS(str)::strTo<size_t>(s) + s.length();
    };
    const auto func3 = [](ReadOnlySzParamWithSizeW s)
    {
        return ::DFG_MODULE_NS(str)::strTo<size_t>(s) + s.length();
    };

    EXPECT_EQ(987654321, func0("987654321"));
    EXPECT_EQ(987654321, func0(std::string("987654321")));

    EXPECT_EQ(size_t(3876543210), func1(L"3876543210"));
    EXPECT_EQ(size_t(3876543210), func1(std::wstring(L"3876543210")));

    EXPECT_EQ(990, func2("987"));
    EXPECT_EQ(990, func2(std::string("987")));

    EXPECT_EQ(990, func3(L"987"));
    EXPECT_EQ(990, func3(std::wstring(L"987")));

    ReadOnlySzParamOverloadTest("test");
    ReadOnlySzParamOverloadTest(L"test");
    ReadOnlySzParamOverloadTest(std::string("test"));
    ReadOnlySzParamOverloadTest(std::wstring(L"test"));

    ReadOnlySzParamWithSizeOverloadTest("test");
    ReadOnlySzParamWithSizeOverloadTest(L"test");
    ReadOnlySzParamWithSizeOverloadTest(std::string("test"));
    ReadOnlySzParamWithSizeOverloadTest(std::wstring(L"test"));

    // Testing conversion to StringViewSz
    {
        ReadOnlySzParamC a("a");
        ReadOnlySzParamW w(L"b");
        StringViewSzC svC = a;
        StringViewSzW svW = w;
        DFGTEST_EXPECT_LEFT("a", svC);
        DFGTEST_EXPECT_LEFT(L"b", svW);
    }
}

namespace
{
    template <class StringView_T, class RawToTypedConv_T>
    void TestStringViewSubStr(StringView_T sv, RawToTypedConv_T conv, std::true_type)
    {
        using CharT = typename StringView_T::CharT;
        CharT szBufA[] = { 'a', '\0' };
        CharT szBufAb[] = { 'a', 'b', '\0' };
        CharT szBufAbc[] = { 'a', 'b', 'c', '\0' };
        CharT szBufB[] = { 'b', '\0' };
        CharT szBufBc[] = { 'b', 'c', '\0' };
        CharT szBufC[] = { 'c', '\0' };
        CharT szBufEmpty[] = { '\0' };

        EXPECT_EQ(conv(szBufEmpty), sv.substr_startCount(0, 0));
        EXPECT_EQ(conv(szBufA), sv.substr_startCount(0, 1));
        EXPECT_EQ(conv(szBufAb), sv.substr_startCount(0, 2));
        EXPECT_EQ(conv(szBufAbc), sv.substr_startCount(0, 3));
        EXPECT_EQ(conv(szBufAbc), sv.substr_startCount(0, 78954));
        EXPECT_EQ(conv(szBufB), sv.substr_startCount(1, 1));
        EXPECT_EQ(conv(szBufBc), sv.substr_startCount(1, 2));
        EXPECT_EQ(conv(szBufC), sv.substr_startCount(2, 1));
        EXPECT_EQ(conv(szBufEmpty), sv.substr_startCount(1, 0));
        EXPECT_EQ(conv(szBufEmpty), sv.substr_startCount(2, 0));
        EXPECT_EQ(conv(szBufEmpty), sv.substr_startCount(5, 0));
        EXPECT_EQ(conv(szBufEmpty), sv.substr_startCount(5, 10));

        EXPECT_EQ(conv(szBufAbc), sv.substr_start(0));
        EXPECT_EQ(conv(szBufBc), sv.substr_start(1));
        EXPECT_EQ(conv(szBufEmpty), sv.substr_start(5));

        DFGTEST_EXPECT_EQ(conv(szBufEmpty), sv.substr_tailByCount(0));
        DFGTEST_EXPECT_EQ(conv(szBufC), sv.substr_tailByCount(1));
        DFGTEST_EXPECT_EQ(conv(szBufBc), sv.substr_tailByCount(2));
        DFGTEST_EXPECT_EQ(conv(szBufAbc), sv.substr_tailByCount(3));
        DFGTEST_EXPECT_EQ(conv(szBufAbc), sv.substr_tailByCount(1000));
    }

    template <class StringView_T, class RawToTypedConv_T>
    void TestStringViewSubStr(StringView_T, RawToTypedConv_T, std::false_type)
    {
        // Placeholder for types that don't have substr-available.
    }

    template <class StringView_T, class RawToTypedConv_T>
    void TestIndexOperator(RawToTypedConv_T conv, std::true_type)
    {
        using CodePointT = typename std::remove_reference<decltype(*typename StringView_T::PtrT(nullptr))>::type;
        using CharT = typename StringView_T::CharT;
        const CharT sz[] = { CharT('a'), CharT('b'), CharT('c'), CharT('\0') };
        StringView_T sv(conv(sz));

        ASSERT_EQ(3, sv.size());
        EXPECT_EQ(CodePointT('a'), sv[0]);
        EXPECT_EQ(CodePointT('b'), sv[1]);
        EXPECT_EQ(CodePointT('c'), sv[2]);
    }

    template <class StringView_T, class RawToTypedConv_T>
    void TestIndexOperator(RawToTypedConv_T, std::false_type)
    {
    }

    template <class StringView_T, class RawToTypedConv_T>
    void TestStringViewIndexAccess(StringView_T sv, RawToTypedConv_T conv, std::true_type)
    {
        // Note: Code below expects sv == "abc".
        DFG_UNUSED(conv);
        typedef decltype(sv.front()) CodePointT;

        ASSERT_EQ(3, sv.size());
        EXPECT_EQ(CodePointT('a'), sv.front());
        EXPECT_EQ(CodePointT('a'), sv[0]);
        EXPECT_EQ(CodePointT('c'), sv.back());

        sv.pop_front();
        sv.pop_back();

        EXPECT_EQ(CodePointT('b'), sv.front());
        EXPECT_EQ(CodePointT('b'), sv[0]);
        EXPECT_EQ(CodePointT('b'), sv.back());

        sv.clear();
        EXPECT_TRUE(sv.empty());

        // Testing that empty view doesn't cause problems (this is here instead of TestStringViewSubStr() since SzViews can't be default constructed.)
        StringView_T().substr_startCount(0, 0);
        StringView_T().substr_startCount(0, 10000);
    }

    template <class StringView_T, class RawToTypedConv_T>
    void TestStringViewIndexAccess(const StringView_T&, RawToTypedConv_T, std::false_type)
    {
        // Nothing to do here.
    }

    template <class StringView_T, class Char_T, class Str_T, bool TestIndexAccess, class RawToTypedConv>
    void TestStringViewImpl(RawToTypedConv conv, const bool bTestNonSzViews = true)
    {
        using namespace DFG_ROOT_NS;
        const Char_T szEmpty[] = { '\0' };
        const Char_T sz[] = { 'a', 'b', 'c', '\0' };
        const Char_T sz2[] = { 'b', 'c', 'a', '\0' };
        const Str_T s(conv(sz));

        DFGTEST_STATIC_TEST((std::is_same<Str_T, typename StringView_T::StringT>::value));

        StringView_T view(conv(sz));
        StringView_T view2Chars(conv(&sz[0] + 1), 2);
        EXPECT_EQ(conv(&sz[1]), view2Chars.begin());
        EXPECT_EQ(StringView_T(conv(&sz[1])).asUntypedView(), toCharPtr_raw(view2Chars.begin()));
        EXPECT_EQ(2, view2Chars.length());
        StringView_T view2(s);
        EXPECT_EQ(s.c_str(), view2.begin());

        // Test empty()
        EXPECT_TRUE(StringView_T(conv(szEmpty)).empty());

        // Test operator==
        EXPECT_TRUE(StringView_T(conv(szEmpty)) == StringView_T(conv(szEmpty)));
        EXPECT_TRUE(view == view2);

        // Test toString
        {
            const auto sFromToString = view.toString();
            EXPECT_TRUE(sFromToString == view);
        }

        EXPECT_TRUE(StringView_T(Str_T()) == StringView_T(Str_T()));
        if (bTestNonSzViews)
        {
            EXPECT_TRUE(StringView_T(conv(sz + 1), 2) == StringView_T(conv(sz2), 2));
            EXPECT_FALSE(StringView_T(conv(sz + 1), 2) == StringView_T(conv(sz2), 3));
            EXPECT_FALSE(StringView_T(conv(sz), 2) == StringView_T(conv(sz2), 2));
        }

        EXPECT_TRUE(view == s);
        EXPECT_FALSE(view != s);
        EXPECT_TRUE(s == view);
        EXPECT_FALSE(s != view);
        EXPECT_TRUE(view == s.c_str());
        EXPECT_FALSE(view != s.c_str());
        EXPECT_TRUE(s.c_str() == view);
        EXPECT_FALSE(s.c_str() != view);
        EXPECT_FALSE(StringView_T(conv(sz)) == StringView_T(conv(sz2)));
        EXPECT_TRUE(StringView_T(conv(sz)) != StringView_T(conv(sz2)));

        // Test operator<
        {
            // TODO: Better tests.
            EXPECT_EQ(s < Str_T(view.begin(), view.end()), s < view);
            EXPECT_EQ(s < Str_T(view2Chars.begin(), view2Chars.end()), s < view2Chars);
        }

        // Test data()
        {
            EXPECT_EQ(view.begin(), view.data());
            EXPECT_EQ(view2.begin(), view2.data());
        }

        // Testing begin/endRaw()
        {
            EXPECT_EQ(toCharPtr_raw(view.begin()), view.beginRaw());
            EXPECT_EQ(toCharPtr_raw(view.end()), view.endRaw());
        }

        // Testing asUntypedView()
        {
            EXPECT_EQ(StringView<Char_T>(view.beginRaw(), view.endRaw()), view.asUntypedView());
        }

        // Conversion to std::basic_string_view
        {
            const std::basic_string_view<Char_T> stdSv = view;
            DFGTEST_EXPECT_LEFT(view.dataRaw(), stdSv.data());
            DFGTEST_EXPECT_LEFT(view.asUntypedView().size(), stdSv.size());
        }

        //DFG_CLASS_NAME(StringViewUtf8)() == DFG_ASCII("a"); // TODO: make this work

        const auto isTriviallyIndexable = std::integral_constant<bool, StringView_T::isTriviallyIndexable()>();
        TestStringViewSubStr(view, conv, isTriviallyIndexable);
        TestIndexOperator<StringView_T>(conv, isTriviallyIndexable);
        TestStringViewIndexAccess(view, conv, std::integral_constant<bool, TestIndexAccess>());
    }

}

namespace
{
    // Helper for doing some test for every StringView typedef, see existing usages for examples.
    template <class Handler_T>
    void forEachStringViewType(Handler_T&& handler)
    {
        using namespace DFG_ROOT_NS;
        handler(StringViewC());
        handler(StringViewW());
        handler(StringView16());
        handler(StringView32());
        handler(StringViewAscii());
        handler(StringViewLatin1());
        handler(StringViewUtf8());
        handler(StringViewUtf16());
    }

    // The same as forEachStringViewType but for StringViewSz types.
    template <class Handler_T>
    void forEachStringViewSzType(Handler_T&& handler)
    {
        using namespace DFG_ROOT_NS;
        handler(StringViewSzC(""));
        handler(StringViewSzW(L""));
        handler(StringViewSz16(u""));
        handler(StringViewSz32(U""));
        handler(StringViewSzAscii(SzPtrAscii("")));
        handler(StringViewSzLatin1(SzPtrLatin1("")));
        handler(StringViewSzUtf8(SzPtrUtf8("")));
        handler(StringViewSzUtf16(SzPtrUtf16(u"")));
    }
}

TEST(dfgStr, StringView)
{
    using namespace DFG_ROOT_NS;

    TestStringViewImpl<StringViewC, char, std::string, true>([](const char* psz)             { return psz; });
    TestStringViewImpl<StringViewW, wchar_t, std::wstring, true>([](const wchar_t* psz)      { return psz; });
    TestStringViewImpl<StringView16, char16_t, std::u16string, true>([](const char16_t* psz) { return psz; });
    TestStringViewImpl<StringView32, char32_t, std::u32string, true>([](const char32_t* psz) { return psz; });

    TestStringViewImpl<StringViewAscii, char, StringAscii, true>([](const char* psz)          { return DFG_ROOT_NS::SzPtrAscii(psz); });
    TestStringViewImpl<StringViewLatin1, char, StringLatin1, true>([](const char* psz)        { return DFG_ROOT_NS::SzPtrLatin1(psz); });
    TestStringViewImpl<StringViewUtf8, char, StringUtf8, false>([](const char* psz)           { return DFG_ROOT_NS::SzPtrUtf8(psz); });
    TestStringViewImpl<StringViewUtf16, char16_t, StringUtf16, false>([](const char16_t* psz) { return DFG_ROOT_NS::SzPtrUtf16(psz); });

    // Test that StringViewUtf8 accepts SzPtrAscii.
    {
        auto tpszAscii = SzPtrAscii("abc");
        const StringViewUtf8& svUtf8 = tpszAscii;
        DFG_UNUSED(svUtf8);
    }

    // Testing that view can be constructed from (typed) pointer pair
    {
        const char sz[] = "abc";
        StringViewC svc(sz, sz + 2);
        StringViewAscii sva(SzPtrAscii(sz), SzPtrAscii(sz + 2));
        StringViewUtf8 svu(SzPtrUtf8(sz), SzPtrUtf8(sz + 2));
        EXPECT_TRUE("ab" == svc);
        EXPECT_TRUE(SzPtrAscii("ab") == sva);
        EXPECT_TRUE(DFG_UTF8("ab") == svu);
    }

    // Construction from std::basic_string_views. Note that can't have the same for StringViewSz since don't know about null-terminator.
    {
        using namespace ::DFG_ROOT_NS;
        DFGTEST_EXPECT_LEFT("abc", StringViewC(std::string_view("abc")));
        DFGTEST_EXPECT_LEFT(L"abc", StringViewW(std::wstring_view(L"abc")));
        DFGTEST_EXPECT_LEFT(u"abc", StringView16(std::u16string_view(u"abc")));
        DFGTEST_EXPECT_LEFT(U"abc", StringView32(std::u32string_view(U"abc")));
        DFGTEST_EXPECT_LEFT(SzPtrUtf16(u"abc"), (StringViewUtf16(std::u16string_view(u"abc"))));

        // Lines below should fail to compile
        //StringViewAscii(std::string_view("abc"));
        //StringViewLatin1(std::string_view("abc"));
        //StringViewUtf8(std::string_view("abc"));
    }

    // Test that can compare views that have compare-compatible types.
    // TODO: make this work.
    /*
    {
        EXPECT_TRUE(StringViewAscii(DFG_ASCII("abc")) == StringViewUtf8(DFG_ASCII("abc")));
        EXPECT_TRUE(StringViewUtf8(DFG_ASCII("abc")) == StringViewAscii(DFG_ASCII("abc")));
    }
    */

    // Testing that StringViewUtf8 accepts StringAscii.
    {
        StringAscii sAscii(DFG_ASCII("a"));
        StringViewAscii svA = sAscii;
        StringViewUtf8 svU8 = svA;
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("a", svU8);
    }
}

namespace
{
    template <class StringView_T, class Ptr_T>
    void TestStringViewSzNullptrHandling(Ptr_T pNull, std::true_type)
    {
        using namespace ::DFG_ROOT_NS;
        StringView_T svFromNull(pNull);
        DFGTEST_EXPECT_TRUE(svFromNull.empty());
        DFGTEST_EXPECT_LEFT(0, svFromNull.size());
        DFGTEST_EXPECT_LEFT('\0', *toCharPtr_raw(svFromNull.c_str()));
    }

    template <class StringView_T>
    void TestStringViewSzNullptrHandling(::DFG_ROOT_NS::Dummy, std::false_type)
    {
        // nullptr handling is UB for non-one-sized char types so no tests here.
    }

    template <class StringView_T, class Char_T, class Str_T, class RawToTypedConv>
    void TestStringViewSzImpl(RawToTypedConv conv)
    {
        using namespace DFG_ROOT_NS;

        TestStringViewImpl<StringView_T, Char_T, Str_T, false>(conv, false);

        const Char_T sz0[] = { 'a', 'b', 'c', '\0' };
        const Char_T sz1[] = { 'a', 'b', 'c', 'd', '\0' };
        const Char_T sz2[] = { 'a', 'b', 'd', '\0' };
        const Char_T sz3[] = { 'a', '\0', 'b', '\0' };
        StringView_T szView(conv(sz0));
        EXPECT_EQ(conv(sz0), szView.c_str());             // Test existence of c_str()
        EXPECT_TRUE(szView == conv(sz0));
        EXPECT_TRUE(conv(sz0) == szView);
        EXPECT_FALSE(szView == conv(sz1));
        EXPECT_FALSE(szView == conv(sz2));
        EXPECT_EQ(DFG_DETAIL_NS::gnStringViewSzSizeNotCalculated, szView.m_nSize); // This tests implementation detail, not interface.
        EXPECT_EQ(3, szView.length());
        EXPECT_EQ(3, szView.m_nSize); // This tests implementation detail, not interface.
        EXPECT_TRUE(szView == conv(sz0));
        EXPECT_TRUE(conv(sz0) == szView);
        EXPECT_FALSE(szView == conv(sz1));
        EXPECT_FALSE(szView == conv(sz2));
        EXPECT_TRUE(szView != conv(sz2));
        EXPECT_FALSE(szView != conv(sz0));

        // Testing that constructing from string that has embedded nulls can be correctly done with size-constructor.
        {
            StringView_T szView1(conv(sz3));
            StringView_T szView2(conv(sz3), 3);
            const auto szView1Const = szView1;
            EXPECT_EQ(1, szView1.size());
            EXPECT_EQ(1, szView1.length());
            EXPECT_EQ(1, szView1Const.size());
            EXPECT_EQ(1, szView1Const.length());
            EXPECT_EQ(3, szView2.size());
            EXPECT_EQ(3, szView2.length());
            EXPECT_EQ(Char_T('a'), szView2.dataRaw()[0]);
            EXPECT_EQ(Char_T('\0'), szView2.dataRaw()[1]);
            EXPECT_EQ(Char_T('b'), szView2.dataRaw()[2]);
        }

        // Testing that can use operator== with non-sz view and typed SzPtr
        {
            const StringView_T constSv(conv(sz0));
            EXPECT_TRUE(StringView_T(conv(sz0)) == StringView_T(conv(sz0)));
            EXPECT_TRUE(StringView_T(conv(sz0)) != StringView_T(conv(sz1)));
            EXPECT_TRUE(StringView_T(conv(sz0)) == conv(sz0));
            EXPECT_TRUE(conv(sz0) == StringView_T(conv(sz0)));
            EXPECT_TRUE(constSv == conv(sz0));
            EXPECT_TRUE(conv(sz0) == constSv);
            EXPECT_TRUE(typename StringView_T::StringViewT(conv(sz0)) == StringView_T(conv(sz0)));
            EXPECT_TRUE(StringView_T(conv(sz0)) == typename StringView_T::StringViewT(conv(sz0)));
        }

        // Testing that asUntypedView() handles length() correctly; i.e. that untypedView has length iff source view has length calculated.
        {
            StringView_T viewSz(conv(sz0));
            EXPECT_FALSE(viewSz.asUntypedView().isLengthCalculated());
            EXPECT_EQ(3, viewSz.asUntypedView().length());
            EXPECT_EQ(3, viewSz.length());
            EXPECT_TRUE(viewSz.asUntypedView().isLengthCalculated());
            EXPECT_EQ(3, viewSz.asUntypedView().length());
        }

        // Testing construction from nullptr
        TestStringViewSzNullptrHandling<StringView_T>(conv(nullptr), std::integral_constant<bool, sizeof(Char_T) == 1>());
    }
}

TEST(dfgStr, StringViewSz)
{
    using namespace DFG_ROOT_NS;

    TestStringViewSzImpl<StringViewSzC, char, std::string>([](const char* psz)          { return psz; });
    TestStringViewSzImpl<StringViewSzW, wchar_t, std::wstring>([](const wchar_t* psz)   { return psz; });
    TestStringViewSzImpl<StringViewSz16, char16_t, std::u16string>([](const char16_t* psz) { return psz; });
    TestStringViewSzImpl<StringViewSz32, char32_t, std::u32string>([](const char32_t* psz) { return psz; });

    TestStringViewSzImpl<StringViewSzAscii, char, StringAscii>([](const char* psz)      { return DFG_ROOT_NS::SzPtrAscii(psz); });
    TestStringViewSzImpl<StringViewSzLatin1, char, StringLatin1>([](const char* psz)    { return DFG_ROOT_NS::SzPtrLatin1(psz); });
    TestStringViewSzImpl<StringViewSzUtf8, char, StringUtf8>([](const char* psz)        { return DFG_ROOT_NS::SzPtrUtf8(psz); });
    TestStringViewSzImpl<StringViewSzUtf16, char16_t, StringUtf16>([](const char16_t* psz) { return DFG_ROOT_NS::SzPtrUtf16(psz); });

    // Making sure that StringViewSzC().substr_start() returns StringViewSzC instead of StringViewC.
    DFGTEST_STATIC_TEST((std::is_same<StringViewSzC, decltype(StringViewSzC("").substr_start(0))>::value));

    // Embedded null handling
    {
        const StringViewSzC svSz("\0\0"); // Length gets computed with strlen() so will return 0
        StringViewSzC svSized("\0\0", 1); // Null-terminated view of size 1 with embedded null
        DFGTEST_EXPECT_LEFT(0, svSz.size());
        DFGTEST_EXPECT_TRUE(svSz.empty());
        DFGTEST_EXPECT_LEFT(1, svSized.size());
        DFGTEST_EXPECT_FALSE(svSized.empty());

        // StringView conversion
        StringViewC svFromSz(svSz);
        StringViewC svFromSized(svSized);
        DFGTEST_EXPECT_LEFT(0, svFromSz.size());
        DFGTEST_EXPECT_LEFT(svSz.data(), svFromSz.data());
        DFGTEST_EXPECT_LEFT(1, svFromSized.size());
        DFGTEST_EXPECT_LEFT(svSized.data(), svFromSized.data());
    }

    // Test that can compare views that have compare-compatible types.
    // TODO: make this work.
    /*
    {
        EXPECT_TRUE(StringViewSzAscii(DFG_ASCII("abc")) == StringViewSzUtf8(DFG_ASCII("abc")));
        EXPECT_TRUE(StringViewSzUtf8(DFG_ASCII("abc")) == StringViewSzAscii(DFG_ASCII("abc")));
    }
    */
}

TEST(dfgStr, StringViewSz_popFrontBaseChar)
{
    using namespace DFG_ROOT_NS;
    {
        StringViewSzC sv("a\0cd", 4);
        DFGTEST_ASSERT_LEFT(4, sv.size());
        sv.popFrontBaseChar();
        DFGTEST_EXPECT_LEFT(3, sv.size());
        sv.popFrontBaseChar();
        DFGTEST_EXPECT_LEFT(2, sv.size());
        sv.popFrontBaseChar();
        DFGTEST_EXPECT_LEFT(1, sv.size());
        sv.popFrontBaseChar();
        DFGTEST_EXPECT_LEFT(0, sv.size());
        DFGTEST_EXPECT_LEFT('\0', *sv.c_str());
    }
    // View that doesn't have length calculated after constructor
    {
        StringViewSzC sv("abc");
        sv.popFrontBaseChar();
        DFGTEST_EXPECT_LEFT(2, sv.size());
        sv.popFrontBaseChar();
        DFGTEST_EXPECT_LEFT(1, sv.size());
        sv.popFrontBaseChar();
        DFGTEST_EXPECT_LEFT(0, sv.size());
        DFGTEST_EXPECT_LEFT('\0', *sv.c_str());
    }
}

namespace
{

    template <class View_T, class Conv_T>
    void StringView_lessThanImpl(Conv_T conv)
    {
        using namespace DFG_ROOT_NS;
        EXPECT_TRUE(View_T(conv("a")) < View_T(conv("b")));
        EXPECT_TRUE(View_T(conv("a")) < View_T(conv("ab")));
        EXPECT_FALSE(View_T(conv("a")) < View_T(conv("a")));
        EXPECT_EQ(std::strcmp("", "a") < 0, View_T(conv("")) < View_T(conv("a")));
        /*
        EXPECT_TRUE(StringViewC("a") < StringViewC("b"));
        EXPECT_TRUE(StringViewAscii(DFG_ASCII("a")) < StringViewAscii(DFG_ASCII("b")));
        EXPECT_TRUE(StringViewUtf8(DFG_UTF8("a")) < StringViewUtf8(DFG_UTF8("b")));
        EXPECT_FALSE(StringViewUtf8(DFG_UTF8("a")) < StringViewUtf8(DFG_UTF8("a")));
        EXPECT_TRUE(StringViewUtf8(DFG_UTF8("a")) < StringViewUtf8(DFG_UTF8("ab")));
        */
    }
}

TEST(dfgStr, StringView_lessThan)
{
    using namespace DFG_ROOT_NS;
    StringView_lessThanImpl<StringViewC>([](const char* p)      { return p; });
    StringView_lessThanImpl<StringViewAscii>([](const char* p)  { return SzPtrAscii(p); });
    StringView_lessThanImpl<StringViewLatin1>([](const char* p) { return SzPtrLatin1(p); });
    StringView_lessThanImpl<StringViewUtf8>([](const char* p)   { return SzPtrUtf8(p); });

    StringView_lessThanImpl<StringViewSzC>([](const char* p)      { return p; });
    StringView_lessThanImpl<StringViewSzAscii>([](const char* p)  { return SzPtrAscii(p); });
    StringView_lessThanImpl<StringViewSzLatin1>([](const char* p) { return SzPtrLatin1(p); });
    StringView_lessThanImpl<StringViewSzUtf8>([](const char* p)   { return SzPtrUtf8(p); });

    EXPECT_FALSE(StringViewC(nullptr) < StringViewC(nullptr));
}


namespace
{
    template <class StringView_T, class UntypedStringView_T>
    static void StringView_autoConvToUntyped_impl(const StringView_T& sv)
    {
        using namespace DFG_ROOT_NS;
        UntypedStringView_T svUt(sv);
        EXPECT_STREQ(toCharPtr_raw(sv.toString().c_str()), toCharPtr_raw(svUt.toString().c_str()));
    }
}

TEST(dfgStr, StringView_autoConvToUntyped)
{
    using namespace DFG_ROOT_NS;
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewC), DFG_CLASS_NAME(StringViewC)>("abc");
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzC), DFG_CLASS_NAME(StringViewSzC)>("abc");
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzC), DFG_CLASS_NAME(StringViewC)>("abc");

    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewAscii), DFG_CLASS_NAME(StringViewC)>(DFG_ASCII("abc"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzAscii), DFG_CLASS_NAME(StringViewSzC)>(DFG_ASCII("abc"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzAscii), DFG_CLASS_NAME(StringViewC)>(DFG_ASCII("abc"));

    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewLatin1), DFG_CLASS_NAME(StringViewC)>(SzPtrLatin1("abc"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzLatin1), DFG_CLASS_NAME(StringViewSzC)>(SzPtrLatin1("abc"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzLatin1), DFG_CLASS_NAME(StringViewC)>(SzPtrLatin1("abc"));

    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewUtf8), DFG_CLASS_NAME(StringViewC)>(DFG_UTF8("ab\xC3" "\xA4" "c"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzUtf8), DFG_CLASS_NAME(StringViewSzC)>(DFG_UTF8("ab\xC3" "\xA4" "c"));
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzUtf8), DFG_CLASS_NAME(StringViewC)>(DFG_UTF8("ab\xC3" "\xA4" "c"));

    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewW), DFG_CLASS_NAME(StringViewW)>(L"abc");
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzW), DFG_CLASS_NAME(StringViewSzW)>(L"abc");
    StringView_autoConvToUntyped_impl<DFG_CLASS_NAME(StringViewSzW), DFG_CLASS_NAME(StringViewW)>(L"abc");
}

namespace
{
    template <class Sv_T>
    void stringView_trimFrontImpl(const Sv_T trimInput, const Sv_T expected, const ::DFG_ROOT_NS::StringView<typename Sv_T::CharT> svTrimChars)
    {
        using namespace DFG_ROOT_NS;
        Sv_T sv = trimInput;
        const auto nExpectedTrimCount = sv.size() - expected.size();
        const auto nTrimCount = sv.trimFront(svTrimChars);
        DFGTEST_EXPECT_LEFT(expected, sv);
        DFGTEST_EXPECT_LEFT(nExpectedTrimCount, nTrimCount);
        DFGTEST_EXPECT_LEFT(trimInput.size() - nTrimCount, sv.size());
        DFGTEST_EXPECT_LEFT(trimInput.endRaw(), sv.endRaw());
    }

    template <class Sv_T>
    void stringView_trimmed_tailImpl(const Sv_T trimInput, const Sv_T expected, const ::DFG_ROOT_NS::StringView<typename Sv_T::CharT> svTrimChars)
    {
        using namespace DFG_ROOT_NS;
        const auto svTrimmed = trimInput.trimmed_tail(svTrimChars);
        DFGTEST_EXPECT_LEFT(expected, svTrimmed);
        DFGTEST_EXPECT_LEFT(trimInput.beginRaw(), svTrimmed.beginRaw());
    }

#define DFGTEST_TRIM_LAMBDA_DEF(NAME, INPUT, EXPECTED, TRIMCHARS, FUNCNAME) \
     const auto testFunc_##NAME = [](auto sv) \
     { \
         using SvT = decltype(sv); \
         using CharT = typename SvT::CharT; \
         using SzPtrT = typename SvT::SzPtrT; \
         const auto trimInput = DFG_STRING_LITERAL_BY_CHARTYPE(CharT, INPUT); \
         const auto expected = DFG_STRING_LITERAL_BY_CHARTYPE(CharT, EXPECTED); \
         const auto trimChars = DFG_STRING_LITERAL_BY_CHARTYPE(CharT, TRIMCHARS); \
         FUNCNAME<SvT>(SvT(SzPtrT(trimInput)), SvT(SzPtrT(expected)), trimChars); \
     }
} // unnamed namespace

TEST(dfgStr, StringView_trimFront)
{
    using namespace DFG_ROOT_NS;

    #define DFGTEST_TEMP_DEFINE_FUNC(NAME, INPUT, EXPECTED, TRIMCHARS) DFGTEST_TRIM_LAMBDA_DEF(NAME, INPUT, EXPECTED, TRIMCHARS, stringView_trimFrontImpl)

#define DFGTEST_TEMP_DEFAULT_TRIM_CHARS " \r\n\t"
    DFGTEST_TEMP_DEFINE_FUNC(empty, "", "", DFGTEST_TEMP_DEFAULT_TRIM_CHARS);
    DFGTEST_TEMP_DEFINE_FUNC(ws, " \r \n  \t ", "", DFGTEST_TEMP_DEFAULT_TRIM_CHARS);
    DFGTEST_TEMP_DEFINE_FUNC(abc, " abc ", "abc ", DFGTEST_TEMP_DEFAULT_TRIM_CHARS);
    DFGTEST_TEMP_DEFINE_FUNC(customTrimChars, "abc ", " ", "bac");

    forEachStringViewType(testFunc_empty);
    forEachStringViewType(testFunc_ws);
    forEachStringViewType(testFunc_abc);
    forEachStringViewType(testFunc_customTrimChars);

    forEachStringViewSzType(testFunc_empty);
    forEachStringViewSzType(testFunc_ws);
    forEachStringViewSzType(testFunc_abc);
    forEachStringViewSzType(testFunc_customTrimChars);

    {
        std::string s = "     ";
        StringViewC sv(s.c_str(), 2);
        stringView_trimFrontImpl<StringViewC>(sv, "", " ");
    }

    {
        std::string s(4, '\0');
        StringViewSzC sv(s.c_str(), 2);
        stringView_trimFrontImpl<StringViewC>(sv, "", StringViewC("\0", 1));
    }

#undef DFGTEST_TEMP_DEFINE_FUNC
#undef DFGTEST_TEMP_DEFAULT_TRIM_CHARS
}

TEST(dfgStr, StringView_trimmed_tail)
{
    using namespace DFG_ROOT_NS;

#define DFGTEST_TEMP_DEFINE_FUNC(NAME, INPUT, EXPECTED, TRIMCHARS) DFGTEST_TRIM_LAMBDA_DEF(NAME, INPUT, EXPECTED, TRIMCHARS, stringView_trimmed_tailImpl)

#define DFGTEST_TEMP_DEFAULT_TRIM_CHARS " \r\n\t"
    DFGTEST_TEMP_DEFINE_FUNC(empty, "", "", DFGTEST_TEMP_DEFAULT_TRIM_CHARS);
    DFGTEST_TEMP_DEFINE_FUNC(ws, " \r \n  \t ", "", DFGTEST_TEMP_DEFAULT_TRIM_CHARS);
    DFGTEST_TEMP_DEFINE_FUNC(abc, " abc ", " abc", DFGTEST_TEMP_DEFAULT_TRIM_CHARS);
    DFGTEST_TEMP_DEFINE_FUNC(customTrimChars, " abc", " ", "bac");

    forEachStringViewType(testFunc_empty);
    forEachStringViewType(testFunc_ws);
    forEachStringViewType(testFunc_abc);
    forEachStringViewType(testFunc_customTrimChars);

    forEachStringViewSzType(testFunc_empty);
    forEachStringViewSzType(testFunc_ws);
    forEachStringViewSzType(testFunc_abc);
    forEachStringViewSzType(testFunc_customTrimChars);

#undef DFGTEST_TEMP_DEFINE_FUNC
#undef DFGTEST_TEMP_DEFAULT_TRIM_CHARS
}

namespace
{
    template <class View_T, class Owned_T, class RawToTypedConv_T>
    static void testStringViewOrOwnerTyped(RawToTypedConv_T conv)
    {
        using namespace DFG_ROOT_NS;
        using CharT = typename View_T::CharT;

        // Testing availability of default constructor
        {
            StringViewOrOwner<View_T, Owned_T>();
        }

        // Basic view test.
        {
            auto svo = StringViewOrOwner<View_T, Owned_T>::makeView(conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc")));
            EXPECT_TRUE(conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc")) == svo);
            EXPECT_FALSE(svo.isOwner());
        }

        {
            const CharT szLiteral[] = { 'a', 'b', 'c', '\0' };
            auto svo = DFG_ROOT_NS::StringViewOrOwner<View_T, Owned_T>::makeView(conv(szLiteral));
            EXPECT_TRUE(conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc")) == svo);
            EXPECT_FALSE(svo.isOwner());
            EXPECT_EQ(conv(szLiteral), svo.data());
        }

        // Testing makeOwned() and release() with both small and long string (i.e. one with expected SSO-buffer and one with allocated buffer)
        using PtrT = decltype(conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc")));
        const PtrT literals[] = { conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "abc")), conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "0123456789012345678901234567890123456789")) };
        for (size_t i = 0; i < DFG_COUNTOF(literals); ++i)
        {
            const auto& pszTest = literals[i];
            auto svo = StringViewOrOwner<View_T, Owned_T>::makeOwned(Owned_T(pszTest));
            auto svv = StringViewOrOwner<View_T, Owned_T>::makeView(pszTest);
            EXPECT_TRUE(pszTest == svo);
            EXPECT_TRUE(svo.isOwner());
            EXPECT_FALSE(svv.isOwner());
            auto s = svo.release();
            EXPECT_TRUE(svo.owned().empty());
            EXPECT_TRUE(conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "")) == svo);
            EXPECT_TRUE(pszTest == s);

            // Verifying that release() returns the viewed string even if it's not owned.
            const auto s2 = svv.release();
            EXPECT_TRUE(svv.owned().empty());
            EXPECT_TRUE(conv(DFG_STRING_LITERAL_BY_CHARTYPE(CharT, "")) == svv);
            EXPECT_EQ(s, s2);
        }
    }
}

TEST(dfgStr, StringViewOrOwner)
{
    using namespace DFG_ROOT_NS;
    testStringViewOrOwnerTyped<StringViewC, std::string>([](const char* p) { return p; });
    testStringViewOrOwnerTyped<StringViewSzC, std::string>([](const char* p) { return p; });
    testStringViewOrOwnerTyped<StringViewW, std::wstring>([](const wchar_t* p) { return p; });
    testStringViewOrOwnerTyped<StringViewSzW, std::wstring>([](const wchar_t* p) { return p; });

    testStringViewOrOwnerTyped<StringViewAscii, StringAscii>([](const char* psz) { return SzPtrAscii(psz); });
    testStringViewOrOwnerTyped<StringViewLatin1, StringLatin1>([](const char* psz) { return SzPtrLatin1(psz); });
    testStringViewOrOwnerTyped<StringViewUtf8, StringUtf8>([](const char* psz) { return SzPtrUtf8(psz); });
}
#endif
