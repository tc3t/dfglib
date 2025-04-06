#include "stdafx.h"

#if (defined(DFGTEST_BUILD_MODULE_CONCURRENCY) && DFGTEST_BUILD_MODULE_CONCURRENCY == 1) || (!defined(DFGTEST_BUILD_MODULE_CONCURRENCY) && DFGTEST_BUILD_MODULE_DEFAULT == 1)

#define DFGTEST_CONCURRENT_DEBUG_PRINTS 0

#include <dfg/concurrency/ConditionCounter.hpp>
#include <dfg/concurrency/ThreadList.hpp>
#include <dfg/io/nullOutputStream.hpp>
#include <dfg/rand.hpp>
#include <thread>

namespace
{
    // Helper class for synced writing to given std::ostream.
    // Example:
    //      DebugStream strm(std::cout);
    //      for (auto& thread : threads)
    //          strm << one << another << '\n'; // Prints from different thread won't overlap.
    // 
    class LocallySyncedOStream
    {
    public:
        class LocallySyncedOStreamOperation
        {
        public:
            LocallySyncedOStreamOperation(std::ostream& rStrm, std::mutex& rMutex)
                : m_pStream(&rStrm)
                , m_rMutex(rMutex)
            {}

            LocallySyncedOStreamOperation(const LocallySyncedOStreamOperation&) = delete;

            LocallySyncedOStreamOperation(LocallySyncedOStreamOperation&& other) DFG_NOEXCEPT_TRUE
                : m_pStream(other.m_pStream)
                , m_rMutex(other.m_rMutex)
            {
                other.m_pStream = nullptr;
            }

            ~LocallySyncedOStreamOperation()
            {
                if (m_pStream)
                {
                    m_pStream->flush();
                    m_rMutex.unlock();
                }
            }

            template <class T>
            LocallySyncedOStreamOperation& operator<<(const T& obj)
            {
                if (m_pStream)
                    (*m_pStream) << obj;
                return *this;
            }

            std::ostream* m_pStream;
            std::mutex& m_rMutex;
        }; // class LocallySyncedStreamOperation

        LocallySyncedOStream(std::ostream& strm)
            : m_rStream(strm)
        {}

        template <class T>
        LocallySyncedOStreamOperation operator<<(const T& obj)
        {
            m_mutex.lock();
            m_rStream << obj;
            return LocallySyncedOStreamOperation(m_rStream, m_mutex);
        }

        std::ostream& m_rStream;
        std::mutex m_mutex;
    }; // class LocallySyncedOStream

    class ThisThreadPrintable
    {
    public:
        ThisThreadPrintable(size_t nIndex)
            : m_nIndex(nIndex)
        {}

        size_t m_nIndex;
    };

    DFG_MAYBE_UNUSED std::ostream& operator<<(std::ostream& ostrm, const ThisThreadPrintable& printable)
    {
        ostrm << printable.m_nIndex << " (" << std::this_thread::get_id() << ")";
        return ostrm;
    }

} // unnamed namespace

#if DFGTEST_CONCURRENT_DEBUG_PRINTS == 1
    using DebugStream = LocallySyncedOStream;
#else
    // Not optimal to have null-printed stuff evaluated (e.g. debuStrm << someFunction())
    using DebugStream = ::DFG_MODULE_NS(io)::NullOutputStream;
#endif


