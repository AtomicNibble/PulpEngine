#include "stdafx.h"

#include <Time\TimeStamp.h>
#include <Time\DateStamp.h>
#include <Time\DateTimeStamp.h>

X_USING_NAMESPACE;


TEST(DateTime, DateCompare)
{
    core::DateStamp date0(2001, 4, 4);
    core::DateStamp date1(2001, 2, 24);
    
    EXPECT_TRUE(date1 < date0);
    EXPECT_TRUE(date0 > date1);
}

TEST(DateTime, TimeCompare)
{
    core::TimeStamp time0(6, 42, 4, 572);
    core::TimeStamp time1(6, 32, 2, 100);

    EXPECT_TRUE(time1 < time0);

    EXPECT_EQ(42, time0.getMinute());
    EXPECT_TRUE(time1 < time0);

    EXPECT_EQ(32, time1.getMinute());
    EXPECT_TRUE(time1 < time0);

    // ---
    EXPECT_TRUE(time0 > time1);
}



TEST(DateTime, TimeValues)
{
    core::TimeStamp time0(6, 42, 4, 572);

    EXPECT_EQ(6, time0.getHour());
    EXPECT_EQ(42, time0.getMinute());
    EXPECT_EQ(4, time0.getSecond());
    EXPECT_EQ(572, time0.getMilliSecond());

    auto totalSeconds = (4 + (60 * 42) + (60 * 60 * 6));
    auto expectedMS = totalSeconds * 1000 + 572;

    EXPECT_EQ(expectedMS, time0.getMilliSecondsPastMidnight());
}

TEST(DateTime, DateTimeCompare)
{

    core::DateTimeStamp stamp0(2018, 5, 12, 14, 23, 25, 999);
    core::DateTimeStamp stamp1(2018, 5, 12, 14, 23, 28, 999);
    core::DateTimeStamp stamp2(2018, 5, 12, 14, 23, 29, 0);
    core::DateTimeStamp stamp3(2018, 5, 12, 14, 23, 30, 0);

    EXPECT_TRUE(stamp0 < stamp1);
    EXPECT_TRUE(stamp0 < stamp2);
    EXPECT_TRUE(stamp0 < stamp3);

    EXPECT_TRUE(stamp1 < stamp2);
    EXPECT_TRUE(stamp1 < stamp3);

    EXPECT_TRUE(stamp2 < stamp3);

    // -------------------

    EXPECT_TRUE(stamp3 > stamp2);
    EXPECT_TRUE(stamp3 > stamp1);
    EXPECT_TRUE(stamp3 > stamp0);

    EXPECT_TRUE(stamp2 > stamp1);
    EXPECT_TRUE(stamp2 > stamp0);

    EXPECT_TRUE(stamp1 > stamp0);
}
