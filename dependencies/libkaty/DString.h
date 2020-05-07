
#ifndef _DSTRING_H
#define _DSTRING_H

//#include "basics.h"
#include "DBuffer.h"

/*
	DString vs. DBuffer
	DBuffer - for holding raw data
	DString - for strings; takes advantage of DBuffer internally
	
	The difference is that with a DBuffer, if you AppendString() multiple times,
	you will get null-terminators in between each string. With a DString,
	the strings will be concatenated. You can override this behavior in a DBuffer
	by calling AppendStringNoNull instead of AppendString, but there is no function
	for inserting NULLs into a DString, as that doesn't make sense.
*/

class DString
{
public:
	DString();
	DString(DString *other);
	DString(DString &other);
	DString(const char *string);
	DString(const char *string, int length);
	
	void SetTo(const char *string);
	void SetTo(const char *string, int length);
	void SetTo(DString *other);
	void SetTo(DString &other);
	
	void AppendString(const char *str);
	void AppendString(const char *str, int length);
	void AppendString(DString *other) { AppendString(other->String(), other->Length()); }
	void AppendString(DString &other) { AppendString(other.String(), other.Length()); }
	void AppendChar(char ch);
	
	void ReplaceString(const char *oldstring, const char *newstring);
	void ToUpperCase();
	void ToLowerCase();
	
	void EnsureAlloc(int min_required);
	
	void Clear();
	char *String();
	char *StaticString();
	char *TakeString();
	int Length();
	
	bool Contains(const char *searchString);
	int IndexOf(const char *searchString);
	DString *Substring(int offset, int length, DString *out);
	
	char CharAt(int index);
	char LastChar();
	char FirstChar();
	
	void ReplaceUnprintableChars();
	
	void Trim();
	bool StripQuotes(const char *quoteTypes = NULL);
	
private:
	DBuffer fBuffer;
};

#endif
