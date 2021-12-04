#ifndef DFG_TIME_VJYLPJBF
#define DFG_TIME_VJYLPJBF

#include "dfgDefs.hpp"

#include <ctime>		// For ::time

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(time) {

namespace DFG_DETAIL_NS
{
    // Precondition: pszFormat must be valid.
    inline std::string localDateStr(const char* pszFormat, const size_t nReserveSize)
    {
        std::string s;
        const auto t = std::time(nullptr);
        std::tm* pTm = nullptr;
#if DFG_MSVC_VER > 0
        std::tm tm;
        auto err = localtime_s(&tm, &t);
        if (err == 0)
            pTm = &tm;
#else
        pTm = std::localtime(&t);
#endif
        if (!pTm)
            return s;

        if (nReserveSize > 0)
            s.resize(nReserveSize);
        const auto nSize = std::strftime(&s[0], s.size(), pszFormat, pTm);
        s.resize(nSize);
        return s;
    }
} // namespace DFG_DETAIL_NS

// Returns current local date in format yyyy-mm-dd
// TODO: test
inline std::string localDate_yyyy_mm_dd_C()
{
    return DFG_DETAIL_NS::localDateStr("%Y-%m-%d", 12);
}

// Returns current local date in format yyyy-mm-ddXhh:mm:ss, where X is by default ' '.
// TODO: test
inline std::string localDate_yyyy_mm_dd_hh_mm_ss_C(const char cDateTimeSeparator = ' ')
{
    auto s = DFG_DETAIL_NS::localDateStr("%Y-%m-%d %H:%M:%S", 20);
    if (s.size() > 10)
        s[10] = cDateTimeSeparator;
    return s;
}

}} // module time

#endif
