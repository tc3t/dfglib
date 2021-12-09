#include "stdafx.h"

#if (defined(DFGTEST_BUILD_MODULE_TIME) && DFGTEST_BUILD_MODULE_TIME == 1) || (!defined(DFGTEST_BUILD_MODULE_TIME) && DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/time.hpp>
#include <dfg/time/DateTime.hpp>
#include <ctime>
#include <dfg/str/format_fmt.hpp>

#ifdef _WIN32
    #include <Windows.h>
#endif

#if DFG_LANGFEAT_CHRONO_11

#include <chrono>

TEST(dfgTime, DateTime)
{
    using namespace DFG_MODULE_NS(time);
    DateTime dt0(2016, 7, 29, 12, 13, 14, 15);
    DateTime dt1(2016, 7, 29, 12, 13, 15, 15);
    const DateTime dtUtc0(2016, 7, 29, 12, 13, 14, 15, UtcOffsetInfo(std::chrono::seconds(-3600)));
    const DateTime dtUtc1(2016, 7, 29, 12, 13, 14, 15, UtcOffsetInfo(std::chrono::seconds(7200)));
    const DateTime dtUtc2(2016, 7, 30, 23,  2,  3,  4, UtcOffsetInfo(std::chrono::seconds(0)));
    const DateTime dt2(2016, 7, 30, 23, 2, 3, 4);
    const DateTime dtUtc3(2016, 7, 31,  1,  2,  3,  4, UtcOffsetInfo(std::chrono::seconds(7200)));
    DFGTEST_EXPECT_TRUE(DateTime().isNull());
    EXPECT_EQ(dt0.year(), 2016);
    EXPECT_EQ(dt0.month(), 7);
    EXPECT_EQ(dt0.day(), 29);
    EXPECT_EQ(dt0.hour(), 12);
    EXPECT_EQ(dt0.minute(), 13);
    EXPECT_EQ(dt0.second(), 14);
    EXPECT_EQ(dt0.millisecond(), 15);
}

#ifdef _WIN32

TEST(dfgTime, DateTime_dayOfWeek)
{
    using namespace DFG_MODULE_NS(time);
    DateTime dt0(2016, 7, 29, 12, 13, 14, 15);
    const DateTime dtUtc2(2016, 7, 30, 23, 2, 3, 4, UtcOffsetInfo(std::chrono::seconds(0)));
    const DateTime dtUtc3(2016, 7, 31, 1, 2, 3, 4, UtcOffsetInfo(std::chrono::seconds(7200)));

    DFGTEST_EXPECT_LEFT(DayOfWeek::unknown, DateTime().dayOfWeek());
    DFGTEST_EXPECT_LEFT(DayOfWeek::Friday, dt0.dayOfWeek());
    DFGTEST_EXPECT_LEFT(DayOfWeek::Saturday, dtUtc2.dayOfWeek());
    DFGTEST_EXPECT_LEFT(DayOfWeek::Sunday, dtUtc3.dayOfWeek());
}

