#pragma once

#include"sortMultiple.hpp"
#include "../numeric/average.hpp"
#include "../cont/elementType.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(alg) {

    // See https://en.wikipedia.org/wiki/Ranking
    enum RankStrategy
    {
        RankStrategyOrdinal, // Integer strategy. Each item is given unique value. In case of equal elements, the order is unspecified.
        RankStrategyFractional, // Floating point strategy. Equal items are given mean of ranks that they would have in ordinal ranking. 
        // NOTE: if adding items, see if areRankStrategyAndElementCompatible needs to be updated.
        RankStrategyDefault = RankStrategyOrdinal
    };

    namespace DFG_DETAIL_NS
    {
        template <class RankElem_T>
        bool areRankStrategyAndRankElementTypeCompatible(const RankStrategy rankStrategy)
        {
            // Note: The comparison order is like this on purpose to avoid "unreferenced formal parameter"-warning that would be generated 
            //       if is_floating_point was first.
            return (rankStrategy != RankStrategyFractional || std::is_floating_point<RankElem_T>::value);
        }
    }

    template <class Offset_T, class Cont_T, class RankCont_T>
    void rankImpl(const Cont_T& src, RankCont_T& dstRanks, const RankStrategy rankStrategy = RankStrategyDefault, const Offset_T indexOffset = 0)
    {
        if (!DFG_DETAIL_NS::areRankStrategyAndRankElementTypeCompatible<typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<decltype(dstRanks)>::type>(rankStrategy))
        {
            dstRanks.resize(0);
            return;
        }

        const auto sortIndexes = computeSortIndexes(src);

        dstRanks.resize(sortIndexes.size());
        for (size_t i = 0, nCount = dstRanks.size(); i < nCount; ++i)
        {
            dstRanks[sortIndexes[i]] = indexOffset + static_cast<Offset_T>(i);
        }

        if (rankStrategy == RankStrategyOrdinal)
            return;
        else if (rankStrategy == RankStrategyFractional)
        {
            for (size_t i = 0, nCount = dstRanks.size(); i < nCount;)
            {
                const auto srcIndex = sortIndexes[i];
                const auto& current = src[srcIndex];
                size_t nEqualEnd = i + 1;
                Offset_T rankSum = static_cast<Offset_T>(i + indexOffset);
                for (; nEqualEnd < nCount; ++nEqualEnd)
                {
                    const auto srcIndex2 = sortIndexes[nEqualEnd];
                    if (!(current == src[srcIndex2]))
                        break;
                    else
                        rankSum += static_cast<Offset_T>(nEqualEnd + indexOffset);
                }

                const auto nEqualCount = nEqualEnd - i;
                if (nEqualCount > 1)
                {
                    const auto avg = rankSum / nEqualCount;
                    for (auto k = i; k < nEqualEnd; ++k)
                        dstRanks[sortIndexes[k]] = static_cast<Offset_T>(avg);
                }

                i = nEqualEnd;
            }
        }
    }

    template <class Cont_T>
    std::vector<double> rank(const Cont_T& src, const RankStrategy rankStrategy = RankStrategyDefault, const double indexOffset = 0)
    {
        std::vector<double> ranks;
        rankImpl(src, ranks, rankStrategy, indexOffset);
        return std::move(ranks);
    }

    // Template version using user defined index type. Note that if rank strategy needs floating point elements but Index_T is integer, returned vector will be empty.
    template <class Index_T, class Cont_T>
    std::vector<Index_T> rankT(const Cont_T& src, const RankStrategy rankStrategy = RankStrategyDefault, const Index_T indexOffset = 0)
    {
        std::vector<Index_T> ranks;
        rankImpl(src, ranks, rankStrategy, indexOffset);
        return std::move(ranks);
    }

} } // module namespace
