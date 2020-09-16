#include <stdafx.h>
#include <dfg/cont.hpp>
#include <dfg/cont/table.hpp>
#include <dfg/cont/arrayWrapper.hpp>
#include <string>
#include <deque>
#include <list>
#include <memory>
#include <dfg/ptrToContiguousMemory.hpp>
#include <dfg/dfgBase.hpp>
#include <dfg/ReadOnlySzParam.hpp>
#include <dfg/cont/interleavedXsortedTwoChannelWrapper.hpp>
#include <dfg/cont/valueArray.hpp>
#include <dfg/cont/CsvConfig.hpp>
#include <dfg/cont/IntervalSet.hpp>
#include <dfg/cont/IntervalSetSerialization.hpp>
#include <dfg/cont/MapToStringViews.hpp>
#include <dfg/cont/MapVector.hpp>
#include <dfg/cont/ViewableSharedPtr.hpp>
#include <dfg/cont/SetVector.hpp>
#include <dfg/cont/SortedSequence.hpp>
#include <dfg/cont/TorRef.hpp>
#include <dfg/cont/TrivialPair.hpp>
#include <dfg/cont/UniqueResourceHolder.hpp>
#include <dfg/rand.hpp>
#include <dfg/typeTraits.hpp>
#include <dfg/io/BasicOmcByteStream.hpp>
#include <dfg/io/OmcByteStream.hpp>
#include <dfg/iter/szIterator.hpp>
#include <dfg/cont/contAlg.hpp>
#include <dfg/io/cstdio.hpp>
#include <dfg/time/timerCpu.hpp>

#if (DFG_LANGFEAT_MUTEX_11 == 1)
    DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
        #include <thread> // Causes compiler warning at least in VC2012
    DFG_END_INCLUDE_WITH_DISABLED_WARNINGS
#endif // (DFG_LANGFEAT_MUTEX_11 == 1)

TEST(dfgCont, makeVector)
{
    auto v1 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(1);
    ASSERT_EQ(v1.size(), 1);
    EXPECT_EQ(v1[0] == 1, true);

    auto v2 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(1, 2);
    EXPECT_EQ(v2.size(), 2);
    EXPECT_EQ(v2[0], 1);
    EXPECT_EQ(v2[1], 2);

    auto v2_2 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(std::string("a"), std::string("b"));
    EXPECT_EQ(v2_2.size(), 2);
    EXPECT_EQ(v2_2[0], "a");
    EXPECT_EQ(v2_2[1], "b");

    std::string s0 = "c";
    std::string s1 = "d";
    auto v2_3 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(s0, s1);
    EXPECT_EQ(v2_3.size(), 2);
    EXPECT_EQ(v2_3[0], "c");
    EXPECT_EQ(v2_3[1], "d");
    EXPECT_EQ(s0, "c");
    EXPECT_EQ(s1, "d");

    auto v3 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(8, 6, 9);
    EXPECT_EQ(v3.size() == 3 && v3[0] == 8 && v3[1] == 6 && v3[2] == 9, true);

    auto v4 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(5, 6, 7, 8);
    EXPECT_EQ(v4.size() == 4 && v4[0] == 5 && v4[1] == 6 && v4[2] == 7 && v4[3] == 8, true);

    /*
    auto v5 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector(1.1, 2.2, 3.3, 4.4, 5.5);
    EXPECT_EQ(v5.size() == 5 && v5[0] == 1.1 && v5[1] == 2.2 && v5[2] == 3.3 && v5[3] == 4.4 && v5[4] == 5.5, true);

    auto v6 = DFG_ROOT_NS::DFG_SUB_NS_NAME(cont)::makeVector('a', 'b', 'c', 'd', 'e', 'f');
    EXPECT_EQ(v6.size() == 6 && v6[0] == 'a' && v6[1] == 'b' && v6[2] == 'c' && v6[3] == 'd' && v6[4] == 'e' && v6[5] == 'f', true);
    */
}

TEST(dfgCont, ArrayWrapper)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    const double arr[] = { 1, 2, 3, 4, 5, 6 };
    std::vector<double> vec(cbegin(arr), cend(arr));
    const std::vector<double> cvec(cbegin(arr), cend(arr));
    std::array<double, DFG_COUNTOF(arr)> stdArr;
    std::copy(std::begin(arr), std::end(arr), stdArr.begin());
    std::string str;
    const std::string strConst;

    // Test existence of default constructor exists (doesn't compile if fails).
    DFG_CLASS_NAME(ArrayWrapperT)<int> wrapEmpty;

    // Test conversion from non-const to const wrapper.
    DFG_CLASS_NAME(ArrayWrapperT)<const int> wrapEmptyConst = wrapEmpty;

    const auto wrapArr = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(arr);
    const auto wrapVec = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(vec);
    const auto wrapcVec = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(cvec);
    const auto wrapStdArr = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(stdArr);
    //const auto wrapStr = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(str); // TODO: this should work, but for now commented out as it doesn't compile.
    const auto wrapStrConst = DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(strConst);
    EXPECT_EQ(count(arr), wrapArr.size());
    EXPECT_EQ(count(vec), wrapVec.size());
    EXPECT_EQ(count(cvec), wrapcVec.size());
    EXPECT_EQ(count(stdArr), wrapStdArr.size());
    for (size_t i = 0; i<wrapArr.size(); ++i)
    {
        EXPECT_EQ(&wrapArr[i], &arr[i]);
        EXPECT_EQ(&wrapVec[i], &vec[i]);
        EXPECT_EQ(&wrapcVec[i], &cvec[i]);
        EXPECT_EQ(&wrapStdArr[i], &stdArr[i]);
    }

    EXPECT_EQ(ptrToContiguousMemory(wrapArr), wrapArr.data());
    EXPECT_EQ(ptrToContiguousMemory(wrapVec), wrapVec.data());
    EXPECT_EQ(ptrToContiguousMemory(wrapcVec), wrapcVec.data());
    EXPECT_EQ(ptrToContiguousMemory(wrapStdArr), wrapStdArr.data());

    *wrapVec.begin() = 9;
    EXPECT_EQ(9, vec[0]);
    EXPECT_EQ(9, wrapVec[0]);
    //*wrapVec.cbegin() = 9; // This should fail to compile.
}

namespace
{
    template <class Cont_T>
    void elementTypeTestImpl(const Cont_T& cont)
    {
        DFG_UNUSED(cont);
        using namespace DFG_MODULE_NS(cont);
        // Note: this uses decltype(cont) on purpose.
        DFGTEST_STATIC((std::is_same<typename DFG_CLASS_NAME(ElementType)<decltype(cont)>::type, typename Cont_T::value_type>::value));
    }
}

TEST(dfgCont, elementType)
{
    using namespace DFG_MODULE_NS(cont);
    std::vector<int> vecInt;
    std::array<std::string, 3> arrStrings;
    std::vector<double> arrVectors[3];
    DFGTEST_STATIC((std::is_same<DFG_CLASS_NAME(ElementType)<decltype(vecInt)>::type, int>::value));
    DFGTEST_STATIC((std::is_same<DFG_CLASS_NAME(ElementType)<decltype(arrStrings)>::type, std::string>::value));
    DFGTEST_STATIC((std::is_same<DFG_CLASS_NAME(ElementType)<decltype(arrVectors)>::type, std::vector<double>>::value));
    elementTypeTestImpl(vecInt);
    elementTypeTestImpl(arrStrings);
}

namespace
{
    template <class Cont_T>
    void testValueArrayInterface(Cont_T& cont)
    {
        cont.data();
        EXPECT_TRUE(cont.empty());
        typename Cont_T::value_type arr[] = { 5.0, -3.5, 9.6, 7.8 };
        cont.assign(arr);
        EXPECT_EQ(DFG_COUNTOF(arr), cont.size());
        // TODO: min, max, minmax, average, shiftValues, multiply, divide
        // begin, cbegin, end, cend
    }
}

TEST(dfgCont, ValueArray)
{
    using namespace DFG_MODULE_NS(cont);

    DFG_CLASS_NAME(ValueArray)<double> valArray;
    DFG_CLASS_NAME(ValueVector)<double> valVector;
    DFG_CLASS_NAME(ValueArrayT)<std::valarray<double>> valArrayExplicit;

    // Checking that ValueVector can be constructed from initializer list
    // std::valarray -based ValueArray does not support it so there's no test for it.
    {
        ValueVector<double> vals = { 5.0, 6.0, 7.0 };
        EXPECT_EQ(ValueVector<double>({ 5.0, 6.0, 7.0 }), vals);
    }

    testValueArrayInterface(valArray);
    testValueArrayInterface(valVector);
    testValueArrayInterface(valArrayExplicit);

    // TODO: define interface for ValueArray by test cases.
}

#if DFG_LANGFEAT_MUTEX_11

TEST(dfgCont, ViewableSharedPtr)
{
    using namespace DFG_MODULE_NS(cont);

    int resetNotifier0 = 0;
    int resetNotifier1 = 0;
    int resetNotifier2 = 0;

    DFG_CLASS_NAME(ViewableSharedPtr)<const int> sp(std::make_shared<int>(1));
    EXPECT_TRUE(sp);
    EXPECT_EQ(0, sp.getReferrerCount());
    auto viewer0 = sp.createViewer();
    auto viewer1 = sp.createViewer();
    EXPECT_EQ(0, sp.getReferrerCount());
    sp.addResetNotifier(DFG_CLASS_NAME(SourceResetNotifierId)(&viewer0), [&](DFG_CLASS_NAME(SourceResetParam)) { ++resetNotifier0; });
    sp.addResetNotifier(DFG_CLASS_NAME(SourceResetNotifierId)(&viewer1), [&](DFG_CLASS_NAME(SourceResetParam)) { ++resetNotifier1; });

    {
        auto spActiveView = viewer0.view();
        EXPECT_EQ(1, *spActiveView);
        EXPECT_EQ(1, sp.getReferrerCount());
    }

    EXPECT_EQ(1, *viewer0.view());
    EXPECT_EQ(1, *viewer1.view());
    EXPECT_EQ(0, sp.getReferrerCount());
    EXPECT_EQ(0, resetNotifier0);
    EXPECT_EQ(0, resetNotifier1);

    sp.reset(std::make_shared<int>(2));
    EXPECT_EQ(0, sp.getReferrerCount());

    EXPECT_EQ(2, *viewer0.view());
    EXPECT_EQ(2, *viewer0.view());
    EXPECT_EQ(1, resetNotifier0);
    EXPECT_EQ(1, resetNotifier1);
    EXPECT_EQ(0, sp.getReferrerCount());

    // Test automatic handling of short-lived viewer (e.g. automatic removal of reset notifier).
    {
        auto viewer2 = sp.createViewer();
        EXPECT_EQ(0, sp.getReferrerCount());
        EXPECT_EQ(2, *viewer2.view());
        EXPECT_EQ(0, sp.getReferrerCount());
        sp.addResetNotifier(DFG_CLASS_NAME(SourceResetNotifierId)(&viewer2), [&](DFG_CLASS_NAME(SourceResetParam)) { ++resetNotifier2; });
    }
    sp.reset(std::make_shared<int>(3));
    EXPECT_EQ(0, sp.getReferrerCount());
    EXPECT_EQ(3, *viewer0.view());
    EXPECT_EQ(3, *viewer1.view());
    EXPECT_EQ(2, resetNotifier0);
    EXPECT_EQ(2, resetNotifier1);
    EXPECT_EQ(0, resetNotifier2);

    EXPECT_FALSE(viewer0.isNull());
    viewer0.reset();
    EXPECT_TRUE(viewer0.isNull());
    EXPECT_EQ(0, sp.getReferrerCount());
    sp.reset(std::make_shared<int>(4));
    EXPECT_EQ(0, sp.getReferrerCount());
    EXPECT_EQ(2, resetNotifier0);
    EXPECT_EQ(3, resetNotifier1);
    EXPECT_EQ(4, *viewer1.view());
    auto spCopy = sp.sharedPtrCopy();
    EXPECT_EQ(1, sp.getReferrerCount());
    auto pData = sp.get();
    sp.reset();
    EXPECT_EQ(0, sp.getReferrerCount());
    EXPECT_EQ(4, resetNotifier1);
    EXPECT_TRUE(spCopy.use_count() == 1);
    EXPECT_EQ(nullptr, viewer1.view().get());
    EXPECT_EQ(4, *spCopy);
    EXPECT_EQ(pData, spCopy.get());

    // Testing viewer behaviour when ViewableSharedPtr object is destroyed before viewer.
    {
        std::unique_ptr<DFG_CLASS_NAME(ViewableSharedPtr)<int>> sp2(new DFG_CLASS_NAME(ViewableSharedPtr)<int>(std::make_shared<int>(1)));
        auto viewer = sp2->createViewer();
        sp2.reset();
        EXPECT_EQ(nullptr, viewer.view()); // After ViewableSharedPtr() has been destroyed, expecting viewer to return null views.
    }
}

