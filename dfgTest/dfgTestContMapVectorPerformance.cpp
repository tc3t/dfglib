#include "stdafx.h"

#if (DFGTEST_BUILD_MODULE_DEFAULT == 1)

// Note: this file does not use DFG_CLASS_NAME-macros on purpose.

#include <dfg/typeTraits.hpp>
#include <dfg/cont/TrivialPair.hpp>

// This could be used to mark TrivialPair<int,int> as trivially copyable for compilers that do not support the type trait. 
// For now not marked to avoid giving impression that this optimization was available by default on those compilers.
#if 0 // !DFG_LANGFEAT_HAS_IS_TRIVIALLY_COPYABLE
DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(TypeTraits)
{
    template <> struct IsTriviallyCopyable<DFG_MODULE_NS(cont)::TrivialPair<int,int>> : public std::true_type { };
} }
#endif

#include <dfg/build/compilerDetails.hpp>
#include <dfg/build/languageFeatureInfo.hpp>
#include <dfg/typeTraits.hpp>
#include <dfg/cont/tableCsv.hpp>
#include <dfg/cont/valueArray.hpp>
#include <dfg/str/strTo.hpp>

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

#include <dfg/time.hpp>
#include <dfg/time/DateTime.hpp>

#define DFG_TEST_COMPILE_MAP_BENCHMARKS 0

#if DFG_TEST_COMPILE_MAP_BENCHMARKS
    #include <flat_pubby/flat_map.hpp>
    #include <flat_sm/flat_map.h>
#endif

namespace
{

template <class T>
struct typeToName
{
    static std::string name() { return typeid(T).name(); }
};

template <> struct typeToName<int> { static std::string name() { return "int"; } };
template <> struct typeToName<double> { static std::string name() { return "double"; } };
template <> struct typeToName<std::string> { static std::string name() { return "std::string"; } };
template <class T0, class T1> struct typeToName<std::pair<T0, T1>> { static std::string name() { return "std::pair<" + typeToName<T0>::name() + ", " + typeToName<T1>::name() + ">"; } };
template <class T0, class T1> struct typeToName<DFG_MODULE_NS(cont)::TrivialPair<T0, T1>> { static std::string name() { return "TrivialPair<" + typeToName<T0>::name() + ", " + typeToName<T1>::name() + ">"; } };

template <class Key_T, class Val_T>
std::string containerDescription(const std::map<Key_T, Val_T>&) { return "std::map<" + typeToName<Key_T>::name() + "," + typeToName<Val_T>::name() + ">"; }

template <class Key_T, class Val_T>
std::string containerDescription(const std::unordered_map<Key_T, Val_T>&) { return "std::unordered_map<" + typeToName<Key_T>::name() + "," + typeToName<Val_T>::name() + ">"; }

template <class Key_T, class Val_T>
std::string containerDescription(const boost::container::flat_map<Key_T, Val_T>&) { return "boost::flat_map<" + typeToName<Key_T>::name() + "," + typeToName<Val_T>::name() + ">"; }
template <class Key_T, class Val_T>
std::string containerDescription(const DFG_MODULE_NS(cont)::MapVectorAoS<Key_T, Val_T>& cont)
{
    return DFG_ROOT_NS::format_fmt("MapVectorAoS<{},{}>, sorted: {}", typeToName<Key_T>::name(), typeToName<Val_T>::name(), int(cont.isSorted()));
}
template <class Key_T, class Val_T>
std::string containerDescription(const DFG_MODULE_NS(cont)::MapVectorSoA<Key_T, Val_T>& cont)
{
    return DFG_ROOT_NS::format_fmt("MapVectorSoA<{},{}>, sorted: {}", typeToName<Key_T>::name(), typeToName<Val_T>::name(), int(cont.isSorted()));
}

template <class Val_T>
std::string containerDescription(const DFG_MODULE_NS(cont)::Vector<Val_T>&)
{
    return DFG_ROOT_NS::format_fmt("Vector<{}>", typeToName<Val_T>::name());
}

template <class Val_T> std::string containerDescription(const std::vector<Val_T>&)              { return "std::vector<" + typeToName<Val_T>::name() + ">"; }
template <class Val_T> std::string containerDescription(const boost::container::vector<Val_T>&) { return "boost::vector<" + typeToName<Val_T>::name() + ">"; }

#if DFG_TEST_COMPILE_MAP_BENCHMARKS
    template <class T0, class T1>
    std::string containerDescription(const fc::vector_map<T0, T1>&)
    {
        return DFG_ROOT_NS::format_fmt("pubby_flat_map::vector_map<{}, {}>", typeToName<T0>::name(), typeToName<T1>::name());
    }

    template <class T0, class T1>
    std::string containerDescription(const fc::flat_map<dfg::cont::Vector<dfg::cont::TrivialPair<T0, T1>>>&)
    {
        return DFG_ROOT_NS::format_fmt("pubby_flat_map<Vector<TrivialPair<{}, {}>", typeToName<T0>::name(), typeToName<T1>::name());
    }
#endif // DFG_TEST_COMPILE_MAP_BENCHMARKS

namespace
{
    std::string generateCompilerInfoForOutputFilename()
    {
        return dfg::format_fmt("{}_{}_{}", DFG_COMPILER_NAME_SIMPLE, 8 * sizeof(void*), DFG_BUILD_DEBUG_RELEASE_TYPE);
    }

