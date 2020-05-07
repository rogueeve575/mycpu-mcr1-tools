
#ifndef _KTCURSES_H
#define _KTCURSES_H

void textcolor(int c);
void textbg(int c);
void update_current_color();

void clrscr();
void cstat(const char *str, ...);
void cstatnocr(const char *str, ...);
void coninhibitupdate(bool enable);
void cdrawwindow(int x1, int y1, int x2, int y2, const char *title = NULL, const int fgcolor = -1, const int bgcolor = -1);

int conwidth();
int conheight();
void gotoxy(int x, int y);
int wherex();
int wherey();

int getkey();
int waitkey();

bool curses_init();
void curses_close();

#undef KEY_F1
#undef KEY_F2
#undef KEY_F3
#undef KEY_F4
#undef KEY_F5
#undef KEY_F6
#undef KEY_F7
#undef KEY_F8
#undef KEY_F9
#undef KEY_F10
#undef KEY_F11
#undef KEY_F12
#undef KEY_LEFTARROW
#undef KEY_RIGHTARROW
#undef KEY_UPARROW
#undef KEY_DOWNARROW
#undef KEY_PGUP
#undef KEY_PGDN

#define KEY_F1		-2
#define KEY_F2		-3
#define KEY_F3		-4
#define KEY_F4		-5
#define KEY_F5		-6
#define KEY_F6		-7
#define KEY_F7		-8
#define KEY_F8		-9
#define KEY_F9		-10
#define KEY_F10		-11
#define KEY_F11		-12
#define KEY_F12		-13

#define KEY_LEFTARROW	-14
#define KEY_RIGHTARROW	-15
#define KEY_UPARROW		-16
#define KEY_DOWNARROW	-17

#define KEY_PGUP		-18
#define KEY_PGDN		-19

#endif
