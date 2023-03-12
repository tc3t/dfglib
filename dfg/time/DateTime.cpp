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

DayOfWeek toDayOfWeek(const std::tm& tm)
{
    DFG_STATIC_ASSERT(static_cast<uint8>(DayOfWeek::Sunday) == 0, "Implementation assumes sunday == 0");
    return (tm.tm_wday >= 0 && tm.tm_wday <= 6) ? static_cast<DayOfWeek>(tm.tm_wday) : DayOfWeek::unknown;
}

#ifdef _WIN32
DayOfWeek toDayOfWeek(const SYSTEMTIME& st)
{
    DFG_STATIC_ASSERT(static_cast<uint8>(DayOfWeek::Sunday) == 0, "Implementation assumes sunday == 0");
    DFG_STATIC_ASSERT((std::is_unsigned_v<decltype(st.wDayOfWeek)>), "Condition below assumes unsigned wDayOfWeek");
    return (st.wDayOfWeek <= 6) ? static_cast<DayOfWeek>(st.wDayOfWeek) : DayOfWeek::unknown;
}
#endif // _WIN32

std::tm stdGmTime(const time_t t)
{
    std::tm tm{};
#ifdef _WIN32
    const auto err = gmtime_s(&tm, &t);
    return (err == 0) ? tm : std::tm{};
#else
    return (gmtime_r(&t, &tm) != nullptr) ? tm : std::tm{};
#endif
}

std::time_t tmUtcToTime_t(std::tm& tm)
{
    const auto origTm = tm;
 #ifdef _WIN32
    const auto t = _mkgmtime64(&tm);
#else
    // https://stackoverflow.com/questions/283166/easy-way-to-convert-a-struct-tm-expressed-in-utc-to-time-t-type
    // https://stackoverflow.com/questions/12353011/how-to-convert-a-utc-date-time-to-a-time-t-in-c
    const auto t = timegm(&tm);
#endif
    // Functions above don't return error on (some) invalid dates, they just adjusts fields. 
    // So checking that date wasn't changed.
    if (origTm.tm_year != tm.tm_year || origTm.tm_mon != tm.tm_mon || origTm.tm_mday != tm.tm_mday)
        return std::time_t(-1);
    return t;
}

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
    if (month < 0 || month > 12 || day < 0 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59 || milliseconds < 0 || milliseconds > 999)
    {
        *this = DateTime();
        return;
    }
    m_year = saturateCast<uint16>(year);
    m_month = saturateCast<uint8>(month);
    m_day = saturateCast<uint8>(day);
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
        limited(tm.tm_year, -10000, 10000) + 1900, // Some arbitrary limits to prevent overflows, but lets outright bad input values to still get noticed in constructor.
        limited(tm.tm_mon, -10, 20) + 1, // Limiting like above.
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        0,
        utcOffsetInfo);
    if (!dt.isNull())
        dt.m_dayOfWeek = toDayOfWeek(tm);
    return dt;
}

DateTime DateTime::fromTime_t(const std::time_t t, const std::chrono::seconds utcOffset)
{
    if (utcOffset.count() < -24 * 60 * 60 || utcOffset.count() > 24 * 60 * 60)
        return DateTime();
    // First determining calendar date in UTC with given offset.
    const auto tm = stdGmTime(t + utcOffset.count());
    auto dt = fromStdTm(tm);
    dt.m_utcOffsetInfo.setOffset(utcOffset);
    return dt;
}

#ifdef _WIN32

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

    bool isOutrightInvalidSYSTEMTIME(const SYSTEMTIME& st)
    {
        return (st.wYear < 1601 || st.wYear > 30827 || st.wDay == 0 || st.wMonth == 0);
    }
}

