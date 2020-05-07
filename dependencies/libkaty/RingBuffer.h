
#ifndef _RINGBUFFER_H
#define _RINGBUFFER_H

// generic Head & Tail circular buffer.
#include <cstring>

class RingBuffer
{
public:
	RingBuffer(int maxitems);
	virtual ~RingBuffer();
	
	void *AddItem(void *item);
	void *ReadOldest();
	void *ReadNewest();
	void *PeekOldest();
	void *PeekNewest();
	
	uint32_t AddInt(uint32_t item);
	uint32_t ReadOldestInt();
	uint32_t ReadNewestInt();
	
	bool ContainsItem(void *item);
	bool ContainsInt(uint32_t item);
	
	void MakeEmpty();
	int CountItems();
	
	inline bool IsEmpty() { return fHead == fTail; }
	inline bool IsFull()  { return CountItems() >= (fSize - 1); }
	
	void Dump(void);
	
private:
	size_t *fBuffer;
	int fHead, fTail, fSize;
};



#endif
