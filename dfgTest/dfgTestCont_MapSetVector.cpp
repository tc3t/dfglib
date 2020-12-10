#include <stdafx.h>
#include <dfg/cont.hpp>
#include <dfg/cont/table.hpp>
#include <string>
#include <memory>
#include <dfg/ptrToContiguousMemory.hpp>
#include <dfg/dfgBase.hpp>
#include <dfg/ReadOnlySzParam.hpp>
#include <dfg/cont/CsvConfig.hpp>
#include <dfg/cont/MapVector.hpp>
#include <dfg/cont/SetVector.hpp>
#include <dfg/numeric/accumulate.hpp>

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

            EXPECT_TRUE(::DFG_MODULE_NS(cont)::isEqualContent(mSorted, mSortedFromUnsorted));

            // Code below compiled fine in MSVC2019.7.x and earlier, but no longer compiled on MSVC2019.8.3 (16.8.3)
            // Caused errors like: "error C2070: '_Elem1': illegal sizeof operand"
            //EXPECT_TRUE(std::equal(mSorted.begin(), mSorted.end(), mSortedFromUnsorted.begin()));
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
    DFG_CLASS_NAME(MapVectorSoA) < std::string, int > mapVectorSoaSorted;
    DFG_CLASS_NAME(MapVectorSoA) < std::string, int > mapVectorSoaUnsorted;
    mapVectorSoaUnsorted.setSorting(false);
    DFG_CLASS_NAME(MapVectorAoS) < std::string, int > mapVectorAosSorted;
    DFG_CLASS_NAME(MapVectorAoS) < std::string, int > mapVectorAosUnsorted;
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
        DFG_CLASS_NAME(MapVectorAoS) < size_t, double > mm;
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
        DFG_CLASS_NAME(MapVectorSoA) < float, double > xyVals;
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
        std::transform(keyRange.begin(), keyRange.end(), keyRange.begin(), [](const float d) { return d + 1; });
        std::transform(valueRange.begin(), valueRange.end(), valueRange.begin(), [](const double d) { return d + 1; });

        EXPECT_EQ(14, DFG_MODULE_NS(numeric)::accumulate(xyVals.keyRange()));
        EXPECT_EQ(30, DFG_MODULE_NS(numeric)::accumulate(xyVals.valueRange()));
    }

    // Test insert
    {
        using namespace DFG_ROOT_NS;
        DFG_CLASS_NAME(MapVectorAoS) < int32, double > mmAos;
        DFG_CLASS_NAME(MapVectorSoA) < int32, double > mmSoa;
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
    using MapSoaIntInt = DFG_CLASS_NAME(MapVectorSoA) < int, int > ;

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
    template <class Expected_T, class Range_T>
    bool areRangesEqual(const Expected_T& expected, const Range_T& m)
    {
        return expected.size() == m.size() && std::equal(expected.begin(), expected.end(), m.begin());
    }

    template <class Cont_T>
    static void testMapVector_keyValueRanges()
    {
        Cont_T m;
        const auto& mc = m;
        m[4] = 40;
        m[5] = 50;
        m[6] = 60;
        std::array<int, 3> expectedKeys =   { 4, 5, 6 };
        std::array<double, 3> expectedValues = { 40, 50, 60 };
        EXPECT_TRUE(areRangesEqual(expectedKeys, m.keyRange()));
        EXPECT_TRUE(areRangesEqual(expectedKeys, mc.keyRange()));
        EXPECT_TRUE(areRangesEqual(expectedKeys, m.keyRange_modifiable()));

        EXPECT_TRUE(areRangesEqual(expectedValues, m.valueRange()));
        EXPECT_TRUE(areRangesEqual(expectedValues, mc.valueRange()));
        EXPECT_TRUE(areRangesEqual(expectedValues, m.valueRange_const()));

        *(m.keyRange_modifiable().begin() + 2) = 7;
        *(m.valueRange().begin()) = 30;
        expectedKeys[2] = 7;
        expectedValues[0] = 30;
        EXPECT_TRUE(areRangesEqual(expectedKeys, m.keyRange()));
        EXPECT_TRUE(areRangesEqual(expectedValues, m.valueRange()));
    }
}

TEST(dfgCont, MapVector_keyValueRanges)
{
    using namespace ::DFG_MODULE_NS(cont);
    testMapVector_keyValueRanges<MapVectorAoS<int, double>>();
    testMapVector_keyValueRanges<MapVectorSoA<int, double>>();
}

