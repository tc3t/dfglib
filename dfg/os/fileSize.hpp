#pragma once

#include "../dfgDefs.hpp"
#include "../dfgBaseTypedefs.hpp"
#include "../io/widePathStrToFstreamFriendlyNonWide.hpp"

#include <sys/types.h>
#include <sys/stat.h>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os) {

    namespace DFG_DETAIL_NS
    {
        template <class Stat_T, class Path_T, class Func_T>
        std::pair<int, Stat_T> statImpl(const Path_T& path, Func_T func)
        {
            Stat_T buffer;
            auto rv = func(path.c_str(), &buffer);
            return std::make_pair(rv, buffer);
        }

#ifdef _WIN32
        inline std::pair<int, struct __stat64> stat(const DFG_CLASS_NAME(ReadOnlySzParamC)& path) { return statImpl<struct __stat64>(path, _stat64); }
        inline std::pair<int, struct __stat64> stat(const DFG_CLASS_NAME(ReadOnlySzParamW)& path) { return statImpl<struct __stat64>(path, _wstat64); }
#else
        inline std::pair<int, struct stat64> stat(const DFG_CLASS_NAME(ReadOnlySzParamC)& path)
        { 
            return statImpl<struct stat64>(path, stat64);
        }
#endif
    }

// TODO: Revise return type.
template <class Char_T>
inline auto fileSizeT(const DFG_CLASS_NAME(ReadOnlySzParam)<Char_T>& sFile) -> uint64
{
    auto info = DFG_DETAIL_NS::stat(DFG_MODULE_NS(io)::pathStrToFileApiFriendlyPath(sFile));
    DFG_STATIC_ASSERT(sizeof(info.second.st_size) >= 8, "stat() should use at least 64-bit size.");
    return (info.first == 0) ? info.second.st_size : 0;
}

// TODO: Revise return value type.
// If file does not exist in the given path, returns 0.
// Note: Likely returns 0 if not having sufficient access rights to given path.
inline auto fileSize(const DFG_CLASS_NAME(ReadOnlySzParamC)& sFile) -> uint64 { return fileSizeT<char>(sFile); }
inline auto fileSize(const DFG_CLASS_NAME(ReadOnlySzParamW)& sFile) -> uint64 { return fileSizeT<wchar_t>(sFile); }

} } // module namespace
