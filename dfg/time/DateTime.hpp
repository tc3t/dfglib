#pragma once
#include "../dfgDefs.hpp"
#include "../dfgBaseTypedefs.hpp"
#include "../numericTypeTools.hpp"
#include "../build/languageFeatureInfo.hpp"
#include "../ReadOnlySzParam.hpp"
#include <ctime>

#if DFG_LANGFEAT_CHRONO_11
#include <chrono>
#endif // DFG_LANGFEAT_CHRONO_11

#ifdef _WIN32
    struct _SYSTEMTIME;
#endif

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(time) {

enum class DayOfWeek : uint8 { Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, unknown };

// Wrapper for std::gmtime(). If conversion fails, returns zero-initialized std::tm.
// On Windows, guaranteed to be thread-safe (according to https://en.cppreference.com/w/cpp/chrono/c/gmtime std::gmtime() "may not be thread-safe.")
std::tm stdGmTime(const time_t t);

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
        m_offsetValue(static_cast<int>(offset.count()))
    {
    }

    // If both return true from isSet(), return value is other.setOffsetInSeconds() - this->offsetDiffInSeconds()
    int32 offsetDiffInSeconds(const UtcOffsetInfo& other) const;

    bool isSet() const { return m_offsetValue != s_nNotSetValueMimicing; }

    // Returns +HH:MM offset in seconds, e.g. +01:00 returns 3600. If isSet() == false, returns 0.
    int32 offsetInSeconds() const { return (isSet()) ? m_offsetValue : 0; }
    void setOffset(const std::chrono::seconds& offset) { m_offsetValue = static_cast<int>(offset.count()); }
    void setOffsetInSeconds(const int nOffset) { m_offsetValue = nOffset; }

    int32 m_offsetValue; // Timezone specifier in seconds or unset (=s_nNotSetValueMimicing). Positive means +HH:MM timezone, e.g. value 3600 means +01:00.
}; // class UtcOffsetInfo

class DateTime
{
public:
    DateTime(); // Constructs DateTime which doesn't represent any datetime and for which isNull() is true.

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
    // Members tm_yday and tm_isdst are ignored.
    static DateTime fromStdTm(const std::tm& tm, UtcOffsetInfo utcOffsetInfo = UtcOffsetInfo());

    // Constructs DateTime from std::time_t. If time_t is valid, resulting DateTime has identical epochTime, but
    // calendar DateTime depends on utcOffset, which by default is 0.
    // For example if 't' corresponds to 2021-12-08 12:00:00, by default resulting DateTime is 2021-12-08 12:00:00Z
    // If given offset 3600, resulting DateTime is 2021-12-08 13:00:00+01:00
    static DateTime fromTime_t(std::time_t t, std::chrono::seconds utcOffset = std::chrono::seconds(0));

    // Returns std::tm not taking UTC offset into account nor milliseconds, i.e. as if UTC offset and milliseconds were zero.
    // Members tm_wday, tm_yday and tm_isdst are set to -1.
    // If year is < 1900, returns zero-initialized std::tm.
    // If 'this' is not a valid datetime, behaviour is undefined.
    std::tm toStdTm_utcOffsetIgnored() const;

    // Returns true iff 'this' is equivalent to default-constructed DateTime.
    bool isNull() const;

#ifdef _WIN32
    explicit DateTime(const _SYSTEMTIME& st);

    // Converts 'this' to SYSTEMTIME ignoring UTC offset, i.e. interpreting 'this' as if having UTC offset 0. 
    // If conversion fails, returns zero-initialized SYSTEMTIME
    _SYSTEMTIME toSYSTEMTIME() const;

    // Return value is positive if st0 < st1
    static std::chrono::duration<int64, std::ratio<1, 10000000>> privTimeDiff(const _SYSTEMTIME& st0, const _SYSTEMTIME& st1);
    static std::chrono::seconds timeDiffInSecondsI(const _SYSTEMTIME& st0, const _SYSTEMTIME& st1);

    // Returns seconds from 'this' to other, positive if 'this' is before 'other'.
    // Real, accurate-to-seconds time difference between the dates may differ from the result since leap second handling is unspecified.
	std::chrono::duration<double> secondsTo(const DateTime& other) const;

    DayOfWeek dayOfWeek() const;
#endif

    // If 'this' is valid datetime for unix time and UtfOffset structure is set, returns corresponding unix time; otherwise behaviour is undefined.
    // Milliseconds are treated as zero.
    int64 toSecondsSinceEpoch() const;
    int64 toMillisecondsSinceEpoch() const; // Like toSecondsSinceEpoch, but takes milliseconds into account and returned value is in milliseconds

    // Returns true if 'this' and 'other' have equivalent year, month, day, hour, minute, second and millisecond parts.
    bool isLocalDateTimeEquivalent(const DateTime& other) const;
    // Returns true if 'this' and 'tm' have equivalent year, month, day, hour, minute and second parts.
    bool isLocalDateTimeEquivalent(const std::tm& tm) const;

    // Returned value is guaranteed to return system (OS) time that is not dependent on TZ environment variable.
    // This behaviour differs from that of e.g. std::localtime (see systemTime_local-test)
    static DateTime systemTime_local();
    static DateTime systemTime_utc();

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
    uint32 m_milliSecSinceMidnight;
    UtcOffsetInfo m_utcOffsetInfo;
}; // class DateTime

#endif // DFG_LANGFEAT_CHRONO_11

} } // module namespace