TEST(dfgCont, ViewableSharedPtr_editing)
{
    using namespace DFG_MODULE_NS(cont);

    DFG_CLASS_NAME(ViewableSharedPtr)<int> sp(std::make_shared<int>(1));
    sp.edit([](int& rEdit, const int* pOld)
    {
        EXPECT_EQ(1, rEdit);
        EXPECT_EQ(nullptr, pOld);
        rEdit++;
    });
    EXPECT_EQ(2, *sp.get());
    auto viewer = sp.createViewer(); // Just creating a viewer should not force edit() to create a new object.
    sp.edit([](int& rEdit, const int* pOld)
    {
        EXPECT_EQ(2, rEdit);
        EXPECT_EQ(nullptr, pOld);
        rEdit++;
    });
    EXPECT_EQ(3, *sp.get());
    auto viewOn3 = viewer.view();
    EXPECT_EQ(3, *viewOn3);
    sp.edit([](int& rEdit, const int* pOld) // Now there's an actual view, edit() should create a new object.
    {
        ASSERT_NE(nullptr, pOld);
        EXPECT_EQ(3, *pOld);
        EXPECT_NE(&rEdit, pOld);
        rEdit = 4;
    });
    EXPECT_EQ(4, *sp.get());
    EXPECT_EQ(0, sp.getReferrerCount()); // Zero since edit() should have created a new object.

    EXPECT_EQ(3, *viewOn3);         // Concrete view shouldn't be updated as it's a fixed reference to old object...
    EXPECT_EQ(4, *viewer.view());  // ...but new view created through viewer should have up-to-date value.

    // Resetting a viewer shouldn't affect anything.
    viewer.reset();
    sp.edit([](int& rEdit, const int* pOld) { EXPECT_EQ(4, rEdit); EXPECT_EQ(nullptr, pOld); });

    // If taking a copy of the shared_ptr, edit() should again be forced to create a new object even when there are no viewers.
    auto spCopy = sp.sharedPtrCopy();
    sp.edit([](int& rEdit, const int* pOld) { ASSERT_NE(nullptr, pOld); EXPECT_EQ(4, *pOld); rEdit = *pOld + 1; });
    EXPECT_EQ(5, *sp.get());
    EXPECT_EQ(4, *spCopy); // spCopy should point to old data as edit() should have created a new object.
}

TEST(dfgCont, ViewableSharedPtr_editingWithThreads)
{
    using namespace DFG_MODULE_NS(cont);

    DFG_CLASS_NAME(ViewableSharedPtr)<int> sp(std::make_shared<int>(1));

    sp.edit([&](int& rEdit, const int* pOld)
    {
        EXPECT_EQ(1, rEdit);
        EXPECT_EQ(nullptr, pOld);
        std::thread t1([&]()
        {
            // Creating viewer should work, but...
            auto viewer = sp.createViewer();
            EXPECT_FALSE(viewer.isNull());
            // ...creating actual view should fail as main thread is in middle of editing.
            EXPECT_EQ(nullptr, viewer.view());
        });
        t1.join();
    });
    EXPECT_EQ(1, *sp.get());

    auto viewer = sp.createViewer();
    ASSERT_FALSE(viewer.isNull());

    sp.edit([&](int& rEdit, const int* pOld)
    {
        EXPECT_EQ(1, rEdit);
        EXPECT_EQ(nullptr, pOld);
        std::thread t1([&]()
        {
            // Simulating the case where another thread tries to create a view with existing viewer while edit() is ongoing.
            EXPECT_EQ(nullptr, viewer.view());
        });
        t1.join();
    });
    EXPECT_EQ(1, *sp.get());

    auto view1 = viewer.view();

    sp.edit([&](int& rEdit, const int* pOld)
    {
        EXPECT_NE(nullptr, pOld);
        rEdit = *pOld + 1;
        std::thread t1([&]()
        {
            // Now that there's an active view, edit() should create a new object and creating view to old data should work.
            auto viewer = sp.createViewer();
            ASSERT_FALSE(viewer.isNull());
            auto concreteView = viewer.view();
            ASSERT_NE(nullptr, concreteView);
            EXPECT_EQ(1, *concreteView);
        });
        t1.join();
    });
    EXPECT_EQ(2, *sp.get());
}

#endif // DFG_LANGFEAT_MUTEX_11

namespace
{
    template <class T>
    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TorRef)<T> TorRefHelper(T& item, const bool bRef)
    {
        using namespace DFG_MODULE_NS(cont);
        if (bRef)
            return &item;
        else
            return DFG_CLASS_NAME(TorRef)<T>::makeInternallyOwning(item);
    }

    template <class Cont_T>
    void testArrowOperator()
    {
        Cont_T tor;
        tor->append("a");
        EXPECT_EQ("a", tor.item());
    }
}

TEST(dfgCont, TorRef)
{
    using namespace DFG_MODULE_NS(cont);

    // Basic test with non-const T.
    {
        DFG_CLASS_NAME(TorRef)<int> tor;
        EXPECT_FALSE(tor.hasRef());
        int& ref = tor;
        EXPECT_EQ(0, ref);
    }

    // // Basic test with const T.
    {
        DFG_CLASS_NAME(TorRef)<const int> tor;
        EXPECT_FALSE(tor.hasRef());
        const int& ref = tor;
        EXPECT_EQ(0, ref);
    }

    // Basic test with non-integral type.
    {
        std::vector<double> vec;
        auto rvRef = TorRefHelper(vec, true);
        EXPECT_TRUE(rvRef.hasRef());
        EXPECT_EQ(&vec, &rvRef.item());
        EXPECT_TRUE(rvRef->empty()); // Test operator-> overload.
        rvRef->resize(2); // Test calling non-const.

        const auto rvNonRef = TorRefHelper(vec, false);
        EXPECT_FALSE(rvNonRef.hasRef());
        EXPECT_EQ(rvNonRef.internalStorage().itemPtr(), &rvNonRef.item());
    }

    // Basic test with non-integral const type.
    {
        const std::vector<double> vec;
        const auto rvRef = TorRefHelper(vec, true);
        EXPECT_TRUE(rvRef.hasRef());
        EXPECT_EQ(&vec, &rvRef.item());
        EXPECT_TRUE(rvRef->empty()); // Test operator-> overload.
        //rvRef->resize(2); // This should fail to compile because vec is const.

        const auto rvNonRef = TorRefHelper(vec, false);
        EXPECT_FALSE(rvNonRef.hasRef());
        EXPECT_EQ(rvNonRef.internalStorage().itemPtr(), &rvNonRef.item());
    }

    // Basic test with shared_ptr reference.
    {
        auto sp = std::make_shared<const int>(10);
        DFG_CLASS_NAME(TorRefShared)<const int> rv(sp);
        EXPECT_TRUE(rv.hasRef());
        EXPECT_EQ(10, rv.item());
        EXPECT_EQ(2, sp.use_count());

        // Test that reference is alive even after the original shared_ptr get's reset.
        sp.reset();
        EXPECT_EQ(1, rv.referenceObject().use_count());
        EXPECT_EQ(10, rv.item());
    }

    // // Basic test with stack storage.
    {
        DFG_CLASS_NAME(TorRef)<const int, DFG_DETAIL_NS::TorRefInternalStorageStack<int>> tor;
        EXPECT_FALSE(tor.hasRef());
        const int& ref = tor;
        EXPECT_EQ(0, ref);
    }

    // Test operator -> 
    {
        testArrowOperator<DFG_CLASS_NAME(TorRef)<std::string>>();
        testArrowOperator<DFG_CLASS_NAME(TorRefShared)<std::string>>();
        testArrowOperator<DFG_CLASS_NAME(TorRef)<std::string, DFG_DETAIL_NS::TorRefInternalStorageStack<std::string>>>();
    }
}