TEST(dfgTime, DateTime_secondsTo)
{
    using namespace DFG_MODULE_NS(time);
    DateTime dt0(2016, 7, 29, 12, 13, 14, 15);
    DateTime dt1(2016, 7, 29, 12, 13, 15, 15);
    const DateTime dtUtc0(2016, 7, 29, 12, 13, 14, 15, UtcOffsetInfo(std::chrono::seconds(-3600)));
    const DateTime dtUtc1(2016, 7, 29, 12, 13, 14, 15, UtcOffsetInfo(std::chrono::seconds(7200)));
    const DateTime dtUtc2(2016, 7, 30, 23, 2, 3, 4, UtcOffsetInfo(std::chrono::seconds(0)));
    const DateTime dt2(2016, 7, 30, 23, 2, 3, 4);
    const DateTime dtUtc3(2016, 7, 31, 1, 2, 3, 4, UtcOffsetInfo(std::chrono::seconds(7200)));
    const DateTime dt1990(1990, 06, 01, 12, 1, 2, 250, UtcOffsetInfo(std::chrono::seconds(0)));
    const DateTime dt2020(2020, 06, 01, 13, 2, 3, 750, UtcOffsetInfo(std::chrono::seconds(0)));

    DFGTEST_EXPECT_LEFT(std::chrono::seconds(1), dt0.secondsTo(dt1));
    DFGTEST_EXPECT_LEFT(std::chrono::seconds(-1), dt1.secondsTo(dt0));
    DFGTEST_EXPECT_LEFT(std::chrono::seconds(-10800), dtUtc0.secondsTo(dtUtc1));
    DFGTEST_EXPECT_LEFT(std::chrono::seconds(10800), dtUtc1.secondsTo(dtUtc0));
    DFGTEST_EXPECT_LEFT(0, dtUtc2.secondsTo(dtUtc3).count());
    DFGTEST_EXPECT_LEFT(7200, dt2.secondsTo(dtUtc3).count());
    DFGTEST_EXPECT_LEFT(-7200, dtUtc3.secondsTo(dt2).count());

    DFGTEST_EXPECT_LEFT(946774861.5, dt1990.secondsTo(dt2020).count());
    DFGTEST_EXPECT_LEFT(-1 * dt1990.secondsTo(dt2020).count(), dt2020.secondsTo(dt1990).count());
}

TEST(dfgTime, DateTime_systemTime_local)
{
    using namespace DFG_MODULE_NS(time);

    const auto t = std::time(nullptr);
    const auto dt0 = DateTime::systemTime_local();

    std::tm tm0, tm1, tm2;
    localtime_s(&tm0, &t);
    _putenv_s("TZ", "GST-1GDT");
    _tzset(); // Applies TZ environment variable to globals that e.g. localtime use.
    const auto dt1 = DateTime::systemTime_local();
    localtime_s(&tm1, &t);
    _putenv_s("TZ", "GST-2GDT");
    _tzset();
    localtime_s(&tm2, &t);
    EXPECT_EQ((tm1.tm_hour + 1) % 24, tm2.tm_hour); // Value returned by localtime_s is affected by TZ environment variable.
    const auto dt2 = DateTime::systemTime_local();

    const auto diffInSeconds01 = dt0.secondsTo(dt1);
    const auto diffInSeconds02 = dt1.secondsTo(dt2);

    // Value returned by systemTime_local() should not be dependent on TZ environment variable, tested below using arbitrary resolution of 1 seconds.
    // TODO: verify that tm0 and dt0 are almost equal.
    const auto msTm = DateTime::millisecondsSinceMidnight(tm0.tm_hour, tm0.tm_min, tm0.tm_sec, 0);
    EXPECT_TRUE(msTm <= dt0.millisecondsSinceMidnight()); // May fail if day changes between calls std::time() and DateTime::systemTime_local().
    EXPECT_TRUE(dt0.millisecondsSinceMidnight() < 500 || msTm < dt0.millisecondsSinceMidnight() + 500);
    EXPECT_TRUE(diffInSeconds01 < std::chrono::seconds(1));
    EXPECT_TRUE(diffInSeconds02 < std::chrono::seconds(1));

    EXPECT_TRUE(dt0.utcOffsetInfo().isSet());
    EXPECT_TRUE(dt1.utcOffsetInfo().isSet());
    EXPECT_TRUE(dt2.utcOffsetInfo().isSet());

    EXPECT_EQ(dt0.utcOffsetInfo().offsetInSeconds(), dt1.utcOffsetInfo().offsetInSeconds());
    EXPECT_EQ(dt0.utcOffsetInfo().offsetInSeconds(), dt2.utcOffsetInfo().offsetInSeconds());
}
#endif // WIN32

static constexpr int tzNone = -1;
static constexpr int tzZ = -2;

