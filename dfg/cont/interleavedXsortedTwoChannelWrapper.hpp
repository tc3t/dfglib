#pragma once

#include "../dfgDefs.hpp"
#include "../dfgBase.hpp"
#include "../iter/interleavedIterator.hpp"
#include "../math/interpolationLinear.hpp"
#include "../ptrToContiguousMemory.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(cont) {

// TODO: test
template <class Data_T>
class DFG_CLASS_NAME(InterleavedXsortedTwoChannelWrapper)
{
public:
	typedef DFG_MODULE_NS(iter)::DFG_CLASS_NAME(InterleavedSemiIterator)<Data_T> Iterator;
	typedef Iterator SingleChannelIterator;

    // [in] pInterleavedData: Pointer to externally owned data. The data must be
    //						   sorted in ascending order with respect to first channel.
    // [in] nTotalElementCount: Total number of elements in 'pInterleavedData'; must be even.
    explicit DFG_CLASS_NAME(InterleavedXsortedTwoChannelWrapper)(Data_T* pInterleavedData, size_t nTotalElementCount)
    {
		Construct(pInterleavedData, nTotalElementCount);
    }

	template <class Cont_T>
	explicit DFG_CLASS_NAME(InterleavedXsortedTwoChannelWrapper)(Cont_T&& cont)
	{
		Construct(ptrToContiguousMemory(cont), count(cont));
	}

    Data_T operator()(const Data_T& x) const
    {
        if (x <= getXMin())
            return getFirstY();
        else if (x >= getXMax())
            return getLastY();
		Iterator iterBegin(m_pData, 2);
        const Iterator iterEnd(m_pData + m_nDataElementCount, 2);
        auto iter = std::lower_bound(iterBegin, iterEnd, x);
        if (iter != iterEnd && iter != iterBegin)
        {
			if (iter.getChannel(0) == x)
				return iter.getChannel(1);
            auto iterPrev = iter-1;
            return DFG_SUB_NS_NAME(math)::interpolationLinear_X_X0Y0_X1Y1(x, iterPrev.getChannel(0), iterPrev.getChannel(1), iter.getChannel(0), iter.getChannel(1));
        }
        return (iter != iterEnd) ? iter.getChannel(1) : std::numeric_limits<Data_T>::quiet_NaN();
    }

    Data_T getXMin() const
    {
        return (m_nDataElementCount > 0) ? m_pData[0] : std::numeric_limits<Data_T>::quiet_NaN();
    }

    Data_T getXMax() const
    {
        return (m_nDataElementCount >= 2) ? m_pData[m_nDataElementCount-2] : std::numeric_limits<Data_T>::quiet_NaN();
    }

    Data_T getFirstY() const
    {
        return m_pData[1];
    }

    Data_T getLastY() const
    {
        return m_pData[m_nDataElementCount-1];
    }

    const Data_T& getY(size_t nIndex) const
    {
        DFG_ASSERT_UB(nIndex >= 0 && 2*nIndex < m_nDataElementCount);
        return m_pData[nIndex*2 + 1];
    }

private:
	void Construct(Data_T* pInterleavedData, size_t nTotalElementCount)
	{
		m_pData = pInterleavedData;
		m_nDataElementCount = nTotalElementCount;
		SingleChannelIterator iterBegin(m_pData, 2);
		SingleChannelIterator iterEnd(m_pData + m_nDataElementCount, 2);
		DFG_ASSERT(std::is_sorted(iterBegin, iterEnd));
	}

    Data_T* m_pData;
    size_t m_nDataElementCount;
};

}} // module cont
