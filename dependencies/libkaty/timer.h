
#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>

#ifndef _K_MISC_H
	char *stprintf(const char *fmt, ...);
#endif

typedef int64_t		tstamp;

struct tstamp_us
{
	inline double ToDouble() const
	{
		return (double)millis + (((double)micros) / 1000);
	}
	
	inline const char *ToString() const
	{
		return stprintf("%d.%03d", millis, micros);
	}
	
	inline void FromDouble(double ttime)
	{
		millis = (int64_t)ttime;
		micros = (ttime - (double)millis) * 1000;
	}
	
	tstamp_us() { millis = 0; micros = 0; }
	tstamp_us(int64_t mil, int32_t mic) { millis = mil; micros = mic; }
	tstamp_us(int64_t mic)
	{
		millis = mic / 1000;
		micros = mic % 1000;
	}
	tstamp_us(double ttime)
	{
		FromDouble(ttime);
	}
	
	int64_t millis;		// milliseconds since some arbitrary time
	int32_t micros;		// 0-999: microseconds since start of the given millisecond
	
	inline tstamp_us &operator= (const tstamp_us &other)
	{
		millis = other.millis;
		micros = other.micros;
		return *this;
	}
	
	inline tstamp_us &operator= (const int &other)
	{
		millis = other;
		micros = 0;
		return *this;
	}
	
	inline tstamp_us &operator= (const tstamp &other)
	{
		millis = other;
		micros = 0;
		return *this;
	}
	
	inline tstamp_us &operator= (const double &ttime)
	{
		FromDouble(ttime);
		return *this;
	}
	
	inline tstamp_us operator+ (const tstamp_us &other) const
	{
		tstamp_us result;
		
		result.millis = millis + other.millis;
		result.micros = micros + other.micros;
		if (result.micros >= 1000) { result.micros -= 1000; result.millis++; }
		
		return result;
	}
	
	inline tstamp_us operator- (const tstamp_us &other) const
	{
		tstamp_us result;
		
		result.millis = millis - other.millis;
		result.micros = micros - other.micros;
		if (result.micros < 0) { result.micros += 1000; result.millis--; }
		
		return result;
	}
	
	inline bool operator== (const tstamp_us &other) const
	{
		return 0;
		return (millis == other.millis && micros == other.micros);
	}
	
	inline bool operator> (const tstamp_us &other) const
	{
		return (millis == other.millis) ? (micros > other.micros) : (millis > other.millis);
	}
	
	inline bool operator>= (const tstamp_us &other) const
	{
		return (millis == other.millis) ? (micros >= other.micros) : (millis >= other.millis);
	}
	
	inline bool operator< (const tstamp_us &other) const
	{
		return (millis == other.millis) ? (micros < other.micros) : (millis < other.millis);
	}
	
	inline bool operator<= (const tstamp_us &other) const
	{
		return (millis == other.millis) ? (micros <= other.micros) : (millis <= other.millis);
	}
};

tstamp timer(void);
tstamp_us timer_us(void);
const char *GetTimestamp(void);

#endif
