#include "DateTime.hpp"
#include "../dfgAssert.hpp"
#include "../str/strTo.hpp"
#include "../alg.hpp"

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

    if (sv.size() >= 8 && sv[4] == '-' && sv[7] == '-') // Something starting with ????-??-?? (ISO 8601, https://en.wikipedia.org/wiki/ISO_8601)
    {
        rv.m_year = strTo<int>(sv.substr_startCount(0, 4));
        rv.m_month = strTo<int>(sv.substr_startCount(5, 2));
        rv.m_day = strTo<int>(sv.substr_startCount(8, 2));

        const auto isTzStartChar = [](const char& c) { return ::DFG_MODULE_NS(alg)::contains("Z+-", c); };

        // size 19 is yyyy-MM-ddThh:mm:ss
        if (sv.size() >= 19 && sv[13] == ':' && sv[16] == ':' && ::DFG_MODULE_NS(alg)::contains("T ", sv[10])) // Case ????-??-??[T ]hh:mm:ss[.zzz][Z|HH:MM]
        {
            const auto h = strTo<int>(sv.substr_startCount(11, 2));
            const auto m = strTo<int>(sv.substr_startCount(14, 2));
            const auto s = strTo<int>(sv.substr_startCount(17, 2));
            const bool bHasMsDot = (sv.size() >= 23 && sv[19] == '.');
            const auto ms = (bHasMsDot) ? strTo<int>(sv.substr_startCount(20, 3)) : 0;

            rv.m_milliSecSinceMidnight = millisecondsSinceMidnight(h, m, s, ms);

            // Timezone specifier after milliseconds? Accepted formats: Z, +-hh, +-hh:mm
            if ((bHasMsDot && sv.size() >= 24 && isTzStartChar(sv[23])) || (!bHasMsDot && sv.size() >= 20 && isTzStartChar(sv[19])))
            {
                const size_t nStartPos = (bHasMsDot) ? 23 : 19;
                if (sv[nStartPos] == 'Z')
                    rv.m_utcOffsetInfo.setOffsetInSeconds(0);
                else if (sv.size() > nStartPos + 2)
                {
                    const auto signFactor = (sv[nStartPos] == '+') ? 1 : -1;
                    const auto tzh = strTo<int>(sv.substr_startCount(nStartPos + 1, 2));
                    const auto tzm = (sv.size() > nStartPos + 5) ? strTo<int>(sv.substr_startCount(nStartPos + 4, 2)) : 0;
                    rv.m_utcOffsetInfo.setOffsetInSeconds(signFactor * 60 * (60 * tzh + tzm));

                }
            }
        }
    }
    return rv;
}

#ifdef _WIN32
DateTime::DateTime(const SYSTEMTIME& st)
{
    m_year = st.wYear;
    m_month = static_cast<uint8>(st.wMonth);
    m_day = static_cast<uint8>(st.wDay);
    m_milliSecSinceMidnight = millisecondsSinceMidnight(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

auto DateTime::privTimeDiff(const SYSTEMTIME& st0, const SYSTEMTIME& st1) -> std::chrono::duration<int64, std::ratio<1, 10000000>>
{
    const auto toInteger = [](const _SYSTEMTIME& st)
    {
        FILETIME ft;
        SystemTimeToFileTime(&st, &ft);
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

SYSTEMTIME DateTime::toSystemTime() const
{
    SYSTEMTIME st;
    st.wYear = m_year;
    st.wMonth = m_month;;
    //st.wDayOfWeek = ; TODO
    st.wDay = m_day;
    st.wHour = hour();
    st.wMinute = minute();
    st.wSecond = second();
    st.wMilliseconds = millisecond();
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

#ifdef _WIN32
std::chrono::duration<double> DateTime::secondsTo(const DateTime& other) const
{
    return privTimeDiff(toSystemTime(), other.toSystemTime()) - std::chrono::duration<double>(m_utcOffsetInfo.offsetDiffInSeconds(other.m_utcOffsetInfo));
}
#endif

} } // namespace dfg::time

#endif // DFG_LANGFEAT_CHRONO_11
