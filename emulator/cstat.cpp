
#include "main.h"
#include "cstat.fdh"

#define FG_COLOR		7
#define BG_COLOR		0
char *spaceBuffer = NULL;
int spaceBufferLen = 0;

struct LogLine
{
	DString prefix;
	DString line;
};
List<LogLine> logLines;

int height;

bool cstat_init(int _numLines)
{
	// initialize the space padding buffer
	spaceBufferLen = 0;
	spacepad(conwidth() + 1);
	
	height = _numLines;
	
	// draw the window border
	//void cdrawwindow(int x1, int y1, int x2, int y2, const char *title, const int bgcolor)
	int top = conheight() - height - 2;
	cdrawwindow(0, top, conwidth() - 1, conheight() - 1, "Debug", 10, BG_COLOR);
	
	// reroute stat() calls to here
	SetStatHook(cstat_hook);
	
	return 0;
}

void cstat_hook(const char *prefix, int color, uint32_t options, const char *fmt, va_list arg)
{
char buf[4096];

	// handle CRs in the stat
	int maxWidth = conwidth() - 2;
	LogLine *logline = new LogLine();
	
	{
		char *buf2 = buf;
		if (prefix)
		{
			int len = snprintf(buf, sizeof(buf), "%s: ", prefix);
			buf2 += len;
			logline->prefix.SetTo(buf, len);
		}
		
		vsnprintf(buf2, sizeof(buf) - (buf2 - buf), fmt, arg);
	}
	
	int length = strlen(buf);
	if (length > maxWidth) length = maxWidth;
	logline->line.SetTo(buf, length);
	
	if (logline->line.Length() < maxWidth)
		logline->line.AppendString(spacepad(maxWidth - logline->line.Length()));
	
	if (logLines.CountItems() >= height)
		delete logLines.RemoveItem(0);
	
	logLines.AddItem(logline);
	
	update();
	//usleep(200 * 1000);
}

static void update(void)
{
	textcolor(FG_COLOR);
	textbg(BG_COLOR);
	
	coninhibitupdate(true);
	
	int top = conheight() - height - 1;
	for(int i=0;i<logLines.CountItems();i++)
	{
		LogLine *logline = logLines.ItemAt(i);
		
		gotoxy(1, top+i);
		
		if (logline->prefix.Length())
		{
			textcolor(15);
			cstatnocr("%s", logline->prefix.String());
			textcolor(14);
			cstatnocr("%s", logline->line.String() + logline->prefix.Length());
			textcolor(FG_COLOR);
		}
		else
		{
			cstatnocr("%s", logline->line.String());
		}
	}
	
	coninhibitupdate(false);
}

/*enum {	// options for StatHookFunc
	STAT_NO_CR	= 0x01,		// this is from statnocr
	STAT_ERROR	= 0x02,		// this is from staterr
};

typedef void (*StatHookFunc)(const char *prefix, int color, uint32_t options, \
						const char *fmt, va_list arg);
*/

const char *spacepad(int num)
{
	if (num >= spaceBufferLen)
	{
		spaceBufferLen = num;
		
		if (spaceBuffer)
			spaceBuffer = (char *)realloc(spaceBuffer, spaceBufferLen + 1);
		else
			spaceBuffer = (char *)malloc(spaceBufferLen + 1);
		
		memset(spaceBuffer, ' ', spaceBufferLen);
		spaceBuffer[spaceBufferLen] = 0;
	}
	
	return (spaceBuffer + spaceBufferLen) - num;
}