    class BenchmarkResultTable : public DFG_MODULE_NS(cont)::TableCsv<char, DFG_ROOT_NS::uint32>
    {
        typedef DFG_ROOT_NS::uint32 uint32;

    public:
        void addReducedValuesAndWriteToFile(const uint32 nFirstResultColumn, const dfg::StringViewSzAscii& svBaseName)
        {
            // Calculate averages etc.
            addReducedValues(nFirstResultColumn);

            DFG_MODULE_NS(io)::OfStream ostrm(dfg::format_fmt("testfiles/generated/{}_{}.csv", svBaseName.c_str().c_str(), generateCompilerInfoForOutputFilename()));
            writeToStream(ostrm);
        }

        void addReducedValues(const uint32 firstResultCol)
        {
            using namespace DFG_ROOT_NS;
            using namespace DFG_MODULE_NS(str);

            const auto nRowCount = this->rowCountByMaxRowIndex();
            const auto nColCount = this->colCountByMaxColIndex();

            this->addString(DFG_ASCII("avg"),       0, nColCount);
            this->addString(DFG_ASCII("median"),    0, nColCount + 1);
            this->addString(DFG_ASCII("sum"),       0, nColCount + 2);
            
            DFG_MODULE_NS(cont)::ValueVector<double> vals;
            for (uint32 r = 1; r < nRowCount; ++r)
            {
                vals.clear();
                for (uint32 c = firstResultCol; c < nColCount; ++c)
                {
                    auto p = (*this)(r, c);
                    if (!p)
                        continue;
                    vals.push_back(DFG_MODULE_NS(str)::strTo<double>(p.rawPtr()));
                }

                const auto avg = vals.average();
                const auto median = vals.median();
                const auto sum = vals.sum();

                char sz[32];
                this->addString(SzPtrUtf8(toStr(avg, sz, 6)), r, nColCount);
                this->addString(SzPtrUtf8(toStr(median, sz, 6)), r, nColCount + 1);
                this->addString(SzPtrUtf8(toStr(sum, sz, 6)), r, nColCount + 2);
            }
        }
    };

    template <class T>
    std::string GenerateOutputFilePathForVectorInsert(const dfg::StringViewSzC& s)
    {
        std::string sTypeSuffix = typeToName<T>::name();
        for (size_t j = 0; j < sTypeSuffix.size(); ++j)
        {
            auto ch = sTypeSuffix[j];
            if (ch == '<' || ch == '>' || ch == ',' || ch == ':')
                sTypeSuffix[j] = '_';
        }
        return dfg::format_fmt("testfiles/generated/{}{}_{}.txt", s.c_str(), sTypeSuffix, generateCompilerInfoForOutputFilename());
    }

} // unnamed namespace

template <class Cont_T, class Generator_T, class InsertPosGenerator_T>
Cont_T VectorInsertImpl(Generator_T generator, InsertPosGenerator_T indexGenerator, const int nCount, BenchmarkResultTable* pTable, const int nRow)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);

    Cont_T cont;
    DFG_MODULE_NS(time)::TimerCpu timer;
    cont.reserve(nCount);
    cont.push_back(generator(1));
    for (int i = 1; i < nCount; ++i)
    {
        const auto nPos = indexGenerator(cont.size());
        cont.insert(cont.begin() + nPos, generator(nPos));
    }
    const auto elapsedTime = timer.elapsedWallSeconds();
    //const auto sReservationInfo = (capacity != NumericTraits<size_t>::maxValue) ? format_fmt(", reserved: {}", int(capacity >= cont.size())) : "";
    if (pTable)
        pTable->addString(floatingPointToStr<StringUtf8>(elapsedTime, 4 /*number of significant digits*/), nRow, pTable->colCountByMaxColIndex() - 1);

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

template <> DFG_MODULE_NS(cont)::TrivialPair<int, int> generate<DFG_MODULE_NS(cont)::TrivialPair<int, int>>(size_t randVal)
{
    auto val = generate<int>(randVal);
    return DFG_MODULE_NS(cont)::TrivialPair<int, int>(val, val);
}

template <class Pair_T>
std::ostream& pairLikeItemStreaming(std::ostream& ostrm, const Pair_T& a)
{
    ostrm << a.first << "," << a.second;
    return ostrm;
}

template <class T0, class T1>
std::ostream& operator<<(std::ostream& ostrm, const std::pair<T0, T1>& a)
{
    return pairLikeItemStreaming(ostrm, a);
}

template <class T0, class T1>
std::ostream& operator<<(std::ostream& ostrm, const dfg::cont::TrivialPair<T0, T1>& a)
{
    return pairLikeItemStreaming(ostrm, a);
}

