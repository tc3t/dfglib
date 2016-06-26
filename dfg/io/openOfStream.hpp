#pragma once

// Implements opening a std::ofstream from path including wchar_t-path.

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "textEncodingTypes.hpp"
#include "../utf/utfBom.hpp"
#include "../utf.hpp"
#include "../ptrToContiguousMemory.hpp"
#include <fstream>
#if !defined(_MSC_VER) && defined(_WIN32)
    #include <Windows.h> // For MultiByteToWideChar
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
    #elif defined(_WIN32) // e.g. MinGW
            // TODO: use proper pragma, this version is one that works with MSVC.
            #pragma message("Warning: Using untested implementation of opening file stream from wchar_t path.")

            // Convert wchar_t path to char path and open the file through it.
            
            const UINT codePage = CP_ACP;

            BOOL bDefaultUsed = false;
            const auto nRequiredSize = ::WideCharToMultiByte(codePage, 0 /* flags */, sPath, sPath.length(), nullptr, 0, "?", &bDefaultUsed);

            if (nRequiredSize <= 0 || bDefaultUsed)
                return nullptr;
            std::string s;
            s.resize(nRequiredSize);
            ::WideCharToMultiByte(codePage, 0 /* flags */, sPath, sPath.length(), ptrToContiguousMemory(s), s.length(), "?", &bDefaultUsed);
            if (!s.empty() && s.back() == '\0')
                s.pop_back();
            return pFileBuf->open(s, openmode);
    #else
            // For non-Windows, for now convert to UTF8 assuming that source path is proper code points.
            // TODO: use proper pragma, this version is one that works with MSVC.
            #pragma message("Warning: Using untested implementation of opening file stream from wchar_t path.")
            m_ostrm.open(DFG_MODULE_NS(utf)::codePointsToUtf8(sPath), openmode);
    #endif
    }

    // TODO: test
    inline void openOfStream(std::ofstream& strm, const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath, std::ios_base::openmode openmode) { openOfStream(strm.rdbuf(), sPath, openmode); }
    inline void openOfStream(std::ofstream& strm, const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath, std::ios_base::openmode openmode) { openOfStream(strm.rdbuf(), sPath, openmode); }

    

} }
