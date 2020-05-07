
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "stat.h"
#include "timer.h"
#include "misc.h"
#include "DString.h"
#include "stat.fdh"

static FILE *logfp;
static bool useSyslog;
static bool syncLog = true;
static bool useConsole = true;
static char logFilename[1024];
static uint32_t logfileOptions = LOGFILE_OPTION_DATE;
static DString *log_queue;		// for statnocr
static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;
static StatHookFunc statHook;

// strdupa() function isn't available on BSD
#ifdef __GLIBC__
#define HAVE_STRDUPA
#endif

// used by prefixstat. also, let's start exporting it
/*static*/ const char *ega_to_ansi_seq(int egaColor)
{
	const char *bgColorString = "";
	if (egaColor & 0xff00)		// background color
	{
		int bgcolor = (egaColor >> 8) & 7;
		int rbxor = ((bgcolor & 4) >> 2) ^ (bgcolor & 1);
		int bgAnsiColor = ((bgcolor ^ rbxor ^ (rbxor << 2)) | (bgcolor & 2)) + 40;
		bgColorString = stprintf(";%d", bgAnsiColor);
	}
	
	bool bold = (egaColor & 8) != 0;
	
	// ansi is just BGR vs. ega's RGB, so we can just swap the R and B bits
	// instead of using a lookup table
	egaColor &= 7;
	int rbxor = ((egaColor & 4) >> 2) ^ (egaColor & 1);
	int ansiColor = (egaColor ^ rbxor ^ (rbxor << 2)) | (egaColor & 2);
	
	// use new 90's range for bright colors
	if (bold) ansiColor += 90; else ansiColor += 30;
	
	return stprintf("%s;%d%sm", \
		bold ? "\e[1" : "\e[0",
		ansiColor,
		bgColorString);
}

/*
void c------------------------------() {}
*/

const char *GetLogFilename(void)
{
	return logFilename;
}

void SetLogFilename(const char *fname)
{
	SetLogFilename(fname, true);
}

void SetLogFilename(const char *fname, bool append)
{
	maxcpy(logFilename, fname, sizeof(logFilename));
	if (!append) remove(fname);
	
	if (logfp) fclose(logfp);
	logfp = fopen(logFilename, "a+");
	if (!logfp)
	{
		fprintf(stderr, "libkaty: failed to open log file '%s' for writing\n", fname);
		return;
	}
}

void SetLogFileOptions(uint32_t newOptions)
{
	logfileOptions = newOptions;
}

void SetLogFileSync(bool on)
{
	syncLog = on;
}

void StatToSyslog(bool on)
{
	useSyslog = on;
}

void StatToConsole(bool on)
{
	useConsole = on;
}

void SetStatHook(StatHookFunc func)
{
	statHook = func;
}

static void write_logfile(const char *buf)
{
	if (!logFilename[0]) return;
	
	pthread_mutex_lock(&logMutex);
	
	if (!logfp)
	{
		logfp = fopen(logFilename, "a+");
		
		if (!logfp)
		{
			fprintf(stderr, "libkaty: Couldn't open logfile '%s'\n", logFilename);
			pthread_mutex_unlock(&logMutex);
			return;
		}
	}
	
	if (log_queue)	// prepend any previous statnocr's that previously weren't logged
	{
		int buf_len = strlen(buf);
		char *concat_line = (char *)alloca(log_queue->Length() + buf_len + 1);
		memcpy(concat_line, log_queue->String(), log_queue->Length());
		memcpy(&concat_line[log_queue->Length()], buf, buf_len + 1);
		buf = concat_line;
		
		delete log_queue;
		log_queue = NULL;
	}
	
	// if the line contains carriage returns, split it across multiple lines
	// where each line gets the date prefix/etc
	const char *first_cr;
	if ((first_cr = strchr(buf, '\n')))
	{
		#ifdef HAVE_STRDUPA
		char *fullout = strdupa(buf);
		#else
		char *fullout = strdup(buf);
		#endif
		char *next_cr = fullout + (first_cr - buf);
		char *line = fullout;
		
		do
		{
			*next_cr = '\0';
			do_write_logline(line, logfp);
			
			line = next_cr + 1;
			next_cr = strchr(line, '\n');
		}
		while(next_cr != NULL);
		
		do_write_logline(line, logfp);

		#ifndef HAVE_STRDUPA
		free(fullout);
		#endif
	}
	else
	{
		do_write_logline(buf, logfp);
	}
	
	if (syncLog)
	{
		fclose(logfp);
		logfp = NULL;
	}
	
	pthread_mutex_unlock(&logMutex);
}

