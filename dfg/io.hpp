#ifndef DFG_IO_QVIOUELD
#define DFG_IO_QVIOUELD

#include "dfgBase.hpp"
#include "str.hpp"
#include "ptrToContiguousMemory.hpp"
#include "io/BasicIfStream.hpp"
#include "io/textEncodingTypes.hpp"

#include "cont/elementType.hpp"

#include <fstream>
#include <vector>
#include <type_traits>
#include <strstream>

#include "io/BasicImStream.hpp"
#include "dfgBase.hpp"
#include "utf/utfBom.hpp"
#include "io/fileToByteContainer.hpp"


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    

#ifdef _WIN32
#define DFG_NATIVE_ENDOFLINE_STR	"\r\n"
#else
#pragma message( "Note: Using default native end-of-line string." )
#define DFG_NATIVE_ENDOFLINE_STR	"\n"
#endif

    enum EndOfLineType
    {
        EndOfLineTypeN,      // \n
        EndOfLineTypeRN,     // \r\n
        EndOfLineTypeR,      // \r
        EndOfLineTypeNative,
        EndOfLineTypeMixed,
    };

    // Note: These are placeholder interfaces and are not guaranteed to be equal to base classes
    // (e.g. availability of freeze()-function for strstream)
    // TODO: make a proper implementations and move to separate files.
    typedef std::istrstream DFG_CLASS_NAME(IStrStream);
    typedef std::ostrstream DFG_CLASS_NAME(OStrStream);
    typedef std::istringstream DFG_CLASS_NAME(IStringStream);
    typedef std::ostringstream DFG_CLASS_NAME(OStringStream);
    typedef std::ifstream DFG_CLASS_NAME(IfStream);
    typedef std::ofstream DFG_CLASS_NAME(OfStream);


// Returns EOL-char from EndOfLineType. In case of \r\n, returns \n.
// TODO: test
inline char eolCharFromEndOfLineType(EndOfLineType eolType)
{
    if (eolType != EndOfLineTypeNative)
        return (eolType != EndOfLineTypeR) ? '\n' : '\r';
    else
        return (lastOfStrLiteral(DFG_NATIVE_ENDOFLINE_STR) == '\n') ? '\n' : '\r';
}

// Returns eol-string from EndOfLineType. For EndOfLineTypeMixed, return empty.
// TODO: test
inline std::string eolStrFromEndOfLineType(EndOfLineType eolType)
{
    switch (eolType)
    {
        case EndOfLineTypeN: return "\n";
        case EndOfLineTypeRN: return "\r\n";
        case EndOfLineTypeR: return "\r";
        case EndOfLineTypeNative: return DFG_NATIVE_ENDOFLINE_STR;
        case EndOfLineTypeMixed: return "";
        default: return "";
    }
}

// Returns binary stream to given output path.
// Note: Return type is not guaranteed to be std::ofstream.
#ifdef _MSC_VER
inline std::ofstream createOutputStreamBinaryFile(DFG_CLASS_NAME(ReadOnlyParamStr)<char> sPath)
{
    std::ofstream strm(sPath, std::ios::binary | std::ios::out);
    return strm;
}
#endif

// Returns input binary stream from given output path.
// Note: Return type is not guaranteed to be std::ifstream.
//inline std::ifstream createInputStreamBinaryFile(NonNullCStr pszPath)
inline DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicIfStream) createInputStreamBinaryFile(const DFG_CLASS_NAME(ReadOnlyParamStrC)& sPath)
{
    //std::ifstream strm(pszPath, std::ios::binary | std::ios::in);
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicIfStream) strm(sPath);
    return strm;
}

template <class Strm_T>
inline Strm_T& writeBinary(Strm_T& ostrm, const void* pData, const size_t nSizeInBytes)
{
    ostrm.write(static_cast<const char*>(pData), nSizeInBytes);
    return ostrm;
}

// Writes object of class T to binary stream.
// If stream object is not in binary mode, the behaviour is undefined.
template <class Strm_T, class T>
Strm_T& writeBinary(Strm_T& ostrm, const T& obj)
{
    DFG_STATIC_ASSERT(std::is_pointer<T>::value == false, "io::writeBinary: writing pointers as binary is disabled.");
    return writeBinary(ostrm, &obj, sizeof(obj));
}

// Reads bytes to buffer of given size from given binary stream.
// To get the number of bytes read, with std::istream call istrm.gcount() after calling this function.
// Note: Stream must be opened in binary mode.
template <class StrmT> StrmT& readBinary(StrmT& istrm, void* pBuf, const size_t nBufSize)
{
    // TODO: Check read mode status.
    // TODO: Check for size_t -> std::streamsize overflow.
    return static_cast<StrmT&>(istrm.read(reinterpret_cast<char*>(pBuf), static_cast<std::streamsize>(nBufSize)));
}

// Provided for convenience: reads bytes to object of type T from binary stream.
template <class Strm_T, class T>
Strm_T& readBinary(Strm_T& istrm, T& obj)
{
    #ifdef _MSC_VER // TODO: Add proper check, this was added for GCC 4.8.1
        // is_pod was added for VC2013, where, unlike with VC2010 and VC2012, array of pods returned false for has_trivial_assign<T>.
        DFG_STATIC_ASSERT(std::has_trivial_assign<T>::value == true || std::is_pod<T>::value == true, "readBinary(Strm_T, T&) only accepts trivially assignable or pod T's");
    #endif
    return readBinary(istrm, &obj, sizeof(obj));
}