template <class T>
void VectorInsertImpl(const int nCount, BenchmarkResultTable* pTable = nullptr, const int nRow = 0, const int nTypeCol = 0)
{
    using namespace DFG_ROOT_NS;
    if (pTable)
    {
        pTable->addString(SzPtrUtf8(containerDescription(std::vector<T>()).c_str()), nRow, nTypeCol);
        pTable->addString(SzPtrUtf8(containerDescription(boost::container::vector<T>()).c_str()), nRow + 1, nTypeCol);
        pTable->addString(SzPtrUtf8(containerDescription(DFG_MODULE_NS(cont)::Vector<T>()).c_str()), nRow + 2, nTypeCol);
    }

#if 0 // If true, using file-based insert positions.
    std::vector<int> contInsertIndexes;
    contInsertIndexes.reserve(50000);
    std::ifstream istrm("testfiles/vectorInsertIndexes_50000.txt");
    {
        int i;
        while (istrm >> i)
        {
            contInsertIndexes.push_back(i);
        }
    }

    const auto indexGenerator = [&](const size_t nContSize)
                                {
                                    return contInsertIndexes[(nContSize - 1) % contInsertIndexes.size()];
                                };
#else // Case: using random generator based insert positions.
    const unsigned long nRandEngSeed = 12345678;
    auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
    auto distr = DFG_MODULE_NS(rand)::makeDistributionEngineUniform(&randEng, 0, NumericTraits<int>::maxValue);
    const auto indexGenerator = [&](const size_t nContSize) -> ptrdiff_t
                                {
                                    if (nContSize == 1)
                                        randEng.seed(nRandEngSeed);
                                    return distr() % nContSize;
                                };
#endif

    const auto stdVec = VectorInsertImpl<std::vector<T>>(generate<T>, indexGenerator, nCount, pTable, nRow);
    const auto boostVec = VectorInsertImpl<boost::container::vector<T>>(generate<T>, indexGenerator, nCount, pTable, nRow + 1);
    const auto dfgVec = VectorInsertImpl<DFG_MODULE_NS(cont)::Vector<T>>(generate<T>, indexGenerator, nCount, pTable, nRow + 2);
    ASSERT_EQ(nCount, stdVec.size());
    ASSERT_EQ(stdVec.size(), boostVec.size());
    ASSERT_EQ(stdVec.size(), dfgVec.size());

    EXPECT_TRUE(std::equal(stdVec.begin(), stdVec.end(), boostVec.begin()));
    EXPECT_TRUE(std::equal(stdVec.begin(), stdVec.end(), dfgVec.begin()));
    
    if (pTable)
    {
        dfg::io::OfStream ostrm(GenerateOutputFilePathForVectorInsert<T>("generatedVectorInsertValues"));
        for (size_t i = 0; i < stdVec.size(); ++i)
            ostrm << stdVec[i] << '\n';
    }
}

} // unnamed namespace

#if DFG_TEST_COMPILE_MAP_BENCHMARKS // On/off switch for the whole performance test.

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

    void insertImpl(fc::flat_map<dfg::cont::Vector<dfg::cont::TrivialPair<int, int>>>& cont, decltype(DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded())& re)
    {
        auto key = generateKey(re);
        cont.insert(dfg::cont::TrivialPair<int, int>(key, key));
    }

    template <class Cont_T>
    void AddInsertPerformanceTimeElement(BenchmarkResultTable& resultTable, const double elapsedTime, const Cont_T& cont, const std::string& sReservationInfo, const size_t nRow, const dfg::StringViewSzAscii& svDesc)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(str);
        if (resultTable(nRow, 6) == nullptr)
            resultTable.setElement(nRow, 6, SzPtrAscii(toStrT<std::string>(cont.size()).c_str()));
        else
            EXPECT_EQ(strTo<size_t>(resultTable(nRow, 6).c_str()), cont.size());
        if (resultTable(nRow, 7) == nullptr)
            resultTable.setElement(nRow, 7, SzPtrUtf8((svDesc.c_str().c_str() + containerDescription(cont) + sReservationInfo).c_str()));
        resultTable.addString(floatingPointToStr<StringUtf8>(elapsedTime, 4 /*number of significant digits*/), nRow, resultTable.colCountByMaxColIndex() - 1);
    }

    template <class Cont_T>
    void insertForVectorPerformanceTester(Cont_T& cont, const unsigned long nRandEngSeed, const int nCount, const size_t nRow, BenchmarkResultTable& resultTable)
    {
        using namespace DFG_ROOT_NS;
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        auto distr = DFG_MODULE_NS(rand)::makeDistributionEngineUniform(&randEng, 0, NumericTraits<int>::maxValue);
        const auto nInitialCapacity = cont.capacity();
        randEng.seed(nRandEngSeed);
        DFG_MODULE_NS(time)::TimerCpu timer;
        for (int i = 0; i < nCount; ++i)
        {
            const auto key = generateKey(randEng);
            cont.push_back(key);
            cont.push_back(key);
        }
        const auto elapsedTime = timer.elapsedWallSeconds();
        const auto sReservationInfo = format_fmt("), reserved: {}", int(nInitialCapacity >= cont.size()));
        std::cout << "Insert time with interleaved vector (" << containerDescription(cont) << sReservationInfo << ": " << elapsedTime << '\n';
        AddInsertPerformanceTimeElement(resultTable, elapsedTime, cont, sReservationInfo, nRow, DFG_ASCII("Interleaved "));
    }

    template <class Cont_T>
    void insertPerformanceTester(Cont_T& cont, const unsigned long nRandEngSeed, const int nCount, const size_t nRow, BenchmarkResultTable& resultTable, const size_t capacity = DFG_ROOT_NS::NumericTraits<size_t>::maxValue)
    {
        using namespace DFG_ROOT_NS;
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        randEng.seed(nRandEngSeed);
        DFG_MODULE_NS(time)::TimerCpu timer;
        for (int i = 0; i < nCount; ++i)
            insertImpl(cont, randEng);
        const auto elapsedTime = timer.elapsedWallSeconds();
        const auto sReservationInfo = (capacity != NumericTraits<size_t>::maxValue) ? format_fmt(", reserved: {}", int(capacity >= cont.size())) : "";
        std::cout << "Insert time " << containerDescription(cont) << sReservationInfo << ": " << elapsedTime << '\n';
        AddInsertPerformanceTimeElement(resultTable, elapsedTime, cont, sReservationInfo, nRow, DFG_ASCII(""));
    }

    template <class Key_T, class Val_T>
    void insertPerformanceTesterUnsortedPush_sort_and_unique(DFG_MODULE_NS(cont)::MapVectorAoS<Key_T, Val_T>& cont, const unsigned long nRandEngSeed, const int nCount, const size_t nRow, BenchmarkResultTable& resultTable, const size_t capacity = DFG_ROOT_NS::NumericTraits<size_t>::maxValue)
    {
        using namespace DFG_ROOT_NS;
        typedef typename DFG_MODULE_NS(cont)::MapVectorAoS<Key_T, Val_T>::value_type value_type;
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        randEng.seed(nRandEngSeed);
        cont.setSorting(false);
        DFG_MODULE_NS(time)::TimerCpu timer;
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
        AddInsertPerformanceTimeElement(resultTable, elapsedTime, cont, sReservationInfo, nRow, DFG_ASCII("MapVectorAoS push-sort-unique"));
    }

    template <class Cont_T>
    size_t findPerformanceTester(Cont_T& cont, const unsigned long nRandEngSeed, const int nCount, const size_t nRow, BenchmarkResultTable& resultTable)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(str);

        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        randEng.seed(nRandEngSeed);
        DFG_MODULE_NS(time)::TimerCpu timer;
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
        
        if (resultTable(nRow, 5) == nullptr)
            resultTable.setElement(nRow, 5, SzPtrAscii(toStrT<std::string>(cont.size()).c_str()));
        else
            EXPECT_EQ(strTo<size_t>(resultTable(nRow, 5).c_str()), cont.size());
        
        if (resultTable(nRow, 6) == nullptr)
            resultTable.setElement(nRow, 6, SzPtrAscii(toStrT<std::string>(nCount).c_str()));
        else
            EXPECT_EQ(strTo<size_t>(resultTable(nRow, 6).c_str()), nCount);

        if (resultTable(nRow, 7) == nullptr)
            resultTable.setElement(nRow, 7, SzPtrAscii(toStrT<std::string>(nFound).c_str()));
        else
            EXPECT_EQ(strTo<size_t>(resultTable(nRow, 7).c_str()), nFound);

        if (resultTable(nRow, 8) == nullptr)
            resultTable.setElement(nRow, 8, SzPtrUtf8((containerDescription(cont)).c_str()));
        resultTable.addString(floatingPointToStr<StringUtf8>(elapsed, 4 /*number of significant digits*/), nRow, resultTable.colCountByMaxColIndex() - 1);

        return nFound;
    }
}