static void do_write_logline(const char *line, FILE *fp)
{
	if (logfileOptions & LOGFILE_OPTION_PID)
		fprintf(fp, "[%d] ", getpid());
	
	if (logfileOptions & LOGFILE_OPTION_DATE)
	{
		fputs(GetTimestamp(), logfp);
		fputs(" ", fp);
	}
	
	fputs(line, fp);
	fputs("\n", fp);
}


void stat(const char *fmt, ...)
{
va_list ar;
char buf[32768];

	va_start(ar, fmt);
	{
		if (statHook)
		{
			(*statHook)(NULL, -1, 0, fmt, ar);
			va_end(ar);
			return;
		}
		
		vsnprintf(buf, sizeof(buf), fmt, ar);
	}
	va_end(ar);
	
	if (useConsole)
	{
		puts(buf);
		fflush(stdout);
	}
	
	if (logFilename[0])
		write_logfile(buf);
	
	if (useSyslog)
		syslog(LOG_INFO, "%s\n", buf);
}

void prefixstat(const char *prefix, int color, const char *fmt, ...)
{
va_list ar;
char buf[32768];

	va_start(ar, fmt);
	{
		if (statHook)
		{
			(*statHook)(prefix, color, 0, fmt, ar);
			va_end(ar);
			return;
		}
		
		vsnprintf(buf, sizeof(buf), fmt, ar);
	}
	va_end(ar);
	
	if (color >= 0)
	{
		fputs(ega_to_ansi_seq(color), stdout);
		
		fputs(prefix, stdout);
		fputs(": ", stdout);
		
		fputs(buf, stdout);
		puts("\e[0m");
	}
	else
	{
		fputs(prefix, stdout);
		fputs(": ", stdout);
		puts(buf);
	}
	
	fflush(stdout);
	
	if (logFilename[0])
		write_logfile(buf);
	
	if (useSyslog)
		syslog(LOG_INFO, "%s\n", buf);
}

void statnocr(const char *fmt, ...)
{
va_list ar;
char buf[32768];

	va_start(ar, fmt);
	{
		if (statHook)
		{
			(*statHook)(NULL, -1, STAT_NO_CR, fmt, ar);
			va_end(ar);
			return;
		}
		
		vsnprintf(buf, sizeof(buf), fmt, ar);
	}
	va_end(ar);
	
	if (useConsole)
	{
		printf("%s", buf);
		fflush(stdout);
	}
	
	if (!log_queue)
		log_queue = new DString(buf);
	else
		log_queue->AppendString(buf);
	
	if (useSyslog)
		syslog(LOG_INFO, "%s", buf);
}

