#pragma once

#ifndef X_DATE_H_
#define X_DATE_H_

X_NAMESPACE_BEGIN(core)

class DateStamp
{
public:
    typedef char Description[11];

public:
    DateStamp();
    DateStamp(uint16_t year, uint8_t month, uint8_t day);

    bool operator>(const DateStamp& rhs) const;
    bool operator<(const DateStamp& rhs) const;

    int getYear(void) const;
    int getQuarter(void) const;
    int getMonth(void) const;
    int getDay(void) const;
    int getDayOfYear(void) const;

    int isLeapYear(void) const;

    int daysInMonth(int month) const;

    static DateStamp getSystemDate(void);

    const char* toString(Description& desc) const;

private:
    uint16_t year_;
    uint8_t month_;
    uint8_t day_;
};

X_NAMESPACE_END

#endif // X_DATE_H_
