#pragma once

#include "../dfgDefs.hpp"
#include <iterator>
#include "../cont/elementType.hpp"
#include <algorithm>
#include <type_traits>
#include <limits>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(numeric) {

    // Returns median from given range assuming it is nth_element-ordered with respect to element at index size()/2.
    // In case of even size, the (size()/2-1) element is promised to be 'left'.
    template <class Iterable_T, class Val_T>
    auto medianInNthSorted(const Iterable_T& data, const Val_T left) -> typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Iterable_T>::type
    {
        typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Iterable_T>::type ValueT;
        DFG_STATIC_ASSERT(std::is_floating_point<ValueT>::value, "Current implementation accepts only floating point types");
        const auto iBegin = std::begin(data);
        const auto iEnd = std::end(data);
        if (iBegin == iEnd)
            return std::numeric_limits<ValueT>::quiet_NaN();
        const auto n = std::distance(iBegin, iEnd);
        const auto iTarget = iBegin + n / 2;
        if (n % 2 == 1)
            return *iTarget;
        else // Case: even number of elements. Return the average of middle elements.
            return (left + *iTarget) / 2;
    }

    // Returns median from given range assuming it is nth_element-ordered with respect to element at index size()/2.
    template <class Iterable_T>
    auto medianInNthSorted(const Iterable_T& data) -> typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Iterable_T>::type
    {
        typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Iterable_T>::type ValueT;
        const auto iBegin = std::begin(data);
        const auto iEnd = std::end(data);
        const auto n = std::distance(iBegin, iEnd);
        const auto iTarget = iBegin + n / 2;
        if (n % 2 == 0 && iBegin != iTarget)
            return medianInNthSorted(data, *std::max_element(iBegin, iTarget));
        else
            return medianInNthSorted(data, std::numeric_limits<ValueT>::quiet_NaN());
    }

    // Returns median in sorted range.
    template <class Iterable_T>
    auto medianInSorted(const Iterable_T& data) -> typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Iterable_T>::type
    {
        const auto iBegin = std::begin(data);
        const auto iEnd = std::end(data);
        const auto n = std::distance(iBegin, iEnd);
        const auto iTarget = iBegin + n / 2;
        if (n % 2 == 0 && iTarget != iBegin)
            return medianInNthSorted(data, *(iTarget - 1));
        else
            return medianInNthSorted(data);
    }

    // Takes modifiable range and is allowed (but not guaranteed) to change the order of elements.
    template <class Iterable_T>
    auto medianModifying(Iterable_T& data) -> typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Iterable_T>::type
    {
        const auto iBegin = std::begin(data);
        const auto iEnd = std::end(data);
        const auto n = std::distance(iBegin, iEnd);
        const auto iTarget = iBegin + n / 2;
        std::nth_element(iBegin, iTarget, iEnd);
        return medianInNthSorted(data);
    }

    // Returns median of given iterable. If given iterable can be modified, consider using medianModifying().
    template <class Iterable_T>
    auto median(const Iterable_T& iterable) -> typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Iterable_T>::type
    {
        typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Iterable_T>::type ValueT;
        std::vector<ValueT> dataCopy(DFG_ROOT_NS::cbegin(iterable), DFG_ROOT_NS::cend(iterable)); // Placeholder implementation hack.
        return medianModifying(dataCopy);
    }

} } // namespace module math
