#pragma once

#include "../os/memoryMappedFile.hpp"
#include "StreamBufferMem.hpp"
#include "../baseConstructorDelegate.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    class FileMemoryMapped : public boost::iostreams::mapped_file_source
    {
    public:
        typedef boost::iostreams::mapped_file_source BaseClass;

        FileMemoryMapped() {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_1(FileMemoryMapped, BaseClass) {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_2(FileMemoryMapped, BaseClass) {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_3(FileMemoryMapped, BaseClass) {}
    };

    // IfStream that uses memory mapped file.
    // Note: Since IfmmStream is based on std::istream, if performance matters, consider manual memory mapping and using BasicImStream
    class IfmmStream : public std::istream
    {
    public:
        typedef std::istream BaseClass;
        typedef FileMemoryMapped FileMemoryMappedT;
        typedef StreamBufferMem<char> StreamBufT;

        IfmmStream() : BaseClass(nullptr) {}

        template <class T>
        IfmmStream(const T& t) : BaseClass(nullptr), m_mappedFileSource(t)
        {
            PrivInit();
        }

        template <class Path_T>
        void open(const Path_T& path)
        {
            m_mappedFileSource.open(path);
            if (m_mappedFileSource.is_open())
            {
                m_streamBuf.setSource(m_mappedFileSource.data(), m_mappedFileSource.size());
                if (&m_streamBuf != rdbuf())
                {
                    rdbuf(&m_streamBuf); // Note: calls clear()
                }
            }
            else
                setstate(std::ios_base::failbit);
        }

        bool is_open() const
        {
            return m_mappedFileSource.is_open();
        }

        const char* data() const
        {
            return m_mappedFileSource.data();
        }

        size_t size() const
        {
            return m_mappedFileSource.size();
        }

        void PrivInit()
        {
            m_streamBuf.setSource(m_mappedFileSource.data(), m_mappedFileSource.size());
            init(&m_streamBuf);
        }

        FileMemoryMappedT m_mappedFileSource;
        StreamBufT m_streamBuf;
    }; // class IfmmStream

} }