// Writes items in container to stream so that separator is put only between
// items, for example no excess separator will appear at the end of the stream.
// TODO: test
template <class StreamT, class Iterable_T, class SepT, class WriteFunc>
void writeDelimited(StreamT& strm, const Iterable_T& cont, const SepT& sep, const WriteFunc writer)
{
    auto iter = DFG_ROOT_NS::cbegin(cont);
    const auto iterEnd = DFG_ROOT_NS::cend(cont);
    if (iter == iterEnd)
        return;
    writer(strm, *iter);
    ++iter;
    for(; iter != iterEnd; ++iter)
    {
        strm << sep;
        writer(strm, *iter);
    }
}

// Overload for convenience using default writer, strm << object.
template <class StreamT, class ContT, class SepT>
void writeDelimited(StreamT& strm, const ContT& cont, const SepT& sep)
{
    typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<ContT>::type ElementT;
    writeDelimited(strm, cont, sep, [](StreamT& strm, const ElementT& item)
    {
        strm << item;
    });
}

template <class Stream_T>
inline TextEncoding checkBOM(Stream_T& istrm)
{
    const auto startPos = istrm.tellg();
    unsigned char buf[5] = { 0, 0, 0xFD, 0xFD, 0 };

    readBinary(istrm, buf, 4);
    istrm.seekg(startPos);

    auto encoding = encodingUnknown;

#define IS_BOM(BOM) memcmp(buf, DFG_MODULE_NS(utf)::BOM, sizeof(DFG_MODULE_NS(utf)::BOM)) == 0

    // Note: UTF32 must be checked before UTF16 because they begin with the same sequence.
    if (IS_BOM(bomUTF8))
        encoding = encodingUTF8;
    else if (IS_BOM(bomUTF32Le))
        encoding = encodingUTF32Le;
    else if (IS_BOM(bomUTF32Be))
        encoding = encodingUTF32Be;
    else if (IS_BOM(bomUTF16Le))
        encoding = encodingUTF16Le;
    else if (IS_BOM(bomUTF16Be))
        encoding = encodingUTF16Be;

#undef IS_BOM

    return encoding;
}

// Checks BOM marker from file in given path.
// TODO: test, especially small files with less bytes than longest BOM-markers.
inline TextEncoding checkBOMFromFile(const DFG_CLASS_NAME(ReadOnlyParamStrC)& sPath)
{
    auto istrm = createInputStreamBinaryFile(sPath);
    return checkBOM(istrm);
}

// Ignores given number of items from stream.
// TODO: test
template <class Strm_T>
void ignore(Strm_T& strm, const int nCount) {strm.ignore(nCount);}

// Peeks next character from stream.
// TODO: test
template <class Strm_T>
auto peekNext(Strm_T& strm) -> decltype(strm.peek()) {return strm.peek();}

// Reads one character from stream, eofChar() if not successful.
// TODO: test
template <class Strm_T>
auto readOne(Strm_T& strm) -> decltype(strm.get()) {return strm.get();}

// TODO: test
template <class Strm_T>
bool isStreamInReadableState(Strm_T& strm) {return strm;}

// TODO: test
template <class Strm_T>
bool isStreamInGoodState(Strm_T& strm) {return strm.good();}

// TODO: test
template <class Strm_T>
decltype(Strm_T::traits_type::eof()) eofChar(Strm_T& /*strm*/)
{
    return Strm_T::traits_type::eof();
}

// TODO: test
// Note: Text files is not supported, because according to VC2010 documentation
//       relative seeks is not supported in text files.
template <class Strm_T>
void advanceBinaryStream(Strm_T& strmBinary, const std::streamsize nCount)
{
    strmBinary.seekg(nCount, std::ios_base::cur);
}

// TODO: test
template <class Strm_T, class Pred>
void advanceWhileNextPred(Strm_T& strm, Pred pred)
{
    while(isStreamInReadableState(strm) && pred(peekNext(strm)))
        ignore(strm, 1);
}

// TODO: test
template <class Strm_T, class Str_T>
void advanceWhileNextIsAnyOf(Strm_T& strm, const Str_T& sMatchChars)
{
    typedef decltype(sMatchChars[0]) CharT;
    advanceWhileNextPred(strm, [&](const CharT ch)
    {
        return (std::find(cbegin(sMatchChars), cend(sMatchChars), ch) != cend(sMatchChars));
    });
}

// Stream that discards all data written to it.
// Note: Very hacky and incomplete implementation, see e.g. boost::basic_null_sink or
// Poco::NullOutputStream if a proper implementation is needed.
// TODO: test
// TODO: Make interface compatible with std::ostream.
template <class Char_T, class BaseClass_T = std::basic_ostream<Char_T>>
class DFG_CLASS_NAME(NullSink)
{
public:
    typedef BaseClass_T BaseClass;
    DFG_CLASS_NAME(NullSink)() : BaseClass(nullptr)
    {}

    template <class T>
    DFG_CLASS_NAME(NullSink)& operator<<(const T&) const { return *this; }

    std::streamsize write(const Char_T*, std::streamsize n) {return n;}
};

}} // module io

#endif
