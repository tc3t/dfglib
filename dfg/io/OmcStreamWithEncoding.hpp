#pragma once

#include "../dfgDefs.hpp"
#include "textEncodingTypes.hpp"
#include "../build/languageFeatureInfo.hpp"
#include <streambuf>
#include "../utf.hpp"
#include "OmcByteStream.hpp"
#include "../numericTypeTools.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    // Simple output buffer with following properties:
    //	-Writes either to external buffer or if not given, to internal. Buffer type is template parameter.
    //	-Guarantees contiguous storage and access to non-copying sequence of characters through data() method.
    //		If Cont_T::data() returns null terminated string, then the sequence is also null terminated.
    //		This is the case for example for std::basic_string<T> objects.
    //  -Guarantees non-copying access to underlying data container object.
    //	-Supports releasing the content through move operation.
    // Requirements for Cont_T
    //	-Must provider data() method that provides access to contiguous data.
    template <class Cont_T>
    class DFG_CLASS_NAME(OmcStreamBufferWithEncoding) : public DFG_CLASS_NAME(OmcByteStreamBuffer)<Cont_T>
    {
    public:
        typedef DFG_CLASS_NAME(OmcByteStreamBuffer)<Cont_T> BaseClass;
        typedef typename BaseClass::int_type int_type;

        DFG_CLASS_NAME(OmcStreamBufferWithEncoding)(Cont_T* pCont, TextEncoding encoding) :
            BaseClass(pCont),
            m_encoding(encoding)
        {
        }

        void setEncoding(TextEncoding encoding)
        {
            m_encoding = encoding;
        }

        TextEncoding encoding() const
        { 
            return m_encoding; 
        }

        int_type overflow(int_type cp) override
        {
            using namespace DFG_MODULE_NS(utf);
            DFG_ASSERT_UB(m_pData != nullptr);
            if (cp != EOF)
            {
                if (m_encoding == encodingUnknown)
                    this->writeOne(static_cast<char>(cp));
                else
                {
                    const uint32 cpUnsigned = (cp >= 0) ? cp : uint8(int8(cp));
                    cpToEncoded(cpUnsigned, std::back_inserter(this->container()), m_encoding);
                }
            }
            return cp;
        }

        std::streamsize xsputn(const char* s, std::streamsize num) override
        {
            for (auto i = num; i > 0; --i, ++s)
                overflow(*s);
            return num;
        }

        std::streamsize writeUnicodeChar(const uint32 c)
        {
            if (isValWithinLimitsOfType<int_type>(c))
            {
                const auto startPos = currentPosInBytes();
                overflow(static_cast<int_type>(c));
                return currentPosInBytes() - startPos;
            }
            else
                return 0;
        }

        TextEncoding m_encoding;
    };

    template <class Cont_T>
    class DFG_CLASS_NAME(OmcStreamWithEncoding) : public DFG_CLASS_NAME(OmcByteStream)<Cont_T, DFG_CLASS_NAME(OmcStreamBufferWithEncoding)<Cont_T>>
    {
    public:
        typedef DFG_CLASS_NAME(OmcByteStream)<Cont_T, DFG_CLASS_NAME(OmcStreamBufferWithEncoding)<Cont_T>> BaseClass;
        typedef DFG_CLASS_NAME(OmcStreamWithEncoding) ThisClass;

        DFG_CLASS_NAME(OmcStreamWithEncoding)(Cont_T* pCont, TextEncoding encoding) :
            BaseClass(pCont, encoding)
        {
        }

#if DFG_LANGFEAT_MOVABLE_STREAMS
        DFG_CLASS_NAME(OmcStreamWithEncoding)(DFG_CLASS_NAME(OmcStreamWithEncoding)&& other) :
            BaseClass(std::move(other))
        {
        }
#endif

        TextEncoding encoding() const { return this->m_streamBuf.encoding(); }

        std::streamsize writeUnicodeChar(const uint32 c)
        {
            return this->m_streamBuf.writeUnicodeChar(c);
        }

    }; // Class OmcStreamWithEncoding

#if DFG_LANGFEAT_MOVABLE_STREAMS
    template <class Cont_T>
    DFG_CLASS_NAME(OmcStreamWithEncoding)<Cont_T> createOmcStreamWithEncoding(Cont_T* pCont, TextEncoding encoding)
    {
        return DFG_CLASS_NAME(OmcStreamWithEncoding)<Cont_T>(pCont, encoding);
    }
#endif

} } // module namespace