TEST(dfgCont, MapToStringViews)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);

    DFGTEST_STATIC_TEST(
                        (sizeof(MapToStringViews<int, std::string, StringStorageType::sizeOnly>::StringDetail))
                            >
                         sizeof(MapToStringViews<int, std::string, StringStorageType::szSized>::StringDetail)
                        );
    DFGTEST_STATIC_TEST(
                        (sizeof(MapToStringViews<int, std::string, StringStorageType::sizeAndNullTerminated>::StringDetail))
                            >
                        sizeof(MapToStringViews<int, std::string, StringStorageType::szSized>::StringDetail)
                        );

    // Basic testing with untyped string
    {
        MapToStringViews<int, std::string, StringStorageType::sizeOnly> v;
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(0, v.size());
        v.insert(0, "a");
        EXPECT_EQ("a", v[0]);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(1, v.size());
        EXPECT_EQ(2, v.m_data.size());
        v.insert(1, "b");
        EXPECT_EQ("a", v[0]);
        EXPECT_EQ("b", v[1]);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(2, v.size());
        const auto nDataCapacityBeforeClear = v.m_data.capacity();
        const auto nDetailCapacityBeforeClear = v.m_keyToStringDetails.capacity();
        v.clear_noDealloc();
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(0, v.size());
        EXPECT_EQ(1, v.m_data.size()); // Checking that shared null wasn't removed.
        EXPECT_EQ(nDataCapacityBeforeClear, v.m_data.capacity());
        EXPECT_EQ(nDetailCapacityBeforeClear, v.m_keyToStringDetails.capacity());
        v.insert(-50, "c");
        EXPECT_EQ("", v[0]);
        EXPECT_EQ("c", v[-50]);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(1, v.size());
    }

    // Null terminator handling
    {
        MapToStringViews<int, std::string, StringStorageType::sizeAndNullTerminated> v;
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(0, v.size());
        v.insert(0, "a");
        EXPECT_EQ("a", v[0]);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(1, v.size());
        ASSERT_EQ(3, v.m_data.size()); // Making sure that null terminator has been added
        EXPECT_EQ('\0', v.m_data[2]); // Making sure that null terminator has been added
    }

    // realloc testing
    {
        MapToStringViews<int, std::string, StringStorageType::sizeOnly> v;
        for (int i = 0; i < 100; ++i)
            v.insert(i, std::string(1, static_cast<char>(i)));
        EXPECT_EQ(100, v.size());
        for (int i = 0; i < 100; ++i)
        {
            ASSERT_EQ(1, v[i].size());
            EXPECT_EQ(i, *v[i].beginRaw());
        }
    }

    // Basic typed testing.
    {
        MapToStringViews<int, StringUtf8, StringStorageType::szSized> v;
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(0, v.size());
        v.insert(0, DFG_UTF8("a"));
        EXPECT_EQ(1, v[0].length());
        EXPECT_EQ('a', *v[0].beginRaw());
        v.insert(5, DFG_UTF8("abcdefgh"));
        ASSERT_EQ(2, v.size());
        EXPECT_EQ("abcdefgh", StringViewC(v[5].beginRaw(), v[5].endRaw()));
    }

    // Testing storage handling
    {
        MapToStringViews<int, std::string, StringStorageType::szSized> vSzSized;
        MapToStringViews<int, std::string, StringStorageType::sizeAndNullTerminated> vSizeAndSz;
        MapToStringViews<int, std::string, StringStorageType::sizeOnly> vSizeOnly;
        vSzSized.insert(0, "ab");
        vSizeAndSz.insert(0, "ab");
        vSizeOnly.insert(0, "ab");
        vSzSized.insert(1, "cd");
        vSizeAndSz.insert(1, "cd");
        vSizeOnly.insert(1, "cd");
        EXPECT_EQ(7, vSzSized.m_data.size());
        EXPECT_EQ(7, vSizeAndSz.m_data.size());
        EXPECT_EQ(5, vSizeOnly.m_data.size());
    }

    // Testing handling of size_type argument
    {
        MapToStringViews<int, std::string, StringStorageType::sizeAndNullTerminated, uint16> v16;
        MapToStringViews<int, std::string, StringStorageType::sizeAndNullTerminated, uint32> v32;
        DFGTEST_STATIC(sizeof(decltype(v16)::StringDetail) < sizeof(decltype(v32)::StringDetail));
        v16.insert(0, "abc");
        v32.insert(0, "abc");
        EXPECT_EQ(v16[0], v32[0]);
    }

    // Testing that trying to insert more chars than allowed by size_type is handled.
    {
        MapToStringViews<int, std::string, StringStorageType::sizeAndNullTerminated, uint8> v8;
        const std::string s = std::string(200, 'a');
        v8.insert(0, s);
        v8.insert(1, s);
        EXPECT_EQ(1, v8.size()); // Expecting that latter insert-request is ignored.
        EXPECT_EQ(s, v8[0]);
    }

    // Testing that inserting empty items use sharedNull.
    {
        MapToStringViews<int, std::string> mDefault;
        MapToStringViews<int, std::string, StringStorageType::szSized> mSzSized;
        MapToStringViews<int, std::string, StringStorageType::sizeOnly> mSizeOnly;
        for (int i = 0; i < 10; ++i)
        {
            mDefault.insert(i, "");
            mSzSized.insert(i, "");
            mSizeOnly.insert(i, "");
        }
        EXPECT_EQ("", mDefault[0]);
        EXPECT_EQ("", mSzSized[0]);
        EXPECT_EQ("", mSizeOnly[0]);
        EXPECT_EQ(1, mDefault.m_data.size());
        EXPECT_EQ(1, mSzSized.m_data.size());
        EXPECT_EQ(1, mSizeOnly.m_data.size());
    }

    // Testing that trying to insert more entries than allowed by size_type is handled.
    {
        MapToStringViews<int, std::string, StringStorageType::sizeAndNullTerminated, uint8> m8;
        for (int i = 0; i < 260; ++i)
        {
            m8.insert(i, "");
        }
        EXPECT_EQ(255, m8.size());
    }

    // Testing basic iteration
    {
        MapToStringViews<int, std::string> m8;
        m8.insert(0, "a");
        m8.insert(1, "b");
        m8.insert(2, "c");
        std::string s;
        for (const auto& item : m8)
        {
            s.append(item.second(m8).c_str());
        }
        EXPECT_EQ("abc", s);
    }

    // Testing basic iteration with types string
    {
        MapToStringViews<size_t, StringUtf8> m;
        m.insert(7, DFG_UTF8("a"));
        m.insert(9, DFG_UTF8("b"));
        const std::array<size_t, 2> expectedKeys = {7, 9};
        const std::array<StringUtf8, 2> expectedValues = { StringUtf8(DFG_UTF8("a")), StringUtf8(DFG_UTF8("b")) };
        std::vector<size_t> keys;
        std::vector<StringUtf8> values;
        for (const auto& item : m)
        {
            keys.push_back(item.first);
            values.push_back(item.second(m).toString());
        }
        ASSERT_EQ(expectedKeys.size(), keys.size());
        ASSERT_EQ(expectedValues.size(), values.size());
        EXPECT_TRUE(std::equal(expectedKeys.begin(), expectedKeys.end(), keys.begin()));
        EXPECT_TRUE(std::equal(expectedValues.begin(), expectedValues.end(), values.begin()));
    }

    // Testing front/back operations
    {
        MapToStringViews<int, std::string> m;
        m.insert(-20, "a");
        m.insert(5, "b");
        m.insert(30, "c");
        EXPECT_EQ(-20, m.frontKey());
        EXPECT_EQ("a", m.frontValue());
        EXPECT_EQ(30,  m.backKey());
        EXPECT_EQ("c", m.backValue());
    }
}

TEST(dfgCont, SortedSequence)
{
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(alg);
    using namespace DFG_MODULE_NS(rand);

    auto randEng = createDefaultRandEngineRandomSeeded();
    auto distrEng = makeDistributionEngineUniform(&randEng, -1000, 1000);

    {
        const size_t nCount = 100;

        DFG_CLASS_NAME(SortedSequence)<std::vector<int>> sseq;
        DFG_CLASS_NAME(SortedSequence)<std::deque<int>> sseqDeque;
        DFG_CLASS_NAME(SortedSequence)<std::list<int>> sseqList;

        EXPECT_TRUE(sseq.empty());
        EXPECT_TRUE(sseqDeque.empty());
        EXPECT_TRUE(sseqList.empty());

        sseq.reserve(nCount);

        for (size_t i = 0; i < nCount; ++i)
        {
            auto val = distrEng();
            sseq.insert(val);
            sseqDeque.insert(val);
            sseqList.insert(val);
            EXPECT_TRUE(std::is_sorted(sseq.begin(), sseq.end()));
            EXPECT_TRUE(std::equal(sseq.begin(), sseq.end(), sseqDeque.begin()));
            EXPECT_TRUE(std::equal(sseq.begin(), sseq.end(), sseqList.begin()));

            ASSERT_TRUE(sseq.find(val) != sseq.end());
            EXPECT_EQ(val, *sseq.find(val));

            ASSERT_TRUE(sseqDeque.find(val) != sseqDeque.end());
            EXPECT_EQ(val, *sseqDeque.find(val));

            ASSERT_TRUE(sseqList.find(val) != sseqList.end());
            EXPECT_EQ(val, *sseqList.find(val));
        }
    }
}

TEST(dfgCont, InterleavedXsortedTwoChannelWrapper)
{
    using namespace DFG_MODULE_NS(cont);
    const std::array<double, 6> arr = { 1, 3, 5, 10, 15, 60 };
    DFG_CLASS_NAME(InterleavedXsortedTwoChannelWrapper)<const double> wrap(arr);
    EXPECT_EQ(3, wrap.getFirstY());
    EXPECT_EQ(60, wrap.getLastY());
    EXPECT_EQ(15, wrap.getXMax());
    EXPECT_EQ(1, wrap.getXMin());
    EXPECT_EQ(3, wrap.getY(0));
    EXPECT_EQ(10, wrap.getY(1));
    EXPECT_EQ(60, wrap.getY(2));

    EXPECT_EQ(3, wrap(0));
    EXPECT_EQ(3, wrap(1));
    EXPECT_EQ(6.5, wrap(3));
    EXPECT_EQ(10, wrap(5));
    EXPECT_EQ(35, wrap(10));
    EXPECT_EQ(60, wrap(15));
    EXPECT_EQ(60, wrap(150));
}

namespace
{
    template  <class Map_T>
    std::map<std::string, int> createStdMap(const Map_T& m)
    {
        std::map<std::string, int> rv;
        for (auto iter = m.begin(), iterEnd = m.end(); iter != iterEnd; ++iter)
            rv[iter->first] = iter->second;
        return rv;
    }

    template <class Map_T>
    void testMapInterface(Map_T& m, const unsigned long nRandEngSeed)
    {
        using namespace DFG_ROOT_NS;

        const Map_T& mConst = m;

        EXPECT_EQ(0, m.size());
        EXPECT_EQ(0, mConst.size());
        EXPECT_EQ(true, m.empty());
        EXPECT_EQ(true, mConst.empty());

        // operator[]
        m["a"] = 1;
        EXPECT_EQ(1, m.size());
        EXPECT_EQ(1, mConst.size());
        m["b"] = 2;
        EXPECT_EQ(2, m.size());
        EXPECT_EQ(2, mConst.size());
        m["a"] = 3; // Replacing.
        EXPECT_EQ(2, m.size());
        EXPECT_EQ(2, mConst.size());
        m[std::string("c")] = 'c';
        EXPECT_EQ(3, m.size());
        EXPECT_EQ(3, mConst.size());

        const std::string sConstKey = "f";
        m[sConstKey] = 'f';
        EXPECT_EQ(4, m.size());
        EXPECT_EQ(4, mConst.size());

        // find
        EXPECT_TRUE(m.find("a") != m.end());
        EXPECT_TRUE(mConst.find("a") != mConst.end());

        // insert
        {
            auto i1 = m.insert(std::pair<std::string, int>("d", 'd'));
            EXPECT_EQ(true, i1.second);
            EXPECT_EQ("d", i1.first->first);
            EXPECT_EQ('d', i1.first->second);

            auto i2 = m.insert(std::pair<std::string, int>("d", 'e'));
            EXPECT_EQ(false, i2.second);
            EXPECT_EQ("d", i2.first->first);
            EXPECT_EQ('d', i2.first->second);

            auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
            randEng.seed(nRandEngSeed);
            for (size_t i = 0; i < 50; ++i)
            {
                std::string s(1, DFG_MODULE_NS(rand)::rand<int8>(randEng, 0, 127));
                m.insert(std::pair<std::string, int>(s, DFG_MODULE_NS(rand)::rand(randEng, 1000, 2000)));
            }
        }

        // move constructor and move assignment
        {
            ASSERT_FALSE(m.empty());
            Map_T mCopy = m;
            Map_T mCopy2 = m;
            Map_T mCopy3 = m;
            Map_T mMoveCtor(std::move(mCopy));
            Map_T mMoveAssign;
            mMoveAssign = std::move(mCopy2);
            EXPECT_TRUE(mCopy.empty());
            EXPECT_TRUE(mCopy2.empty());
            ASSERT_EQ(m.size(), mCopy3.size());
            ASSERT_EQ(m.size(), mMoveCtor.size());
            ASSERT_EQ(m.size(), mMoveAssign.size());
            EXPECT_TRUE(createStdMap(m) == createStdMap(mCopy3));
            EXPECT_TRUE(createStdMap(m) == createStdMap(mMoveCtor));
            EXPECT_TRUE(createStdMap(m) == createStdMap(mMoveAssign));
        }
    }

    template <class Map_T>
    void verifyEqual(const std::map<std::string, int>& mStd, const Map_T& m)
    {
        EXPECT_TRUE(mStd == createStdMap(m));
    }

    template <class Map_T>
    void eraseTester(Map_T& m)
    {
        const auto rv0 = m.erase(m.begin());
        EXPECT_EQ(m.begin(), rv0);
        EXPECT_EQ(1, m.erase(m.frontKey()));
        EXPECT_EQ(0, m.erase("invalid_key"));

        // Test unsorted removal
        if (!m.isSorted())
        {
            auto mEqualityTester = createStdMap(m);
            mEqualityTester.erase(m.backKey());
            m.erase(m.backIter());
            verifyEqual(mEqualityTester, m);

            mEqualityTester.erase(m.frontKey());
            m.erase(m.begin());
            verifyEqual(mEqualityTester, m);

            EXPECT_TRUE(m.size() > 30); // Just a arbitrary test to make sure that m has big enough size for these tests.
            const auto nRemoveCount = 3 * m.size() / 4;
            const auto iterFirst = m.begin() + 3;
            const auto iterEnd = iterFirst + nRemoveCount;
            for (auto i = iterFirst; i != iterEnd; ++i)
                mEqualityTester.erase(i->first);
            m.erase(iterFirst, iterEnd);
            verifyEqual(mEqualityTester, m);
        }

        // Erase all
        const auto rv1 = m.erase(m.begin(), m.end());
        EXPECT_EQ(0, m.size());
        EXPECT_TRUE(m.empty());
        EXPECT_EQ(m.end(), rv1);

        m["a"] = 1;
        m.clear();
        EXPECT_TRUE(m.empty());
    }

