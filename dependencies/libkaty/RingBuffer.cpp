
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "RingBuffer.h"
#include "RingBuffer.fdh"

RingBuffer::RingBuffer(int maxitems)
{
	fHead = fTail = 0;
	fSize = maxitems + 1;
	
	int nbytes = fSize * sizeof(size_t);
	fBuffer = (size_t *)malloc(nbytes);
	memset(fBuffer, 0, nbytes);
}

RingBuffer::~RingBuffer()
{
	free(fBuffer);
}

void RingBuffer::Dump(void)
{
char str[64];
int i;

	for(i=0;i<fSize;i++)
	{
		sprintf(str, "      %08zx", fBuffer[i]);
		if (i == fHead) { str[0] = 'H'; str[1] = '>'; }
		if (i == fTail) { str[3] = 'T'; str[4] = '>'; }
		stat("%s", str);
	}
}

/*
void c------------------------------() {}
*/

// add an item to the buffer, overwriting the oldest item if the buffer is full.
void *RingBuffer::AddItem(void *item)
{
	fBuffer[fHead] = (size_t)item;
	if (++fHead >= fSize) fHead = 0;
	
	void *oldValue = NULL;
	if (fHead == fTail)
	{
		oldValue = (void *)fBuffer[fHead];
		if (++fTail >= fSize)
			fTail = 0;
	}
	
	return oldValue;
}

// return and remove the oldest item in the buffer.
void *RingBuffer::ReadOldest()
{
	if (fHead == fTail)
		return NULL;
	
	void *item = (void *)fBuffer[fTail];
	if (++fTail >= fSize)
		fTail = 0;
	
	return item;
}

void *RingBuffer::PeekOldest()
{
	if (fHead == fTail)
		return NULL;
	
	void *item = (void *)fBuffer[fTail];
	return item;
}

// return and remove the newest item in the buffer.
void *RingBuffer::ReadNewest()
{
	if (fHead == fTail)
		return NULL;
	
	if (fHead == 0)
		fHead = (fSize - 1);
	else
		fHead--;
	
	return (void *)fBuffer[fHead];
}

void *RingBuffer::PeekNewest()
{
	void *item = ReadNewest();
	AddItem(item);
	
	return item;
}

bool RingBuffer::ContainsItem(void *item)
{
int p = fTail;

	for(;;)
	{
		if (p == fHead) return false;
		if ((void *)fBuffer[p] == item) return true;
		
		if (++p >= fSize)
			p = 0;
	}
}

/*
void c------------------------------() {}
*/

uint32_t RingBuffer::AddInt(uint32_t item)
{
	return (uint32_t)(size_t)AddItem((void *)(size_t)item);
}

uint32_t RingBuffer::ReadOldestInt()
{
	return (uint32_t)(size_t)ReadOldest();
}

uint32_t RingBuffer::ReadNewestInt()
{
	return (uint32_t)(size_t)ReadNewest();
}

bool RingBuffer::ContainsInt(uint32_t item)
{
	return ContainsItem((void *)(size_t)item);
}

/*
void c------------------------------() {}
*/

// returns how many items are in the buffer
int RingBuffer::CountItems()
{
	if (fTail > fHead)
	{	// eg "2, 1"
		// distance from tail to end, + head position
		return (fSize - fTail) + fHead;
	}
	else if (fTail < fHead)
	{	// eg "0, 5"
		// distance between head and tail
		return (fHead - fTail);
	}
	else
	{
		return 0;
	}
}

void RingBuffer::MakeEmpty()
{
	fHead = fTail = 0;
}






