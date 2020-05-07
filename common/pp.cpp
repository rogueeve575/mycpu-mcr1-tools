
//#define DEBUG
#include "common.h"
#include "pp.fdh"
using namespace PP;

Preprocessor::Preprocessor(const char *baseDir)
{
	t = NULL;
	outtokens = new List<Token>();
	includePath.SetTo(baseDir);
	recursion = 0;
}

Preprocessor::~Preprocessor()
{
	macros.Clear();
	macrosHash.Clear();
	defines.Clear();
	definesHash.Clear();
	if (outtokens) outtokens->Delete();
	delete t;
}

/*
void c------------------------------() {}
*/

// use the given list of tokens to emit tokens to the output.
// output tokens are copies of the ones in the tokenizer.
// the input tokens become owned by the preprocessor.
bool Preprocessor::ProcessTokens(List<Token> *intokens)
{
	Tokenizer *tempTokenizer = new Tokenizer(intokens);
	Tokenizer *oldTokenizer = t;
	
	bool result = ProcessTokens(tempTokenizer);
	
	delete tempTokenizer;
	t = oldTokenizer;
	
	return result;
}

// use the given tokenizer to emit tokens to the output
// output tokens are copies of the ones in the tokenizer.
// the tokenizer becomes owned by the preprocessor.
bool Preprocessor::ProcessTokens(Tokenizer *_t)
{
KeyValuePair *kv;

	this->t = _t;
	//stat("Preprocessor: ProcessTokens(): parsing %d tokens", t->CountTokens());
	
	for(;;)
	{
		Token *tkn = t->next_token();
		if (tkn->type == TKN_EOF) break;
		
		dstat("recursion=%d: pp read '%s'", recursion, tkn->Describe());
		
		if (tkn->type == TKN_MACRO)
		{
			if (read_macro())
				return 1;
		}
		else if (tkn->type == TKN_INVOKE_MACRO)
		{
			if (invoke_macro())
				return 1;
		}
		else if (tkn->type == TKN_INCLUDE)
		{
			if (handle_include())
				return 1;
		}
		else if (tkn->type == TKN_ENUM)
		{
			if (read_enum())
				return 1;
		}
		else if (tkn->type == TKN_WORD && !strcasecmp(tkn->text, "define"))
		{
			if (read_define())
				return 1;
		}
		else if (tkn->type == TKN_WORD && definesHash.HasKey(tkn->text))
		{
			dstat("encountered define '%s'", tkn->text);
			
			recursion++;
			Define *def = (Define *)definesHash.LookupPtr(tkn->text);
			bool result = ProcessTokens(DuplicateTokenList(&def->tokens));
			recursion--;
			
			if (result)
				return 1;
		}
		else if (tkn->type == TKN_WORD && (kv = globalEnums.LookupPair(tkn->text)))
		{
			tkn = new Token(TKN_NUMBER, kv->value);
			tkn->text = strdup(stprintf("%d", tkn->value));
			outtokens->AddItem(tkn);
		}
		else
		{
			outtokens->AddItem(tkn->Duplicate());
		}
	}
	
	return 0;
}

/*
void c------------------------------() {}
*/

bool Preprocessor::read_macro(void)
{
	// read name
	Token *tkn = t->next_token();
	if (!t->expect_token(tkn, TKN_WORD))
		return 1;
	
	// check if name already in use
	if (macrosHash.LookupValue(tkn->text) >= 0)
	{
		staterr("Multiple definition of macro '%s'", tkn->text);
		return 1;
	}
	
	stat("%sDefining macro '%s'.", indent(), tkn->text);
	Macro *macro = new Macro(tkn->text);
	
	// read in list of variables
	dstat("Reading macro variable list");
	for(;;)
	{
		tkn = t->next_token();
		dstat("read_macro: %s", tkn->Describe());
		
		if (tkn->type == TKN_EOL) break;
		else if (tkn->type == TKN_EOF)
		{
			staterr("unexpected EOF reading definition for macro '%s'", macro->name);
			delete macro;
			return 1;
		}
		else if (tkn->type == TKN_VARIABLE || tkn->type == TKN_WORD)
		{
			macro->variables.AddValue(tkn->text, macro->variables.CountItems());
			dstat("Macro argument %d = '%s'", \
				 macro->variables.ItemAt(macro->variables.CountItems() - 1)->value,
				 macro->variables.ItemAt(macro->variables.CountItems() - 1)->keyname);
			
			Token *comma = t->next_token();
			if (comma->type == TKN_EOL) break;
			if (comma->type != TKN_COMMA)
			{
				staterr("expected: ',' or EOL while parsing args for macro '%s'; got '%s'",
					macro->name, comma->Describe());
				return 1;
			}
		}
		else
		{
			staterr("unexpected token '%s' reading definition for macro '%s'", tkn->Describe(), macro->name);
			delete macro;
			return 1;
		}
	}
	
	// read in macro source
	dstat("Reading macro source for '%s'...", macro->name);
	for(;;)
	{
		tkn = t->next_token();
		dstat("read_macro source: %s", tkn->Describe());
		
		if (tkn->type == TKN_ENDM) break;
		else if (tkn->type == TKN_EOF)
		{
			staterr("unexpected EOF reading definition for macro '%s'", macro->name);
			delete macro;
			return 1;
		}
		
		macro->tokens.AddItem(tkn->Duplicate());
	}
	
	// add to available macros
	macrosHash.AddValue(macro->name, macros.CountItems());
	macros.AddItem(macro);
	
	return 0;
}

