#pragma once

#include "../dfgDefs.hpp"
#include <vector>
#include <thread>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(concurrency) {

// Helper class for storing list of threads taking care of automatic joining on destruction and providing some operations.
class ThreadList
{
public:
    ~ThreadList();

    void push_back(std::thread&& thread);

    size_t size() const;

    // Precondition: all threads must be joinable.
    void joinAllAndClear();

    std::vector<std::thread> m_threads;
}; // class ThreadList

inline ThreadList::~ThreadList()
{
    joinAllAndClear();
}

inline size_t ThreadList::size() const
{
    return m_threads.size();
}

inline void ThreadList::push_back(std::thread&& thread)
{
    m_threads.push_back(std::move(thread));
}

// Precondition: all threads must be joinable.
inline void ThreadList::joinAllAndClear()
{
    for (auto& thread : m_threads)
        thread.join();
    m_threads.clear();
}

} } // namespace dfg::concurrency
