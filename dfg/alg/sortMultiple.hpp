#pragma once

#include "../dfgDefs.hpp"
#include "../cont/elementtype.hpp"
#include "generateAdjacent.hpp"
#include "../func.hpp"
#include <vector>
#include <algorithm>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(alg) {

    // TODO: add stable sort variants. Or better, make sort algorithm to be an (optional) input parameter.

    // For given iterable, returns a list of indexes that define the order of elements in sortSeq if it was sorted using predicate pred.
    // For example if sortSeq = {2.5, 3.1, 1.2} and pred is 'less than', returned list will be {2, 0, 1} which corresponds to sequence {1.2, 2.5, 3.1}.
    template <class Pred_T>
    std::vector<size_t> computeSortIndexesBySizeAndPred(const size_t nMaxIndex, Pred_T&& pred)
    {
        std::vector<size_t> indexMapNewToOld(nMaxIndex);

        generateAdjacent(indexMapNewToOld, 0, 1);
        std::sort(std::begin(indexMapNewToOld), std::end(indexMapNewToOld), [&](const size_t a, const size_t b)
        {
            return pred(a, b);
        });
        // indexMapNewToOld[i] now tells which index of original source should be at position i in sorted order.
        return indexMapNewToOld;
    }

    // For given iterable, returns a list of indexes that define the order of elements in sortSeq if it was sorted using predicate pred.
    // For example if sortSeq = {2.5, 3.1, 1.2} and pred is 'less than', returned list will be {2, 0, 1} which corresponds to sequence {1.2, 2.5, 3.1}.
    template <class T, class Pred_T>
    std::vector<size_t> computeSortIndexes(const T& sortSeq, Pred_T&& pred)
    {
        return computeSortIndexesBySizeAndPred(count(sortSeq), [&](const size_t a, const size_t b) { return pred(sortSeq[a], sortSeq[b]); });
    }

    // Convenience overload using less-than comparison.
    template <class T>
    std::vector<size_t> computeSortIndexes(const T& sortSeq)
    {
        typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<T>::type ElementT;
        return computeSortIndexes(sortSeq, [](const ElementT& a, const ElementT& b) {return a < b; });
    }

    namespace DFG_DETAIL_NS
    {
        template <class T>
        void DefaultSwapImpl(T& a, T& b)
        {
            // ADL (http://en.cppreference.com/w/cpp/language/adl)
            using std::swap; 
            swap(a, b);
        }

        template <class SwapImpl_T>
        void sortByIndexArray_tN_sN_WithSwapImpl(std::vector<size_t> indexMap, SwapImpl_T swapImpl)
        {
            // indexMap[i] tells which index in seq should be placed as i'th element in sorted order.

            const auto nSize = indexMap.size();

            for (size_t i = 0; i < nSize; ++i)
            {
                auto iSrc = indexMap[i];
                if (iSrc == i)
                    continue;
                auto iDest = i;
                while (iSrc != i)
                {
                    swapImpl(iDest, iSrc);
                    indexMap[iDest] = iDest;
                    iDest = iSrc;
                    iSrc = indexMap[iDest];
                }
                indexMap[iDest] = iDest;
            }

        }

        template <class Arr_T, class SwapImpl_T>
        void sortByIndexArray_tN_sN_WithSwapImpl(Arr_T& arr, std::vector<size_t> indexMap, SwapImpl_T swapImpl)
        {
            sortByIndexArray_tN_sN_WithSwapImpl(std::move(indexMap), [&](const size_t a, const size_t b)
            {
                swapImpl(arr[a], arr[b]);
            });
        }
    }

    // Sorts given array to order given by indexMap. indexMap[i] gives the index of element which should be located at i in the sorted order.
    // i.e. {arr[0], arr[1],..., arr[n]} -> {arr[indexMap[0]], arr[indexMap[1]],..., arr[indexMap[n]]}
    // Precondition: for all indexes i in indexMap, indexMap[i] >= 0 && indexMap[i] < indexMap.size() 
    // Time: O(n)
    // Space: O(n) (needed only for index type)
    // Note: Reordering is done in-place using move so element does not need to be copyable.
    // References:
    //     http://stackoverflow.com/questions/7365814/in-place-array-reordering
    template <class Arr_T>
    void sortByIndexArray_tN_sN(Arr_T& arr, std::vector<size_t> indexMap)
    {
        DFG_DETAIL_NS::sortByIndexArray_tN_sN_WithSwapImpl(arr, std::move(indexMap), &DFG_DETAIL_NS::DefaultSwapImpl<decltype(arr[0])>);
    }

    // Sorts multiple ranges with respect to first.
    // Given parameter should be a container of pointers that point to containers to be sorted with the first defining the sort order.
    // Any element can be nullptr (note though that if first is nullptr, sorting is skipped).
    template <class ContOfIterables_T>
    void sortMultipleInPtrCont(ContOfIterables_T& iterables)
    {
        if (iterables.empty() || *iterables.begin() == nullptr)
            return;

        const auto& sortSeq = **iterables.begin();

        size_t nDataCount = count(sortSeq);

        for (auto iter = iterables.begin(), iterEnd = iterables.end(); iter != iterEnd; ++iter)
        {
            if (*iter && count(**iter) != nDataCount)
                return; // Return if data sets are not of equal size.
        }

        const auto indexMapNewToOld = computeSortIndexes(sortSeq);

        for (auto iter = iterables.begin(), iterEnd = iterables.end(); iter != iterEnd; ++iter)
        {
            if (!*iter)
                continue;

            sortByIndexArray_tN_sN(**iter, indexMapNewToOld);
        }
    }

    // Sorts all items in a tuple by ordering defined by the first item in the tuple.
    // TODO: Currently requires tuple element to have 'reference compatible' types; this shouldn't be required.
    template <class Tuple_T>
    void sortMultipleInTuple(Tuple_T& iterables)
    {
        typedef typename std::tuple_element<0, Tuple_T>::type TupleElement;
        const auto indexMapNewToOld = computeSortIndexes(std::get<0>(iterables));
        DFG_MODULE_NS(func)::forEachInTuple(iterables, [&](TupleElement& elem)
        {
            sortByIndexArray_tN_sN(elem, indexMapNewToOld);
        });
    }

    // Sorts all items in a tuple by ordering defined by the first item in the tuple using given predicate.
    template <class Tuple_T, class Pred_T>
    void sortMultipleInTuple(Tuple_T& iterables, Pred_T&& pred)
    {
        typedef typename std::tuple_element<0, Tuple_T>::type TupleElement;
        const auto indexMapNewToOld = computeSortIndexes(std::get<0>(iterables), std::forward<Pred_T>(pred));
        DFG_MODULE_NS(func)::forEachInTuple(iterables, [&](TupleElement& elem)
        {
            sortByIndexArray_tN_sN(elem, indexMapNewToOld);
        });
    }

    // Convenience overload for two sequences.
    template <class Iterable_T0, class Iterable_T1>
    void sortMultiple(Iterable_T0&& sortSequence, Iterable_T1&& i1)
    {
        auto indexMapNewToOld = computeSortIndexes(sortSequence);
        sortByIndexArray_tN_sN(sortSequence, indexMapNewToOld);
        sortByIndexArray_tN_sN(i1, std::move(indexMapNewToOld));
    }

    // Convenience overload for two sequences with predicate
    template <class Pred_T, class Iterable_T0, class Iterable_T1>
    void sortMultipleWithPred(Pred_T&& pred, Iterable_T0&& sortSequence, Iterable_T1&& i1)
    {
        auto indexMapNewToOld = computeSortIndexes(sortSequence, std::forward<Pred_T>(pred));
        sortByIndexArray_tN_sN(sortSequence, indexMapNewToOld);
        sortByIndexArray_tN_sN(i1, std::move(indexMapNewToOld));
    }

    // Convenience overload for three sequences.
    template <class Iterable_T0, class Iterable_T1, class  Iterable_T2>
    void sortMultiple(Iterable_T0&& sortSequence, Iterable_T1&& i1, Iterable_T2&& i2)
    {
        auto indexMapNewToOld = computeSortIndexes(sortSequence);
        sortByIndexArray_tN_sN(sortSequence, indexMapNewToOld);
        sortByIndexArray_tN_sN(i1, indexMapNewToOld);
        sortByIndexArray_tN_sN(i2, std::move(indexMapNewToOld));
    }

    // Convenience overload for three sequences with predicate.
    template <class Pred_T, class Iterable_T0, class Iterable_T1, class  Iterable_T2>
    void sortMultipleWithPred(Pred_T&& pred, Iterable_T0&& sortSequence, Iterable_T1&& i1, Iterable_T2&& i2)
    {
        auto indexMapNewToOld = computeSortIndexes(sortSequence, std::forward<Pred_T>(pred));
        sortByIndexArray_tN_sN(sortSequence, indexMapNewToOld);
        sortByIndexArray_tN_sN(i1, indexMapNewToOld);
        sortByIndexArray_tN_sN(i2, std::move(indexMapNewToOld));
    }

    // Convenience overload for four sequences.
    template <class Iterable_T0, class Iterable_T1, class  Iterable_T2, class Iterable_T3>
    void sortMultiple(Iterable_T0&& sortSequence, Iterable_T1&& i1, Iterable_T2&& i2, Iterable_T3&& i3)
    {
        auto indexMapNewToOld = computeSortIndexes(sortSequence);
        sortByIndexArray_tN_sN(sortSequence, indexMapNewToOld);
        sortByIndexArray_tN_sN(i1, indexMapNewToOld);
        sortByIndexArray_tN_sN(i2, indexMapNewToOld);
        sortByIndexArray_tN_sN(i3, std::move(indexMapNewToOld));
    }

    // Convenience overload for four sequences with predicate.
    template <class Pred_T, class Iterable_T0, class Iterable_T1, class  Iterable_T2, class Iterable_T3>
    void sortMultipleWithPred(Pred_T&& pred, Iterable_T0&& sortSequence, Iterable_T1&& i1, Iterable_T2&& i2, Iterable_T3&& i3)
    {
        auto indexMapNewToOld = computeSortIndexes(sortSequence, std::forward<Pred_T>(pred));
        sortByIndexArray_tN_sN(sortSequence, indexMapNewToOld);
        sortByIndexArray_tN_sN(i1, indexMapNewToOld);
        sortByIndexArray_tN_sN(i2, indexMapNewToOld);
        sortByIndexArray_tN_sN(i3, std::move(indexMapNewToOld));
    }
} }
