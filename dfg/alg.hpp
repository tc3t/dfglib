#ifndef DFG_ALG_TXKYQDAV
#define DFG_ALG_TXKYQDAV

#include "dfgBase.hpp"
#include <algorithm>
#include "ptrToContiguousMemory.hpp"
#include "alg/generateAdjacent.hpp"
#include <iterator> // std::distance

// TODO: Check usage of std::begin/end. Should it use dfg-defined begin/end (e.g. in case that the used environment does not provide those?)

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(alg) {

// Returns index of item 'val' in range 'range'.
// If not found, returns value that is not valid in index of range (can be tested e.g. with isValidIndex).
// In case of duplicates, returns the smallest index that match.
// See also floatIndexInSorted.
template <class RangeT, class T>
size_t indexOf(const RangeT& range, const T& val)
{
    return std::distance(DFG_ROOT_NS::cbegin(range), std::find(DFG_ROOT_NS::cbegin(range), DFG_ROOT_NS::cend(range), val));
}

// For a given range of ascending sorted values, returns interpolated floating position of 'val' in given range.
// If val matches any of the range elements exactly, return value is identical to indexOf.
// If 'val' is smaller than first, it's index if extrapolated linearly from first bin width.
// Similarly for values above last, index is extrapolated from last bin width.
// Examples:
template <class Iterable_T, class T>
double floatIndexInSorted(const Iterable_T& iterable, const T& val)
{
    const auto iterBegin = DFG_ROOT_NS::cbegin(iterable);
    const auto iterEnd = DFG_ROOT_NS::cend(iterable);
    if (iterBegin == iterEnd) // Check for empty range.
        return std::numeric_limits<double>::quiet_NaN();
    auto iterSecond = iterBegin;
    ++iterSecond;
    if (iterSecond == iterEnd) // Check for single item range. If val matches first, return 0, otherwise NaN.
        return (*iterBegin == val) ? 0 : std::numeric_limits<double>::quiet_NaN();
    DFG_ASSERT(std::is_sorted(iterBegin, iterEnd));
    auto iter = std::lower_bound(iterBegin, iterEnd, val);
    if (iter != iterEnd)
    {
        if (iter != iterBegin)
        {
            auto prev = iter;
            --prev;
            return std::distance(iterBegin, prev) + (val - *prev) / (*iter - *prev);
        }
        else // Case: iter == iterBegin which means that val is <= *iterBegin.
        {
            if (val == *iterBegin)
                return 0;
            else
            {
                const auto firstBinWidth = *iterSecond - *iterBegin;
                return 0.0 - (*iterBegin - val) / firstBinWidth;
            }
        }
    }
    else
    {
        auto iterLast = iterEnd;
        --iterLast;
        auto iterOneBeforeLast = iterLast;
        --iterOneBeforeLast;
        const auto lastBinWidth = *iterLast - *iterOneBeforeLast;
        return (count(iterable) - 1) + (val - *iterLast) / lastBinWidth;
    }
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
}

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
    return std::move(f);
}

// Like forEachFwd, but takes func by (universal) reference.
// TODO: test
template <class Range_T, class Func>
void forEachFwdFuncRef(Range_T&& range, Func&& f)
{
    DFG_DETAIL_NS::forEachFwdImpl(range, f, std::integral_constant<bool, IsContiguousRange<Range_T>::value>());
}

// Like ForEachFwd, but also provides item index to functor.
// TODO: Should work also when range is rvalue.
// TODO: test
template <class RangeT, class Func>
void forEachFwdWithIndex(RangeT& range, Func f)
{
    auto iterEnd = std::end(range);
    size_t i = 0;
    for(auto iter = std::begin(range); iter != iterEnd; ++iter, ++i)
        f(*iter, i);
}

// Like std::fill, but supports range-based semantics.
// TODO: Should work also when range is rvalue.
template <class RangeT, class ValueT>
void fill(RangeT& range, const ValueT& val)
{
    std::fill(std::begin(range), std::end(range), val);
}

// Returns iterator to the element that is nearest to "val", where the distance is determined by diff abs.
// TODO: test
template <class Iterable_T, class Value_T>
auto findNearest(const Iterable_T& iterable, const Value_T& val) -> decltype(iterable.begin())
{
    typedef double DiffType;
    auto diffFunc = std::minus<DiffType>();
    const auto iterEnd = iterable.end();
    if (iterable.begin() == iterEnd)
        return iterEnd;
    auto iter = iterable.begin();
    auto smallestDiff = diffFunc(val, *iter);
    if (smallestDiff == 0)
        return iter;
    auto iterSmallest = iter;
    ++iter;
    for(; iter != iterEnd; ++iter)
    {
        const DiffType newDiff = diffFunc(val, *iter);
        if (std::abs(newDiff) < std::abs(smallestDiff))
        {
            smallestDiff = newDiff;
            iterSmallest = iter;
            if (smallestDiff == 0)
                return iter;
        }
    }
    return iterSmallest;
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

// Given two ranges {a0, a1, ..., an} and {b0, b1, ..., bn), erases all items ai where corresponding
// bi is false.
// TODO: test
template <class Cont_T, class FlagCont_T>
void eraseByFlags(Cont_T& cont, const FlagCont_T& flags)
{
    auto flagIter = cbegin(flags);
    auto iterValidEnd = removeIf(cont.begin(), cont.end(), [&](typename Cont_T::value_type&) -> bool
    {
        if (flagIter != flags.end())
        {
            const bool b = *flagIter;
            ++flagIter;
            return b;
        }
        else
            return false;
    });
    cont.erase(iterValidEnd, cont.end());
}


}} // module alg

#endif // include guard
