#pragma once

#include <string>
#include "../ReadOnlySzParam.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    // Converts wchar_t path to std::string that e.g. std::ofstream accepts.
    // TODO: test
    std::string widePathStrToFstreamFriendlyNonWide(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath);

    inline DFG_CLASS_NAME(ReadOnlySzParamC) pathStrToFileApiFriendlyPath(const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath) { return sPath; }

#ifdef _WIN32
    inline DFG_CLASS_NAME(ReadOnlySzParamW) pathStrToFileApiFriendlyPath(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath) { return sPath; }
#else
    inline std::string pathStrToFileApiFriendlyPath(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath) { return widePathStrToFstreamFriendlyNonWide(sPath); }
#endif

} } // module namespace