    template <class Map_T>
    void verifyMapVectors(Map_T& mSorted, Map_T& mUnsorted, const std::map<std::string, int>& mapExpected)
    {
        DFGTEST_STATIC((std::is_same<std::string, typename Map_T::key_type>::value));
        DFGTEST_STATIC((std::is_same<int, typename Map_T::mapped_type>::value));

        ASSERT_FALSE(mapExpected.empty());
        ASSERT_FALSE(mSorted.empty());
        ASSERT_FALSE(mUnsorted.empty());

        EXPECT_EQ((*mapExpected.begin()).first, (*mSorted.begin()).first);

        EXPECT_TRUE(mSorted.hasKey(mapExpected.begin()->first));
        EXPECT_TRUE(mUnsorted.hasKey(mapExpected.begin()->first));

        EXPECT_FALSE(mSorted.hasKey("invalid_key"));
        EXPECT_FALSE(mUnsorted.hasKey("invalid_key"));

        EXPECT_EQ(mSorted.backIter()->first, mSorted.backKey());
        EXPECT_EQ(mUnsorted.backIter()->first, mUnsorted.backKey());

        EXPECT_EQ(mSorted.backIter()->second, mSorted.backValue());
        EXPECT_EQ(mUnsorted.backIter()->second, mUnsorted.backValue());

        EXPECT_EQ(mSorted.begin()->first, mSorted.frontKey());
        EXPECT_EQ(mUnsorted.begin()->first, mUnsorted.frontKey());

        EXPECT_EQ(mSorted.begin()->second, mSorted.frontValue());
        EXPECT_EQ(mUnsorted.begin()->second, mUnsorted.frontValue());

        {
            const auto mapFromSorted = createStdMap(mSorted);
            EXPECT_EQ(mapExpected, mapFromSorted);
        }

        {
            const auto mapFromUnsorted = createStdMap(mUnsorted);
            EXPECT_EQ(mapExpected, mapFromUnsorted);
        }

        // Test insert(key, value) -interface.
        mSorted.insert(std::string("insert_two_param"), 123);
        mUnsorted.insert(std::string("insert_two_param"), 123);

        // Check that algorithms compile (may fail if iterator is missing typedefs etc.).
        EXPECT_EQ(mSorted.size(), std::distance(mSorted.cbegin(), mSorted.cend()));
        std::for_each(mSorted.cbegin(), mSorted.cend(), [](DFG_ROOT_NS::DFG_CLASS_NAME(Dummy)) {});

        // Test that setSorting(true) sorts.
        {
            auto mSortedFromUnsorted = mUnsorted;
            mSortedFromUnsorted.setSorting(true);

            EXPECT_TRUE(std::equal(mSorted.begin(), mSorted.end(), mSortedFromUnsorted.begin()));
        }

        eraseTester(mSorted);
        eraseTester(mUnsorted);
    }

    class CopyTracker
    {
    public:
        CopyTracker(int val = 0)
            : m_nVal(val)
        {}

        CopyTracker(const CopyTracker& other) :
            m_nVal(other.m_nVal)
        {
            ++s_nCopyCount;
        }

        CopyTracker& operator=(const CopyTracker& other)
        {
            this->m_nVal = other.m_nVal;
            ++s_nCopyCount;
            return *this;
        }

        operator int() const { return m_nVal; }

        int m_nVal = 0;
        static int s_nCopyCount;
    };

    int CopyTracker::s_nCopyCount = 0;

    template <class Key_T, class Value_T> using MapVectorAoSHelper = ::DFG_MODULE_NS(cont)::MapVectorAoS<Key_T, Value_T>;
    template <class Key_T, class Value_T> using MapVectorSoAHelper = ::DFG_MODULE_NS(cont)::MapVectorSoA<Key_T, Value_T>;

    template <template <class, class> class Map_T>
    void testMapKeyValueMoves()
    {
        using MapT = Map_T<std::string, std::string>;
        MapT mm;
        std::string s(30, 'a');
        std::string s2(30, 'b');
        std::string s3(30, 'c');
        std::string s4(30, 'd');
        std::string s5(30, 'e');
        std::string s6(30, 'f');
        const std::string s7(30, 'g');
        mm[std::move(s)] = std::move(s2);
        mm.insert(std::move(s3), std::move(s4));
        EXPECT_TRUE(s.empty());
        EXPECT_TRUE(s2.empty());
        EXPECT_TRUE(s3.empty());
        EXPECT_TRUE(s4.empty());
        EXPECT_EQ(2, mm.size());
        // Moving key-only
        mm.insert(std::move(s5), s6);
        EXPECT_TRUE(s5.empty());
        EXPECT_EQ(s6, mm[std::string(30, 'e')]);
        // Moving value-only
        mm.insert(s7, std::move(s6));
        EXPECT_TRUE(s6.empty());
        EXPECT_EQ(std::string(30, 'f'), mm[s7]);

        // Testing that insert() won't copy unless needed.
        {
            using Map1 = Map_T<CopyTracker, CopyTracker>;
            Map1 m;
            m[CopyTracker(1)] = CopyTracker(2);
            // non-movable key, movable value
            {
                const auto nCopyCountBefore = CopyTracker::s_nCopyCount;
                CopyTracker ct(1);
                m.insert(ct, CopyTracker(2));
                EXPECT_EQ(nCopyCountBefore, CopyTracker::s_nCopyCount);
            }
            // movable key, non-movable value
            {
                const auto nCopyCountBefore = CopyTracker::s_nCopyCount;
                CopyTracker ct(2);
                m.insert(CopyTracker(1), ct);
                EXPECT_EQ(nCopyCountBefore, CopyTracker::s_nCopyCount);
            }
            // non-movable key, non-movable value
            {
                const auto nCopyCountBefore = CopyTracker::s_nCopyCount;
                CopyTracker ct0(1);
                CopyTracker ct1(2);
                m.insert(ct0, ct1);
                EXPECT_EQ(nCopyCountBefore, CopyTracker::s_nCopyCount);
            }
            // movable key, movable value
            {
                const auto nCopyCountBefore = CopyTracker::s_nCopyCount;
                m.insert(CopyTracker(1), CopyTracker(2));
                EXPECT_EQ(nCopyCountBefore, CopyTracker::s_nCopyCount);
            }
        }
    }

} // unnamed namespace

TEST(dfgCont, MapVector)
{
    using namespace DFG_MODULE_NS(cont);

    std::map<std::string, int> mStd;
    DFG_CLASS_NAME(MapVectorSoA)<std::string, int> mapVectorSoaSorted;
    DFG_CLASS_NAME(MapVectorSoA)<std::string, int> mapVectorSoaUnsorted;
    mapVectorSoaUnsorted.setSorting(false);
    DFG_CLASS_NAME(MapVectorAoS)<std::string, int> mapVectorAosSorted;
    DFG_CLASS_NAME(MapVectorAoS)<std::string, int> mapVectorAosUnsorted;
    mapVectorAosUnsorted.setSorting(false);

    const int randEngSeed = 12345678;
    testMapInterface(mStd, randEngSeed);
    mapVectorSoaSorted.reserve(10);
    testMapInterface(mapVectorSoaSorted, randEngSeed);
    testMapInterface(mapVectorSoaUnsorted, randEngSeed);
    EXPECT_EQ(mStd.size(), mapVectorSoaSorted.size());
    EXPECT_EQ(mStd.size(), mapVectorSoaUnsorted.size());

    testMapInterface(mapVectorAosSorted, randEngSeed);
    testMapInterface(mapVectorAosUnsorted, randEngSeed);
    EXPECT_EQ(mStd.size(), mapVectorAosSorted.size());
    EXPECT_EQ(mStd.size(), mapVectorAosUnsorted.size());

    // Test some basic []-operations.
    {
        DFG_CLASS_NAME(MapVectorAoS)<size_t, double> mm;
        size_t i = 0;
        mm[i] = 2;
        mm[size_t(3)] = 4;
    }

    testMapKeyValueMoves<MapVectorAoSHelper>();
    testMapKeyValueMoves<MapVectorSoAHelper>();

    verifyMapVectors(mapVectorSoaSorted, mapVectorSoaUnsorted, mStd);
    verifyMapVectors(mapVectorAosSorted, mapVectorAosUnsorted, mStd);

    // Test access to underlying key/value container for SoA-case.
    {
        DFG_CLASS_NAME(MapVectorSoA)<float, double> xyVals;
        xyVals.insert(1, 5);
        xyVals.insert(2, 6);
        xyVals.insert(3, 7);
        xyVals.insert(4, 8);
        const auto& xyValsConst = xyVals;
        EXPECT_EQ(10, DFG_MODULE_NS(numeric)::accumulate(xyVals.keyRange()));
        EXPECT_EQ(26, DFG_MODULE_NS(numeric)::accumulate(xyVals.valueRange()));
        EXPECT_EQ(26, DFG_MODULE_NS(numeric)::accumulate(xyVals.valueRange_const()));
        EXPECT_EQ(10, DFG_MODULE_NS(numeric)::accumulate(xyValsConst.keyRange()));
        EXPECT_EQ(26, DFG_MODULE_NS(numeric)::accumulate(xyValsConst.valueRange()));

        // Test editing keys and values
        auto keyRange = xyVals.keyRange_modifiable();
        auto valueRange = xyVals.valueRange();
        std::transform(keyRange.begin(), keyRange.end(), keyRange.begin(), [](const float d) { return d+1; });
        std::transform(valueRange.begin(), valueRange.end(), valueRange.begin(), [](const double d) { return d + 1; });

        EXPECT_EQ(14, DFG_MODULE_NS(numeric)::accumulate(xyVals.keyRange()));
        EXPECT_EQ(30, DFG_MODULE_NS(numeric)::accumulate(xyVals.valueRange()));
    }

    // Test insert
    {
        using namespace DFG_ROOT_NS;
        DFG_CLASS_NAME(MapVectorAoS)<int32, double> mmAos;
        DFG_CLASS_NAME(MapVectorSoA)<int32, double> mmSoa;
        int16 i16 = 3;
        int32 i = 1;
        const int32 ci = 2;
        mmAos.insert(i, 10.0);
        mmAos.insert(i, 10.0);
        mmAos.insert(ci, 20.0);
        mmSoa.insert(i, 10.0);
        mmSoa.insert(ci, 20.0);
        mmAos.insert(i16, 30.0);
        mmSoa.insert(i16, 30.0);
        
        const int32 kval = 4;
        const double dval = 40;
        mmAos.insert(kval, dval);
        mmSoa.insert(kval, dval);

        EXPECT_EQ(10, mmAos[i]);
        EXPECT_EQ(10, mmSoa[i]);
        EXPECT_EQ(20, mmAos[ci]);
        EXPECT_EQ(20, mmSoa[ci]);
        EXPECT_EQ(30, mmAos[i16]);
        EXPECT_EQ(30, mmSoa[i16]);
        EXPECT_EQ(dval, mmAos[kval]);
        EXPECT_EQ(dval, mmSoa[kval]);
    }

    // Testing erase
    {
        {
            MapVectorSoA<int, int> mIntInt;
            mIntInt[1] = 2;
            // erase should return the number of elements removed.
            EXPECT_EQ(0, mIntInt.erase(10));
            EXPECT_EQ(1, mIntInt.erase(1));
            EXPECT_TRUE(mIntInt.empty());
        }
        {
            MapVectorAoS<int, int> mIntInt;
            mIntInt[1] = 2;
            // erase should return the number of elements removed.
            EXPECT_EQ(0, mIntInt.erase(10));
            EXPECT_EQ(1, mIntInt.erase(1));
            EXPECT_TRUE(mIntInt.empty());
        }
    }
}