TEST(dfgConcurrency, ConditionCounter)
{
    using namespace ::DFG_MODULE_NS(concurrency);

    // Test for "wake thread when all workers are done"
    {
        ThreadList threads;
        const auto nThreadCount = 8;
        std::atomic<size_t> anDecrementCounter{ 0 };
        ConditionCounter waitCondition(nThreadCount);
        DebugStream strm(std::cout);
        for (size_t i = 0; i < nThreadCount; ++i)
        {
            threads.push_back(std::thread([&, i]()
                {
                    auto randEng = ::DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();
                    auto distEng = ::DFG_MODULE_NS(rand)::makeDistributionEngineUniform(&randEng, 0, 20);
                    const auto sleepTime = std::chrono::milliseconds(distEng());
                    strm << "Entered thread " << ThisThreadPrintable(i) << " and sleeping for " << sleepTime.count() << " ms\n";
                    std::this_thread::sleep_for(sleepTime);
                    anDecrementCounter.fetch_add(1);
                    waitCondition.decrementCounter();
                    strm << "Done for thread " << ThisThreadPrintable(i) << '\n';
                }));
        }
        DFGTEST_EXPECT_LEFT(nThreadCount, threads.size());
        waitCondition.waitCounter();
        DFGTEST_EXPECT_TRUE(waitCondition.m_mutex.try_lock());
        DFGTEST_EXPECT_LEFT(0, waitCondition.m_nCounter);
        waitCondition.m_mutex.unlock();
        DFGTEST_EXPECT_LEFT(nThreadCount, anDecrementCounter.load());
        strm << "Done\n";
    }

    // Test for "unblock worker threads from single thread"
    {
        ConditionCounter waitCondition(1);
        const size_t nThreadCount = 8;
        std::atomic<size_t> anUnblockCounter{ 0 };
        ThreadList threads;
        DebugStream strm(std::cout);
        for (size_t i = 0; i < nThreadCount; ++i)
        {
            threads.push_back(std::thread([&waitCondition, i, &strm, &anUnblockCounter]()
                {
                    strm << "Entered thread " << ThisThreadPrintable(i) << '\n';
                    waitCondition.waitCounter();
                    strm << "Unblocked thread " << ThisThreadPrintable(i) << '\n';
                    anUnblockCounter.fetch_add(1);
                }));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        DFGTEST_EXPECT_LEFT(0, anUnblockCounter.load());
        waitCondition.decrementCounter();
        threads.joinAllAndClear();
        DFGTEST_EXPECT_LEFT(nThreadCount, anUnblockCounter.load());
    }

    // Testing that 0-constructed object behaves
    {
        ConditionCounter waitCondition(0);
        waitCondition.waitCounter(); // Should not block
    }
}

TEST(dfgConcurrency, ConditionCounter_waitFor)
{
    using namespace ::DFG_MODULE_NS(concurrency);

    {
        ConditionCounter waitCondition(1);
        const size_t nThreadCount = 2;
        std::atomic<size_t> anFailedWaitFors{ 0 };
        std::atomic<size_t> anUnblockCounter{ 0 };
        ThreadList threads;
        DebugStream strm(std::cout);
        for (size_t i = 0; i < nThreadCount; ++i)
        {
            threads.push_back(std::thread([&waitCondition, i, &strm, &anUnblockCounter, &anFailedWaitFors]()
                {
                    strm << "Entered thread " << ThisThreadPrintable(i) << '\n';
                    anFailedWaitFors += size_t(waitCondition.waitCounterFor(std::chrono::milliseconds(1)) == false);
                    anFailedWaitFors += size_t(waitCondition.waitCounterFor(std::chrono::milliseconds(1)) == false);
                    anFailedWaitFors += size_t(waitCondition.waitCounterFor(std::chrono::milliseconds(1000)) == false);
                    waitCondition.waitCounter();
                    anUnblockCounter.fetch_add(1);
                }));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(40)); // Semi-arbitrary, lower value like 15 ms seemed to be too low since anFailedWaitFors would vary between runs.
        waitCondition.decrementCounter();
        threads.joinAllAndClear();
        DFGTEST_EXPECT_LEFT(0, waitCondition.m_nCounter);
        DFGTEST_EXPECT_TRUE(waitCondition.waitCounterFor(std::chrono::milliseconds(0)));
        DFGTEST_EXPECT_LEFT(nThreadCount, anUnblockCounter.load());
        DFGTEST_EXPECT_LEFT(2 * nThreadCount, anFailedWaitFors.load());
    }
}

#endif // On/off switch
