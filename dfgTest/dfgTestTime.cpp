#include "stdafx.h"

#if (DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/time.hpp>
#include <dfg/time/DateTime.hpp>
#include <ctime>
#include <dfg/str/format_fmt.hpp>

#if DFG_LANGFEAT_CHRONO_11

#include <chrono>

#ifdef _WIN32
TEST(dfgTime, DateTime)
{
    using namespace DFG_MODULE_NS(time);
    DFG_CLASS_NAME(DateTime) dt0(2016, 7, 29, 12, 13, 14, 15);
    DFG_CLASS_NAME(DateTime) dt1(2016, 7, 29, 12, 13, 15, 15);
    const DFG_CLASS_NAME(DateTime) dtUtc0(2016, 7, 29, 12, 13, 14, 15, DFG_CLASS_NAME(UtcOffsetInfo)(std::chrono::seconds(-3600)));
    const DFG_CLASS_NAME(DateTime) dtUtc1(2016, 7, 29, 12, 13, 14, 15, DFG_CLASS_NAME(UtcOffsetInfo)(std::chrono::seconds(7200)));
    const DFG_CLASS_NAME(DateTime) dtUtc2(2016, 7, 30, 23,  2,  3,  4, DFG_CLASS_NAME(UtcOffsetInfo)(std::chrono::seconds(0)));
    const DFG_CLASS_NAME(DateTime) dt2(2016, 7, 30, 23, 2, 3, 4);
    const DFG_CLASS_NAME(DateTime) dtUtc3(2016, 7, 31,  1,  2,  3,  4, DFG_CLASS_NAME(UtcOffsetInfo)(std::chrono::seconds(7200)));
    EXPECT_EQ(dt0.year(), 2016);
    EXPECT_EQ(dt0.month(), 7);
    EXPECT_EQ(dt0.day(), 29);
    EXPECT_EQ(dt0.hour(), 12);
    EXPECT_EQ(dt0.minute(), 13);
    EXPECT_EQ(dt0.second(), 14);
    EXPECT_EQ(dt0.millisecond(), 15);
    EXPECT_EQ(std::chrono::seconds(1), dt0.secondsTo(dt1));
    EXPECT_EQ(std::chrono::seconds(-1), dt1.secondsTo(dt0));
    EXPECT_EQ(std::chrono::seconds(-10800), dtUtc0.secondsTo(dtUtc1));
    EXPECT_EQ(std::chrono::seconds(10800), dtUtc1.secondsTo(dtUtc0));
    EXPECT_EQ(0, dtUtc2.secondsTo(dtUtc3).count());
    EXPECT_EQ(7200, dt2.secondsTo(dtUtc3).count());
    EXPECT_EQ(-7200, dtUtc3.secondsTo(dt2).count());
}

TEST(dfgTime, DateTime_systemTime_local)
{
    using namespace DFG_MODULE_NS(time);

    const auto t = std::time(nullptr);
    const auto dt0 = DFG_CLASS_NAME(DateTime)::systemTime_local();

    std::tm tm0, tm1, tm2;
    localtime_s(&tm0, &t);
    _putenv_s("TZ", "GST-1GDT");
    _tzset(); // Applies TZ environment variable to globals that e.g. localtime use.
    const auto dt1 = DFG_CLASS_NAME(DateTime)::systemTime_local();
    localtime_s(&tm1, &t);
    _putenv_s("TZ", "GST-2GDT");
    _tzset();
    localtime_s(&tm2, &t);
    EXPECT_EQ((tm1.tm_hour + 1) % 24, tm2.tm_hour); // Value returned by localtime_s is affected by TZ environment variable.
    const auto dt2 = DFG_CLASS_NAME(DateTime)::systemTime_local();

    const auto diffInSeconds01 = dt0.secondsTo(dt1);
    const auto diffInSeconds02 = dt1.secondsTo(dt2);

    // Value returned by systemTime_local() should not be dependent on TZ environment variable, tested below using arbitrary resolution of 1 seconds.
    // TODO: verify that tm0 and dt0 are almost equal.
    const auto msTm = DFG_CLASS_NAME(DateTime)::millisecondsSinceMidnight(tm0.tm_hour, tm0.tm_min, tm0.tm_sec, 0);
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
            DFGTEST_EXPECT_LEFT(timezoneOffsetInSeconds, rv.utcOffsetInfo().offsetInSeconds());
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
}

#endif // DFG_LANGFEAT_CHRONO_11

#endif