TEST(dfgCont, MapVector_pushBack)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using MapSoaIntInt = DFG_CLASS_NAME(MapVectorSoA)<int, int>;

    {
        MapSoaIntInt m;
        std::vector<int> vals0 = { 10, 20, 30 };
        std::vector<int> vals1 = { 1, 2, 3 };
        m.pushBackToUnsorted(vals0, vals1);
        EXPECT_EQ(0, m.size());
        m.setSorting(false);
        m.pushBackToUnsorted(vals0, vals1);
        EXPECT_EQ(3, m.size());
        m.pushBackToUnsorted(vals1, vals0);
        EXPECT_EQ(6, m.size());
        std::vector<int> expectedKeys = { 10, 20, 30, 1, 2, 3 };
        std::vector<int> expectedVals = { 1, 2, 3, 10, 20, 30 };
        ASSERT_EQ(expectedKeys.size(), m.keyRange().size());
        ASSERT_EQ(expectedVals.size(), m.valueRange().size());
        EXPECT_TRUE(std::equal(m.keyRange().begin(), m.keyRange().end(), expectedKeys.begin()));
        EXPECT_TRUE(std::equal(m.valueRange().begin(), m.valueRange().end(), expectedVals.begin()));


        m.pushBackToUnsorted(vals0, [](int i) { return i + 1; }, vals1, [](int i) { return i - 1; });
        expectedKeys.push_back(11);
        expectedKeys.push_back(21);
        expectedKeys.push_back(31);
        expectedVals.push_back(0);
        expectedVals.push_back(1);
        expectedVals.push_back(2);
        EXPECT_TRUE(std::equal(m.keyRange().begin(), m.keyRange().end(), expectedKeys.begin()));
        EXPECT_TRUE(std::equal(m.valueRange().begin(), m.valueRange().end(), expectedVals.begin()));
    }
}

namespace
{
    template <class Set_T>
    void testSetInterface(Set_T& se, const unsigned long nRandEngSeed)
    {
        using namespace DFG_ROOT_NS;

        const Set_T& seConst = se;

        EXPECT_EQ(0, se.size());
        EXPECT_EQ(0, seConst.size());
        EXPECT_EQ(true, se.empty());
        EXPECT_EQ(true, seConst.empty());

        // insert
        se.insert("a");
        EXPECT_EQ(1, se.size());
        EXPECT_EQ(1, seConst.size());
        se.insert("b");
        EXPECT_EQ(2, se.size());
        EXPECT_EQ(2, seConst.size());
        se.insert("a");
        EXPECT_EQ(2, se.size());
        EXPECT_EQ(2, seConst.size());
        se.insert("c");
        EXPECT_EQ(3, se.size());
        EXPECT_EQ(3, seConst.size());

        // find
        EXPECT_TRUE(se.find("a") != se.end());
        EXPECT_TRUE(seConst.find("a") != seConst.end());

        auto i1 = se.insert("d");
        EXPECT_EQ(true, i1.second);
        EXPECT_EQ("d", *i1.first);

        auto i2 = se.insert("d");
        EXPECT_EQ(false, i2.second);
        EXPECT_EQ("d", *i2.first);

        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        randEng.seed(nRandEngSeed);
        for (size_t i = 0; i < 50; ++i)
        {
            std::string s(1, DFG_MODULE_NS(rand)::rand<int8>(randEng, 0, 127));
            se.insert(s);
        }

        Set_T seCopy = se;
        Set_T seCopy2 = se;
        Set_T seMoved = std::move(seCopy);
        Set_T seAssign;
        Set_T seAssign2;
        seAssign = std::move(seCopy2);
        seAssign2 = se;
        EXPECT_TRUE(seCopy.empty());
        EXPECT_TRUE(seCopy2.empty());
        EXPECT_TRUE(se == se);
        EXPECT_FALSE(se == seCopy);
        EXPECT_TRUE(se != seCopy);
        EXPECT_EQ(se, seMoved);
        EXPECT_EQ(se, seAssign);
        EXPECT_EQ(se, seAssign2);
    }

    template  <class Set_T>
    std::set<std::string> createStdSet(const Set_T& se)
    {
        std::set<std::string> rv;
        for (auto iter = se.begin(), iterEnd = se.end(); iter != iterEnd; ++iter)
            rv.insert(*iter);
        return rv;
    }

    template <class Set_T>
    void verifyEqual(const std::set<std::string>& seStd, const Set_T& se)
    {
        EXPECT_TRUE(seStd == createStdSet(se));
    }

    template <class Set_T>
    void eraseTesterForSet(Set_T& se)
    {
        const auto rv0 = se.erase(se.begin());
        EXPECT_EQ(se.begin(), rv0);
        EXPECT_EQ(1, se.erase(se.front()));
        EXPECT_EQ(0, se.erase("invalid_key"));

        // Test unsorted removal
        if (!se.isSorted())
        {
            auto seEqualityTester = createStdSet(se);
            seEqualityTester.erase(se.back());
            se.erase(se.backIter());
            verifyEqual(seEqualityTester, se);

            seEqualityTester.erase(se.front());
            se.erase(se.begin());
            verifyEqual(seEqualityTester, se);

            EXPECT_TRUE(se.size() > 30); // Just a arbitrary test to make sure that m has big enough size for these tests.
            const auto nRemoveCount = 3 * se.size() / 4;
            const auto iterFirst = se.begin() + 3;
            const auto iterEnd = iterFirst + nRemoveCount;
            for (auto i = iterFirst; i != iterEnd; ++i)
                seEqualityTester.erase(*i);
            se.erase(iterFirst, iterEnd);
            verifyEqual(seEqualityTester, se);
        }

        // Erase all
        const auto rv1 = se.erase(se.begin(), se.end());
        EXPECT_EQ(0, se.size());
        EXPECT_TRUE(se.empty());
        EXPECT_EQ(se.end(), rv1);

        se.insert("a");
        se.clear();
        EXPECT_TRUE(se.empty());
    }

    template <class Set_T>
    void verifySetVectors(Set_T& seSorted, Set_T& seUnsorted, const std::set<std::string>& setExpected)
    {
        DFGTEST_STATIC((std::is_same<std::string, typename Set_T::key_type>::value));

        ASSERT_FALSE(setExpected.empty());

        EXPECT_TRUE(seSorted.hasKey(*setExpected.begin()));
        EXPECT_TRUE(seUnsorted.hasKey(*setExpected.begin()));

        EXPECT_FALSE(seSorted.hasKey("invalid_key"));
        EXPECT_FALSE(seUnsorted.hasKey("invalid_key"));

        EXPECT_EQ(*seSorted.backIter(), seSorted.back());
        EXPECT_EQ(*seUnsorted.backIter(), seUnsorted.back());

        EXPECT_EQ(*seSorted.begin(), seSorted.front());
        EXPECT_EQ(*seUnsorted.begin(), seUnsorted.front());

        verifyEqual(setExpected, seSorted);
        verifyEqual(setExpected, seUnsorted);

        // Test that setSorting(true) sorts.
        {
            auto seSortedFromUnsorted = seUnsorted;
            seSortedFromUnsorted.setSorting(true);

            EXPECT_TRUE(std::equal(seSorted.begin(), seSorted.end(), seSortedFromUnsorted.begin())); // Note: this does not work for SoA-iterator due to missing value_type typedef.
        }

        eraseTesterForSet(seSorted);
        eraseTesterForSet(seUnsorted);
    }

} // unnamed namespace

TEST(dfgCont, SetVector)
{
    using namespace DFG_MODULE_NS(cont);

    std::set<std::string> mStd;
    DFG_CLASS_NAME(SetVector)<std::string> setVectorSorted;
    DFG_CLASS_NAME(SetVector)<std::string> setVectorUnsorted; setVectorUnsorted.setSorting(false);
    
    const int randEngSeed = 12345678;
    testSetInterface(mStd, randEngSeed);
    testSetInterface(setVectorSorted, randEngSeed);
    testSetInterface(setVectorUnsorted, randEngSeed);

    verifyEqual(mStd, setVectorSorted);
    verifyEqual(mStd, setVectorUnsorted);

    verifySetVectors(setVectorSorted, setVectorUnsorted, mStd);

    // Test that SetVector works with std::inserter.
    {
        DFG_CLASS_NAME(SetVector)<std::string> setVectorSorted2;
        DFG_CLASS_NAME(SetVector)<std::string> setVectorUnsorted2; setVectorUnsorted2.setSorting(false);
        auto insertIter = std::inserter(setVectorSorted2, setVectorSorted2.end());
        *insertIter = "bcd";
        *insertIter = "abc";
        *insertIter = "abc";
        auto insertIter2 = std::inserter(setVectorUnsorted2, setVectorUnsorted2.end());
        *insertIter2 = "bcd";
        *insertIter2 = "abc";
        *insertIter2 = "abc";
        EXPECT_EQ(2, setVectorSorted2.size());
        EXPECT_EQ(setVectorSorted2.size(), setVectorUnsorted2.size());
        EXPECT_EQ(setVectorSorted2, setVectorUnsorted2);
    }

    // Test operator==
    {
        DFG_CLASS_NAME(SetVector)<std::string> setVectorSorted3;
        DFG_CLASS_NAME(SetVector)<std::string> setVectorUnsorted3; setVectorUnsorted3.setSorting(false);
        DFG_CLASS_NAME(SetVector)<std::string> setVectorUnsorted4; setVectorUnsorted4.setSorting(false);
        setVectorSorted3.insert("b");
        setVectorSorted3.insert("a");
        setVectorUnsorted3.insert("b");
        setVectorUnsorted3.insert("a");
        setVectorUnsorted4.insert("a");
        setVectorUnsorted4.insert("b");
        EXPECT_EQ(setVectorSorted3, setVectorUnsorted3); // Since sets have different sorting, their internal data storage is different
        EXPECT_EQ(setVectorUnsorted3, setVectorUnsorted4); // Both are unsorted but elements are stored in different order.
        setVectorSorted3.insert("d");
        setVectorUnsorted3.insert("e");
        setVectorUnsorted4.insert("c");
        EXPECT_NE(setVectorSorted3, setVectorUnsorted4);
        EXPECT_NE(setVectorUnsorted3, setVectorUnsorted4);
    }

    // Test some basic []-operations.
    {
        DFG_CLASS_NAME(SetVector)<int> si;
        si.insert(1);
        int i = 2;
        si.insert(i);
        const int ci = 3;
        si.insert(ci);
        EXPECT_EQ(3, si.size());
    }

    // Test moves
    {
        DFG_CLASS_NAME(SetVector)<std::string> se;
        std::string s(30, 'a');
        se.insert(std::move(s));
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(1, se.size());
    }
}

