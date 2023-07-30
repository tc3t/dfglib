#pragma once

#include "../dfgDefs.hpp"
#include "../dfgAssert.hpp"
#include "../ptrToContiguousMemory.hpp"
#include <algorithm>
#include <initializer_list>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(alg) {

namespace DFG_DETAIL_NS
{
    constexpr bool arePtrsAsIntegersLinearlyComparable()
    {
        return true; // TODO: actually check this
    }

    template <class T0, class T1>
    bool areOverlappingSpansImpl(const Span<T0>& lower, const Span<T1>& upper)
    {
        const auto nLowerSize = lower.size();
        const auto nUpperSize = upper.size();
        if (nLowerSize == 0 || nUpperSize == 0)
            return false; // Empty span is interpreted to never overlap with anything even if pointer is inside other span.
        const auto nLowerBegin = reinterpret_cast<uintptr_t>(lower.data());
        const auto nUpperBegin = reinterpret_cast<uintptr_t>(upper.data());
        DFG_ASSERT_CORRECTNESS(nLowerBegin <= nUpperBegin);
        const auto nLowerEnd = nLowerBegin + nLowerSize * sizeof(T0);
        return nUpperBegin < nLowerEnd;
    }

    template <class T0, class T1>
    bool areOverlappingSpans(const Span<T0>& s0, const Span<T1>& s1)
    {
        const auto nBegin0 = reinterpret_cast<uintptr_t>(s0.data());
        const auto nBegin1 = reinterpret_cast<uintptr_t>(s1.data());
        return (nBegin0 <= nBegin1) ? areOverlappingSpansImpl(s0, s1) : areOverlappingSpansImpl(s1, s0);
    }

    template <class Iter0_T, class Iter1_T>
    bool areContiguousByTypeAndOverlappingRangesImpl(const RangeIterator_T<Iter0_T>& r0, const RangeIterator_T<Iter1_T>& r1)
    {
        using T0 = std::decay_t<decltype(*std::begin(r0))>;
        using T1 = std::decay_t<decltype(*std::begin(r1))>;
        const auto bContiguousRanges = IsContiguousRange<RangeIterator_T<Iter0_T>>::value && IsContiguousRange<RangeIterator_T<Iter1_T>>::value;
        if constexpr (bContiguousRanges)
        {
            const auto p0 = ptrToContiguousMemory(r0);
            const auto p1 = ptrToContiguousMemory(r1);
            return DFG_DETAIL_NS::areOverlappingSpans(Span<const T0>(p0, r0.size()), Span<const T1>(p1, r1.size()));
        }
        else if (r0.isSizeOne() && r1.isSizeOne())
        {
            const auto p0 = &*r0.begin();
            const auto p1 = &*r1.begin();
            return DFG_DETAIL_NS::areOverlappingSpans(Span<const T0>(p0, 1), Span<const T1>(p1, 1));
        }
        else
            return false;
    }

    // Checks if given ranges are contiguous ranges by type and overlapping
    // @note Implementation is not standard-wise guaranteed to work. Might be ok on platforms that guarantee certain memory layout.
    // @note Range of size 0 is interpreted to never overlap with everything
    // @note Ranges of size 1 can be determined overlapping even for iterator ranges that are not contiguous by type (e.g. std::list iterators).
    // @note If ranges are piecewise contiguous, possible overlap is not detected (unless both are of size 1)
    // Related links:
    //      https://stackoverflow.com/questions/51699331/how-to-check-that-two-arbitrary-memory-ranges-are-not-overlapped-in-c-c ('How to check that two arbitrary memory ranges are not overlapped in c/c++')
    //      https://stackoverflow.com/questions/39160613/can-the-following-code-be-true-for-pointers-to-different-things ('Can the following code be true for pointers to different things')
    //      https://devblogs.microsoft.com/oldnewthing/20170927-00/?p=97095 ('How to check if a pointer is in a range of memory')
    template <class Range0_T, class Range1_T>
    bool areContiguousByTypeAndOverlappingRanges(const Range0_T& r0, const Range1_T& r1)
    {
        const auto iterBegin0 = std::begin(r0);
        const auto iterBegin1 = std::begin(r1);
        const auto iterEnd0 = std::end(r0);
        const auto iterEnd1 = std::end(r1);
        return areContiguousByTypeAndOverlappingRangesImpl(makeRange(iterBegin0, iterEnd0), makeRange(iterBegin1, iterEnd1));
    }
} // namespace DFG_DETAIL_NS


