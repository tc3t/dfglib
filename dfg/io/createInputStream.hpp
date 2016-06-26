#pragma once

#include "../dfgDefs.hpp"
#include "BasicIfStream.hpp"
#include "../ReadOnlySzParam.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {


// Returns input binary stream from given output path.
// Note: Return type is not guaranteed to be std::ifstream.
//inline std::ifstream createInputStreamBinaryFile(NonNullCStr pszPath)
template <class Char_T>
inline DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicIfStream) createInputStreamBinaryFile(const DFG_CLASS_NAME(ReadOnlySzParam)<Char_T>& sPath)
{
    //std::ifstream strm(pszPath, std::ios::binary | std::ios::in);
    DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicIfStream) strm(sPath);
    return strm;
}

inline DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicIfStream) createInputStreamBinaryFile(const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath)
{
    return createInputStreamBinaryFile<char>(sPath);
}

inline DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicIfStream) createInputStreamBinaryFile(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath)
{
    return createInputStreamBinaryFile<wchar_t>(sPath);
}

} } // module io