namespace
{
    template <class Key_T, class Val_T>
    struct ValueTypeCompareFunctor
    {
        bool operator()(const DFG_MODULE_NS(cont)::TrivialPair<Key_T, Val_T>& left, const std::pair<Key_T, Val_T>& right) const
        {
            return left.first == right.first && left.second == right.second;
        }

        bool operator()(const std::pair<Key_T, Val_T>& left, const DFG_MODULE_NS(cont)::TrivialPair<Key_T, Val_T>& right) const
        {
            return right == left;
        }

        bool operator()(const std::pair<Key_T, Val_T>& left, const std::pair<Key_T, Val_T>& right) const
        {
            return left == right;
        }

        bool operator()(const DFG_MODULE_NS(cont)::TrivialPair<Key_T, Val_T>& left, const DFG_MODULE_NS(cont)::TrivialPair<Key_T, Val_T>& right) const
        {
            return left == right;
        }
    };
}

bool operator==(const std::pair<int, int>& a, const dfg::cont::TrivialPair<int, int>& b)
{
    return a.first == b.first && a.second == b.second;
}

TEST(dfgCont, MapVectorPerformance)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(str);
    const int randEngSeed = 12345678;

#ifdef DFG_BUILD_TYPE_DEBUG
    const auto nCount = 1000;
#else
    const auto nCount = 50000;
