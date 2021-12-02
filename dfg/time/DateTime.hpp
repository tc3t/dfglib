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


#if DFG_LANGFEAT_CHRONO_11
class UtcOffsetInfo
{
public:
    static const int32 s_nNotSetValueMimicing = NumericTraits<int32>::minValue; // With the 'mimicing not set'-value this UtcOffsetInfo mimics UtcOffsetInfo of another operand in certain operations.
    UtcOffsetInfo() :
        m_offsetValue(s_nNotSetValueMimicing)
    {
    }

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
	DateTime(int year, int month, int day, int hour, int minute, int second, int milliseconds, UtcOffsetInfo utcOffsetInfo = UtcOffsetInfo());

    // Parses DateTime from string
    // Supported formats
    //      yyyy-mm-ddX
    //      yyyy-mm-ddXhh:mm:ss
    //      yyyy-mm-ddXhh:mm:ssZ
    //      yyyy-mm-ddXhh:mm:ss+hh
    //      yyyy-mm-ddXhh:mm:ss+hh:mm
    //      yyyy-mm-ddXhh:mm:ss.zzz
    //      yyyy-mm-ddXhh:mm:ss.zzzZ
    //      yyyy-mm-ddXhh:mm:ss.zzz+hh:mm
    //      yyyy-mm-ddXhh:mm:ss.zzz-hh:mm
    // where X is either ' ' or 'T'
    // If input string does not have supported format or has invalid values (e.g. month 13), behaviour is undefined (i.e. function must not be used for untrusted strings).
    static DateTime fromString(const StringViewC& sv);

    // Returns std::tm not taking UTC offset into account nor milliseconds, i.e. as if UTC offset and milliseconds were zero.
    // Members tm_wday, tm_yday and tm_isdst are set to -1.
    // If year is < 1900, returns zero-initialized std::tm.
    // If 'this' is not a valid datetime, behaviour is undefined.
    std::tm toStdTm_utcOffsetIgnored() const;

    // Returns true iff 'this' is equivalent to default-constructed DateTime.
    bool isNull() const;

#ifdef _WIN32
    explicit DateTime(const _SYSTEMTIME& st);

    _SYSTEMTIME toSystemTime() const;

    // Return value is positive if st0 < st1
    static std::chrono::duration<int64, std::ratio<1, 10000000>> privTimeDiff(const _SYSTEMTIME& st0, const _SYSTEMTIME& st1);
    static std::chrono::seconds timeDiffInSecondsI(const _SYSTEMTIME& st0, const _SYSTEMTIME& st1);

    // Return value is positive if *this < other
	std::chrono::duration<double> secondsTo(const DateTime& other) const;

    // If 'this' is valid datetime for unix time and UtfOffset structure is set, returns corresponding unix time; otherwise behaviour is undefined.
    // Milliseconds are treated as zero.
    int64 toSecondsSinceEpoch() const;
    int64 toMillisecondsSinceEpoch() const; // Like toSecondsSinceEpoch, but takes milliseconds into account and returned value is in milliseconds
#endif

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
    uint32 m_milliSecSinceMidnight;
    UtcOffsetInfo m_utcOffsetInfo;
}; // class DateTime

#endif // DFG_LANGFEAT_CHRONO_11

} } // module namespace
