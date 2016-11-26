#include <stdafx.h>

#include <dfg/cont/MapVector.hpp>
#include <dfg/cont/TrivialPair.hpp>
#include <dfg/cont/Vector.hpp>
#include <dfg/rand.hpp>
#include <dfg/str/format_fmt.hpp>
#include <dfg/time/timerCpu.hpp>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <boost/container/flat_map.hpp>
#include <boost/container/vector.hpp>

namespace
{

template <class T>
struct typeToName
{
    static std::string name() { return typeid(T).name(); }
};

template <> struct typeToName<std::string> { static std::string name() { return "std::string"; } };
template <class T0, class T1> struct typeToName<std::pair<T0, T1>> { static std::string name() { return "std::pair<" + typeToName<T0>::name() + ", " + typeToName<T1>::name() + ">"; } };

template <class Key_T, class Val_T>
std::string containerDescription(const std::map<Key_T, Val_T>&) { return "std::map<" + typeToName<Key_T>::name() + "," + typeToName<Val_T>::name() + ">"; }

template <class Key_T, class Val_T>
std::string containerDescription(const std::unordered_map<Key_T, Val_T>&) { return "std::unordered_map<" + typeToName<Key_T>::name() + "," + typeToName<Val_T>::name() + ">"; }

template <class Key_T, class Val_T>
std::string containerDescription(const boost::container::flat_map<Key_T, Val_T>&) { return "boost::flat_map<" + typeToName<Key_T>::name() + "," + typeToName<Val_T>::name() + ">"; }
template <class Key_T, class Val_T>
std::string containerDescription(const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorAoS)<Key_T, Val_T>& cont)
{
    return DFG_ROOT_NS::format_fmt("MapVectorAoS<{},{}>, sorted: {}", typeToName<Key_T>::name(), typeToName<Val_T>::name(), int(cont.isSorted()));
}
template <class Key_T, class Val_T>
std::string containerDescription(const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<Key_T, Val_T>& cont)
{
    return DFG_ROOT_NS::format_fmt("MapVectorSoA<{},{}>, sorted: {}", typeToName<Key_T>::name(), typeToName<Val_T>::name(), int(cont.isSorted()));
}

template <class Val_T>
std::string containerDescription(const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Vector)<Val_T>&)
{
    return DFG_ROOT_NS::format_fmt("Vector<{}>", typeToName<Val_T>::name());
}

template <class Val_T> std::string containerDescription(const std::vector<Val_T>&) { return "std::vector<" + typeToName<Val_T>::name() + ">"; }
template <class Val_T> std::string containerDescription(const boost::container::vector<Val_T>&) { return "boost::vector<" + typeToName<Val_T>::name() + ">"; }


template <class Cont_T, class Generator_T>
Cont_T VectorInsertImpl(Generator_T generator, const int nCount)
{
    using namespace DFG_ROOT_NS;
    const int nRandEngSeed = 12345678;

    auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
    auto distr = DFG_MODULE_NS(rand)::makeDistributionEngineUniform(&randEng, 0, NumericTraits<int>::maxValue);
    randEng.seed(static_cast<unsigned long>(nRandEngSeed));
    Cont_T cont;
    DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
    cont.reserve(nCount);
    cont.push_back(generator(randEng()));
    for (int i = 1; i < nCount; ++i)
    {
        const auto nPos = distr() % cont.size();
        cont.insert(cont.begin() + nPos, generator(randEng()));
    }
    const auto elapsedTime = timer.elapsedWallSeconds();
    //const auto sReservationInfo = (capacity != NumericTraits<size_t>::maxValue) ? format_fmt(", reserved: {}", int(capacity >= cont.size())) : "";
    if (nCount > 100)
        std::cout << "Insert time " << containerDescription(cont) /*<< sReservationInfo*/ << ": " << elapsedTime << '\n';
    return cont;
}

template <class T> T generate(size_t randVal);

