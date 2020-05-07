
#ifndef _SCANNER_H
#define _SCANNER_H

// scanner and preprocessor
#define EOF_CHAR		8

enum {		// ExclusionFlags for next_char
	SKIP_WHITESPACE = 0x01,
	SKIP_EOL = 0x02
};

class Scanner
{
public:
	Scanner();
	~Scanner();
	
	bool LoadFile(const char *fname);
	bool LoadString(const char *str);
	bool LoadText(const char *newText, int length);
	void Clear();
	
	int next_char();
	int peek_next_char();
	int back_char();
	
	int next_char(uint8_t exclusionFlags);
	int peek_next_char(uint8_t exclusionFlags);
	int back_char(uint8_t exclusionFlags);
	
	inline int get_pos() { return cursor; }
	void push_pos();
	void pop_pos();
	int discard_pos();	// pop a pushed position and return it-- don't change the cursor position
	
	int get_linenum_at_pos(int pos);
	int get_linenum_at_cursor(int offset = 0);
	
private:
	void populate_line_lut(void);
	
	char *text;
	int textLength;
	int cursor;
	
	int *line_lut;
	IntList pos_stack;
};


#endif
