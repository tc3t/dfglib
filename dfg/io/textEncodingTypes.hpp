#pragma once

#include "../dfgDefs.hpp"
#include "../bits/byteSwap.hpp"
#include <algorithm>
#include <cstring>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    enum TextEncoding
    {
        encodingUnknown,
        encodingUTF8,
        encodingUTF16Le,
        encodingUTF16Be,
        encodingUTF32Le,
        encodingUTF32Be,
        encodingLatin1, // ASCII + Latin1, https://en.wikipedia.org/wiki/Latin-1_Supplement_(Unicode_block)
                        // Note that there is some potentially confusing terminology: searching for "latin1"
                        // in Wikipedia lead to page about "ISO/IEC 8859-1" https://en.wikipedia.org/wiki/ISO/IEC_8859-1,
                        // which notes (as of 2024-08) that ranges such as [0x00, 0x1f] are "Undefined".
                        // But as commented in https://stackoverflow.com/questions/12794825/is-iso-8859-1-a-unicode-charset
                        // in practice latin1 is often interpreted as ISO/IEC 8859-1 "supplemented with the C0 and C1 control codes from ISO/IEC 6429"
                        // "ISO-8859-1 is the IANA preferred name for this standard when supplemented with the C0 and C1 control codes from ISO/IEC 6429"
                        // (quote from wikipedia https://en.wikipedia.org/w/index.php?title=ISO/IEC_8859-1&oldid=1240566855)
        encodingUCS2Le,
        encodingUCS2Be,
        encodingUCS4Le,
        encodingUCS4Be,
        encodingWindows1252, // https://en.wikipedia.org/wiki/Windows-1252
        lastTextEncodingItem = encodingWindows1252
    };
    constexpr size_t NumberOfTextCodingItems = lastTextEncodingItem + 1u;

    constexpr inline bool isUtfEncoding(const TextEncoding encoding)
    {
        return (encoding >= encodingUTF8 && encoding <= encodingUTF32Be);
    }

    constexpr inline bool isBigEndianEncoding(const TextEncoding encoding)
    {
        DFG_STATIC_ASSERT(NumberOfTextCodingItems == 12, "Number of text encoding items has changed, check if implementation is up-to-date.");
        return (encoding == encodingUTF16Be || encoding == encodingUTF32Be || encoding == encodingUCS2Be || encoding == encodingUCS4Be);
    }

    // Returns true iff ASCII-bytes can be written unmodified to storage that uses given encoding.
    inline bool areAsciiBytesValidContentInEncoding(const TextEncoding encoding)
    {
        DFG_STATIC_ASSERT(NumberOfTextCodingItems == 12, "Number of text encoding items has changed, check if implementation is up-to-date.");
        return encoding == encodingUTF8 || encoding == encodingLatin1 || encoding == encodingWindows1252;
    }

    namespace DFG_DETAIL_NS
    {
        const std::pair<TextEncoding, const char*> EncodingStrIdTable[] =
        {
            std::pair<TextEncoding, const char*>(encodingLatin1,  "Latin1"),
            std::pair<TextEncoding, const char*>(encodingUTF8,    "UTF8"),
            std::pair<TextEncoding, const char*>(encodingUTF16Be, "UTF16BE"),
            std::pair<TextEncoding, const char*>(encodingUTF16Le, "UTF16LE"),
            std::pair<TextEncoding, const char*>(encodingUTF32Be, "UTF32BE"),
            std::pair<TextEncoding, const char*>(encodingUTF32Le, "UTF32LE"),
            std::pair<TextEncoding, const char*>(encodingUCS2Be,  "UCS2BE"),
            std::pair<TextEncoding, const char*>(encodingUCS2Le,  "UCS2LE"),
            std::pair<TextEncoding, const char*>(encodingUCS4Be,  "UCS4BE"),
            std::pair<TextEncoding, const char*>(encodingUCS4Le,  "UCS4LE"),
            std::pair<TextEncoding, const char*>(encodingWindows1252, "windows-1252"),
        };
    }

    inline const char* encodingToStrId(const TextEncoding encoding)
    {
        using namespace DFG_DETAIL_NS;
        DFG_STATIC_ASSERT(NumberOfTextCodingItems == 12, "Number of text encoding items has changed, check if implementation is up-to-date.");
        auto iter = std::find_if(std::begin(EncodingStrIdTable), std::end(EncodingStrIdTable), [&](const std::pair<TextEncoding, const char*>& entry)
        {
            return (entry.first == encoding);
        });
        return (iter != std::end(EncodingStrIdTable)) ? iter->second : "";
    }

    inline TextEncoding strIdToEncoding(const char* const psz)
    {
        using namespace DFG_DETAIL_NS;
        DFG_STATIC_ASSERT(NumberOfTextCodingItems == 12, "Number of text encoding items has changed, check if implementation is up-to-date.");
        auto iter = std::find_if(std::begin(EncodingStrIdTable), std::end(EncodingStrIdTable), [&](const std::pair<TextEncoding, const char*>& entry)
        {
            return (strcmp(entry.second, psz) == 0);
        });
        return (iter != std::end(EncodingStrIdTable)) ? iter->first : encodingUnknown;
    }

    inline TextEncoding hostNativeFixedSizeUtfEncodingFromCharType(const size_t charSizeInBytes)
    {
        if (charSizeInBytes == 1)
            return encodingLatin1;
        else if (charSizeInBytes == 2)
            return (byteOrderHost() == ByteOrderLittleEndian) ? encodingUCS2Le : encodingUCS2Be;
        else if (charSizeInBytes == 4)
            return (byteOrderHost() == ByteOrderLittleEndian) ? encodingUCS4Le : encodingUCS4Be;
        else
            return encodingUnknown;
    }

    inline TextEncoding hostNativeUtfEncodingFromCharType(const size_t charSizeInBytes)
    {
        if (charSizeInBytes == 1)
            return encodingUTF8;
        if (charSizeInBytes == 2)
            return (byteOrderHost() == ByteOrderLittleEndian) ? encodingUTF16Le : encodingUTF16Be;
        else if (charSizeInBytes == 4)
            return (byteOrderHost() == ByteOrderLittleEndian) ? encodingUTF32Le : encodingUTF32Be;
        return encodingUnknown;
    }

    // Returns the number of bytes that single base character of given encoding takes
    inline size_t baseCharacterSize(const TextEncoding encoding)
    {
        DFG_STATIC_ASSERT(NumberOfTextCodingItems == 12, "Number of text encoding items has changed, check if implementation is up-to-date.");
        switch (encoding)
        {
            case encodingUTF16Be:
            case encodingUTF16Le:
            case encodingUCS2Be:
            case encodingUCS2Le:
                return 2;
            case encodingUTF32Be:
            case encodingUTF32Le:
            case encodingUCS4Be:
            case encodingUCS4Le:
                return 4;
            default: return 1;
        }
    }

} }
