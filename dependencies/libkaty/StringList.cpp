
#include <stdlib.h>
#include <string.h>

#include "StringList.h"
#include "StringList.fdh"


StringList::~StringList()
{
	MakeEmpty();
}

/*
void c------------------------------() {}
*/

void StringList::Shuffle()
{
int i, count = CountItems();

	for(i=0;i<count;i++)
	{
		int swap = randrange(0, count - 1);
		if (swap != i)
		{
			SwapItems(i, swap);
		}
	}
}

bool StringList::ContainsString(const char *term)
{
int i;
char *str;

	for(i=0; str = StringAt(i); i++)
	{
		if (!strcmp(str, term))
			return true;
	}
	
	return false;
}

bool StringList::ContainsCaseString(const char *term)
{
int i;
char *str;

	for(i=0; str = StringAt(i); i++)
	{
		if (!strcasecmp(str, term))
			return true;
	}
	
	return false;
}

int StringList::IndexOfString(const char *term)
{
int i;
char *str;

	for(i=0; str = StringAt(i); i++)
	{
		if (!strcmp(str, term))
			return i;
	}
	
	return -1;
}

/*
void c------------------------------() {}
*/

void StringList::AddString(const char *str)
{
	BList::AddItem(strdup(str));
}

void StringList::AddString(const char *str, int len)
{
	char *temp = (char *)malloc(len + 1);
	memcpy(temp, str, len);
	temp[len] = 0;
	
	BList::AddItem(temp);
}

void StringList::AddStringNoCopy(const char *str)
{
	BList::AddItem(str);
}

bool StringList::RemoveString(int index)
{
	char *str = StringAt(index);
	if (str)
	{
		BList::RemoveItem(index);
		free(str);
		return 0;
	}
	
	return 1;
}

// removes ALL occurances of the given string from the list, if any, and returns number of removed items
int StringList::RemoveString(const char *str)
{
int i, removedCount = 0;
char *entry;

	for(i=0; entry = StringAt(i); i++)
	{
		if (!strcmp(entry, str))
		{
			BList::RemoveItem(i);
			free(entry);
			removedCount++;
			i--;
		}
	}
	
	return removedCount;
}

int StringList::RemoveCaseString(const char *str)
{
int i, removedCount = 0;
char *entry;

	for(i=0; entry = StringAt(i); i++)
	{
		if (!strcasecmp(entry, str))
		{
			BList::RemoveItem(i);
			free(entry);
			removedCount++;
			i--;
		}
	}
	
	return removedCount;
}

/*
void c------------------------------() {}
*/

void StringList::SwapItems(int index1, int index2)
{
	BList::SwapItems(index1, index2);
}

void StringList::DumpContents()
{
int i, count = CountItems();

	stat("StringList %08x; %d entries", this, count);
	for(i=0;i<count;i++)
	{
		char *str = StringAt(i);
		stat("(%d) <%08x>: '%s'", i, str, str ? str : "(null)");
	}
}

/*
void c------------------------------() {}
*/

char *StringList::StringAt(int index) const
{
	return (char *)BList::ItemAt(index);
}

void StringList::MakeEmpty()
{
	int i, count = CountItems();
	if (count)
	{
		for(i=0;i<count;i++)
			free(ItemAt(i));
		
		BList::MakeEmpty();
	}
}

/*
void c------------------------------() {}
*/

StringList &StringList::operator= (const StringList &other)
{
	StringList::MakeEmpty();
	
	for(int i=0;;i++)
	{
		char *str = other.StringAt(i);
		if (!str) break;
		
		AddString(str);
	}
	
	return *this;
}

bool StringList::operator== (const StringList &other) const
{
	if (CountItems() != other.CountItems())
		return false;
	
	for(int i=0;;i++)
	{
		char *str1 = StringAt(i);
		char *str2 = other.StringAt(i);
		
		if (!str1 || !str2)
			return (!str1 && !str2);
		
		if (strcmp(str1, str2) != 0)
			return false;
	}
}

bool StringList::operator!= (const StringList &other) const
{
	return !(*this == other);
}


