
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <ncurses.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <memory.h>
#include <fcntl.h>

#include "curses.h"
#include "curses.fdh"

static uint8_t color_matrix[16][16];
static int foreground_color = 7;
static int background_color = 0;
static int update_inhibit_count = 0;
static int nextpair = 1;

static int active_fg = -1, active_bg = -1;

static const char ega_colors[] = {
	COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
	COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
};

void update_current_color()
{
	if (foreground_color == active_fg && \
		background_color == active_bg)
	{
		return;
	}
	
	uint8_t pair = color_matrix[foreground_color & 7][background_color];
	if (pair == 0xff)
	{
		pair = nextpair++;
		init_pair(pair, \
				ega_colors[foreground_color & 7], \
				ega_colors[background_color]);
		
		color_matrix[foreground_color & 7][background_color] = pair;
	}
	
	attrset(COLOR_PAIR(pair) | ((foreground_color & 8) ? A_BOLD : 0));
	active_fg = foreground_color;
	active_bg = background_color;
}

void textcolor(int c)
{
	foreground_color = (c & 15);
}

void textbg(int c)
{
	background_color = (c & 7);
}

/*
void c------------------------------() {}
*/

void clrscr()
{
	update_current_color();
	gotoxy(0, 0);
	
	erase();
	
	if (!update_inhibit_count)
		refresh();
}

/*
void c------------------------------() {}
*/

void cstat(const char *fmt, ...)
{
va_list ar;
char buf[32768];

	va_start(ar, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ar);
	va_end(ar);
	
	coninhibitupdate(true);
	cstatnocr("%s\n", buf);
	coninhibitupdate(false);
}

void cstatnocr(const char *str, ...)
{
va_list ar;
char buf[32768];

	va_start(ar, str);
	vsnprintf(buf, sizeof(buf), str, ar);
	va_end(ar);
	
	update_current_color();
	printw("%s", buf);
	
	if (!update_inhibit_count)
		refresh();
}

void coninhibitupdate(bool enable)
{
	if (enable)
	{
		update_inhibit_count++;
	}
	else
	{
		if (update_inhibit_count > 0)
		{
			update_inhibit_count--;
			
			if (!update_inhibit_count)
				refresh();
		}
	}
}

void cdrawwindow(int x1, int y1, int x2, int y2, const char *title, int fgcolor, int bgcolor)
{
	int x, y;
	
	textcolor((fgcolor > 0) ? fgcolor : 0);
	textbg((bgcolor > 0) ? bgcolor : 0);
	update_current_color();
	
	// draw top border
	gotoxy(x1, y1); addch(ACS_ULCORNER);
	for(x=x1+1;x<=x2-1;x++) addch(ACS_HLINE);
	addch(ACS_URCORNER);
	
	// draw bottom border
	gotoxy(x1, y2); addch(ACS_LLCORNER);
	for(x=x1+1;x<=x2-1;x++) addch(ACS_HLINE);
	addch(ACS_LRCORNER);
	
	// draw sides
	for(int i=0;i<2;i++)
	{
		x = i ? x2 : x1;
		
		for(y=y1+1;y<=y2-1;y++)
		{
			gotoxy(x, y);
			addch(ACS_VLINE);
		}
	}
	
	if (title && title[0])
	{
		int maxTitleWidth = ((x2 - x1) + 1) - 4;
		
		char *limitedTitle = strdupa(title);
		int len = strlen(limitedTitle);
		if (len > maxTitleWidth)
			limitedTitle[maxTitleWidth] = 0;
		
		gotoxy(x1+2, y1);
		textcolor(15); update_current_color();
		printw(" %s ", limitedTitle);
	}
	
	// clear the interior of the window
	if (bgcolor >= 0)
	{
		int width = ((x2 - x1) + 1) - 2;
		char spaces[width];
		memset(spaces, ' ', sizeof(spaces));
		spaces[width] = 0;
		
		for(y=y1+1;y<=y2-1;y++)
		{
			gotoxy(x1+1, y);
			cstatnocr(spaces);
		}
	}
}

/*
void c------------------------------() {}
*/

int conwidth()
{
	return COLS;
}

