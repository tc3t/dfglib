#pragma once

#include "../dfgDefs.hpp"
#include "MapVector.hpp"
#include "../math.hpp"
#include "../numericTypeTools.hpp"


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
    using IntervalCont = MapVectorSoA<T, T>;
    using ReadOnlyInputT = typename std::conditional<std::is_integral<T>::value, T, const T&>::type;

    static IntervalSet makeSingleInterval(ReadOnlyInputT left, ReadOnlyInputT right);

    // Inserts single item
    void insert(const T& t);

    // Inserts closed interval
    void insertClosed(const T& first, const T& right);

    bool hasValue(const T& val) const;

    // Returns the number of elements in set defined by the ranges
    // Note: guaranteed to return valid value only if size is at most NumericTraits<size_t>::maxValue. If value is greater, returned value is faulty, but will not trigger UB.
    //       In practice this means that if sizeof(T) < sizeof(size_t), don't need to worry about this. And in case of sizeof(T) == sizeof(size_t), only failing case is that 
    //       intervals define the whole range [NumericTraits<T>::minValue, NumericTraits<T>::maxValue].
    sizeType sizeOfSet() const;

    sizeType intervalCount() const { return m_intervals.size(); }

    bool isSingleInterval(ReadOnlyInputT left, ReadOnlyInputT right) const;

    // Calls given function for each contiguous range in ascending order.
    // Function gets two arguments: (T lowerBound, T upperBound).
    template <class Func_T>
    void forEachContiguousRange(Func_T&& func) const;

    bool empty() const;

    // Returns the number of elements in range [lower, uppper].
    // Note: like with sizeOfSet(), this will return faulty result if count would be > maxOf(sizeType).
    sizeType countOfElementsInRange(const T lower, const T upper) const;

    // Returns the largest element in the set.
    // Precondition: !empty()
    T maxElement() const;

    // Returns the smallest element in the set.
    // Precondition: !empty()
    T minElement() const;

    static sizeType countOfElementsInIntersection(const T lower0, const T upper0, const T lower1, const T upper1);

    // Wraps negative elements '-k' to Max(0, 'nBound - k'). For example wrapping {-2, -1, 3} with wrapNegatives(10) would result to {3, 8, 9}
    // and {-10, -1, 3} with wrapNegatives(7) -> {0, 3, 6}
    // If nBound is negative, does nothing.
    void wrapNegatives(T nBound);

    void privMergeIntervalsStartingFrom(const sizeType n);
    bool privIsWithinInterval(const sizeType n, const T& item) const;
    typename IntervalCont::const_iterator privFirstIntervalWithLeftLowerOrEqualTo(const T item) const;

    // Key = start, value = end. (both are inclusive)
    // Content in m_intervals shall always fulfill the following conditions:
    //  1. m_intervals.keyRange()[i] <= m_intervals.valueRange()[i] for all i in [0, m_intervals.size()[
    //  2. m_intervals.value(i) < m_intervals.keyRange()[i + 1] for all i in [0, m_intervals.size() - 1[
    // In other words all intervals are disjoint and stored in ascending order.
    MapVectorSoA<T, T> m_intervals; 
}; // IntervalSet

template <class T>
auto IntervalSet<T>::makeSingleInterval(ReadOnlyInputT left, ReadOnlyInputT right) -> IntervalSet<T>
{
    IntervalSet<T> is;
    is.insertClosed(left, right);
    return is;
}

template <class T>
bool IntervalSet<T>::isSingleInterval(ReadOnlyInputT left, ReadOnlyInputT right) const
{
    return m_intervals.size() == 1 && m_intervals.frontKey() == left && m_intervals.frontValue() == right;
}

template <class T>
void IntervalSet<T>::insert(const T& t)
{
    insertClosed(t, t);
}

