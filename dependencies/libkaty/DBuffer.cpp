
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "DBuffer.h"
#include "stat.h"
#include "DBuffer.fdh"


DBuffer::DBuffer()
{
	fData = &fBuiltInData[0];
	fAllocSize = sizeof(fBuiltInData);
	fAllocdExternal = false;
	fLength = 0;
}

DBuffer::~DBuffer()
{
	if (fAllocdExternal)
		free(fData);
}

/*
void c------------------------------() {}
*/

// append data to the end of the buffer
void DBuffer::AppendData(const uint8_t *data, int length)
{
	// handle case where data passed in is within our own buffer
	if (data >= fData && data <= fData + (fLength - 1))
	{	// yes it is. get the index within our buffer of the pointer we received
		int index = (data - fData);
		EnsureAlloc(fLength + length);
		data = fData + index;	// in case EnsureAlloc realloc'd
	}
	else
	{
		EnsureAlloc(fLength + length);
	}
	
	memcpy(&fData[fLength], data, length);
	fLength += length;
}

// append a string, along with it's null-terminator.
void DBuffer::AppendString(const char *str)
{
	AppendData((uint8_t *)str, strlen(str) + 1);
}

// append a string, without it's null-terminator.
void DBuffer::AppendStringNoNull(const char *str)
{
	AppendData((uint8_t *)str, strlen(str));
}


void DBuffer::AppendBool(bool value)
{
uint8_t ch = (uint8_t)value;
	AppendData((uint8_t *)&ch, 1);
}

void DBuffer::Append16(uint16_t value)
{
	AppendData((uint8_t *)&value, 2);
}

void DBuffer::Append32(uint32_t value)
{
	AppendData((uint8_t *)&value, 4);
}

void DBuffer::Append64(uint64_t value)
{
	AppendData((uint8_t *)&value, 8);
}

void DBuffer::Append16BE(uint16_t value)
{
	Append8(value >> 8);
	Append8(value & 0xff);
}

void DBuffer::Append32BE(uint32_t value)
{
	Append8((value >> 24) & 0xff);
	Append8((value >> 16) & 0xff);
	Append8((value >> 8) & 0xff);
	Append8(value & 0xff);
}

/*
void c------------------------------() {}
*/

void DBuffer::SetTo(const char *string)
{
	SetTo((const uint8_t *)string, strlen(string) + 1);
}

void DBuffer::SetTo(DBuffer *other)
{
	SetTo(other->Data(), other->Length());
}

void DBuffer::SetLength(int newLength)
{
	if (newLength <= fLength)
		fLength = newLength;
}

/*
void c------------------------------() {}
*/

void DBuffer::ReplaceUnprintableChars()
{
char *data = (char *)fData;
int length = fLength;
int i;

	for(i=0;i<length;i++)
	{
		if (data[i] == '\n' || data[i] == '\r')
		{
			data[i] = '+';
		}
		else if ((data[i] < 32 || ((unsigned char)data[i]) > 127) && data[i] != 0)
		{
			data[i] = '`';
		}
	}
}

/*
void c------------------------------() {}
*/

DBuffer& DBuffer::operator= (const DBuffer &other)
{
	SetTo((DBuffer *)&other);
	return *this;
}

// return the data contained in the buffer
uint8_t *DBuffer::Data()
{
	return (uint8_t *)fData;
}

// return the data, along with a trailing null-terminator
char *DBuffer::String()
{
	// ensure the data returned is null-terminated
	if (fLength == 0 || fData[fLength - 1] != '\0')
	{
		EnsureAlloc(fLength + 1);
		fData[fLength] = 0;
	}
	
	return (char *)fData;
}

// return the length of the buffer. note that this will include
// any null-terminators.
int DBuffer::Length()
{
	return fLength;
}