#endif
    const auto nFindCount = 5 * nCount;
    const auto nIterationCount = 5;

    BenchmarkResultTable table;
    table.addString(DFG_ASCII("Date"), 0, 0);
    table.addString(DFG_ASCII("Test machine"), 0, 1);
    table.addString(DFG_ASCII("Test Compiler"), 0, 2);
    table.addString(DFG_ASCII("Pointer size"), 0, 3);
    table.addString(DFG_ASCII("Build type"), 0, 4);
    table.addString(DFG_ASCII("Insert count"), 0, 5);
    table.addString(DFG_ASCII("Inserted count"), 0, 6);
    table.addString(DFG_ASCII("Test type"), 0, 7);
    const auto nLastStaticColumn = 7;

    BenchmarkResultTable tableFindBench;
    tableFindBench.addString(DFG_ASCII("Date"), 0, 0);
    tableFindBench.addString(DFG_ASCII("Test machine"), 0, 1);
    tableFindBench.addString(DFG_ASCII("Test Compiler"), 0, 2);
    tableFindBench.addString(DFG_ASCII("Pointer size"), 0, 3);
    tableFindBench.addString(DFG_ASCII("Build type"), 0, 4);
    tableFindBench.addString(DFG_ASCII("Key count"), 0, 5);
    tableFindBench.addString(DFG_ASCII("Find count"), 0, 6);
    tableFindBench.addString(DFG_ASCII("Found count"), 0, 7);
    tableFindBench.addString(DFG_ASCII("Test type"), 0, 8);
    const auto nLastStaticColumnFindBench = 8;

    for(size_t i = 0; i<nIterationCount; ++i)
    {
        if (i == 0)
        {
            const StringUtf8 sTime(SzPtrUtf8(DFG_MODULE_NS(time)::localDate_yyyy_mm_dd_C().c_str()));
            const auto sCompiler = SzPtrUtf8(DFG_COMPILER_NAME_SIMPLE);
            const StringUtf8 sPointerSize(SzPtrUtf8(DFG_MODULE_NS(str)::toStrC(sizeof(void*)).c_str()));
            const auto sBuildType = SzPtrUtf8(DFG_BUILD_DEBUG_RELEASE_TYPE);
            const StringUtf8 sInsertCount(SzPtrUtf8(DFG_MODULE_NS(str)::toStrC(nCount).c_str()));
            for (int et = 1; et <= 15; ++et)
            {
                const auto r = table.rowCountByMaxRowIndex();
                table.addString(sTime, r, 0);
                table.addString(sCompiler, r, 2);
                table.addString(sPointerSize, r, 3);
                table.addString(sBuildType, r, 4);
                table.addString(sInsertCount, r, 5);
            }

            for (int et = 1; et <= 11; ++et)
            {
                const auto r = tableFindBench.rowCountByMaxRowIndex();
                tableFindBench.addString(sTime, r, 0);
                tableFindBench.addString(sCompiler, r, 2);
                tableFindBench.addString(sPointerSize, r, 3);
                tableFindBench.addString(sBuildType, r, 4);
            }
        }

        table.addString(SzPtrUtf8(("Time#" + toStrC(i)).c_str()), 0, table.colCountByMaxColIndex());
        tableFindBench.addString(SzPtrUtf8(("Time#" + toStrC(i)).c_str()), 0, tableFindBench.colCountByMaxColIndex());

        std::vector<int> stdVecInterleaved; stdVecInterleaved.reserve(2 * nCount);
        boost::container::vector<int> boostVecInterleaved; boostVecInterleaved.reserve(2 * nCount);
        std::map<int, int> mStd;
        std::unordered_map<int, int> mStdUnordered;
        boost::container::flat_map<int, int> mBoostFlatMap; mBoostFlatMap.reserve(nCount);
        MapVectorAoS<int, int> mAoS_rs; mAoS_rs.reserve(nCount);
        MapVectorAoS<int, int> mAoS_ns;
        MapVectorAoS<int, int> mAoS_ru; mAoS_ru.reserve(nCount); mAoS_ru.setSorting(false);
        MapVectorAoS<int, int> mAoS_nu; mAoS_nu.setSorting(false);
        MapVectorSoA<int, int> mSoA_rs; mSoA_rs.reserve(nCount);
        MapVectorSoA<int, int> mSoA_ns;
        MapVectorSoA<int, int> mSoA_ru; mSoA_ru.reserve(nCount); mSoA_ru.setSorting(false);
        MapVectorSoA<int, int> mSoA_nu; mSoA_nu.setSorting(false);
        fc::vector_map<int, int> mPubbyMap;
        fc::flat_map<dfg::cont::Vector<dfg::cont::TrivialPair<int, int>>> mPubbyMapTrivialPair;

        MapVectorAoS<int, int> mUniqueAoSInsert; mUniqueAoSInsert.reserve(nCount); mUniqueAoSInsert.setSorting(false);
        MapVectorAoS<int, int> mUniqueAoSInsertNotReserved; mUniqueAoSInsertNotReserved.setSorting(false);

#define CALL_PERFORMANCE_TEST_DFGLIB(name, ROW) insertPerformanceTester(name, randEngSeed, nCount, ROW, table, name.capacity());

        CALL_PERFORMANCE_TEST_DFGLIB(mAoS_rs, 1);
        CALL_PERFORMANCE_TEST_DFGLIB(mAoS_ns, 2);
        CALL_PERFORMANCE_TEST_DFGLIB(mAoS_ru, 3);
        CALL_PERFORMANCE_TEST_DFGLIB(mAoS_nu, 4);
        CALL_PERFORMANCE_TEST_DFGLIB(mSoA_rs, 5);
        CALL_PERFORMANCE_TEST_DFGLIB(mSoA_ns, 6);
        CALL_PERFORMANCE_TEST_DFGLIB(mSoA_ru, 7);
        CALL_PERFORMANCE_TEST_DFGLIB(mSoA_nu, 8);
        insertPerformanceTester(mStd, randEngSeed, nCount, 9, table);
        insertPerformanceTester(mStdUnordered, randEngSeed, nCount, 10, table);
        insertPerformanceTester(mBoostFlatMap, randEngSeed, nCount, 11, table);
        insertPerformanceTesterUnsortedPush_sort_and_unique(mUniqueAoSInsert, randEngSeed, nCount, 12, table, mUniqueAoSInsert.capacity());
        insertPerformanceTesterUnsortedPush_sort_and_unique(mUniqueAoSInsertNotReserved, randEngSeed, nCount, 13, table, mUniqueAoSInsertNotReserved.capacity());
        insertForVectorPerformanceTester(stdVecInterleaved, randEngSeed, nCount, 14, table);
        insertForVectorPerformanceTester(boostVecInterleaved, randEngSeed, nCount, 15, table);
        insertPerformanceTester(mPubbyMap, randEngSeed, nCount, 16, table);
        insertPerformanceTester(mPubbyMapTrivialPair, randEngSeed, nCount, 17, table);

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
        EXPECT_EQ(mAoS_rs.size(), mPubbyMap.size());
        EXPECT_EQ(mAoS_rs.size(), mPubbyMapTrivialPair.size());

#define DFG_TEMP_CHECK_EQUALITY(CONT) EXPECT_TRUE(std::equal(mAoS_ns.begin(), mAoS_ns.end(), CONT.begin(), ValueTypeCompareFunctor<int, int>()));

        DFG_TEMP_CHECK_EQUALITY(mStd);
        DFG_TEMP_CHECK_EQUALITY(mBoostFlatMap);
        DFG_TEMP_CHECK_EQUALITY(mAoS_rs);
        // Check that unsorted AoS-containers are expected. TODO: implement operator==
        {
            {
                auto compareTemp = mAoS_ru;
                compareTemp.setSorting(true);
                DFG_TEMP_CHECK_EQUALITY(compareTemp);
            }
            {
                auto compareTemp2 = mAoS_nu;
                compareTemp2.setSorting(true);
                DFG_TEMP_CHECK_EQUALITY(compareTemp2);
            }
        }

        // Check that AoS and SoA are identical. TODO: implement operator==
        {
            auto iSoa = mSoA_ns.begin();
            for (auto iAos = mAoS_ns.begin(), iEndAos = mAoS_ns.end(); iAos != iEndAos; ++iAos, ++iSoa)
            {
                EXPECT_TRUE(iAos->first == iSoa->first);
                EXPECT_TRUE(iAos->second == iSoa->second);
            }
        }

        // Check that SoA-maps are identical with each other. TODO: implement operator==
        {
            EXPECT_TRUE(std::equal(mSoA_ns.beginKey(), mSoA_ns.endKey(), mSoA_rs.beginKey()));

            const auto compareUnordered = [&](const decltype(mSoA_ru)& mUnordered)
                                        {
                                            auto compareTemp = mUnordered;
                                            compareTemp.setSorting(true);
                                            auto iu = compareTemp.begin();
                                            for (auto i = mSoA_ns.begin(), iEnd = mSoA_ns.end(); i != iEnd; ++i, ++iu)
                                            {
                                                EXPECT_TRUE(i->first == iu->first);
                                                EXPECT_TRUE(i->second == iu->second);
                                            }
                                        };

            compareUnordered(mSoA_ru);
            compareUnordered(mSoA_nu);
        }
        
        DFG_TEMP_CHECK_EQUALITY(mUniqueAoSInsert); // This requires sort() to be result-wise identical to stable_sort() for the generated data.
        DFG_TEMP_CHECK_EQUALITY(mUniqueAoSInsertNotReserved); // This requires sort() to be result-wise identical to stable_sort() for the generated data.
        EXPECT_TRUE(std::equal(stdVecInterleaved.begin(), stdVecInterleaved.end(), boostVecInterleaved.begin()));

#undef DFG_TEMP_CHECK_EQUALITY

#undef CALL_PERFORMANCE_TEST_DFGLIB

        {
            const auto randEngSeedFind = randEngSeed * 2;
            const auto findings = findPerformanceTester(mAoS_rs, randEngSeedFind, nFindCount, 1, tableFindBench);
            EXPECT_EQ(findings, findPerformanceTester(mAoS_ns, randEngSeedFind, nFindCount, 2, tableFindBench));
            EXPECT_EQ(findings, findPerformanceTester(mAoS_ru, randEngSeedFind, nFindCount, 3, tableFindBench));
            EXPECT_EQ(findings, findPerformanceTester(mAoS_nu, randEngSeedFind, nFindCount, 4, tableFindBench));
            EXPECT_EQ(findings, findPerformanceTester(mSoA_rs, randEngSeedFind, nFindCount, 5, tableFindBench));
            EXPECT_EQ(findings, findPerformanceTester(mSoA_ns, randEngSeedFind, nFindCount, 6, tableFindBench));
            EXPECT_EQ(findings, findPerformanceTester(mSoA_ru, randEngSeedFind, nFindCount, 7, tableFindBench));
            EXPECT_EQ(findings, findPerformanceTester(mSoA_nu, randEngSeedFind, nFindCount, 8, tableFindBench));
            EXPECT_EQ(findings, findPerformanceTester(mStd, randEngSeedFind, nFindCount, 9, tableFindBench));
            EXPECT_EQ(findings, findPerformanceTester(mStdUnordered, randEngSeedFind, nFindCount, 10, tableFindBench));
            EXPECT_EQ(findings, findPerformanceTester(mBoostFlatMap, randEngSeedFind, nFindCount, 11, tableFindBench));
        }
    }

    table.addReducedValuesAndWriteToFile(nLastStaticColumn + 1, DFG_ASCII("benchmarkMapVectorInsertPerformance"));
    tableFindBench.addReducedValuesAndWriteToFile(nLastStaticColumnFindBench + 1, DFG_ASCII("benchmarkMapVectorFindPerformance"));
}

