#pragma once

#include "../dfgDefs.hpp"
#include "BasicImStream.hpp"
#include "../utf/utfBom.hpp"
#include "../utf.hpp"
#include "textEncodingTypes.hpp"
#include "checkBom.hpp"
#include "../iter/RawStorageIterator.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    // For reading fixed-size characters from memory.
    template <class Char_T>
    class DFG_CLASS_NAME(StreamBufferMem) : public DFG_CLASS_NAME(BasicStreamBuffer_T)<std::basic_streambuf<Char_T>>
    {
    public:
        typedef DFG_CLASS_NAME(BasicStreamBuffer_T)<std::basic_streambuf<Char_T>> BaseClass;
        typedef const Char_T* IteratorType;
        typedef typename BaseClass::int_type int_type;
        typedef typename BaseClass::off_type off_type;
        typedef typename BaseClass::pos_type pos_type;

        DFG_CLASS_NAME(StreamBufferMem)(IteratorType p = nullptr, const size_t nSize = 0) :
            BaseClass(p, nSize)
        {
        }

        // Move constructor
        DFG_CLASS_NAME(StreamBufferMem)(DFG_CLASS_NAME(StreamBufferMem)&& other) :
            BaseClass(std::move(static_cast<BaseClass&>(other)))
        {
        }

        // Read current.
        int_type underflow() override
        {
            return (this->m_pCurrent != this->m_pEnd) ? std::char_traits<Char_T>::to_int_type(*this->m_pCurrent) : BaseClass::underflow();
        }

        // Read and advance.
        int_type uflow() override
        {
            return (this->m_pCurrent != this->m_pEnd) ? std::char_traits<Char_T>::to_int_type(*this->m_pCurrent++) : BaseClass::underflow();
        }

        off_type sizeInBytes() const
        {
            return (this->m_pEnd - this->m_pBegin) * sizeof(Char_T);
        }

        off_type currentPos() const
        {
            return this->m_pCurrent - this->m_pBegin;
        }

        // Seeks in characters. Note that, according to MSDN documentation, seekg in not supported for text streams.
        // In any case this function is needed for tellg(), which calls this with off == 0 and dir == cur.
        std::streampos seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode om) override
        {
            if (dir == std::ios_base::beg)
            {
                return seekpos(off, om);
            }
            else if (dir == std::ios_base::cur)
            {
                return seekpos(currentPos() + off, om);
            }
            else if (dir == std::ios_base::end)
            {
                return seekpos(sizeInBytes() + off, om);
            }
            else
                return std::streampos(-1);
        }

        pos_type seekpos(pos_type pos, std::ios_base::openmode /*om*/) override
        {
            if (pos >= 0 && pos <= sizeInBytes())
            {
                this->m_pCurrent = this->m_pBegin + static_cast<std::ptrdiff_t>(pos);
                return pos;
            }
            else
                return pos_type(off_type(-1));
        }
    }; // class StreamBufferMem

// Supports reading characters from memory with various encodings.
class DFG_CLASS_NAME(StreamBufferMemWithEncoding) : public DFG_CLASS_NAME(StreamBufferMem)<char>
{
public:
    typedef DFG_CLASS_NAME(StreamBufferMem)<char> BaseClass;
    //typedef BaseClass::IteratorType IteratorType; // TODO: check what requirements there are for this.

    DFG_CLASS_NAME(StreamBufferMemWithEncoding)(IteratorType p, const size_t nSize, const TextEncoding encoding) :
        BaseClass(p, nSize),
        m_encoding(encoding),
        m_pReadImpl(&readAndAdvanceByte)
    {
        using namespace DFG_MODULE_NS(utf);

        DFG_CLASS_NAME(BasicImStream) istrm(this->m_pCurrent, nSize);
        const auto streamEncoding = checkBOM(istrm);
        if (m_encoding != encodingUnknown && streamEncoding != encodingUnknown && m_encoding != streamEncoding)
        {
            DFG_ASSERT_WITH_MSG(false, "Stream encoding and user given encoding are contradictory, using the one given by user.");
        }
        if (m_encoding == encodingUnknown)
            m_encoding = streamEncoding;
        this->m_pCurrent += bomSizeInBytes(streamEncoding);
        setReaderByEncoding(m_encoding);
    }

