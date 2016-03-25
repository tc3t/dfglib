#ifndef DFG_TIME_VJYLPJBF
#define DFG_TIME_VJYLPJBF

#include "dfgDefs.hpp"
#include "buildConfig.hpp"

#include <ctime>		// For ::time

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(time) {

enum Weekday {WeekdayMon, WeekdayTue, WeekdayWed, WeekdayThu, WeekdayFri, WeekdaySat, WeekdaySun};

// Returns current local date in format yyyy-mm-dd
// TODO: test
inline std::string localDate_yyyy_mm_dd_C()
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
    
    s.resize(12);
    const auto nSize = std::strftime(&s[0], s.size(), "%Y-%m-%d", pTm);
    s.resize(nSize);
    return s;
}

}} // module time

#endif