namespace
{
    static auto DateTime_fromStringImpl(const int year, const int month, const int day,
                                        int hour, int minute, int second, int ms,
                                        int timezoneOffsetInSeconds) -> ::DFG_MODULE_NS(time)::DateTime
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(time);
        auto sDate = format_fmt("{0:04}-{1:02}-{2:02}", year, month, day);
        if (hour >= 0)
            formatAppend_fmt(sDate, " {0:02}:{1:02}:{2:02}", hour, minute, second);
        else
        {
            hour = 0;
            minute = 0;
            second = 0;
        }
        if (ms != -1)
            formatAppend_fmt(sDate, ".{0:03}", ms);
        else
            ms = 0;
        if (timezoneOffsetInSeconds != tzNone)
        {
            if (timezoneOffsetInSeconds == tzZ)
            {
                sDate += "Z";
                timezoneOffsetInSeconds = 0;
            }
            else
            {
                const int positiveTzOffset = std::abs(timezoneOffsetInSeconds);
                if (timezoneOffsetInSeconds >= 0)
                    sDate += "+";
                else
                    sDate += "-";
                const auto tzh = positiveTzOffset / (60 * 60);
                const auto tzm = (positiveTzOffset - (tzh * 60 * 60)) / 60;
                if (positiveTzOffset % 3600 == 1)
                {
                    formatAppend_fmt(sDate, "{0:02}", tzh);
                    timezoneOffsetInSeconds = (positiveTzOffset - 1) * ((timezoneOffsetInSeconds > 0) ? 1 : -1);
                }
                else
                    formatAppend_fmt(sDate, "{0:02}:{1:02}", tzh, tzm);
            }
        }
        const auto rv = DateTime::fromString(sDate);
        DFGTEST_EXPECT_LEFT(year, rv.year());
        DFGTEST_EXPECT_LEFT(month, rv.month());
        DFGTEST_EXPECT_LEFT(day, rv.day());
        DFGTEST_EXPECT_LEFT(hour, rv.hour());
        DFGTEST_EXPECT_LEFT(minute, rv.minute());
        DFGTEST_EXPECT_LEFT(second, rv.second());
        DFGTEST_EXPECT_LEFT(ms, rv.millisecond());
        if (timezoneOffsetInSeconds == tzNone)
            DFGTEST_EXPECT_FALSE(rv.utcOffsetInfo().isSet());
        else
        {
            DFGTEST_EXPECT_TRUE(rv.utcOffsetInfo().isSet());
            DFGTEST_EXPECT_LEFT(timezoneOffsetInSeconds, rv.utcOffsetInfo().offsetInSeconds());
        }
        return rv;
    }
}

TEST(dfgTime, DateTime_fromString)
{
    using namespace DFG_MODULE_NS(time);

    // Basic positive tests
    {
        DateTime_fromStringImpl(2021, 11, 28, -1, 1, 2, -1, tzNone);  // Tests yyyy-mm-dd
        DateTime_fromStringImpl(2021, 11, 28, 12, 1, 2, -1, tzNone);  // Tests yyyy-mm-dd hh:mm:ss
        DateTime_fromStringImpl(2021, 11, 28, 12, 0, 0, -1, tzZ);     // Tests yyyy-mm-dd hh:mm:ssZ
        DateTime_fromStringImpl(2021, 11, 28, 12, 1, 2, 125, tzNone); // Tests yyyy-mm-dd hh:mm:ss.zzz
        DateTime_fromStringImpl(2021, 11, 28, 12, 0, 0, 125, tzZ);    // Tests yyyy-mm-dd hh:mm:ss.zzzZ
        DateTime_fromStringImpl(2021, 11, 28, 12, 0, 0, 125, 0);      // Tests yyyy-mm-dd hh:mm:ss+00:00
        DateTime_fromStringImpl(2021, 11, 28, 12, 1, 2, 0, 3600);     // Tests yyyy-mm-dd hh:mm:ss.zzz+hh:mm
        DateTime_fromStringImpl(2021, 11, 28, 12, 1, 2, -1, -3600);   // Tests yyyy-mm-dd hh:mm:ss-hh:mm
        DateTime_fromStringImpl(2021, 11, 28, 12, 1, 2, -1, -3601);   // Tests yyyy-mm-dd hh:mm:ss-hh
        DateTime_fromStringImpl(2021, 11, 28, 12, 1, 2, 0, 3601);     // Tests yyyy-mm-dd hh:mm:ss.zzz+hh
        DateTime_fromStringImpl(2021, 11, 28, 12, 1, 2, 123, 11700);  // Tests yyyy-mm-dd hh:mm:ss.zzz+03:15 

        DFGTEST_EXPECT_LEFT(125, DateTime::fromString("2021-11-28T12:01:02.125").millisecond()); // Tests that can parse when date and time are separated by 'T'
    }

    // Basic negative tests
    {
        DFGTEST_EXPECT_TRUE(DateTime::fromString("-100-12-02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-13-02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2012-12-32").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021--12-02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12--02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("abcd-12-02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12- 2").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021- 2-02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021.12.02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-ab-02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-ab").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02A12:01:02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01").isNull()); // Allowing this could be ok, but at least for now not accepted.
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:-1:02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:-2").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12.01:02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01.02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 ab:01:02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:ab:02").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:ab").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 24:00:00").isNull()); // h == 24 is not supported
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:60:00").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.abc").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.1").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.12").isNull());

        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02A01:00").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02+-1:00").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02+01:-2").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02+24:00").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.1000Z").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.100A01:00").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.100+aa:00").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.100+1:2").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.100+24:00").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.100+01:99").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.100+01:-1").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.100+01:ab").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.100+ab:01").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.100+-1").isNull());
        DFGTEST_EXPECT_TRUE(DateTime::fromString("2021-12-02 12:01:02.100+24").isNull());
    }
}