template <> int generate<int>(size_t randVal) { return static_cast<int>(randVal); }
template <> double generate<double>(size_t randVal) { return static_cast<double>(randVal); }
template <> std::pair<int, int> generate<std::pair<int, int>>(size_t randVal)
{
    auto val = generate<int>(randVal);
    return std::pair<int, int>(val, val);
}
template <> DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TrivialPair)<int, int> generate<DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TrivialPair)<int, int>>(size_t randVal)
{
    auto val = generate<int>(randVal);
    return DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TrivialPair)<int, int>(val, val);
}

template <class T>
void VectorInsertImpl(const int nCount)
{
    const auto stdVec = VectorInsertImpl<std::vector<T>>(generate<T>, nCount);
    const auto boostVec = VectorInsertImpl<boost::container::vector<T>>(generate<T>, nCount);
    const auto dfgVec = VectorInsertImpl<DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Vector)<T>>(generate<T>, nCount);
    ASSERT_EQ(nCount, stdVec.size());
    ASSERT_EQ(stdVec.size(), boostVec.size());
    ASSERT_EQ(stdVec.size(), dfgVec.size());

    EXPECT_TRUE(std::equal(stdVec.begin(), stdVec.end(), boostVec.begin()));
    EXPECT_TRUE(std::equal(stdVec.begin(), stdVec.end(), dfgVec.begin()));
}

} // unnamed namespace

#if 0 // On/off switch for the whole performance test.

namespace
{
    int generateKey(decltype(DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded())& re)
    {
        return DFG_MODULE_NS(rand)::rand<int>(re, -10000000, 10000000);
    }

    int generateValue(decltype(DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded())& re)
    {
        return DFG_MODULE_NS(rand)::rand<int>(re, -10000000, 10000000);
    }

    template <class Cont_T>
    void insertImpl(Cont_T& cont, decltype(DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded())& re)
    {
        auto key = generateKey(re);
        cont.insert(std::pair<int, int>(key, key));
    }

    template <class Cont_T>
    void insertForVectorPerformanceTester(Cont_T& cont, const unsigned long nRandEngSeed, const int nCount)
    {
        using namespace DFG_ROOT_NS;
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        auto distr = DFG_MODULE_NS(rand)::makeDistributionEngineUniform(&randEng, 0, NumericTraits<int>::maxValue);
        const auto nInitialCapacity = cont.capacity();
        randEng.seed(nRandEngSeed);
        DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
        for (int i = 0; i < nCount; ++i)
        {
            const auto key = generateKey(randEng);
            cont.push_back(key);
            cont.push_back(key);
        }
        const auto elapsedTime = timer.elapsedWallSeconds();
        const auto sReservationInfo = format_fmt("), reserved: {}", int(nInitialCapacity >= cont.size()));
        std::cout << "Insert time with interleaved vector (" << containerDescription(cont) << sReservationInfo << ": " << elapsedTime << '\n';
    }

    template <class Cont_T>
    void insertPerformanceTester(Cont_T& cont, const unsigned long nRandEngSeed, const int nCount, const size_t capacity = DFG_ROOT_NS::NumericTraits<size_t>::maxValue)
    {
        using namespace DFG_ROOT_NS;
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        randEng.seed(nRandEngSeed);
        DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
        for (int i = 0; i < nCount; ++i)
            insertImpl(cont, randEng);
        const auto elapsedTime = timer.elapsedWallSeconds();
        const auto sReservationInfo = (capacity != NumericTraits<size_t>::maxValue) ? format_fmt(", reserved: {}", int(capacity >= cont.size())) : "";
        std::cout << "Insert time " << containerDescription(cont) << sReservationInfo << ": " << elapsedTime << '\n';
    }

