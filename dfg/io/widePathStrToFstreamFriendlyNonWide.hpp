#pragma once

#include <string>
#include "../ReadOnlySzParam.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    // Converts wchar_t path to std::string that e.g. std::ofstream accepts.
    // TODO: test
    std::string widePathStrToFstreamFriendlyNonWide(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath);

} } // module namespace
