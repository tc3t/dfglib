#pragma once

#include "../dfgDefs.hpp"
#include "../cont/Vector.hpp"


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    // A placeholder for non std::ostream-inherited OmcByteStream.
    template <class Cont_T = DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Vector)<char>>
    class BasicOmcByteStream
    {
    public:
        BasicOmcByteStream() :
            m_pEffectiveContainer(&m_internalData)
        {
        }

        BasicOmcByteStream(Cont_T* pDest) :
            m_pEffectiveContainer(pDest)
        {
            if (!m_pEffectiveContainer)
                m_pEffectiveContainer = &m_internalData;
        }

        Cont_T releaseData()          { return std::move(m_internalData); }
        Cont_T releaseDataAndBuffer()
        {
            auto cont = releaseData();
            m_pEffectiveContainer = &m_internalData;
            return std::move(cont);
        }

        BasicOmcByteStream& write(const char* p, const std::streamsize nCount)
        {
            m_pEffectiveContainer->insert(m_pEffectiveContainer->end(), p, p + nCount);
            return *this;
        }

        Cont_T m_internalData;
        Cont_T* m_pEffectiveContainer; // Points to effective container, never null after construction.
    }; // Class BasicOmcStream

} } // module namespace