    template <class Key_T, class Val_T>
    void insertPerformanceTesterUnsortedPush_sort_and_unique(DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorAoS)<Key_T, Val_T>& cont, const unsigned long nRandEngSeed, const int nCount, const size_t capacity = DFG_ROOT_NS::NumericTraits<size_t>::maxValue)
    {
        using namespace DFG_ROOT_NS;
        typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorAoS)<Key_T, Val_T>::value_type value_type;
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        randEng.seed(nRandEngSeed);
        cont.setSorting(false);
        DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
        for (int i = 0; i < nCount; ++i)
        {
            auto key = generateKey(randEng);
            cont.m_storage.push_back(value_type(key, key));
        }
        cont.setSorting(true);
        cont.m_storage.erase(std::unique(cont.m_storage.begin(), cont.m_storage.end(), [](const value_type& left, const value_type& right) {return left.first == right.first; }), cont.m_storage.end());
        const auto elapsedTime = timer.elapsedWallSeconds();
        const auto sReservationInfo = (capacity != NumericTraits<size_t>::maxValue) ? format_fmt(", reserved: {}: ", int(capacity >= cont.size())) : ": ";
        std::cout << "Insert time with MapVectorAoS push-sort-unique" << sReservationInfo  << elapsedTime << '\n';
    }

    template <class Cont_T>
    size_t findPerformanceTester(Cont_T& cont, const unsigned long nRandEngSeed, const int nCount)
    {
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        randEng.seed(nRandEngSeed);
        DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
        size_t nFound = 0;
        for (int i = 0; i < nCount; ++i)
        {
            auto key = DFG_MODULE_NS(rand)::rand<int>(randEng, -10000000, 10000000);
            nFound += (cont.find(key) != cont.end());
        }
        const auto elapsed = timer.elapsedWallSeconds();
        std::cout << "Find time with " << containerDescription(cont) << ": " << elapsed << '\n';
        std::cout << "Container size: " << cont.size() << '\n';
        std::cout << "Found item count: " << nFound << '\n';
        return nFound;
    }
}

namespace
{
    template <class Key_T, class Val_T>
    struct ValueTypeCompareFunctor
    {
        bool operator()(const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TrivialPair)<Key_T, Val_T>& left, const std::pair<Key_T, Val_T>& right) const
        {
            return left.first == right.first && left.second == right.second;
        }

        bool operator()(const std::pair<Key_T, Val_T>& left, const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TrivialPair)<Key_T, Val_T>& right) const
        {
            return right == left;
        }

        bool operator()(const std::pair<Key_T, Val_T>& left, const std::pair<Key_T, Val_T>& right) const
        {
            return left == right;
        }

        bool operator()(const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TrivialPair)<Key_T, Val_T>& left, const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TrivialPair)<Key_T, Val_T>& right) const
        {
            return left == right;
        }
    };
}

