#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../utf/utfBom.hpp"
#include "textEncodingTypes.hpp"
#include "createInputStream.hpp"
#include "readBinary.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

inline TextEncoding checkBOM(const char* pData, const size_t nCount)
{
    auto encoding = encodingUnknown;

#define IS_BOM(BOM) (nCount >= sizeof(DFG_MODULE_NS(utf)::BOM) && (memcmp(pData, DFG_MODULE_NS(utf)::BOM, sizeof(DFG_MODULE_NS(utf)::BOM)) == 0))

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

inline TextEncoding checkBOM(const unsigned char* pData, const size_t nCount)
{
    return checkBOM(reinterpret_cast<const char*>(pData), nCount);
}

template <class Stream_T>
inline TextEncoding checkBOM(Stream_T& istrm)
{
    const auto startPos = istrm.tellg();
    unsigned char buf[5] = { 0, 0, 0xFD, 0xFD, 0 };

    readBinary(istrm, buf, 4);
    istrm.seekg(startPos);

    return checkBOM(buf, 4);
}

// Checks BOM marker from file in given path.
// TODO: test, especially small files with less bytes than longest BOM-markers.
template <class Char_T>
inline TextEncoding checkBOMFromFile(const DFG_CLASS_NAME(ReadOnlySzParam)<Char_T> & sPath)
{
    auto istrm = createInputStreamBinaryFile(sPath);
    return checkBOM(istrm);
}

inline TextEncoding checkBOMFromFile(const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath)
{
    return checkBOMFromFile<char>(sPath);
}

inline TextEncoding checkBOMFromFile(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath)
{
    return checkBOMFromFile<wchar_t>(sPath);
}

} } // module io
