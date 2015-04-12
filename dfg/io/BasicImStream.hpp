#pragma once

#include "BasicIStreamCRTP.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(io) {

namespace DFG_DETAIL_NS
{
    template <class Char_T>
    class DefaultBasicStreamBufferBase
    {
    public:
        typedef Char_T char_type;
        typedef std::char_traits<char_type> traits_type; // TODO: revise this.
    };
} // namespace DFG_DETAIL_NS


template <class Base_T>
class DFG_CLASS_NAME(BasicStreamBuffer_T) : public Base_T
{
public:
    typedef typename Base_T::char_type char_type;
    typedef const char_type* ConstCharTPtr;

    DFG_CLASS_NAME(BasicStreamBuffer_T)(const ConstCharTPtr pData, const size_t nCharCount) :
        m_pBegin(pData),
        m_pCurrent(pData),
        m_pEnd(pData + nCharCount)
    {}

    DFG_CLASS_NAME(BasicStreamBuffer_T)(DFG_CLASS_NAME(BasicStreamBuffer_T)&& other) :
        m_pBegin(other.m_pBegin),
        m_pCurrent(other.m_pCurrent),
        m_pEnd(other.m_pEnd)
    {
        other.m_pBegin = nullptr;
        other.m_pCurrent = nullptr;
        other.m_pEnd = nullptr;
    }

    size_t countInCharsTotal() const { return m_pEnd - m_pBegin; }
    size_t countInBytesTotal() const { return countInCharsTotal() * sizeof(char_type); }

    size_t countInCharsRemaining() const { return m_pEnd - m_pCurrent; }
    size_t countInBytesRemaining() const { return countInCharsRemaining() * sizeof(char_type); }

    template <class T>
    T get(T valueIfUnableToGet)
    { 
        return (m_pCurrent != m_pEnd) ? std::char_traits<char_type>::to_int_type(*m_pCurrent++) : valueIfUnableToGet;
    }

    bool isAtEnd() const { return m_pCurrent == m_pEnd; }

    void read(char_type* p, const size_t nCharCount)
    {
        const auto nReadCharCount = Min(nCharCount, countInCharsRemaining());
        memcpy(p, m_pCurrent, nReadCharCount * sizeof(char_type));
        m_pCurrent += nReadCharCount;
    }

    size_t readBytes(char* p, const size_t nCount)
    {
        const auto nRemainingCharCount = countInCharsRemaining();
        const auto nRequestedCharCount = nCount / sizeof(char_type); // Note: In case of nCount not being multiple of sizeof(char_type), using floor value.

        const auto nReadCharCount = Min(nRemainingCharCount, nRequestedCharCount);

        const auto nReadByteCount = nReadCharCount * sizeof(char_type);

        memcpy(p, m_pCurrent, nReadByteCount);
        m_pCurrent += nReadCharCount;
        return nReadByteCount;
    }

    size_t tellg() const
    {
        return m_pCurrent - m_pBegin;
    }
    void seekg(const size_t& pos)
    {
        m_pCurrent = m_pBegin + Min(countInCharsTotal(), pos);
    }

    void setSource(ConstCharTPtr pBegin, size_t nSize)
    {
        m_pBegin = pBegin;
        m_pCurrent = m_pBegin;
        m_pEnd = m_pBegin + nSize;
    }
    
    ConstCharTPtr m_pCurrent;
    ConstCharTPtr m_pBegin;
    ConstCharTPtr m_pEnd;
};

// Basic input stream for reading characters from memory using stream-like interface.
template <class Char_T>
class DFG_CLASS_NAME(BasicImStream_T) : public DFG_CLASS_NAME(BasicIStreamCRTP)<DFG_CLASS_NAME(BasicImStream_T<Char_T>), Char_T>
{
public:
    typedef DFG_CLASS_NAME(BasicIStreamCRTP)<DFG_CLASS_NAME(BasicImStream_T<Char_T>), Char_T> BaseClass;
    
    typedef DFG_CLASS_NAME(BasicStreamBuffer_T)<DFG_DETAIL_NS::DefaultBasicStreamBufferBase<Char_T>> StreamBufferT;
    typedef typename BaseClass::int_type int_type;
    typedef typename BaseClass::PosType PosType;

    DFG_CLASS_NAME(BasicImStream_T)(const Char_T* pData, const size_t nCharCount) :
        m_streamBuffer(pData, nCharCount)
    {}

    size_t sizeInCharacters() const { return m_streamBuffer.countInCharsTotal(); }
    size_t countOfRemainingElems() const { return m_streamBuffer.countInCharsRemaining(); }
    int_type get() { return m_streamBuffer.get(this->eofVal()); }
    void get(Char_T& dst) { dst = static_cast<Char_T>(get()); }
    bool good() const { return !m_streamBuffer.isAtEnd(); }
    DFG_CLASS_NAME(BasicImStream_T)& read(Char_T* p, const size_t nCharCount)
    {
        m_streamBuffer.read(p, nCharCount);
        return *this;
    }

    size_t readBytes(char* p, const size_t nCount)
    {
        return m_streamBuffer.readBytes(p, nCount);
    }

    inline PosType tellg() const
    {
        return m_streamBuffer.tellg();
    }
    inline void seekg(const PosType& pos)
    {
        m_streamBuffer.seekg(static_cast<size_t>(pos));
    }

    StreamBufferT m_streamBuffer;
};

typedef DFG_CLASS_NAME(BasicImStream_T<char>) DFG_CLASS_NAME(BasicImStream);


template <class T> inline size_t readBytes(DFG_CLASS_NAME(BasicImStream_T)<T>& istrm, char* pDest, const size_t nMaxReadSize)
{
    return istrm.readBytes(pDest, nMaxReadSize);
}

}} // module io
