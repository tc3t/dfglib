#pragma once

#include "../dfgDefs.hpp"
#include "../bits/byteSwap.hpp"
#include <algorithm>
#include <cstring>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    enum TextEncoding
    {
        encodingUnknown,
        encodingNone,
        encodingUTF8,
        encodingUTF16Le,
        encodingUTF16Be,
        encodingUTF32Le,
        encodingUTF32Be,
        encodingLatin1, // ASCII + Latin1, https://en.wikipedia.org/wiki/Latin-1_Supplement_(Unicode_block)
        encodingUCS2Le,
        encodingUCS2Be,
        encodingUCS4Le,
        encodingUCS4Be,
        encodingWindows1252 // https://en.wikipedia.org/wiki/Windows-1252
    };

    inline bool isUtfEncoding(const TextEncoding encoding)
    {
        return (encoding >= encodingUTF8 && encoding <= encodingUTF32Be);
    }

    inline bool isBigEndianEncoding(const TextEncoding encoding)
    {
        return (encoding == encodingUTF16Be || encoding == encodingUTF32Be);
    }

    // Returns true iff ASCII-bytes can be written unmodified to storage that uses given encoding.
    inline bool areAsciiBytesValidContentInEncoding(const TextEncoding encoding)
    {
        return encoding == encodingUTF8 || encoding == encodingLatin1 || encoding == encodingNone;
    }

    namespace DFG_DETAIL_NS
    {
        const std::pair<TextEncoding, const char*> EncodingStrIdTable[] =
        {
            std::pair<TextEncoding, const char*>(encodingLatin1, "Latin1"),
            std::pair<TextEncoding, const char*>(encodingUTF8, "UTF8"),
            std::pair<TextEncoding, const char*>(encodingUTF16Be, "UTF16BE"),
            std::pair<TextEncoding, const char*>(encodingUTF16Le, "UTF16LE"),
            std::pair<TextEncoding, const char*>(encodingUTF32Be, "UTF32BE"),
            std::pair<TextEncoding, const char*>(encodingUTF32Le, "UTF32LE"),
        };
    }

    inline const char* encodingToStrId(const TextEncoding encoding)
    {
        using namespace DFG_DETAIL_NS;
        auto iter = std::find_if(std::begin(EncodingStrIdTable), std::end(EncodingStrIdTable), [&](const std::pair<TextEncoding, const char*>& entry)
        {
            return (entry.first == encoding);
        });
        return (iter != std::end(EncodingStrIdTable)) ? iter->second : "";
    }

    inline TextEncoding strIdToEncoding(const char* const psz)
    {
        using namespace DFG_DETAIL_NS;
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
        if (charSizeInBytes == 2)
            return (byteOrderHost() == ByteOrderLittleEndian) ? encodingUCS2Le : encodingUCS2Be;
        else if (charSizeInBytes == 4)
            return (byteOrderHost() == ByteOrderLittleEndian) ? encodingUCS4Le : encodingUCS4Be;
        return encodingUnknown;
    }

    inline size_t baseCharacterSize(TextEncoding encoding)
    {
        switch (encoding)
        {
            case encodingUTF8:
                return 1;
            case encodingUTF16Be:
            case encodingUTF16Le:
                return 2;
            case encodingUTF32Be:
            case encodingUTF32Le:
                return 4;
            default: return 0;
        }
    }

} }