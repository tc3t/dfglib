#include "widePathStrToFstreamFriendlyNonWide.hpp"

#include "../ReadOnlySzParam.hpp"
#include "../utf.hpp"
#include "../ptrToContiguousMemory.hpp"
#include "../iter/szIterator.hpp"

#if defined(_WIN32)
    #include <Windows.h> // For MultiByteToWideChar
#endif

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

#if defined(_WIN32)
    std::string widePathStrToFstreamFriendlyNonWide(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath)
    {
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
        return s;
    }

#else // case: non-Windows

    std::string widePathStrToFstreamFriendlyNonWide(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath)
    {
        // For non-Windows, for now convert to UTF8 assuming that source path is proper code points.
        return DFG_MODULE_NS(utf)::codePointsToUtf8(DFG_ROOT_NS::makeSzRange(sPath.c_str()));
    }

#endif // _WIN32

} } // Module namespace