    // Move constructor
    DFG_CLASS_NAME(StreamBufferMemWithEncoding)(DFG_CLASS_NAME(StreamBufferMemWithEncoding)&& other) :
        BaseClass(std::move(static_cast<BaseClass&>(other))),
        m_encoding(other.m_encoding),
        m_pReadImpl(other.m_pReadImpl)
    {
    }

    void setReaderByEncoding(const TextEncoding encoding)
    {
        switch (encoding)
        {
            case encodingUTF8:          m_pReadImpl = &readAndAdvanceUtf8; break;
            case encodingUTF16Le:       m_pReadImpl = &readAndAdvanceUtf16Le; break;
            case encodingUTF16Be:       m_pReadImpl = &readAndAdvanceUtf16Be; break;
            case encodingUCS2Le:        m_pReadImpl = &readAndAdvanceUcs2Le; break;
            case encodingUCS2Be:        m_pReadImpl = &readAndAdvanceUcs2Be; break;
            case encodingUTF32Le:       m_pReadImpl = &readAndAdvanceUtf32Le; break;
            case encodingUTF32Be:       m_pReadImpl = &readAndAdvanceUtf32Be; break;
            case encodingUCS4Le:        m_pReadImpl = &readAndAdvanceUcs4Le; break;
            case encodingUCS4Be:        m_pReadImpl = &readAndAdvanceUcs4Be; break;
            case encodingLatin1:        m_pReadImpl = &readAndAdvanceByte; break;
            case encodingWindows1252:   m_pReadImpl = &readAndAdvanceWindows1252; break;
            default: m_pReadImpl = &readAndAdvanceByte;
        }
    }

    TextEncoding encoding() const
    {
        return m_encoding;
    }

    static constexpr int_type eofValue()
    {
        return std::char_traits<char>::eof();
    }

    static uint8 toIntType(const char c)
    {
        return static_cast<uint8>(std::char_traits<char>::to_int_type(c));
    }

    // Implementation for case sizeof(Elem_T) == 1
    template <class Elem_T, class BswapFunc_T>
    static int_type readAndAdvanceImpl(IteratorType& iter, const IteratorType& end, BswapFunc_T bswapFunc, std::true_type)
    {
        DFG_STATIC_ASSERT((std::is_same<IteratorType, const char*>::value), "This function needs to be modified if not dealing with pointer iterator.");
        if (iter == end)
            return eofValue();
        auto p2 = reinterpret_cast<const Elem_T*>(iter);
        auto p2End = reinterpret_cast<const Elem_T*>(end);
        auto rv = DFG_MODULE_NS(utf)::readUtfCharAndAdvance(p2, p2End, bswapFunc);
        iter = reinterpret_cast<IteratorType>(p2);
        return rv;
    }

    // Implementation for case sizeof(Elem_T) != 1
    template <class Elem_T, class BswapFunc_T>
    static int_type readAndAdvanceImpl(IteratorType& iter, const IteratorType paramEnd, BswapFunc_T bswapFunc, std::false_type)
    {
        DFG_STATIC_ASSERT((std::is_same<IteratorType, const char*>::value), "This function needs to be modified if not dealing with pointer iterator.");
        auto end = paramEnd;
        if (iter == end)
            return eofValue();
        // Check that (end - iter) can be divided by sizeof(Elem_T) (i.e. that there are no incomplete elements)
        const auto remainder = ((end - iter) % sizeof(Elem_T));
        if (remainder != 0)
        {
            // Uneven input, modify end to point to last whole element
            end -= remainder;
        }
        if (static_cast<size_t>(end - iter) < sizeof(Elem_T)) // Don't have a whole element?
        {
            iter = paramEnd;
            return eofValue();
        }

        DFG_MODULE_NS(iter)::RawStorageIterator<const Elem_T> p2(iter);
        DFG_MODULE_NS(iter)::RawStorageIterator<const Elem_T> p2End(end);
        auto rv = DFG_MODULE_NS(utf)::readUtfCharAndAdvance(p2, p2End, bswapFunc);
        iter = p2.ptrChar();
        return rv;
    }

    template <class Elem_T, class BswapFunc_T>
    static int_type readAndAdvanceImpl(IteratorType& iter, const IteratorType& end, BswapFunc_T bswapFunc)
    {
        return readAndAdvanceImpl<Elem_T>(iter, end, bswapFunc, std::integral_constant<bool, sizeof(Elem_T) == 1>());
    }

    static int_type readAndAdvanceByte(IteratorType& iter, const IteratorType& end)
    {
        DFG_STATIC_ASSERT(sizeof(*iter) == 1, "Implementation expects char-type");
        return (iter != end) ? toIntType(*iter++) : eofValue();
    }

