
//#define DEBUG
#include "common.h"
#include "tokenizer.fdh"

#define is_whitespace(ch)		((ch) == ' ' || (ch) == '\t')

static Token eof_token(TKN_EOF);

static bool is_first_char_of_keyword[256];
static int num_keywords;

static const KeywordDef *keywords;
static const KeywordDef unsorted_keywords[] =
{
//	word	token	concurrent	whole_word_only
	{ ",", TKN_COMMA, true, false },
	{ "=", TKN_EQUALS, true, false },
	{ "!=", TKN_NOTEQUALS, true, false },
	{ ":", TKN_COLON, true, false },
	
	{ "{", TKN_OPEN_CURLY_BRACE, true, false },
	{ "}", TKN_CLOSE_CURLY_BRACE, true, false },
	{ "[", TKN_OPEN_SQUARE_BRACKET, true, false },
	{ "]", TKN_CLOSE_SQUARE_BRACKET, true, false },
	{ "(", TKN_OPEN_PAREN, true, false },
	{ ")", TKN_CLOSE_PAREN, true, false },
	{ "!", TKN_EXCLAMATION, true, false },
	{ "@", TKN_INVOKE_MACRO, true, false },
	
	{ "if", TKN_IF, true, true },
	{ "else", TKN_ELSE, true, true },
	
	{ "&&", TKN_LOGICAL_AND, true, false },
	{ "||", TKN_LOGICAL_OR, true, false },
	{ "&", TKN_BITWISE_AND, true, false },
	{ "|", TKN_BITWISE_OR, true, false },
	{ "+", TKN_PLUS, true, false },
	{ "-", TKN_MINUS, true, true },
	
	{ "enum", TKN_ENUM, true, true },
	{ "macro", TKN_MACRO, true, true },
	{ "endm", TKN_ENDM, true, true },
	{ "end", TKN_END, true, true },
	{ "include", TKN_INCLUDE, true, true },
	
	{ "dstreg", TKN_DSTREG, true, true },
	{ "srcreg", TKN_SRCREG, true, true },
	{ "parm8", TKN_PARM8, true, true },
	{ "parm16", TKN_PARM16, true, true },
	{ "mem16", TKN_PARM16, true, true },
	
	{ "inputs", TKN_INPUTS, true, true },
	{ "outputs", TKN_OUTPUTS, true, true },
	
	{ "endfile", TKN_EOF, true, true },
	
	{ NULL, -1, 0 }
};

Tokenizer::Tokenizer()
{
	_init();
}

Tokenizer::Tokenizer(List<Token> *tokenList)
{
	_init();
	tokens = tokenList;
}

Tokenizer::~Tokenizer()
{
	Clear();
}

void Tokenizer::_init()
{
static Locker initLocker;
static bool inited = false;

	// check if we need to do initialization of any globals
	initLocker.Lock();
	if (!inited)
	{
		inited = true;
		stat("Tokenizer: performing first-time initilization...");
		
		// keywords must be listed in order of descending length
		keywords = sort_keywords(unsorted_keywords);
		
		// initialize the global lookup table which tells if each possible ASCII character
		// is the potential start of a keyword, if a previous class instance hasn't done so.
		memset(is_first_char_of_keyword, 0, sizeof(is_first_char_of_keyword));
		num_keywords = 0;
		
		for(int i=0;keywords[i].word;i++)
		{
			char ch = keywords[i].word[0];
			is_first_char_of_keyword[toupper(ch)] = true;
			is_first_char_of_keyword[tolower(ch)] = true;
			
			num_keywords++;
		}
	}
	initLocker.Unlock();
	
	// initialize variables
	tokens = NULL;
	tokenCursor = 0;
	decodePins = false;
	simpleMatching = false;
}

void Tokenizer::Clear(void)
{
	if (tokens)
	{
		tokens->Clear();
		delete tokens;
		tokens = NULL;
	}
	
	tokenCursor = 0;
	s.Clear();
}

