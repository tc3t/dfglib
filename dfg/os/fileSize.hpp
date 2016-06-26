#pragma once

#include "../dfgDefs.hpp"
#include "../dfgBaseTypedefs.hpp"
#include "../io/BasicIfStream.hpp"


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os) {

// TODO: Revise return type.
template <class Char_T>
inline auto fileSizeT(const DFG_CLASS_NAME(ReadOnlySzParam)<Char_T>& sFile) -> uint64
{
    using namespace DFG_MODULE_NS(io);
    BasicIfStream istrm(sFile);
    istrm.seekg(BasicIfStream::SeekOriginEnd, 0);
    const auto endPos = istrm.tellg();
    const uint64 nSize = endPos;
    return nSize;
}

// TODO: Revise return value type.
// If file does not exist in the given path, returns 0.
// Note: Likely returns 0 if not having sufficient access rights to given path.
inline auto fileSize(const DFG_CLASS_NAME(ReadOnlySzParamC)& sFile) -> uint64 { return fileSizeT<char>(sFile); }
inline auto fileSize(const DFG_CLASS_NAME(ReadOnlySzParamW)& sFile) -> uint64 { return fileSizeT<wchar_t>(sFile); }

} } // module namespace