int conheight()
{
	return LINES;
}

void gotoxy(int x, int y)
{
	move(y, x);
}

int wherex()
{
int x, y;
	getyx(stdscr, y, x);
	return x;
}

int wherey()
{
int x, y;
	getyx(stdscr, y, x);
	return y;
}

/*
void c------------------------------() {}
*/
	
int getkey()
{
	int ch = getch();
	if (ch < 0)
		return ch;
	
	if (ch == 27)
	{
		char str[8192];
		int len = 0;
		
		while(len < sizeof(str))
		{
			ch = getch();
			if (ch < 0 || (len == 0 && ch == 27))
			{
				if (len == 0) return 27;	// simple ESC or back-to-back ESC
				return -1;					// unknown escape code (eat it)
			}
			
			str[len++] = ch;
			int t = translate_seq(str, len);
			if (t) return t;
		}
		
		return -1;
	}
	
	return ch;
}

// tries to translate an escape sequence into a keycode.
// returns 0 if the sequence is unknown.
static int translate_seq(const char *seq, int len)
{
	static const int translations[][80] =
	{
		{ KEY_UPARROW, 2, 91, 65 },
		{ KEY_DOWNARROW, 2, 91, 66 },
		{ KEY_LEFTARROW, 2, 91, 68 },
		{ KEY_RIGHTARROW, 2, 91, 67 },
		
		{ KEY_PGUP, 3, 91, 53, 126 },
		{ KEY_PGDN, 3, 91, 54, 126 },
		
		{ KEY_F1, 3, 79, 80, 0 },
		{ KEY_F2, 3, 79, 81, 0 },
		{ KEY_F3, 3, 79, 82, 0 },
		{ KEY_F4, 3, 79, 83, 0 },
		{ KEY_F5, 5, 91, 49, 53, 126, 0 },
		{ KEY_F6, 5, 91, 49, 55, 126, 0 },
		{ KEY_F7, 5, 91, 49, 56, 126, 0 },
		{ KEY_F8, 5, 91, 49, 57, 126, 0 },
		{ KEY_F9, 5, 91, 50, 48, 126, 0 },
		{ KEY_F10, 5, 91, 50, 49, 126, 0 },
		{ KEY_F11, 5, 91, 50, 51, 126, 0 },
		{ KEY_F12, 5, 91, 50, 52, 126, 0 },
		// Text-terminal version of F... keys
		{ KEY_F1, 3, 91, 91, 65 },
		{ KEY_F2, 3, 91, 91, 66 },
		{ KEY_F3, 3, 91, 91, 67 },
		{ KEY_F4, 3, 91, 91, 68 },
		{ KEY_F5, 3, 91, 91, 69 },
		{ KEY_F6, 3, 91, 91, 70 },
		{ KEY_F7, 3, 91, 91, 71 },
		{ KEY_F8, 3, 91, 91, 72 },
		{ KEY_F9, 3, 91, 91, 73 },
		{ KEY_F10, 3, 91, 91, 74 },
		{ KEY_F11, 3, 91, 91, 75 },
		{ KEY_F12, 3, 91, 91, 76 },
		
		0
	};
	
	for(int i=0;;i++)
	{
		if (translations[i][0] == 0)
			return 0;
		
		if (len == translations[i][1])
		{
			bool matched = true;
			for(int j=0;j<len;j++)
			{
				if (seq[j] != translations[i][j+2])
				{
					matched = false;
					break;
				}
			}
			
			if (matched)
				return translations[i][0];
		}
	}
}

int waitkey()
{
	for(;;)
	{
		int key = getkey();
		if (key != -1) return key;
		
		usleep(10 * 1000);
	}
}

/*
void c------------------------------() {}
*/

bool curses_init()
{
	initscr();
	start_color();
	nonl();
	cbreak();
	noecho();
	timeout(0);
	curs_set(0);
	scrollok(stdscr, 0);
	nodelay(stdscr, 1);
	clrscr();
	
	memset(color_matrix, 0xff, sizeof(color_matrix));
	textcolor(7); textbg(0);
	return 0;
}

void curses_close()
{
	endwin();
}