/*
void c------------------------------() {}
*/

bool Tokenizer::Tokenize_File(const char *fname)
{
	Clear();
	
	if (s.LoadFile(fname))
	{
		staterr("failed to load file '%s' into scanner", fname);
		return 1;
	}
	
	// turn the raw text into tokens list
	if (Tokenize())
	{
		Clear();
		return 1;
	}
	
	stat("Tokenizer: Parsed '%s': %d tokens.", fname, tokens->CountItems());
	return 0;
}

bool Tokenizer::Tokenize_String(const char *string, int stringLength)
{
	Clear();
	
	if (stringLength < 0)
		stringLength = strlen(string);
	
	if (s.LoadText(string, stringLength))
	{
		staterr("Failed to load given %d-byte string into scanner.", stringLength);
		return 1;
	}
	
	if (Tokenize())
	{
		Clear();
		return 1;
	}
	
	//stat("Tokenizer: Parsed '%s': %d tokens", string, tokens->CountItems());
	return 0;
}


List<Token> *Tokenizer::GetTokens()
{
	List<Token> *out = tokens;
	tokenCursor = 0;
	tokens = NULL;
	return out;
}

/*
void c------------------------------() {}
*/

void Tokenizer::rewind()
{
	tokenCursor = 0;
}

Token *Tokenizer::next_token()
{
	Token *tkn = tokens->ItemAt(tokenCursor++);
	return tkn ? tkn : &eof_token;
}

Token *Tokenizer::next_token_except_eol()
{
Token *tkn;

	do
	{
		tkn = next_token();
	}
	while(tkn->type == TKN_EOL);
	
	return tkn;
}
	
Token *Tokenizer::peek_next_token()
{
	Token *tkn = tokens->ItemAt(tokenCursor);
	return tkn ? tkn : &eof_token;
}

Token *Tokenizer::back_token()
{
	Token *tkn = tokens->ItemAt(--tokenCursor);
	return tkn ? tkn : &eof_token;
}

// read tokens until an EOL or EOF is encountered, adding the tokens making up the line to the given list.
// the EOL or EOF token is not included.
// the tokens returned are duplicates and will belong to the caller.
// if the cursor is at end-of-file when the function is called, returns nonzero.
bool Tokenizer::read_line(List<Token> *lineout, bool skip_empty_lines)
{
	if (skip_empty_lines)
	{
		while(peek_next_token()->type == TKN_EOL)
			next_token();
	}
	
	if (peek_next_token()->type == TKN_EOF)
		return 1;
	
	for(;;)
	{
		Token *tkn = next_token();
		if (tkn->type == TKN_EOL || tkn->type == TKN_EOF) break;
		
		lineout->AddItem(tkn->Duplicate());
	}
	
	return 0;
}

/*
void c------------------------------() {}
*/

// fetches the next token and returns it if it is of the specified type,
// else displays and error message and returns NULL.
Token *Tokenizer::expect_token(int expect_type, const char *expect_text)
{
	Token *tkn = next_token();
	if (expect_token(tkn, expect_type, expect_text))
		return tkn;
		
	back_token(); back_token();
	Token *prevtkn = next_token();
	next_token();
	
	staterr("error after ( %s ) on line %d", prevtkn->Describe(), prevtkn->line);
	return NULL;
}

// returns true if the given token is of the specified type,
// else displays and error message and returns false.
bool Tokenizer::expect_token(Token *tkn, int expect_type, const char *expect_text)
{
	if (tkn == NULL)
	{
		Token expected(expect_type, expect_text);
		staterr("expected: ( %s ) instead of ( null )",
			expected.Describe());
		return 1;
	}
	
	bool ok = true;
	
	if (tkn->type != expect_type)
		ok = false;
	else if ((expect_text != NULL) && (tkn->text == NULL || strcasecmp(tkn->text, expect_text)))
		ok = false;
	
	if (!ok)
	{
		Token expected(expect_type, expect_text);
		staterr("expected: ( %s ) instead of ( %s )",
			expected.Describe(),
			//tkn->line,
			tkn->Describe());
	}
	
	return ok;
}

