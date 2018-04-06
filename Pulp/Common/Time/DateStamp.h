#pragma once

#ifndef X_DATE_H_
#define X_DATE_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Util
/// \class Date
/// \brief A class that provides date manipulation functions.
/// \sa Time
class DateStamp
{
public:
    /// A type representing the date format: ISO (ISO 8601) YYYY-MM-DD plus a null terminator.
    typedef char Description[11];

    DateStamp(unsigned short year, unsigned char month, unsigned char day);

    int GetYear() const;
    int GetQuarter() const;
    int GetMonth() const;
    int GetDay() const;
    int GetDayOfYear() const;

    int IsLeapYear() const;

    int DaysInMonth(int month) const;

    /// Returns the current date system data.
    static DateStamp GetSystemDate(void);

    const char* ToString(Description& desc) const;

private:
    unsigned short year_;
    unsigned char month_;
    unsigned char day_;
};

X_NAMESPACE_END

#endif // X_DATE_H_