namespace
{
    template <class T0, class T1, bool IsTrivial>
    void testTrivialPair()
    {
        using namespace DFG_MODULE_NS(cont);
        typedef DFG_CLASS_NAME(TrivialPair)<T0, T1> PairT;

        // Test that TrivialPair has expected properties.
        {
            typedef DFG_MODULE_NS(TypeTraits)::IsTriviallyCopyable<PairT> IsTriviallyCopyableType;
#if DFG_MSVC_VER != DFG_MSVC_VER_2012
            // Test that either IsTriviallyCopyableType is not available or that it returns expected result.
            DFGTEST_STATIC((std::is_base_of<DFG_MODULE_NS(TypeTraits)::UnknownAnswerType, IsTriviallyCopyableType>::value ||
                std::is_base_of<std::integral_constant<bool, IsTrivial>, IsTriviallyCopyableType>::value));
#else // DFG_MSVC_VER == DFG_MSVC_VER_2012
            DFGTEST_STATIC((std::is_base_of<std::is_trivially_copyable<PairT>, IsTriviallyCopyableType>::value));
#endif
            
#if (defined(_MSC_VER) && (_MSC_VER >= DFG_MSVC_VER_2015)) || (defined(__GNUG__) && (__GNUC__ >= 5)) || (!defined(_MSC_VER) && !defined(__GNUG__))
            DFGTEST_STATIC(std::is_trivial<PairT>::value == IsTrivial);
            DFGTEST_STATIC(std::is_trivially_constructible<PairT>::value == IsTrivial);
            DFGTEST_STATIC((std::is_trivially_assignable<PairT, PairT>::value == IsTrivial));
            DFGTEST_STATIC((std::is_trivially_constructible<PairT>::value == IsTrivial));
            DFGTEST_STATIC((std::is_trivially_copyable<PairT>::value == IsTrivial));
            DFGTEST_STATIC((std::is_trivially_copy_assignable<PairT>::value == IsTrivial));
            DFGTEST_STATIC((std::is_trivially_copy_constructible<PairT>::value == IsTrivial));
            DFGTEST_STATIC((std::is_trivially_default_constructible<PairT>::value == IsTrivial));
            DFGTEST_STATIC((std::is_trivially_destructible<PairT>::value == IsTrivial));
            DFGTEST_STATIC((std::is_trivially_move_assignable<PairT>::value == IsTrivial));
            DFGTEST_STATIC((std::is_trivially_move_constructible<PairT>::value == IsTrivial));
            DFGTEST_STATIC((std::is_move_constructible<PairT>::value == (std::is_move_constructible<T0>::value && std::is_move_constructible<T1>::value)));
            DFGTEST_STATIC((std::is_nothrow_move_constructible<PairT>::value == (std::is_nothrow_move_constructible<T0>::value && std::is_nothrow_move_constructible<T1>::value)));
            DFGTEST_STATIC((std::is_nothrow_constructible<PairT>::value == (std::is_nothrow_constructible<T0>::value && std::is_nothrow_constructible<T1>::value)));
            DFGTEST_STATIC((std::is_nothrow_copy_constructible<PairT>::value == IsTrivial));
            DFGTEST_STATIC((std::is_nothrow_constructible<PairT, T0, T1>::value == (std::is_nothrow_constructible<T0>::value && std::is_nothrow_constructible<T1>::value)));
            DFGTEST_STATIC((std::is_nothrow_constructible<PairT, T0&, T1&>::value == IsTrivial));
            DFGTEST_STATIC((std::is_nothrow_constructible<PairT, T0&&, T1&&>::value == (std::is_nothrow_constructible<T0, T0&&>::value && std::is_nothrow_constructible<T1, T1&&>::value)));
#endif
        }

        // Test value initialization
        {
            auto tp = DFG_CLASS_NAME(TrivialPair)<T0, T1>();
            //DFG_CLASS_NAME(TrivialPair)<T0, T1> tp2; // This should have default initialized values when compiled with C++11 compiler (e.g. for int's value shown in debugger should be arbitrary)
            EXPECT_EQ(T0(), tp.first);
            EXPECT_EQ(T1(), tp.second);
        }

        // Test existence of two parameter constructor.
        {
            auto t0 = T0();
            auto t1 = T1();
            DFG_CLASS_NAME(TrivialPair)<T0, T1> tp2(t0, t1);
            DFG_CLASS_NAME(TrivialPair)<T0, T1> tp22(tp2.first, tp2.second);
            EXPECT_EQ(tp2, tp22);
        }

        // Test move constructor
        {
            std::vector<T0> vec0(2, T0());
            std::vector<T1> vec1(3, T1());
            DFG_CLASS_NAME(TrivialPair)<std::vector<T0>, std::vector<T1>> tp3(std::move(vec0), std::move(vec1));
            EXPECT_TRUE(vec0.empty());
            EXPECT_TRUE(vec1.empty());
            EXPECT_EQ(2, tp3.first.size());
            EXPECT_EQ(3, tp3.second.size());
        }
    }
}

TEST(dfgCont, TrivialPair)
{
    using namespace DFG_MODULE_NS(cont);
    testTrivialPair<int, int, true>();
    testTrivialPair<char, double, true>();
    testTrivialPair<DFG_CLASS_NAME(TrivialPair)<int, int>, bool, true>();
    testTrivialPair<std::string, bool, false>();

    // Test move assignment of items
    {
        DFG_CLASS_NAME(TrivialPair)<std::string, std::string> tp;
        DFG_CLASS_NAME(TrivialPair)<std::string, std::string> tp2("aa", "bb");
        tp = std::move(tp2);
        EXPECT_TRUE(tp2.first.empty());
        EXPECT_TRUE(tp2.second.empty());
        EXPECT_EQ("aa", tp.first);
        EXPECT_EQ("bb", tp.second);
    }
}

TEST(dfgCont, Vector)
{
    using namespace DFG_MODULE_NS(cont);

    {
        DFG_CLASS_NAME(Vector) < int > v;
        v.push_back(1);
        auto v2(v);
        auto v3(std::move(v));
        decltype(v) v4;
        v4 = std::move(v2);
        EXPECT_TRUE(v.empty());
        EXPECT_TRUE(v2.empty());
        EXPECT_EQ(v3, v4);
    }

    // Checking that Vector can be constructed from initializer list
    {
        const Vector<int> v = { 1, 2, 3, 4 };
        EXPECT_EQ(Vector<int>({1, 2, 3, 4}), v);
    }
}

namespace
{
    template <class Cont_T>
    void contAlgImpl()
    {
        Cont_T v;
        v.push_back(1);
        v.push_back(2);
        DFG_MODULE_NS(cont)::popFront(v);
        EXPECT_EQ(Cont_T(1, 2), v);
        v.push_back(3);
        v.push_back(4);
        auto iterTail = v.end();
        std::advance(iterTail, -1);
        DFG_MODULE_NS(cont)::cutTail(v, iterTail);
        DFG_MODULE_NS(cont)::popFront(v);
        EXPECT_EQ(Cont_T(1, 3), v);
    }

    template <class StringView_T, class Ptr_T>
    void StringViewCutTailTests(Ptr_T psz)
    {
        StringView_T sv(psz);
        ASSERT_EQ(3, sv.length());
        DFG_MODULE_NS(cont)::cutTail(sv, sv.end() - ptrdiff_t(2));
        EXPECT_EQ(1, sv.length());
        typedef decltype(sv[0]) CodePointType;
        EXPECT_EQ(CodePointType('a'), sv[0]);
    }
}

namespace
{
    template <class Cont_T>
    static void tryReserveTests()
    {
        using namespace DFG_ROOT_NS;
        {
            Cont_T cont;
            EXPECT_TRUE(DFG_MODULE_NS(cont)::tryReserve(cont, 1000));
        }
        {
            // Commented out as this triggers a (non-fatal) "Debug Error!" at least in MSVC debug builds.
            //Cont_T cont;
            //EXPECT_FALSE(DFG_MODULE_NS(cont)::tryReserve(cont, cont.max_size()));
        }
        {
            Cont_T cont;
            EXPECT_FALSE(DFG_MODULE_NS(cont)::tryReserve(cont, NumericTraits<size_t>::maxValue));
        }
    }
}

TEST(dfgCont, contAlg)
{
    DFGTEST_STATIC(DFG_MODULE_NS(cont)::DFG_DETAIL_NS::cont_contAlg_hpp::Has_pop_front<int>::value == false);
    DFGTEST_STATIC(DFG_MODULE_NS(cont)::DFG_DETAIL_NS::cont_contAlg_hpp::Has_pop_front<std::deque<int>>::value == true);
    DFGTEST_STATIC(DFG_MODULE_NS(cont)::DFG_DETAIL_NS::cont_contAlg_hpp::Has_pop_front<std::list<int>>::value == true);

    DFGTEST_STATIC(DFG_MODULE_NS(cont)::DFG_DETAIL_NS::cont_contAlg_hpp::Has_cutTail<std::vector<int>>::value == false);
    DFGTEST_STATIC(DFG_MODULE_NS(cont)::DFG_DETAIL_NS::cont_contAlg_hpp::Has_cutTail<DFG_ROOT_NS::DFG_CLASS_NAME(StringViewC)>::value == true);

    contAlgImpl<std::deque<int>>();
    contAlgImpl<std::list<int>>();
    contAlgImpl<std::string>();
    contAlgImpl<std::vector<int>>();

    // Test cutTail for StringViewC
    {
        StringViewCutTailTests<DFG_ROOT_NS::DFG_CLASS_NAME(StringViewC)>("abc");
        StringViewCutTailTests<DFG_ROOT_NS::DFG_CLASS_NAME(StringViewAscii)>(DFG_ROOT_NS::SzPtrAscii("abc"));
        StringViewCutTailTests<DFG_ROOT_NS::DFG_CLASS_NAME(StringViewLatin1)>(DFG_ROOT_NS::SzPtrLatin1("abc"));
    }

    // tryReserve()
    {
        tryReserveTests<std::vector<int>>();
        tryReserveTests<DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Vector)<int>>();
    }
}

TEST(dfgCont, contAlg_eraseIf)
{
    std::map<int, int> m;
    for (int i = 0; i < 10; ++i)
        m[i] = 0;
    ::DFG_MODULE_NS(cont)::eraseIf(m, [](const decltype(*m.begin())& v) { return v.first % 2 == 0; });
    ASSERT_EQ(5, m.size());
    EXPECT_EQ(1, m.begin()->first);
    ::DFG_MODULE_NS(cont)::eraseIf(m, [](::DFG_ROOT_NS::Dummy) { return true; });
    EXPECT_TRUE(m.empty());
}

TEST(dfgCont, VectorSso)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);

    const size_t nTotalSize = 10;

    DFG_CLASS_NAME(VectorSso)<size_t, nTotalSize-2> v;

    DFGTEST_STATIC(v.s_ssoBufferSize == nTotalSize - 2);

    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    v.push_back(5);
    v.cutTail(v.begin() + 3);
    ASSERT_EQ(3, v.size());
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_TRUE(v.isSsoStorageInUse());
    v.clear();
    EXPECT_TRUE(v.isSsoStorageInUse());

    std::vector<size_t> vVector;
    EXPECT_TRUE(v.empty());
    for (size_t i = 0; i < nTotalSize; ++i)
    {
        v.push_back(i);
        vVector.push_back(i);
    }
    EXPECT_FALSE(v.isSsoStorageInUse());
    EXPECT_EQ(nTotalSize, v.size());
    EXPECT_FALSE(v.empty());
    for (size_t i = 0; i < v.size(); ++i)
        EXPECT_EQ(i, v[i]);

    EXPECT_EQ(0, v.front());
    EXPECT_EQ(nTotalSize - 1 , v.back());

    ASSERT_EQ(v.size(), vVector.size());

    EXPECT_TRUE((std::equal(v.begin(), v.end(), vVector.begin())));

    v.pop_front();
    EXPECT_EQ(nTotalSize - 1, v.size());
    EXPECT_EQ(1, v.front());

    v.pop_back();
    EXPECT_EQ(nTotalSize - 2, v.size());
    EXPECT_EQ(nTotalSize - 2, v.back());

    v.cutTail(v.begin() + 3);
    EXPECT_EQ(3, v.size());
    EXPECT_EQ(1, v.front());
    EXPECT_EQ(3, v.back());

    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_TRUE(v.isSsoStorageInUse());
}

