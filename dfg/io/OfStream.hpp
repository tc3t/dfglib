#pragma once

#include "../dfgDefs.hpp"
#include "../readOnlyParamStr.hpp"
#include "OmcStreamWithEncoding.hpp"
#include "textEncodingTypes.hpp"
#include "openOfStream.hpp"
#include "../utf/utfBom.hpp"
#include <fstream>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    class DFG_CLASS_NAME(OfStreamBufferWithEncoding) : public std::basic_streambuf<char>
    //==============================================
    {
    public:
        typedef std::basic_streambuf<char> BaseClass;
        typedef BaseClass::int_type int_type;
        typedef BaseClass::off_type off_type;
        typedef BaseClass::pos_type pos_type;

        DFG_CLASS_NAME(OfStreamBufferWithEncoding)() :
            m_encodingBuffer(nullptr, encodingUnknown)
        {
        }

        template <class Char_T>
        DFG_CLASS_NAME(OfStreamBufferWithEncoding)(const DFG_CLASS_NAME(ReadOnlyParamStr)<Char_T>& sPath, TextEncoding encoding) :
            m_encodingBuffer(nullptr, encoding)
        {
            open(sPath, std::ios_base::binary | std::ios_base::out);
        }

        void close()
        {
            m_strmBuf.close();
        }

        template <class Char_T>
        std::basic_filebuf<char>* open(const DFG_CLASS_NAME(ReadOnlyParamStr)<Char_T>& sPath, std::ios_base::openmode openMode)
        {
            auto rv = openOfStream(&m_strmBuf, sPath, openMode);
            writeBom(m_encodingBuffer.encoding());
            return rv;
        }

        void writeBom(TextEncoding encoding)
        {
            const auto bomBytes = DFG_MODULE_NS(utf)::encodingToBom(encoding);
            m_strmBuf.sputn(bomBytes.data(), bomBytes.size());
        }

        bool is_open() const
        {
            return m_strmBuf.is_open();
        }

        int_type overflow(int_type byte) override
        {
            const auto rv = m_encodingBuffer.overflow(byte);
            if (rv != EOF)
            {
                m_strmBuf.sputn(m_encodingBuffer.data(), m_encodingBuffer.size());
            }
            m_encodingBuffer.pubseekpos(0);
            m_encodingBuffer.container().clear();
            return rv;
        }

        std::streamsize xsputn(const char* s, std::streamsize num) override
        {
            for (auto i = num; i > 0; --i, ++s)
                overflow(*s);
            return num;
        }

        int sync() override
        {
            return m_strmBuf.pubsync();
        }

        // Writes bytes directly skipping encoding.
        std::streamsize writeBytes(const char* p, const size_t nCount)
        {
            return m_strmBuf.sputn(p, nCount);
        }

        TextEncoding encoding() const { return m_encodingBuffer.encoding(); }

        std::basic_filebuf<char> m_strmBuf;
        DFG_CLASS_NAME(OmcStreamBufferWithEncoding)<std::string> m_encodingBuffer;
    }; // class OfStreamBufferWithEncoding

    class DFG_CLASS_NAME(OfStreamWithEncoding) : public std::ostream
    //========================================
    {
    public:
        typedef std::ostream BaseClass;

        DFG_CLASS_NAME(OfStreamWithEncoding)() : 
            BaseClass(&m_streamBuffer)
        {
        }

        DFG_CLASS_NAME(OfStreamWithEncoding)(const DFG_CLASS_NAME(ReadOnlyParamStrC)& sPath, TextEncoding encoding) :
            BaseClass(&m_streamBuffer),
            m_streamBuffer(sPath, encoding)
        {
        }

        DFG_CLASS_NAME(OfStreamWithEncoding)(const DFG_CLASS_NAME(ReadOnlyParamStrW)& sPath, TextEncoding encoding) :
            BaseClass(&m_streamBuffer),
            m_streamBuffer(sPath, encoding)
        {
        }

        void close()
        {
            m_streamBuffer.close();
        }

        void open(const DFG_CLASS_NAME(ReadOnlyParamStrC)& sPath, std::ios_base::openmode openMode = std::ios_base::out | std::ios_base::binary)
        {
            auto p = m_streamBuffer.open(sPath, openMode);
            if (p == nullptr)
                setstate(std::ios_base::failbit);
        }

        void open(const DFG_CLASS_NAME(ReadOnlyParamStrW)& sPath, std::ios_base::openmode openMode = std::ios_base::out | std::ios_base::binary)
        {
            auto p = m_streamBuffer.open(sPath, std::ios_base::out | openMode);
            if (p == nullptr)
                setstate(std::ios_base::failbit);
        }

        bool is_open() const
        {
            return m_streamBuffer.is_open();
        }

        TextEncoding encoding() const { return m_streamBuffer.encoding(); }

        std::streamsize writeBytes(const char* psz, const size_t nCount) { return m_streamBuffer.writeBytes(psz, nCount); }

        DFG_CLASS_NAME(OfStreamBufferWithEncoding) m_streamBuffer;
    };

} } // module namespace
