
#include <katy/katy.h>
#include "scanner.h"
#include "scanner.fdh"


Scanner::Scanner()
{
	text = NULL;
	line_lut = NULL;
	Clear();
}

Scanner::~Scanner()
{
	Clear();
}

/*
void c------------------------------() {}
*/

bool Scanner::LoadFile(const char *fname)
{
	Clear();
	
	statnocr("Scanner: Loading '%s': ");
	FILE *fp = fopen(fname, "rb");
	if (!fp)
	{
		staterr("failed to open file '%s': %s", fname, strerror(errno));
		return 1;
	}
	
	// load the entire file into memory
	fseek(fp, 0, SEEK_END);
	int fileLength = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	statnocr("%d bytes: ", fileLength);
	
	char *buffer = (char *)malloc(fileLength + 1);
	buffer[fileLength] = 0;
	fread(buffer, fileLength, 1, fp);
	fclose(fp);
	
	stat("OK");
	int result = LoadText(buffer, fileLength);
	free(buffer);
	return result;
}

bool Scanner::LoadString(const char *str)
{
	int len = strlen(str);
	return LoadText(str, len);
}

bool Scanner::LoadText(const char *newText, int length)
{
	Clear();
	if (length <= 0)
	{
		staterr("invalid text length %d", length);
		return 1;
	}
	
	// ensure there are only UNIX-style line endings in the file by removing any "\r" characters.
	char *buffer = (char *)malloc(length + 1);
	int newLength = 0;
	for(int i=0;i<length;i++)
	{
		if (newText[i] != '\r')
			buffer[newLength++] = newText[i];
	}
	
	if (newLength < length)
		buffer = (char *)realloc(buffer, newLength + 1);
	
	buffer[newLength] = 0;
	
	this->text = buffer;
	this->textLength = newLength;
	
	return 0;
}

void Scanner::Clear(void)
{
	if (text)
	{
		free(text);
		text = NULL;
	}
	
	if (line_lut)
	{
		free(line_lut);
		line_lut = NULL;
	}
	
	textLength = 0;
	cursor = 0;
	
	pos_stack.MakeEmpty();
}

/*
void c------------------------------() {}
*/

int Scanner::next_char()
{
	if (cursor < 0 || cursor >= textLength)
	{
		cursor++;
		return EOF_CHAR;
	}
	
	return text[cursor++];
}

int Scanner::peek_next_char()
{
	if (cursor < 0 || cursor >= textLength)
		return EOF_CHAR;
	
	return text[cursor];
}

int Scanner::back_char()
{
	cursor--;
	
	if (cursor < 0 || cursor >= textLength)
		return EOF_CHAR;
	
	return text[cursor];
}

/*
void c------------------------------() {}
*/

int Scanner::next_char(uint8_t exclusionFlags)
{
	for(;;)
	{
		char ch = next_char();
		if ((exclusionFlags & SKIP_WHITESPACE) && (ch == ' ' || ch == '\t')) continue;
		if ((exclusionFlags & SKIP_EOL) && ch == '\n') continue;
		
		return ch;
	}
}

int Scanner::back_char(uint8_t exclusionFlags)
{
	for(;;)
	{
		char ch = back_char();
		if ((exclusionFlags & SKIP_WHITESPACE) && (ch == ' ' || ch == '\t')) continue;
		if ((exclusionFlags & SKIP_EOL) && ch == '\n') continue;
		
		return ch;
	}
}

int Scanner::peek_next_char(uint8_t exclusionFlags)
{
char ch;

	push_pos();
	ch = next_char(exclusionFlags);
	pop_pos();
	
	return ch;
}
	
/*
void c------------------------------() {}
*/

void Scanner::push_pos()
{
	pos_stack.AddItem(cursor);
}

void Scanner::pop_pos()
{
	cursor = discard_pos();
}

int Scanner::discard_pos()
{
	if (pos_stack.CountItems() == 0)
	{
		staterr("Position stack underflow");
		exit(-1);
	}
	
	int value = pos_stack.LastItem();
	pos_stack.RemoveItem(pos_stack.CountItems() - 1);
	return value;
}

/*
void c------------------------------() {}
*/

void Scanner::populate_line_lut(void)
{
	if (line_lut) { free(line_lut); line_lut = NULL; }
	if (textLength <= 0)
		return;
	
	line_lut = (int *)malloc(textLength * sizeof(int));
	
	int currentLine = 1;
	for(int i=0;i<textLength;i++)
	{
		line_lut[i] = currentLine;
		if (text[i] == '\n') currentLine++;
	}
}

int Scanner::get_linenum_at_pos(int pos)
{
	if (!text || textLength <= 0) return -1;
	if (!line_lut) populate_line_lut();
	
	if (pos >= textLength) pos = textLength - 1;
	if (pos < 0) pos = 0;
	
	return line_lut[pos];
}

int Scanner::get_linenum_at_cursor(int offset)
{
	return get_linenum_at_pos(cursor + offset);
}