bool Preprocessor::invoke_macro()
{
	// read name of macro we're invoking
	Token *tkn = t->next_token();
	if (!t->expect_token(tkn, TKN_WORD))
		return 1;
	
	Macro *macro = macros.ItemAt(macrosHash.LookupValue(tkn->text));
	if (!macro)
	{
		staterr("undefined macro '%s'", tkn->text);
		return 1;
	}
	
	stat("%sExpanding macro '%s'.", indent(), macro->name);
	int nargs = macro->variables.CountItems();
	//stat("macro '%s' found; %d arguments expected", macro->name, nargs);
	
	// read in the values of the macro arguments
	//stat("Reading macro variable values...");
	List<Token> varValues;
	for(;;)
	{
		tkn = t->next_token();
		//stat("varvalue: %s", tkn->Describe());
		
		if (tkn->type == TKN_EOL) break;
		else if (tkn->type == TKN_EOF)
		{
			staterr("unexpected EOF reading arguments for invocation of macro '%s'", macro->name);
			return 1;
		}
		
		// check if it's a valid argument to be passed to a macro.
		// fixme: this check could probably be improved a bit.
		if (tkn->type != TKN_COMMA)
		{
			varValues.AddItem(tkn);
			//stat("Argument %d = '%s'", varValues.CountItems() - 1, tkn->Describe());
			
			Token *comma = t->next_token();
			if (comma->type == TKN_EOL) break;
			if (comma->type != TKN_COMMA)
			{
				staterr("expected: ',' or EOL while parsing args for invocation of macro '%s'; got '%s'",
					macro->name, comma->Describe());
				return 1;
			}
		}
		else
		{
			staterr("unexpected token '%s' reading invocation of macro '%s'", tkn->Describe(), macro->name);
			return 1;
		}
	}
	
	if (varValues.CountItems() != nargs)
	{
		staterr("incorrect number of arguments to macro '%s': expected %d, got %d",
			macro->name, varValues.CountItems(), nargs);
		return 1;
	}
	
	// iterate over the macros tokens and create a list of new tokens which we will recursively
	// pass to ProcessTokens, replacing any variables with their values
	//stat("Expanding macro...");
	List<Token> *newTokens = new List<Token>();
	for(int i=0;;i++)
	{
		Token *tkn = macro->tokens.ItemAt(i);
		if (!tkn) break;
		
		// replace variables with their values in varValues above
		if (tkn->type == TKN_VARIABLE)
		{
			Token *varValue = varValues.ItemAt(macro->variables.LookupValue(tkn->text));
			if (!varValue)
			{
				staterr("Macro expansion failure: variable '%s' not defined", tkn->text);
				return 1;
			}
			
			//stat("Variable '%s' = '%s'", tkn->text, varValue);
			tkn = varValue;
		}
		
		//stat("Iterate/Convert: %s", tkn->Describe());
		newTokens->AddItem(tkn->Duplicate());
	}
	
	//staterr("== Recursively invoking macro %s", macro->name);
	recursion++;
	
	if (ProcessTokens(newTokens))
		return 1;
		
	recursion--;
	//staterr("== Returned from invocation of macro %s", macro->name);
	
	return 0;
}

/*
void c------------------------------() {}
*/

bool Preprocessor::handle_include()
{
	// read name
	Token *tkn = t->next_token();
	if (!t->expect_token(tkn, TKN_WORD))
		return 1;
	
	DString fullfname;
	fullfname.SetTo(includePath);
	path_combine(&fullfname, tkn->text);
	
	stat("Include: '%s'", fullfname.String());
	
	recursion++;
	DString saveIncludePath(includePath);
	Tokenizer *saveT = t;
	includePath.SetTo(GetFilePath(fullfname.String()));
	{
		dstat("%sfullfname: '%s'", indent(), fullfname.String());
		dstat("%sincludePath: '%s'", indent(), includePath.String());
		
		Tokenizer *subT = new Tokenizer();
		if (subT->Tokenize_File(fullfname.String()))
			return 1;
		
		if (ProcessTokens(subT))
			return 1;
		
		delete subT;
	}
	t = saveT;
	includePath.SetTo(saveIncludePath);
	recursion--;
	
	return 0;
}

/*
void c------------------------------() {}
*/

