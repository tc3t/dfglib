#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "IfmmStream.hpp"
#include "ImStreamWithEncoding.hpp"
#include "StreamBufferMem.hpp"
#include "BasicIfStream.hpp"
#include "fileToByteContainer.hpp"
#include "widePathStrToFstreamFriendlyNonWide.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    // TODO: wchar_t overloads for open.
    class IfStreamBufferWithEncoding : public std::basic_streambuf<char>
    //==============================================
    {
    public:
        typedef std::basic_streambuf<char> BaseClass;
        typedef BaseClass::int_type int_type;
        typedef BaseClass::off_type off_type;
        typedef BaseClass::pos_type pos_type;
        using StreamBufferMem = StreamBufferMemWithEncoding;

        IfStreamBufferWithEncoding()
        {
        }

        IfStreamBufferWithEncoding(const StringViewSzC& sPath, TextEncoding encoding)
        {
            open(sPath, encoding);
        }

        IfStreamBufferWithEncoding(const StringViewSzW& sPath, TextEncoding encoding)
        {
            open(sPath, encoding);
        }

        // "reads characters from the associated input sequence to the get area" (cppreference.com)
        int_type underflow() override
        {
            return (m_spStreamBufferCont) ? m_spStreamBufferCont->underflow() : traits_type::eof();
        }

        // Like underflow, but additionally "... advances the next pointer " (cppreference.com)
        int_type uflow() override
        {
            return (m_spStreamBufferCont) ? m_spStreamBufferCont->uflow() : traits_type::eof();
        }

        std::streampos seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode om) override
        {
            return (m_spStreamBufferCont) ? m_spStreamBufferCont->seekoff(off, dir, om) : std::streampos(-1);
        }

        pos_type seekpos(pos_type pos, std::ios_base::openmode om) override
        {
            return (m_spStreamBufferCont) ? m_spStreamBufferCont->seekpos(pos, om) : pos_type(off_type(-1));
        }

        bool is_open() const
        {
            return (m_spStreamBufferCont != nullptr);
        }

        void close()
        {
            m_spStreamBufferCont.reset();
            m_memoryMappedFile.close();
            m_fallback.clear();
        }

        template <class Char_T>
        void openT(const StringViewSz<Char_T> & sPath, DFG_MODULE_NS(io)::TextEncoding encoding)
        {
            close();
            // Hack: for now just read it all and use in-memory parsing classes.
            bool bRetry = true;
            try // open will throw e.g. if file is not found.
            {
                m_memoryMappedFile.open(sPath.c_str());
                m_spStreamBufferCont.reset(new StreamBufferMem(m_memoryMappedFile.data(), m_memoryMappedFile.size(), encoding));
                bRetry = false;
            }
            catch (std::exception&)
            {
            }
            if (bRetry)
            {
                m_fallback = fileToVector(sPath);
                if (!m_fallback.empty())
                    m_spStreamBufferCont.reset(new StreamBufferMem(m_fallback.data(), m_fallback.size(), encoding));
            }
        }

        void open(const StringViewSzC& sPath, TextEncoding encoding)
        {
            openT(sPath, encoding);
        }

        void open(const StringViewSzW& sPath, TextEncoding encoding)
        {
            // m_memoryMappedFile doesn't like wchar_t, so convert it to char-version.
            openT<char>(widePathStrToFstreamFriendlyNonWide(sPath), encoding);
        }

        TextEncoding encoding() const
        {
            return (m_spStreamBufferCont) ? m_spStreamBufferCont->encoding() : encodingUnknown;
        }

        void setEncoding(TextEncoding encoding)
        {
            if (m_spStreamBufferCont)
                m_spStreamBufferCont->setReaderByEncoding(encoding);
        }

        FileMemoryMapped m_memoryMappedFile;
        std::vector<char> m_fallback; // Hack: Temporary solution for m_memoryMappedFile not being able to open if file is in use but readable. In this case read the file to a vector.

        // TODO: This member doesn't need to be heap allocated. Currently done like this to avoid need for reinitialization
        // of stream buffer.
        std::unique_ptr<StreamBufferMem> m_spStreamBufferCont;
    }; // class IfStreamBufferWithEncoding

    class IfStreamWithEncoding : public std::istream
    //========================================
    {
        typedef std::istream BaseClass;

    public:
        IfStreamWithEncoding() :
            BaseClass(&m_strmBuffer)
        {}

        IfStreamWithEncoding(const StringViewSzC& sPath, TextEncoding encoding = encodingUnknown) :
            BaseClass(&m_strmBuffer),
            m_strmBuffer(sPath, encoding)
        {
            if (!m_strmBuffer.is_open())
                setstate(std::ios::failbit);
        }

        IfStreamWithEncoding(const StringViewSzW& sPath, TextEncoding encoding = encodingUnknown) :
            BaseClass(&m_strmBuffer),
            m_strmBuffer(sPath, encoding)
        {
            if (!m_strmBuffer.is_open())
                setstate(std::ios::failbit);
        }

        bool is_open() const
        {
            return m_strmBuffer.is_open();
        }

        void close()
        {
            m_strmBuffer.close();
        }

        void open(const StringViewSzC& sPath, TextEncoding encoding = encodingUnknown)
        {
            m_strmBuffer.open(sPath, encoding);
        }

        TextEncoding encoding() const
        {
            return m_strmBuffer.encoding();
        }

        void setEncoding(TextEncoding encoding)
        {
            m_strmBuffer.setEncoding(encoding);
        }

        IfStreamBufferWithEncoding m_strmBuffer;
    };

} } // module namespace