namespace
{
    class LookUpString : public DFG_ROOT_NS::StringViewC
    {
    public:
        DFG_BASE_CONSTRUCTOR_DELEGATE_1(LookUpString, DFG_ROOT_NS::StringViewC) {}
        operator std::string() const { return std::string(begin(), end()); }
    };

    template <class Cont_T>
    size_t MapPerformanceComparisonWithStdStringKeyAndConstCharLookUpImpl(Cont_T& cont, const char* pszTitle, const size_t nMapSize, const size_t nCount, BenchmarkResultTable* pTable, const int nRow)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(rand);
        using namespace DFG_MODULE_NS(str);
        auto randEng = createDefaultRandEngineUnseeded();
        randEng.seed(125498);

        for (size_t i = 0; i < nMapSize; ++i)
        {
            cont.insert(std::pair<std::string, int>("c:/an/example/path/file" + std::to_string(uint64_t(i)) + ".txt", DFG_MODULE_NS(rand)::rand(randEng, 0, 100000))); // With this string comparison is relatively expensive as keys begin with the same sequence.
            //cont.insert(std::pair<std::string, int>(std::to_string(i) + "c:/an/example/path/file" + ".txt", rand(randEng, 0, 100000))); // With this string comparison is cheap since it can be resolved after a few chars.
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

        if (pTable)
            pTable->addString(SzPtrAscii(toStrC(nCount).c_str()), nRow, 6);

        const auto endIter = cont.end();
        size_t nSum = 0;
        DFG_MODULE_NS(time)::TimerCpu timer;
        for (size_t i = 0; i < nCount; ++i)
        {
            const auto index = loopUpIndexRandomizer();
            auto iter = cont.find(arrLookupStrings[index]);
            if (iter != endIter)
                nSum += iter->second;
        }

        const auto elapsedTime = timer.elapsedWallSeconds();

        if (pTable)
            pTable->addString(floatingPointToStr<StringUtf8>(elapsedTime, 4 /*number of significant digits*/), nRow, pTable->colCountByMaxColIndex() - 1);

        std::cout << "Find time with " << pszTitle << ": " << elapsedTime << '\n';
        std::cout << "Map size: " << cont.size() << ", Rounds: " << nCount << ", Sum: " << nSum << '\n';
        return nSum;
    }
} // unnamed namespace