TEST(dfgTime, DateTime_fromStdTm)
{
    using namespace ::DFG_MODULE_NS(time);

    // According to https://en.cppreference.com/w/cpp/chrono/c/time
    // "The encoding of calendar time in std::time_t is unspecified, but most systems conform to the POSIX specification "
    // Checking that implementation conforms by using some fixed unix times.
    const std::time_t t20210601_120102 = 1622548862;
    const std::time_t t20211205_120102 = 1638705662;
    const auto tm20210601_120102 = stdGmTime(t20210601_120102);
    const auto tm20211250_120102 = stdGmTime(t20211205_120102);
    DFGTEST_EXPECT_LEFT(121, tm20210601_120102.tm_year);
    DFGTEST_EXPECT_LEFT(5,   tm20210601_120102.tm_mon);
    DFGTEST_EXPECT_LEFT(1,   tm20210601_120102.tm_mday);
    DFGTEST_EXPECT_LEFT(12,  tm20210601_120102.tm_hour);
    DFGTEST_EXPECT_LEFT(1,   tm20210601_120102.tm_min);
    DFGTEST_EXPECT_LEFT(2,   tm20210601_120102.tm_sec);
    DFGTEST_EXPECT_LEFT(2,   tm20210601_120102.tm_wday);
    DFGTEST_EXPECT_LEFT(151, tm20210601_120102.tm_yday);

    DFGTEST_EXPECT_LEFT(121, tm20211250_120102.tm_year);
    DFGTEST_EXPECT_LEFT(11,  tm20211250_120102.tm_mon);
    DFGTEST_EXPECT_LEFT(5,   tm20211250_120102.tm_mday);
    DFGTEST_EXPECT_LEFT(12,  tm20211250_120102.tm_hour);
    DFGTEST_EXPECT_LEFT(1,   tm20211250_120102.tm_min);
    DFGTEST_EXPECT_LEFT(2,   tm20211250_120102.tm_sec);
    DFGTEST_EXPECT_LEFT(0,   tm20211250_120102.tm_wday);
    DFGTEST_EXPECT_LEFT(338, tm20211250_120102.tm_yday);

    const auto dt20211250_120102 = DateTime::fromStdTm(tm20211250_120102);
    const auto dt20211250_120102Z = DateTime::fromStdTm(tm20211250_120102, UtcOffsetInfo(std::chrono::seconds(0)));
    const auto dt20211250_120102Plus1 = DateTime::fromStdTm(tm20211250_120102, UtcOffsetInfo(std::chrono::seconds(3600)));
    const auto dt20211250_120102Minus1 = DateTime::fromStdTm(tm20211250_120102, UtcOffsetInfo(std::chrono::seconds(-3600)));
    DFGTEST_EXPECT_TRUE(dt20211250_120102.isLocalDateTimeEquivalent(tm20211250_120102));
    DFGTEST_EXPECT_TRUE(dt20211250_120102.isLocalDateTimeEquivalent(dt20211250_120102Z));
    DFGTEST_EXPECT_TRUE(dt20211250_120102.isLocalDateTimeEquivalent(dt20211250_120102Plus1));
    DFGTEST_EXPECT_TRUE(dt20211250_120102.isLocalDateTimeEquivalent(dt20211250_120102Minus1));
    DFGTEST_EXPECT_LEFT(3600, dt20211250_120102Z.toSecondsSinceEpoch() - dt20211250_120102Plus1.toSecondsSinceEpoch());
    DFGTEST_EXPECT_LEFT(-3600, dt20211250_120102Z.toSecondsSinceEpoch() - dt20211250_120102Minus1.toSecondsSinceEpoch());
}

