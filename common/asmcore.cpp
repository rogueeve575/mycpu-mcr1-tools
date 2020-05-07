
//#define DEBUG
#include "common.h"
#include "asmcore.h"
#include "asmcore.fdh"

#define TABLE_VERSION		1

ASMCore::ASMCore()
{
	memset(&opcode_to_insdef, 0, sizeof(opcode_to_insdef));
}

ASMCore::~ASMCore()
{
	opcodes.Clear();
	memset(&opcode_to_insdef, 0, sizeof(opcode_to_insdef));
}

/*
void c------------------------------() {}
*/

bool ASMCore::LoadTable(const char *fname)
{
	stat("== Loading assembler table '%s' ==", fname);
	
	FILE *fp = fopen(fname, "rb");
	if (!fp)
	{
		staterr("Failed to open file %s: %s", fname, strerror(errno));
		return 1;
	}
	
	// check signature
	int a = fgetc(fp);
	int b = fgetc(fp);
	if (a != 'A' || b != 'T')
	{
		staterr("invalid signature word");
		fclose(fp);
		return 1;
	}
	
	int version = fgetc(fp);
	if (version != TABLE_VERSION)
	{
		staterr("Unsupported version %d: expected version %d", version, TABLE_VERSION);
		fclose(fp);
		return 1;
	}
	
	bool done = false;
	while(!done)
	{
		bool error = false;
		
		// read in sections
		int sectionType = fgetc(fp);
		switch(sectionType)
		{
			case 'O':
				error = LoadOpcodeList(fp);
			break;
			
			case 'R':
				error = LoadParmValues(fp);
			break;
			
			case 'E':
				done = true;
			break;
			
			default:
				staterr("Unknown section type '%s'", printChar(sectionType));
				error = true;
			break;
		}
		
		if (error)
		{
			staterr("Error processing %s", fname);
			fclose(fp);
			return 1;
		}
	}
	
	fclose(fp);
	
	stat("== Assembler table loaded successfully ==");
	return 0;
}

bool ASMCore::LoadOpcodeList(FILE *fp)
{
	stat("Loading opcode mnemonics...");
	
	int numOpcodes = fgetc(fp);
	if (numOpcodes <= 0)
	{
		staterr("Invalid number of opcodes: %d", numOpcodes);
		return 1;
	}
	
	opcodes.Clear();
	memset(&opcode_to_insdef, 0, sizeof(opcode_to_insdef));
	
	for(int i=0;;i++)
	{
		int ch = fgetc(fp);
		if (ch == 'E') break;
		if (ch != ':')
		{
			staterr("expected: ':' or 'E' on index %d; got %s", i, printChar(ch));
			return 1;
		}
		else if (ch < 0)
		{
			staterr("unexpected end of file");
			return 1;
		}
		
		if (i > numOpcodes)
		{
			staterr("more opcodes in list than specified");
			return 1;
		}
		
		// read in the opcode info
		auto *ins = new InstructionDef();
		ins->opcodeNum = fgetc(fp);
		ins->usesParm = false;
		ins->length = 1;
		
		int numTokens = fgetc(fp);
		for(int j=0;j<numTokens;j++)
		{
			int type = fgetc(fp);
			if (type == 0xff) type = fgetl(fp);
			
			Token *tkn = new Token();
			tkn->type = type;
			
			if (type == TKN_NUMBER)
			{
				tkn->value = fgetl(fp);
				tkn->text = strdup(stprintf("%d", tkn->value));
			}
			else
			{
				DString text;
				freadstring(fp, &text);
				tkn->text = strdup(text.String());
			}
			
			if (tkn->type == TKN_DSTREG || tkn->type == TKN_SRCREG) ins->usesParm = true;
			if (tkn->type == TKN_PARM8) ins->length++;
			if (tkn->type == TKN_PARM16) ins->length += 2;
			
			ins->tokens.AddItem(tkn);
		}
		
		if (ins->usesParm) ins->length++;
		//stat(""); stat("   %s", ins->Describe());
		
		opcodes.AddItem(ins);
		opcode_to_insdef[ins->opcodeNum] = ins;
	}
	
	stat("Parsed %d opcode defs OK.", opcodes.CountItems());
	return 0;
}

bool ASMCore::LoadParmValues(FILE *fp)
{
DString regName;

	stat("Loading register names for dstreg/srcreg...");
	parmValues.Clear();
	
	int printCounter = 0;
	for(int i=0;;i++)
	{
		int ch = fgetc(fp);
		if (ch == 'E') break;
		if (ch != ':')
		{
			staterr("expected: ':' or 'E' on index %d; got %s", i, printChar(ch));
			return 1;
		}
		else if (ch < 0)
		{
			staterr("unexpected end of file");
			return 1;
		}
		
		int value = fgetc(fp);
		if (value >= 16)
		{
			staterr("Value out of range on index %d: got %d, max 16", i, value);
			return 1;
		}
		
		regName.Clear();
		freadstring(fp, &regName);
		
		parmValues.AddValue(regName.String(), value);
		
		// print it
		statnocr("  [ '%s' = %d ]", regName.String(), value);
		if (++printCounter >= 8)
		{
			printCounter = 0;
			stat("");
		}
	}
	
	if (printCounter)
		stat("");
	
	return 0;
}

