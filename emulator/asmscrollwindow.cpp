
#include "main.h"
#include "asmscrollwindow.h"
#include "asmscrollwindow.fdh"

#define FG_COLOR		7
#define BG_COLOR		1

#define SELECTED_FG		15
#define SELECTED_BG		4

#define SEL_SCROLL_MARGIN	4

ASMScrollWindow::ASMScrollWindow(Emulator *emu, int _x1, int _y1, int _x2, int _y2)
{
	// give room for border
	x1 = _x1+1;
	y1 = _y1+1;
	x2 = _x2-1;
	y2 = _y2-1;
	
	width = (x2 - x1) + 1;
	height = (y2 - y1) + 1;
	numItems = height;
	
	pcTop = 0;
	currentPC = 0;
	needRedrawBorder = true;
	
	this->emu = emu;
	this->asmcore = emu->asmcore;
}

ASMScrollWindow::~ASMScrollWindow()
{
	dasmLines.Clear();
}

/*
void c------------------------------() {}
*/

void ASMScrollWindow::SetPC(int newPC)
{
	currentPC = newPC;
	if (!IsAddressInList(newPC))
		pcTop = newPC;
	
	dasmLines.Clear();
	int address = pcTop;
	for(int i=0;i<numItems;i++)
	{
		auto *dasm = asmcore->Disassemble(&emu->memory[address], DASMFlags::IgnoreErrors);
		dasm->address = address;
		dasmLines.AddItem(dasm);
		
		address = (address + dasm->length) & 0xFFFF;
	}
}

void ASMScrollWindow::Draw()
{
	coninhibitupdate(true);
	
	if (needRedrawBorder)
	{
		needRedrawBorder = false;
		cdrawwindow(x1-1, y1-1, x2+1, y2+1, "Assembly", 11, BG_COLOR);
	}

	char lineBuffer[width + 1];
	int maxLineLength = (width - 3);
	
	int address = pcTop;
	for(int i=0;i<numItems;i++)
	{
		gotoxy(x1+1, y1+i);
		
		if (address == currentPC)
		{
			textcolor(SELECTED_FG);
			textbg(SELECTED_BG);
			cstatnocr("* ");
		}
		else
		{
			textcolor(FG_COLOR);
			textbg(BG_COLOR);
			cstatnocr("  ");
		}
		
		// fetch the line that goes in this position and limit it's length
		DString dasmText;
		DisassemblyResult *dasm = dasmLines.ItemAt(i);
		asmcore->FormatDisassemblyResult(dasm, &dasmText, DASMFlags::IncludeHex | DASMFlags::IncludeAddress);
		
		const char *line = dasmText.String();
		int lineLength = dasmText.Length();
		
		if (lineLength > maxLineLength)
		{
			memcpy(lineBuffer, line, maxLineLength);
			lineBuffer[maxLineLength] = 0;
			line = lineBuffer;
			lineLength = maxLineLength;
		}
		
		// print the line
		gotoxy(x1+1+2, y1+i);
		cstatnocr("%s", line);
		
		// pad rest of line with spaces
		if (lineLength < maxLineLength)
			cstatnocr("%s", spacepad(maxLineLength - lineLength));
		
		address = (address + dasm->length) & 0xffff;
	}
	
	coninhibitupdate(false);
}

/*
void c------------------------------() {}
*/

bool ASMScrollWindow::IsAddressInList(int address)
{
	int numItems = dasmLines.CountItems();
	if (numItems == 0)
		return false;
	
	// check if address is before first instruction or after last instruction
	if (address < dasmLines.ItemAt(0)->address || \
		address > dasmLines.ItemAt(numItems - 1)->address)
	{
		return false;
	}
	
	// check each line in case the address is in-between a multi-byte instruction
	for(int i=0;i<numItems;i++)
	{
		DisassemblyResult *ins = dasmLines.ItemAt(i);
		if (ins->address == address)
			return true;
	}
	
	return false;
}

