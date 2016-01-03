#include "../dfgDefs.hpp"
#include "../dfgBase.hpp"
#include "../dfgAssert.hpp"
#include "../numeric/average.hpp"
#include <limits>
#include <vector>
#include <queue>
#include <functional>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(func) {

    // Functor that remembers the median of it's call parameters.
    // TODO: test with integer Data_T.
    template <class Data_T, class MedianVal_T = Data_T, class Cont_T = std::vector<Data_T>>
    struct DFG_CLASS_NAME(MemFuncMedian)
    {
        DFG_CLASS_NAME(MemFuncMedian)()
        {}

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
    };


} } // Module namespace