namespace
{
    static int gUniqueResourceHolderTestState = 1;
    static int getGlobalState()            { return gUniqueResourceHolderTestState; }
    static void setGlobalState(int newVal) { gUniqueResourceHolderTestState = newVal; }
}

TEST(dfgCont, UniqueResourceHolder)
{
    // malloc-holder
    {
        using namespace ::DFG_ROOT_NS::DFG_SUB_NS_NAME(cont);
        bool freed = false;
        {
            auto resource = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_explicitlyConvertible(reinterpret_cast<int*>(std::malloc(sizeof(int)*100)), [&](void* p) { free(p); freed = true; });
            *resource.data() = 1;
            EXPECT_EQ(1, *resource.data());
        }
        EXPECT_TRUE(freed);
    }

    // Global flag-setter
    {
        EXPECT_EQ(1, getGlobalState());
        auto resource = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_explicitlyConvertible([]() -> int {auto oldState = getGlobalState(); setGlobalState(2); return oldState; }(), [](int oldVal) { setGlobalState(oldVal); });
        EXPECT_EQ(2, getGlobalState());
    }
    EXPECT_EQ(1, getGlobalState());

    // fopen
    {
        auto retCode = EOF;
        {
            auto resource = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_explicitlyConvertible(DFG_MODULE_NS(cstdio)::fopen("testfiles/matrix_3x3.txt", "r"), [&](FILE* file) { retCode = fclose(file); });
            char buf[1];
            const auto readCount = fread(buf, 1, 1, resource.data()); // Test 'container-to-resource'-conversion.
            // const auto readCount = fread(buf, 1, 1, resource); // This should fail to compile.
            EXPECT_EQ(1, readCount);
        }
        EXPECT_EQ(0, retCode);
    }

    // fopen (implicitly convertible)
    {
        auto retCode = EOF;
        {
            auto resource = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_implicitlyConvertible(DFG_MODULE_NS(cstdio)::fopen("testfiles/matrix_3x3.txt", "r"), [&](FILE* file) { retCode = fclose(file); });
            char buf[1];
            const auto readCount = fread(buf, 1, 1, resource); // Uses implicit conversion to raw resource type.
            EXPECT_EQ(1, readCount);
        }
        EXPECT_EQ(0, retCode);
    }

    // Implicit conversions
    {
        const auto modifier = [](int* val) { return (val) ? ++(*val) : 0; };
        auto resourceVec = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_implicitlyConvertible(new std::vector<int>(1, 1), [](std::vector<int>* p) { delete p; });
        auto resourceConstVec = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_implicitlyConvertible(new const std::vector<int>(1, 1), [](const std::vector<int>* p) { delete p; });
        auto resourceInt = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_implicitlyConvertible(new int(2), [](int* p) { delete p; });
        ASSERT_EQ(1, resourceConstVec->size()); // Tests availability of -> operator for const
        resourceVec->pop_back(); // Tests availability of -> operator for non-const
        EXPECT_TRUE(resourceVec->empty());
        int* resPtr = resourceInt; // implicit conversion
        ASSERT_TRUE(resPtr != nullptr);
        EXPECT_EQ(2, *resPtr);
        EXPECT_EQ(resPtr, resourceInt.data());
        EXPECT_EQ(2, *resourceInt.data());
        //EXPECT_EQ(2, *resourceInt); // Note: this fails to compile (ambiguous overload for 'operator*').
        EXPECT_EQ(3, modifier(resourceInt)); // Tests ability to modify through implicit conversion
        //resourceInt += 2; // This should fail to compile.
    }

    // Test movability.
    {
        int val = 0;
        {
            auto resource = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_explicitlyConvertible((char*)(malloc(1)), [&](char* p) { val++; free(p);  });
            //decltype(resource) resource2 = resource; // This should fail to compile.
           
            const char* const p = resource.data();
            decltype(resource) resource3 = std::move(resource);
            EXPECT_EQ(p, resource3.data());
            EXPECT_EQ(nullptr, resource.data()); // This is implementation detail; feel free to change, but if this fails without intented changed, check implementation for logic errors.
        }
        EXPECT_EQ(1, val); // Make sure that moving disables deleter call from 'moved from' -object.
    }

    // Test size of UniqueResourceHolder (implementation details)
    {
        auto resourceRuntimeFp = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_explicitlyConvertible(malloc(1), &std::free);
        auto resourceRunTimeFp2 = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_implicitlyConvertible(malloc(1), &std::free);
        DFGTEST_STATIC(sizeof(resourceRuntimeFp) == sizeof(resourceRunTimeFp2)); // This is hard requirement
        DFGTEST_STATIC(sizeof(resourceRuntimeFp) == 3 * sizeof(int*)); // This is implementation detail; feel free to change, but if this fails without intented changes, check implementation for logic errors.

        auto resourceLambda = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_explicitlyConvertible(malloc(1), [](void* p) { std::free(p); });
        auto resourceLambda2 = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_implicitlyConvertible(malloc(1), [](void* p) { std::free(p); });
        DFGTEST_STATIC(sizeof(resourceLambda) == sizeof(resourceLambda2)); // This is hard requirement
        DFGTEST_STATIC(sizeof(resourceLambda2) == 2 * sizeof(int*)); // This is implementation detail; feel free to change (especially getting rid of 2 * would be desired),
                                                                     // but if this fails without intented changes, check implementation for logic errors.
    }

    // Test 'const UniqueResourceHolder' semantics
    {
        const auto resource0 = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_implicitlyConvertible(new int(1), [](int* p) { delete p; });
        const auto resource1 = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_implicitlyConvertible(new const int(1), [](const int* p) { delete p; });
        //const auto resource2 = DFG_MODULE_NS(cont)::makeUniqueResourceHolder_implicitlyConvertible(new const int(1), [](int* p) { delete p; }); // This should fail to compile (deleter takes non-const int*).
        //*resource1.data() = 2; // This should fail to compile (resource is const int*). 
    }
}

namespace
{
    void testIntIntervalSet1to4(const char* psz)
    {
        auto intIntervalSet = ::DFG_MODULE_NS(cont)::intervalSetFromString<int>(psz);
        EXPECT_TRUE(intIntervalSet.hasValue(1));
        EXPECT_TRUE(intIntervalSet.hasValue(2));
        EXPECT_TRUE(intIntervalSet.hasValue(3));
        EXPECT_TRUE(intIntervalSet.hasValue(4));
        EXPECT_FALSE(intIntervalSet.hasValue(0));
        EXPECT_FALSE(intIntervalSet.hasValue(5));
        EXPECT_EQ(4, intIntervalSet.sizeOfSet()); // size() returns the number of elements, not the number of intervals.
    }

    void testIntIntervalSetMinus4toMinus2(const char* psz)
    {
        auto intIntervalSet = ::DFG_MODULE_NS(cont)::intervalSetFromString<int>(psz);
        EXPECT_TRUE(intIntervalSet.hasValue(-4));
        EXPECT_TRUE(intIntervalSet.hasValue(-3));
        EXPECT_TRUE(intIntervalSet.hasValue(-2));
        EXPECT_EQ(3, intIntervalSet.sizeOfSet());
    }
}

TEST(dfgCont, IntervalSet)
{
    {
        ::DFG_MODULE_NS(cont)::IntervalSet<int> is;
        const auto verifyIntervals = [&](const std::vector<int>& expectedItems)
            {
                std::vector<int> intervalBoundaries;
                is.forEachContiguousRange([&](const int left, const int right)
                {
                    intervalBoundaries.push_back(left);
                    intervalBoundaries.push_back(right);
                });
                EXPECT_EQ(expectedItems, intervalBoundaries);
            };
        
        verifyIntervals({});
        EXPECT_TRUE(is.empty());

        is.insertClosed(1, 3);
        EXPECT_FALSE(is.empty());
        EXPECT_EQ(1, is.minElement());
        EXPECT_EQ(3, is.maxElement());
        EXPECT_EQ(3, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());
        verifyIntervals({1, 3});

        is.insertClosed(1, 1);
        EXPECT_EQ(3, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());
        is.insertClosed(2, 2);
        verifyIntervals({ 1, 3 });

        EXPECT_EQ(3, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());
        is.insertClosed(3, 3);

        EXPECT_EQ(3, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());

        is.insertClosed(1, 3);
        EXPECT_EQ(3, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());

        is.insertClosed(2, 3);
        EXPECT_EQ(3, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());

        is.insertClosed(4, 3);
        EXPECT_EQ(3, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());

        is.insertClosed(4, 4);
        EXPECT_EQ(4, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());
        verifyIntervals({ 1, 4 });

        is.insertClosed(0, 0);
        EXPECT_EQ(5, is.sizeOfSet());

        is.insertClosed(-3, -2);
        EXPECT_EQ(7, is.sizeOfSet());

        is.insertClosed(6, 8);
        EXPECT_EQ(10, is.sizeOfSet());

        is.insertClosed(5, 8);
        EXPECT_EQ(11, is.sizeOfSet());

        is.insertClosed(0, 10);
        // Should now have [-3, -2] and [0, 10]
        EXPECT_EQ(13, is.sizeOfSet());
        EXPECT_EQ(2, is.intervalCount());
        verifyIntervals({ -3, -2, 0, 10 });

        is.insertClosed(-2, 1);
        // Should now have [-3, 10]
        EXPECT_EQ(14, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());

        is.insertClosed(-4, 11);
        // Should now have [-4, 11]
        EXPECT_EQ(16, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());
        verifyIntervals({ -4, 11 });

        is.insertClosed(-6, -6);
        // Should now have [-6, -6], [-4, 11]
        EXPECT_EQ(17, is.sizeOfSet());
        EXPECT_EQ(2, is.intervalCount());

        is.insertClosed(-9, -8);
        // Should now have [-9, -8], [-6, -6], [-4, 11]
        EXPECT_EQ(19, is.sizeOfSet());
        EXPECT_EQ(3, is.intervalCount());
        verifyIntervals({ -9, -8, -6, -6, -4, 11 });
        EXPECT_FALSE(is.empty());
        EXPECT_EQ(-9, is.minElement());
        EXPECT_EQ(11, is.maxElement());

        is.insertClosed(-9, 12);
        // Should now have [-9, 12]
        EXPECT_EQ(22, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());
        EXPECT_FALSE(is.empty());
        EXPECT_EQ(-9, is.minElement());
        EXPECT_EQ(12, is.maxElement());
    }

    {
        using namespace DFG_ROOT_NS;
        ::DFG_MODULE_NS(cont)::IntervalSet<int> is;
        for (int i = 0; i < 100; ++i)
            is.insertClosed(i, i);
        EXPECT_EQ(100, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());
        EXPECT_FALSE(is.empty());
        EXPECT_EQ(0, is.minElement());
        EXPECT_EQ(99, is.maxElement());
        is.insertClosed(0, 99);
        EXPECT_EQ(100, is.sizeOfSet());
        EXPECT_EQ(1, is.intervalCount());
        EXPECT_FALSE(is.empty());
        EXPECT_EQ(0, is.minElement());
        EXPECT_EQ(99, is.maxElement());

        EXPECT_EQ(1, is.countOfElementsInRange(0, 0));
        EXPECT_EQ(51, is.countOfElementsInRange(0, 50));
        EXPECT_EQ(100, is.countOfElementsInRange(0, 99));
        EXPECT_EQ(100, is.countOfElementsInRange(0, 1000));
        EXPECT_EQ(100, is.countOfElementsInRange(-100, 1000));
        EXPECT_EQ(0, is.countOfElementsInRange(100, 1000));
        EXPECT_EQ(100, is.countOfElementsInRange(minValueOfType<int>(), maxValueOfType<int>()));
    }

    // countOfElementsInRange()
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(cont);
        IntervalSet<int> is;
        EXPECT_EQ(0, is.countOfElementsInRange(minValueOfType<int>(), maxValueOfType<int>()));
        is.insertClosed(0, 10);
        EXPECT_EQ(11, is.countOfElementsInRange(minValueOfType<int>(), maxValueOfType<int>()));
        EXPECT_EQ(11, is.countOfElementsInRange(minValueOfType<int>(), 10));
        EXPECT_EQ(1, is.countOfElementsInRange(10, maxValueOfType<int>()));

        is.insert(20);
        is.insertClosed(30, 33);
        is.insertClosed(40, 45);
        is.insert(50);

        EXPECT_EQ(12, is.countOfElementsInRange(15, 50));
        EXPECT_EQ(2, is.countOfElementsInRange(20, 30));
        EXPECT_EQ(3, is.countOfElementsInRange(20, 31));
        EXPECT_EQ(4, is.countOfElementsInRange(20, 32));
        EXPECT_EQ(5, is.countOfElementsInRange(20, 33));
        EXPECT_EQ(6, is.countOfElementsInRange(20, 40));
        EXPECT_EQ(9, is.countOfElementsInRange(20, 43));
        EXPECT_EQ(11, is.countOfElementsInRange(20, 45));
        EXPECT_EQ(4, is.countOfElementsInRange(30, 39));
        EXPECT_EQ(5, is.countOfElementsInRange(30, 40));
        EXPECT_EQ(8, is.countOfElementsInRange(30, 43));
        EXPECT_EQ(10, is.countOfElementsInRange(30, 45));
        EXPECT_EQ(10, is.countOfElementsInRange(30, 46));
        EXPECT_EQ(4, is.countOfElementsInRange(31, 40));
        EXPECT_EQ(2, is.countOfElementsInRange(33, 40));
        EXPECT_EQ(1, is.countOfElementsInRange(34, 40));
        EXPECT_EQ(2, is.countOfElementsInRange(34, 41));
        EXPECT_EQ(6, is.countOfElementsInRange(34, 45));
        EXPECT_EQ(6, is.countOfElementsInRange(34, 46));
        EXPECT_EQ(7, is.countOfElementsInRange(34, 50));
        EXPECT_EQ(7, is.countOfElementsInRange(34, 51));
        EXPECT_EQ(0, is.countOfElementsInRange(51, 34));
    }

    // countOfElementsInIntersection
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(cont);
        EXPECT_EQ(256, IntervalSet<int8>::countOfElementsInIntersection(-128, 127, -128, 127));
        EXPECT_EQ(128, IntervalSet<int8>::countOfElementsInIntersection(0, 127, -128, 127));
        EXPECT_EQ(128, IntervalSet<int8>::countOfElementsInIntersection(-128, 127, 0, 127));
        EXPECT_EQ(0, IntervalSet<int8>::countOfElementsInIntersection(0, 127, -128, -127));
        EXPECT_EQ(0, IntervalSet<int8>::countOfElementsInIntersection(-128, -127, 0, 127));
        EXPECT_EQ(51, IntervalSet<int8>::countOfElementsInIntersection(-128, 50, 0, 127));
        EXPECT_EQ(51, IntervalSet<int8>::countOfElementsInIntersection(0, 127, -128, 50));
    }

    // isSingleInterval(), makeSingleInterval()
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(cont);
        EXPECT_TRUE(IntervalSet<int>::makeSingleInterval(0, 2).isSingleInterval(0, 2));
        EXPECT_FALSE(IntervalSet<int>::makeSingleInterval(0, 2).isSingleInterval(0, 3));
        EXPECT_FALSE(IntervalSet<int>::makeSingleInterval(0, 2).isSingleInterval(1, 2));
        EXPECT_TRUE(IntervalSet<int>::makeSingleInterval(1, 0).empty());
        IntervalSet<int> is;
        EXPECT_FALSE(is.isSingleInterval(1, 3));
        is.insert(1);
        EXPECT_TRUE(is.isSingleInterval(1, 1));
        is.insert(2);
        EXPECT_TRUE(is.isSingleInterval(1, 2));
        is.insert(4);
        EXPECT_FALSE(is.isSingleInterval(1, 4));
    }

    // Handling of numerical limits
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(cont);
        IntervalSet<int32> ic;
        ic.insertClosed(int32_min, int32_min + 2);
        ic.insertClosed(10, 10);
        EXPECT_EQ(4, ic.sizeOfSet());
        EXPECT_TRUE(ic.hasValue(int32_min));
        EXPECT_TRUE(ic.hasValue(int32_min + 1));
        EXPECT_TRUE(ic.hasValue(int32_min + 2));
        EXPECT_TRUE(ic.hasValue(10));
    }
}