// for back-compat with old programs that haven't been recompiled yet
// do we still need this? it's been a pretty long time... (as of 02/06/2020)
void _staterr2(const char *prfunc_in, const char *fmt, ...)
{
va_list ar;
char buf[4096];
char *func, *ptr;

	// process the __PRETTY_FUNCTION__
	#ifdef HAVE_STRDUPA
	char *prfunc = strdupa(prfunc_in);		// so we can modify it
	#else
	char *prfunc = strdup(prfunc_in);
	#endif

	// truncate at '('
	ptr = strchr(prfunc, '(');
	if (ptr) *ptr = 0;
	
	func = strrchr(prfunc, ' ');			// skip over return type
	if (func) func++; else func = prfunc;
	
	// if function is main, substitute the program name instead
	#ifdef HAVE_STRDUPA
	if (func[0] == 'm' && func[1] == 'a' && func[2] == 'i' && func[3] == 'n' && func[4] == 0)
		func = strdupa(GetFileSpec(GetCurrentEXEFile()));
	#endif

	va_start(ar, fmt);
	{
		if (statHook)
		{
			(*statHook)(func, -1, STAT_ERROR, fmt, ar);
			va_end(ar);
			return;
		}
		
		vsnprintf(buf, sizeof(buf), fmt, ar);
	}
	va_end(ar);
	
	char line[sizeof(buf) + 1024];
	snprintf(line, sizeof(line), "\e[1;97m%s: \e[1;93m%s\e[0m", func, buf);
	puts(line);
	fflush(stdout);
	
	if (logFilename[0])
	{
		char temp[4096+1024];
		snprintf(temp, sizeof(temp), "%s: %s\n", func, buf);
		
		write_logfile(temp);
	}
	
	if (useSyslog)
		syslog(LOG_ERR, "%s: %s\n", func, buf);

	#ifndef HAVE_STRDUPA
	free(prfunc);
	#endif
}

void _staterr3(const char *prfunc_in, int lineNo, const char *fmt, ...)
{
va_list ar;
char buf[4096];
char *func, *ptr;

	// process the __PRETTY_FUNCTION__
	#ifdef HAVE_STRDUPA
	char *prfunc = strdupa(prfunc_in);		// so we can modify it
	#else
	char *prfunc = strdup(prfunc_in);
	#endif
	
	// truncate at '('
	ptr = strchr(prfunc, '(');
	if (ptr) *ptr = 0;
	
	func = strrchr(prfunc, ' ');			// skip over return type
	if (func) func++; else func = prfunc;
	
	// if function is main, substitute the program name instead
	/*if (func[0] == 'm' && func[1] == 'a' && func[2] == 'i' && func[3] == 'n' && func[4] == 0)
		func = strdupa(GetFileSpec(GetCurrentEXEFile()));*/
	
	va_start(ar, fmt);
	{
		if (statHook)
		{
			(*statHook)(func, -1, STAT_ERROR, fmt, ar);
			va_end(ar);
			return;
		}
		
		vsnprintf(buf, sizeof(buf), fmt, ar);
	}
	va_end(ar);
	
	char line[sizeof(buf) + 1024];
	snprintf(line, sizeof(line), "\e[1;97m%s\e[0m:\e[1;97m%d\e[0m: \e[1;93m%s\e[0m", func, lineNo, buf);
	puts(line);
	fflush(stdout);
	
	if (logFilename[0])
	{
		char temp[4096+1024];
		snprintf(temp, sizeof(temp), "%s: %s\n", func, buf);
		
		write_logfile(temp);
	}
	
	if (useSyslog)
		syslog(LOG_ERR, "%s: %s\n", func, buf);

	#ifndef HAVE_STRDUPA
	free(prfunc);
	#endif
}

void _crash(const char *file, int line, const char *fmt, ...)
{
va_list ar;
char err[2048];

	va_start(ar, fmt);
	vsnprintf(err, sizeof(err), fmt, ar);
	va_end(ar);
	
	stat("crashed at %c[0;1;97m<< %s:%c[93m%d%c[97m >>: %c[9;91;43m%s%c[0m", \
		27, file, 27, line, 27, 27, err, 27);
	if (logfp) { fclose(logfp); logfp = NULL; }
	exit(1);
}

/*void _staterr(const char *fmt, ...)
{
va_list ar;
char buf[2048];

	va_start(ar, fmt);
	vsnprintf(buf, sizeof(buf)-4, fmt, ar);
	va_end(ar);
	
	puts(buf);
	fflush(stdout);
	
	if (logFilename[0])
	{
		if (!logfp)	logfp = fopen(logFilename, "a+");
		if (logfp)
		{
			fprintf(logfp, "%s << %s >>\n", GetTimestamp(), buf);
			if (syncLog) { fclose(logfp); logfp = NULL; }
		}
	}
	
	if (useSyslog)
		syslog(LOG_ERR, "%s\n", buf);
}*/
