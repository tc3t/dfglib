#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "OmcStreamWithEncoding.hpp"
#include "textEncodingTypes.hpp"
#include "openOfStream.hpp"
#include "../utf/utfBom.hpp"
#include <fstream>
#include "../build/languageFeatureInfo.hpp"
#include "../dfgBase.hpp"
#include "../str.hpp"

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
        DFG_CLASS_NAME(OfStreamBufferWithEncoding)(const DFG_CLASS_NAME(ReadOnlySzParam)<Char_T>& sPath, TextEncoding encoding, const bool bWriteBom = true) :
            m_encodingBuffer(nullptr, encoding)
        {
            open(sPath, std::ios_base::binary | std::ios_base::out, bWriteBom);
        }

        void close()
        {
            m_strmBuf.close();
        }

        template <class Char_T>
        std::basic_filebuf<char>* open(const DFG_CLASS_NAME(ReadOnlySzParam)<Char_T>& sPath, std::ios_base::openmode openMode, const bool bWriteBom = true)
        {
            auto rv = openOfStream(&m_strmBuf, sPath, openMode);
            if (bWriteBom)
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
                privWriteEncodingBufferToStream(false); // false == don't clear buffer (because it's done in line below)
            }
            m_encodingBuffer.clearBufferWithoutDeallocAndSeekToBegin();
            return rv;
        }

        std::streamsize xsputn(const char* s, std::streamsize num) override
        {
            for (auto i = num; i > 0; --i, ++s)
                overflow(std::char_traits<char>::to_int_type(*s));
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

        template <class Iterable_T>
        std::streamsize writeBytes(const Iterable_T& iterable)
        {
            return writeBytes(ptrToContiguousMemory(iterable), count(iterable));
        }

        // Not tested
        std::streamsize writeUnicodeChar(uint32 c)
        {
            const auto rv = m_encodingBuffer.writeUnicodeChar(c);
            privWriteEncodingBufferToStream();
            return rv;
        }

        TextEncoding encoding() const { return m_encodingBuffer.encoding(); }

        void privWriteEncodingBufferToStream(const bool bClearEncodingBuffer = true)
        {
            m_strmBuf.sputn(m_encodingBuffer.data(), m_encodingBuffer.size());
            if (bClearEncodingBuffer)
                m_encodingBuffer.clearBufferWithoutDeallocAndSeekToBegin();
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

        DFG_CLASS_NAME(OfStreamWithEncoding)(const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath, TextEncoding encoding, const bool bWriteBom = true) :
            BaseClass(&m_streamBuffer),
            m_streamBuffer(sPath, encoding, bWriteBom)
        {
        }

        DFG_CLASS_NAME(OfStreamWithEncoding)(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath, TextEncoding encoding, const bool bWriteBom = true) :
            BaseClass(&m_streamBuffer),
            m_streamBuffer(sPath, encoding, bWriteBom)
        {
        }

        void close()
        {
            m_streamBuffer.close();
        }

        void open(const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath, std::ios_base::openmode openMode = std::ios_base::out | std::ios_base::binary)
        {
            auto p = m_streamBuffer.open(sPath, openMode);
            if (p == nullptr)
                setstate(std::ios_base::failbit);
        }

        void open(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath, std::ios_base::openmode openMode = std::ios_base::out | std::ios_base::binary)
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

        template <class Iterable_T>
        std::streamsize writeBytes(const Iterable_T& iterable) { return m_streamBuffer.writeBytes(iterable); }
        
        // TODO: revise behaviour of operator<<. Currently is a mess accepting arbitrary collection of types.
        std::ostream& operator<<(const DFG_CLASS_NAME(StringViewUtf8)& sv)
        {
            auto iter = sv.beginRaw();
            const auto iterEnd = sv.endRaw();
            while (iter != iterEnd)
            {
                const auto cp = DFG_MODULE_NS(utf)::readUtfCharAndAdvance(iter, iterEnd, []() {});
                m_streamBuffer.writeUnicodeChar(cp);
            }
            return *this;
        }

        // TODO: test
        std::ostream& operator<<(const DFG_CLASS_NAME(StringViewLatin1)& sv)
        {
            const auto iterEnd = sv.endRaw();
            for (auto iter = sv.beginRaw(); iter != iterEnd; ++iter)
                writeUnicodeChar(uint8(*iter));
            return *this;
        }

        // TODO: test
        std::ostream& operator<<(const DFG_CLASS_NAME(StringViewAscii)& sv)
        {
            if (areAsciiBytesValidContentInEncoding(encoding())) // Optimization especially for UTF8-output: write ASCII-bytes with writeBytes() instead of per-char writeUnicodeChar().
            {
                writeBytes(sv.dataRaw(), sv.size());
                return *this;
            }
            else
                return operator<<(DFG_CLASS_NAME(StringViewLatin1)(sv.data(), sv.size()));
        }

        // TODO: test
        template <class T>
        std::ostream& privWriteNumericValue(const T val)
        {
            char buffer[32];
            DFG_MODULE_NS(str)::toStr(val, buffer);
            return operator<<(DFG_CLASS_NAME(StringViewAscii)(SzPtrAscii(buffer)));
        }

#define DFG_TEMP_DEFINE_STREAM_OPERATOR_FOR_NUMERIC(T) std::ostream& operator<<(const T val) { return privWriteNumericValue(val); }

        DFG_TEMP_DEFINE_STREAM_OPERATOR_FOR_NUMERIC(int32);
        DFG_TEMP_DEFINE_STREAM_OPERATOR_FOR_NUMERIC(uint32);
        DFG_TEMP_DEFINE_STREAM_OPERATOR_FOR_NUMERIC(int64);
        DFG_TEMP_DEFINE_STREAM_OPERATOR_FOR_NUMERIC(uint64);
        DFG_TEMP_DEFINE_STREAM_OPERATOR_FOR_NUMERIC(float);
        DFG_TEMP_DEFINE_STREAM_OPERATOR_FOR_NUMERIC(double);

#undef DFG_TEMP_DEFINE_STREAM_OPERATOR_FOR_NUMERIC

        std::ostream& operator<<(const char c)
        {
            writeUnicodeChar(c);
            return *this;
        }

        std::ostream& operator<<(const SzPtrUtf8R tpsz)
        {
            return operator<<(DFG_CLASS_NAME(StringViewUtf8)(tpsz));
        }

        // TODO: test
        template <class T>
        std::streamsize writeUnicodeChar(T c)
        { 
            DFG_STATIC_ASSERT(std::is_unsigned<T>::value, "Expecting unsigned types"); // For now require unsigned types as previously passing negative char to uint32 yielded unexpected results.
            return m_streamBuffer.writeUnicodeChar(c);
        }

        std::streamsize writeUnicodeChar(const char c)
        {
            return writeUnicodeChar(uint8(c));
        }

        DFG_CLASS_NAME(OfStreamBufferWithEncoding) m_streamBuffer;
    };

    // Output file stream guaranteed to
    //     -be inherited from std::ostream
    //     -by default open files in binary mode.
    // Intended to be used in place of std::ofstream.
    // Note: preliminary placeholder quality.
    class DFG_CLASS_NAME(OfStream) : public std::ofstream
    {
    public:
        typedef std::ofstream BaseClass;

        // Dumps bytes to file overwriting existing content.
        static bool dumpBytesToFile_overwriting(const DFG_CLASS_NAME(StringViewSzC)& sPath, const char* p, size_t nCount)
        {
            DFG_CLASS_NAME(OfStream) ostrm(sPath.c_str());
            if (!ostrm.is_open())
                return false;
            while (nCount > 0)
            {
                const auto nWriteCount = (nCount > NumericTraits<std::streamsize>::maxValue) ? NumericTraits<std::streamsize>::maxValue : static_cast<std::streamsize>(nCount);
                ostrm.write(p, nWriteCount);
                p += nWriteCount;
                nCount -= static_cast<size_t>(nWriteCount);

            }
            return ostrm.good();
        }

        DFG_CLASS_NAME(OfStream)()
        {
        }

        DFG_CLASS_NAME(OfStream)(const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath)
        {
            open(sPath);
        }

        DFG_CLASS_NAME(OfStream)(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath)
        {
            open(sPath);
        }

        void open(const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath)
        {
            openOfStream(*this, sPath, std::ios::binary | std::ios::out);
        }

        void open(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath)
        {
            openOfStream(*this, sPath, std::ios::binary | std::ios::out);
        }

#if DFG_LANGFEAT_MOVABLE_STREAMS
        DFG_CLASS_NAME(OfStream)(DFG_CLASS_NAME(OfStream)&& other) : 
            BaseClass(std::move(other))
        {
        }
#endif // DFG_LANGFEAT_MOVABLE_STREAMS
    };

} } // module namespace
