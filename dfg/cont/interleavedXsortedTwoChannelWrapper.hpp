#pragma once

#include "../dfgDefs.hpp"
#include "../dfgBase.hpp"
#include "../iter/interleavedIterator.hpp"
#include "../math/interpolationLinear.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(cont) {

// TODO: test
template <class Data_T>
class DFG_CLASS_NAME(InterleavedXsortedTwoChannelWrapper)
{
public:
	// [in] pInterleavedData: Pointer to externally owned data. The data must be
	//						   sorted in ascending order with respect to first channel.
	// [in] nTotalElementCount: Total number of elements in 'pInterleavedData'; must be even.
	DFG_CLASS_NAME(InterleavedXsortedTwoChannelWrapper)(Data_T* pInterleavedData, size_t nTotalElementCount) :
		m_pData(pInterleavedData),
		m_nDataElementCount(nTotalElementCount)
	{
		for(size_t i = 2; i<m_nDataElementCount; i+=2)
			DFG_ASSERT_UB(m_pData[i] >= m_pData[i-2]);
	}

	Data_T operator()(const Data_T& x) const
	{
		if (x <= getXMin())
			return getFirstY(0);
		else if (x >= getXMax())
			return getLastY();
		DFG_SUB_NS_NAME(iter)::DFG_CLASS_NAME(InterleavedSemiIterator)<Data_T> iterBegin(m_pData, 2);
		const DFG_SUB_NS_NAME(iter)::DFG_CLASS_NAME(InterleavedSemiIterator)<Data_T> iterEnd(m_pData + m_nDataElementCount, 2);
		auto iter = std::lower_bound(iterBegin, iterEnd, x);
		if (iter != iterEnd && iter != iterBegin)
		{
			auto iterPrev = iter-1;
			return DFG_SUB_NS_NAME(math)::interpolationLinear(x, iterPrev.gtChannel(0), iterPrev.getChannel(1), iter.getChannel(0), iter.getChannel(1));
		}
		return (iter != iterEnd) ? iter.getChannel(1) : std::numeric_limits<Data_T>::quiet_NaN();
	}

	Data_T getXMin() const
	{
		return (m_nDataElementCount > 0) ? m_pData[0] : std::numeric_limits<Data_T>::quiet_NaN();
	}

	Data_T getXMax() const
	{
		return (m_nDataElementCount >= 1) ? m_pData[m_nDataElementCount-2] : std::numeric_limits<Data_T>::quiet_NaN();
	}

	Data_T getFirstY() const
	{
	    return m_pData[0];
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
	Data_T* m_pData;
	size_t m_nDataElementCount;
};

}} // module cont
