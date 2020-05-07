
#include "main.h"
#include "scrollwindow.h"
#include "scrollwindow.fdh"

#define FG_COLOR		7
#define BG_COLOR		1

#define SELECTED_FG		7
#define SELECTED_BG		4

#define ACTIVE_FG		15
#define ACTIVE_BG		1

#define SEL_SCROLL_MARGIN	5

ScrollWindow::ScrollWindow(int _x1, int _y1, int _x2, int _y2)
{
	// give room for border
	x1 = _x1+1;
	y1 = _y1+1;
	x2 = _x2-1;
	y2 = _y2-1;
	
	width = (x2 - x1) + 1;
	height = (y2 - y1) + 1;
	
	numItems = 5;
	scrollTop = 0;
	selectedIndex = 1;
	activeIndex = 3;
	isForeground = true;
	needRedrawBorder = true;
	enableManualSelMove = true;
}

ScrollWindow::~ScrollWindow()
{
	
}

void ScrollWindow::SetNumItems(int num)
{
	numItems = num;
	if (selectedIndex > numItems)
		selectedIndex = numItems - 1;
	
	if (scrollBottom() >= numItems)
	{
		scrollTop = numItems - (height - 1);
		if (scrollTop < 0) scrollTop = 0;
	}
	
	staterr("numItems = %d selectedIndex = %d scrollTop = %d", numItems, selectedIndex, scrollTop);
}

void ScrollWindow::SetTitle(const char *newTitle)
{
	title.SetTo(newTitle);
	needRedrawBorder = true;
}

void ScrollWindow::SetSelectionIndex(int index)
{
	selectedIndex = index;
	
	if (selectedIndex >= numItems)
		selectedIndex = numItems - 1;
	else if (selectedIndex < 0)
		selectedIndex = 0;
	
	MakeSelectionVisible();
}

/*
void c------------------------------() {}
*/

void ScrollWindow::MakeSelectionVisible()
{
	int maxScrollTop = (numItems - height);
	
	// if selection is completely not visible, put it at the upper part of the screen
	if (selectedIndex < scrollTop + SEL_SCROLL_MARGIN)
	{
		scrollTop = selectedIndex - SEL_SCROLL_MARGIN;
	}
	else
	{
		if (selectedIndex > scrollBottom() - SEL_SCROLL_MARGIN)
			scrollTop = selectedIndex - (height - 1 - SEL_SCROLL_MARGIN);
	}
	
	// sanity check ensure we don't go past ends
	if (scrollTop > maxScrollTop)
		scrollTop = maxScrollTop;
	
	if (scrollTop < 0) scrollTop = 0;
}

void ScrollWindow::Draw()
{
	if (needRedrawBorder)
	{
		needRedrawBorder = false;
		cdrawwindow(x1-1, y1-1, x2+1, y2+1, title.String(), 14, BG_COLOR);
	}
	
	// draw lines
	char lineBuffer[width + 1];
	int numVisibleItems = height;
	int maxLineLength = (width - 4);
	
	coninhibitupdate(true);
	
	for(int i=0;i<numVisibleItems;i++)
	{
		int lineNo = scrollTop + i;
		gotoxy(x1+1, y1+i);
		textbg(BG_COLOR);
		
		if (lineNo == activeIndex)
		{
			textcolor(ACTIVE_FG);
			cstatnocr("*");
		}
		else
		{
			textcolor(FG_COLOR);
			cstatnocr(" ");
		}
		
		if (lineNo == selectedIndex)
		{
			textcolor((lineNo == activeIndex) ? ACTIVE_FG : SELECTED_FG);
			textbg(SELECTED_BG);
		}
		else if (lineNo == activeIndex)
		{
			textcolor(ACTIVE_FG);
			textbg(ACTIVE_BG);
		}
		else
		{
			textcolor(FG_COLOR);
			textbg(BG_COLOR);
		}
		
		// fetch the line that goes in this position and limit it's length
		const char *line = GetItemAtIndex(lineNo);
		
		int lineLength = strlen(line);
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
	}
	
	coninhibitupdate(false);
}
