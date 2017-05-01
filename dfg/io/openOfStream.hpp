#pragma once

// Implements opening a std::ofstream from path including wchar_t-path.

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "textEncodingTypes.hpp"
#include "../utf/utfBom.hpp"
#include "../utf.hpp"
#include "../ptrToContiguousMemory.hpp"
#include <fstream>
#if !defined(_MSC_VER)
    #include "widePathStrToFstreamFriendlyNonWide.hpp"
#endif

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    // TODO: test
    inline std::basic_filebuf<char>* openOfStream(std::basic_filebuf<char>* pFileBuf, const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath, std::ios_base::openmode openmode)
    {
        if (!pFileBuf)
            return nullptr;
        return pFileBuf->open(sPath, openmode);
    }

    // TODO: test
    inline std::basic_filebuf<char>* openOfStream(std::basic_filebuf<char>* pFileBuf, const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath, std::ios_base::openmode openmode)
    {
        if (!pFileBuf)
            return nullptr;

    #if defined(_MSC_VER) // MSVC has wchar_t overload extension so use it.
            return pFileBuf->open(sPath, openmode);
    #else
            return pFileBuf->open(widePathStrToFstreamFriendlyNonWide(sPath), openmode);
    #endif
    }

    // TODO: test
    inline void openOfStream(std::ofstream& strm, const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath, std::ios_base::openmode openmode) { openOfStream(strm.rdbuf(), sPath, openmode); }
    inline void openOfStream(std::ofstream& strm, const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath, std::ios_base::openmode openmode) { openOfStream(strm.rdbuf(), sPath, openmode); }

} }