TEST(dfgCont, IntervalSet_wrapNegatives)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);

    // Basic test
    {
        IntervalSet<int> ic;
        ic.insertClosed(-3, -1);
        ic.insertClosed(1, 3);
        ic.wrapNegatives(-1); // Should do nothing
        ic.wrapNegatives(10);
        ASSERT_EQ(6, ic.sizeOfSet());
        EXPECT_EQ(1, ic.minElement());
        EXPECT_EQ(9, ic.maxElement());
        EXPECT_TRUE(ic.hasValue(2));
        EXPECT_TRUE(ic.hasValue(3));
        EXPECT_TRUE(ic.hasValue(7));
        EXPECT_TRUE(ic.hasValue(8));
    }

    // Overlapping with highest positives
    {
        IntervalSet<int> ic;
        // [-4, 2], [5, 6], [20, 21]
        ic.insertClosed(-4, 2);
        ic.insertClosed(5, 6);
        ic.insertClosed(20, 21);
        ASSERT_EQ(11, ic.sizeOfSet());
        ic.wrapNegatives(22);
        ASSERT_EQ(9, ic.sizeOfSet());
        EXPECT_EQ(0, ic.minElement());
        EXPECT_EQ(21, ic.maxElement());
        EXPECT_TRUE(ic.hasValue(1));
        EXPECT_TRUE(ic.hasValue(2));
        EXPECT_TRUE(ic.hasValue(5));
        EXPECT_TRUE(ic.hasValue(6));
        EXPECT_TRUE(ic.hasValue(18));
        EXPECT_TRUE(ic.hasValue(19));
        EXPECT_TRUE(ic.hasValue(20));
    }

    // Wrapping to 0
    {
        IntervalSet<int> ic;
        ic.insertClosed(-20, -10);
        ic.insertClosed(-3, -1);
        ic.insertClosed(4, 4);
        ic.wrapNegatives(0); // Should wrap negatives to 0
        ASSERT_EQ(2, ic.sizeOfSet());
        EXPECT_EQ(0, ic.minElement());
        EXPECT_EQ(4, ic.maxElement());
    }

    // Wrapping to middle
    {
        IntervalSet<int> ic;
        // [-4, 2], [5, 6], [9, 9], [20, 21]
        ic.insertClosed(-4, 2);
        ic.insertClosed(5, 6);
        ic.insertClosed(9, 9);
        ic.insertClosed(20, 21);
        ASSERT_EQ(12, ic.sizeOfSet());
        ic.wrapNegatives(10);
        // Should now have [0, 2] [5, 9], [20, 21]
        ASSERT_EQ(10, ic.sizeOfSet());
        EXPECT_EQ(0, ic.minElement());
        EXPECT_EQ(21, ic.maxElement());
        EXPECT_TRUE(ic.hasValue(1));
        EXPECT_TRUE(ic.hasValue(2));
        EXPECT_TRUE(ic.hasValue(5));
        EXPECT_TRUE(ic.hasValue(6));
        EXPECT_TRUE(ic.hasValue(7));
        EXPECT_TRUE(ic.hasValue(8));
        EXPECT_TRUE(ic.hasValue(8));
        EXPECT_TRUE(ic.hasValue(20));
    }

    // Testing numerical limits
    {
        {
            IntervalSet<int32> ic;
            ic.insertClosed(int32_min, int32_min + 2);
            ic.insertClosed(10, 10);
            ic.wrapNegatives(0);
            ASSERT_EQ(2, ic.sizeOfSet());
            EXPECT_EQ(0, ic.minElement());
            EXPECT_EQ(10, ic.maxElement());
        }

        {
            IntervalSet<int32> ic;
            // [min, min + 2], [-1], [max - 10, max - 9], [max]
            ic.insertClosed(int32_min, int32_min + 2);
            ic.insert(-1);
            ic.insertClosed(int32_max - 10, int32_max - 9);
            ic.insert(int32_max);
            ic.wrapNegatives(int32_max);
            // Should now have [0, 1], [max - 10, max - 9], [max - 1, max]
            ASSERT_EQ(6, ic.sizeOfSet());
            EXPECT_EQ(0, ic.minElement());
            EXPECT_EQ(int32_max, ic.maxElement());
            EXPECT_TRUE(ic.hasValue(1));
            EXPECT_TRUE(ic.hasValue(int32_max - 10));
            EXPECT_TRUE(ic.hasValue(int32_max - 9));
            EXPECT_TRUE(ic.hasValue(int32_max - 1));
            EXPECT_TRUE(ic.hasValue(int32_max));
        }
    }

    // Test to make sure that -1, 0 boundary does not get merged.
    {
        IntervalSet<int32> ic;
        ic.insertClosed(-9, -1);
        ic.wrapNegatives(8);
        EXPECT_EQ(8, ic.sizeOfSet());
        EXPECT_EQ(0, ic.minElement());
        EXPECT_EQ(7, ic.maxElement());
    }
}

namespace
{
    void testInt32IntervalBounds(std::true_type) // Case: 64-bit size_t
    {
        auto ic32 = ::DFG_MODULE_NS(cont)::intervalSetFromString<DFG_ROOT_NS::int32>("-2147483648 : 2147483647");
        EXPECT_EQ(4294967296ull, ic32.sizeOfSet());
    }

    void testInt32IntervalBounds(std::false_type) // Case: 32-bit size_t
    {
    }
}

TEST(dfgCont, intervalSetFromString)
{
    {
        testIntIntervalSet1to4("1:4");
        testIntIntervalSet1to4(" 1:4");
        testIntIntervalSet1to4(" 1 :4");
        testIntIntervalSet1to4(" 1 :  4");
        testIntIntervalSet1to4("1;2;3;4");
        testIntIntervalSet1to4("4;3;2;1");
        testIntIntervalSet1to4("1:2;3:4");
        testIntIntervalSet1to4("1:3;2:4");
        testIntIntervalSet1to4("5:3;1:3;2:4");
    }

    // Testing bound handling
    {
        {
            using namespace DFG_ROOT_NS;

            // int8 case commented out as strTo() doesn't properly parse int8-types.
            //auto ic8 = ::DFG_MODULE_NS(cont)::intervalSetFromString<int8>("-128 - 127");
            //EXPECT_EQ(256, ic8.sizeOfSet());

            auto ic16 = ::DFG_MODULE_NS(cont)::intervalSetFromString<int16>("-32768 : 32767");
            EXPECT_EQ(65536, ic16.sizeOfSet());

            auto ic32_0 = ::DFG_MODULE_NS(cont)::intervalSetFromString<int32>("-2147483648 : 0");
            EXPECT_EQ(2147483649, ic32_0.sizeOfSet());

            auto ic32_1 = ::DFG_MODULE_NS(cont)::intervalSetFromString<int32>("-2147483648 : -1");
            EXPECT_EQ(2147483648, ic32_1.sizeOfSet());

            testInt32IntervalBounds(std::integral_constant<bool, sizeof(size_t) >= 8>());

            //auto ic64 = ::DFG_MODULE_NS(cont)::intervalSetFromString<int16>("-32768-32767");
            //EXPECT_EQ(65536, intIntervalSet.size());
        }
    }

    // Testing parsing of negative items
    {
        testIntIntervalSetMinus4toMinus2("-4:-2");
        testIntIntervalSetMinus4toMinus2("-4:-4; -3 : -2");
        
    }
}
