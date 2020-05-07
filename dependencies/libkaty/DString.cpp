
#include "katy.h"
#include "DString.h"
#include "DString.fdh"

DString::DString()
{
}

DString::DString(const char *str)
{
	SetTo(str);
}

DString::DString(const char *str, int length)
{
	SetTo(str, length);
}

DString::DString(DString *other)
{
	//SetTo(other->String(), other->Length());
	SetTo(other);
}

DString::DString(DString &other)
{
	//SetTo(other.String(), other.Length());
	SetTo(other);
}

/*
void c------------------------------() {}
*/

void DString::SetTo(const char *str, int length)
{
	fBuffer.SetTo((uint8_t *)str, length);
}

void DString::SetTo(const char *str)
{
	fBuffer.SetTo((uint8_t *)str, strlen(str));
}

void DString::SetTo(DString *other)
{
	fBuffer.SetTo(other->fBuffer.Data(), other->fBuffer.Length());
}

void DString::SetTo(DString &other)
{
	fBuffer.SetTo(other.fBuffer.Data(), other.fBuffer.Length());
}

/*
void c------------------------------() {}
*/

void DString::AppendString(const char *str)
{
	fBuffer.AppendData((uint8_t *)str, strlen(str));
}

void DString::AppendString(const char *str, int length)
{
	fBuffer.AppendData((uint8_t *)str, length);
}

void DString::AppendChar(char ch)
{
	fBuffer.AppendData((uint8_t *)&ch, 1);
}

/*
void c------------------------------() {}
*/

void DString::ReplaceString(const char *from, const char *to)
{
DString newstr;
int seekindex;
bool changed = false;
int fromlen, tolen;

	seekindex = 0;
	for(;;)
	{
		char *base = String() + seekindex;
		char *hitptr = strstr(base, from);
		if (!hitptr)
		{	// no more hits in string
			if (changed)
			{
				newstr.AppendString(base);
				SetTo(newstr);
			}
			
			return;
		}
		
		if (!changed)
		{
			changed = true;
			fromlen = strlen(from);
			tolen = strlen(to);
		}
		
		int hitindex = (hitptr - base);
		if (hitindex) newstr.AppendString(base, hitindex);
		newstr.AppendString(to, tolen);
		
		seekindex += hitindex;
		seekindex += fromlen;
	}
}

void DString::ToUpperCase()
{
	char *str = String();
	for(int i=0;str[i];i++)
		str[i] = toupper(str[i]);
}

void DString::ToLowerCase()
{
	char *str = String();
	for(int i=0;str[i];i++)
		str[i] = tolower(str[i]);
}

/*
void c------------------------------() {}
*/

void DString::EnsureAlloc(int min_required)
{
	fBuffer.EnsureAlloc(min_required);
}

void DString::ReplaceUnprintableChars()
{
	fBuffer.ReplaceUnprintableChars();
}

/*
void c------------------------------() {}
*/

void DString::Clear()
{
	fBuffer.Clear();
}

int DString::Length()
{
	return fBuffer.Length();
}

char *DString::String()
{
	return fBuffer.String();
}

char *DString::TakeString()
{
	return fBuffer.TakeString();
}

char *DString::StaticString()
{
	return staticstr(fBuffer.String());
}

/*
void c------------------------------() {}
*/

bool DString::Contains(const char *searchString)
{
	return strstr(String(), searchString) != NULL;
}

int DString::IndexOf(const char *searchString)
{
	char *str = String();
	char *ptr = strstr(str, searchString);
	if (!ptr) return -1;
	
	return ptr - str;
}

DString *DString::Substring(int offset, int length, DString *out)
{
	if (offset < 0)
	{
		length += offset;
		offset = 0;
	}
	
	if (offset >= Length() || length <= 0)
	{
		out->Clear();
	}
	else if (offset + length >= Length())
	{
		out->SetTo(&String()[offset]);
	}
	else
	{
		out->SetTo(&String()[offset], length);
	}
	
	return out;
}

/*
void c------------------------------() {}
*/
	
char DString::CharAt(int index)
{
	if (index < 0 || index >= Length())
		return 0;
	
	return (char)fBuffer.Data()[index];
}

char DString::LastChar()
{
	int offset = Length() - 1;
	if (offset < 0)
		return 0;
	
	return (char)fBuffer.Data()[offset];
}

char DString::FirstChar()
{
	return String()[0];
}

/*
void c------------------------------() {}
*/

// remove any whitespace from either side of the string
void DString::Trim()
{
	// left trim
	char *orgstr = String();
	char *str = orgstr;
	while(str[0] == ' ' || str[0] == '\t') str++;
	
	// right trim
	int len = Length() - (str - orgstr);
	while(len > 0)
	{
		if (str[len-1] != ' ' && str[len-1] != '\t')
			break;
		
		len--;
	}
	
	// apply
	SetTo(str, len);
}

// strip quotes of any the types given in the string
bool DString::StripQuotes(const char *quoteTypes)
{
	if (Length() == 0)
		return 0;	// empty string, we're done
	
	if (!quoteTypes)
		quoteTypes = "\"'";
	
	// look at the first and last char and see if they are any of
	// the characters that constitute a quote
	int fch = FirstChar();
	int lch = LastChar();
	
	bool hasStartingQuote = (strchr(quoteTypes, fch) != NULL);
	bool hasEndingQuote = (strchr(quoteTypes, lch) != NULL);
	
	if (hasStartingQuote != hasEndingQuote)
	{
		staterr("mismatched quotes: %s quote is missing", hasEndingQuote ? "starting" : "ending");
		return 1;
	}
	
	if (!hasStartingQuote)
		return 0;	// it's unquoted, we're done
	
	if (fch != lch)
	{
		staterr("mismatched quote types: starting <%s>, ending <%s>", printChar(fch), printChar(lch));
		return 1;
	}
	
	if (Length() < 2)	// string is just a single quote
	{
		staterr("string contains just a single quote: <%s>", printChar(fch));
		return 1;
	}
	
	// ok, we are good, strip them!
	SetTo(String() + 1, Length() - 2);
	return 0;
}