TEST(dfgCont, MapVectorPerformance)
{
    using namespace DFG_MODULE_NS(cont);
    const int randEngSeed = 12345678;

#ifdef _DEBUG
    const auto nCount = 1000;
#else
    const auto nCount = 50000;
#endif
    const auto nFindCount = 5 * nCount;

    {
        std::vector<int> stdVecInterleaved; stdVecInterleaved.reserve(2 * nCount);
        boost::container::vector<int> boostVecInterleaved; boostVecInterleaved.reserve(2 * nCount);
        std::map<int, int> mStd;
        std::unordered_map<int, int> mStdUnordered;
        boost::container::flat_map<int, int> mBoostFlatMap; mBoostFlatMap.reserve(nCount);
        DFG_CLASS_NAME(MapVectorAoS)<int, int> mAoS_rs; mAoS_rs.reserve(nCount);
        DFG_CLASS_NAME(MapVectorAoS)<int, int> mAoS_ns;
        DFG_CLASS_NAME(MapVectorAoS)<int, int> mAoS_ru; mAoS_ru.reserve(nCount); mAoS_ru.setSorting(false);
        DFG_CLASS_NAME(MapVectorAoS)<int, int> mAoS_nu; mAoS_nu.setSorting(false);
        DFG_CLASS_NAME(MapVectorSoA)<int, int> mSoA_rs; mSoA_rs.reserve(nCount);
        DFG_CLASS_NAME(MapVectorSoA)<int, int> mSoA_ns;
        DFG_CLASS_NAME(MapVectorSoA)<int, int> mSoA_ru; mSoA_ru.reserve(nCount); mSoA_ru.setSorting(false);
        DFG_CLASS_NAME(MapVectorSoA)<int, int> mSoA_nu; mSoA_nu.setSorting(false);

        DFG_CLASS_NAME(MapVectorAoS)<int, int> mUniqueAoSInsert; mUniqueAoSInsert.reserve(nCount); mUniqueAoSInsert.setSorting(false);
        DFG_CLASS_NAME(MapVectorAoS)<int, int> mUniqueAoSInsertNotReserved; mUniqueAoSInsertNotReserved.setSorting(false);

#define CALL_PERFORMANCE_TEST_DFGLIB(name) insertPerformanceTester(name, randEngSeed, nCount, name.capacity());

        CALL_PERFORMANCE_TEST_DFGLIB(mAoS_rs);
        CALL_PERFORMANCE_TEST_DFGLIB(mAoS_ns);
        CALL_PERFORMANCE_TEST_DFGLIB(mAoS_ru);
        CALL_PERFORMANCE_TEST_DFGLIB(mAoS_nu);
        CALL_PERFORMANCE_TEST_DFGLIB(mSoA_rs);
        CALL_PERFORMANCE_TEST_DFGLIB(mSoA_ns);
        CALL_PERFORMANCE_TEST_DFGLIB(mSoA_ru);
        CALL_PERFORMANCE_TEST_DFGLIB(mSoA_nu);
        insertPerformanceTester(mStd, randEngSeed, nCount);
        insertPerformanceTester(mStdUnordered, randEngSeed, nCount);
        insertPerformanceTester(mBoostFlatMap, randEngSeed, nCount);
        insertPerformanceTesterUnsortedPush_sort_and_unique(mUniqueAoSInsert, randEngSeed, nCount, mUniqueAoSInsert.capacity());
        insertPerformanceTesterUnsortedPush_sort_and_unique(mUniqueAoSInsertNotReserved, randEngSeed, nCount, mUniqueAoSInsertNotReserved.capacity());
        insertForVectorPerformanceTester(stdVecInterleaved, randEngSeed, nCount);
        insertForVectorPerformanceTester(boostVecInterleaved, randEngSeed, nCount);

        EXPECT_EQ(mAoS_rs.size(), mAoS_ns.size());
        EXPECT_EQ(mAoS_rs.size(), mAoS_ru.size());
        EXPECT_EQ(mAoS_rs.size(), mAoS_nu.size());
        EXPECT_EQ(mAoS_rs.size(), mSoA_rs.size());
        EXPECT_EQ(mAoS_rs.size(), mSoA_ns.size());
        EXPECT_EQ(mAoS_rs.size(), mSoA_ru.size());
        EXPECT_EQ(mAoS_rs.size(), mSoA_nu.size());
        EXPECT_EQ(mAoS_rs.size(), mStd.size());
        EXPECT_EQ(mAoS_rs.size(), mStdUnordered.size());
        EXPECT_EQ(mAoS_rs.size(), mBoostFlatMap.size());
        EXPECT_EQ(mAoS_rs.size(), mUniqueAoSInsert.size());
        EXPECT_EQ(mAoS_rs.size(), mUniqueAoSInsertNotReserved.size());

        EXPECT_TRUE(std::equal(mAoS_ns.begin(), mAoS_ns.end(), mStd.begin(), ValueTypeCompareFunctor<int, int>()));
        EXPECT_TRUE(std::equal(mAoS_ns.begin(), mAoS_ns.end(), mBoostFlatMap.begin(), ValueTypeCompareFunctor<int, int>()));
        EXPECT_TRUE(std::equal(mAoS_ns.begin(), mAoS_ns.end(), mAoS_rs.begin(), ValueTypeCompareFunctor<int, int>()));
        EXPECT_TRUE(std::equal(mAoS_ns.begin(), mAoS_ns.end(), mUniqueAoSInsert.begin(), ValueTypeCompareFunctor<int, int>())); // This requires sort() to be result-wise identical to stable_sort() for the generated data.
        EXPECT_TRUE(std::equal(mAoS_ns.begin(), mAoS_ns.end(), mUniqueAoSInsertNotReserved.begin(), ValueTypeCompareFunctor<int, int>())); // This requires sort() to be result-wise identical to stable_sort() for the generated data.
        EXPECT_TRUE(std::equal(stdVecInterleaved.begin(), stdVecInterleaved.end(), boostVecInterleaved.begin()));
        

#undef CALL_PERFORMANCE_TEST_DFGLIB

        {
            const auto randEngSeedFind = randEngSeed * 2;
            const auto findings = findPerformanceTester(mAoS_rs, randEngSeedFind, nFindCount);
            EXPECT_EQ(findings, findPerformanceTester(mAoS_ns, randEngSeedFind, nFindCount));
            EXPECT_EQ(findings, findPerformanceTester(mAoS_ru, randEngSeedFind, nFindCount));
            EXPECT_EQ(findings, findPerformanceTester(mAoS_nu, randEngSeedFind, nFindCount));
            EXPECT_EQ(findings, findPerformanceTester(mSoA_rs, randEngSeedFind, nFindCount));
            EXPECT_EQ(findings, findPerformanceTester(mSoA_ns, randEngSeedFind, nFindCount));
            EXPECT_EQ(findings, findPerformanceTester(mSoA_ru, randEngSeedFind, nFindCount));
            EXPECT_EQ(findings, findPerformanceTester(mSoA_nu, randEngSeedFind, nFindCount));
            EXPECT_EQ(findings, findPerformanceTester(mStd, randEngSeedFind, nFindCount));
            EXPECT_EQ(findings, findPerformanceTester(mStdUnordered, randEngSeedFind, nFindCount));
            EXPECT_EQ(findings, findPerformanceTester(mBoostFlatMap, randEngSeedFind, nFindCount));
        }
    }
}

