#pragma once

#include "../dfgDefs.hpp"
#include "../readOnlyParamStr.hpp"
#include "OmcStreamWithEncoding.hpp"
#include "textEncodingTypes.hpp"
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

        DFG_CLASS_NAME(OfStreamBufferWithEncoding)(const DFG_CLASS_NAME(ReadOnlyParamStrC)& sPath, TextEncoding encoding) :
            m_encodingBuffer(nullptr, encoding)
        {
            m_strmBuf.open(sPath.c_str(), std::ios_base::binary | std::ios_base::out);
            const auto bomBytes = DFG_MODULE_NS(utf)::encodingToBom(encoding);
            m_strmBuf.sputn(bomBytes.data(), bomBytes.size());
        }

        void close()
        {
            m_strmBuf.close();
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

        void close()
        {
            m_streamBuffer.close();
        }

        DFG_CLASS_NAME(OfStreamBufferWithEncoding) m_streamBuffer;
    };

} } // module namespace
