
#ifndef _SCROLLWINDOW_H
#define _SCROLLWINDOW_H


class ScrollWindow
{
public:
	ScrollWindow(int _x1, int _y1, int _x2, int _y2);
	~ScrollWindow();
	
	void SetNumItems(int num);
	void SetTitle(const char *newTitle);
	
	void SetSelectionIndex(int index);
	inline int GetSelectionIndex() { return selectedIndex; }
	void MakeSelectionVisible();
	
	inline int Width() { return width; }
	inline int Height() { return height; }
	
	// "active" line refers to the line that will next be executed
	inline void SetActiveIndex(int index) { activeIndex = index; }
	inline bool EnableManualSelMove(bool enable) { enableManualSelMove = enable; }
	
	void SetIndex(int index)
	{
		SetActiveIndex(index);
		SetSelectionIndex(index);
	}
	
	virtual const char *GetItemAtIndex(int index)
	{
		return stprintf("%d. Item %d", 'A'+index);
	}
	
	void Draw();
	
	
private:
	inline int scrollBottom() { return scrollTop + (height - 1); }

private:
	int x1, y1, x2, y2;
	int width, height;
	
	int numItems;
	int scrollTop;
	int selectedIndex;
	int activeIndex;
	
	bool isForeground;
	bool needRedrawBorder;
	bool enableManualSelMove;
	DString title;
};



#endif