/*
void c------------------------------() {}
*/

DisassemblyResult *ASMCore::Disassemble(const uint8_t *ins, uint8_t flags)
{
DisassemblyResult *result;

	if (*ins == 0xff)
		result = NULL;
	else
		result = _Disassemble(ins);
	
	if (!result && (flags & DASMFlags::IgnoreErrors))
	{	// return a fake ??? result
		
		result = new DisassemblyResult();
		result->insdef = NULL;
		result->hextext.SetTo(stprintf("%02x", *ins));
		result->dasmtext.SetTo("???");
		result->length = 1;
	}
	
	return result;
}

DisassemblyResult *ASMCore::_Disassemble(const uint8_t *ins)
{
	uint8_t opcode = *(ins++);
	InstructionDef *def = LookupOpcode(opcode);
	if (!def)
		return NULL;
	
	dstat("Definition: %s", def->Describe());
	DString hextext(stprintf("%02x", opcode));
	
	uint8_t parm = 0;
	if (def->usesParm)
	{
		parm = *(ins++);
		hextext.AppendString(stprintf("%02x", parm));
		if (def->length > 2) hextext.AppendChar(' ');
	}
	else
	{
		if (def->length > 1) hextext.AppendChar(' ');
	}
	
	// iterate over the tokens in the definition and decode the instruction into text
	DString str;
	for(int i=0;;i++)
	{
		Token *tkn = def->tokens.ItemAt(i);
		if (!tkn) break;
		
		dstat("'%s'", tkn->Describe());
		
		switch(tkn->type)
		{
			case TKN_PARM8:
			{
				int value = *(ins++);
				str.AppendString(stprintf("0x%02x", value));
				hextext.AppendString(stprintf("%02x", value));
			}
			break;
			
			case TKN_PARM16:
			{
				int lsb = *(ins++);
				int msb = *(ins++);
				str.AppendString(stprintf("0x%04x", (msb << 8) | lsb));
				hextext.AppendString(stprintf("%02x%02x", lsb, msb));
			}
			break;
			
			case TKN_DSTREG:
			case TKN_SRCREG:
			{
				int index = (tkn->type == TKN_DSTREG) ? ((parm >> 4 & 0x0f)) : (parm & 0x0f);
				const char *regName = parmValues.ValueToKeyName(index);
				if (!regName)
					regName = stprintf("?%d", index);
				
				DString regNameLower(regName);
				regNameLower.ToLowerCase();
				
				str.AppendString(&regNameLower);
			}
			break;
			
			case TKN_COMMA:
			{
				str.AppendString(", ");		// space after comma
			}
			break;
			
			default:
			{
				str.AppendString(tkn->text);
				
				if (i == 0 && def->tokens.CountItems() > 1)
				{	// tab after mnemonic
					int padding = 7 - str.Length();
					do { str.AppendChar(' '); } while(--padding > 0);	// ensure at least 1 space is output
				}
			}
			break;
		}
		
		dstat("str = '%s'", str.String());
		dstat("");
	}
	
	// return the results
	auto *result = new DisassemblyResult();
	result->insdef = def;
	result->length = def->length;
	result->dasmtext.SetTo(&str);
	result->hextext.SetTo(&hextext);
	
	return result;
}

// quick disassembly to a string. returns length of opcode or -1 on error.
int ASMCore::Disassemble(const uint8_t *ins, DString *out, uint8_t flags)
{
	DisassemblyResult *result = Disassemble(ins, flags);
	if (!result)
		return -1;
	
	int length = result->length;
	if (FormatDisassemblyResult(result, out, flags))
		length = -1;	// return an error
	
	delete result;
	return length;
}

bool ASMCore::FormatDisassemblyResult(DisassemblyResult *result, DString *out, uint8_t flags)
{
	if (flags & DASMFlags::IncludeAddress)
		out->AppendString(stprintf("%04x  ", result->address));
	
	if (flags & DASMFlags::IncludeHex)
	{
		out->AppendString(&result->hextext);
		int padding = 10 - result->hextext.Length();
		do { out->AppendChar(' '); } while(--padding > 0);	// ensure at least 1 space is output
	}
	
	out->AppendString(&result->dasmtext);
	
	return 0;
}

/*
void c------------------------------() {}
*/