/*
void c------------------------------() {}
*/

void Tokenizer::add_token(Token *tkn)
{
	if (!tokens)
	{
		staterr("attempt to add %s without tokens list allocated", tkn->Describe());
		exit(1);
	}
	
	// don't add consecutive TKN_EOL's, there's no need
	if (tkn->type == TKN_EOL)
	{
		Token *lastToken = tokens->LastItem();
		if (lastToken && lastToken->type == TKN_EOL)
		{
			//stat("skipping redundant EOL");
			delete tkn;
			return;
		}
		
		if (lastToken)
			tkn->line = lastToken->line;
		else
			tkn->line = 1;
	}
	else
	{
		tkn->line = get_current_line();
	}
	
	dstat("added token ( %s ) from line %d", tkn->Describe(), tkn->line);
	tokens->AddItem(tkn);
}

void Tokenizer::add_token(int type, const char *text)
{
	Token *tkn = new Token(type, text);
	add_token(tkn);
}

/*
void c------------------------------() {}
*/

Token::Token()
{
	type = -1;
	value = -1;
	line = -1;
	text = NULL;
}

Token::Token(int _type, const char *_text)
{
	this->type = _type;
	this->text = _text ? strdup(_text) : NULL;
	this->value = -1;
	this->line = -1;
}

Token::Token(int _type, int _value)
{
	this->type = _type;
	this->value = _value;
	this->text = strdup(stprintf("%d", _value));
	this->line = -1;
}

Token::~Token()
{
	if (text) free(text);
}

const char *Token::Describe(void)
{
	DString str;
	const char *name;
	
	str.AppendString("{ ");
	
	switch(this->type)
	{
		case TKN_WORD: name = "TKN_WORD"; break;
		case TKN_NUMBER: name = "TKN_NUMBER"; break;
		case TKN_VARIABLE: name = "TKN_VARIABLE"; break;
		case TKN_PIN: name = "TKN_PIN"; break;
		
		case TKN_COMMA: name = "TKN_COMMA"; break;
		case TKN_EQUALS: name = "TKN_EQUALS"; break;
		case TKN_NOTEQUALS: name = "TKN_NOTEQUALS"; break;
		case TKN_COLON: name = "TKN_COLON"; break;
		case TKN_EXCLAMATION: name = "TKN_EXCLAMATION"; break;
		case TKN_INVOKE_MACRO: name = "TKN_INVOKE_MACRO"; break;
		
		case TKN_OPEN_CURLY_BRACE: name = "TKN_OPEN_CURLY_BRACE"; break;
		case TKN_CLOSE_CURLY_BRACE: name = "TKN_CLOSE_CURLY_BRACE"; break;
		
		case TKN_OPEN_SQUARE_BRACKET: name = "TKN_OPEN_SQUARE_BRACKET"; break;
		case TKN_CLOSE_SQUARE_BRACKET: name = "TKN_CLOSE_SQUARE_BRACKET"; break;
		
		case TKN_OPEN_PAREN: name = "TKN_OPEN_PAREN"; break;
		case TKN_CLOSE_PAREN: name = "TKN_CLOSE_PAREN"; break;
		
		case TKN_IF: name = "TKN_IF"; break;
		case TKN_ELSE: name = "TKN_ELSE"; break;
		
		case TKN_ENUM: name = "TKN_ENUM"; break;
		case TKN_MACRO: name = "TKN_MACRO"; break;
		case TKN_ENDM: name = "TKN_ENDM"; break;
		case TKN_END: name = "TKN_END"; break;
		case TKN_INCLUDE: name = "TKN_INCLUDE"; break;
		
		case TKN_LOGICAL_AND: name = "TKN_LOGICAL_AND"; break;
		case TKN_LOGICAL_OR: name = "TKN_LOGICAL_OR"; break;
		case TKN_BITWISE_AND: name = "TKN_BITWISE_AND"; break;
		case TKN_BITWISE_OR: name = "TKN_BITWISE_OR"; break;
		case TKN_PLUS: name = "TKN_PLUS"; break;
		case TKN_MINUS: name = "TKN_MINUS"; break;
		
		case TKN_INPUTS: name = "TKN_INPUTS"; break;
		case TKN_OUTPUTS: name = "TKN_OUTPUTS"; break;
		
		case TKN_DSTREG: name = "TKN_DSTREG"; break;
		case TKN_SRCREG: name = "TKN_SRCREG"; break;
		case TKN_PARM8: name = "TKN_PARM8"; break;
		case TKN_PARM16: name = "TKN_PARM16"; break;
		
		case TKN_EOL: name = "TKN_EOL"; break;
		case TKN_EOF: name = "TKN_EOF"; break;
		
		default:
			name = stprintf("(invalid token type %d)", this->type);
			break;
	}
	
	str.AppendString(name);
	
	if (this->text)
	{
		const char *displayText = this->text;
		if (displayText[0] == '\n')
			displayText = "\\n";
		
		str.AppendString(stprintf(" [text='%s']", displayText));
	}
	
	/*if (this->indexText)
		str.AppendString(stprintf(" [indexText='%s']", this->indexText));*/
	
	if (this->value >= 0)
		str.AppendString(stprintf(" [value=%d]", this->value));
	
	str.AppendString(" }");
	
	return str.StaticString();
}

