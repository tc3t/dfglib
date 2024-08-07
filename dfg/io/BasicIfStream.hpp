#pragma once

#include "../dfgBase.hpp"
#include "BasicIStreamCRTP.hpp"
#include "../ReadOnlySzParam.hpp"
#include "BasicImStream.hpp"
#include <cstdio>

#ifndef _WIN32
	#include "widePathStrToFstreamFriendlyNonWide.hpp"
#endif

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(io) {

// Returns zero on successful fseek, non-zero on failure.
inline int fseek_dfg(std::FILE* pFile, const int64 offset, const int origin)
{
    if (!pFile)
        return -1;
#ifdef _WIN32
    return _fseeki64(pFile, offset, origin);
#else
    return fseeko64(pFile, offset, origin);
#endif
}

// Returns current position as offset from beginning, -1 on error.
inline int64 ftell_dfg(std::FILE* pFile)
{
    if (!pFile)
        return -1;
#ifdef _WIN32
    return _ftelli64(pFile);
#else
    return ftello64(pFile);
#endif
}

// Basic input stream for reading data from file, currently only binary input is implemented.
class BasicIfStream : public BasicIStreamCRTP<BasicIfStream, int64>
{
public:
    using OffType = int64;
    enum SeekOrigin { SeekOriginBegin, SeekOriginCurrent, SeekOriginEnd};
    enum { s_nDefaultBufferSize = 4096 };

    class Buffer : public BasicImStream_T<char>
    {
    public:
        using BaseClass = BasicImStream_T<char>;

        Buffer(const size_t nBufferSize = 0)
            : BaseClass(nullptr, 0)
        {
            resizeStorage(nBufferSize);
        }

        Buffer(Buffer&& other) noexcept
            : BaseClass(nullptr, 0)
        {
            // Setting buffer
            const auto nBufferedCount = other.countOfRemainingElems();
            const auto nStartPos = (nBufferedCount > 0) ? other.currentPtr() - other.beginPtr() : 0;
            const auto nOldMemViewSize = other.m_streamBuffer.countInBytesTotal();
            this->m_storage.swap(other.m_storage);
            other.clearBufferView();
            if (nBufferedCount > 0)
            {
                this->m_streamBuffer.setSource(this->m_storage.data(), nOldMemViewSize);
                this->m_streamBuffer.seekg(nStartPos);
            }
        }

        char* data()
        {
            return m_storage.data();
        }

        size_t storageCapacity() const
        {
            return m_storage.size();
        }

        void setActiveBufferFromStorage(const size_t nCount)
        {
            this->m_streamBuffer.setSource(this->data(), Min(nCount, storageCapacity()));
        }

        size_t resizeStorage(const size_t nSizeRequest)
        {
            if (m_streamBuffer.countInBytesRemaining() > 0)
                return storageCapacity();
            if (nSizeRequest > Min(m_storage.max_size(), saturateCast<size_t>(maxValueOfType<OffType>())))
                return storageCapacity();
            const auto nOldSize = m_storage.size();
            try
            {
                m_storage.resize(nSizeRequest);
            }
            catch (const std::exception&)
            {
                m_storage.resize(nOldSize);
            }
            clearBufferView();
            return storageCapacity();
        }

        int readOne()
        {
            return this->m_streamBuffer.get(eofVal());
        }

        // Empties buffer view without touching storage.
        void clearBufferView()
        {
            this->m_streamBuffer.setSource(nullptr, 0);
        }

    private:
        std::vector<char> m_storage;
    };

    BasicIfStream(const ReadOnlySzParamC sPath, const size_t nBufferSize = s_nDefaultBufferSize)
        : m_buffer(nBufferSize)
    {
#ifdef _MSC_VER
        #pragma warning(disable : 4996) // This function or variable may be unsafe
#endif // _MSC_VER

        // Note: can't use fopen_s directly since it can't open file for sharing (broke test case dfgIo.IfStreamWithEncodingReadOnlyFile)
        m_pFile = std::fopen(sPath.c_str(), "rb");

#ifdef _MSC_VER
        #pragma warning(default : 4996)
#endif // _MSC_VER
    }

    BasicIfStream(const ReadOnlySzParamW sPath, const size_t nBufferSize = s_nDefaultBufferSize)
        : m_buffer(nBufferSize)
    {
#ifdef _WIN32
    #ifdef _MSC_VER
        #pragma warning(disable : 4996) // This function or variable may be unsafe
    #endif // _MSC_VER

        // Note: can't use _wfopen_s since it can't open file for sharing (broke test case dfgIo.IfStreamWithEncodingReadOnlyFile)
        m_pFile = _wfopen(sPath.c_str(), L"rb");

    #ifdef _MSC_VER
        #pragma warning(default : 4996)
    #endif // _MSC_VER
#else
        m_pFile = std::fopen(pathStrToFileApiFriendlyPath(sPath).c_str(), "rb");
#endif
    }

    inline BasicIfStream(BasicIfStream&& other) noexcept;

    ~BasicIfStream()
    {
        if (m_pFile)
            std::fclose(m_pFile);
    }

    // Returns total buffer size in bytes.
    inline size_t bufferSize() const;
    // Tries to set new buffer size and returns value effective after this call. This may differ from requested.
    // Buffer size can be changed only if there are no buffered bytes.
    inline size_t bufferSize(size_t nSizeRequest);

    // Returns the number of bytes available in memory buffer, i.e. bytes that can be read without reading from file device.
    inline size_t bufferedByteCount() const;

    // Returns single character, eofVal() if unable to read.
    inline int get();

    inline BasicIfStream& read(char* p, const size_t nCount);
    inline size_t fread(char* p, const size_t nCount);
    inline size_t readBytes(char* p, const size_t nCount);

    // Returns read position
    inline PosType tellg() const;

    // Returns true on successful seek, false on failure.
    inline bool seekg(SeekOrigin seekOrigin, OffType offset);
    // Sets read position as absolute value, effectively identical to seekg(SeekOriginBegin, pos);
    inline bool seekg(const PosType& pos);

    inline bool good() const;

    // Fills buffer from current file pos.
    inline void privFillBuffer();

    std::FILE* m_pFile;
    Buffer m_buffer;
};

inline BasicIfStream::BasicIfStream(BasicIfStream&& other) noexcept
    : m_pFile(other.m_pFile)
    , m_buffer(std::move(other.m_buffer))
{
    other.m_pFile = nullptr;
}


inline size_t BasicIfStream::bufferSize() const
{
    return m_buffer.storageCapacity();
}

inline size_t BasicIfStream::bufferSize(const size_t nSizeRequest)
{
    return m_buffer.resizeStorage(nSizeRequest);
}

inline size_t BasicIfStream::bufferedByteCount() const
{
    return m_buffer.countOfRemainingElems();
}

inline void BasicIfStream::privFillBuffer()
{
    if (!m_pFile)
        return;
    const auto nRead = std::fread(m_buffer.data(), 1, m_buffer.storageCapacity(), m_pFile);
    m_buffer.setActiveBufferFromStorage(nRead);
}

inline int BasicIfStream::get()
{
    if (bufferSize() == 0)
        return (m_pFile != nullptr) ? fgetc(m_pFile) : eofVal();
    if (!m_buffer.canReadOne())
        privFillBuffer();
    return m_buffer.readOne();
}

inline BasicIfStream& BasicIfStream::read(char* p, const size_t nCount)
{
    fread(p, nCount);
    return *this;
}

inline size_t BasicIfStream::readBytes(char* p, const size_t nCount)
{
    return fread(p, nCount);
}

inline size_t BasicIfStream::fread(char* p, size_t nCount)
{
    // First reading all bytes that are available in buffer.
    size_t nRead = 0;
    nRead = m_buffer.readBytes(p, Min(nCount, bufferedByteCount()));
    if (nRead == nCount)
        return nRead; // All were available in buffer.
    // Read request is more than what buffer had. Either fill new buffer and read remaining from there
    // or if request is more than buffer size, reading directly from file.
    nCount -= nRead;
    p += nRead;
    if (nCount < bufferSize())
    {
        privFillBuffer();
        nRead += m_buffer.readBytes(p, nCount);
        return nRead;
    }
    return (m_pFile) ? std::fread(p, 1, nCount, m_pFile) : 0;
}

inline bool BasicIfStream::good() const
{
    if (bufferedByteCount() > 0)
        return true;
    else
        return (m_pFile) ? std::ferror(m_pFile) == 0 && std::feof(m_pFile) == 0 : false;
}

inline auto BasicIfStream::tellg() const -> PosType
{
    PosType pos = PosType();
    if (m_pFile)
    {
        pos = ftell_dfg(m_pFile);
        DFG_ASSERT_CORRECTNESS(bufferedByteCount() <= static_cast<uint64>(pos));
        pos -= static_cast<decltype(pos)>(bufferedByteCount());
    }
    return pos;
}

inline bool BasicIfStream::seekg(const PosType& pos)
{
    return seekg(SeekOriginBegin, pos);
}

inline bool BasicIfStream::seekg(const SeekOrigin seekOrigin, OffType offset)
{
    DFG_STATIC_ASSERT(SeekOriginBegin == SEEK_SET && SeekOriginCurrent == SEEK_CUR && SeekOriginEnd == SEEK_END, "Check SeekOriginEnums; with current implementation should match with fseek() macros");
    if (!m_pFile)
        return false;
    if (seekOrigin == SeekOriginCurrent)
    {
        const OffType nShift = static_cast<OffType>(bufferedByteCount());
        offset = Max(offset, minValueOfType(offset) + nShift); // overflow guard
        offset -= nShift;
    }
    m_buffer.clearBufferView(); // For now just bluntly clearing the whole buffer; in many cases it would be possible to adjust it.
    return (fseek_dfg(m_pFile, offset, seekOrigin) == 0);
}

inline size_t readBytes(BasicIfStream& istrm, char* pDest, const size_t nMaxReadSize)
{
    return istrm.readBytes(pDest, nMaxReadSize);
}

}} // module io
