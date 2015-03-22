#pragma once

#include "../dfgDefs.hpp"
#include "../math/pow.hpp"
#include "../alg/rank.hpp"
#include <iterator> // std::begin et al
#include <cmath>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(dataAnalysis) {

// Returns the Pearson correlation between two data sets (see http://en.wikipedia.org/wiki/Correlation)
// TODO: Test
template <class Iter1_T, class Iter2_T, class FilterFunc_T>
double correlation(const Iter1_T iter1Begin, const Iter1_T iter1End, const Iter2_T iter2Begin, const Iter2_T iter2End, FilterFunc_T filterFunc)
{
    auto forEachFunc = [&](double& sum, double& sumSquare, size_t& nCount, const size_t nIndex, const double& val)
                        {
                            if (filterFunc(nIndex))
                            {
                                sum += val;
                                sumSquare += val * val;
                                nCount++;
                            }
                        };

    double sum1 = 0;
    double sumSquare1 = 0;
    size_t nCount = 0;
    size_t nIndex = 0;
    for(auto iter = iter1Begin; iter != iter1End; ++iter, ++nIndex)
    {
        forEachFunc(sum1, sumSquare1, nCount, nIndex, *iter);
    }
    double sum2 = 0;
    double sumSquare2 = 0;
    size_t nCount2 = 0;
    nIndex = 0;
    for(auto iter = iter2Begin; iter != iter2End; ++iter, ++nIndex)
    {
        forEachFunc(sum2, sumSquare2, nCount2, nIndex, *iter);
    }
    if (nCount != nCount2)
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double productSum = 0;
    {
        auto iter2 = iter2Begin;
        size_t i = 0;
        for(auto iter = iter1Begin; iter != iter1End; ++iter, ++iter2, ++i)
        {
            if (filterFunc(i))
                productSum += (*iter) * (*iter2);
        }
    }
    const double xAvg = sum1 / nCount;
    const double yAvg = sum2 / nCount;
    const double var1 = sumSquare1 - nCount * DFG_MODULE_NS(math)::pow2(xAvg);
    const double var2 = sumSquare2 - nCount * DFG_MODULE_NS(math)::pow2(yAvg);
    return (productSum - nCount * xAvg * yAvg) / std::sqrt(var1 * var2);
}

template <class Cont0_T, class Cont1_T, class FilterFunc_T>
double correlation(const Cont0_T& cont0, const Cont1_T& cont1, const FilterFunc_T filterFunc)
{
    return correlation(std::begin(cont0), std::end(cont0), std::begin(cont1), std::end(cont1), filterFunc);
}

template <class Cont0_T, class Cont1_T>
double correlation(const Cont0_T& cont0, const Cont1_T& cont1)
{
    auto trueFunc = [](const size_t) -> bool {return true;};
    return correlation(cont0, cont1, trueFunc);
}

template <class Cont0_T, class Cont1_T>
double correlationRankSpearman(const Cont0_T& cont0, const Cont1_T& cont1)
{
    const auto indexes0 = DFG_MODULE_NS(alg)::rank(cont0);
    const auto indexes1 = DFG_MODULE_NS(alg)::rank(cont1);

    return correlation(indexes0, indexes1);
}

}} // module dataAnalysis