// Shrinking replace, i.e. replacing sequence is shorter.
// @param cont Iterable from which subarrays are replaced and that has erase() member function that can be used to erase tail.
// @param oldSub Iterable that represents subarray to replace in 'cont'. Note: must not overlap with 'cont'
// @param newSub Iterable that represents subarray that is to replace 'oldSub'. Note: must not overlap with modifiable part of 'cont'
// Precondition: std::size(newSub) < std::size(oldSub)
// Precondition: 'oldSub' and 'newSub' must not overlap with modifiable part of 'cont'
//     -In practice this means 
//          -'oldSub' must not overlap with 'cont'
//          -'newSub' can overlap with 'cont' only if it is before first occurrence of 'oldSub' and doesn't overlap with it
//          -'oldSub' and 'newSub' can overlap
// Algorithmic properties:
//      Time:
//          Search : At most O(N * S) (N = size(cont), S = size(oldSub))
//          Replace: At most O(R * K) (R = replace count <= floor(N / S), K = size(newSub))
//      Space: replace is guaranteed to be done in-place. Searching may use helper buffer (depending on implementation of std::search()) and memory characteristics on erase() depend on container.
template <class Cont_T, class OldSeq_T, class NewSeq_T>
size_t replaceSubarrays_shrinking(Cont_T& cont, const OldSeq_T& oldSub, const NewSeq_T& newSub)
{
    DFG_MAYBE_UNUSED const auto nOrigSize = std::size(cont);
    const auto nOldSubSize = std::size(oldSub);
    const auto nNewSubSize = std::size(newSub);
    if (nNewSubSize >= nOldSubSize)
    {
        DFG_ASSERT_INVALID_ARGUMENT(true, "New sub size is not smaller for shrinking implementation");
        return 0;
    }
    DFG_ASSERT_UB(!DFG_DETAIL_NS::arePtrsAsIntegersLinearlyComparable() || !DFG_DETAIL_NS::areContiguousByTypeAndOverlappingRanges(cont, oldSub));

    const auto iterOldSubBegin = std::begin(oldSub);
    const auto iterOldSubEnd = std::end(oldSub);
    const auto iterNewSubBegin = std::begin(newSub);
    const auto iterNewSubEnd = std::end(newSub);
    const auto iterBegin = std::begin(cont);
    const auto iterEnd = std::end(cont);
    auto iterReadPos = iterBegin;
    auto iterWritePos = iterBegin;
    size_t nMatchCount = 0;
    for (; iterReadPos != iterEnd;)
    {
        auto iterNextMatch = std::search(iterReadPos, iterEnd, iterOldSubBegin, iterOldSubEnd);
        const bool bMatchFound = (iterNextMatch != iterEnd);
        if (bMatchFound)
            ++nMatchCount;
        const auto nCopyCount = std::distance(iterReadPos, iterNextMatch);
        // Moving elements until found subarray start (only needed if read and write pos differ).
        if (iterWritePos != iterReadPos && nCopyCount > 0)
        {
            auto iterReadEnd = iterReadPos;
            std::advance(iterReadEnd, nCopyCount);
            std::move(iterReadPos, iterReadEnd, iterWritePos);
        }
        std::advance(iterWritePos, nCopyCount);
        // Replacing matching subarray with newSub
        if (bMatchFound)
        {
            std::copy(iterNewSubBegin, iterNewSubEnd, iterWritePos);
            std::advance(iterWritePos, nNewSubSize);
            iterReadPos = iterNextMatch;
            std::advance(iterReadPos, nOldSubSize);
        }
        else
            iterReadPos = iterNextMatch;
    }
    cont.erase(iterWritePos, cont.end());
    DFG_ASSERT_CORRECTNESS(cont.size() == nOrigSize - nMatchCount * (nOldSubSize - nNewSubSize));
    return nMatchCount;
}

