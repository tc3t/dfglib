#pragma once
#include "../dfgDefs.hpp"
#include "../dfgBaseTypedefs.hpp"
#include "../numericTypeTools.hpp"
#include "../build/languageFeatureInfo.hpp"
#include "../ReadOnlySzParam.hpp"

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

    // If isSet() == false, returns 0.
    int32 offsetInSeconds() const { return (isSet()) ? m_offsetValue : 0; }
    void setOffset(const std::chrono::seconds& offset) { m_offsetValue = static_cast<int>(offset.count()); }
    void setOffsetInSeconds(const int nOffset) { m_offsetValue = nOffset; }

    int32 m_offsetValue; // Positive means ahead of UTC-time, e.g. value 3600 means 1 hour ahead of UTC.
}; // class UtcOffsetInfo

class DateTime
{
public:
    DateTime();
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

#ifdef _WIN32
    explicit DateTime(const _SYSTEMTIME& st);

    _SYSTEMTIME toSystemTime() const;

    // Return value is positive if st0 < st1
    static std::chrono::duration<int64, std::ratio<1, 10000000>> privTimeDiff(const _SYSTEMTIME& st0, const _SYSTEMTIME& st1);
    static std::chrono::seconds timeDiffInSecondsI(const _SYSTEMTIME& st0, const _SYSTEMTIME& st1);

    // Return value is positive if *this < other
	std::chrono::duration<double> secondsTo(const DateTime& other) const;
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
    uint8 month() const { return m_month; }
    uint8 day() const { return m_day; }

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
