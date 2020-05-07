
#ifndef _PP_H
#define _PP_H

class Tokenizer;

namespace PP
{
	class Macro
	{
	public:
		Macro(const char *_name);
		~Macro();
		
		char *name;				// name used to invoke the macro
		HashTable variables;	// map of variable names to their index (order they are in when macro is called)
		List<Token> tokens;		// tokens making up the macro
	};
	
	class Define
	{
	public:
		Define(const char *_name) { name = strdup(_name); }
		~Define() { free(name); tokens.Clear(); }
		
		char *name;
		List<Token> tokens;
	};
}

class Preprocessor
{
public:
	Preprocessor(const char *includeBaseDir);
	~Preprocessor();
	
	//inline void SetIncludePath(const char *newPath) { includePath.SetTo(baseDir); }
	
	bool ProcessTokens(Tokenizer *_t);
	bool ProcessTokens(List<Token> *intokens);	// process the given list of tokens and emit to output
	List<Token> *GetTokens();
	Tokenizer *GetTokenizer();

private:
	bool read_macro();
	bool invoke_macro();	// read in macro name and arguments from current pos and invoke the macro recursively
	
	bool handle_include();
	bool read_enum();
	
	bool read_define();
	
	const char *indent();
	
private:
	Tokenizer *t;
	
	List<PP::Macro> macros;
	HashTable macrosHash;
	
	List<PP::Define> defines;
	HashTable definesHash;
	
	HashTable globalEnums;
	
	DString includePath;
	
	List<Token> *outtokens;
	int recursion;
};

Tokenizer *Preprocess_File(const char *fname);


#endif