const char *Token::DescribeShort()
{
	if (type == TKN_WORD)
		return text;
	else if (type == TKN_NUMBER)
	{
		if (value > 255)
			return stprintf("0x%03X", value);
		else
			return stprintf("%d", value);
	}
	else if (type == TKN_VARIABLE)
		return stprintf("$%s", value);
	else
		return Describe();
}

/*
void c------------------------------() {}
*/

void Tokenizer::close_word()
{
	if (curword.Length() == 0)
		return;
	
	Token *tkn = new Token(TKN_WORD);
	tkn->text = strdup(curword.String());
	
	if (word_starting_line)
	{
		tkn->line = word_starting_line;
		word_starting_line = 0;
	}
	else
	{
		staterr("close_word without word_starting_line: '%s'", curword.String());
	}
	
	if (is_string_numeric(tkn->text, false, false))
	{
		tkn->type = TKN_NUMBER;
		tkn->value = atoi(tkn->text);
	}
	else if (strbegin(tkn->text, "0x") && \
			 str_contains_only(tkn->text+2, "0123456789ABCDEFabcdef"))
	{
		tkn->type = TKN_NUMBER;
		tkn->value = strtol(tkn->text+2, NULL, 16);
	}
	else if (strbegin(tkn->text, "0b") && \
			 str_contains_only(tkn->text+2, "01"))
	{
		tkn->type = TKN_NUMBER;
		tkn->value = strtol(tkn->text+2, NULL, 2);
	}
	else if (decodePins && \
				((toupper(tkn->text[0]) == 'A' || \
				toupper(tkn->text[0]) == 'D') && \
				tkn->text[1] && is_string_numeric(&tkn->text[1], false, false)))
	{
		tkn->type = TKN_PIN;
		tkn->value = atoi(&tkn->text[1]);
	}
	else if (tkn->text[0] == '$')
	{
		tkn->type = TKN_VARIABLE;
		memmove(tkn->text, tkn->text+1, strlen(tkn->text));
	}
	
	//stat("Closed word and added token: %s", tkn->Describe());
	curword.Clear();
	add_token(tkn);
}

