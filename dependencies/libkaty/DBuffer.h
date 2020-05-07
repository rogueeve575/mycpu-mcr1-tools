
#ifndef _DBUFFER_H
#define _DBUFFER_H

/*
	DBuffer vs. DString
	
	The difference is that with a DBuffer, if you AppendString() multiple times,
	you will get null-terminators in between each string. With a DString,
	the strings will be concatenated. You can override this behavior in a DBuffer
	by calling AppendStringNoNull instead of AppendString, but there is no function
	for inserting NULLs into a DString, as that doesn't make sense.
*/

#define DBUFFER_BUILTIN_SIZE			128

class DBuffer
{
public:
	DBuffer();
	~DBuffer();
	
	void SetTo(const uint8_t *newData, int length);
	void SetTo(const char *string);	// DEPRECATED
	void SetTo(DBuffer *other);
	
	void AppendData(DBuffer *other) { AppendData(other->Data(), other->Length()); }
	void AppendData(const uint8_t *data, int length);
	void AppendString(const char *str);
	void AppendStringNoNull(const char *str);
	
	void AppendBool(bool value);
	void AppendChar(char ch);
	void Append8(uint8_t value);
	void Append16(uint16_t value);
	void Append32(uint32_t value);
	void Append64(uint64_t value);
	void Append16BE(uint16_t value);
	void Append32BE(uint32_t value);
	
	bool ReadTo(DBuffer *line, uint8_t ch, bool add_null=true);
	void EnsureAlloc(int min_required);
	
	void ReplaceUnprintableChars();
	
	DBuffer &operator= (const DBuffer &other);
	
	// ---------------------------------------
	
	void Clear();
	uint8_t *Data();
	char *String();
	int Length();
	void SetLength(int newLength);
	
	uint8_t *TakeData();
	char *TakeString();

private:
	uint8_t *fData;
	int fLength;
	int fAllocSize;
	bool fAllocdExternal;
	
	uint8_t fBuiltInData[DBUFFER_BUILTIN_SIZE];
};


#endif
