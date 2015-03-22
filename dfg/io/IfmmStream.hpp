#pragma once

#include "../os/memoryMappedFile.hpp"
#include "ImcByteStream.hpp"
#include "StreamBufferMem.hpp"
#include "../baseConstructorDelegate.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    class DFG_CLASS_NAME(FileMemoryMapped) : public boost::iostreams::mapped_file_source
    {
    public:
        typedef boost::iostreams::mapped_file_source BaseClass;

        DFG_CLASS_NAME(FileMemoryMapped)() {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_1(DFG_CLASS_NAME(FileMemoryMapped), BaseClass) {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_2(DFG_CLASS_NAME(FileMemoryMapped), BaseClass) {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_3(DFG_CLASS_NAME(FileMemoryMapped), BaseClass) {}
    };

    // IfStream that uses memory mapped file.
    class DFG_CLASS_NAME(IfmmStream) : public std::istream
    {
    public:
        typedef std::istream BaseClass;
        typedef DFG_CLASS_NAME(FileMemoryMapped) FileMemoryMappedT;
        typedef DFG_CLASS_NAME(StreamBufferMem)<char> StreamBufT;

        DFG_CLASS_NAME(IfmmStream)() : BaseClass(nullptr) {}

        template <class T>
        DFG_CLASS_NAME(IfmmStream)(const T& t) : BaseClass(nullptr), m_mappedFileSource(t)
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