TEST(dfgCont, MapPerformanceComparisonWithStdStringKeyAndConstCharLookUp)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(str);

    std::map<std::string, int> stdMap;
    std::map<std::string, int> stdUnorderedMap;
    boost::container::flat_map<std::string, int> boostFlatMap;
    MapVectorAoS<std::string, int> mAoS_sorted; 
    MapVectorAoS<std::string, int> mAoS_unsorted; mAoS_unsorted.setSorting(false);
    MapVectorSoA<std::string, int> mSoA_sorted;
    MapVectorSoA<std::string, int> mSoA_unsorted; mSoA_unsorted.setSorting(false);
    fc::vector_map<std::string, int> pubbyVectorMap;
    std::flat_map<std::string, int> smFlatMap;

    const size_t nMapSize = 3;
    //const size_t nMapSize = 1000;
    const auto nContainerCount = 9;

#ifdef DFG_BUILD_TYPE_DEBUG
    const size_t nFindCount = Max<size_t>(10, 100 / nMapSize);
#else
    const size_t nFindCount = static_cast<size_t>((nMapSize <= 20) ? 100000000LL / nMapSize : 10000000000LL / nMapSize);
#endif

    BenchmarkResultTable table;
    table.addString(DFG_ASCII("Date"), 0, 0);
    table.addString(DFG_ASCII("Test machine"), 0, 1);
    table.addString(DFG_ASCII("Test Compiler"), 0, 2);
    table.addString(DFG_ASCII("Pointer size"), 0, 3);
    table.addString(DFG_ASCII("Build type"), 0, 4);
    table.addString(DFG_ASCII("Map size"), 0, 5);
    table.addString(DFG_ASCII("Find count"), 0, 6);
    table.addString(DFG_ASCII("Test type"), 0, 7);
    const auto nLastStaticColumn = 7;

    const auto sTime = StringUtf8::fromRawString(DFG_MODULE_NS(time)::localDate_yyyy_mm_dd_hh_mm_ss_C());

    for (size_t i = 0; i < 1; ++i) // Iterations.
    {
        if (i == 0)
        {
            const auto sCompiler = SzPtrUtf8(DFG_COMPILER_NAME_SIMPLE);
            const StringUtf8 sPointerSize(SzPtrUtf8(toStrC(sizeof(void*)).c_str()));
            const auto sBuildType = SzPtrUtf8(DFG_BUILD_DEBUG_RELEASE_TYPE);
            const StringUtf8 sMapSize(SzPtrUtf8(toStrC(nMapSize).c_str()));

            for (int ct = 0; ct < nContainerCount; ++ct)
            {
                const auto r = table.rowCountByMaxRowIndex();
                table.addString(sTime, r, 0);
                table.addString(sCompiler, r, 2);
                table.addString(sPointerSize, r, 3);
                table.addString(sBuildType, r, 4);
                table.addString(sMapSize, r, 5);
            }
        }

        table.addString(SzPtrUtf8(("Time#" + toStrC(i)).c_str()), 0, table.colCountByMaxColIndex());

        std::vector<size_t> results;
#define CALL_ELEMENTARY_TEST(CONT, TITLE, ROW) \
        table.addString(DFG_ASCII(TITLE), ROW, 7 /*=column*/); \
        results.push_back(MapPerformanceComparisonWithStdStringKeyAndConstCharLookUpImpl(CONT, TITLE, nMapSize, nFindCount, &table, ROW));

        CALL_ELEMENTARY_TEST(mAoS_sorted,       "MapVectorAoS_sorted",      1);
        CALL_ELEMENTARY_TEST(mAoS_unsorted,     "MapVectorAoS_unsorted",    2);
        CALL_ELEMENTARY_TEST(mSoA_sorted,       "MapVectorSoA_sorted",      3);
        CALL_ELEMENTARY_TEST(mSoA_unsorted,     "MapVectorSoA_unsorted",    4);
        CALL_ELEMENTARY_TEST(stdMap,            "std::map",                 5);
        CALL_ELEMENTARY_TEST(stdUnorderedMap,   "std::unordered_map",       6);
        CALL_ELEMENTARY_TEST(boostFlatMap,      "boost::flat_map",          7);
        CALL_ELEMENTARY_TEST(smFlatMap,         "smFlatMap",                8);
        CALL_ELEMENTARY_TEST(pubbyVectorMap,    "pubbyVectorMap",           9);
#undef CALL_ELEMENTARY_TEST

        EXPECT_EQ(nContainerCount, results.size());
        const auto nFirstResult = results.front();
        EXPECT_TRUE(std::all_of(results.begin(), results.end(), [=](const size_t a) { return nFirstResult == a; }));
    }

    const StringAscii sFilenameBase(SzPtrAscii(format_fmt("benchmarkStringMapWithCharPtrLookup_N{}_M{}", nMapSize, nFindCount).c_str()));
    table.addReducedValuesAndWriteToFile(nLastStaticColumn + 1, sFilenameBase);
}

