#pragma once

#ifndef X_TIMESTAMP_H_
#define X_TIMESTAMP_H_

X_NAMESPACE_BEGIN(core)

/// \class TimeStamp
/// \brief A class that provides time manipulation functions.
class TimeStamp
{
public:
    /// A type representing the time format: extended ISO (ISO 8601) hh:mm:ss,mmmm plus a null terminator.
    typedef char Description[14];
    /// A type representing the time format suitable to be used as a filename: hh-mm-ss plus a null terminator.
    typedef char FileDescription[9];

    /// Constructs a time.
    TimeStamp(int hour, int minute, int second, int millisecond = 0);

public:
    int GetHour(void) const;
    int GetMinute(void) const;
    int GetSecond(void) const;
    int GetMilliSecond(void) const;

    // Total Milliseconds past midnight :D
    int GetMilliSecondsPastMidnight(void) const;

    const char* ToString(Description& desc) const;
    const char* ToString(FileDescription& desc) const;

    static TimeStamp GetSystemTime(void);

private:
    void InternalToMSPM(void) const;
    void InternalToTime(void) const;

    mutable int Time_;
};

X_NAMESPACE_END

#endif // X_TIMESTAMP_H_
