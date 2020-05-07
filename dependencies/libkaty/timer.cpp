
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "timer.h"
#include "misc.h"
#include "DString.h"
#include "timer.fdh"

// returns the current tick count in milliseconds
tstamp timer(void)
{
struct timespec ts;
uint64_t result;

	#ifdef CLOCK_MONOTONIC_RAW
		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);	// doesn't change frequency due to ntpd adjustments
	#else
		clock_gettime(CLOCK_MONOTONIC, &ts);
	#endif
	
	result = ts.tv_sec;
	result *= 1000;
	result += (ts.tv_nsec / 1000000);
	return (tstamp)result;
}

// more accurate timer that returns time down to microseconds
tstamp_us timer_us(void)
{
struct timespec ts;
tstamp_us result;

	#ifdef CLOCK_MONOTONIC_RAW
		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);	// doesn't change frequency due to ntpd adjustments
	#else
		clock_gettime(CLOCK_MONOTONIC, &ts);
	#endif
	
	result.millis = (ts.tv_sec * 1000);
	result.millis += (ts.tv_nsec / 1000000);
	
	result.micros = (ts.tv_nsec / 1000) % 1000;
	
	return result;
}

/*
void c------------------------------() {}
*/

const char *GetTimestamp(void)
{
	int bufferSize;
	char *buffer = GetStaticStr(&bufferSize);
	
	time_t ts = time(NULL);
	struct tm *tm = localtime(&ts);
	strftime(buffer, bufferSize, "%m/%d/%Y %I:%M:%S%p", tm);
	
	return buffer;
}

const char *GetHumanTimestamp(void)
{
char buffer[64];

	time_t ts = time(NULL);
	struct tm *tm = localtime(&ts);
	
	// get nice day-of-month with suffix
	int dayOfMonth = tm->tm_mday;
	
	DString niceDay(stprintf("%d", dayOfMonth));
	if (((dayOfMonth / 10) % 10) == 1)	// 2nd digit being a 1 means english goes off the rails with 11th, 12th, 112th...
	{
		niceDay.AppendString("th");
	}
	else switch(dayOfMonth % 10)
	{
		case 1: niceDay.AppendString("st"); break;
		case 2: niceDay.AppendString("nd"); break;
		case 3: niceDay.AppendString("rd"); break;
		default: niceDay.AppendString("th"); break;
	}
	
	// generate main string
	strftime(buffer, sizeof(buffer), "%Y %a %B ", tm);
	DString str(buffer);
	str.AppendString(niceDay);
	str.AppendChar(' ');
	
	strftime(buffer, sizeof(buffer), "%I", tm);
	str.AppendString((buffer[0] == '0') ? buffer + 1 : buffer);
	
	strftime(buffer, sizeof(buffer), ":%M %p", tm);
	str.AppendString(buffer);
	
	return str.StaticString();
}
