#pragma once

#ifndef _X_DATE_TIME_STAMP_COMP_H_
#define _X_DATE_TIME_STAMP_COMP_H_

X_NAMESPACE_BEGIN(core)


// a compressed date stamp.
// can store 127 year span.
// bits: | year(7) | month(4) | day(5) | = 16
struct DateStampSmall
{
	DateStampSmall() : date_(0) {}

	DateStampSmall(int year, int month, int day) {
		setDate(year, month, day);
	}

	void setDate(int year, int month, int day){
		date_ = ((year << 9) & 0x7F) | ((month << 5) & 0xF) << (day & 0x1F);
	}

	int year(void) const { return (date_ >> 9) + 1980; }
	int month(void) const { return (date_ >> 5) & 0xF; }
	int day(void) const { return date_ & 0x1F; }

	static DateStampSmall systemDate(void);

protected:
	uint16_t date_;
};


// a compressed time stamp.
// can store time time with 2 second percision
// hour:min:second
// 24:60:32
// bits: | hour(5) |  min(6) | sec(5) | = 16
struct TimeStampSmall
{
	TimeStampSmall() : time_(0) {}

	TimeStampSmall(int hour, int min, int sec) {
		setTime(hour, min, sec);
	}

	void setTime(int hour, int min, int sec) {
		time_ = (hour & 0x1F << 11) | (min & 0x3F << 5) | ((sec >> 1) & 0x1F);
	}

	int hour(void) const { return (time_ >> 11); }
	int min(void) const { return (time_ >> 5) & 0x3f; }
	int sec(void) const { return (time_ << 1) & 0x1f; }

	static TimeStampSmall systemTime(void);

protected:
	uint16_t time_;
};


// Compressed Time & Date stamp.
struct DateTimeStampSmall :
	public TimeStampSmall,
	public DateStampSmall
{
	DateTimeStampSmall() {}

	DateTimeStampSmall(int year, int month, int day,
		int hour, int min, int second) {
		setDate(year, month, day);
		setTime(hour, min, second);
	}

	static DateTimeStampSmall systemDateTime(void);


};

X_ENSURE_SIZE(TimeStampSmall, 2);
X_ENSURE_SIZE(DateStampSmall, 2);
X_ENSURE_SIZE(DateTimeStampSmall, 4);


X_NAMESPACE_END

#endif // !_X_DATE_TIME_STAMP_COMP_H_
