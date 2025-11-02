#ifndef DFG_IO_DELIMITEDTEXTREADER_VGDXYCDA
#define DFG_IO_DELIMITEDTEXTREADER_VGDXYCDA

#include "../io.hpp"
#include "../io/BasicImStream.hpp"
#include <bitset>
#include <type_traits>
#include "../ptrToContiguousMemory.hpp"
#include "ImStreamWithEncoding.hpp"
#include "IfStream.hpp"
#include "../dfgAssert.hpp"
#include "../utf.hpp"
#include "../cont/contAlg.hpp"
#include "../cont/elementType.hpp"
#include "../build/inlineTools.hpp"
#include "../preprocessor/compilerInfoMsvc.hpp"

#include <iterator>


#ifdef _MSC_VER
    #define DFG_DELIMITED_TEXT_READER_INLINING  DFG_FORCEINLINE
#else
    #define DFG_DELIMITED_TEXT_READER_INLINING
#endif

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(io) {

namespace DFG_DETAIL_NS
{
    template <class Strm_T>
    struct IsStreamStringViewCCompatible { enum { value = false }; };

    template <>
    struct IsStreamStringViewCCompatible<BasicImStream> { enum { value = true }; };
}

class DelimitedTextReader
{
public:
    typedef int CharType;
    enum 
    {
        s_nMetaCharNone         = -1,
        s_nMetaCharAutoDetect   = -2
    };
    //static const CharType s_nMetaCharNone = -1;
    //static const CharType s_nMetaCharAutoDetect = -2;

    static bool isMetaChar(const CharType c)
    {
        return (c < 0);
    }

    enum ReadFlag
    {
        rfSkipLeadingWhitespaces, // When set, leading whitespaces in cell are not read into cell contents.
        lastReadFlag = rfSkipLeadingWhitespaces
    };

    enum cellHandlerRv
    {
        cellHrvContinue,
        cellHrvSkipRestOfLine,
        cellHrvTerminateRead, // When set, reading should terminated immediately after handling that cell.
        cellHrvSkipRestOfLineAndTerminate // When set, current line is skipped and reading is terminated.
    };

    enum {rsLookingForNewData         = 0x1,
          //rsInNakedCell               = 0x2,
          rsInEnclosedCell            = 0x4,
          rsPastEnclosedCell          = 0x8,
          rsSeparatorEncountered      = 0x10,
          rsEndOfLineEncountered      = 0x20,
          rsEndOfStream               = 0x40,
          rsTerminated                = 0x80
          };
    typedef int ReadState;

    class CellReaderBase
    {
    protected: // Instances of this class should be possible to create only through derived classes.
        CellReaderBase() :
            m_readState(rsLookingForNewData)
        {
        }

    public:

        bool isReadStateEof() const
        {
            return (m_readState & rsEndOfStream) != 0;
        }

        bool isReadStateEol() const
        {
            return (m_readState & rsEndOfLineEncountered) != 0;
        }

        bool isReadStateTerminated() const
        {
            return m_readState == rsTerminated;
        }

        bool isReadStateEolOrSeparatorEncountered() const
        {
            return ((m_readState & (rsSeparatorEncountered | rsEndOfLineEncountered)) != 0);
        }

        bool isReadStateSeparatorEncountered() const
        {
            return (m_readState & rsSeparatorEncountered) != 0;
        }

        // Check whether read state is end-of-line or end-of-stream.
        // Note that this does not detect state of streams (e.g. fail state).
        bool isReadStateEolOrEof() const
        {
            return (m_readState & (rsEndOfLineEncountered | rsEndOfStream)) != 0;
        }

        bool isReadStateEolOrEofOrTerminated() const
        {
            return (m_readState & (rsEndOfLineEncountered | rsEndOfStream | rsTerminated)) != 0;
        }

        bool isReadStateEndOfCell() const
        {
            return (m_readState & (rsSeparatorEncountered | rsEndOfLineEncountered | rsEndOfStream)) != 0;
        }

        ReadState m_readState;
    }; // Class CellReaderBase

    class FormatDefinition
    {
    };

    typedef int32 InternalCharType;

    template <class T>
    static DFG_CONSTEXPR InternalCharType bufferCharToInternalImpl(T c, std::true_type)
    {
        typedef typename std::make_unsigned<T>::type UnsignedType;
        return static_cast<InternalCharType>(static_cast<UnsignedType>(c));
    }

    template <class T>
    static DFG_CONSTEXPR InternalCharType bufferCharToInternalImpl(T c, std::false_type)
    {
        DFG_STATIC_ASSERT(sizeof(T) <= sizeof(InternalCharType), "Check what's going on, char type is not expected to have size > sizeof(InternalCharType)");
        return static_cast<InternalCharType>(c);
    }

    // Implements conversion needed for correct characters comparison: if not using this function to convert buffer char before comparison, comparing read char(-1) with meta char s_nMetaCharNone(-1) would erroneously return true.
    template <class T>
    static DFG_CONSTEXPR InternalCharType bufferCharToInternal(T c)
    {
        return bufferCharToInternalImpl(c, std::integral_constant<bool, sizeof(T) < sizeof(InternalCharType)>());
    }

    enum CsvFormatFlags
    {
        CsvFormatFlagNoRnTranslation = 0x1,
        CsvFormatFlagsDefault = 0
    };


    template <int Enc_T, int Eol_T, int Sep_T, int Flags_T = CsvFormatFlagsDefault>
    class FormatDefinitionSingleCharsCompileTime
    {
    public:
        typedef std::bitset<lastReadFlag + 1> FlagContainer;
        static const int s_enc   = Enc_T;
        static const int s_eol   = Eol_T;
        static const int s_sep   = Sep_T;
        static const int s_flags = Flags_T;

        static const bool s_hasCompileTimeSeparatorChar = true;

        DFG_STATIC_ASSERT(s_enc != s_nMetaCharAutoDetect, "Compile-time enclosing char can't be auto detectable.");
        DFG_STATIC_ASSERT(s_eol != s_nMetaCharAutoDetect, "Compile-time end-of-line char can't be auto detectable.");
        DFG_STATIC_ASSERT(s_sep != s_nMetaCharAutoDetect, "Compile-time separator char can't be auto detectable.");

        FormatDefinitionSingleCharsCompileTime()
        {}

        FormatDefinitionSingleCharsCompileTime(int , int , int)
        {
            DFG_ASSERT_CORRECTNESS(false); // This overload should not be called.
        }

        DFG_CONSTEXPR uint32 getEnclosingMarkerLengthInChars() const
        {
            return (s_enc >= 0) ? 1 : 0;
        }

        DFG_CONSTEXPR uint32 getEolMarkerLengthInChars() const
        {
            return (s_eol >= 0) ? 1 : 0;
        }

        DFG_CONSTEXPR int getEnc() const { return s_enc; }
        DFG_CONSTEXPR int getEol() const { return s_eol; }
        DFG_CONSTEXPR int getSep() const { return s_sep; }

        // Returns true if \r\n end-of-line is to be read as \n.
        DFG_CONSTEXPR bool isRnTranslationEnabled() const { return !(s_flags & CsvFormatFlagNoRnTranslation); }

        void copySepatorCharFrom(const FormatDefinitionSingleCharsCompileTime&) const
        {
            // Nothing to do here since chars are the same due to identical types.
        }

        bool testFlag(ReadFlag flag) const
        {
            return m_flags.test(flag);
        }

        void setFlag(ReadFlag flag, const bool bState = true)
        {
            m_flags.set(flag, bState);
        }

        FlagContainer m_flags;
    };

    class FormatDefinitionSingleChars
    {
    public:
        typedef std::bitset<lastReadFlag + 1> FlagContainer;

        static const bool s_hasCompileTimeSeparatorChar = false;

        DFG_CONSTEXPR FormatDefinitionSingleChars(int cEnc, int cEol, int cSep) :
            m_cEnc(cEnc),
            m_cEol(cEol),
            m_cSep(cSep)
        {
        }

        uint32 getEnclosingMarkerLengthInChars() const
        {
            return (m_cEnc >= 0) ? 1 : 0;
        }

        uint32 getEolMarkerLengthInChars() const
        {
            return (m_cEol >= 0) ? 1 : 0;
        }

        DFG_CONSTEXPR int getEnc() const {return m_cEnc;}
        DFG_CONSTEXPR int getEol() const {return m_cEol;}
        DFG_CONSTEXPR int getSep() const {return m_cSep;}

        bool isRnTranslationEnabled() const { return true; }

        void copySepatorCharFrom(const FormatDefinitionSingleChars& other)
        {
            m_cSep = other.m_cSep;
        }

        bool testFlag(ReadFlag flag) const
        {
            return m_flags.test(flag);
        }

        void setFlag(ReadFlag flag, const bool bState = true)
        {
            m_flags.set(flag, bState);
        }

        int m_cEnc;
        int m_cEol;
        int m_cSep;
        FlagContainer m_flags;
    };

    template <class Buffer_T, class BufferChar_T>
    class CharAppenderDefault
    {
    public:
        typedef Buffer_T        BufferType;
        typedef BufferChar_T    BufferChar;
        typedef typename std::make_unsigned<BufferChar>::type UnsignedBufferChar;

        // If true, calling operator() is guaranteed to append, i.e. size_before_operator() < size_after_operator().
        static const bool s_isAppendOperatorGuaranteedToIncrementSize = true;
        static constexpr bool s_hasTrivialPopBack = true;

        template <class Char_T>
        void operator()(Buffer_T& buffer, const Char_T& ch)
        {
            // Check whether read value fits in data type used in buffer.
            // Note that using unsigned char so that in common case of Char = signed char
            // values [0x80, 0xFF] will be read.
            if (isValWithinLimitsOfType(ch, UnsignedBufferChar()))
            {
                buffer.push_back(static_cast<BufferChar>(ch));
            }
            else // Case: read char doesn't fit in buffer char-type.
            {
                // TODO: Make this behaviour customisable (callback which receives this and read character, or handle through return value?)
                buffer.push_back('?');
            }
        }

        // Reads *p.
        template <class Char_T>
        void operator()(Buffer_T& buffer, const Char_T* p)
        {
            DFG_ASSERT_UB(p != nullptr);
            typedef typename std::remove_reference<decltype(*p)>::type PlainType;
            typedef typename std::make_unsigned<PlainType>::type UnsignedCharT;
            operator()(buffer, UnsignedCharT(*p)); // With std-streams, negative char-values gets passed as positive ints so to maintain the behaviour, translate to unsigned.
        }
    }; // Class CharAppenderDefault

    // Replacement for std::basic_string. Problem with populating std::string in this context is that it keeps the buffer null-terminated on every push_back.
    // This is inefficient in this use pattern and with this custom buffer chars can be pushed back without handling the null terminator. Null terminator can be set when the
    // the cell is read completely.
    template <class Char_T, size_t SsoBufferSize_T = 32>
    class CharBuffer : public std::vector<Char_T>
    {
    public:
        CharBuffer()
        {
            this->reserve(SsoBufferSize_T);
        }

        std::basic_string<Char_T> toStr() const
        {
            return std::basic_string<Char_T>(this->data(), this->size());
        }

        StringView<Char_T> toView() const
        {
            return StringView<Char_T>(this->data(), this->size());
        }

        operator std::basic_string<Char_T>() const
        {
            return toStr();
        }

        operator StringView<Char_T>() const
        {
            return toView();
        }
    }; // Class CharBuffer

    // Specialized string view buffer for case where input data is in memory and can be read untranslated -> use string view to underlying data instead of populating a separate buffer.
    class StringViewCBuffer : public StringViewC
    {
    public:
        typedef char* iterator;

        void reset(const char* p, const size_t n)
        {
            m_pFirst = p;
            m_nSize = n;
        }

        void incrementSize()
        {
            DFG_ASSERT_UB(m_pFirst != nullptr);
            m_nSize++;
        }
    }; // StringViewCBuffer

    // Char appender for StringViewCBuffer.
    class CharAppenderStringViewCBuffer
    {
    public:
        typedef StringViewCBuffer BufferType;

        static const bool s_isAppendOperatorGuaranteedToIncrementSize = true;
        static constexpr bool s_hasTrivialPopBack = true;

        void operator()(StringViewCBuffer& sv, const char* p)
        {
            DFG_UNUSED(p);
            sv.incrementSize();
        }
    }; // Class CharAppenderStringViewC

    // A buffer that can be used like StringViewCBuffer, but also supporting enclosed cells.
    // Note that the only case where StringViewCBuffer is not enough, is when there are double enclosed item in cell - otherwise everything can be done StringViewCBuffer.
    // The logic is that when double enclosed item is found, a flag m_bNeedTemporaryBuffer is set and when read is done, cell is reparsed to temporary buffer.
    class StringViewCBufferWithEnclosedCellSupport : public StringViewCBuffer
    {
    public:
        using BaseClass = StringViewCBuffer;

        void clear()
        {
            BaseClass::clear();
            m_temporaryBuffer.clear();
            m_bNeedTemporaryBuffer = false;
        }

        void onEnclosedCellRead(const int cEnc)
        {
            if (m_bNeedTemporaryBuffer && !this->empty())
            {
                m_temporaryBuffer.clear();
                const auto pEnd = this->end();
                for (auto p = this->begin(); p != pEnd; ++p)
                {
                    // Note: unless parsing is broken, enclosing chars in parsed view come in pairs.
                    if (*p != cEnc)
                        m_temporaryBuffer.push_back(*p);
                    else
                    {
                        // Current char is enclosing; pushing it to buffer and skipping the following.
                        m_temporaryBuffer.push_back(*p);
                        DFG_ASSERT_CORRECTNESS(p + 1 != pEnd && *(p + 1) == cEnc);
                        if (p + 1 != pEnd) // This should always be true
                            ++p;
                    }
                }
                this->reset(m_temporaryBuffer.data(), m_temporaryBuffer.size());
                m_bNeedTemporaryBuffer = false; // Resetting flag so that following cell won't use temporary buffer unless needed.
            }
        }

        CharBuffer<char> m_temporaryBuffer;
        bool m_bNeedTemporaryBuffer = false;
    }; // class StringViewCBufferWithEnclosedCellSupport

    // Char appender for StringViewCBufferWithEnclosedCellSupport.
    class CharAppenderStringViewCBufferWithEnclosedCellSupport : public CharAppenderStringViewCBuffer
    {
    public:
        typedef StringViewCBufferWithEnclosedCellSupport BufferType;
        static const bool s_isAppendOperatorGuaranteedToIncrementSize = true;
        static constexpr bool s_hasTrivialPopBack = true;
    }; // Class CharAppenderStringViewCBufferWithEnclosedCellSupport

    template <class Buffer_T>
    class CharAppenderUtf
    {
    public:
        typedef Buffer_T BufferType;
        using BufferChar = typename ::DFG_MODULE_NS(cont)::ElementType<BufferType>::type;

        static const bool s_isAppendOperatorGuaranteedToIncrementSize = false; // TODO: revise.
        static constexpr bool s_hasTrivialPopBack = (sizeof(BufferChar) == 4);

        template <class Char_T>
        void operator()(Buffer_T& buffer, const Char_T& ch)
        {
            DFG_MODULE_NS(utf)::cpToUtf(ch, std::back_inserter(buffer), sizeof(BufferChar), DFG_ROOT_NS::ByteOrderHost);
        }

        // Reads *p.
        template <class Char_T>
        void operator()(Buffer_T& buffer, const Char_T* p)
        {
            DFG_ASSERT_UB(p != nullptr);
            typedef typename std::remove_reference<decltype(*p)>::type PlainType;
            typedef typename std::make_unsigned<PlainType>::type UnsignedCharT;
            operator()(buffer, UnsignedCharT(*p)); // With std-streams, negative char-values gets passed as positive ints so to maintain the behaviour, translate to unsigned.
        }

        InternalCharType popLastChar(Buffer_T& buffer)
        {
            if (buffer.empty())
                return DelimitedTextReader::s_nMetaCharNone;
            if constexpr (sizeof(BufferChar) == 1)
            {
                auto iter = buffer.end() - 1;
                for (; iter != buffer.begin(); --iter)
                {
                    const auto nLeading = ((*iter & 0xff) >> 6);
                    if (nLeading != 2)
                        break;
                }
                const auto c = utf8::unchecked::peek_next(iter);
                buffer.erase(iter, buffer.end());
                return saturateCast<InternalCharType>(c);
            }
            else if constexpr (sizeof(BufferChar) == 2)
            {
                auto baseChar = *(buffer.end() - 1);
                buffer.pop_back();
                if (!utf8::internal::is_trail_surrogate(baseChar))
                    return bufferCharToInternal(baseChar);

                std::array<char16_t, 2> u16buf;
                u16buf[1] = baseChar;
                DFG_ASSERT_CORRECTNESS(!buffer.empty() && utf8::internal::is_lead_surrogate(buffer.back()));
                if (buffer.empty())
                    return ::DFG_MODULE_NS(utf)::DFG_DETAIL_NS::gDefaultUnrepresentableCharReplacementUtf; // Should not happen for valid input.
                u16buf[0] = *(buffer.end() - 1);
                buffer.pop_back();
                uint32_t trail_surrogate = utf8::internal::mask16(u16buf[1]);
                const auto cp = (u16buf[0] << 10) + trail_surrogate + utf8::internal::SURROGATE_OFFSET;
                return bufferCharToInternal(cp);
            }
            else
            {
                DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(BufferChar, "Only buffer char sizes 1 and 2 are supported");
            }
        }
    }; // Class CharAppenderUtf

    class CharAppenderNone
    {
    public:
        static const bool s_isAppendOperatorGuaranteedToIncrementSize = false;

        template <class Buffer_T, class Char_T>
        void operator()(Buffer_T&, const Char_T&)
        {
        }
    };

    /*
        -Cell content related functionality such as:
            -Handling underlying character buffer
            -Storing format specification
    */
    template <class BufferChar_T, class InputChar_T = BufferChar_T, class Buffer_T = CharBuffer<BufferChar_T>, class CharAppender_T = CharAppenderDefault<Buffer_T, BufferChar_T>, class FormatDef_T = FormatDefinitionSingleChars>
    class CellData
    {
    public:
        typedef FormatDef_T FormatDef;
        typedef BufferChar_T BufferChar;
        typedef InputChar_T	InputChar;
        typedef Buffer_T	Buffer;
        typedef CharAppender_T  CharAppender;
        typedef typename std::make_unsigned<BufferChar>::type UnsignedBufferChar;
        typedef typename std::make_unsigned<InputChar>::type UnsignedInputChar;
        typedef typename Buffer::iterator iterator;
        typedef typename Buffer::const_iterator const_iterator;

        // Note: Check is done because according to VC2010 documentation make_unsigned doesn't guarantee
        //       that the defined type has size equal to original size.
        static_assert(sizeof(BufferChar) == sizeof(UnsignedBufferChar), "Unexpected UnsignedChar-size");

        CellData(const FormatDef& formatDef) :
            m_formatDef(formatDef),
            m_status(cellHrvContinue)
        {
            privCellDataInit(false);
        }

        CellData(int cSep, int cEnc, int cEol) :
            m_formatDef(cEnc, cEol, cSep),
            m_status(cellHrvContinue)
        {
            privCellDataInit(true);
        }

        void privCellDataInit(const bool bInitFormatFlags)
        {
            if (bInitFormatFlags)
                m_formatDef.setFlag(rfSkipLeadingWhitespaces, true);
            if (m_formatDef.getSep() != ' ')
                m_whiteSpaces.push_back(' ');
            if (m_formatDef.getSep() != '\t')
                m_whiteSpaces.push_back('\t');
        }

        FormatDef& getFormatDefInfo()
        {
            return m_formatDef;
        }

        const FormatDef& getFormatDefInfo() const
        {
            return m_formatDef;
        }

        bool isLastCharacterWhitespace() const
        {
            auto iter = iteratorToLastChar();
            if (iter != end())
                return (m_whiteSpaces.find(*iter) != m_whiteSpaces.npos);
            return false;
        }

        // Returns true if character was removed, false otherwise.
        bool removeWhitespaceCharacter(const BufferChar cRem)
        {
            const auto nSizeBefore = m_whiteSpaces.size();
            m_whiteSpaces.erase(std::remove_if(m_whiteSpaces.begin(), m_whiteSpaces.end(), [&](const BufferChar c) {return c == cRem; }));
            return (nSizeBefore != m_whiteSpaces.size());
        }

        // Precondition: !empty()
        InternalCharType popLastChar()
        {
            DFG_ASSERT_UB(!m_buffer.empty());
            if constexpr (CharAppender::s_hasTrivialPopBack)
            {
                const auto c = m_buffer.back();
                m_buffer.pop_back();
                return bufferCharToInternal(c);
            }
            else
                return m_charAppender.popLastChar(m_buffer);
        }

        // Precondition: !empty()
        void popFrontChar()
        {
            DFG_ASSERT_UB(!m_buffer.empty());
            DFG_MODULE_NS(cont)::popFront(m_buffer);
        }

        BufferChar back() const
        {
            DFG_ASSERT_UB(!empty());
            return m_buffer.back();
        }

        // Implementation for default buffer type
        void onCellReadImpl(std::true_type)
        {
            // Uncomment below if buffer should be null terminated.
            #if 0
                m_buffer.push_back('\0');
                m_buffer.m_nSize--; // Hack: Decrement size so that null terminator gets leaved out from real size.
            #endif
        }

        // Implementation for non-default buffer type
        void onCellReadImpl(std::false_type)
        {
            // Nothing to do here.
        }

        template <class T>
        struct IsDefaultCharBuffer { static const bool value = false; };

        template <class Char_T, size_t N>
        struct IsDefaultCharBuffer<CharBuffer<Char_T, N>> { static const bool value = true; };

        void onCellRead()
        {
            onCellReadImpl(std::integral_constant<bool, IsDefaultCharBuffer<Buffer>::value>());
        }

        template <class Reader_T>
        void onCellReadBeginImpl(Reader_T& reader, std::true_type)
        {
            getBuffer().reset(reader.getStream().m_streamBuffer.m_pCurrent, 0);
        }

        template <class Reader_T>
        void onCellReadBeginImpl(Reader_T& reader, std::false_type)
        {
            reader.getCellBuffer().clear();
        }

        template <class Reader_T>
        void onCellReadBegin(Reader_T& reader)
        {
            onCellReadBeginImpl(reader, std::integral_constant<bool, std::is_base_of<StringViewCBuffer, Buffer>::value &&
                                                                     DFG_DETAIL_NS::IsStreamStringViewCCompatible<typename Reader_T::StreamT>::value>());
            
        }

        template <class Reader_T>
        void onEnclosedCellReadBegin(Reader_T& reader)
        {
            onCellReadBegin(reader);
        }

        size_t size() const
        {
            return sizeInChars();
        }

        size_t sizeInChars() const
        {
            return m_buffer.size();
        }

        bool empty() const
        {
            return m_buffer.empty();
        }

        void clear()
        {
            m_buffer.clear();
        }

        bool isEqual(const int ch) const
        {
            return (m_buffer.size() == 1 && bufferCharToInternal(m_buffer.front()) == ch);
        }

        bool isLastChar(const int ch) const
        {
            return (!m_buffer.empty() && (bufferCharToInternal(m_buffer.back()) == ch));
        }

        bool isOneBeforeLast(const int ch) const
        {
            return m_buffer.size() >= 2 && bufferCharToInternal(m_buffer[m_buffer.size()-2]) == ch;
        }

        Buffer& getBuffer()
        {
            return m_buffer;
        }

        const Buffer& getBuffer() const
        {
            return m_buffer;
        }

        template <class Char_T>
        void appendChar(const Char_T ch)
        {
            m_charAppender(getBuffer(), ch);
        }

        void appendChar(const BufferChar* p)
        {
            m_charAppender(getBuffer(), p);
        }

        iterator        end()        { return m_buffer.end();  }
        const_iterator  end()  const { return m_buffer.end();  }
        const_iterator  cend() const { return m_buffer.cend(); }

        bool isCharAt(const_iterator iter, const int c) const
        {
            return (iter != end() && bufferCharToInternal(*iter) == c);
        }

        iterator iteratorToLastChar()
        {
            return (!m_buffer.empty()) ? m_buffer.end() - 1 : m_buffer.end();
        }

        const_iterator iteratorToLastChar() const
        {
            return (!m_buffer.empty()) ? m_buffer.end() - 1 : m_buffer.end();
        }

        cellHandlerRv getReadStatus() const
        {
            return m_status;
        }

        void setReadStatus(cellHandlerRv newStatus)
        {
            m_status = newStatus;
        }

        bool doesBufferEndWithEndOfLineItem() const
        {
            return isLastChar(m_formatDef.getEol());
        }

        Buffer m_buffer;

        std::basic_string<BufferChar> m_whiteSpaces;
        FormatDef m_formatDef;
        cellHandlerRv m_status;
        CharAppender m_charAppender;
    }; // Class CellData

    enum class CellType
    {
        naked,
        enclosed
    };

    template <class Buffer_T>
    class GenericParsingImplementations
    {
    public:
        typedef Buffer_T CellBuffer;
        typedef typename CellBuffer::FormatDef FormatDef;


        static constexpr bool isEmptyGivenSuccessfulReadCharCall(const CellBuffer&, std::true_type)
        {
            return false;
        }

        static DFG_FORCEINLINE bool isEmptyGivenSuccessfulReadCharCall(const CellBuffer& buffer, std::false_type)
        {
            return buffer.empty();
        }

        static DFG_FORCEINLINE bool isEmptyGivenSuccessfulReadCharCall(const CellBuffer& buffer)
        {
            return isEmptyGivenSuccessfulReadCharCall(buffer, std::integral_constant<bool, CellBuffer::CharAppender::s_isAppendOperatorGuaranteedToIncrementSize>());
        }

        static DFG_FORCEINLINE void separatorAutoDetection(const ReadState, FormatDef&, CellBuffer&, std::true_type)
        {
            // Nothing to do here since compile time separator info implies that it can't be runtime detected.
        }

        static DFG_FORCEINLINE void separatorAutoDetection(const ReadState rs, FormatDef& formatDef, CellBuffer& buffer, std::false_type)
        {
            if (formatDef.getSep() == s_nMetaCharAutoDetect)
            {
                if (buffer.empty())
                    return;
                const auto cLast = buffer.getBuffer().back();
                if (rs != rsInEnclosedCell && (cLast == ',' || cLast == ';' || cLast == '\t' || cLast == '\x1f')) // \x1f is unit separator (https://en.wikipedia.org/wiki/Unit_Separator)
                {
                    formatDef.m_cSep = cLast;
                    if (cLast == '\t') // If separator is autodetected to be \t, remove it from the list of whitespace-characters.
                        buffer.removeWhitespaceCharacter('\t');
                }
            }
        }

        // Handles separator auto detection.
        static DFG_FORCEINLINE void separatorAutoDetection(const ReadState rs, FormatDef& formatDef, CellBuffer& buffer)
        {
            separatorAutoDetection(rs, formatDef, buffer, std::integral_constant<bool, FormatDef::s_hasCompileTimeSeparatorChar>());
        }

        // Returns true if read char got trimmed, false otherwise.
        static DFG_FORCEINLINE bool preTrimmer(const FormatDef& formatDef, CellBuffer& buffer)
        {
            // Check for skippable chars such as whitespace before any cell data or whitespace after enclosed cell.
            if (formatDef.testFlag(rfSkipLeadingWhitespaces))
            {
                if (buffer.isLastCharacterWhitespace())
                {
                    buffer.clear();
                    return true;
                }
            }
            return false;
        }

        static DFG_FORCEINLINE CellType detectCellTypeByStart(CellBuffer& buffer)
        {
            return (!isEmptyGivenSuccessfulReadCharCall(buffer) && bufferCharToInternal(buffer.back()) == buffer.getFormatDefInfo().getEnc()) ? CellType::enclosed : CellType::naked;
        }

        // Checks whether given buffer char is separator and if yes, sets readstate. Returns true if caller should invoke 'break', false otherwise.
        template <class Char_T>
        static DFG_FORCEINLINE bool separatorChecker(ReadState& rs, const FormatDef& formatDef, const Char_T bufferChar)
        {
            if (bufferCharToInternal(bufferChar) != formatDef.getSep())
                return false;
            else
            {
                rs = rsSeparatorEncountered;
                return true;
            }
        }

        // Checks whether most recent item in buffer is separator and if yes, pops it and sets readstate. Returns true if caller should invoke 'break', false otherwise.
        static DFG_FORCEINLINE bool separatorChecker(ReadState& rs, CellBuffer& buffer)
        {
            if (isEmptyGivenSuccessfulReadCharCall(buffer) || bufferCharToInternal(buffer.back()) != buffer.getFormatDefInfo().getSep())
                return false;
            else
            {
                buffer.popLastChar();
                rs = rsSeparatorEncountered;
                return true;
            }
        }

        // Checks whether most recent item in buffer is end-of-line and if yes, pops it and sets readstate; optionally also removes \r in case of \r\n eol.
        // Returns true if caller should invoke 'break', false otherwise.
        static DFG_FORCEINLINE bool eolChecker(ReadState& rs, CellBuffer& buffer)
        {
            if (isEmptyGivenSuccessfulReadCharCall(buffer) || bufferCharToInternal(buffer.back()) != buffer.getFormatDefInfo().getEol())
                return false;
            else
            {
                buffer.popLastChar();
                // \r\n handling.
                if (buffer.getFormatDefInfo().getEol() == '\n' && buffer.getFormatDefInfo().isRnTranslationEnabled() && buffer.isLastChar('\r'))
                    buffer.popLastChar();
                rs = rsEndOfLineEncountered;
                return true;
            }
        }

        // Overload for checking if given buffer char is eol.
        template <class Char_T>
        static DFG_FORCEINLINE bool eolChecker(ReadState& rs, const FormatDef& formatDef, const Char_T bufferChar)
        {
            if (bufferCharToInternal(bufferChar) != formatDef.getEol())
                return false;
            else
            {
                rs = rsEndOfLineEncountered;
                return true;
            }
        }

        template <class T>
        static void onEnclosedCellRead(const T&, const int) {}

        static void onEnclosedCellRead(StringViewCBufferWithEnclosedCellSupport& buffer, const int cEnc)
        {
            buffer.onEnclosedCellRead(cEnc);
        }

        template <class Char_T>
        static void onPastEnclosedCellCharacter(CellBuffer&, const Char_T) {}

        template <class T, class T2>
        static void onDoubleClosingItemFoundImpl(T& controller, const T2&)
        {
            controller.popLastChar();
        }

        template <class T>
        static void onDoubleClosingItemFoundImpl(T&, StringViewCBufferWithEnclosedCellSupport& charBuffer)
        {
            charBuffer.m_bNeedTemporaryBuffer = true;
        }

        template <class T>
        static void onDoubleClosingItemFound(T& controller)
        {
            onDoubleClosingItemFoundImpl(controller, controller.getBuffer());
        }

        template <class Reader_T>
        static DFG_FORCEINLINE void returnValueHandler(Reader_T& reader)
        {
            const auto bufferReadStatus = reader.getCellBuffer().getReadStatus();
            if (bufferReadStatus == cellHrvSkipRestOfLine || bufferReadStatus == cellHrvSkipRestOfLineAndTerminate)
            {
                if (!reader.isReadStateEol()) // Don't skip if already on eol, otherwise the next line will be skipped.
                    readUntilEolOrEof(reader);
                DFG_ASSERT_CORRECTNESS(reader.isReadStateEolOrEof());
            }
            if (bufferReadStatus == cellHrvTerminateRead || bufferReadStatus == cellHrvSkipRestOfLineAndTerminate)
            {
                reader.m_readState = rsTerminated;
            }
        }

        template <class Reader_T, class CellHandler_T>
        static DFG_FORCEINLINE void read(Reader_T& reader, CellHandler_T&& cellHandler)
        {
            size_t nRow = 0;
            auto cellHandlerWrapper = [&](size_t nCol, decltype(reader.getCellBuffer())& cellData)
            {
                cellHandler(nRow, nCol, cellData);
            };

            for (; ; ++nRow)
            {
                readRow(reader, cellHandlerWrapper);
                if (reader.isReadStateEof() || reader.isReadStateTerminated() || !reader.isStreamGood())
                    break;
                else
                {
                    reader.m_readState = rsLookingForNewData;
                    reader.getCellBuffer().setReadStatus(cellHrvContinue);
                }
            }
        }
    }; // class GenericParsingImplementations


    // Extension of GenericParsingImplementations supporting handling of past-enclosed-cell characters.
    template <class CellController_T>
    class GenericParsingWithPastEnclosedHandling : public GenericParsingImplementations<CellController_T>
    {
    public:
        using BaseClass = GenericParsingImplementations<CellController_T>;
        using Buffer = typename CellController_T::Buffer;
        using BufferChar = typename CellController_T::BufferChar;

        template <class Char_T>
        static void onPastEnclosedCellCharacter(CellController_T& rCellController, const Char_T c)
        {
            if constexpr (std::is_same_v<Buffer, StringViewCBufferWithEnclosedCellSupport>)
            {
                // Copying characters past enclosed cell to temporary buffer which will later be used
                // in onEnclosedCellRead().
                auto& rTempBuffer = rCellController.getBuffer().m_temporaryBuffer;
                CharAppenderDefault<decltype(rTempBuffer), char>()(rTempBuffer, c);
            }
            else
            {
                rCellController.appendChar(c);
            }
        }

        static void onEnclosedCellRead(Buffer& rBuffer, const int cEnc)
        {
            if constexpr (std::is_same_v<Buffer, StringViewCBufferWithEnclosedCellSupport>)
            {
                auto& rTempBuffer = rBuffer.m_temporaryBuffer;
                if (rTempBuffer.empty())
                    BaseClass::onEnclosedCellRead(rBuffer, cEnc); // No 'past enclosed cell' -characters found -> normal handling.
                else // Case: there are past-enclosed characters, appending them after the actual content.
                {
                    auto excessBuffer = rTempBuffer; // Takes copy of current past-enclosed cell characters
                    rTempBuffer.clear(); // Clears temporary buffer to make sure normal handler won't misuse temp buffer content.
                    BaseClass::onEnclosedCellRead(rBuffer, cEnc);
                    if (rTempBuffer.empty()) // If temp buffer is empty, need to copy current content, otherwise buffer is already using temp buffer.
                        rTempBuffer.assign(rBuffer.begin(), rBuffer.end());
                    std::copy(excessBuffer.begin(), excessBuffer.end(), std::back_inserter(rTempBuffer)); // Copies past-enclosed to current buffer.
                    rBuffer.reset(rTempBuffer.data(), rTempBuffer.size()); // Sets buffer to manually tweaked content.
                }
            }
            else
            {
                DFG_UNUSED(rBuffer);
                DFG_UNUSED(cEnc);
            }
        }
    }; // class GenericParsingWithPastEnclosedHandling

    /* Basic parsing implementations that may yield better read performance with the following restrictions:
     *      -No pre-cell or post-cell trimming (e.g. in case of ', a' pre-cell trimming could remove leading whitespaces and read cell as "a" instead of " a")
     *      -No enclosed cell support (e.g. can't have separators or new lines within cells and enclosing characters will be read like any other non-control character)
     *      -No cell handler return value handling (e.g. can't terminate in middle of reading)
     */
    template <class Buffer_T>
    class BarebonesParsingImplementations
    {
    public:
        typedef Buffer_T CellBuffer;
        typedef typename CellBuffer::FormatDef FormatDef;

        static DFG_FORCEINLINE bool isEmptyGivenSuccessfulReadCharCall(const CellBuffer& buffer)
        {
            return GenericParsingImplementations<Buffer_T>::isEmptyGivenSuccessfulReadCharCall(buffer);
        }

        static DFG_FORCEINLINE void separatorAutoDetection(const ReadState, const FormatDef&, CellBuffer&)
        {
            // Auto detection is not supported in bare-bones parsing.
        }

        // Returns true if read char got trimmed ignored, false otherwise.
        static constexpr bool preTrimmer(const FormatDef&, CellBuffer&)
        {
            return false;
        }

        static constexpr CellType detectCellTypeByStart(CellBuffer&)
        {
            return CellType::naked;
        }

        // Returns true if caller should invoke 'break', false otherwise.
        static DFG_FORCEINLINE bool separatorChecker(ReadState& rs, CellBuffer& buffer)
        {
            return GenericParsingImplementations<Buffer_T>::separatorChecker(rs, buffer);
        }

        template <class Char_T>
        static DFG_FORCEINLINE bool separatorChecker(ReadState& rs, const FormatDef& formatDef, Char_T bufferChar)
        {
            return GenericParsingImplementations<Buffer_T>::separatorChecker(rs, formatDef, bufferChar);
        }

        // Returns true if caller should invoke 'break', false otherwise.
        static DFG_FORCEINLINE bool eolChecker(ReadState& rs, CellBuffer& buffer)
        {
            return GenericParsingImplementations<Buffer_T>::eolChecker(rs, buffer);
        }

        template <class Char_T>
        static DFG_FORCEINLINE bool eolChecker(ReadState& rs, const FormatDef& formatDef, Char_T bufferChar)
        {
            return GenericParsingImplementations<Buffer_T>::eolChecker(rs, formatDef, bufferChar);
        }

        template <class Reader_T>
        static DFG_FORCEINLINE void returnValueHandler(const Reader_T&)
        {
            // Barebones parsing does not support return values.
        }

        // Barebones parsing does not support enclosed cells so these should never get called.
        template <class T> static DFG_FORCEINLINE void onEnclosedCellRead(const T&, const int) { DFG_ASSERT_CORRECTNESS(false); }
        template <class T> static DFG_FORCEINLINE void onPastEnclosedCellCharacter(CellBuffer&, const T&)   { DFG_ASSERT_CORRECTNESS(false); }

        // Specialization for whole stream reading when handling untranslated stream with StringViewCBuffer (i.e. can use StringView to source bytes).
        // TODO: test
        template <class Reader_T, class CellHandler_T>
        static DFG_FORCEINLINE void read(Reader_T& reader, CellHandler_T&& cellHandler, std::true_type)
        {
            BasicImStream& strm = reader.getStream();
            StringViewCBuffer& buffer = reader.getCellBuffer().getBuffer();
            const auto formatDef = reader.getCellBuffer().getFormatDefInfo();
            size_t nRow = 0;
            size_t nCol = 0;
            const auto pFirst = strm.currentPtr();
            auto p = pFirst;
            const auto pEnd = strm.endPtr();
            auto pCellStart = p;
            for (; p != pEnd; ++p)
            {
                if (bufferCharToInternal(*p) != formatDef.getSep() && bufferCharToInternal(*p) != formatDef.getEol())
                    continue;

                buffer.reset(pCellStart, p - pCellStart);

                // \r\n handling.
                if (formatDef.isRnTranslationEnabled() && formatDef.getEol() == '\n' && bufferCharToInternal(*p) == '\n' && p != pCellStart && *(p - 1) == '\r')
                    buffer.pop_back(); // pop \r

                cellHandler(nRow, nCol, reader.getCellBuffer());
                pCellStart = p + 1;
                if (bufferCharToInternal(*p) == formatDef.getEol())
                {
                    ++nRow;
                    nCol = 0;
                }
                else
                    nCol++;
            }
            // Call handler if any of the following conditions are true:
            //    -buffer is not empty (cell ends to eof)
            //    -last char is separator (interpret that "a," is two cells)
            if (pCellStart != p || (pEnd != pFirst && (bufferCharToInternal(*(pEnd - 1)) == formatDef.getSep())))
            {
                buffer.reset(pCellStart, p - pCellStart);
                cellHandler(nRow, nCol, reader.getCellBuffer());
            }
            strm.seekToEnd();
        }

        template <class T>
        static DFG_FORCEINLINE void onDoubleClosingItemFound(const T&) { DFG_ASSERT_CORRECTNESS(false); }

        // Default implementation, see true_type-overload for documentation.
        template <class Reader_T, class CellHandler_T>
        static DFG_FORCEINLINE void read(Reader_T& reader, CellHandler_T&& cellHandler, std::false_type)
        {
            GenericParsingImplementations<Buffer_T>::read(reader, std::forward<CellHandler_T>(cellHandler));
        }

        template <class Reader_T, class CellHandler_T>
        static DFG_FORCEINLINE void read(Reader_T& reader, CellHandler_T&& cellHandler)
        {
            typedef std::integral_constant<bool, DFG_DETAIL_NS::IsStreamStringViewCCompatible<typename Reader_T::StreamT>::value && std::is_same<StringViewCBuffer, typename Reader_T::CellBuffer::Buffer>::value> TagType;
            read(reader, std::forward<CellHandler_T>(cellHandler), TagType());
        }

    }; // BarebonesParsingImplementations

    template <class CellBuffer_T,
              class Stream_T,
              class CellParsingImplementations_T = GenericParsingImplementations<CellBuffer_T>>
    class CellReader : public CellReaderBase
    {
    public:
        typedef CellReaderBase BaseClass;
        typedef CellBuffer_T                    CellBuffer;
        typedef typename CellBuffer::InputChar  InputChar;
        typedef Stream_T                        StreamT;
        typedef typename CellBuffer::FormatDef  FormatDef;
        typedef CellParsingImplementations_T    CellParsingImplementations;

        CellReader(StreamT& rStrm, CellBuffer& rCd) :
            m_rStrm(rStrm),
            m_rCellBuffer(rCd)
        {
        }

        StreamT& getStream()
        {
            return m_rStrm;
        }

        // readChar implementation for case when reading bytes from BasicImStream (contiguous memory).
        template <class Buffer_T>
        DFG_DELIMITED_TEXT_READER_INLINING bool readCharImpl(Buffer_T& buffer, const ReadState rs, std::true_type)
        {
            auto& strm = static_cast<BasicImStream&>(getStream());
            if (!strm.m_streamBuffer.isAtEnd())
            {
                buffer.appendChar(strm.m_streamBuffer.m_pCurrent);
                ++strm.m_streamBuffer.m_pCurrent;
                CellParsingImplementations::separatorAutoDetection(rs, buffer.getFormatDefInfo(), buffer);
                return true;
            }
            else
                return false;
        }

        // readChar implementation for default case.
        template <class BufferT>
        DFG_DELIMITED_TEXT_READER_INLINING bool readCharImpl(BufferT& buffer, const ReadState rs, std::false_type)
        {
            // Note: When stream has current codecvt-facet, get() will read one
            // character which may be less or more that sizeof(ch)-bytes. The result may also
            // be truncated.
            const auto ch = readOne(getStream());
            const bool bRead = (ch != eofChar(getStream()));
            if (bRead)
            {
                buffer.appendChar(ch);
                CellParsingImplementations::separatorAutoDetection(rs, buffer.getFormatDefInfo(), buffer);
            }
            return bRead;
        }

        // Reads char from stream and returns true if successful, false otherwise.
        // Note that return value does not tell whether read char was successfully stored
        // in buffer, but returned 'true' guarantees that buffer.appendChar() was called.
        template <class Buffer_T>
        DFG_DELIMITED_TEXT_READER_INLINING bool readChar(Buffer_T& buffer, const ReadState rs)
        {
            return readCharImpl(buffer, rs, std::integral_constant<bool, DFG_DETAIL_NS::IsStreamStringViewCCompatible<StreamT>::value>());
        }

        // Reads char using given buffer and current read state.
        template <class BufferT>
        DFG_DELIMITED_TEXT_READER_INLINING bool readChar(BufferT& buffer)
        {
            return readChar(buffer, m_readState);
        }

        DFG_DELIMITED_TEXT_READER_INLINING bool readChar()
        {
            return readChar(getCellBuffer());
        }

        // Skips one char from stream and returns true iff read. If pChar is given, skipped char as returned from stream is stored there.
        template <class Char_T>
        DFG_DELIMITED_TEXT_READER_INLINING bool skipChar(Char_T* pChar)
        {
            auto c = readOne(getStream());
            const bool bRead = (c != eofChar(getStream()));
            if (pChar)
                *pChar = c;
            return bRead;
        }

        CellBuffer& getCellBuffer()				{return m_rCellBuffer;}
        const CellBuffer& getCellBuffer() const	{return m_rCellBuffer;}
        CellBuffer getCellBufferCopy() const    {return m_rCellBuffer;}

        FormatDef& getFormatDefInfo()
        {
            return m_rCellBuffer.getFormatDefInfo();
        }

        const FormatDef& getFormatDefInfo() const
        {
            return m_rCellBuffer.getFormatDefInfo();
        }

        bool isStreamGood() const
        {
            return isStreamInGoodState(m_rStrm);
        }

        StreamT& m_rStrm;
        CellBuffer& m_rCellBuffer;
    }; // class CellReader

    // Helper function for creating reader from stream and cell data.
    template <class Stream_T, class CellData_T>
    static CellReader<typename std::remove_reference<CellData_T>::type, Stream_T> createReader(Stream_T& rStrm, CellData_T&& cellData)
    {
        typedef typename std::remove_reference<CellData_T>::type CellDataT;
        return CellReader<CellDataT, Stream_T>(rStrm, cellData);
    }

    // Helper function for creating reader from stream and cell data using only basic parsing functionality.
    template <class Stream_T, class CellData_T>
    static auto createReader_basic(Stream_T& rStrm, CellData_T&& cellData) -> CellReader<typename std::remove_reference<CellData_T>::type, Stream_T, BarebonesParsingImplementations<typename std::remove_reference<CellData_T>::type>>
    {
        DFG_ASSERT_WITH_MSG(cellData.getFormatDefInfo().getEnc() == s_nMetaCharNone, "Basic parsing does not support enclosing character, but format definition has defined one.");
        DFG_ASSERT_WITH_MSG(cellData.getFormatDefInfo().getSep() != s_nMetaCharAutoDetect, "Basic parsing does not support separator auto-detection.");
        DFG_ASSERT_WITH_MSG(cellData.getFormatDefInfo().getEol() != s_nMetaCharAutoDetect, "Basic parsing does not support end-of-line auto-detection.");

        typedef typename std::remove_reference<CellData_T>::type CellDataT;
        return CellReader<CellDataT, Stream_T, BarebonesParsingImplementations<CellDataT>>(rStrm, cellData);
    }

    // Read csv-style cell using given reader.
    template <class Reader>
    static DFG_DELIMITED_TEXT_READER_INLINING void readCell(Reader& reader)
    {
        typedef typename Reader::CellParsingImplementations ParsingImplementations;

        reader.m_readState = rsLookingForNewData;

        reader.getCellBuffer().onCellReadBegin(reader);

        // Pre-trimming
        bool bGoodRead = false;
        for (bGoodRead = reader.readChar(); bGoodRead && ParsingImplementations::preTrimmer(reader.getFormatDefInfo(), reader.getCellBuffer());)
        {
            reader.getCellBuffer().onCellReadBegin(reader);
            if (reader.isReadStateEndOfCell())
                break;
            bGoodRead = reader.readChar();
        }

        if (bGoodRead)
        {
            const auto cellType = ParsingImplementations::detectCellTypeByStart(reader.getCellBuffer());
            if (cellType == CellType::naked)
            {
                if (!ParsingImplementations::separatorChecker(reader.m_readState, reader.getCellBuffer()) &&
                    !ParsingImplementations::eolChecker(reader.m_readState, reader.getCellBuffer()))
                {
                    while (!reader.isReadStateEndOfCell() &&
                        reader.readChar() &&
                        !ParsingImplementations::separatorChecker(reader.m_readState, reader.getCellBuffer()) && // separatorChecker() call is guaranteed to be done after successful reader.readChar() call. 
                        !ParsingImplementations::eolChecker(reader.m_readState, reader.getCellBuffer())) // eolChecker() call is guaranteed to be done after successful reader.readChar() call. 
                    {
                    }
                }
            }
            else // case: in enclosed cell.
            {
                auto& buffer = reader.getCellBuffer();
                auto& rs = reader.m_readState;
                reader.getCellBuffer().onEnclosedCellReadBegin(reader);
                rs = rsInEnclosedCell;
                while (!reader.isReadStateEndOfCell() && reader.readChar())
                {
                    if (ParsingImplementations::isEmptyGivenSuccessfulReadCharCall(buffer) || bufferCharToInternal(buffer.back()) != buffer.getFormatDefInfo().getEnc())
                        continue;
                    // Found enclosing char; reading next char to determine if it was ending one.
                    if (!reader.readChar(buffer, rsLookingForNewData)) // Using explicitly set read state to avoid separator auto detection being ignored in case of separator following ending enclosing (test cases in DelimitedTextReader_autoDetectCsvSeparator)
                    {
                        // Couldn't read char after enclosing item -> read enclosing char looks like ending one.
                        buffer.popLastChar();
                        reader.m_readState |= rsEndOfStream;
                        break;
                    }
                    if (bufferCharToInternal(buffer.back()) == buffer.getFormatDefInfo().getEnc())
                    {
                        // Found double enclosing, handling it and continuing cell reading.
                        ParsingImplementations::onDoubleClosingItemFound(buffer);
                    }
                    else // case: found something else than enclosing item after first one.
                    {
                        if (!ParsingImplementations::separatorChecker(reader.m_readState, reader.getCellBuffer()) &&
                            !ParsingImplementations::eolChecker(reader.m_readState, reader.getCellBuffer()))
                        {
                            // Something else than cell end found after enclosing item.
                            rs = rsPastEnclosedCell;
                            const auto cFirstAfterEnclosing = buffer.popLastChar(); // Pop whatever came after enclosing
                            buffer.popLastChar(); // Pop ending enclosing
                            ParsingImplementations::onPastEnclosedCellCharacter(buffer, cFirstAfterEnclosing);

                            // Skipping remaining chars after ending enclosing char.
                            decltype(readOne(reader.getStream())) c = '\0';
                            while (!reader.isReadStateEndOfCell() && reader.skipChar(&c))
                            {
                                if (ParsingImplementations::separatorChecker(reader.m_readState, buffer.getFormatDefInfo(), c) ||
                                    ParsingImplementations::eolChecker(reader.m_readState, buffer.getFormatDefInfo(), c))
                                {
                                    break;
                                }
                                else
                                {
                                    ParsingImplementations::onPastEnclosedCellCharacter(buffer, c);
                                }

                            }
                            if (!reader.isStreamGood())
                                reader.m_readState |= rsEndOfStream;
                            DFG_ASSERT_CORRECTNESS(reader.isReadStateEndOfCell());
                        }
                        else
                        {
                            // Note: separatorChecker() and eolChecker() pop sep/eol if found.
                            buffer.popLastChar(); // Pop ending enclosing
                            break;
                        }
                    }
                }
                ParsingImplementations::onEnclosedCellRead(buffer.getBuffer(), buffer.getFormatDefInfo().getEnc());
            } // Enclosed cell parsing
        }

        reader.getCellBuffer().onCellRead();

        if (!reader.isStreamGood())
            reader.m_readState |= rsEndOfStream;
    }

    // Reads until end of line or end of stream.
    // Note that end-of-line is with respect to cells; eol within cells are not taken into account.
    template <class Reader>
    static void readUntilEolOrEof(Reader& reader)
    {
        const auto nEolSize = reader.getFormatDefInfo().getEolMarkerLengthInChars();
        const auto nEnclosingItemLength = reader.getFormatDefInfo().getEnclosingMarkerLengthInChars();

        if (nEolSize == 0)
            return;

        if (nEnclosingItemLength == 0)
        { // Case: There's no enclosing item -> simply search for end-of-line item
            auto& rCellBuffer = reader.getCellBuffer();
            rCellBuffer.clear();

            const auto nEolLength = rCellBuffer.getFormatDefInfo().getEolMarkerLengthInChars();
            while(reader.readChar())
            {
                if (rCellBuffer.doesBufferEndWithEndOfLineItem())
                {
                    reader.m_readState = rsEndOfLineEncountered;
                    break;
                }

                // Keep buffer size at minimum: with eol-length of 1, no buffer needs to be kept.
                // With size 2, keep only one char etc.
                if (nEolLength > 0 && rCellBuffer.size() > nEolLength - 1)
                    rCellBuffer.popFrontChar();
            }
        }
        else // Case: Enclosing item exists, must parse line cell by cell.
        {
            // Simply read cells without any further handling.
            for(auto rs = reader.m_readState; rs != rsEndOfLineEncountered && rs != rsEndOfStream && reader.isStreamGood(); rs = reader.m_readState)
            {
                readCell(reader);
            }
        }
    }

    // Reads row of delimited data. Note that since cells may contain eol-items, term 'row' is not the same
    // as row in text editor.
    // CellHandler is given two parameters: size_t nCol, and cellDataBuffer.
    template <class CellReader_T, class CellHandler>
    static DFG_DELIMITED_TEXT_READER_INLINING void readRow(CellReader_T&& reader, CellHandler&& cellDataReceiver)
    {
        typedef typename std::remove_reference<CellReader_T>::type::CellParsingImplementations ParsingImplementations;

        size_t nCol = 0;
        while (!reader.isReadStateEolOrEofOrTerminated() && reader.isStreamGood())
        {
            readCell(reader);

            // Check empty line.
            const bool bEmptyLine = (nCol == 0 && reader.isReadStateEolOrEof() && reader.getCellBuffer().empty());
            // Note: Call handler also for empty rows but not when line ends to EOF.
            if (!bEmptyLine || !reader.isReadStateEof() || reader.isReadStateEolOrSeparatorEncountered()) // Last check is to check parsing for inputs such as "\n" and ",": without the check these would not trigger cellDataReceived-call.
                cellDataReceiver(nCol, reader.getCellBuffer());

            ++nCol;

            if (reader.isReadStateEof() && reader.isReadStateSeparatorEncountered()) // If stream ends with ",", call handler again. e.g. "," should trigger handle for both col 0 and col 1. 
            {
                reader.getCellBuffer().clear();
                cellDataReceiver(nCol, reader.getCellBuffer());
                ++nCol;
            }

            ParsingImplementations::returnValueHandler(reader);

            if (bEmptyLine)
                break;
        }
    }

    // Convenience readRow().
    // Item handler function is given three parameters:
    // column index, Pointer to beginning of buffer, buffer size.
    // For example: func(const size_t nCol, const char* const p, const size_t nSize) 
    // TODO: test
    template <class Char_T, class Stream_T, class ItemHandlerFunc>
    static void readRow(Stream_T&& istrm,
                        const int cSeparator,
                        const int cEnclosing,
                        const int cEndOfLine,
                        ItemHandlerFunc ihFunc)
    {
        CellData<Char_T> cd(cSeparator, cEnclosing, cEndOfLine);
        auto reader = createReader(istrm, cd);
        readRow(reader, [&](const size_t nCol, const decltype(cd)& cellData)
        {
            const auto& buffer = cellData.getBuffer();
            return ihFunc(nCol, buffer.data(), buffer.size());
        });
    }

    // Convenience readRow() taking format definition object, which is expected to have separatorChar(), enclosingChar() and eolCharFromEndOfLineType()
    template <class Char_T, class Stream_T, class Format_T, class ItemHandlerFunc>
    static void readRow(Stream_T& istrm, const Format_T& format, ItemHandlerFunc&& ihFunc)
    {
        readRow<Char_T>(istrm,  format.separatorChar(), format.enclosingChar(), format.eolCharFromEndOfLineType(), std::forward<ItemHandlerFunc>(ihFunc));
    }

    // TODO: test
    template <class Char_T, class InputRange_T, class ItemHandlerFunc_T>
    static void tokenizeLine(const InputRange_T& input,
        const int cSeparator,
        const int cEnclosing,
        ItemHandlerFunc_T ihFunc)
    {
        const auto pData = DFG_ROOT_NS::ptrToContiguousMemory(input);
        DFG_MODULE_NS(io)::BasicImStream_T<Char_T> strm(pData, input.size());
        readRow<Char_T>(strm, cSeparator, cEnclosing, s_nMetaCharNone, ihFunc);
    }

    template <class Char_T, class InputRange_T, class Cont_T>
    static void tokenizeLineToContainer(const InputRange_T& input,
        const int cSeparator,
        const int cEnclosing,
        Cont_T& cont)
    {
        tokenizeLine<Char_T>(input, cSeparator, cEnclosing, [&](const size_t /*nCol*/, const Char_T* const p, const size_t nSize)
        {
#if defined(_MSC_VER) && (_MSC_VER <= DFG_MSVC_VER_2010)
            typedef DFG_MODULE_NS(cont)::ElementType<Cont_T>::type ElemType;
#else
            typedef typename DFG_MODULE_NS(cont)::ElementType<Cont_T>::type ElemType;
#endif
            std::inserter(cont, cont.end()) = ElemType(p, p + nSize);
        });
    }

    // Reads data row by row in order.
    // CellHandler is a function that will receive three parameters:
    // 0: size_t: current row
    // 1: size_t: current column
    // 2: const CellData&: const reference to given CellData-object.
    // For example: auto cellHandler = [](const size_t r, const size_t c, const decltype(CellData)& cellData) {};
    template <class CellReader_T, class CellHandler_T>
    static DFG_DELIMITED_TEXT_READER_INLINING void read(CellReader_T& reader, CellHandler_T&& cellHandler)
    {
        CellReader_T::CellParsingImplementations::read(reader, std::forward<CellHandler_T>(cellHandler));
    }

    template <class BufferChar_T, class CharAppender_T>
    struct ParsingDefinition
    {
        typedef BufferChar_T        BufferChar;
        typedef CharAppender_T      CharAppender;
        typedef std::true_type      IsBasicReaderPossibleType; // Ability to use basic reader does not depend on BufferChar nor on CharAppender.
    };

    static bool isFormatStringViewCCompatible(const FormatDefinitionSingleChars& format)
    {
        // Note: this is in a way a bit too coarse: enclosing character itself is ok as long as cells do not have enclosing character in them, i.e.
        //       "abc","def" could be read with StringViewCBuffer, but "a""bc","def" could not.
        return format.getEnc() == s_nMetaCharNone;
    }

    template <class Char_T, class Stream_T, class ItemHandlerFunc>
    static auto read(Stream_T& istrm,
        const FormatDefinitionSingleChars& formatDef,
        ItemHandlerFunc&& ihFunc) -> FormatDefinitionSingleChars
    {
        const bool appenderTypeCondition = DFG_DETAIL_NS::IsStreamStringViewCCompatible<Stream_T>::value && std::is_same<Char_T, char>::value;
        typedef typename std::conditional<appenderTypeCondition, CharAppenderStringViewCBuffer, CharAppenderDefault<CharBuffer<Char_T>, Char_T>>::type AppenderTypeForStringViewCCompatibleCase;
        if (isFormatStringViewCCompatible(formatDef))
            return readEx(ParsingDefinition<Char_T, AppenderTypeForStringViewCCompatibleCase>(), istrm, formatDef, std::forward<ItemHandlerFunc>(ihFunc));
        else
            return readEx(ParsingDefinition<Char_T, CharAppenderDefault<CharBuffer<Char_T>, Char_T>>(), istrm, formatDef, std::forward<ItemHandlerFunc>(ihFunc));
    }

    // Convenience overload.
    // Item handler function is given four parameters:
    // row index, column index, Pointer to beginning of buffer, buffer size.
    // For example: func(const size_t nRow, const size_t nCol, const char* const pData, const size_t nSize) 
    // TODO: test
    template <class Char_T, class Stream_T, class ItemHandlerFunc>
    static auto read(Stream_T& istrm,
                        const InternalCharType cSeparator,
                        const InternalCharType cEnclosing,
                        const InternalCharType cEndOfLine,
                        ItemHandlerFunc&& ihFunc) -> FormatDefinitionSingleChars
    {
        FormatDefinitionSingleChars formatDef(cEnclosing, cEndOfLine, cSeparator);
        formatDef.setFlag(rfSkipLeadingWhitespaces, false);
        return read<Char_T>(istrm, formatDef, std::forward<ItemHandlerFunc>(ihFunc));
    }

    // Convenience overload that takes csv format object, which is expected to have separatorChar(), enclosingChar() and eolCharFromEndOfLineType()
    template <class Char_T, class Stream_T, class Format_T, class ItemHandlerFunc_T>
    static auto read(Stream_T& istrm, const Format_T& format, ItemHandlerFunc_T&& ihFunc) -> FormatDefinitionSingleChars
    {
        return read<Char_T>(istrm, format.separatorChar(), format.enclosingChar(), format.eolCharFromEndOfLineType(), std::forward<ItemHandlerFunc_T>(ihFunc));
    }

    template <class Stream_T, class CellData_T, class ReaderCreator_T, class ItemHandlerFunc_T>
    static auto readImpl(Stream_T& istrm, CellData_T& cellData, ReaderCreator_T readerCreator, ItemHandlerFunc_T&& ihFunc) -> FormatDefinitionSingleChars
    {
        auto reader = readerCreator(istrm, cellData);
        read(reader, [&](const size_t r, const size_t c, const CellData_T& cd)
        {
            ihFunc(r, c, cd.getBuffer().data(), cd.getBuffer().size());
        });
        return reader.getFormatDefInfo();
    }

    // Implementation for case: IsBasicReaderPossibleType() == true
    template <class Stream_T, class CellData_T, class ItemHandlerFunc_T>
    static auto readImpl(std::true_type, Stream_T& istrm, CellData_T& cellData, ItemHandlerFunc_T&& ihFunc) -> FormatDefinitionSingleChars
    {
        return readImpl(istrm, cellData, &createReader_basic<Stream_T, CellData_T&>, std::forward<ItemHandlerFunc_T>(ihFunc));
    }

    // Implementation for case: IsBasicReaderPossibleType() == false
    template <class Stream_T, class CellData_T, class ItemHandlerFunc_T>
    static auto readImpl(std::false_type, Stream_T& istrm, CellData_T& cellData, ItemHandlerFunc_T&& ihFunc) -> FormatDefinitionSingleChars
    {
        return readImpl(istrm, cellData, &createReader<Stream_T, CellData_T&>, std::forward<ItemHandlerFunc_T>(ihFunc));
    }

    // Item handler function is given four parameters:
    // row index, column index, Pointer to beginning of buffer, buffer size.
    // For example: func(const size_t nRow, const size_t nCol, const char* const pData, const size_t nSize) 
    // TODO: test
    template <class ParsingDefinition_T, class Stream_T, class ItemHandlerFunc>
    static auto readEx(ParsingDefinition_T,
                     Stream_T& istrm,
                     const FormatDefinitionSingleChars formatDefArg,
                     ItemHandlerFunc&& ihFunc) -> FormatDefinitionSingleChars
    {
        typedef typename ParsingDefinition_T::BufferChar Char;
        typedef typename ParsingDefinition_T::CharAppender Appender;

        // TODO: in case of autodetect-controls, peek input to determine actual chars.

        CellData<Char, Char, typename Appender::BufferType, Appender> cellData(formatDefArg);
        const auto& formatDef = cellData.getFormatDefInfo();
        if (formatDef.getEnc() == s_nMetaCharNone && formatDef.getSep() != s_nMetaCharAutoDetect && formatDef.getEol() != s_nMetaCharAutoDetect && !formatDef.testFlag(rfSkipLeadingWhitespaces))
            return readImpl(typename ParsingDefinition_T::IsBasicReaderPossibleType(), istrm, cellData, std::forward<ItemHandlerFunc>(ihFunc));
        else // Case: using default reader
            return readImpl(std::false_type(), istrm, cellData, std::forward<ItemHandlerFunc>(ihFunc));
    }

    // Convenience overload
    template <class ParsingDefinition_T, class Stream_T, class ItemHandlerFunc>
    static auto readEx(ParsingDefinition_T parseDef,
        Stream_T& istrm,
        const InternalCharType cSeparator,
        const InternalCharType cEnclosing,
        const InternalCharType cEndOfLine,
        ItemHandlerFunc&& ihFunc) -> FormatDefinitionSingleChars
    {
        return readEx<ParsingDefinition_T>(parseDef, istrm, FormatDefinitionSingleChars(cEnclosing, cEndOfLine, cSeparator), std::forward<ItemHandlerFunc>(ihFunc));
    }

    // Note: Data parameter is char* instead of void* to make it less likely to call this wrongly for example like
    //       std::vector<int> v; readFromMemory(v.data(), v.size());
    template <class CellHandler, class CellDataT>
    static void readFromMemory(const char* p, const size_t nSize, CellDataT& cd, CellHandler&& cellHandler, const TextEncoding encoding = encodingUnknown)
    {
        ImStreamWithEncoding istrm(p, nSize, encoding);
        CellReader<CellDataT, std::istream> reader(istrm, cd);
        return read(reader, cellHandler);
    }

    // Reads delimited data from file in path 'sPath' using given cell data and
    // after every read cell, calls cellhandler functor.
    // This function will automatically detect BOM-identified UTF-8 and UFT-16(LE and BE)
    // files and create streams that will internally handle decoding.
    template <class CellHandler, class CellDataT>
    static void readFromFile(const StringViewSzC& sPath, CellDataT& cd, CellHandler&& cellHandler, const TextEncoding encoding = encodingUnknown)
    {
        IfStreamWithEncoding istrm(sPath, encoding);
        CellReader<CellDataT, std::istream> reader(istrm, cd);
        return read(reader, cellHandler);
    }

    // Returns sepator item if found, s_nMetaCharNone if not found.
    // Note: checks only the first csv-row, so if no separator is on first row, returns s_nMetaCharNone.
    template <class Char_T>
    static CharType autoDetectCsvSeparatorFromString(const Char_T* psz, const Char_T cEnc = '"', const Char_T cEol = '\n', const TextEncoding notImplemented = encodingUnknown)
    {
        DFG_UNUSED(notImplemented);
        CellData<Char_T> cellDataHandler(s_nMetaCharAutoDetect, cEnc, cEol);
        DFG_MODULE_NS(io)::BasicImStream_T<Char_T> strm(psz, DFG_MODULE_NS(str)::strLen(psz));
        auto reader = createReader(strm, cellDataHandler);
        DFG_SUB_NS_NAME(io)::DelimitedTextReader::readCell(reader);
        const auto cSep = cellDataHandler.getFormatDefInfo().getSep();
        return (isMetaChar(cSep)) ? s_nMetaCharNone : cSep;
    }

}; // DelimitedTextReader

template <class Char_T>
std::ostream& operator<<(std::ostream& strm, const DFG_MODULE_NS(io)::DelimitedTextReader::CharBuffer<Char_T>& buffer)
{
    strm.write(buffer.data(), buffer.size());
    return strm;
}

template <class Char_T>
bool operator==(const DFG_MODULE_NS(io)::DelimitedTextReader::CharBuffer<Char_T>& left, const std::basic_string<Char_T>& right)
{
    return right == StringView<Char_T>(left.data(), left.size());
}

template <class Char_T>
bool operator==(const std::basic_string<Char_T>& left, const DFG_MODULE_NS(io)::DelimitedTextReader::CharBuffer<Char_T>& right)
{
    return right == left;
}

}} // module namespace

#endif
