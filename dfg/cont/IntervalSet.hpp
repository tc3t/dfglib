#pragma once

#include "../dfgDefs.hpp"
#include "MapVector.hpp"
#include "../math.hpp"


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

// Implements set of intervals of given type.
//  e.g. instead of storing 100 int items as values 1...100, with IntervalSet<int> this can be stored as one range [1,100].
// Related implementations
//      -Boost.Icl (Interval Container Library)
template <class T>
class IntervalSet
{
public:
    using sizeType = ::std::size_t;

    // Inserts single item
    void insert(const T& t);

    // Inserts closed interval
    void insertClosed(const T& first, const T& right);

    bool hasValue(const T& val) const;

    // Returns the number of elements in set defined by the ranges
    // Note: this may overflow. Guarantees not to trigger UB, but result may be wrong in case of overflow.
    sizeType sizeOfSet() const;

    sizeType intervalCount() const { return m_intervals.size(); }

    void privMergeIntervalsStartingFrom(const sizeType n);

    bool privIsWithinInterval(const sizeType n, const T& item) const;

    // Key = start, value = end. (both are inclusive)
    // Content in m_intervals shall always fulfill the following conditions:
    //  1. m_intervals.keyRange()[i] <= m_intervals.valueRange()[i] for all i in [0, m_intervals.size()[
    //  2. m_intervals.value(i) < m_intervals.keyRange()[i + 1] for all i in [0, m_intervals.size() - 1[
    // In other words all intervals are disjoint and stored in ascending order.
    MapVectorSoA<T, T> m_intervals; 
};

template <class T>
void IntervalSet<T>::insert(const T& t)
{
    insertClosed(t, t);
}

namespace
{
    // TODO: should have generic safeInt operations in a separate module instead of quick hacks like this.
    template <class T>
    typename std::make_unsigned<T>::type safeDiff(const T& lesser, const T& greater, std::true_type)
    {
        if (greater < 0)
            return greater - lesser;
        else if (lesser < 0)
            return static_cast<typename std::make_unsigned<T>::type>(greater) + ::DFG_MODULE_NS(math)::absAsUnsigned(lesser);
        else
            return static_cast<typename std::make_unsigned<T>::type>(greater - lesser);
    }

    template <class T>
    typename std::make_unsigned<T>::type safeDiff(const T& lesser, const T& greater, std::false_type)
    {
        return greater - lesser; // T is unsigned.
    }
}

template <class T>
auto IntervalSet<T>::sizeOfSet() const -> sizeType
{
    sizeType n = 0;
    for (const auto item : m_intervals)
        n += safeDiff(item.first, item.second, std::is_signed<T>()) + sizeType(1);
    return n;
}

template <class T>
bool IntervalSet<T>::privIsWithinInterval(const sizeType n, const T& item) const
{
    if (n >= m_intervals.size())
        return false;
    auto iter = m_intervals.begin() + n;
    return iter->first <= item && iter->second >= item;
}

template <class T>
void IntervalSet<T>::privMergeIntervalsStartingFrom(const sizeType n)
{
    if (m_intervals.empty() || m_intervals.size() - 1 <= n)
        return;
    while (n + 1 < m_intervals.size())
    {
        auto curr = m_intervals.begin() + n;
        auto next = curr + 1;
        if (curr->second < next->first)
            return;
        curr->second = Max(curr->second, next->second);
        m_intervals.erase(next);
    }
}

template <class T>
void IntervalSet<T>::insertClosed(const T& left, const T& right)
{
    DFG_STATIC_ASSERT(std::is_integral<T>::value, "For now IntervalSet supports only integer types");
    if (right < left)
        return;
    const auto keys = m_intervals.keyRange();
    auto values = m_intervals.valueRange();
    auto iterLb = std::lower_bound(keys.cbegin(), keys.cend(), left); // iterLb points to first element >= 'left'
    if (iterLb == keys.cend())
    {
        // In this case all keys are < 'left' -> this is either new disjoint set at end, extends last interval or is within last interval
        if (keys.empty())
        {
            m_intervals[left] = right;
            return;
        }
        const auto& backValue = m_intervals.backValue();
        if (backValue < left)
        {
            m_intervals[left] = right; // New interval
        }
        else // Case: new interval gets merged to last interval
        {
            m_intervals.backValue() = Max(right, m_intervals.backValue());
        }
    }
    else
    {
        // In this case *iter >= left -> new range can extend previous and merge to following, add new disjoint or merge to following.

        sizeType nLeftIntervalIndex = iterLb - keys.begin();
        sizeType nIndex = m_intervals.size();
        if (privIsWithinInterval(nLeftIntervalIndex, left))
            nIndex = nLeftIntervalIndex;
        else if (nLeftIntervalIndex >= 1 && privIsWithinInterval(nLeftIntervalIndex - 1, left))
            nIndex = nLeftIntervalIndex - 1;
        if (nIndex < m_intervals.size())
        {
            const auto nExistingRight = m_intervals.valueRange()[nIndex];
            m_intervals.valueRange()[nIndex] = Max(nExistingRight, right);
            privMergeIntervalsStartingFrom(nIndex);
        }
        else
        {
            // Left is not within any existing interval
            m_intervals[left] = right;
            privMergeIntervalsStartingFrom(m_intervals.find(left) - m_intervals.begin());
            
        } 
    }
}

template <class T>
bool IntervalSet<T>::hasValue(const T& val) const
{
    if (m_intervals.empty())
        return false;
    const auto& keys = m_intervals.keyRange();
    auto iter = std::lower_bound(keys.begin(), keys.end(), val);
    if (iter == keys.end())
        return val <= m_intervals.backValue();
    if (*iter == val)
        return true;
    else if (iter != keys.begin())
    {
        --iter;
        return val <= m_intervals.valueRange()[iter - keys.begin()];
    }
    return false;
}

} } // module namespace