namespace
{
    class LookUpString : public DFG_ROOT_NS::DFG_CLASS_NAME(StringViewC)
    {
    public:
        DFG_BASE_CONSTRUCTOR_DELEGATE_1(LookUpString, DFG_ROOT_NS::DFG_CLASS_NAME(StringViewC)) {}
        operator std::string() const { return std::string(begin(), end()); }
    };

    template <class Cont_T>
    void MapPerformanceComparisonWithStdStringKeyAndConstCharLookUpImpl(Cont_T& cont, const char* pszTitle)
    {
        using namespace DFG_MODULE_NS(rand);
        auto randEng = createDefaultRandEngineUnseeded();
        randEng.seed(125498);

        const size_t nMapSize = 2;
        //const size_t nMapSize = 1000;

        for (size_t i = 0; i < nMapSize; ++i)
        {
            cont.insert(std::pair<std::string, int>("c:/an/example/path/file" + std::to_string(uint64_t(i)) + ".txt", rand(randEng, 0, 100000))); // String comparison is relatively expensive as keys begin with the same sequence.
            //cont.insert(std::pair<std::string, int>(std::to_string(i) + "c:/an/example/path/file" + ".txt", rand(randEng, 0, 100000))); // String comparison is cheap since it can be resolved after a few chars.
        }
        const char* arrLookupStrings[] = 
        //const std::string arrLookupStrings[] =
        //const LookUpString arrLookupStrings[] =
        {
            "c:/an/example/path/file5.txt",
            "c:/an/example/path/file55",
            "d:/temp",
            "C:/temp",
            "c:/an/example/path/file1.txt",
            "c:/an/example/path/file.txt"
        };

        auto loopUpIndexRandomizer = makeDistributionEngineUniform(&randEng, 0, int(DFG_COUNTOF(arrLookupStrings) - 1));

#ifdef _DEBUG
        const size_t nCount = 100 / cont.size();
#else
        const size_t nCount = static_cast<size_t>((cont.size() <= 20) ? 100000000LL / cont.size() : 10000000000LL / cont.size()); 
#endif

        const auto endIter = cont.end();
        size_t nSum = 0;
        DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
        for (size_t i = 0; i < nCount; ++i)
        {
            const auto index = loopUpIndexRandomizer();
            auto iter = cont.find(arrLookupStrings[index]);
            if (iter != endIter)
                nSum += iter->second;
        }
        std::cout << "Find time with " << pszTitle << ": " << timer.elapsedWallSeconds() << '\n';
        std::cout << "Map size: " << cont.size() << ", Rounds: " << nCount << ", Sum: " << nSum << '\n';
    }
}

