/*
 * datetime.h
 *
 * Caution! duplicates Win32 SYSTEMTIME struct.
 *
 * DateTime does NO value checking. calling diff... or operator- might kill you.
 */

#ifndef _DATETIME_H
#define _DATETIME_H

#include <iostream>
#define INCL_DOSDATETIME

#include <windows.h>
#include <stdio.h>

const unsigned short Days[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

class DateTime 
{
public:
 
	DateTime();
	DateTime(DateTime& dt)
	: hour(dt.hour), minute(dt.minute), second(dt.second), millisecond(dt.millisecond),
	  day(dt.day), month(dt.month), year(dt.year), weekday(dt.weekday)
	{}
	~DateTime() {}

	void now() { GetLocalTime((LPSYSTEMTIME)this); }


	void setYear(unsigned int aYear)			{ year = aYear; }
	void setMonth(unsigned int aMonth)			{ month = aMonth; }
	void setDay(unsigned int aDay)				{ day = aDay; }
	void setHour(unsigned int anHour)			{ hour = anHour; }
	void setMinute(unsigned int aMinute)		{ minute = aMinute; }
	void setSecond(unsigned int aSecond)		{ second = aSecond; }
	void setMillisecond(unsigned int msec)		{ millisecond = msec; }

	unsigned int getYear()		{ return year; }
	unsigned int getMonth() 	{ return month; }
	unsigned int getDay()		{ return day; }
	unsigned int getHour()		{ return hour; }
	unsigned int getMinute()	{ return minute; }
	unsigned int getSecond()	{ return second; }
	unsigned int getMillisecond() { return millisecond; }

	unsigned int dayInYear()
	{
		unsigned long delta = Days[month -1] + day;

		if (isLeapYear() && (month > 2))
			return delta + 1;

		return delta;
	}

	// difference in milliseconds
	int operator-(DateTime& b)
	{
		return diffSeconds(b) * 1000 + millisecond - b.millisecond;
	}

	int operator<(DateTime& b) { return (*this - b) < 0; }

	int diffDays(DateTime& b)
	{
		return (year-b.year) * 365
			 + ((year-1) / 4) - ((b.year-1) / 4)
			 - ((year-1) / 100) + ((b.year-1) / 100)
			 + ((year-1) / 400) - ((b.year-1) / 400)
			 - b.dayInYear() + dayInYear();
	}

	int diffHours(DateTime& b)
	{
	    return diffDays(b) * 24 + hour - b.hour;
	}

	int diffMinutes(DateTime& b)
	{
		return diffHours(b) * 60 + minute - b.minute;
	}

	int diffSeconds(DateTime& b)
	{
		return diffMinutes(b) * 60 + second - b.second;
	}

	unsigned int isLeapYear()
	{
		return ( ((year % 4) == 0) && ((year % 100) != 0) 
			|| ((year % 400) == 0) );
	}

	unsigned int isValid() { return (hour != -1); }

	friend std::ostream& operator<<(std::ostream& out, const DateTime& d)
	{ return out << (int)d.day << '.' << (int)d.month << '.' << d.year << ' ' << (int)d.hour << ':' << (int)d.minute << ':' << (int)d.second << '.' << (int)d.millisecond; }

	char* asString()	// must be deleted after use
	{
		char* s = new char[23];

		if (s == 0) return 0;

		sprintf(s, "%02d/%02d/%04d %02d:%02d:%02d.%03d", month, day, year, hour, minute, second, millisecond);

		return s;
	}

private:

	unsigned short year;
	unsigned short month;
	unsigned short weekday;
	unsigned short day;
	unsigned short hour;
	unsigned short minute;
	unsigned short second;
	unsigned short millisecond;
};

#endif