template <class T>
auto IntervalSet<T>::sizeOfSet() const -> sizeType
{
    sizeType n = 0;
    for (const auto item : m_intervals)
        n += ::DFG_MODULE_NS(math)::numericDistance(item.first, item.second) + sizeType(1);
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
        if (backValue < left && left > backValue + 1)
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

        sizeType nRightIntervalIndex = iterLb - keys.begin(); // Index of interval whose start value is >= to 'left' of new interval.
        sizeType nIndex = m_intervals.size();
        if (privIsWithinInterval(nRightIntervalIndex, left)) // Is left boundary within right interval?
            nIndex = nRightIntervalIndex;
        else if (nRightIntervalIndex >= 1 && privIsWithinInterval(nRightIntervalIndex - 1, left)) // Is left boundary within left interval?
            nIndex = nRightIntervalIndex - 1;
        if (nIndex < m_intervals.size()) // Was left value within existing interval?
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

template <class T>
template <class Func_T>
void IntervalSet<T>::forEachContiguousRange(Func_T&& func) const
{
    for (const auto& kv : m_intervals)
    {
        func(kv.first, kv.second);
    }
}

template <class T>
auto IntervalSet<T>::countOfElementsInIntersection(const T lower0, const T upper0, const T lower1, const T upper1) -> sizeType
{
    DFG_STATIC_ASSERT(std::is_integral<T>::value, "This function is for integer types only");
    if (lower0 > upper0 || lower1 > upper1)
        return 0;
    const auto firstLower = Min(lower0, lower1);
    const auto firstUpper = (firstLower == lower0) ? upper0 : upper1;
    const auto secondLower = (firstLower == lower0) ? lower1 : lower0;
    const auto secondUpper = (firstLower == lower0) ? upper1 : upper0;
    if (secondLower > firstUpper)
        return 0; // Disjoint intervals.
    else // Case: secondLower is within first interval.
        return sizeType(Min(firstUpper, secondUpper) - Max(firstLower, secondLower)) + 1;
}


template <class T>
auto IntervalSet<T>::countOfElementsInRange(const T lower, const T upper) const -> sizeType
{
    DFG_STATIC_ASSERT(std::is_integral<T>::value, "This function is for integer types only");
    sizeType n = 0;
    if (empty() || lower > upper)
        return 0;
    auto iter = privFirstIntervalWithLeftLowerOrEqualTo(lower);
    if (iter == m_intervals.end())
    {
        // lower is less than left in all intervals -> starting from first interval
        iter = m_intervals.begin();
    }
    const auto iterEnd = m_intervals.end();
    for (; iter != iterEnd; ++iter)
    {
        if (iter->first > upper) // Remaining intervals are beyond upper so there's nothing left to do.
            return n;
        n += countOfElementsInIntersection(iter->first, iter->second, lower, upper);
    }
    return n;
}

template <class T>
bool IntervalSet<T>::empty() const
{
    return m_intervals.empty();
}

// Returns the largest element in the set.
// Precondition: !empty()
template <class T>
auto IntervalSet<T>::maxElement() const -> T
{
    DFG_ASSERT(!empty());
    return (!empty()) ? m_intervals.backValue() : minValueOfType<T>();
}

// Returns the smallest element in the set.
// Precondition: !empty()
template <class T>
auto IntervalSet<T>::minElement() const->T
{
    DFG_ASSERT(!empty());
    return (!empty()) ? m_intervals.frontKey() : maxValueOfType<T>();
}

template <class T>
void IntervalSet<T>::wrapNegatives(const T nBound)
{
    if (nBound < 0)
        return;
    for (size_t i = 0; i < m_intervals.size();)
    {
        const auto intervalLeft = m_intervals.keyRange()[i];
        const auto intervalRight = m_intervals.valueRange()[i];
        auto left = intervalLeft;
        if (left >= 0)
            return; // This and remaining items are non-negative -> nothing left to do.
        auto right = intervalRight;
        if (right >= 0) // If interval is [-k, n >= 0], adjusting it to [0, n]
        {
            m_intervals.keyRange_modifiable()[i] = 0;
            right = -1;
        }
        left = Max(-nBound, left); // To make sure that nBound - left doesn't go negative
        right = Max(-nBound, right); // To make sure that nBound - right doesn't go negative
        DFG_ASSERT_CORRECTNESS(left <= right);
        insertClosed(nBound + left, nBound + right);
        if (intervalRight < 0) // Removing old only if it was completely in negative side in case which it moves completely to positive side.
            DFG_VERIFY(m_intervals.erase(intervalLeft) == 1);
    }
}

template <class T>
auto IntervalSet<T>::privFirstIntervalWithLeftLowerOrEqualTo(const T item) const -> typename IntervalCont::const_iterator
{
    if (empty())
        return m_intervals.end();
    const auto keys = m_intervals.keyRange();
    auto iterLb = std::lower_bound(keys.cbegin(), keys.cend(), item); // iterLb points to first element >= 'item'
    if (iterLb != keys.end() && *iterLb == item)
        return m_intervals.begin() + (iterLb - keys.begin());
    if (iterLb == keys.begin())
        return m_intervals.end();
    --iterLb;
    return m_intervals.begin() + (iterLb - keys.begin());
}

} } // module namespace
