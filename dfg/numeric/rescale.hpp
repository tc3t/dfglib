#pragma once

#include "../dfgDefs.hpp"
#include <algorithm>
#include <type_traits>
#include "../ptrToContiguousMemory.hpp"
#include "vectorizingLoop.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(numeric) {

namespace DFG_DETAIL_NS
{
    template <class Iterable_T, class Value_T, class New_T, class Scale_T>
    void rescaleImpl(Iterable_T& iterable, New_T minEnd, Value_T oldMin, Scale_T scaleFactor, std::false_type)
    {
        std::transform(std::begin(iterable), std::end(iterable), std::begin(iterable), [=](const Value_T oldVal)
        {
            return (minEnd + (oldVal - oldMin) * scaleFactor);
        });
    }

    template <class Iterable_T, class Value_T, class New_T, class Scale_T>
    void rescaleImpl(Iterable_T& iterable, New_T minEnd, Value_T oldMin, Scale_T scaleFactor, std::true_type)
    {
        auto p = ::DFG_ROOT_NS::ptrToContiguousMemory(iterable);
        const auto nSize = count(iterable);
        DFG_ZIMPL_VECTORIZING_LOOP(p, nSize, = minEnd + (p[i] - oldMin) * scaleFactor);
    }
}

// For given set of values in range [min_element, max_element], map them so that
// minimum values map to newLimit0, maximum values map to newLimit1 and points in the middle linearly.
// If either range has length of zero, new values will be newLimit0.
// Behaviour is undefined if source or destination range limits contain NaN values.
template <class Iterable_T, class T>
void rescale(Iterable_T& iterable, const T& newLimit0, const T& newLimit1)
{
    const auto iterBegin = std::begin(iterable);
    const auto iterEnd = std::end(iterable);
    auto minMaxPair = std::minmax_element(iterBegin, iterEnd);
    if (minMaxPair.first == iterEnd || minMaxPair.second == iterEnd)
        return;
    const auto oldMin = *minMaxPair.first;
    typedef typename std::remove_const<decltype(oldMin)>::type ValueType;
    DFG_STATIC_ASSERT(std::is_floating_point<ValueType>::value, "Only floating point types are supported in " DFG_CURRENT_FUNCTION_NAME);
    const auto oldMax = *minMaxPair.second;
    const auto oldRange = oldMax - oldMin;
    const auto minEnd = newLimit0;
    const auto maxEnd = newLimit1;
    const auto newRange = maxEnd - minEnd;
    const auto scaleFactor = (oldRange > 0) ? newRange / oldRange : 0;

    if (scaleFactor != 0)
    {
        DFG_DETAIL_NS::rescaleImpl(iterable,
            ValueType(minEnd),
            oldMin,
            scaleFactor,
            std::integral_constant<bool, std::is_pointer<typename RangeToBeginPtrOrIter<Iterable_T>::type>::value>()
            );
    }
    else
        std::fill(iterBegin, iterEnd, ValueType(minEnd));
}

} } // module namespace
