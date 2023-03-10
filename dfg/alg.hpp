#ifndef DFG_ALG_TXKYQDAV
#define DFG_ALG_TXKYQDAV

#include "dfgBase.hpp"
#include <algorithm>
#include "ptrToContiguousMemory.hpp"
#include "alg/generateAdjacent.hpp"
#include <iterator> // std::distance
#include "cont/elementType.hpp"
#include "rangeIterator.hpp"
#include "math.hpp"

// TODO: Check usage of std::begin/end. Should it use dfg-defined begin/end (e.g. in case that the used environment does not provide those?)

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(alg) {

namespace DFG_DETAIL_NS
{
    template <class Elem_T>
    struct DefaultSortKeyFunc
    {
        const Elem_T& operator()(const Elem_T& a) const
        {
            return a;
        }
    };
} // namespace DFG_DETAIL_NS

// Returns index of item 'val' in range 'range'.
// If not found, returns value that is not valid in index of range (can be tested e.g. with isValidIndex).
// In case of duplicates, returns the smallest index that match.
// See also floatIndexInSorted.
template <class RangeT, class T>
size_t indexOf(const RangeT& range, const T& val)
{
    return std::distance(DFG_ROOT_NS::cbegin(range), std::find(DFG_ROOT_NS::cbegin(range), DFG_ROOT_NS::cend(range), val));
}

// Tests existence of element in a range.
template <class Range_T, class T>
bool contains(const Range_T& range, const T& val)
{
    return isValidIndex(range, indexOf(range, val));
}

// For a given range of ascending sorted(*) values, returns interpolated floating position of 'val' in given range.
// If val matches any of the range elements exactly, return value is identical to indexOf.
// If 'val' is smaller than first, it's index if extrapolated linearly from first bin width.
// Similarly for values above last, index is extrapolated from last bin width.
// (*) Range is expected to be sorted in the sense that range of [toSortKey(iterable[0]), ... toSortKey(iterable[last])] is sorted.
// Examples:
template <class Iterable_T, class T, class ToSortKey_T>
double floatIndexInSorted(const Iterable_T& iterable, const T& val, const ToSortKey_T& toSortKey)
{
    using ValueT = typename ::DFG_MODULE_NS(cont)::ElementType<Iterable_T>::type;
    const auto iterBegin = DFG_ROOT_NS::cbegin(iterable);
    const auto iterEnd = DFG_ROOT_NS::cend(iterable);
    if (iterBegin == iterEnd) // Check for empty range.
        return std::numeric_limits<double>::quiet_NaN();
    auto iterSecond = iterBegin;
    ++iterSecond;
    if (iterSecond == iterEnd) // Check for single item range. If val matches first, return 0, otherwise NaN.
        return (toSortKey(*iterBegin) - val == 0) ? 0 : std::numeric_limits<double>::quiet_NaN();
    const auto sortPredLowerBound = [&](const ValueT& a, const T& b) { return toSortKey(a) < b; };
    DFG_ASSERT(std::is_sorted(iterBegin, iterEnd, [&](const ValueT& a, const ValueT& b) { return toSortKey(a) < toSortKey(b); }));
    auto iter = std::lower_bound(iterBegin, iterEnd, val, sortPredLowerBound);
    if (iter != iterEnd)
    {
        if (iter != iterBegin)
        {
            auto prev = iter;
            --prev;
            return static_cast<double>(std::distance(iterBegin, prev)) + (val - toSortKey(*prev)) / (toSortKey(*iter) - toSortKey(*prev));
        }
        else // Case: iter == iterBegin which means that val is <= *iterBegin.
        {
            if (toSortKey(*iterBegin) - val == 0)
                return 0;
            else
            {
                const auto firstBinWidth = toSortKey(*iterSecond) - toSortKey(*iterBegin);
                return 0.0 - (toSortKey(*iterBegin) - val) / firstBinWidth;
            }
        }
    }
    else
    {
        auto iterLast = iterEnd;
        --iterLast;
        auto iterOneBeforeLast = iterLast;
        --iterOneBeforeLast;
        const auto lastBinWidth = toSortKey(*iterLast) - toSortKey(*iterOneBeforeLast);
        return static_cast<double>(count(iterable) - 1) + (val - toSortKey(*iterLast)) / lastBinWidth;
    }
}

template <class Iterable_T, class T>
double floatIndexInSorted(const Iterable_T& iterable, const T& val)
{
    using ValueT = typename ::DFG_MODULE_NS(cont)::ElementType<Iterable_T>::type;
    return floatIndexInSorted(iterable, val, DFG_DETAIL_NS::DefaultSortKeyFunc<ValueT>());
}

namespace DFG_DETAIL_NS
{
    template <class Range_T, class Func>
    void forEachFwdImpl(Range_T&& range, Func&& f, std::true_type)
    {
        auto iter = ptrToContiguousMemory(range);
        auto iterEnd = iter + count(range);
        for (; iter != iterEnd; ++iter)
            f(*iter);
    }

    template <class Range_T, class Func>
    void forEachFwdImpl(Range_T&& range, Func&& f, std::false_type)
    {
        const auto iterEnd = std::end(range);
        for (auto iter = std::begin(range); !isAtEnd(iter, iterEnd); ++iter)
            f(*iter);
    }

} // namespace DFG_DETAIL_NS

// Like std::for_each, but accepts ranges and explicitly guarantees in order advance.
// Function name suffix Fwd emphasizes that the iteration will be done in order
// from first to last.
// Note: range should be taken as reference instead of copy since it may refer to container itself
//       or iterator pair; in the first case copying is unacceptable.
// TODO: Test
template <class Range_T, class Func>
Func forEachFwd(Range_T&& range, Func f)
{
    DFG_DETAIL_NS::forEachFwdImpl(range, f, std::integral_constant<bool, IsContiguousRange<Range_T>::value>());
    return f;
}

// Like forEachFwd, but takes func by (universal) reference.
// TODO: test
template <class Range_T, class Func>
void forEachFwdFuncRef(Range_T&& range, Func&& f)
{
    DFG_DETAIL_NS::forEachFwdImpl(range, f, std::integral_constant<bool, IsContiguousRange<Range_T>::value>());
}

template <class Index_T, class RangeT, class Func>
void forEachFwdWithIndexT(RangeT& range, Func f)
{
    auto iterEnd = std::end(range);
    Index_T i = 0;
    for (auto iter = std::begin(range); iter != iterEnd; ++iter, ++i)
        f(*iter, i);
}

// Like ForEachFwd, but also provides item index to functor.
// TODO: Should work also when range is rvalue.
// TODO: test
template <class RangeT, class Func>
void forEachFwdWithIndex(RangeT& range, Func f)
{
    forEachFwdWithIndexT<size_t>(range, f);
}

// Like std::fill, but supports range-based semantics.
// TODO: Should work also when range is rvalue.
template <class RangeT, class ValueT>
void fill(RangeT& range, const ValueT& val)
{
    std::fill(std::begin(range), std::end(range), val);
}

// Returns iterator to the element that is nearest to "val", where the distance is determined by given function.
// DiffFunc_T shall return non-negative value.
// DiffFunc_T takes two parameters: (ElementOfIterable, Value_T)
// In case of multiple minimum value items, returns iterator to the first occurrence.
// See also: floatIndexInSorted()
template <class Iterable_T, class Value_T, class DiffFunc_T>
auto findNearest(const Iterable_T& iterable, const Value_T& val, DiffFunc_T diffFunc) -> decltype(iterable.begin())
{
    const auto iterEnd = iterable.end();
    if (iterable.begin() == iterEnd)
        return iterEnd;
    auto iter = iterable.begin();
    auto smallestDiff = diffFunc(*iter, val);
    if (smallestDiff == 0)
        return iter;
    auto iterSmallest = iter;
    ++iter;
    for(; iter != iterEnd; ++iter)
    {
        const auto newDiff = diffFunc(*iter, val);
        if (newDiff < smallestDiff)
        {
            smallestDiff = newDiff;
            iterSmallest = iter;
            if (smallestDiff == 0)
                return iter;
        }
    }
    return iterSmallest;
}

// Returns iterator to the element that is nearest to "val", where the distance is determined by absolute value of difference (operator-).
template <class Iterable_T, class Value_T>
auto findNearest(const Iterable_T& iterable, const Value_T& val) -> decltype(iterable.begin())
{
    typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Iterable_T>::type ElemT;
    return findNearest(iterable, val, [](const ElemT& e, const Value_T& v) {return std::abs(v - e); });
}

// ForEach version where the function gets iterator instead of the object itself.
template <class Iter_T, class Func_T>
void forEachFwdIter(Iter_T begin, const Iter_T end, Func_T f)
{
    for(; begin != end; ++begin)
    {
        f(begin);
    }
}

// Overload taking data range.
template <class Iterable_T, class Func_T>
void forEachFwdIter(const Iterable_T& iterable, Func_T f)
{
    forEachFwdIter(iterable.begin(), iterable.end(), f);
}

// Remove_if version that is guaranteed to iterate the range in forward manner calling
// predicate on every step.
// TODO: test
template<class Iter_T, class Pred_T>
Iter_T removeIf(Iter_T begin, Iter_T end, Pred_T p)
{
    begin = std::find_if(begin, end, p);
    if (begin != end)
    {
        auto i = begin;
        ++i;
        for(; i != end; ++i)
        {
            if (!p(*i))
                *begin++ = std::move(*i);
        }
    }
    return begin;
}

// Given two ranges {a0, a1, ..., an} and {b0, b1, ..., bn}, erases all items ai where corresponding
// bi is false.
// TODO: test
template <class Cont_T, class FlagCont_T>
void keepByFlags(Cont_T& cont, const FlagCont_T& flags)
{
    auto flagIter = ::DFG_ROOT_NS::cbegin(flags);
    auto iterValidEnd = removeIf(cont.begin(), cont.end(), [&](typename Cont_T::value_type&) -> bool
    {
        if (flagIter != flags.end())
        {
            const bool b = *flagIter;
            ++flagIter;
            return !b;
        }
        else
            return false;
    });
    cont.erase(iterValidEnd, cont.end());
}

template <class Iter_T>
class NearestRangeReturnValue
{
public:
    NearestRangeReturnValue(Iter_T iterBegin, Iter_T iterEnd, Iter_T iterNearest)
        : m_begin(iterBegin)
        , m_end(iterEnd)
        , m_nearest(iterNearest)
    {}

    Iter_T begin() const    { return m_begin; }
    Iter_T end() const      { return m_end; }
    Iter_T nearest() const  { return m_nearest; }
    size_t size() const     { return static_cast<size_t>(std::distance(m_begin, m_end)); }
    bool empty() const      { return begin() == end(); }
    // Returns range that has items on left side of nearest()
    RangeIterator_T<Iter_T> leftRange()  const { return makeRange(begin(), nearest()); }
    // Returns range that has items on right side of nearest()
    RangeIterator_T<Iter_T> rightRange() const { return makeRange((!empty()) ? nearest() + 1 : end(), end()); }
    
    Iter_T m_begin;
    Iter_T m_end;
    Iter_T m_nearest;
};

// Given a sorted range as expected by floatIndexInSorted(), search value, count and toSortkey-function, returns a range of nearest items and the iterator to nearest item.
// If elements have equal distance, prefers to include left-side items.
template <class Iterable_T, class T, class ToSortKey_T>
auto nearestRangeInSorted(Iterable_T&& iterable, const T& val, size_t nCount, ToSortKey_T&& toSortKey) -> NearestRangeReturnValue<decltype(std::begin(iterable))>
{
    using IterT = decltype(std::begin(iterable));
    using ReturnT = NearestRangeReturnValue<IterT>;
    const auto iterBegin = std::begin(iterable);
    const auto iterEnd = std::end(iterable);
    if (iterBegin == iterEnd || nCount == 0 || ::DFG_MODULE_NS(math)::isNan(val))
        return ReturnT(iterEnd, iterEnd, iterEnd);
    const auto nRangeSize = static_cast<size_t>(std::distance(iterBegin, iterEnd));
    if (nRangeSize == 1)
        return ReturnT(iterBegin, iterEnd, iterBegin);

    nCount = Min(nCount, nRangeSize);
    const auto findex = floatIndexInSorted(iterable, val, toSortKey);
    if (findex <= 0.5)
        return ReturnT(iterBegin, iterBegin + nCount, iterBegin);
    else if (findex >= static_cast<double>(nRangeSize - 1))
        return ReturnT(iterEnd - nCount, iterEnd, iterEnd - 1);

    // Getting here means that index if within [0.5, nRangeSize - 1]
    const auto nNearest = ::DFG_ROOT_NS::round<size_t>(findex);
    const auto iterNearest = iterBegin + nNearest;
    if (nCount == 1)
        return ReturnT(iterNearest, iterNearest + 1, iterNearest);

    auto iterNearestBegin = iterNearest;
    auto iterNearestEnd = iterNearest + 1;

    for (size_t nIncludeCount = 1; nIncludeCount < nCount; ++nIncludeCount)
    {
        if (iterNearestBegin == iterBegin) // Nothing left on left side?
        {
            DFG_ASSERT_UB(std::distance(iterNearestEnd, iterEnd) >= static_cast<ptrdiff_t>((nCount - nIncludeCount)));
            iterNearestEnd += (nCount - nIncludeCount); // Fill right items
            break;
        }
        if (iterNearestEnd == iterEnd) // Nothing left on right side?
        {
            DFG_ASSERT_UB(std::distance(iterBegin, iterNearestBegin) >= static_cast<ptrdiff_t>((nCount - nIncludeCount)));
            iterNearestBegin -= (nCount - nIncludeCount); // Fill left items
            break;
        }
        // Getting here means that there's next item on both side. Determining which one is closer
        const auto leftDistance = val - toSortKey(*(iterNearestBegin - 1));
        const auto rightDistance = toSortKey(*iterNearestEnd) - val;
        DFG_ASSERT(leftDistance >= 0); // Expecting distance to be non-negative
        DFG_ASSERT(rightDistance >= 0); // Expecting distance to be non-negative
        if (leftDistance <= rightDistance)
            --iterNearestBegin;
        else
            ++iterNearestEnd;
    }
    return ReturnT(iterNearestBegin, iterNearestEnd, iterNearest);
}

template <class Iterable_T, class T>
auto nearestRangeInSorted(Iterable_T&& iterable, const T& val, size_t nCount) -> NearestRangeReturnValue<decltype(std::begin(iterable))>
{
    using ValueT = typename ::DFG_MODULE_NS(cont)::ElementType<Iterable_T>::type;
    return nearestRangeInSorted(std::forward<Iterable_T>(iterable), val, nCount, DFG_DETAIL_NS::DefaultSortKeyFunc<ValueT>());
}


}} // module alg

#endif // include guard
