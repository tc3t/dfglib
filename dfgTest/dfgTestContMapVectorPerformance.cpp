#include <stdafx.h>

#if 0 // On/off switch for the whole performance test.

#if DFG_MSVC_VER >= DFG_MSVC_VER_2012

#include <dfg/baseConstructorDelegate.hpp>
#include <dfg/cont/MapVector.hpp>
#include <dfg/rand.hpp>
#include <dfg/ReadOnlySzParam.hpp>
#include <dfg/str/format_fmt.hpp>
#include <dfg/time/timerCpu.hpp>
#include <chrono>
#include <map>
#include <unordered_map>
#include <boost/container/flat_map.hpp>

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
        
        cont.insert(std::pair<int, int>(generateKey(re), generateValue(re)));
    }

    template <class T>  std::string typeToName()                { return typeid(T).name(); }
    template <>         std::string typeToName<std::string>()   { return "std::string"; }


    template <class Key_T, class Val_T>
    std::string containerDescription(const std::map<Key_T, Val_T>&) { return "std::map<" + typeToName<Key_T>() + "," + typeToName<Val_T>() + ">"; }

    template <class Key_T, class Val_T>
    std::string containerDescription(const std::unordered_map<Key_T, Val_T>&) { return "std::unordered_map<" + typeToName<Key_T>() + "," + typeToName<Val_T>() + ">"; }

    template <class Key_T, class Val_T>
    std::string containerDescription(const boost::container::flat_map<Key_T, Val_T>&) { return "boost::flat_map<" + typeToName<Key_T>() + "," + typeToName<Val_T>() + ">"; }
    template <class Key_T, class Val_T>
    std::string containerDescription(const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorAoS)<Key_T, Val_T>& cont)
    { 
        return DFG_ROOT_NS::format_fmt("MapVectorAoS<{},{}>, sorted: {}", typeToName<Key_T>(), typeToName<Val_T>(), int(cont.isSorted()));
    }
    template <class Key_T, class Val_T>
    std::string containerDescription(const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<Key_T, Val_T>& cont)
    { 
        return DFG_ROOT_NS::format_fmt("MapVectorSoA<{},{}>, sorted: {}", typeToName<Key_T>(), typeToName<Val_T>(), int(cont.isSorted()));
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
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        randEng.seed(nRandEngSeed);
        cont.setSorting(false);
        DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
        for (int i = 0; i < nCount; ++i)
            cont.insertNonExisting(generateKey(randEng), generateValue(randEng));
        cont.setSorting(true);
        cont.m_storage.erase(std::unique(cont.m_storage.begin(), cont.m_storage.end(), [](const std::pair<Key_T, Val_T>& left, const std::pair<Key_T, Val_T>& right) {return left.first == right.first; }), cont.m_storage.end());
        const auto elapsedTime = timer.elapsedWallSeconds();
        const auto sReservationInfo = (capacity != NumericTraits<size_t>::maxValue) ? format_fmt(", reserved: {}: ", int(capacity >= cont.size())) : ": ";
        std::cout << "Insert time with MapVectorAoS push-sort-unique" << sReservationInfo  << elapsedTime << '\n';
    }

    template <class Cont_T>
    void findPerformanceTester(Cont_T& cont, const unsigned long nRandEngSeed, const int nCount)
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
    }
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
        std::map<int, int> mStd;
        std::unordered_map<int, int> mStdUnordered;
        boost::container::flat_map<int, int> mBoostFlatMap;
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

#undef CALL_PERFORMANCE_TEST_DFGLIB

        const auto randEngSeedFind = randEngSeed * 2;
        findPerformanceTester(mAoS_rs, randEngSeedFind, nFindCount);
        findPerformanceTester(mAoS_ns, randEngSeedFind, nFindCount);
        findPerformanceTester(mAoS_ru, randEngSeedFind, nFindCount);
        findPerformanceTester(mAoS_nu, randEngSeedFind, nFindCount);
        findPerformanceTester(mSoA_rs, randEngSeedFind, nFindCount);
        findPerformanceTester(mSoA_ns, randEngSeedFind, nFindCount);
        findPerformanceTester(mSoA_ru, randEngSeedFind, nFindCount);
        findPerformanceTester(mSoA_nu, randEngSeedFind, nFindCount);
        findPerformanceTester(mStd, randEngSeedFind, nFindCount);
        findPerformanceTester(mStdUnordered, randEngSeedFind, nFindCount);
        findPerformanceTester(mBoostFlatMap, randEngSeedFind, nFindCount);
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
        const size_t nCount = 1000000 / cont.size();
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

#endif // DFG_MSVC_VER >= DFG_MSVC_VER_VC2012

#endif // on/off switch