TEST(dfgCont, MapVector_makeIndexMapped)
{
    using namespace DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(cont);
    {
        const std::array<int, 3> indexes = { 0, 1, 2 };
        const std::array<double, 3> values = { 10, 20.5, 3.25 };
        const auto mvAos = MapVectorAoS<int, double>::makeIndexMapped(values, false);
        const auto mvSoa = MapVectorSoA<int, double>::makeIndexMapped(values, false);
        EXPECT_TRUE(isEqualContent(indexes, mvAos.keyRange()));
        EXPECT_TRUE(isEqualContent(values, mvAos.valueRange()));
        EXPECT_TRUE(isEqualContent(indexes, mvSoa.keyRange()));
        EXPECT_TRUE(isEqualContent(values, mvSoa.valueRange()));
    }

    // Testing handling of index overflow
    {
        std::array<int, 128> indexes;
        std::array<int, 130> values;
        ::DFG_MODULE_NS(alg)::generateAdjacent(indexes, 0, 1);
        ::DFG_MODULE_NS(alg)::generateAdjacent(values, 1000, 1);
        const auto mvAos = MapVectorAoS<int8, int>::makeIndexMapped(values, false);
        const auto mvSoa = MapVectorSoA<int8, int>::makeIndexMapped(values, false);
        const auto expectValues = makeRange(values.begin(), values.begin() + indexes.size());
        EXPECT_TRUE(isEqualContent(indexes, mvAos.keyRange()));
        EXPECT_TRUE(isEqualContent(expectValues, mvAos.valueRange()));
        EXPECT_TRUE(isEqualContent(indexes, mvSoa.keyRange()));
        EXPECT_TRUE(isEqualContent(expectValues, mvSoa.valueRange()));
    }

    // Testing move handling
    {
        // Moving from rvalue-container.
        {
            std::array<std::string, 2> values{ "abcd", "efgh" };
            auto m = MapVectorAoS<int, std::string>::makeIndexMapped(std::move(values), true);
            EXPECT_TRUE(values[0].empty());
            EXPECT_TRUE(values[1].empty());
            EXPECT_EQ(2, m.size());
            EXPECT_EQ("abcd", m[0]);
            EXPECT_EQ("efgh", m[1]);
        }

        // Checking that lvalue reference doesn't get modified with disallowed moving
        {
            std::array<std::string, 2> values{ "abcd", "efgh" };
            auto m = MapVectorAoS<int, std::string>::makeIndexMapped(values, false);
            EXPECT_EQ("abcd", values[0]);
            EXPECT_EQ("efgh", values[1]);
            EXPECT_EQ(2, m.size());
            EXPECT_EQ("abcd", m[0]);
            EXPECT_EQ("efgh", m[1]);
        }

        // Handling of range passed in as rvalue referencing non-const values.
        {
            std::array<std::string, 2> values{ "abcd", "efgh" };
            auto m = MapVectorAoS<int, std::string>::makeIndexMapped(makeRange(values), false);
            EXPECT_EQ("abcd", values[0]);
            EXPECT_EQ("efgh", values[1]);
            EXPECT_EQ(2, m.size());
            EXPECT_EQ("abcd", m[0]);
            EXPECT_EQ("efgh", m[1]);

            auto m2 = MapVectorAoS<int, std::string>::makeIndexMapped(makeRange(values), true);
            EXPECT_TRUE(values[0].empty());
            EXPECT_TRUE(values[1].empty());
            EXPECT_EQ(2, m2.size());
            EXPECT_EQ("abcd", m2[0]);
            EXPECT_EQ("efgh", m2[1]);
        }

        // Handling of range passed in as rvalue referencing const values.
        {
            const std::array<std::string, 2> values{ "abcd", "efgh" };
            auto m = MapVectorAoS<int, std::string>::makeIndexMapped(makeRange(values), true);
            EXPECT_EQ("abcd", values[0]);
            EXPECT_EQ("efgh", values[1]);
            EXPECT_EQ(2, m.size());
            EXPECT_EQ("abcd", m[0]);
            EXPECT_EQ("efgh", m[1]);
        }

        // Ability to move from const reference range referencing non-const values
        {
            std::array<std::string, 2> values{ "abcd", "efgh" };
            const auto range = makeRange(values);
            auto m = MapVectorAoS<int, std::string>::makeIndexMapped(range, true);
            EXPECT_TRUE(values[0].empty());
            EXPECT_TRUE(values[1].empty());
            EXPECT_EQ(2, m.size());
            EXPECT_EQ("abcd", m[0]);
            EXPECT_EQ("efgh", m[1]);
        }
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
    DFG_CLASS_NAME(SetVector) < std::string > setVectorSorted;
    DFG_CLASS_NAME(SetVector) < std::string > setVectorUnsorted; setVectorUnsorted.setSorting(false);

    const int randEngSeed = 12345678;
    testSetInterface(mStd, randEngSeed);
    testSetInterface(setVectorSorted, randEngSeed);
    testSetInterface(setVectorUnsorted, randEngSeed);

    verifyEqual(mStd, setVectorSorted);
    verifyEqual(mStd, setVectorUnsorted);

    verifySetVectors(setVectorSorted, setVectorUnsorted, mStd);

    // Test that SetVector works with std::inserter.
    {
        DFG_CLASS_NAME(SetVector) < std::string > setVectorSorted2;
        DFG_CLASS_NAME(SetVector) < std::string > setVectorUnsorted2; setVectorUnsorted2.setSorting(false);
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
        DFG_CLASS_NAME(SetVector) < std::string > setVectorSorted3;
        DFG_CLASS_NAME(SetVector) < std::string > setVectorUnsorted3; setVectorUnsorted3.setSorting(false);
        DFG_CLASS_NAME(SetVector) < std::string > setVectorUnsorted4; setVectorUnsorted4.setSorting(false);
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
        DFG_CLASS_NAME(SetVector) < int > si;
        si.insert(1);
        int i = 2;
        si.insert(i);
        const int ci = 3;
        si.insert(ci);
        EXPECT_EQ(3, si.size());
    }

    // Test moves
    {
        DFG_CLASS_NAME(SetVector) < std::string > se;
        std::string s(30, 'a');
        se.insert(std::move(s));
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(1, se.size());
    }
}
