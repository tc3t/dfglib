#ifndef DFG_IO_DELIMITEDTEXTREADER_VGDXYCDA
#define DFG_IO_DELIMITEDTEXTREADER_VGDXYCDA

#include "../io.hpp"
#include "../cont/table.hpp"
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
#include "../cont/vectorSso.hpp"
#include "../build/inlineTools.hpp"
#include "../preprocessor/compilerInfoMsvc.hpp"

#ifndef _MSC_VER // TODO: Add proper check, workaround originally introduced for gcc 4.8.1
    #include <boost/format/detail/compat_workarounds.hpp>
    #include <locale>
#endif // !_MSC_VER

#include <iterator>



DFG_ROOT_NS_BEGIN { DFG_SUB_NS(io) {

namespace DFG_DETAIL_NS
{
    template <class Strm_T>
    struct IsStreamStringViewCCompatible { enum { value = false }; };

    template <>
    struct IsStreamStringViewCCompatible<DFG_CLASS_NAME(BasicImStream)> { enum { value = true }; };
}

class DFG_CLASS_NAME(DelimitedTextReader)
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

    enum ReadState {rsLookingForNewData         = 0x1,
                    rsInNakedCell               = 0x2,
                    rsInEnclosedCell            = 0x4,
                    rsPastEnclosedCell          = 0x8,
                    rsSeparatorEncountered      = 0x10,
                    rsEndOfLineEncountered      = 0x20,
                    rsEndOfStream               = 0x40,
                    rsTerminated                = 0x80
                   };

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
            return m_readState == rsEndOfStream;
        }

        bool isReadStateEol() const
        {
            return m_readState == rsEndOfLineEncountered;
        }

        bool isReadStateTerminated() const
        {
            return m_readState == rsTerminated;
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

    template <int Enc_T, int Eol_T, int Sep_T>
    class FormatDefinitionSingleCharsCompileTime
    {
    public:
        typedef std::bitset<lastReadFlag + 1> FlagContainer;
        static const int s_enc = Enc_T;
        static const int s_eol = Eol_T;
        static const int s_sep = Sep_T;

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

        FormatDefinitionSingleChars(int cEnc, int cEol, int cSep) :
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

        int getEnc() const {return m_cEnc;}
        int getEol() const {return m_cEol;}
        int getSep() const {return m_cSep;}

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

    // Specialized string view buffer for case where input data is in memory and can be read untranslated -> use string view to underlying data instead of populating a separate buffer.
    class StringViewCBuffer : public DFG_CLASS_NAME(StringViewC)
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
            m_nSize++;
        }
    }; // StringViewCBuffer

    // Char appender for StringViewCBuffer.
    class CharAppenderStringViewCBuffer
    {
    public:
        typedef StringViewCBuffer BufferType;

        static const bool s_isAppendOperatorGuaranteedToIncrementSize = true;

        void operator()(StringViewCBuffer& sv, const char* p)
        {
            DFG_UNUSED(p);
            sv.incrementSize();
        }
    }; // Class CharAppenderStringViewC

    template <class Buffer_T>
    class CharAppenderUtf
    {
    public:
        typedef Buffer_T BufferType;

        static const bool s_isAppendOperatorGuaranteedToIncrementSize = false; // TODO: revise.

        template <class Char_T>
        void operator()(Buffer_T& buffer, const Char_T& ch)
        {
            DFG_MODULE_NS(utf)::cpToUtf(ch, std::back_inserter(buffer), sizeof(typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<decltype(buffer)>::type), DFG_ROOT_NS::ByteOrderHost);
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

    // Replacement for std::basic_string. Problem with populating std::string in this context is that it keeps the buffer null-terminated on every push_back.
    // This is inefficient in this use pattern and with this custom buffer chars can be pushed back without handling the null terminator. Null terminator can be set when the
    // the cell is read completely.
    template <class Char_T, size_t SsoBufferSize_T = 32>
    class CharBuffer : public DFG_MODULE_NS(cont)::DFG_CLASS_NAME(VectorSso)<Char_T, SsoBufferSize_T>
    {
    public:
        operator std::basic_string<Char_T>() const
        {
            return std::basic_string<Char_T>(this->data(), this->size());
        }

        operator DFG_CLASS_NAME(StringViewC)() const
        {
            return DFG_CLASS_NAME(StringViewC)(this->data(), this->size());
        }
    }; // Class CharBuffer

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

        // Defines max char for codecvt-template.
        static const uint32 s_nMaxCodeCvtChar = NumericTraits<UnsignedInputChar>::maxValue;

        CellData(const FormatDef& formatDef) :
            m_formatDef(formatDef),
            m_status(cellHrvContinue)
        {
            privCellDataInit();
        }

        CellData(int cSep, int cEnc, int cEol) :
            m_formatDef(cEnc, cEol, cSep),
            m_status(cellHrvContinue)
        {
            privCellDataInit();
        }

        void privCellDataInit()
        {
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
        void popLastChar()
        {
            DFG_ASSERT_UB(!m_buffer.empty());
            m_buffer.pop_back();
        }

        // Precondition: !empty()
        void popFrontChar()
        {
            DFG_ASSERT_UB(!m_buffer.empty());
            DFG_MODULE_NS(cont)::popFront(m_buffer);
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
            onCellReadBeginImpl(reader, std::integral_constant<bool, std::is_same<StringViewCBuffer, Buffer>::value &&
                                                                     DFG_DETAIL_NS::IsStreamStringViewCCompatible<typename Reader_T::StreamT>::value>());
            
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
            return (m_buffer.size() == 1 && m_buffer.front() == ch);
        }

        bool isLastChar(const int ch) const
        {
            return (!m_buffer.empty() && (m_buffer.back() == ch));
        }

        bool isOneBeforeLast(const int ch) const
        {
            return m_buffer.size() >= 2 && m_buffer[m_buffer.size()-2] == ch;
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
            return (iter != end() && *iter == c);
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

        template <class Iteratable>
        typename Iteratable::const_iterator iteratorToEndingEolItem(const Iteratable& buffer) const
        {
            const auto nBufSize = buffer.size();
            if (nBufSize > 0 && (buffer.isLastChar(m_formatDef.getEol())))
            {
                if (m_formatDef.getEol() == '\n' && nBufSize >= 2 && buffer.isOneBeforeLast('\r')) // Case: \r\n
                    return buffer.cend() - 2;
                else
                    return buffer.cend() - 1;
            }
            return buffer.cend();
        }

        const_iterator iteratorToEndingEolItem() const
        {
            return iteratorToEndingEolItem(*this);
        }

        const_iterator iteratorToEndingEnclosingItem() const
        {
            auto iter = iteratorToLastChar();
            if (iter == m_buffer.end() || !isCharAt(iter, m_formatDef.getEnc()))
                return m_buffer.end();
            else
                return iter;
        }

        bool doesBufferEndWithEnclosingItem() const
        {
            return isLastChar(m_formatDef.getEnc());
        }

        const_iterator iteratorToEndingSeparatorItem() const
        {
            auto iter = iteratorToLastChar();
            if (iter == getBuffer().cend() || !isCharAt(iter, m_formatDef.getSep()))
                return getBuffer().cend();
            else
                return iter;
        }

        bool isBufferSeparatorItem() const
        {
            return isEqual(m_formatDef.getSep());
        }

        bool doesBufferEndWithEndOfLineItem() const
        {
            return isLastChar(m_formatDef.getEol());
        }

        void setBufferEnd(const_iterator endIter)
        {
            auto& buffer = getBuffer();
            DFG_MODULE_NS(cont)::cutTail(buffer, endIter);
        }

        Buffer m_buffer;

        std::basic_string<BufferChar> m_whiteSpaces;
        FormatDef m_formatDef;
        cellHandlerRv m_status;
        CharAppender m_charAppender;
    }; // Class CellData

    template <class Buffer_T>
    class GenericParsingImplementations
    {
    public:
        typedef Buffer_T CellBuffer;
        typedef typename CellBuffer::FormatDef FormatDef;

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
                if (rs != rsInEnclosedCell && (cLast == ',' || cLast == ';' || cLast == '\t'))
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

        // Returns true if caller should invoke 'continue', false otherwise.
        static DFG_FORCEINLINE bool prePostTrimmer(const ReadState rs, const FormatDef& formatDef, CellBuffer& buffer)
        {
            // Check for skippable chars such as whitespace before any cell data or whitespace after enclosed cell.
            if ((rs == rsLookingForNewData && formatDef.testFlag(rfSkipLeadingWhitespaces)) || rs == rsPastEnclosedCell)
            {
                if (buffer.isLastCharacterWhitespace())
                {
                    buffer.popLastChar();
                    return true;
                }
            }
            return false;
        }

        static DFG_FORCEINLINE void nakedCellStateHandler(ReadState& rs, CellBuffer& buffer)
        {
            if (rs == rsLookingForNewData && buffer.sizeInChars() == 1)
                rs = rsInNakedCell;
        }

        // Returns true if caller should invoke 'break', false otherwise.
        static DFG_FORCEINLINE bool separatorChecker(ReadState& rs, CellBuffer& buffer)
        {
            const auto iterToEndingSeparatorItem = buffer.iteratorToEndingSeparatorItem();
            if (rs != rsInEnclosedCell && iterToEndingSeparatorItem != buffer.cend())
            {
                buffer.setBufferEnd(iterToEndingSeparatorItem);
                rs = rsSeparatorEncountered;
                return true;
            }
            return false;
        }

        // Returns true if caller should invoke 'break', false otherwise.
        static DFG_FORCEINLINE bool eolChecker(ReadState& rs, CellBuffer& buffer)
        {
            const auto iterToEndingEolItem = buffer.iteratorToEndingEolItem();
            if (rs != rsInEnclosedCell && iterToEndingEolItem != buffer.cend())
            {
                buffer.setBufferEnd(iterToEndingEolItem);
                rs = rsEndOfLineEncountered;
                return true;
            }
            return false;
        }

        template <class TempBufferType_T, class CharReader_T, class IsStreamGood_T>
        static DFG_FORCEINLINE void enclosedCellReader(ReadState& rs, CellBuffer& buffer, CharReader_T charReader, IsStreamGood_T isStreamGood)
        {
            // Check for characters after enclosing item. Simply ignore them.
            if (rs == rsPastEnclosedCell)
            {
                if (!buffer.empty()) // Revise: This check might be redundant.
                    buffer.popLastChar();
                return;
            }

            const auto nEncLength = buffer.getFormatDefInfo().getEnclosingMarkerLengthInChars();
            const bool bFirstEncCharChance = (rs != rsInEnclosedCell && buffer.sizeInChars() == nEncLength);

            // Check for opening enclosing item.
            if (bFirstEncCharChance && buffer.iteratorToEndingEnclosingItem() != buffer.cend())
            {
                rs = rsInEnclosedCell;
                buffer.clear();
                return; // To prevent beginning enclosing item from being part of double enclosing test.
            }

            // If in enclosed cell, check for double enclosing items within cell text.
            // Note: if reading naked cell that has enclosing items within the text,
            // the enclosing items are treated as normal chars.
            if (rs == rsInEnclosedCell)
            {
                const auto iterToEndingEnclosingItem = buffer.iteratorToEndingEnclosingItem();
                if (iterToEndingEnclosingItem != buffer.cend())
                {
                    // TODO: makes redundant memory allocation in practically all cases; could use
                    //       static buffer.
                    TempBufferType_T tempBuffer(buffer);
                    tempBuffer.clear();

                    const auto rsBefore = rs;
                    for (size_t i = 0; i<nEncLength && isStreamGood(); ++i)
                    {
                        // Read char using explicitly set read state in order to avoid automatic separator detection being ignored due to rsInEnclosedCell-state.
                        const auto bGoodRead = charReader(tempBuffer, rsLookingForNewData);

                        if (!bGoodRead) // Not a good read => didn't find double enclosing item.
                        {
                            buffer.setBufferEnd(iterToEndingEnclosingItem);
                            rs = rsEndOfStream;
                            break;
                        }
                        else if (tempBuffer.isBufferSeparatorItem()) // Found separator after enclosing item.
                        {
                            buffer.setBufferEnd(iterToEndingEnclosingItem);
                            buffer.getFormatDefInfo().copySepatorCharFrom(tempBuffer.getFormatDefInfo()); // This is needed in case that separator auto-detection is triggered for the temp buffer.
                            rs = rsSeparatorEncountered;
                            break;
                        }
                        else if (tempBuffer.doesBufferEndWithEndOfLineItem()) // Found EOL after enclosing item.
                        {
                            buffer.setBufferEnd(iterToEndingEnclosingItem);
                            rs = rsEndOfLineEncountered;
                            break;
                        }
                        else if (tempBuffer.isLastCharacterWhitespace()) // Note: Assumption made: enclosing item must not contain whitespace char.
                        {
                            buffer.setBufferEnd(iterToEndingEnclosingItem);
                            rs = rsPastEnclosedCell;
                            break;
                        }
                    }

                    // Check whether the item after enclosing item is neither enclosing item nor separator.
                    // This is not expected.
                    if (rs == rsBefore && !tempBuffer.doesBufferEndWithEnclosingItem())
                    {
                        buffer.setBufferEnd(iterToEndingEnclosingItem);
                        rs = rsPastEnclosedCell;
                        //reader.onSyntaxError();
                    }
                }
            }
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
    }; // class GenericParsingImplementations

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

        static DFG_FORCEINLINE bool isEmptyGivenSuccessfulReadCharCall(const CellBuffer&, std::true_type)
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

        static DFG_FORCEINLINE void separatorAutoDetection(const ReadState, const FormatDef&, CellBuffer&)
        {
            // Auto detection is not supported in bare-bones parsing.
        }

        // Returns true if caller should invoke 'continue', false otherwise.
        static DFG_FORCEINLINE bool prePostTrimmer(const ReadState, const FormatDef&, CellBuffer&)
        {
            return false;
        }

        static DFG_FORCEINLINE void nakedCellStateHandler(ReadState&, CellBuffer&)
        {
        }

        // Returns true if caller should invoke 'break', false otherwise.
        static DFG_FORCEINLINE bool separatorChecker(ReadState& rs, CellBuffer& buffer)
        {
            if (isEmptyGivenSuccessfulReadCharCall(buffer) || buffer.getBuffer().back() != buffer.getFormatDefInfo().getSep())
                return false;
            else
            {
                buffer.popLastChar();
                rs = rsSeparatorEncountered;
                return true;
            }
        }

        // Returns true if caller should invoke 'break', false otherwise.
        static DFG_FORCEINLINE bool eolChecker(ReadState& rs, CellBuffer& buffer)
        {
            const auto iterToEndingEolItem = buffer.iteratorToEndingEolItem();
            if (iterToEndingEolItem != buffer.cend())
            {
                buffer.setBufferEnd(iterToEndingEolItem);
                rs = rsEndOfLineEncountered;
                return true;
            }
            return false;
        }

        template <class TempBufferType_T, class CharReader_T, class IsStreamGood_T>
        static DFG_FORCEINLINE void enclosedCellReader(ReadState&, CellBuffer&, CharReader_T, IsStreamGood_T)
        {
        }

        template <class Reader_T>
        static DFG_FORCEINLINE void returnValueHandler(const Reader_T&)
        {
            // Barebones parsing does not support return values.
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
        bool readCharImpl(Buffer_T& buffer, const ReadState rs, std::true_type)
        {
            auto& strm = static_cast<DFG_CLASS_NAME(BasicImStream)&>(getStream());
            if (!strm.m_streamBuffer.isAtEnd())
            {
                buffer.appendChar(strm.m_streamBuffer.m_pCurrent);
                ++strm.m_streamBuffer.m_pCurrent;
                CellParsingImplementations::separatorAutoDetection(rs, getFormatDefInfo(), buffer);
                return true;
            }
            else
                return false;
        }

        // readChar implementation for default case.
        template <class BufferT>
        bool readCharImpl(BufferT& buffer, const ReadState rs, std::false_type)
        {
            // Note: When stream has current codecvt-facet, get() will read one
            // character which may be less or more that sizeof(ch)-bytes. The result may also
            // be truncated.
            const auto ch = readOne(getStream());
            #ifdef _MSC_VER // TODO: add proper check, originally introduced as workaround for gcc 4.8.1 
                const bool bRead = (ch != eofChar(getStream()));
            #else
                const bool bRead = (ch != boost::io::CompatTraits<typename StreamT::traits_type>::compatible_type::eof());
            #endif
            if (bRead)
            {
                buffer.appendChar(ch);
                CellParsingImplementations::separatorAutoDetection(rs, getFormatDefInfo(), buffer);
            }
            return bRead;
        }

        // Reads char from stream and returns true if successful, false otherwise.
        // Note that return value does not tell whether read char was successfully stored
        // in buffer, but returned 'true' guarantees that buffer.appendChar() was called.
        template <class Buffer_T>
        bool readChar(Buffer_T& buffer, const ReadState rs)
        {
            return readCharImpl(buffer, rs, std::integral_constant<bool, DFG_DETAIL_NS::IsStreamStringViewCCompatible<StreamT>::value>());
        }

        // Reads char using given buffer and current read state.
        template <class BufferT>
        bool readChar(BufferT& buffer)
        {
            return readChar(buffer, m_readState);
        }

        bool readChar()
        {
            return readChar(getCellBuffer());
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

        CellBuffer& m_rCellBuffer;
        StreamT& m_rStrm;
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
        DFG_ASSERT_WITH_MSG(!isMetaChar(cellData.getFormatDefInfo().getSep()), "Basic parsing expects concrete character for separator.");
        DFG_ASSERT_WITH_MSG(!isMetaChar(cellData.getFormatDefInfo().getEol()), "Basic parsing expects concrete character for end-of-line.");

        typedef typename std::remove_reference<CellData_T>::type CellDataT;
        return CellReader<CellDataT, Stream_T, BarebonesParsingImplementations<CellDataT>>(rStrm, cellData);
    }

    // Read csv-style cell using given reader.
    template <class Reader>
    static void readCell(Reader& reader)
    {
        typedef typename Reader::CellParsingImplementations ParsingImplementations;

        reader.m_readState = rsLookingForNewData;

        reader.getCellBuffer().onCellReadBegin(reader);

        while(!reader.isReadStateEndOfCell() && reader.readChar())
        {
            // prePostTrimmer: Possible check for skippable chars such as whitespace before any cell data or whitespace after enclosed cell.
            if (ParsingImplementations::prePostTrimmer(reader.m_readState, reader.getFormatDefInfo(), reader.getCellBuffer()))
                continue;

            // Set naked cell state if needed.
            ParsingImplementations::nakedCellStateHandler(reader.m_readState, reader.getCellBuffer());

            // Check for separator. Calling separatorChecker() is guaranteed to be done after successful reader.readChar() call. 
            if (ParsingImplementations::separatorChecker(reader.m_readState, reader.getCellBuffer()))
                break;

            // Check for end of line. Calling eolChecker() is guaranteed to be done after successful reader.readChar() call.
            if (ParsingImplementations::eolChecker(reader.m_readState, reader.getCellBuffer()))
                break;

            // Enclosed cell parsing
            typedef decltype(reader.getCellBufferCopy()) TempBufferT;
            ParsingImplementations::template enclosedCellReader<TempBufferT>(reader.m_readState,
                                                                             reader.getCellBuffer(),
                                                                             [&](TempBufferT& buffer, const ReadState rs) { return reader.readChar(buffer, rs); },
                                                                             [&]() { return reader.isStreamGood(); } );
        }

        reader.getCellBuffer().onCellRead();

        if (!reader.isStreamGood())
            reader.m_readState = rsEndOfStream;
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
    static void readRow(CellReader_T&& reader, CellHandler&& cellDataReceiver)
    {
        typedef typename std::remove_reference<CellReader_T>::type::CellParsingImplementations ParsingImplementations;

        size_t nCol = 0;
        while (!reader.isReadStateEolOrEofOrTerminated() && reader.isStreamGood())
        {
            readCell(reader);

            // Check empty line.
            const bool bEmptyLine = (nCol == 0 && reader.isReadStateEolOrEof() && reader.getCellBuffer().empty());
            // Note: Call handler also for empty rows but not when line ends to EOF.
            if (!bEmptyLine || !reader.isReadStateEof())
                cellDataReceiver(nCol, reader.getCellBuffer());

            ++nCol;

            ParsingImplementations::returnValueHandler(reader);

            if (bEmptyLine)
                break;
        }
    }

    // Convenience readRow.
    // Item handler function is given three parameters:
    // column index, Pointer to beginning of buffer, buffer size.
    // For example: func(const size_t nCol, const char* const p, const size_t nSize) 
    // TODO: test
    template <class Char_T, class Stream_T, class ItemHandlerFunc>
    static void readRow(Stream_T& istrm,
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

    // TODO: test
    template <class Char_T, class InputRange_T, class ItemHandlerFunc_T>
    static void tokenizeLine(const InputRange_T& input,
        const int cSeparator,
        const int cEnclosing,
        ItemHandlerFunc_T ihFunc)
    {
        const auto pData = DFG_ROOT_NS::ptrToContiguousMemory(input);
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream_T)<Char_T> strm(pData, input.size());
        readRow<Char_T>(strm, cSeparator, cEnclosing, -1, ihFunc);
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
            typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Cont_T>::type ElemType;
#else
            typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Cont_T>::type ElemType;
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
    template <class CellReader, class CellHandler>
    static void read(CellReader& reader, CellHandler&& cellHandler)
    {
        size_t nRow = 0;
        auto cellHandlerWrapper = [&](size_t nCol, decltype(reader.getCellBuffer())& cellData)
                                    {
                                        cellHandler(nRow, nCol, cellData);
                                    };

        for(; ; ++nRow)
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

    // Convenience overload.
    // Item handler function is given four parameters:
    // row index, column index, Pointer to beginning of buffer, buffer size.
    // For example: func(const size_t nRow, const size_t nCol, const char* const psz, const size_t nSize) 
    // TODO: test
    template <class Stream_T, class Char_T, class ItemHandlerFunc>
    static void read(Stream_T& istrm,
                        const Char_T cSeparator,
                        const Char_T cEnclosing,
                        const Char_T cEndOfLine,
                        ItemHandlerFunc ihFunc)
    {
        auto cellData = CellData<Char_T>(cSeparator, cEnclosing, cEndOfLine);
        auto reader = createReader(istrm, cellData);
        read(reader, [&](const size_t r, const size_t c, const decltype(cellData)& cd)
        {
            ihFunc(r, c, cd.getBuffer().data(), cd.getBuffer().size());
        });
    }

    // Note: Data parameter is char* instead of void* to make it less likely to call this wrongly for example like
    //       std::vector<int> v; readFromMemory(v.data(), v.size());
    template <class CellHandler, class CellDataT>
    static void readFromMemory(const char* p, const size_t nSize, CellDataT& cd, CellHandler&& cellHandler, const TextEncoding encoding = encodingUnknown)
    {
        DFG_CLASS_NAME(ImStreamWithEncoding) istrm(p, nSize, encoding);
        CellReader<CellDataT, std::istream> reader(istrm, cd);
        return read(reader, cellHandler);
    }

    // Reads delimited data from file in path 'sPath' using given cell data and
    // after every read cell, calls cellhandler functor.
    // This function will automatically detect BOM-identified UTF-8 and UFT-16(LE and BE)
    // files and create streams that will internally handle decoding.
    template <class CellHandler, class CellDataT>
    static void readFromFile(const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath, CellDataT& cd, CellHandler&& cellHandler, const TextEncoding encoding = encodingUnknown)
    {
        DFG_CLASS_NAME(IfStreamWithEncoding) istrm(sPath, encoding);
        CellReader<CellDataT, std::istream> reader(istrm, cd);
        return read(reader, cellHandler);
    }

    // Reads table to container.
    template <class Stream_T, class Char_T>
    static DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Table)<std::basic_string<Char_T>> readTableToStringContainer(Stream_T& strm, const Char_T cSeparator, const Char_T cEnc, const Char_T eol)
    {
        CellData<Char_T> cellDataHandler(cSeparator, cEnc, eol);
        auto reader = createReader(strm, cellDataHandler);
        DFG_MODULE_NS(cont)::DFG_CLASS_NAME(Table)<std::basic_string<Char_T>> tableStrings;
        auto cellHandler = [&](const size_t nRow, const size_t /*nCol*/, const decltype(cellDataHandler)& cdh)
        {
            tableStrings.pushBackOnRow(nRow, cdh.getBuffer());
        };
        read(reader, cellHandler);
        return tableStrings;
    }

    // Overload enabling the use of user-defined container of type Table<UserStringType>.
    template <class Char_T, class Stream_T, class StrCont_T>
    static void readTableToStringContainer(Stream_T& strm, const Char_T cSeparator, const Char_T cEnc, const Char_T eol, StrCont_T& dest)
    {
        CellData<Char_T> cellDataHandler(cSeparator, cEnc, eol);
        auto reader = createReader(strm, cellDataHandler);
        auto cellHandler = [&](const size_t nRow, const size_t nCol, const decltype(cellDataHandler)& cdh)
        {
            dest.setElement(nRow, nCol, cdh.getBuffer());
            //dest.pushBackOnRow(nRow, cdh.getBuffer());
        };
        read(reader, cellHandler);
    }

    // Returns sepator item if found, s_nMetaCharNone if not found.
    // Note: checks only the first csv-row, so if no separator is on first row, returns s_nMetaCharNone.
    template <class Char_T>
    static CharType autoDetectCsvSeparatorFromString(const Char_T* psz, const Char_T cEnc = '"', const Char_T cEol = '\n', const TextEncoding notImplemented = encodingUnknown)
    {
        DFG_UNUSED(notImplemented);
        CellData<Char_T> cellDataHandler(s_nMetaCharAutoDetect, cEnc, cEol);
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream_T)<Char_T> strm(psz, DFG_MODULE_NS(str)::strLen(psz));
        auto reader = createReader(strm, cellDataHandler);
        DFG_SUB_NS_NAME(io)::DFG_CLASS_NAME(DelimitedTextReader)::readCell(reader);
        const auto cSep = cellDataHandler.getFormatDefInfo().getSep();
        return (isMetaChar(cSep)) ? s_nMetaCharNone : cSep;
    }

}; // DFG_CLASS_NAME(DelimitedTextReader)

template <class Char_T>
std::ostream& operator<<(std::ostream& strm, const DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharBuffer<Char_T>& buffer)
{
    strm.write(buffer.data(), buffer.size());
    return strm;
}

template <class Char_T>
bool operator==(const DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharBuffer<Char_T>& left, const std::basic_string<Char_T>& right)
{
    return right == DFG_CLASS_NAME(StringView)<Char_T>(left.data(), left.size());
}

template <class Char_T>
bool operator==(const std::basic_string<Char_T>& left, const DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharBuffer<Char_T>& right)
{
    return right == left;
}

}} // module namespace

#endif