// return the data, transferring ownership to the caller, and clearing the buffer to empty.
uint8_t *DBuffer::TakeData(void)
{
uint8_t *data;

	if (fAllocdExternal)
	{	// giving them our external data block
		data = fData;
	}
	else
	{	// make a malloc'd copy of our internal data block for them to have
		data = (uint8_t *)malloc(fLength);
		memcpy(data, fBuiltInData, fLength);
	}
	
	// reset ourselves to empty and pointing at internal buffer
	fData = &fBuiltInData[0];
	fAllocSize = DBUFFER_BUILTIN_SIZE;
	fAllocdExternal = false;
	fLength = 0;
	
	return data;
}

// return the data as a string, transferring ownership to the caller,
// and clearing the buffer
char *DBuffer::TakeString(void)
{
	// ensure the data is null-terminated
	if (fLength == 0 || fData[fLength - 1] != '\0')
	{
		EnsureAlloc(fLength + 1);
		fData[fLength++] = 0;
	}
	
	return (char *)TakeData();
}

/*
void c------------------------------() {}
*/

void DBuffer::EnsureAlloc(int min_required)
{
	if (min_required > fAllocSize)
	{	// it's grown than our buffer; we'll need to reallocate.
		
		// give extra headroom for additional chars
		int headroom = (min_required / 2);
		if (headroom < DBUFFER_BUILTIN_SIZE) headroom = DBUFFER_BUILTIN_SIZE;
		else if (headroom > 256000) headroom = 256000;
		
		fAllocSize = (min_required + headroom);
		
		if (fAllocdExternal)
		{	// realloc to increase size of existing external buffer
			fData = (uint8_t *)realloc(fData, fAllocSize);
		}
		else
		{	// moving from internal to external memory
			fAllocdExternal = true;
			
			fData = (uint8_t *)malloc(fAllocSize);
			memcpy(fData, fBuiltInData, fLength);
		}
	}
}

void DBuffer::Clear()
{
	// free any external memory and switch back to builtin
	if (fAllocdExternal)
	{
		free(fData);
		fData = &fBuiltInData[0];
		fAllocSize = DBUFFER_BUILTIN_SIZE;
		fAllocdExternal = false;
	}
	
	fLength = 0;
}

void DBuffer::SetTo(const uint8_t *newData, int length)
{
	// we only free the old buffer after the new data is copied,
	// to prevent problems if a pointer into our current buffer is passed
	uint8_t *oldData = NULL;
	
	if (length < DBUFFER_BUILTIN_SIZE)
	{	// new data is small and can fit in internal memory
		if (fAllocdExternal)	// if we were previously external, become internal
		{
			oldData = fData;
			fAllocdExternal = false;
		}
		
		fData = &fBuiltInData[0];
		fAllocSize = sizeof(fBuiltInData);
		
		if (length)
			memmove(fData, newData, length);
	}
	else if (length > fAllocSize || length < fAllocSize / 2)
	{	// it's too big to be internal, and
		// 1) it is either bigger than our existing buffer, or
		// 2) it is significantly smaller than the last thing that was stored, so we should
		//    create a new buffer for it so as not to waste memory.
		if (fAllocdExternal) oldData = fData;
		fAllocdExternal = true;
		
		fAllocSize = (length + DBUFFER_BUILTIN_SIZE);	// arbitrary, is just space for growing
		fData = (uint8_t *)malloc(fAllocSize);
		if (length) memcpy(fData, newData, length);
	}
	else
	{	// it is external but fits in existing buffer and is not too much smaller, so let's re-use the same buffer
		
		// use memmove instead of copy because we can't be sure that user
		// didn't pass a pointer to within our own current buffer
		if (length)
			memmove(fData, newData, length);
	}
	
	fLength = length;
	
	if (oldData)
		free(oldData);
}

void DBuffer::AppendChar(char ch)
{
	AppendData((uint8_t *)&ch, 1);
}

void DBuffer::Append8(uint8_t value)
{
	AppendData(&value, 1);
}