TEST(dfgTime, DateTime_fromTime_t)
{
    using namespace ::DFG_MODULE_NS(time);
    const std::time_t t20211205_120102 = 1638705662;

    const auto dt0Z       = DateTime::fromTime_t(t20211205_120102);
    const auto dt0Plus12  = DateTime::fromTime_t(t20211205_120102, std::chrono::seconds(12 * 60 * 60));
    const auto dt0Minus12 = DateTime::fromTime_t(t20211205_120102, std::chrono::seconds(-12 * 3600));

    DFGTEST_EXPECT_LEFT(t20211205_120102, dt0Z.toSecondsSinceEpoch());
    DFGTEST_EXPECT_LEFT(t20211205_120102, dt0Plus12.toSecondsSinceEpoch());
    DFGTEST_EXPECT_LEFT(t20211205_120102, dt0Minus12.toSecondsSinceEpoch());
    DFGTEST_EXPECT_FALSE(dt0Z.isLocalDateTimeEquivalent(dt0Plus12));
    DFGTEST_EXPECT_FALSE(dt0Z.isLocalDateTimeEquivalent(dt0Minus12));
    DFGTEST_EXPECT_FALSE(dt0Plus12.isLocalDateTimeEquivalent(dt0Minus12));
    DFGTEST_EXPECT_TRUE(DateTime(2021, 12, 5, 12, 1, 2, 0).isLocalDateTimeEquivalent(dt0Z));
    DFGTEST_EXPECT_TRUE(DateTime(2021, 12, 6,  0, 1, 2, 0).isLocalDateTimeEquivalent(dt0Plus12));
    DFGTEST_EXPECT_TRUE(DateTime(2021, 12, 5,  0, 1, 2, 0).isLocalDateTimeEquivalent(dt0Minus12));

    const auto dtInvalidOffset = DateTime::fromTime_t(t20211205_120102, std::chrono::seconds(-360000000));
    DFGTEST_EXPECT_TRUE(dtInvalidOffset.isNull());
}

TEST(dfgTime, DateTime_toSecondsSinceEpoch)
{
    using namespace ::DFG_MODULE_NS(time);

    const DateTime time0(2021, 11, 30, 12, 01, 02, 0, UtcOffsetInfo(std::chrono::seconds(0)));
    const DateTime time1(2021, 11, 30, 12, 01, 02, 250, UtcOffsetInfo(std::chrono::seconds(3600)));
    const DateTime time2(2021, 11, 30, 12, 01, 02, 750, UtcOffsetInfo(std::chrono::seconds(-3600)));

    // toSecondsSinceEpoch
    {
        const auto epochTime0 = time0.toSecondsSinceEpoch();
        const auto epochTime1 = time1.toSecondsSinceEpoch();
        const auto epochTime2 = time2.toSecondsSinceEpoch();
        DFGTEST_EXPECT_LEFT(1638273662, epochTime0);
        DFGTEST_EXPECT_LEFT(1638273662 - 3600, epochTime1);
        DFGTEST_EXPECT_LEFT(1638273662 + 3600, epochTime2);
    }

    // toMilllisecondsSinceEpoch
    {
        const auto epochTime0 = time0.toMillisecondsSinceEpoch();
        const auto epochTime1 = time1.toMillisecondsSinceEpoch();
        const auto epochTime2 = time2.toMillisecondsSinceEpoch();
        DFGTEST_EXPECT_LEFT(1638273662000, epochTime0);
        DFGTEST_EXPECT_LEFT(1638273662250 - 3600000, epochTime1);
        DFGTEST_EXPECT_LEFT(1638273662750 + 3600000, epochTime2);
    }
}