// Size-preserving replace, i.e. replacing sequence has the same length as to-be-replaced sequence.
// @param cont Iterable from which subarrays are replaced.
// @param oldSub Iterable that represents subarray to replace in 'cont'. Note: must not overlap with 'cont'
// @param newSub Iterable that represents subarray that is to replace 'oldSub'. Note: must not overlap with modifiable part of 'cont'
// Precondition: std::size(oldSub) == std::size(newSub)
// Precondition: 'oldSub' and 'newSub' must not overlap with modifiable part of 'cont'
// @note: unlike size changing replaces, this function can be used with ranges that don't have resize-capability (e.g. std::array)
// Algorithmic properties:
//      Time:
//          Search : At most O(N * S) (N = size(cont), S = size(oldSub))
//          Replace: At most O(R * K) (R = replace count <= floor(N / S), K = size(newSub))
//      Space: replace is guaranteed to be done in-place. Searching may use helper buffer (depending on implementation of std::search()).
template <class Cont_T, class OldSeq_T, class NewSeq_T>
size_t replaceSubarrays_sizeEqual(Cont_T& cont, const OldSeq_T& oldSub, const NewSeq_T& newSub)
{
    const auto nOldSubSize = std::size(oldSub);
    const auto nNewSubSize = std::size(newSub);
    if (nNewSubSize != nOldSubSize)
    {
        DFG_ASSERT_INVALID_ARGUMENT(true, "Sub sizes are not identical");
        return 0;
    }
    DFG_ASSERT_UB(!DFG_DETAIL_NS::arePtrsAsIntegersLinearlyComparable() || !DFG_DETAIL_NS::areContiguousByTypeAndOverlappingRanges(cont, oldSub));

    auto iter = std::begin(cont);
    const auto iterEnd = std::end(cont);
    const auto iterNewSubBegin = std::begin(newSub);
    const auto iterNewSubEnd = std::end(newSub);
    const auto iterOldSubBegin = std::begin(oldSub);
    const auto iterOldSubEnd = std::end(oldSub);
    size_t nMatchCount = 0;
    while (iter != iterEnd)
    {
        iter = std::search(iter, iterEnd, iterOldSubBegin, iterOldSubEnd);
        if (iter != iterEnd)
        {
            std::copy(iterNewSubBegin, iterNewSubEnd, iter);
            ++nMatchCount;
            std::advance(iter, nOldSubSize);
        }
    }
    return nMatchCount;
}

// Expanding replace, i.e. replacing sequence is longer.
// @param cont Iterable from which subarrays are replaced and that has resize() and swap() member functions.
// @param oldSub Iterable that represents subarray to replace in 'cont'.
// @param newSub Iterable that represents subarray that is to replace 'oldSub'.
// @param pHelperCont If given, all temporary writing will be done to this container and it will be swap()'ed with 'cont' when the result is ready.
//                    Thus if matches were found, on return it will hold original contents of 'cont'.
// Precondition: std::size(newSub) > std::size(oldSub)
// Precondition: If 'pHelperCont' is not given, 'oldSub' and 'newSub' must not overlap with 'cont'
//      -Currently iterables can overlap even if not given since helper container is always used, but this is an implementation detail
//       that can change.
// Algorithmic properties:
//      Time:
//          Search : At most O(N * S) (N = size(cont), S = size(oldSub))
//          Replace: At most O(R * K) (R = replace count <= floor(N / S), K = size(newSub))
//      Space: In addition to possible space allocated by std::search(), allocates memory at most for helper container big enough to contain result. If sufficiently large 'pHelperCont'
//             is given, guarantees to not allocate for helper container if resize() and swap() behave like in typical std-containers.
template <class Cont_T, class OldSeq_T, class NewSeq_T>
size_t replaceSubarrays_expanding(Cont_T& cont, const OldSeq_T& oldSub, const NewSeq_T& newSub, Cont_T* pHelperCont = nullptr)
{
    const auto nOrigSize = std::size(cont);
    const auto nOldSubSize = std::size(oldSub);
    const auto nNewSubSize = std::size(newSub);
    if (nNewSubSize <= nOldSubSize)
    {
        DFG_ASSERT_INVALID_ARGUMENT(true, "New sub size is not bigger for expanding implementation");
        return 0;
    }
    DFG_ASSERT_UB(!DFG_DETAIL_NS::arePtrsAsIntegersLinearlyComparable() || !DFG_DETAIL_NS::areContiguousByTypeAndOverlappingRanges(cont, oldSub));

    // First counting matches so that can allocate result container with single allocation
    size_t nMatchCount = 0;
    const auto iterBegin = std::begin(cont);
    const auto iterEnd = std::end(cont);
    const auto iterNewSubBegin = std::begin(newSub);
    const auto iterNewSubEnd = std::end(newSub);
    const auto iterOldSubBegin = std::begin(oldSub);
    const auto iterOldSubEnd = std::end(oldSub);
    for (auto iter = iterBegin; iter != iterEnd;)
    {
        iter = std::search(iter, iterEnd, iterOldSubBegin, iterOldSubEnd);
        if (iter != iterEnd)
        {
            nMatchCount++;
            std::advance(iter, nOldSubSize);
        }
    }
    if (nMatchCount == 0)
        return 0; // No subarray found -> nothing to do
    const size_t nNewSize = nOrigSize + nMatchCount * (nNewSubSize - nOldSubSize);

    // Below is mostly the same algorithm to shrinking case: has not been refactored into common code since 
    // optimal expanding case might not need to use temporary container but instead resize original and start
    // filling from back. Also with containers such as std::list where insert to middle is cheap, this implementation is not optimal.
    auto iterReadPos = iterBegin;
    Cont_T contHelper;
    auto& newCont = (pHelperCont) ? *pHelperCont : contHelper;
    newCont.resize(nNewSize);
    auto iterWritePos = newCont.begin();
    for (; iterReadPos != iterEnd;)
    {
        auto iterNextMatch = std::search(iterReadPos, iterEnd, iterOldSubBegin, iterOldSubEnd);
        const bool bMatchFound = (iterNextMatch != iterEnd);
        const auto nCopyCount = std::distance(iterReadPos, iterNextMatch);
        // Copying elements until found subarray start.
        if (nCopyCount > 0)
        {
            auto iterReadEnd = iterReadPos;
            std::advance(iterReadEnd, nCopyCount);
            std::copy(iterReadPos, iterReadEnd, iterWritePos);
            std::advance(iterWritePos, nCopyCount);
        }
        // Replacing matching subarray with newSub
        if (bMatchFound)
        {
            std::copy(iterNewSubBegin, iterNewSubEnd, iterWritePos);
            std::advance(iterWritePos, nNewSubSize);
            iterReadPos = iterNextMatch;
            std::advance(iterReadPos, nOldSubSize);
        }
        else
            iterReadPos = iterNextMatch;
    }
    cont.swap(newCont);
    DFG_ASSERT_CORRECTNESS(cont.size() == nNewSize);
    return nMatchCount;
}