TEST(dfgCont, MapPerformanceComparisonWithStdStringKeyAndConstCharLookUp)
{
    using namespace DFG_MODULE_NS(cont);

    std::map<std::string, int> stdMap;
    std::map<std::string, int> stdUnorderedMap;
    boost::container::flat_map<std::string, int> boostFlatMap;
    DFG_CLASS_NAME(MapVectorAoS)<std::string, int> mAoS_sorted; 
    DFG_CLASS_NAME(MapVectorAoS)<std::string, int> mAoS_unsorted; mAoS_unsorted.setSorting(false);
    DFG_CLASS_NAME(MapVectorSoA)<std::string, int> mSoA_sorted;
    DFG_CLASS_NAME(MapVectorSoA)<std::string, int> mSoA_unsorted; mSoA_unsorted.setSorting(false);

    MapPerformanceComparisonWithStdStringKeyAndConstCharLookUpImpl(mAoS_sorted,     "MapVectorAoS_sorted  ");
    MapPerformanceComparisonWithStdStringKeyAndConstCharLookUpImpl(mAoS_unsorted,   "MapVectorAoS_unsorted");
    MapPerformanceComparisonWithStdStringKeyAndConstCharLookUpImpl(mSoA_sorted,     "MapVectorSoA_sorted  ");
    MapPerformanceComparisonWithStdStringKeyAndConstCharLookUpImpl(mSoA_unsorted,   "MapVectorSoA_unsorted");
    MapPerformanceComparisonWithStdStringKeyAndConstCharLookUpImpl(stdMap,          "std::map             ");
    MapPerformanceComparisonWithStdStringKeyAndConstCharLookUpImpl(stdUnorderedMap, "std::unordered_map   ");
    MapPerformanceComparisonWithStdStringKeyAndConstCharLookUpImpl(boostFlatMap,    "boost::flat_map      ");
}

TEST(dfgCont, VectorInsertPerformance)
{
#ifdef _DEBUG
    const int nCount = 100;
#else
    const int nCount = 100000;
#endif

    VectorInsertImpl<int>(nCount);
    VectorInsertImpl<double>(nCount);
    VectorInsertImpl<std::pair<int,int>>(nCount);
    VectorInsertImpl<DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TrivialPair)<int,int>>(nCount);
}

#endif // on/off switch for performance tests.

// This is not a performance test but placed in this file for now to get the same infrastructure as the performance test.
TEST(dfgCont, VectorInsert)
{
    const int nCount = 10;
    VectorInsertImpl<int>(nCount);
    VectorInsertImpl<double>(nCount);
    VectorInsertImpl<std::pair<int, int>>(nCount);
    VectorInsertImpl<DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TrivialPair)<int, int>>(nCount);
}
