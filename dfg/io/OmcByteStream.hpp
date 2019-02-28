#pragma once

#include "../dfgDefs.hpp"
#include "../dfgBase.hpp"
#include "../dfgAssert.hpp"
#include "../dfgBaseTypedefs.hpp"
#include "../ptrToContiguousMemory.hpp"
#include <ostream>
#include <streambuf>
#include <memory>
#include <vector>
#include "../build/languageFeatureInfo.hpp"
#include "../cont/contAlg.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    // Simple output buffer with following properties:
    //	-Writes either to external buffer or if not given, to internal. Buffer type is template parameter.
    //	-Guarantees contiguous storage and access to non-copying sequence of characters through data() method.
    //		If Cont_T::data() returns null terminated string, then the sequence is also null terminated.
    //		This is the case for example for std::basic_string<T> objects.
    //  -Guarantees non-copying access to underlying data container object.
    //	-Supports releasing the content through move operation.
    // Requirements for Cont_T
    //	-Must provider data() method that provides access to contiguous data.
    template <class Cont_T>
    class DFG_CLASS_NAME(OmcByteStreamBuffer) : public std::basic_streambuf<char>
    //===========================================================================
    {
    public:
        typedef std::basic_streambuf<char> BaseClass;
        typedef typename Cont_T::value_type ElementType;

        // Implements output buffer that writes bytes to any contiguous storage of POD's or equivalent.
        DFG_CLASS_NAME(OmcByteStreamBuffer)(Cont_T* pCont = nullptr) :
            BaseClass(),
            m_pData(pCont),
            m_internalPos(sizeof(ElementType))
        {
            if (m_pData == nullptr)
                m_pData = &m_internalData;
        }

#if DFG_LANGFEAT_MOVABLE_STREAMS
        DFG_CLASS_NAME(OmcByteStreamBuffer)(DFG_CLASS_NAME(OmcByteStreamBuffer) && other)
        {
            m_internalData = std::move(other.m_internalData);
            if (other.m_pData == &other.m_internalData)
                m_pData = &m_internalData;
            else
                m_pData = other.m_pData;
            other.m_pData = &other.m_internalData;
        }
#endif

        char* data()                { return reinterpret_cast<char*>(ptrToContiguousMemory(*m_pData)); }
        const char* data() const    { return reinterpret_cast<const char*>(ptrToContiguousMemory(*m_pData)); }
        
        // Returns content of current buffer by moving and clears the buffer. Does not 
        Cont_T releaseData()
        {
            auto cont = std::move(*m_pData);
            m_pData->clear();
            return std::move(cont);
        }

        // Like releaseData() but also releases reference to external buffer (if such was in use) and will use the internal one after this.
        Cont_T releaseDataAndBuffer()
        {
            auto cont = releaseData();
            m_pData = &m_internalData;
            return std::move(cont);
        }

        void clearBufferWithoutDeallocAndSeekToBegin()
        {
            m_pData->clear();
        }

        size_t size() const { return sizeInBytes(*m_pData); }

        void reserve(const size_t nCount)
        {
            m_pData->reserve(nCount);
        }
        
        bool tryReserve(const size_t nCount)
        {
            return DFG_MODULE_NS(cont)::tryReserve(*m_pData, nCount);
        }

        Cont_T& container() { return *m_pData; }
        const Cont_T& container() const { return *m_pData; }

        std::streamsize writeBytes(const char* p, const size_t nCount)
        {
            for (size_t i = 0; i < nCount; ++i, ++p)
                writeOne(*p);
            return static_cast<std::streamsize>(nCount);
        }

    protected:

        void writeOne(char ch)
        {
            if (m_internalPos >= sizeof(ElementType))
            {
                m_pData->push_back(ElementType());
                m_internalPos = 0;
            }
            char* p = reinterpret_cast<char*>(&m_pData->back());
            p[m_internalPos++] = ch;
        }

        int_type overflow(int_type byte) override
        {
            if (byte < 0 || byte > uint8_max)
                return EOF;
            writeOne(static_cast<char>(byte));
            return byte;
        }

        std::streamsize xsputn(const char* s, std::streamsize num) override
        {
            for (auto i = num; i > 0; --i, ++s)
                writeOne(*s);
            return num;
        }

        std::streampos seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode om) override
        {
            DFG_ASSERT_IMPLEMENTED(false);
            return BaseClass::seekoff(off, dir, om);
        }

        pos_type seekpos(pos_type pos, std::ios_base::openmode om) override
        {
            DFG_ASSERT_IMPLEMENTED(false);
            return BaseClass::seekpos(pos, om);
        }

    protected:

        size_t currentPosInBytes() const
        {
            return (!m_pData->empty()) ? (m_pData->size() - 1) * sizeof(ElementType) + m_internalPos : 0;
        }

        size_t availableBytesInDest() const
        {
            return sizeof(ElementType) - m_internalPos;
        }

        Cont_T* m_pData; // m_pData points either to external resource or internal m_internalData. Must be valid after construction.
        Cont_T m_internalData; // 
        size_t m_internalPos;
    };

    // Output byte stream that writes to given container that has contiguous storage.
    // Note: seeking is not implemented (i.e. seekp() won't work). 
    // Naming Omc: O=Output, mc = memory contiguous
    template <class Cont_T = std::vector<char>, class StreamBuffer_T = DFG_CLASS_NAME(OmcByteStreamBuffer)<Cont_T>>
    class DFG_CLASS_NAME(OmcByteStream) : public std::ostream
    //=======================================================
    {
    public:
        typedef std::ostream BaseClass;

        DFG_CLASS_NAME(OmcByteStream)() :
            BaseClass(&m_streamBuf)
        {
        }
            
        DFG_CLASS_NAME(OmcByteStream)(Cont_T* pDest) :
            BaseClass(&m_streamBuf),
            m_streamBuf(pDest)
        {
        }

        template <class Param0_T>
        DFG_CLASS_NAME(OmcByteStream)(Cont_T* pDest, Param0_T&& par0) :
            BaseClass(&m_streamBuf),
            m_streamBuf(pDest, std::forward<Param0_T>(par0))
        {
        }

#if DFG_LANGFEAT_MOVABLE_STREAMS
        DFG_CLASS_NAME(OmcByteStream)(DFG_CLASS_NAME(OmcByteStream) && other) :
            BaseClass(&m_streamBuf),
            m_streamBuf(std::move(other.m_streamBuf))
        {
        }
#endif

        char* data() { return m_streamBuf.data(); }
        const char* data() const { return m_streamBuf.data(); }

        Cont_T releaseData()                            { return m_streamBuf.releaseData(); }
        Cont_T releaseDataAndBuffer()                   { return m_streamBuf.releaseDataAndBuffer(); }

        void clearBufferWithoutDeallocAndSeekToBegin()  { m_streamBuf.clearBufferWithoutDeallocAndSeekToBegin(); }

        // Guarantees non-copying access to underlying container data.
        const Cont_T& container() const { return m_streamBuf.container(); }

        size_t size() const { return m_streamBuf.size(); }

        std::streamsize writeBytes(const char* p, const size_t nCount)
        {
            return m_streamBuf.writeBytes(p, nCount);
        }

        void reserve(const size_t nCount)
        {
            m_streamBuf.reserve(nCount);
        }

        bool tryReserve(const size_t nCount)
        {
            return m_streamBuf.tryReserve(nCount);
        }

        StreamBuffer_T m_streamBuf;
        
    };

#if DFG_LANGFEAT_MOVABLE_STREAMS
    template <class Cont_T>
    DFG_CLASS_NAME(OmcByteStream)<Cont_T> makeOmcByteStream(Cont_T& dest)
    {
        return DFG_CLASS_NAME(OmcByteStream)<Cont_T>(&dest);
    }
#endif

} }