// read the rawText and create a list of tokens in "tokens"
bool Tokenizer::Tokenize(void)
{
int ch;

	//stat("Tokenizer::Tokenize: parsing %d bytes", rawTextLength);
	
	if (tokens) { tokens->Clear(); delete tokens; }
	tokens = new List<Token>();
	
	word_starting_line = 0;
	curword.Clear();
	
	bool startsWithWhitespace = true;
	for(;;)
	{
		ch = next_char();
		dstat("read char '%s'", printChar(ch));
		
		// whitespace closes any open words, but don't process it.
		if (is_whitespace(ch))
		{
			close_word();
			ch = next_char(SKIP_WHITESPACE);	// run past the whitespace and we'll process that char instead
			startsWithWhitespace = true;
		}
		
		// skip over comments -- TODO, these should be removed by preprocessor, not handled here
		if (ch == '/')
		{
			int next_ch = peek_next_char();
			if (next_ch == '/')		//	'//' end-of-line comment
			{
				dstat("found // comment, skipping to end-of-line");
				for(;;)
				{
					ch = next_char();
					if (ch == '\n' || ch == EOF_CHAR)
					{
						break;
					}
				}
			}
			else if (next_ch == '*')
			{
				next_char();	// read in the '*'
				for(;;)
				{
					ch = next_char();
					
					if (ch == EOF_CHAR)
					{
						staterr("unclosed block comment");
						return 1;
					}
					else if (ch == '*')
					{
						if (peek_next_char() == '/')
						{
							next_char();		// read in the '/'
							ch = next_char();	// read in the next char for processing below
							break;
						}
					}
				}
			}
		}
		
		if (ch == '\n')
		{
			close_word();
			add_token(TKN_EOL);
			startsWithWhitespace = true;
			continue;
		}
		else if (ch == EOF_CHAR)
		{
			close_word();
			break;
		}
		else if (ch == '\"')
		{
			// read to end of quotes and emit as a single TKN_WORD
			close_word();
			word_starting_line = s.get_linenum_at_cursor(-1);	// record line # of first character of word
			
			bool escapeChar = false;
			for(;;)
			{
				ch = next_char();
				
				if (ch == '\n' || ch == EOF_CHAR)
				{
					staterr("unclosed quote before end of %s", (ch == EOF_CHAR) ? "file" : "line");
					return 1;
				}
				
				if (escapeChar)
				{
					escapeChar = false;
					curword.AppendChar(ch);
				}
				else if (ch == '\"')
				{
					break;
				}
				else if (ch == '\\')
				{
					escapeChar = true;
				}
				else
				{
					curword.AppendChar(ch);
				}
			}
			
			close_word();
			startsWithWhitespace = true;
			continue;
		}
		
		// check if this could possibly be the start of a keyword
		bool created_keyword_token = false;
		if (is_first_char_of_keyword[ch] && !simpleMatching)
		{
			//stat("  char is first char of a sequence...scanning keywords");
			for(int i=0;i<num_keywords;i++)
			{
				const char *keyword = keywords[i].word;
				if (toupper(ch) != toupper(keyword[0])) continue;
				if (keywords[i].whole_word_only && !startsWithWhitespace) continue;
				
				// save the current position in case it's wrong
				s.push_pos();
				
				// peek ahead to see if the rest of the identifier is there
				bool sequenceMatches = true;	// assume
				for(int j=1;keyword[j];j++)
				{
					if (toupper(next_char()) != toupper(keyword[j]))
					{
						sequenceMatches = false;
						break;
					}
				}
				
				// for keywords marked whole_word_only, ensure that the following char is whitespace or EOL
				if (sequenceMatches && keywords[i].whole_word_only)
				{
					int pch = peek_next_char();
					if (!is_whitespace(pch) && pch != '\n' && pch != ',' && pch != EOF_CHAR)
						sequenceMatches = false;
				}
				
				if (!sequenceMatches)
				{
					s.pop_pos();
				}
				else
				{
					//stat("FOUND keyword #%d", i);
					s.discard_pos();
					close_word();
					
					if (!keywords[i].can_emit_concurrent)
					{
						Token *lasttkn = tokens->LastItem();
						if (lasttkn && lasttkn->type == keywords[i].tokenType)
						{
							//dstat("not emitting concurrent token");
							created_keyword_token = true;
							break;
						}
					}
					
					Token *tkn = new Token(keywords[i].tokenType);
					tkn->text = strdup(keywords[i].word);
					
					add_token(tkn);
					//dstat("Added sequence token: %s", tkn->Describe());
					
					if (tkn->type == TKN_EOF)	// for "endfile" token
						goto done;
					
					created_keyword_token = true;
					break;
				}
			}
		}
		
		if (!created_keyword_token)
		{
			if (curword.Length() == 0)		// starting a new line
				word_starting_line = s.get_linenum_at_cursor(-1);	// record line # of first character of word
			
			curword.AppendChar(ch);
		}
		
		startsWithWhitespace = false;
	}
	
done: ;
	//stat("Tokenizer::Tokenize: %d tokens generated.", tokens->CountItems());
	return 0;
}

