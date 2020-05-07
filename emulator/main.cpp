
#include "main.h"
#include "main.fdh"

bool app_quitting;

int main(int argc, char **argv)
{
	stat("Starting emulator");
	//usleep(100 * 1000);	// weird delay needed when terminator is started maximized
	
	/*uint8_t value, sreg;
	alu_compute(5, 7, 0, &value, &sreg);
	stat("value=%d", value);
	stat("sreg=%s", describe_sreg(sreg));*/
	
	Emulator *emu = new Emulator();
	if (emu->init())
		return 1;
	
	// start curses and switch us over to cstat printing so that stat and staterr work with curses
	// this activates TUI graphics and initializes the "Debug" window
	stat("Starting curses...");
	if (curses_init()) return 1;
	if (cstat_init(DEBUG_NUM_LINES)) return 1;
	
	if (emu->initgui())
	{
		stat("GUI init failure. Press any key to exit.");
		waitkey();
		return 1;
	}
	
	while(!app_quitting)
	{
		emu->draw();
		emu->handlekey(waitkey());
	}
	
	/*clrscr();
	cdrawwindow(1, 1, conwidth() - 5, 15, "Titlebar", 4);
	
	textcolor(14);
	textbg(1);
	
	gotoxy(4, 3);
	cstatnocr("Hello! I am curses, alive and well in the new age.");
	
	waitkey();*/
	
	curses_close();
	//for(int i=0;i<5;i++)
		//emu->step();
	
	return 1;
}

/*
void c------------------------------() {}
*/