AssemblyResult *ASMCore::Assemble(const char *line, int lineLength)
{
	Tokenizer t;
	if (t.Tokenize_String(line, lineLength))	// automatically strlen()s if linelength is -1
		return NULL;
	
	List<Token> list;
	for(;;)
	{
		Token *tkn = t.next_token();
		if (tkn->type == TKN_EOL || tkn->type == TKN_EOF)
			break;
		
		list.AddItem(tkn);
	}
	
	return Assemble(&list);
}

AssemblyResult *ASMCore::Assemble(List<Token> *tokens)
{
	staterr("Assembling: '%s'", describe_instruction(tokens));
	
	// try to find the best matching instruction
	InstructionDef *def = MatchInstruction(tokens);
	if (!def)
	{
		staterr("No match found for given instruction '%s'", describe_instruction(tokens));
		return NULL;
	}
	
	#if 1
		stat("Matched input with def: %s", def->Describe());
		stat("input: '%s'", describe_instruction(tokens));
		stat("  def: '%s'", describe_instruction(&def->tokens));
	#endif
	
	// they are already true if instruction matched, but sanity check just to be sure
	if (def->tokens.CountItems() != tokens->CountItems())
	{
		staterr("internal error: matched definition has differing number of tokens than input");
		stat("input: '%s'", describe_instruction(tokens));
		stat("  def: '%s'", describe_instruction(&def->tokens));
		return NULL;
	}
	
	// go over the instruction definition and the input tokens
	// and fill in the parm byte and argument bytes from the input tokens
	// as we find their placeholders in the definition.
	uint8_t opcode = def->opcodeNum;
	uint8_t parm = 0xff;	// so that unused bits (if only one of srcreg or dstreg is used, are 1's for better EPROM/flash storage)
	DBuffer extraBytes;
	
	for(int i=1;;i++)	// start at 1 to skip mnemonic
	{
		Token *defTkn = def->tokens.ItemAt(i);
		Token *inTkn = tokens->ItemAt(i);
		if (!defTkn) break;
		
		switch(defTkn->type)
		{
			case TKN_PARM8:
			{
				// matcher has already verified that inTkn will be a TKN_NUMBER and is within range,
				// so we don't need to clutter our code checking again.
				extraBytes.Append8(inTkn->value);
			}
			break;
			
			case TKN_PARM16:
			{
				extraBytes.Append8(inTkn->value & 0xff);
				extraBytes.Append8((inTkn->value >> 8) & 0xff);
			}
			break;
			
			case TKN_DSTREG:
			{
				int value = parmValues.LookupValue(inTkn->text);
				if (value < 0) { staterr("dstreg lookup error: '%s'", inTkn->text); return NULL; }
				
				parm &= 0x0f;
				parm |= (value << 4) & 0xf0;
			}
			break;
			
			case TKN_SRCREG:
			{
				int value = parmValues.LookupValue(inTkn->text);
				if (value < 0) { staterr("srcreg lookup error: '%s'", inTkn->text); return NULL; }
				
				parm &= 0xf0;
				parm |= (value & 0x0f);
			}
			break;
		}
	}
	
	#ifdef DEBUG
		stat("opcode: %02x", opcode);
		if (def->usesParm) stat("parm: %02x", parm);
		statnocr("extrabytes: ");
		for(int i=0;i<extraBytes.Length();i++) statnocr("%02x ", extraBytes.Data()[i]);
		stat("");
	#endif
	
	// build up the final machine code
	DBuffer finalBytes;
	finalBytes.Append8(opcode);
	if (def->usesParm) finalBytes.Append8(parm);
	finalBytes.AppendData(&extraBytes);
	
	// prepare and return the result
	auto *result = new AssemblyResult();
	result->insdef = def;
	result->bytes.SetTo(&finalBytes);
	
	return result;
}

// given a list of tokens, return an InstructionDef record which best matches those tokens.
InstructionDef *ASMCore::MatchInstruction(List<Token> *tokens)
{
	// loop over all InstructionDef's in opcodes list, and try to find a match
	// to the input tokens. return the match whose instruction is shortest.
	// if no match is found, returns NULL.
	InstructionDef *bestMatch = NULL;
	
	for(int i=0;;i++)
	{
		InstructionDef *def = opcodes.ItemAt(i);
		if (!def) break;
		
		//stat("Checking for match with %s", def->Describe());
		if (DoesInstructionMatch(tokens, &def->tokens))
		{
			staterr("Found potential: %s", def->Describe());
			
			if (!bestMatch || def->length < bestMatch->length)
				bestMatch = def;
		}
	}
	
	return bestMatch;
}