#ifdef _WIN32
TEST(dfgTime, DateTime_toSYSTEMTIME)
{
    using namespace ::DFG_MODULE_NS(time);

    // Positive tests
    {
        const DateTime t0Utc(1999, 12, 31, 23, 30, 1, 125, UtcOffsetInfo(std::chrono::seconds(0)));
        const DateTime t0TzPlus(1999, 12, 31, 23, 30, 1, 125, UtcOffsetInfo(std::chrono::seconds(3600)));
        const DateTime t0TzMinus(1999, 12, 31, 23, 30, 1, 125, UtcOffsetInfo(std::chrono::seconds(-3600)));

        const auto t0UtcSt = t0Utc.toSYSTEMTIME();
        const auto t0TzPlusSt = t0TzPlus.toSYSTEMTIME();
        const auto t0TzMinusSt = t0TzMinus.toSYSTEMTIME();

        DFGTEST_EXPECT_LEFT(5, t0UtcSt.wDayOfWeek);
        DFGTEST_EXPECT_LEFT(23, t0TzPlusSt.wHour);
        DFGTEST_EXPECT_LEFT(23, t0TzMinusSt.wHour);

        const DateTime t0UtcRoundTrip(t0UtcSt);

        DFGTEST_EXPECT_LEFT(t0Utc.year(), t0UtcRoundTrip.year());
        DFGTEST_EXPECT_LEFT(t0Utc.month(), t0UtcRoundTrip.month());
        DFGTEST_EXPECT_LEFT(t0Utc.day(), t0UtcRoundTrip.day());
        DFGTEST_EXPECT_LEFT(t0Utc.hour(), t0UtcRoundTrip.hour());
        DFGTEST_EXPECT_LEFT(t0Utc.minute(), t0UtcRoundTrip.minute());
        DFGTEST_EXPECT_LEFT(t0Utc.second(), t0UtcRoundTrip.second());
        DFGTEST_EXPECT_LEFT(t0Utc.millisecond(), t0UtcRoundTrip.millisecond());
        DFGTEST_EXPECT_LEFT(t0Utc.dayOfWeek(), t0UtcRoundTrip.dayOfWeek());
        DFGTEST_EXPECT_LEFT(DayOfWeek::Friday, t0Utc.dayOfWeek());
        // Note: not testing utc offset, it is not preserved in toWin32SystemTime().
    }

    // Negative test
    {
        const auto isZeroSystemTime = [](const SYSTEMTIME& st) { auto p = reinterpret_cast<const char*>(&st); return std::all_of(p, p + sizeof(SYSTEMTIME), [](const char c) { return c == 0; }); };
        DFGTEST_EXPECT_TRUE(isZeroSystemTime(DateTime().toSYSTEMTIME()));
        // Valid SYSTEMTIME years are "1601 through 30827"
        DFGTEST_EXPECT_TRUE(isZeroSystemTime(DateTime(1600, 1, 1, 12, 0, 0, 0).toSYSTEMTIME()));
        DFGTEST_EXPECT_TRUE(isZeroSystemTime(DateTime(30828, 1, 1, 12, 0, 0, 0).toSYSTEMTIME()));
    }
}

#endif // _WIN32

#endif // DFG_LANGFEAT_CHRONO_11

#endif
