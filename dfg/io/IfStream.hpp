#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "IfmmStream.hpp"
#include "ImStreamWithEncoding.hpp"
#include "StreamBufferMem.hpp"
#include "BasicIfStream.hpp"
#include "fileToByteContainer.hpp"
#include "widePathStrToFstreamFriendlyNonWide.hpp"
#include "BasicIfStream.hpp"
#include "checkBom.hpp"
#include "../utf/utfBom.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    enum class FileReadOpenModeRequest
    {
        memoryMap      = 0x1,              // Request to try to memory map file. May be faster at the expense of memory consumption.
        file           = 0x2,              // Request to try to read file directly from file. May be slower than memoryMap, but memory consumption is limited.
        memoryMap_file = memoryMap | file, // Request to try to memory map file and if unsuccessfull, from file.
    };

    class FileReadOpenRequestArgs
    {
    public:
        FileReadOpenRequestArgs(FileReadOpenModeRequest mode = FileReadOpenModeRequest::memoryMap_file, size_t fileBufferSize = 4096)
            : m_openMode(mode)
            , m_nFileBufferSize(fileBufferSize)
        {}
        FileReadOpenModeRequest openMode() const { return m_openMode; }
        uint32 openModeMask() const { return static_cast<uint32>(m_openMode); }
        bool hasOpenMode(const FileReadOpenModeRequest mode) const { return (static_cast<uint32>(mode) & openModeMask()) != 0; }
        size_t fileBufferSize() const { return m_nFileBufferSize; }

        FileReadOpenModeRequest m_openMode;
        size_t m_nFileBufferSize;
    };

    // IfStreamBufferWithEncoding is streambuf for reading encoded characters from file.
    // Supported encoding are the same as in StreamBufferMemWithEncoding.
    // By default file is memory mapped (or read to vector) and then reading is done from memory.
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

        IfStreamBufferWithEncoding(const StringViewSzC& sPath, TextEncoding encoding, FileReadOpenRequestArgs openArgs = FileReadOpenRequestArgs())
        {
            open(sPath, encoding, openArgs);
        }

        IfStreamBufferWithEncoding(const StringViewSzW& sPath, TextEncoding encoding, FileReadOpenRequestArgs openArgs = FileReadOpenRequestArgs())
        {
            open(sPath, encoding, openArgs);
        }

        // When using file stream, making sure that memstream has enough bytes available.
        void privEnsureMemBufferCapacity()
        {
            if (m_spFileStream && m_spStreamBufferCont->countInBytesRemaining() < 4)
            {
                if (!m_spFileStream->good())
                    return;
                const auto nRemained = m_spStreamBufferCont->readBytes(m_fallback.data(), m_fallback.size());
                const auto nRead = m_spFileStream->readBytes(m_fallback.data() + nRemained, m_fallback.size() - nRemained);
                m_spStreamBufferCont->setSource(m_fallback.data(), nRemained + nRead);
            }
        }

        // "reads characters from the associated input sequence to the get area" (cppreference.com)
        int_type underflow() override
        {
            privEnsureMemBufferCapacity();
            return (m_spStreamBufferCont) ? m_spStreamBufferCont->underflow() : traits_type::eof();
        }

        // Like underflow, but additionally "... advances the next pointer " (cppreference.com)
        int_type uflow() override
        {
            privEnsureMemBufferCapacity();
            return (m_spStreamBufferCont) ? m_spStreamBufferCont->uflow() : traits_type::eof();
        }

        static BasicIfStream::SeekOrigin iosBaseSeekDirToSeekOrigin(const std::ios_base::seekdir dir)
        {
            switch (dir)
            {
                case std::ios_base::beg: return BasicIfStream::SeekOriginBegin;
                case std::ios_base::cur: return BasicIfStream::SeekOriginCurrent;
                case std::ios_base::end: return BasicIfStream::SeekOriginEnd;
                default: return BasicIfStream::SeekOriginBegin;
            }
        }

        pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode om) override
        {
            if (m_spFileStream)
            {
                m_spFileStream->seekg(iosBaseSeekDirToSeekOrigin(dir), off - m_spStreamBufferCont->countInBytesRemaining());
                m_spStreamBufferCont->setSource(nullptr, 0);
                const auto rv = m_spFileStream->tellg();
                return pos_type(rv);
            }
            else
                return (m_spStreamBufferCont) ? m_spStreamBufferCont->seekoff(off, dir, om) : std::streampos(-1);
        }

        pos_type seekpos(pos_type pos, std::ios_base::openmode om) override
        {
            if (m_spFileStream)
            {
                m_spFileStream->seekg(pos);
                m_spStreamBufferCont->setSource(nullptr, 0);
                const auto rv = m_spFileStream->tellg();
                return pos_type(rv);
            }
            else
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
            m_spFileStream.reset();
            m_fallback.clear();
        }

        template <class Char_T>
        void openT(const StringViewSz<Char_T> & sPath, DFG_MODULE_NS(io)::TextEncoding encoding, const FileReadOpenRequestArgs openArgs = FileReadOpenRequestArgs())
        {
            close();
            bool bRetry = true;
            if (openArgs.hasOpenMode(FileReadOpenModeRequest::memoryMap))
            {
                try // open will throw e.g. if file is not found.
                {
                    m_memoryMappedFile.open(sPath.c_str());
                    m_spStreamBufferCont.reset(new StreamBufferMem(m_memoryMappedFile.data(), m_memoryMappedFile.size(), encoding));
                    bRetry = false;
                }
                catch (const std::exception&)
                {
                }
                if (bRetry)
                {
                    try
                    {
                        m_fallback = fileToVector(sPath);
                        if (!m_fallback.empty())
                        {
                            m_spStreamBufferCont.reset(new StreamBufferMem(m_fallback.data(), m_fallback.size(), encoding));
                            bRetry = false;
                        }
                    }
                    catch (const std::exception&)
                    {
                    }
                }
            }
            if (bRetry && openArgs.hasOpenMode(FileReadOpenModeRequest::file))
            {
                // Memory mapping wasn't tried or it failed. Opening a file stream
                // with the logics that reading is still done through m_spStreamBufferCont, but
                // instead of having the whole file there, making sure that it always has at least
                // 4 bytes available so that it always has at least one code point to read.
                m_spFileStream.reset(new BasicIfStream(sPath.c_str(), 0)); // Since buffering is done in this class, disabling it from BasicIfStream.
                if (!m_spFileStream->good())
                    return;
                encoding = checkBOM(*m_spFileStream);
                m_spFileStream->seekg(::DFG_MODULE_NS(utf)::bomSizeInBytes(encoding)); // Skipping BOM
                m_spStreamBufferCont.reset(new StreamBufferMem(nullptr, 0, encoding));
                // Resizing buffer requiring that it is within [4, maxValueOfType<BasicIfStream::OffType>()].
                m_fallback.resize(Max(size_t(4), Min(saturateCast<size_t>(maxValueOfType<BasicIfStream::OffType>()), openArgs.fileBufferSize())));
            }
        }

        void open(const StringViewSzC& sPath, TextEncoding encoding, FileReadOpenRequestArgs openArgs = FileReadOpenRequestArgs())
        {
            openT(sPath, encoding, openArgs);
        }

        void open(const StringViewSzW& sPath, TextEncoding encoding, FileReadOpenRequestArgs openArgs = FileReadOpenRequestArgs())
        {
            // m_memoryMappedFile doesn't like wchar_t, so convert it to char-version.
            openT<char>(widePathStrToFstreamFriendlyNonWide(sPath), encoding, openArgs);
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
        std::unique_ptr<BasicIfStream> m_spFileStream;

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

        IfStreamWithEncoding(const StringViewSzC& sPath, TextEncoding encoding = encodingUnknown, FileReadOpenRequestArgs openArgs = FileReadOpenRequestArgs()) :
            BaseClass(&m_strmBuffer),
            m_strmBuffer(sPath, encoding, openArgs)
        {
            if (!m_strmBuffer.is_open())
                setstate(std::ios::failbit);
        }

        IfStreamWithEncoding(const StringViewSzW& sPath, TextEncoding encoding = encodingUnknown, FileReadOpenRequestArgs openArgs = FileReadOpenRequestArgs()) :
            BaseClass(&m_strmBuffer),
            m_strmBuffer(sPath, encoding, openArgs)
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

        void open(const StringViewSzC& sPath, TextEncoding encoding = encodingUnknown, FileReadOpenRequestArgs openArgs = FileReadOpenRequestArgs())
        {
            m_strmBuffer.open(sPath, encoding, openArgs);
            if (!m_strmBuffer.is_open())
                setstate(std::ios::failbit);
        }

        TextEncoding encoding() const
        {
            return m_strmBuffer.encoding();
        }

        void setEncoding(TextEncoding encoding)
        {
            m_strmBuffer.setEncoding(encoding);
        }

        static constexpr int_type eofValue()
        {
            return IfStreamBufferWithEncoding::traits_type::eof();
        }

        IfStreamBufferWithEncoding m_strmBuffer;
    };

} } // module namespace