bool ASMCore::DoesInstructionMatch(List<Token> *candidates, List<Token> *deftokens)
{
	// if wrong number of tokens, reject it immediately
	if (candidates->CountItems() != deftokens->CountItems())
		return false;
	
	for(int i=0;;i++)
	{
		Token *tcnd = candidates->ItemAt(i);
		if (!tcnd) return true;
		Token *tdef = deftokens->ItemAt(i);
		
		if (!CanTokensMatch(tcnd, tdef))
			return false;
	}
}

// returns true if the token "candidate", taken from the input assembly code,
// could potentially be a match for "deftoken", taken from the assembler definitions table.
// handles things such as knowing that "0x01FF" can match a TKN_PARM16 (but is too long for a TKN_PARM8),
// or that "r0" can match TKN_DSTREG.
bool ASMCore::CanTokensMatch(Token *candidate, Token *deftoken)
{
	//stat("CanTokensMatch: \e[1;31mcnd %s \e[0m\e[1m ==  \e[1;32mdef %s \e[0m\e[1m?", candidate->Describe(), deftoken->Describe());
	
	// dstreg/srcreg matching
	if (deftoken->type == TKN_DSTREG || deftoken->type == TKN_SRCREG)
	{
		if (candidate->type == TKN_WORD)
		{
			if (parmValues.HasKey(candidate->text))
				return true;
		}
		
		return false;
	}
	
	// parm8/parm16 matching
	if ((deftoken->type == TKN_PARM8 || deftoken->type == TKN_PARM16) && candidate->type == TKN_NUMBER)
	{
		int value = candidate->value;
		if (value <= 0xff) return true;
		if (value <= 0xffff && deftoken->type == TKN_PARM16) return true;
		return false;
	}
	
	// simple-case straight compare
	if (candidate->type == deftoken->type)
	{
		if (candidate->type == TKN_NUMBER)
		{
			if (candidate->value != deftoken->value)
				return false;
		}
		else if (candidate->type == TKN_WORD)
		{
			if (strcasecmp(candidate->text, deftoken->text) != 0)
				return false;
		}
		
		return true;
	}
	
	return false;
}

bool ASMCore::IsValidMnemonic(const char *mnemonic)
{
	for(int i=0;;i++)
	{
		InstructionDef *def = opcodes.ItemAt(i);
		if (!def) break;
		
		Token *tkn = def->tokens.ItemAt(0);
		if (tkn && tkn->type == TKN_WORD)
		{
			if (!strcasecmp(tkn->text, mnemonic))
				return true;
		}
	}
	
	return false;
}

/*
void c------------------------------() {}
*/

const char *InstructionDef::Describe()
{
DString str;

	str.AppendString(stprintf("opcode 0x%02x ", opcodeNum));
	if (usesParm)
		str.AppendString("usesParm ");
	
	str.AppendString(stprintf("len %d: ", length));
	
	str.AppendString("{ ");
	for(int i=0;i<tokens.CountItems();i++)
	{
		if (i) str.AppendString(", ");
		str.AppendString(tokens.ItemAt(i)->Describe());
	}
	str.AppendString(" }");
	
	return str.StaticString();
}

static const char *describe_instruction(List<Token> *tokens)
{
	DString ins;
	for(int i=0;;i++)
	{
		Token *tkn = tokens->ItemAt(i);
		if (!tkn) break;
		
		DString text(tkn->text);
		(i == 0) ? text.ToUpperCase() : text.ToLowerCase();
		
		ins.AppendString(text.String());
		if (i == 0 || tkn->type == TKN_COMMA) ins.AppendChar(' ');
	}
	
	return ins.StaticString();
}

/*
void TestCore(void)
{
	auto *asmcore = new ASMCore();
	if (asmcore->LoadTable("asmtable.dat"))
	{
		stat("LoadTable failed");
		delete asmcore;
		return 1;
	}
	
	stat("");
	
	#if 1		// Disassembler test
	int offset = 0;
	while(offset < sizeof(testcode))
	{
		DString str;
		int length = asmcore->Disassemble(testcode + offset, &str, DASMFlags::IncludeHex | DASMFlags::IgnoreErrors);
		if (length < 0)
		{
			staterr("Disassembly error");
			break;
		}
		
		stat("%04x   %s;", offset, str.String());
		offset += length;
	}
	#else		// Assembler test
		const char *testString = "BNE 0x0003";
		//const char *testString = "\tLDI r4, 8";
		//const char *testString = "BLT 0x0007";
		AssemblyResult *result = asmcore->Assemble(testString);
		if (!result)
		{
			staterr("Assembly error");
			return 1;
		}
		
		stat("-- Assembly success:");
		stat("insdef: %s", (result->insdef != NULL) ? result->insdef->Describe() : "(null)");
		stat("bytes (%d)%s", result->bytes.Length(), (result->bytes.Length() > 0) ? ":" : "");
		if (result->bytes.Length())
			hexdump(result->bytes.Data(), result->bytes.Length());
	#endif
	
	delete asmcore;
	return 1;
}
*/
