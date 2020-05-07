
#ifndef _STAT_H
#define _STAT_H

#include <stdarg.h>

void SetLogFilename(const char *fname);
void SetLogFilename(const char *fname, bool append);
const char *GetLogFilename(void);

enum {	// options for SetLogFileOptions
	LOGFILE_OPTION_DATE = 0x01,		// include timestamps on each line
	LOGFILE_OPTION_PID = 0x02,		// include pid of current program on each line
	LOGFILE_OPTION_ALL = 0xff
};
void SetLogFileOptions(uint32_t newOptions);

void stat(const char *fmt, ...);
void prefixstat(const char *prefix, int color, const char *fmt, ...);

void _staterr2(const char *prfunc_in, const char *str, ...);
void _staterr3(const char *prfunc_in, int lineNo, const char *str, ...);

#ifndef STATERR_LINE_NUMBERS
	#define staterr(...)	_staterr2(__PRETTY_FUNCTION__, __VA_ARGS__)
#else
	#define staterr(...)	_staterr3(__PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#endif

void statnocr(const char *str, ...);

void _crash(const char *file, int line, const char *fmt, ...);
#define crash(...)	_crash(__FILE__, __LINE__, __VA_ARGS__)

void StatToSyslog(bool on);
void SetLogFileSync(bool on);
void StatToConsole(bool on);

enum {	// options for StatHookFunc
	STAT_NO_CR	= 0x01,		// this is from statnocr
	STAT_ERROR	= 0x02,		// this is from staterr
};

typedef void (*StatHookFunc)(const char *prefix, int color, uint32_t options, \
						const char *fmt, va_list arg);
void SetStatHook(StatHookFunc func);

const char *ega_to_ansi_seq(int egaColor);

#endif
