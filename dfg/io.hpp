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
#include "io/fileToByteContainer.hpp"
#include "io/EndOfLineTypes.hpp"
#include "io/OfStream.hpp"
#include "io/readBinary.hpp"
#include "io/checkBom.hpp"
#include "io/createInputStream.hpp"

#include "typeTraits.hpp"


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    // Note: These are placeholder interfaces and are not guaranteed to be equal to base classes
    // (e.g. availability of freeze()-function for strstream)
    // TODO: make a proper implementations and move to separate files.
    typedef std::istrstream DFG_CLASS_NAME(IStrStream);
    typedef std::ostrstream DFG_CLASS_NAME(OStrStream);
    typedef std::istringstream DFG_CLASS_NAME(IStringStream);
    typedef std::ostringstream DFG_CLASS_NAME(OStringStream);
    typedef std::ifstream DFG_CLASS_NAME(IfStream);

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
#if DFG_LANGFEAT_MOVABLE_STREAMS
inline DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStream) createOutputStreamBinaryFile(DFG_CLASS_NAME(ReadOnlySzParam)<char> sPath)
{
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStream) strm(sPath);
    return strm;
}
#endif // DFG_LANGFEAT_MOVABLE_STREAMS

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
