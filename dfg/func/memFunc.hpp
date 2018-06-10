#pragma once

#include "../dfgDefs.hpp"
#include "../dfgBase.hpp"
#include <limits>

// NOTE: Taking a look at boost::accumulators is recommended before intending to use these.

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(func) {

namespace DFG_DETAIL_NS
{
    template <class T> inline T initialValueForMaxImpl(std::true_type) {return -std::numeric_limits<T>::infinity();}
    template <class T> inline T initialValueForMaxImpl(std::false_type) {return std::numeric_limits<T>::lowest();}

    template <class T> inline T initialValueForMinImpl(std::true_type) {return std::numeric_limits<T>::infinity();}
    template <class T> inline T initialValueForMinImpl(std::false_type) {return std::numeric_limits<T>::max();}

    template <class T> inline T initialValueForMax()
    {
        return initialValueForMaxImpl<T>(std::integral_constant<bool, std::numeric_limits<T>::has_infinity>());
    }

    template <class T> inline T initialValueForMin()
    {
        return initialValueForMinImpl<T>(std::integral_constant<bool, std::numeric_limits<T>::has_infinity>());
    }
} // namespace DFG_DETAIL_NS

/** Functor that remembers the maximum value of it's call parameters.
* Example: maximum value of N rand calls.
* DFG_CLASS_NAME(MemFuncMax)<double> mfMax;
* for(int i = 0; i<N; ++i)
*	mfMax(rand());
* auto maxVal = mfMax.value();
*/
// TODO: Test
template <class DataT> class DFG_CLASS_NAME(MemFuncMax)
{
public:
    DFG_CLASS_NAME(MemFuncMax)() : m_max(dfDetail::initialValueForMax<DataT>())
    {}
    DFG_CLASS_NAME(MemFuncMax)(const DataT& initValue) : m_max(initValue)
    {}
    void operator()(const DataT& val)
    {
        m_max = ::DFG_ROOT_NS::Max(m_max, val);
    }
    const DataT& value() const {return m_max;}

    DataT m_max;
};

// Functor that remembers the minimum value of it's call parameters.
// TODO: Test
template <class DataT> struct DFG_CLASS_NAME(MemFuncMin)
{
    DFG_CLASS_NAME(MemFuncMin)() : m_min(dfDetail::initialValueForMin<DataT>())
    {}
    DFG_CLASS_NAME(MemFuncMin)(const DataT& initValue) : m_min(initValue)
    {}
    void operator()(const DataT& val)
    {
        m_min = ::DFG_ROOT_NS::Min(m_min, val);
    }
    const DataT& value() const {return m_min;}
    DataT m_min;
};

// Functor that remembers the minimum and maximum value of it's call parameters.
// TODO: Test
template <class DataT> struct DFG_CLASS_NAME(MemFuncMinMax)
{
    DFG_CLASS_NAME(MemFuncMinMax)()
    {}
    DFG_CLASS_NAME(MemFuncMinMax)(const DataT& initValueMin, const DataT& initValueMax) :
        m_mfMin(initValueMin),
        m_mfMax(initValueMax)
    {}

    void operator()(const DataT& val)
    {
        m_mfMin(val);
        m_mfMax(val);
    }

    // Returns the difference max - min
    DataT diff() const {return maxValue() - minValue();}
    DataT minValue() const {return m_mfMin.value();}
    DataT maxValue() const {return m_mfMax.value();}
    bool isValid() const { return maxValue() >= minValue(); } // Returns maxValue() >= minValue(), which should be false if and only if no operator() has been called.

    DFG_CLASS_NAME(MemFuncMin)<DataT> m_mfMin;
    DFG_CLASS_NAME(MemFuncMax)<DataT> m_mfMax;
};

// Functor that remembers the sum of it's call parameters.
// TODO: Test
template <class DataT, class SumT = DataT> struct DFG_CLASS_NAME(MemFuncSum)
{
    DFG_CLASS_NAME(MemFuncSum)(const SumT& initValue = 0) : m_sum(initValue)
    {}
    void operator()(const DataT& val)
    {
        m_sum += val;
    }
    const SumT& value() const {return m_sum;}

    SumT m_sum;
};

// Functor that remembers the sum of it's call parameters squared.
// TODO: Test
template <class DataT, class SumT = DataT> struct DFG_CLASS_NAME(MemFuncSquareSum)
{
    DFG_CLASS_NAME(MemFuncSquareSum)(const SumT& initValue = 0) : m_sum(initValue)
    {}
    void operator()(const DataT& val)
    {
        m_sum += val * val;
    }
    const SumT& sum() const {return m_sum;}

    SumT m_sum;
};

// Functor that remembers the sum of it's call parameters and the number of calls made.
// TODO: Test
template <class DataT, class SumT = DataT, class CountT = size_t> struct DFG_CLASS_NAME(MemFuncAvg)
{
    DFG_CLASS_NAME(MemFuncAvg)(const SumT& initValue = 0) : m_mfSum(initValue), m_nCalls(0)
    {}

    void operator()(const DataT& val)
    {
        feedDataPoint(val);
    }
    void feedDataPoint(const DataT& val)
    {
        m_mfSum(val);
        m_nCalls += 1;
    }
    void clear()
    {
        *this = DFG_CLASS_NAME(MemFuncAvg)();
    }

    static SumT privAverageValueForEmptyByHasNan(const std::true_type) { return std::numeric_limits<SumT>::quiet_NaN(); }
    static SumT privAverageValueForEmptyByHasNan(const std::false_type) { return SumT(); }

    static SumT averageValueForEmpty()
    {
        return privAverageValueForEmptyByHasNan(std::integral_constant<bool, std::numeric_limits<SumT>::has_quiet_NaN>());
    }

    SumT sum() const {return m_mfSum.value();}
    // If SumT has quiet_NaN, returns quiet_NaN, else returns SumT().
    SumT average() const {return (m_nCalls != 0) ? sum() / (SumT)m_nCalls : averageValueForEmpty();}
    CountT callCount() const {return m_nCalls;}

private:
    DFG_CLASS_NAME(MemFuncSum)<DataT, SumT> m_mfSum;
    CountT m_nCalls;
};

}} // module func
