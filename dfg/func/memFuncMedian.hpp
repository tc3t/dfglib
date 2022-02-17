#include "../dfgDefs.hpp"
#include "../dfgBase.hpp"
#include "../dfgAssert.hpp"
#include "../numeric/average.hpp"
#include "memFunc.hpp"
#include <limits>
#include <vector>
#include <queue>
#include <functional>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(func) {

    // Functor that remembers the median of it's call parameters.
    // TODO: test with integer Data_T.
    template <class Data_T, class MedianVal_T = Data_T, class Cont_T = std::vector<Data_T>>
    class MemFuncMedian
    {
    public:
        void operator()(const Data_T& val)
        {
            DFG_ASSERT_CORRECTNESS(m_contGreater.size() >= m_contSmaller.size() && m_contGreater.size() - m_contSmaller.size() <= 1);
            if (m_contGreater.empty())
            {
                DFG_ASSERT_CORRECTNESS(m_contSmaller.empty());
                m_contGreater.push(val);
                return;
            }
            if (m_contSmaller.size() == m_contGreater.size())
            {
                const auto smallerTop = m_contSmaller.top();
                if (smallerTop > val)
                {
                    m_contGreater.push(smallerTop);
                    m_contSmaller.pop();
                    m_contSmaller.push(val);
                }
                else
                    m_contGreater.push(val);
            }
            else // Case m_contGreater.size() > m_contSmaller.size()
            {
                if (val > m_contGreater.top())
                {
                    m_contSmaller.push(m_contGreater.top());
                    m_contGreater.pop();
                    m_contGreater.push(val);
                }
                else // case: val < m_contGreater.top()
                {
                    m_contSmaller.push(val);
                }
            }
        }

        MedianVal_T median() const
        { 
            if (m_contGreater.empty())
                return std::numeric_limits<MedianVal_T>::quiet_NaN();
            else if (m_contGreater.size() > m_contSmaller.size())
                return m_contGreater.top();
            else
                return static_cast<MedianVal_T>(DFG_MODULE_NS(numeric)::average(m_contSmaller.top(), m_contGreater.top()));
        }

        std::priority_queue<Data_T, Cont_T> m_contSmaller; // Top element is greatest
        std::priority_queue<Data_T, Cont_T, std::greater<Data_T>> m_contGreater; // Top element is smallest
    }; // class MemFuncMedian


    // MemFunc-version of percentileInSorted_enclosingElem, i.e. a functor that remembers "enclosingElem" percentile of it's call parameters.
    // TODO: test with integer Data_T.
    template <class Data_T, class Percentile_T = Data_T, class Cont_T = std::vector<Data_T>>
    class MemFuncPercentile_enclosingElem
    {
    public:
        MemFuncPercentile_enclosingElem(const double percentile)
            : m_ratiotile(percentile / 100.0)
        {
        }

        void operator()(const Data_T& val)
        {
            // Always keeping m_contSmaller and m_contGreater so that m_contGreater.top() is the result (if any items have been added)
            //      -As an exception case m_ratiotile >= 1 is treated differently: then the result with non-empty memory is always m_max.

            const auto nOldN = (m_contSmaller.size() + m_contGreater.size());
            const auto nNewN = nOldN + 1u;
            const auto nOrdinalRank = floorToInteger<size_t>(m_ratiotile * nNewN);

            m_max(val);

            if (m_contGreater.empty())
            {
                DFG_ASSERT_CORRECTNESS(m_contSmaller.empty());
                m_contGreater.push(val);
                return;
            }
            if (m_ratiotile >= 1)
                return;

            if (!m_contSmaller.empty() && val < m_contSmaller.top())
                m_contSmaller.push(val);
            else
                m_contGreater.push(val);
            const auto nOrdinalRankOfTopGreatest = m_contSmaller.size();
            if (nOrdinalRankOfTopGreatest < nOrdinalRank)
                moveElementFromTo(m_contGreater, m_contSmaller);
            else if (nOrdinalRankOfTopGreatest > nOrdinalRank)
                moveElementFromTo(m_contSmaller, m_contGreater);
            DFG_ASSERT_CORRECTNESS(m_contSmaller.size() == nOrdinalRank);
        }

        Percentile_T percentile() const
        {
            if (m_contGreater.empty())
                return std::numeric_limits<Percentile_T>::quiet_NaN();
            else if (m_ratiotile >= 1)
                return m_max.value();
            else
                return m_contGreater.top();
        }

    private:
        template <class ContFrom_T, class ContTo_T>
        void moveElementFromTo(ContFrom_T& contFrom, ContTo_T& contTo)
        {
            const auto fromTop = contFrom.top();
            contTo.push(fromTop);
            contFrom.pop();
        }

    public:

        double m_ratiotile; // Percentile divided by 100.
        std::priority_queue<Data_T, Cont_T> m_contSmaller; // Top element is greatest
        std::priority_queue<Data_T, Cont_T, std::greater<Data_T>> m_contGreater; // Top element is smallest
        MemFuncMax<Data_T> m_max;
    };


} } // Module namespace