    static int_type readAndAdvanceWindows1252(IteratorType& iter, const IteratorType& end)
    {
        DFG_STATIC_ASSERT(sizeof(*iter) == 1, "Implementation expects char-type");
        return (iter != end) ? DFG_MODULE_NS(utf)::windows1252charToCp(toIntType(*iter++)) : eofValue();
    }

    // TODO: revise and test
    static int_type readAndAdvanceUcs2Le(IteratorType& iter, const IteratorType& end)
    {
        DFG_STATIC_ASSERT(sizeof(*iter) == 1, "Implementation expects char-type");
        if (iter == end)
            return eofValue();
        uint16 val = 0;
        val = toIntType(*iter++);
        if (iter == end)
            return eofValue();
        val += (toIntType(*iter++) << 8);
        return val;
    }

    // TODO: revise and test
    static int_type readAndAdvanceUcs2Be(IteratorType& iter, const IteratorType& end)
    {
        DFG_STATIC_ASSERT(sizeof(*iter) == 1, "Implementation expects char-type");
        if (iter == end)
            return eofValue();
        uint16 val = 0;
        val = (toIntType(*iter++) << 8);
        if (iter == end)
            return eofValue();
        val += toIntType(*iter++);
        return val;
    }

    // TODO: revise and test
    static int_type readAndAdvanceUcs4Le(IteratorType& iter, const IteratorType& end)
    {
        DFG_STATIC_ASSERT(sizeof(*iter) == 1, "Implementation expects char-type");
        if (iter == end)
            return eofValue();
        uint32 val = 0;
        val = toIntType (*iter++);
        if (iter == end)
            return eofValue();
        val += (toIntType(*iter++) << 8);
        if (iter == end)
            return eofValue();
        val += (toIntType(*iter++) << 16);
        if (iter == end)
            return eofValue();
        val += (toIntType(*iter++) << 24);
        return val;
    }

    // TODO: revise and test
    static int_type readAndAdvanceUcs4Be(IteratorType& iter, const IteratorType& end)
    {
        DFG_STATIC_ASSERT(sizeof(*iter) == 1, "Implementation expects char-type");
        if (iter == end)
            return eofValue();
        uint32 val = 0;
        val = (toIntType(*iter++) << 24);
        if (iter == end)
            return eofValue();
        val += (toIntType(*iter++) << 16);
        if (iter == end)
            return eofValue();
        val += (toIntType(*iter++) << 8);
        if (iter == end)
            return eofValue();
        val += toIntType(*iter++);
        return val;
    }

    static int_type readAndAdvanceUtf8(IteratorType& iter, const IteratorType& end)
    {
        return readAndAdvanceImpl<uint8>(iter, end, [](){});
    }

    static int_type readAndAdvanceUtf16Le(IteratorType& iter, const IteratorType& end)
    {
        return readAndAdvanceImpl<uint16>(iter, end, ::DFG_ROOT_NS::byteSwapLittleEndianToHost<uint16>);
    }

    static int_type readAndAdvanceUtf16Be(IteratorType& iter, const IteratorType& end)
    {
        return readAndAdvanceImpl<uint16>(iter, end, ::DFG_ROOT_NS::byteSwapBigEndianToHost<uint16>);
    }

    static int_type readAndAdvanceUtf32Le(IteratorType& iter, const IteratorType& end)
    {
        return readAndAdvanceImpl<uint32>(iter, end, ::DFG_ROOT_NS::byteSwapLittleEndianToHost<uint32>);
    }

    static int_type readAndAdvanceUtf32Be(IteratorType& iter, const IteratorType& end)
    {
        return readAndAdvanceImpl<uint32>(iter, end, ::DFG_ROOT_NS::byteSwapBigEndianToHost<uint32>);
    }

    int_type underflow() override
    {
        auto iter = this->m_pCurrent;
        return (m_pReadImpl) ? m_pReadImpl(iter, m_pEnd) : eofValue();
    }

    int_type uflow() override
    {
        return (m_pReadImpl) ? m_pReadImpl(m_pCurrent, m_pEnd) : eofValue();
    }

    TextEncoding m_encoding;
    typedef int_type(*ReadImpl)(IteratorType&, const IteratorType&);
    ReadImpl m_pReadImpl;
}; // class StreamBufferMemWithEncoding

}} // module namespace