const char *dump_token_list(List<Token> *list, const char *separator)
{
	if (list->CountItems() == 0)
		return "(empty)";
	
	if (!separator) separator = " ";
	int separatorLen = strlen(separator);
	
	DString str;
	for(int i=0;;i++)
	{
		Token *tkn = list->ItemAt(i);
		if (!tkn) break;
		
		if (i) str.AppendString(separator, separatorLen);
		str.AppendString(tkn->DescribeShort());
	}
	
	return str.StaticString();
}

List<Token> *DuplicateTokenList(List<Token> *list)
{
	List<Token> *newList = new List<Token>();
	for(int i=0;;i++)
	{
		Token *tkn = list->ItemAt(i);
		if (!tkn) break;
		
		newList->AddItem(tkn->Duplicate());
	}
	
	return newList;
}

// TODO: move to a "util.cpp" along with any func now in main()
// returns true if str contains only the set of characters in allowed_chars.
// empty strings fail the test.
bool str_contains_only(const char *str, const char *allowed_chars)
{
char ch;

	if (*str == '\0')
		return false;
	
	while((ch = *str) != '\0')
	{
		if (!strchr(allowed_chars, ch))
			return false;
		
		str++;
	}
	
	return true;
}

// entries in the keywords list must be in descending order of length, so that
// keywords which share the first few letters can't produce incorrect results.
// for example, if "end" was allowed to before "endfile" in the keywords list,
// it might be seen as a TKN_END followed by the word "file".
static const KeywordDef *sort_keywords(const KeywordDef *unsortedList)
{
	// count how many keywords there are
	int num_keywords = 0;
	while(unsortedList[num_keywords].word) num_keywords++;
	stat("Tokenizer: sorting %d keywords...", num_keywords);
	
	// allocate the keywords array
	KeywordDef *keywords_out = (KeywordDef *)malloc((num_keywords + 1) * sizeof(KeywordDef));
	
	// precalculate the length of every keyword to avoid lots of calls to strlen
	int keylen[num_keywords];
	for(int i=0;i<num_keywords;i++)
		keylen[i] = strlen(unsortedList[i].word);
	
	// sort the keywords
	int lastLength = INT_MAX;
	int nextKeywordIndex = 0;
	for(;;)
	{
		// find the length of the longest keyword(s) which are less than the last length done.
		int biggestLength = 0;
		for(int i=0;i<num_keywords;i++)
		{
			if (keylen[i] > biggestLength && keylen[i] < lastLength)
				biggestLength = keylen[i];
		}
		
		if (biggestLength == 0)
			break;
		
		// add all keywords of the found length
		dstat("SORT: adding all keywords of length %d", biggestLength);
		for(int i=0;i<num_keywords;i++)
		{
			if (keylen[i] == biggestLength)
			{
				dstat("> '%s'", unsortedList[i]);
				memcpy(&keywords_out[nextKeywordIndex++], &unsortedList[i], sizeof(KeywordDef));
			}
		}
		
		lastLength = biggestLength;
	}
	
	memset(&keywords_out[num_keywords], 0, sizeof(KeywordDef));		// null-terminate the output list
	return keywords_out;
}
