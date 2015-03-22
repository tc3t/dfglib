#pragma once

#include "../dfgDefs.hpp"
#include "../readOnlyParamStr.hpp"
#include "IfmmStream.hpp"
#include "ImStreamWithEncoding.hpp"
#include "StreamBufferMem.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    class DFG_CLASS_NAME(IfStreamBufferWithEncoding) : public std::basic_streambuf<char>
    //==============================================
    {
    public:
        typedef std::basic_streambuf<char> BaseClass;
        typedef BaseClass::int_type int_type;
        typedef BaseClass::off_type off_type;
        typedef BaseClass::pos_type pos_type;

        DFG_CLASS_NAME(IfStreamBufferWithEncoding)()
        {
        }

        DFG_CLASS_NAME(IfStreamBufferWithEncoding)(const DFG_CLASS_NAME(ReadOnlyParamStrC)& sPath, TextEncoding encoding)
        {
            // Hack: for now just read it all and use in-memory parsing classes.
            try // open will throw e.g. if file is not found.
            {
                m_memoryMappedFile.open(sPath);
                m_spStreamBufferCont.reset(new DFG_CLASS_NAME(StreamBufferMemWithEncoding)(m_memoryMappedFile.data(), m_memoryMappedFile.size(), encoding));
            }
            catch (std::exception&)
            {
            }
        }

        int_type underflow() override
        {
            return (m_spStreamBufferCont) ? m_spStreamBufferCont->underflow() : EOF;
        }

        int_type uflow() override
        {
            return (m_spStreamBufferCont) ? m_spStreamBufferCont->uflow() : EOF;
        }

        bool is_open() const
        {
            return (m_spStreamBufferCont != nullptr);
        }

        DFG_CLASS_NAME(FileMemoryMapped) m_memoryMappedFile;

        // TODO: This member doesn't need to be heap allocated. Currently done like this to avoid need for reinitialization
        // of stream buffer.
        std::unique_ptr<DFG_CLASS_NAME(StreamBufferMemWithEncoding)> m_spStreamBufferCont;
    }; // class IfStreamBufferWithEncoding

    class DFG_CLASS_NAME(IfStreamWithEncoding) : public std::istream
    //========================================
    {
        typedef std::istream BaseClass;
    public:
        DFG_CLASS_NAME(IfStreamWithEncoding)(const DFG_CLASS_NAME(ReadOnlyParamStrC)& sPath, TextEncoding encoding = encodingUnknown) :
            BaseClass(&m_strmBuffer),
            m_strmBuffer(sPath, encoding)
        {
            if (!m_strmBuffer.is_open())
                setstate(std::ios::failbit);
        }

        DFG_CLASS_NAME(IfStreamBufferWithEncoding) m_strmBuffer;
    };

} } // module namespace