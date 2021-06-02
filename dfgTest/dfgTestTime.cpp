#include "stdafx.h"

#if (DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/time.hpp>
#include <dfg/time/DateTime.hpp>
#include <ctime>

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

TEST(dfgTime, DataTime_systemTime_local)
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
#endif

#endif // DFG_LANGFEAT_CHRONO_11

#endif
