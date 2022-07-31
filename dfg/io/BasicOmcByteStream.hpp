#pragma once

#include "../dfgDefs.hpp"
#include "../dfgBase.hpp"
#include "../cont/Vector.hpp"
#include "../cont/contAlg.hpp"
#include "../ReadOnlySzParam.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    // A placeholder for non std::ostream-inherited OmcByteStream.
    template <class Cont_T = ::DFG_MODULE_NS(cont)::Vector<char>>
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

        size_t size() const { return sizeInBytes(*m_pEffectiveContainer); }

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

        bool good() const
        {
            return true;
        }

        BasicOmcByteStream& operator<<(const StringViewC& sv)
        { 
            write(sv.data(), sv.size());
            return *this;
        }

         BasicOmcByteStream& operator<<(const char c)
        {
            return operator<<(StringViewC(&c, 1));
        }

        void reserve(const size_t nCount)
        {
            m_pEffectiveContainer->reserve(nCount);
        }

        bool tryReserve(const size_t nCount) 
        {
            return DFG_MODULE_NS(cont)::tryReserve(*m_pEffectiveContainer, nCount);
        }

        size_t capacity() const
        {
            return m_pEffectiveContainer->capacity();
        }

        Cont_T m_internalData;
        Cont_T* m_pEffectiveContainer; // Points to effective container, never null after construction.
    }; // Class BasicOmcByteStream

} } // module namespace