#include <dfg/io/ofstream.hpp>
#include <dfg/str/string.hpp>
#include <dfg/str.hpp>

TEST(dfgCont, VectorInsertPerformance)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(str);

#ifdef DFG_BUILD_TYPE_DEBUG
    const int nCount = 100;
#else
    const int nCount = 50000;
#endif

    BenchmarkResultTable table;
    table.addString(SzPtrUtf8("Date"), 0, 0);
    table.addString(SzPtrUtf8("Test machine"), 0, 1);
    table.addString(SzPtrUtf8("Test Compiler"), 0, 2);
    table.addString(SzPtrUtf8("Pointer size"), 0, 3);
    table.addString(SzPtrUtf8("Build type"), 0, 4);
    table.addString(DFG_UTF8("Insert count"), 0, 5);
    table.addString(SzPtrUtf8("Test type"), 0, 6);
    const auto nLastStaticColumn = 6;

    const auto nElementTypeCount = 4;
    const auto nContainerCount = 3;

    for (size_t i = 0; i < 5; ++i) // Iterations.
    {
        if (i == 0)
        {
            const StringUtf8 sTime(SzPtrUtf8(DFG_MODULE_NS(time)::localDate_yyyy_mm_dd_C().c_str()));
            const auto sCompiler = SzPtrUtf8(DFG_COMPILER_NAME_SIMPLE);
            const StringUtf8 sPointerSize(SzPtrUtf8(DFG_MODULE_NS(str)::toStrC(sizeof(void*)).c_str()));
            const auto sBuildType = SzPtrUtf8(DFG_BUILD_DEBUG_RELEASE_TYPE);
            const StringUtf8 sInsertCount(SzPtrUtf8(DFG_MODULE_NS(str)::toStrC(nCount).c_str()));
            for (int et = 1; et <= nElementTypeCount; ++et)
            {
                for (int ct = 0; ct < nContainerCount; ++ct)
                {
                    const auto r = table.rowCountByMaxRowIndex();
                    table.addString(sTime, r, 0);
                    table.addString(sCompiler, r, 2);
                    table.addString(sPointerSize, r, 3);
                    table.addString(sBuildType, r, 4);
                    table.addString(sInsertCount, r, 5);
                }
            }
        }

        table.addString(SzPtrUtf8(("Time#" + toStrC(i)).c_str()), 0, table.colCountByMaxColIndex());
        VectorInsertImpl<int>(nCount, &table, 1, nLastStaticColumn);
        VectorInsertImpl<double>(nCount, &table, 1 + 1 * nContainerCount, nLastStaticColumn);
        VectorInsertImpl<std::pair<int, int>>(nCount, &table, 1 + 2 * nContainerCount, nLastStaticColumn);
        VectorInsertImpl<DFG_MODULE_NS(cont)::TrivialPair<int, int>>(nCount, &table, 1 + 3 * nContainerCount, nLastStaticColumn);
    }

    // Calculate averages etc.
    table.addReducedValues(nLastStaticColumn + 1);

    DFG_MODULE_NS(io)::OfStream ostrm(format_fmt("testfiles/generated/benchmarkVectorInsert_{}.csv", generateCompilerInfoForOutputFilename()));
    table.writeToStream(ostrm);
}

#endif // on/off switch for performance tests.

// This is not a performance test but placed in this file for now to get the same infrastructure as the performance test.
TEST(dfgCont, VectorInsert)
{
    const int nCount = 10;
    VectorInsertImpl<int>(nCount);
    VectorInsertImpl<double>(nCount);
    VectorInsertImpl<std::pair<int, int>>(nCount);
    VectorInsertImpl<DFG_MODULE_NS(cont)::TrivialPair<int, int>>(nCount);
}

#endif