namespace DFG_DETAIL_NS
{
    template <class Cont_T, class OldSeq_T, class NewSeq_T>
    size_t replaceSubarraysImpl(Cont_T& cont, const OldSeq_T& oldSeq, const NewSeq_T& newSeq)
    {
        const auto nOldSubSize = std::size(oldSeq);
        const auto nNewSubSize = std::size(newSeq);
        if (nNewSubSize < nOldSubSize)
            return replaceSubarrays_shrinking(cont, oldSeq, newSeq);
        else if (nNewSubSize == nOldSubSize)
            return replaceSubarrays_sizeEqual(cont, oldSeq, newSeq);
        else
            return replaceSubarrays_expanding(cont, oldSeq, newSeq);
    }
}

// Generalisation of dfg::str::replaceSubStrsInplace() replacing any matching subarray within a
// sequence container
// Given a sequence A [a0, a1...], range B [b0, b1..] and replacing range C [c0, c1...],
// replaces every subarray in A matching range B by range C using forward search. 
// For example if A = "aaa", B = "aa" and C = "c", result is guaranteed to be "ca"
// Precondition: conditions are different between shrinking, size preserving and expanding replaces, refer to implementations for details
// @note Since this is essentially a proxy for choosing correct branch from shrinking/size-preserving/expanding cases,
//       'cont' must satisfy member functions requirements in all of those.
// @note Term 'subarray' is used here to mean simply index-wise contiguous subsequence, none of the iterables need to be an array in memory layout -sense
// @return Number of occurences replaced
template <class Cont_T, class OldSeq_T, class NewSeq_T>
size_t replaceSubarrays(Cont_T& cont, const OldSeq_T& oldSeq, const NewSeq_T& newSeq)
{
    return DFG_DETAIL_NS::replaceSubarraysImpl(cont, oldSeq, newSeq);
}

// Boilerplate overload to allow usage of initializer lists as oldSeq/newSeq
template <class Cont_T, class T, class NewSeq_T>
size_t replaceSubarrays(Cont_T& cont, const std::initializer_list<T>& oldSeq, const NewSeq_T& newSeq)
{
    return DFG_DETAIL_NS::replaceSubarraysImpl(cont, oldSeq, newSeq);
}

// Boilerplate overload to allow usage of initializer lists as oldSeq/newSeq
template <class Cont_T, class OldSeq_T, class T>
size_t replaceSubarrays(Cont_T& cont, const OldSeq_T& oldSeq, const std::initializer_list<T>& newSeq)
{
    return DFG_DETAIL_NS::replaceSubarraysImpl(cont, oldSeq, newSeq);
}

// Boilerplate overload to allow usage of initializer lists as oldSeq/newSeq
template <class Cont_T, class T>
size_t replaceSubarrays(Cont_T& cont, const std::initializer_list<T>& oldSeq, const std::initializer_list<T>& newSeq)
{
    return DFG_DETAIL_NS::replaceSubarraysImpl(cont, oldSeq, newSeq);
}

} } // namespace dfg::alg