bool Preprocessor::read_enum()
{
	stat("Reading values of enum statement into global scope.");
	
	const char *enumName = NULL;
	if (t->peek_next_token()->type == TKN_WORD)
		enumName = t->next_token()->text;
	
	if (!t->expect_token(t->next_token_except_eol(), TKN_OPEN_CURLY_BRACE))
		return 1;
	
	int value = 0;
	for(;;)
	{
		Token *tkn = t->next_token_except_eol();
		//staterr("read '%s'", tkn->Describe());
		
		if (tkn->type == TKN_WORD)
		{
			const char *entryName = tkn->text;
			if (enumName)
			{
				DString temp(enumName);
				temp.AppendString(">>", 2);
				temp.AppendString(tkn->text);
				entryName = temp.StaticString();
			}
			
			Token *next = t->next_token_except_eol();
			switch(next->type)
			{
				case TKN_EQUALS:
				{
					Token *entryValue = t->next_token();
					if (entryValue->type == TKN_NUMBER)
						value = entryValue->value;
					else
					{
						staterr("Expected number after '=' while processing enum name %s ( got '%s' )",
							entryName,
							entryValue->Describe());
						return 1;
					}
				}
				break;
				
				case TKN_COMMA:
				case TKN_CLOSE_CURLY_BRACE:
					t->back_token();	// let it be processed below
				break;
				
				default:
				{
					staterr("Expected '=' or ',' after enum name '%s' ( got '%s' )", entryName, next->Describe());
					return 1;
				}
				break;
			}
			
			// add the enum name and value to the hashtable
			if (globalEnums.LookupPair(entryName))
			{
				stat("name '%s' used multiple times in global enums", entryName);
				return 1;
			}
			
			stat("adding global enum '%s' = %d", entryName, value);
			globalEnums.AddValue(entryName, value);
			value++;
			
			// check for comma
			Token *comma = t->next_token_except_eol();
			switch(comma->type)
			{
				case TKN_COMMA: break;
				
				case TKN_CLOSE_CURLY_BRACE:
					t->back_token();		// let it be processed by main loop in next iteration
				break;
				
				default:
				{
					staterr("expected: ',' or '}' after enum name %s ( got '%s' )",
						entryName,
						comma->Describe());
					return 1;
				}
				break;
			}
		}
		else if (tkn->type == TKN_CLOSE_CURLY_BRACE)
		{
			stat("Enum closed.");
			break;
		}
		else
		{
			staterr("expected: word or '}' while processing enums on line %d (got %s)",
				tkn->line, tkn->Describe());
			return 1;
		}
	}
	
	return 0;
}

bool Preprocessor::read_define()
{
	const char *defineName;
	if (t->peek_next_token()->type == TKN_WORD)
		defineName = t->next_token()->text;
	else
	{
		staterr("expected: word after 'define', got: %s", t->peek_next_token()->Describe());
		return 1;
	}
	
	// check if define already exists
	if (definesHash.HasKey(defineName))
	{
		staterr("Redefinition of '%s'", defineName);
		return 1;
	}
	
	Define *def = new Define(defineName);
	for(;;)
	{
		Token *tkn = t->next_token();
		if (tkn->type == TKN_EOL || tkn->type == TKN_EOF) break;
		
		def->tokens.AddItem(tkn->Duplicate());
	}
	
	if (def->tokens.CountItems() == 0)
	{
		staterr("Define '%s' defined to nothing", def->name);
		delete def;
		return 1;
	}
	
	defines.AddItem(def);
	definesHash.AddPtr(def->name, def);
	
	stat("Defined '%s' = '%s'", def->name, dump_token_list(&def->tokens));
	return 0;
}

/*
void c------------------------------() {}
*/

List<Token> *Preprocessor::GetTokens()
{
	List<Token> *out = outtokens;
	outtokens = NULL;
	return out;
}

Tokenizer *Preprocessor::GetTokenizer()
{
	List<Token> *tokens = GetTokens();
	return tokens ? new Tokenizer(tokens) : NULL;
}

const char *Preprocessor::indent()
{
static DString spaces;

	int desiredLength = (recursion * 2);
	if (spaces.Length() != desiredLength)
	{
		if (spaces.Length() > desiredLength)
			spaces.Clear();
		
		while(spaces.Length() < desiredLength)
			spaces.AppendString("  ");
	}
	
	return spaces.StaticString();
}

/*
void c------------------------------() {}
*/

Macro::Macro(const char *_name)
{
	name = strdup(_name);
}

Macro::~Macro()
{
	if (name) free(name);
	variables.Clear();
	tokens.Clear();
}

/*
void c------------------------------() {}
*/

Tokenizer *Preprocess_File(const char *fname)
{
	Tokenizer *tzRaw = new Tokenizer();
	if (tzRaw->Tokenize_File(fname))
		return NULL;
	
	// run the preprocessor over the raw tokenized source
	stat("");
	stat("Preprocessing %s...", fname);
	Preprocessor *pp = new Preprocessor(GetFilePath(fname));
	if (pp->ProcessTokens(tzRaw))
	{
		staterr("Preprocessor failed.");
		return NULL;
	}
	
	delete tzRaw;
	
	Tokenizer *t = pp->GetTokenizer();
	if (!t)
	{
		staterr("Failed to get output from preprocessor.");
		return NULL;
	}

	return t;
}