DateTime::DateTime(const SYSTEMTIME& st)
{
    if (isOutrightInvalidSYSTEMTIME(st))
    {
        *this = DateTime();
        return;
    }
    m_year = st.wYear;
    m_month = saturateCast<uint8>(st.wMonth);
    m_day = saturateCast<uint8>(st.wDay);
    m_dayOfWeek = toDayOfWeek(st);
    m_milliSecSinceMidnight = millisecondsSinceMidnight(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

auto DateTime::privTimeDiff(const SYSTEMTIME& st0, const SYSTEMTIME& st1) -> std::chrono::duration<int64, std::ratio<1, 10000000>>
{
    const auto toInteger = [](const _SYSTEMTIME& st)
    {
        const FILETIME ft = systemTimeToFileTime(st);
        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;
        return std::chrono::duration<int64, std::ratio<1, 10000000>>(saturateCast<int64>(uli.QuadPart));
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
    return fromStdTm(stdGmTime(std::time(nullptr)), UtcOffsetInfo(TimeZone::Z));
#if 0 // Old implementations, left for reference to see how platform-specific functions can be used if need be.
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

std::chrono::duration<double> DateTime::secondsTo(const DateTime& other) const
{
    if (this->isNull() || other.isNull())
        return std::chrono::duration<double>(std::numeric_limits<double>::quiet_NaN());
#ifdef _WIN32 // 'else'-implementation works also on Windows, but using SYSTEMTIME since it has wider date support than unix time (e.g. year start from 1601 instead of 1970)
    const auto offsetDiff = std::chrono::duration<double>(m_utcOffsetInfo.offsetDiffInSeconds(other.m_utcOffsetInfo));
    const auto thisSt = toSYSTEMTIME();
    const auto otherSt = other.toSYSTEMTIME();
    if (isOutrightInvalidSYSTEMTIME(thisSt) || isOutrightInvalidSYSTEMTIME(otherSt))
        return std::chrono::duration<double>(std::numeric_limits<double>::quiet_NaN());
    const auto rawDiff = privTimeDiff(toSYSTEMTIME(), other.toSYSTEMTIME());
    return rawDiff - offsetDiff;
#else
    const auto n0 = toMillisecondsSinceEpoch(TimeZone::Z);
    const auto n1 = other.toMillisecondsSinceEpoch(TimeZone::Z);
    if (n0 < 0 || n1 < 0)
        return std::chrono::duration<double>(std::numeric_limits<double>::quiet_NaN());
    const auto nOffsetDiff = std::chrono::duration<double>(m_utcOffsetInfo.offsetDiffInSeconds(other.m_utcOffsetInfo));
    return std::chrono::duration<double>(std::chrono::milliseconds(n1 - n0)) - nOffsetDiff;
#endif
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

std::time_t DateTime::toTime_t() const
{
    return saturateCast<std::time_t>(toSecondsSinceEpoch());
}

int64 DateTime::toSecondsSinceEpoch(const UtcOffsetInfo utcOffset) const
{
    if (!utcOffsetInfo().isSet() && !utcOffset.isSet())
        return -1;
    auto tm = toStdTm_utcOffsetIgnored();
    const auto t = tmUtcToTime_t(tm);
    const auto effectiveUtcOffset = (utcOffset.isSet()) ? utcOffset : this->m_utcOffsetInfo;

    // Note: assuming that t is in seconds
    return (t >= 0) ? static_cast<int64>(t) - effectiveUtcOffset.offsetInSeconds() : -1;
}

int64 DateTime::toMillisecondsSinceEpoch(const UtcOffsetInfo utcOffset) const
{
    auto val = toSecondsSinceEpoch(utcOffset);
    if (val < 0)
        return -1;
    val *= 1000;
    val += this->millisecond();
    return val;
}

auto DateTime::dayOfWeek() const -> DayOfWeek
{
    if (m_dayOfWeek != DayOfWeek::unknown)
        return m_dayOfWeek;
#ifdef _WIN32 // 'else'-implementation works also on Windows, but using SYSTEMTIME since it has wider date support (e.g. year start from 1601 instead of 1900/1970)
    auto st = toSYSTEMTIME();
    return (st.wYear != 0) ? toDayOfWeek(st) : DayOfWeek::unknown;
#else
    auto tm = toStdTm_utcOffsetIgnored();
    tm.tm_isdst = 0;
    const auto t = tmUtcToTime_t(tm); // Note: this updates tm if successful (tm_wday in particular)
    return (t >= 0) ? toDayOfWeek(tm) : DayOfWeek::unknown;
#endif
}

} } // namespace dfg::time

#endif // DFG_LANGFEAT_CHRONO_11
