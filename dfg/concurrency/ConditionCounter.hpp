#pragma once

#include "../dfgDefs.hpp"
#include "../dfgAssert.hpp"
#include <mutex>
#include <condition_variable>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(concurrency) {

// Helper class for easier alternative to condition_variables and mutexes: allows multiple clients to decrement counter and multiple clients to wait for 'counter == 0' condition
//      -Object is initialized with a counter value
//      -Signallers call decrementCounter() to decrease counter
//      -Waiters call waitCounter() to wait for 'counter == 0' condition
// Examples:
//      -Launch N worker threads and wait until they finish a task
//          -Create ConditionCounterT(N)
//          -Launch worker threads giving reference to ConditionCounterT-object and call decrementCounter() for ConditionCounter-object when they have completed the task
//          -In wait thread, call waitCounter()
//      -Unblock N worker threads when a condition is satisfied (alternatively could be done e.g. with std::shared_future)
//          -Create ConditionCounterT(1)
//          -Launch worker threads and call waitCounter()
//          -call decrementCounter() to unblock all workr threads
template <class Int_T>
class ConditionCounterT
{
public:
    ConditionCounterT(const Int_T nCounter)
        : m_nCounter(nCounter)
    {
    }

    // Decreases counter by one
    // Precondition: m_nCounter > 0
    void decrementCounter()
    {
        DFG_ASSERT_UB(m_nCounter > 0);
        std::unique_lock<std::mutex> lock(m_mutex);
        --m_nCounter;
        if (m_nCounter <= 0)
        {
            lock.unlock();
            m_condVar.notify_all();
        }
    }

    // Blocks until counter is zero
    void waitCounter()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_nCounter > 0)
            m_condVar.wait(lock, [&] { return m_nCounter == 0; });
    }

    Int_T m_nCounter;
    std::mutex m_mutex;
    std::condition_variable m_condVar;
}; // class ConditionCounterT

using ConditionCounter = ConditionCounterT<size_t>;

 } } // namespace dfg::concurrency
