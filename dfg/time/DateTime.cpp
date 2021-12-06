#include "DateTime.hpp"
#include "../dfgAssert.hpp"
#include "../str/strTo.hpp"
#include "../alg.hpp"
#include <regex>

#if DFG_LANGFEAT_CHRONO_11

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <sys/time.h>
#endif

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(time)
{

auto UtcOffsetInfo::offsetDiffInSeconds(const UtcOffsetInfo& other) const -> int32
{
    if (isSet() && other.isSet())
        return other.offsetInSeconds() - offsetInSeconds();
    else
    {
        DFG_ASSERT_IMPLEMENTED((isSet() || m_offsetValue == s_nNotSetValueMimicing) && (other.isSet() || other.m_offsetValue == s_nNotSetValueMimicing)); // Only implemented for mimic special value.
        return 0; // If either one or both are mimicing, there's no utc offset difference. 
    }
}

DateTime::DateTime() : DateTime(0, 0, 0, 0, 0, 0, 0)
{
}

DateTime::DateTime(int year, int month, int day, int hour, int minute, int second, int milliseconds, UtcOffsetInfo utcOffsetInfo)
{
    m_year = static_cast<uint16>(year);
    m_month = static_cast<uint8>(month);
    m_day = static_cast<uint8>(day);
    m_milliSecSinceMidnight = millisecondsSinceMidnight(hour, minute, second, milliseconds);
    m_utcOffsetInfo = utcOffsetInfo;
}

DateTime DateTime::fromString(const StringViewC& sv)
{
    using namespace ::DFG_MODULE_NS(str);
    DateTime rv;

    // yyyy-mm-dd (ISO 8601, https://en.wikipedia.org/wiki/ISO_8601)
    std::regex dateRegex(R"((\d{4})-(\d\d)-(\d\d)$)");
    std::cmatch baseMatch;
    if (sv.size() < 10 || !std::regex_match(sv.begin(), sv.begin() + 10, baseMatch, dateRegex) || baseMatch.size() != 4)
        return DateTime();

    const auto asInt = [](const std::csub_match& subMatch) { return ::DFG_MODULE_NS(str)::strTo<int>(StringViewC(subMatch.first, subMatch.second)); };

    // 0 has entire match, so actual captures start from index 1.
    const int nYear  = asInt(baseMatch[1]);
    const int nMonth = asInt(baseMatch[2]);
    const int nDay   = asInt(baseMatch[3]);
    if (nYear <= 0 || nMonth <= 0 || nDay <= 0 || nMonth > 12 || nDay > 31)
        return DateTime();
    rv.m_year  = saturateCast<decltype(rv.m_year)>(nYear);
    rv.m_month = saturateCast<decltype(rv.m_month)>(nMonth);
    rv.m_day   = saturateCast<decltype(rv.m_day)>(nDay);

    if (sv.size() == 10)
        return rv;

    // yyyy-MM-ddThh:mm:ss has size 19
    std::regex timeRegex(R"([ T](\d\d):(\d\d):(\d\d)(\.\d\d\d|)$)");
    const auto nHasMsDot = (sv.size() >= 21 && sv[19] == '.');
    const size_t nTimeEnd = (!nHasMsDot) ? 19 : 23;
    if (sv.size() < 19 || !std::regex_match(sv.begin() + 10, sv.begin() + nTimeEnd, baseMatch, timeRegex) || baseMatch.size() != 5)
        return DateTime();

    const auto h = asInt(baseMatch[1]);
    if (h < 0 || h > 23)
        return DateTime();
    const auto m = asInt(baseMatch[2]);
    if (m < 0 || m > 59)
        return DateTime();
    const auto s = asInt(baseMatch[3]);
    if (s < 0 || s > 60)
        return DateTime();
    const int ms = (baseMatch[4].length() > 0) ? ::DFG_MODULE_NS(str)::strTo<int>(StringViewC(baseMatch[4].first + 1, baseMatch[4].second)) : 0; // + 1 to skip dot
    if (ms < 0 || ms > 999)
        return DateTime();

    rv.m_milliSecSinceMidnight = millisecondsSinceMidnight(h, m, s, ms);

    if (sv.size() == nTimeEnd)
        return rv;

    if (sv.size() == nTimeEnd + 1 && sv.back() == 'Z')
    {
        rv.m_utcOffsetInfo.setOffsetInSeconds(0);
        return rv;
    }

    std::regex tzRegex(R"(([\+-])(\d\d)(:\d\d|)$)");
    if (!std::regex_match(sv.begin() + nTimeEnd, sv.end(), baseMatch, tzRegex) || baseMatch.size() != 4)
        return DateTime();

    const auto signFactor = (baseMatch[1] == '+') ? 1 : -1;
    const auto tzh = asInt(baseMatch[2]);
    if (tzh < 0 || tzh >= 24)
        return DateTime();
    auto tzm = (baseMatch[3].length() > 0) ? ::DFG_MODULE_NS(str)::strTo<int>(StringViewC(baseMatch[3].first + 1, baseMatch[3].second)) : 0; // + 1 to skip ':'
    if (tzm < 0 || tzm >= 60)
        return DateTime();
    rv.m_utcOffsetInfo.setOffsetInSeconds(signFactor * 60 * (60 * tzh + tzm));

    return rv;
}

DateTime DateTime::fromStdTm(const std::tm& tm, const UtcOffsetInfo utcOffsetInfo)
{
    DateTime dt(
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        0,
        utcOffsetInfo);
    DFG_STATIC_ASSERT(static_cast<uint8>(DayOfWeek::Sunday) == 0, "Implementation assumes sunday == 0");
    dt.m_dayOfWeek = static_cast<DayOfWeek>(tm.tm_wday);
    return dt;
}

#ifdef _WIN32
DateTime::DateTime(const SYSTEMTIME& st)
{
    m_year = st.wYear;
    m_month = static_cast<uint8>(st.wMonth);
    m_day = static_cast<uint8>(st.wDay);
    DFG_STATIC_ASSERT(static_cast<uint8>(DayOfWeek::Sunday) == 0, "Implementation assumes sunday == 0");
    m_dayOfWeek = static_cast<DayOfWeek>(st.wDayOfWeek);
    m_milliSecSinceMidnight = millisecondsSinceMidnight(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

namespace
{
    static FILETIME systemTimeToFileTime(const SYSTEMTIME& st)
    {
        FILETIME ft{};
        if (SystemTimeToFileTime(&st, &ft) != 0)
            return ft;
        else
        {
            ft.dwHighDateTime = maxValueOfType<decltype(ft.dwHighDateTime)>();
            ft.dwLowDateTime = maxValueOfType<decltype(ft.dwLowDateTime)>();
            return ft;
        }
    }

    static SYSTEMTIME fileTimeToSystemTime(const FILETIME& ft)
    {
        SYSTEMTIME st{};
        if (FileTimeToSystemTime(&ft, &st) != 0)
            return st;
        else
            return SYSTEMTIME{};
    }
}

auto DateTime::privTimeDiff(const SYSTEMTIME& st0, const SYSTEMTIME& st1) -> std::chrono::duration<int64, std::ratio<1, 10000000>>
{
    const auto toInteger = [](const _SYSTEMTIME& st)
    {
        const FILETIME ft = systemTimeToFileTime(st);
        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;
        return std::chrono::duration<int64, std::ratio<1, 10000000>>(static_cast<int64>(uli.QuadPart));
    };

    const auto i0 = toInteger(st0);
    const auto i1 = toInteger(st1);
    return i1 - i0;
}

auto DateTime::timeDiffInSecondsI(const SYSTEMTIME& st0, const SYSTEMTIME& st1) -> std::chrono::seconds
{
    const auto diff = privTimeDiff(st0, st1);
    typedef decltype(diff) TimeDiffReturnType;
    return std::chrono::seconds(diff.count() * TimeDiffReturnType::period::num / TimeDiffReturnType::period::den);
}

auto DateTime::dayOfWeek() const -> DayOfWeek
{
    if (m_dayOfWeek != DayOfWeek::unknown)
        return m_dayOfWeek;
    auto st = toSYSTEMTIME();
    if (st.wYear != 0)
    {
        DFG_STATIC_ASSERT(static_cast<uint8>(DayOfWeek::Sunday) == 0, "Implementation assumes sunday == 0");
        return static_cast<DayOfWeek>(st.wDayOfWeek);
    }
    else
        return DayOfWeek::unknown;
}

SYSTEMTIME DateTime::toSYSTEMTIME() const
{
    SYSTEMTIME st;

    st.wYear = m_year;
    st.wMonth = m_month;
    st.wDayOfWeek = 0; // According to documentation of SystemTimeToFileTime() "The wDayOfWeek member of the SYSTEMTIME structure is ignored." so doesn't matter if this is wrong at this point.
    st.wDay = m_day;
    st.wHour = hour();
    st.wMinute = minute();
    st.wSecond = second();
    st.wMilliseconds = millisecond();

    if (this->m_dayOfWeek != DayOfWeek::unknown)
    {
        DFG_STATIC_ASSERT(static_cast<uint8>(DayOfWeek::Sunday) == 0, "Implementation assumes sunday == 0");
        const auto dayOfWeekNumeric = static_cast<uint8>(this->m_dayOfWeek);
        st.wDayOfWeek = dayOfWeekNumeric;
    }
    else
    {
        // Hack: filling in weekday by roundtrip conversion with FileTime
        const auto ft = systemTimeToFileTime(st);
        st = fileTimeToSystemTime(ft);
    }
    
    return st;
}
#endif // _WIN32

namespace DFG_DETAIL_NS
{
#ifndef _WIN32
    template <class Func_T>
    DateTime createDateTime(Func_T timeFunc)
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr);

        const time_t secsSinceEpoch = tv.tv_sec;
        struct tm df;
        timeFunc(&secsSinceEpoch, &df);
        DateTime dt(df.tm_year + 1900, df.tm_mon, df.tm_mday, df.tm_hour, df.tm_min, df.tm_sec, tv.tv_usec / 1000);
        return dt;
    }
#endif
} // Detail namespace


auto DateTime::millisecondsSinceMidnight(const int hour, const int minutes, const int seconds, const int milliseconds) -> uint32
{
    return 1000 * (hour * 3600 + minutes * 60 + seconds) + milliseconds;
}

auto DateTime::systemTime_utc() -> DateTime
{
#ifdef _WIN32
    SYSTEMTIME stUtc;
    GetSystemTime(&stUtc); // Returned time is in UTC
    DateTime dt(stUtc);
    dt.m_utcOffsetInfo.setOffsetInSeconds(0);
    return dt;
#else
    auto dt = ::DFG_MODULE_NS(time)::DFG_DETAIL_NS::createDateTime(&gmtime_r);
    dt.m_utcOffsetInfo.setOffsetInSeconds(0);
    return dt;
#endif
}

auto DateTime::systemTime_local() -> DateTime
{
#ifdef _WIN32
    SYSTEMTIME stUtc;
    GetSystemTime(&stUtc); // Returned time is in UTC
    SYSTEMTIME stLocal;
    SystemTimeToTzSpecificLocalTime(nullptr /*nullptr = use current time zone */, &stUtc, &stLocal);
    DateTime dt(stLocal);
    dt.m_utcOffsetInfo.setOffset(timeDiffInSecondsI(stUtc, stLocal));
    return dt;
#else
    auto dt = ::DFG_MODULE_NS(time)::DFG_DETAIL_NS::createDateTime(&localtime_r);
    DFG_ASSERT_IMPLEMENTED(false); // TODO: set UTC-offset.
    return dt;
#endif
}

bool DateTime::isNull() const
{
    return m_year == 0 && m_month == 0 && m_day == 0 && m_dayOfWeek == DayOfWeek::unknown && m_milliSecSinceMidnight == 0 && !m_utcOffsetInfo.isSet();
}

std::tm DateTime::toStdTm_utcOffsetIgnored() const
{
    std::tm tm{};
    if (year() < 1900)
        return tm;
    tm.tm_year = year() - 1900;
    tm.tm_mon  = month() - 1; // std::tm uses range [0, 11]
    tm.tm_mday = day();

    tm.tm_hour  = hour();
    tm.tm_min   = minute();
    tm.tm_sec   = second();
    tm.tm_wday  = -1;
    tm.tm_yday  = -1;
    tm.tm_isdst = -1; // "negative if no information is available" (https://en.cppreference.com/w/cpp/chrono/c/tm)
    return tm;
}

#ifdef _WIN32
std::chrono::duration<double> DateTime::secondsTo(const DateTime& other) const
{
    return privTimeDiff(toSYSTEMTIME(), other.toSYSTEMTIME()) - std::chrono::duration<double>(m_utcOffsetInfo.offsetDiffInSeconds(other.m_utcOffsetInfo));
}

bool DateTime::isLocalDateTimeEquivalent(const DateTime& other) const
{
    return
        this->year()        == other.year()   &&
        this->month()       == other.month()  &&
        this->day()         == other.day()    &&
        this->hour()        == other.hour()   &&
        this->minute()      == other.minute() &&
        this->second()      == other.second() &&
        this->millisecond() == other.millisecond();
}

bool DateTime::isLocalDateTimeEquivalent(const std::tm& tm) const
{
    return isLocalDateTimeEquivalent(fromStdTm(tm));
}

int64 DateTime::toSecondsSinceEpoch() const
{
    if (!utcOffsetInfo().isSet())
        return 0;
    auto tm = toStdTm_utcOffsetIgnored();
    const auto t = _mkgmtime64(&tm);
    return static_cast<int64>(t) - utcOffsetInfo().offsetInSeconds();
}

int64 DateTime::toMillisecondsSinceEpoch() const
{
    auto val = toSecondsSinceEpoch();
    val *= 1000;
    val += this->millisecond();
    return val;
}

#endif // _WIN32

} } // namespace dfg::time

#endif // DFG_LANGFEAT_CHRONO_11
