#pragma once
#include "../dfgDefs.hpp"
#include "../dfgBaseTypedefs.hpp"
#include "../numericTypeTools.hpp"
#include "../build/languageFeatureInfo.hpp"
#include "../ReadOnlySzParam.hpp"
#include <ctime>
#include <limits>

#if DFG_LANGFEAT_CHRONO_11
#include <chrono>
#endif // DFG_LANGFEAT_CHRONO_11

#ifdef _WIN32
    struct _SYSTEMTIME;
#endif

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(time) {

enum class DayOfWeek : uint8 { Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, unknown };

DayOfWeek toDayOfWeek(const std::tm&);
#ifdef _WIN32
    DayOfWeek toDayOfWeek(const _SYSTEMTIME&);
#endif

enum class TimeZone
{
    Z = 0,
    plus1h = 1 * 3600, plus2h = 2 * 3600, plus3h = 3 * 3600, plus4h = 4 * 3600, plus5h = 5 * 3600, plus6h = 6 * 3600,
    plus7h = 7 * 3600, plus8h = 8 * 3600, plus9h = 9 * 3600, plus10h = 10 * 3600, plus11h = 11 * 3600, plus12h = 12 * 3600,
    minus1h = -1 * 3600, minus2h = -2 * 3600, minus3h = -3 * 3600, minus4h = -4 * 3600, minus5h = -5 * 3600, minus6h = -6 * 3600,
    minus7h = -7 * 3600, minus8h = -8 * 3600, minus9h = -9 * 3600, minus10h = -10 * 3600, minus11h = -11 * 3600, minus12h = -12 * 3600,
};

// Thread-safe wrapper for std::gmtime(): i.e. function that converts std::time to std::tm UTC calendar datetime. If conversion fails, returns zero-initialized std::tm.
std::tm stdGmTime(const time_t t);

// Inverse of stdGmTime(): converts std::tm-structure representing date time in UTC to std::time_t
std::time_t tmUtcToTime_t(const std::tm& tm);

#if DFG_LANGFEAT_CHRONO_11
class UtcOffsetInfo
{
public:
    static const int32 s_nNotSetValueMimicing = NumericTraits<int32>::minValue; // With the 'mimicing not set'-value this UtcOffsetInfo mimics UtcOffsetInfo of another operand in certain operations.
    UtcOffsetInfo() :
        m_offsetValue(s_nNotSetValueMimicing)
    {
    }

    // Positive means +HH:MM offset
    UtcOffsetInfo(const std::chrono::seconds& offset) :
        m_offsetValue(saturateCast<int>(offset.count()))
    {
    }

    // Convenience overload to construct UtcOffsetInfo(TimeZone::Z) instead of UtcOffsetInfo(std::chrono::seconds(0))
    UtcOffsetInfo(const TimeZone tz)
        : UtcOffsetInfo(std::chrono::seconds(static_cast<int>(tz)))
    {
    }

    // If both return true from isSet(), return value is other.setOffsetInSeconds() - this->offsetDiffInSeconds()
    int32 offsetDiffInSeconds(const UtcOffsetInfo& other) const;

    bool isSet() const { return m_offsetValue != s_nNotSetValueMimicing; }

    // Returns +HH:MM offset in seconds, e.g. +01:00 returns 3600. If isSet() == false, returns 0.
    int32 offsetInSeconds() const { return (isSet()) ? m_offsetValue : 0; }
    void setOffset(const std::chrono::seconds& offset) { m_offsetValue = saturateCast<int>(offset.count()); }
    void setOffsetInSeconds(const int nOffset) { m_offsetValue = nOffset; }

    int32 m_offsetValue; // Timezone specifier in seconds or unset (=s_nNotSetValueMimicing). Positive means +HH:MM timezone, e.g. value 3600 means +01:00.
}; // class UtcOffsetInfo

class DateTime
{
public:
    DateTime(); // Constructs DateTime which doesn't represent any datetime and for which isNull() is true.

    // Constructs DateTime from given datetime items. If given items do not form a valid DateTime, state of DateTime object is unspecified.
    // year         : year as such
    // month        : in range 1-12
    // day          : in range 1-31
    // hour         : in range 0-23
    // minute       : in range 0-59
    // second       : in range 0-59
    // millisecond  : in range 0-999
    // utcOffsetInfo: defines how this DateTime is related to UTC time
	DateTime(int year, int month, int day, int hour, int minute, int second, int milliseconds, UtcOffsetInfo utcOffsetInfo = UtcOffsetInfo());

    // Creates DateTime from a string
    // Supported formats
    //      yyyy-mm-dd
    //      yyyy-mm-ddXhh:mm:ss
    //      yyyy-mm-ddXhh:mm:ssZ
    //      yyyy-mm-ddXhh:mm:ss+hh
    //      yyyy-mm-ddXhh:mm:ss+hh:mm
    //      yyyy-mm-ddXhh:mm:ss.zzz
    //      yyyy-mm-ddXhh:mm:ss.zzzZ
    //      yyyy-mm-ddXhh:mm:ss.zzz+hh:mm
    //      yyyy-mm-ddXhh:mm:ss.zzz-hh:mm
    // where X is either ' ' or 'T'
    // If input string does not have supported format or has invalid values (e.g. month 13), returned DateTime can be null or unspecified:
    //      -most invalid formats return null DateTime.
    //      -non-null return is not guaranteed to be valid DateTime.
    static DateTime fromString(const StringViewC& sv);

    // Constructs DateTime from std::tm.
    // By default information in std::tm is interpreted as if being timezone-less datetime
    // effectively equivalent to fromString("yyyy-mm-dd hh:mm:ss").
    // Member tm_isdst is ignored.
    // If std::tm does not represent a valid date time, state of returned DateTime is unspecified.
    static DateTime fromStdTm(const std::tm& tm, UtcOffsetInfo utcOffsetInfo = UtcOffsetInfo());

    // Constructs DateTime from std::time_t. If time_t is valid, resulting DateTime has identical epochTime, but
    // calendar DateTime depends on utcOffset, which by default is 0.
    // If time_t is not valid, state of returned DateTime is unspecified.
    // For example if 't' corresponds to 2021-12-08 12:00:00, by default resulting DateTime is 2021-12-08 12:00:00Z
    // If given offset 3600, resulting DateTime is 2021-12-08 13:00:00+01:00
    static DateTime fromTime_t(std::time_t t, std::chrono::seconds utcOffset = std::chrono::seconds(0));

    // Returns std::tm not taking UTC offset into account nor milliseconds, i.e. as if UTC offset and milliseconds were zero.
    // Members tm_wday, tm_yday and tm_isdst are set to -1.
    // If year is < 1900, returns zero-initialized std::tm.
    // If 'this' is not a valid datetime, state of returned std::tm is unspecified.
    std::tm toStdTm_utcOffsetIgnored() const;

    // Returns true iff 'this' is equivalent to default-constructed DateTime.
    bool isNull() const;

#ifdef _WIN32
    // Constructs DateTime from given SYSTEMTIME. If given SYSTEMTIME is not valid, state of DateTime object is unspecified.
    explicit DateTime(const _SYSTEMTIME& st);

    // Converts 'this' to SYSTEMTIME ignoring UTC offset, i.e. interpreting 'this' as if having UTC offset 0. 
    // If conversion fails, returns zero-initialized SYSTEMTIME
    _SYSTEMTIME toSYSTEMTIME() const;

    // Returns time difference between two SYSTEMTIME's in units of 100 ns, return value is positive if st0 is before st1.
    static std::chrono::duration<int64, std::ratio<1, 10000000>> privTimeDiff(const _SYSTEMTIME& st0, const _SYSTEMTIME& st1);
    // Returns time difference between two SYSTEMTIME's in seconds
    static std::chrono::seconds timeDiffInSecondsI(const _SYSTEMTIME& st0, const _SYSTEMTIME& st1);
#endif

    // Returns seconds from 'this' to other, positive if 'this' is before 'other'.
    // Leaps seconds are ignored, i.e. real monotonic time difference between the dates may differ from the result.
    // Return value can be NaN if difference can't be determined.
    // Note about UtcOffset handling:
    //      -If either one of the DateTime's have unset UtcOffsetInfo, both DateTime are treated as if having the same UtcOffsetInfo.
    //      -While this function does not require presence of UtcOffsetInfo, it's worth noting that DateTime's not having UtcOffsetInfo
    //       are inherently error prone in time systems that use dayligth saving:
    //          -If time jumps from 04:00 -> 03:00 due to DST adjustment, time difference between local date time
    //           yyyy-mm-dd 03:30:00 and yyyy-mm-dd 03:30:00 can be 0 or 3600 s - this function returns 0.
    //          -If time jumps from 03:00 -> 04:00 due to DST adjustment
    //              -time difference between local date time yyyy-mm-dd 02:59:00 and yyyy-mm-dd 03:30:00 is not defined - this function returns 31 * 60 s.
    //              -time difference between local date time yyyy-mm-dd 02:59:00 and yyyy-mm-dd 04:01:00 is 2 minutes - this function returns 62 * 60 s.
	std::chrono::duration<double> secondsTo(const DateTime& other) const;

    DayOfWeek dayOfWeek() const;

    // If 'this' is valid datetime for unix time and its UtfOffsetInfo or given UtcOffsetInfo structure is set, returns corresponding unix time; if unable to compute result, returns -1.
    // If utcOffset is given as argument, it overrides utcOffsetInfo in 'this'.
    // Milliseconds are treated as zero.
    int64 toSecondsSinceEpoch(UtcOffsetInfo utcOffset = UtcOffsetInfo()) const;
    int64 toMillisecondsSinceEpoch(UtcOffsetInfo utcOffset = UtcOffsetInfo()) const; // Like toSecondsSinceEpoch, but takes milliseconds into account and returned value is in milliseconds (or -1 on error).

    // Convenience method, effectively a wrapper for toSecondsSinceEpoch().
    std::time_t toTime_t() const;

    // Returns true iff 'this' and 'other' have equivalent year, month, day, hour, minute, second and millisecond parts.
    bool isLocalDateTimeEquivalent(const DateTime& other) const;
    // Returns isLocalDateTimeEquivalent(*this, fromStdTm(tm)).
    bool isLocalDateTimeEquivalent(const std::tm& tm) const;

    // Returns local system time.
    // On Windows, returned value is guaranteed to return system (OS) time that is not dependent on TZ environment variable. This behaviour differs from that of e.g. std::localtime (see systemTime_local-test)
    // On other platforms, TZ environment variable may affect return value. Also state of UtcOffsetInfo is unspecified.
    static DateTime systemTime_local();
    // Returns system time in UTC.
    static DateTime systemTime_utc();

    // Returns milliseconds since midnight given hh:mm:ss.zzz. If input values are not valid, behaviour is undefined.
    static uint32 millisecondsSinceMidnight(int hour, int minutes, int seconds, int milliseconds);
    uint32 millisecondsSinceMidnight() const { return m_milliSecSinceMidnight; }

    // Returns millisecond part of day time, i.e. 'zzz' in hh:mm:ss.zzz
    uint16 millisecond() const
    {
        return m_milliSecSinceMidnight % 1000;
    }

    uint16 year() const { return m_year; }
    uint8 month() const { return m_month; } // Returns month in range [1, 12]
    uint8 day() const { return m_day; }     // Returns month day in range [1, 31]

    // Returns second part of day time, i.e. 'ss' in hh:mm:ss
    uint8 second() const
    {
        return static_cast<uint8>(((m_milliSecSinceMidnight / 1000) % 3600) % 60);
    }

    // Returns second fraction as double, e.g. if time is 12:01:02.125, returns 2.125. May return NaN.
    double secondsAsDouble() const
    {
        return (!isNull()) ? second() + (millisecond() / 1000.0) : std::numeric_limits<double>::quiet_NaN();
    }

    // Returns second part of day time, i.e. 'mm' in hh:mm:ss
    uint8 minute() const
    {
        return static_cast<uint8>(((m_milliSecSinceMidnight / 1000) % 3600) / 60);
    }

    // Returns hour part of day time, i.e. 'hh' in hh:mm:ss
    uint8 hour() const
    {
        return static_cast<uint8>(((m_milliSecSinceMidnight / 1000) / 3600));
    }

	const UtcOffsetInfo& utcOffsetInfo() const { return m_utcOffsetInfo; }

    uint16 m_year;
    uint8 m_month; // In range 1-12
    uint8 m_day;   // In range 1-31
    DayOfWeek m_dayOfWeek = DayOfWeek::unknown;
    uint32 m_milliSecSinceMidnight = 0;
    UtcOffsetInfo m_utcOffsetInfo;
}; // class DateTime

#endif // DFG_LANGFEAT_CHRONO_11

} } // module namespace
