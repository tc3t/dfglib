#pragma once

#include "../dfgDefs.hpp"
#include "../dfgBaseTypedefs.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os) {

// TODO: Test
// TODO: Better return value type.
// TODO: Revise implementation (use of ifstream, type of 'posEnd - pos' and it's casting behaviour).
template <class Char_T>
inline auto fileSize(const DFG_CLASS_NAME(ReadOnlyParamStr)<Char_T>& sFile) -> uint64
{
    std::ifstream istrm(sFile, std::ios_base::binary);
    const auto pos = istrm.tellg();
    istrm.seekg(0, std::ios_base::end);
    const auto posEnd = istrm.tellg();
    return static_cast<uint64>(posEnd - pos);
}

} } // module namespace
