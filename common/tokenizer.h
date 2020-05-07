
#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include "scanner.h"
class Token;

#define EOF_CHAR		8

enum	// Token types
{
	TKN_WORD = 1,
	TKN_NUMBER,
	TKN_VARIABLE,
	TKN_PIN,
	
	TKN_COMMA,
	TKN_EQUALS,
	TKN_NOTEQUALS,
	TKN_COLON,
	TKN_EXCLAMATION,
	TKN_INVOKE_MACRO,
	TKN_INCLUDE,
	
	TKN_OPEN_CURLY_BRACE,
	TKN_CLOSE_CURLY_BRACE,
	TKN_OPEN_SQUARE_BRACKET,
	TKN_CLOSE_SQUARE_BRACKET,
	TKN_OPEN_PAREN,
	TKN_CLOSE_PAREN,
	
	TKN_IF,
	TKN_ELSE,
	TKN_LOGICAL_AND,
	TKN_LOGICAL_OR,
	TKN_BITWISE_AND,
	TKN_BITWISE_OR,
	TKN_PLUS,
	TKN_MINUS,
	
	TKN_INPUTS,
	TKN_OUTPUTS,
	
	TKN_ENUM,
	TKN_MACRO,
	TKN_ENDM,
	TKN_END,
	
	TKN_DSTREG,
	TKN_SRCREG,
	TKN_PARM8,
	TKN_PARM16,
	
	TKN_EOL,
	TKN_EOF,
	
	TKN_LAST
};

struct KeywordDef
{
	const char *word;
	int tokenType;
	
	// if false, will not emit more than one of this type of token in a row
	bool can_emit_concurrent;
	// if true, token must be followed by whitespace or EOL
	bool whole_word_only;
};


class Token
{
public:
	Token();
	Token(int type, const char *text = NULL);
	Token(int type, int value);
	~Token();
	
	const char *Describe();
	const char *DescribeShort();
	
	Token *Duplicate()
	{
		Token *t = new Token(type, text);
		t->value = value;
		t->line = line;
		return t;
	}
	
public:
	int type;
	char *text;
	int value;
	int line;
};


class Tokenizer
{
public:
	Tokenizer();
	Tokenizer(List<Token> *tokenList);
	~Tokenizer();
	
	bool Tokenize_File(const char *fname);
	bool Tokenize_String(const char *string, int stringLength = -1);
	
	void DecodePins(bool newValue) { decodePins = newValue; }
	void SimpleMatching(bool newValue) { simpleMatching = newValue; }
	
	void Clear();
	void rewind();
	
	Token *next_token();
	Token *next_token_except_eol();
	Token *peek_next_token();
	Token *back_token();
	
	bool read_line(List<Token> *lineout, bool skip_empty_lines = false);
	
	Token *expect_token(int expect_type, const char *expect_text = NULL);
	bool expect_token(Token *tkn, int expect_type, const char *expect_text = NULL);
	
	int CountItems() { return tokens ? tokens->CountItems() : 0; }
	List<Token> *GetTokens();
	
	// convenience functions for shorthand and asthetics
public:
	inline int get_current_line() { return s.get_linenum_at_cursor(); }
private:
	inline int next_char()								{ return s.next_char(); }
	inline int peek_next_char()							{ return s.peek_next_char(); }
	inline int back_char()								{ return s.back_char(); }
	
	inline int next_char(uint8_t exclusionFlags)		{ return s.next_char(exclusionFlags); }
	inline int peek_next_char(uint8_t exclusionFlags)	{ return s.peek_next_char(exclusionFlags); }
	inline int back_char(uint8_t exclusionFlags)		{ return s.back_char(exclusionFlags); }

private:
	void _init();
	bool Tokenize(void);
	
	void close_word();
	void add_token(Token *tkn);
	void add_token(int type, const char *text = NULL);
	
private:
	Scanner s;
	DString curword;
	int word_starting_line;
	
	bool decodePins;
	bool simpleMatching;
	
private:
	int tokenCursor;
	List<Token> *tokens;
};

const char *dump_token_list(List<Token> *list, const char *separator = NULL);
List<Token> *DuplicateTokenList(List<Token> *list);

#endif
