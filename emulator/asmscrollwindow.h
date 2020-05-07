
#ifndef _ASMSCROLLWINDOW_H
#define _ASMSCROLLWINDOW_H

class Emulator;

class ASMScrollWindow
{
public:
	ASMScrollWindow(Emulator *emu, int _x1, int _y1, int _x2, int _y2);
	~ASMScrollWindow();
	
	void SetTitle(const char *newTitle);
	
	void SetPC(int newPC);
	
	void Draw();

private:
	bool IsAddressInList(int addr);
	
private:
	int x1, y1, x2, y2;
	int width, height;
	int numItems;
	
	int pcTop;
	int currentPC;
	int BottomPC();
	
	// list of DisassemblyResults which is "height" or less items long.
	// the first item in the list corresponds to pcTop.
	List<DisassemblyResult> dasmLines;
	
	bool needRedrawBorder;
	Emulator *emu;
	ASMCore *asmcore;
};



#endif
