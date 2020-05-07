
#include "main.h"
#include "assembler.fdh"

Assembler::Assembler()
{
	t = NULL;
	runfrom_delta = 0;
	asmcore = NULL;
}

Assembler::~Assembler()
{
	labels.Clear();
	deferredLines.Clear();
	
	delete t; t = NULL;
	delete asmcore; asmcore = NULL;
}

/*
void c------------------------------() {}
*/

bool Assembler::Assemble_File(const char *fname)
{
	stat("Assemble_File: '%s'", fname);
	
	// compute base path of the file we're compiling for io defs and includes
	DString basePath(GetFilePath(fname));
	stat("Assembling in base path '%s'", basePath.String());
	
	// initialize the assembler core class
	asmcore = new ASMCore();
	if (asmcore->LoadTable("asmtable.dat"))
	{
		stat("LoadTable failed");
		delete asmcore;
		return 1;
	}
	
	// initialize the tokenizer and preprocess+tokenize the file
	stat("");
	stat("Tokenizing file %s...", fname);
	t = Preprocess_File(fname);
	if (!t) return 1;
	
	/*for(int i=0;;i++)
	{
		Token *tkn = t->next_token();
		stat("%d: '%s'", i, tkn->Describe());
		if (tkn->type == TKN_EOF) exit(2);
	}*/
	
	stat("Preprocessing complete: %d tokens.", t->CountItems());
	stat("");
	
	if (DoAssembly())
		return 1;
	
	// get the save file name and save
	char *saveFileName = (char *)malloc(strlen(fname) + 8);
	strcpy(saveFileName, fname);
	
	char *ptr = strrchr(saveFileName, '.');
	if (ptr) *ptr = 0;
	strcat(saveFileName, ".bin");
	
	stat("");
	if (SaveOutput(saveFileName))
		return 1;
	
	return 0;
}

bool Assembler::DoAssembly()
{
	stat("Assembling file...");
	
	runfrom_delta = 0;
	output.Clear();
	
	stat("\n== PASS 1: Finding label names ==");
	if (FindLabelNames())
		return 1;
	
	stat("\n== PASS 2: Assembling source code ==");
	if (AssembleLines())
		return 1;
	
	stat("\n== PASS 3: Resolving deferred labels ==");
	stat("\e[1;35m%d deferred line(s) need to be reassembled.\e[0m", deferredLines.CountItems());
	if (deferredLines.CountItems())
	{
		// the easy way is to just erase the output and re-assemble the whole lot, since we now know
		// where all labels are, they should all resolve this time with no deferrences.
		// TODO: reassemble only the lines that need to be, as we designed.
		deferredLines.Clear();
		output.Clear();
		
		if (AssembleLines())
			return 1;
		
		if (deferredLines.CountItems())
		{
			staterr("Re-assembly did not eradicate all deferred lines (%d left)", deferredLines.CountItems());
			return 1;
		}
		else
		{
			stat("\e[1;32mDeferred labels successfully resolved.\e[0m");
		}
	}
	
	stat("\n== Final output ==");
	hexdump(output.Data(), output.Length());
	return 0;
}

bool Assembler::SaveOutput(const char *fname)
{
	stat("Saving %s...", fname);
	FILE *fp = fopen(fname, "wb");
	if (!fp)
	{
		staterr("failed to open output file %s", fname);
		return 1;
	}
	
	fwrite(output.Data(), output.Length(), 1, fp);
	fclose(fp);
	return 0;
}

/*
void c------------------------------() {}
*/

bool Assembler::FindLabelNames()
{
	t->rewind();
	currentScope.Clear();
	labels.Clear();
	
	List<Token> line;
	for(;;)
	{
		line.Clear();
		if (t->read_line(&line, true)) break;
		
		if (IsLabelDefinition(&line))
		{
			const char *labelName = line.ItemAt(0)->text;
			const char *labelScope;
			
			if (labelName[0] == '.')
			{
				labelScope = currentScope.String();
				if (labelScope[0] == 0)
					stat("Warning: local label '%s' is in global scope", labelName);
			}
			else
			{
				labelScope = "";
				currentScope.SetTo(labelName);
			}
			
			if (FindLabel(labelName, labelScope))
			{
				staterr("Duplicate label '%s'", showLabel(labelName, labelScope));
				return 1;
			}
			
			stat("FindLabelNames: found label '%s'", showLabel(labelName, labelScope));
			
			Label *lbl = new Label(labelName, labelScope);
			labels.AddItem(lbl);
		}
	}
	
	stat("Pass 1 complete: %d labels found.", labels.CountItems());
	line.Clear();
	return 0;
}

/*
void c------------------------------() {}
*/

bool Assembler::AssembleLines()
{
	t->rewind();
	currentScope.Clear();
	deferredLines.Clear();
	
	List<Token> line;
	for(;;)
	{
		line.Clear();
		if (t->read_line(&line, true)) break;
		stat(""); staterr("Read line: [ %s; ]", dump_token_list(&line, "; "));
		if (line.CountItems() == 0) continue;	// just to be sure
		
		Token *firstToken = line.ItemAt(0);
		
		if (IsLabelDefinition(&line))
		{
			if (HandleLabel(firstToken->text))
				return 1;
		}
		else if (firstToken->type == TKN_WORD)
		{
			// go through the tokens and "defer" any TKN_WORDs which are labels by replacing them
			// with a 16-bit dummy number, then add them to the list of lines to be reassembled
			// at the end once we have found the location of all labels.
			int startingPos = GetRunfromPos();
			List<DeferredLine> deferred;
			List<Token> *newLine = DeferLabels(&line, &deferred);
			if (!newLine) return 1;
			
			bool error = false;
			if (firstToken->text[0] == '.')
			{
				if (HandleAssemblerDirective(newLine, (deferred.CountItems() != 0)))
					error = true;
			}
			else if (asmcore->IsValidMnemonic(firstToken->text))
			{
				if (AssembleLine(newLine))
					error = true;
			}
			else
			{
				staterr("Unknown mnemonic: '%s'", dump_token_list(&line));
				error = true;
			}
			
			// set the predictedLength on any deferred records that were created
			if (deferred.CountItems())
			{
				int insLength = GetRunfromPos() - startingPos;
				stat("Setting predictedLength %d for %d defer records", insLength, deferred.CountItems());
				
				for(int i=deferred.CountItems()-1;i>=0;i--)
					deferred.ItemAt(i)->predictedLength = insLength;
			}
			
			newLine->Delete();
			if (error) return 1;
		}
		else
		{
			staterr("Syntax error: '%s'", dump_token_list(&line));
			return 1;
		}
	}
	
	return 0;
}

bool Assembler::AssembleLine(List<Token> *line)
{
	stat("\e[1;31mAssembling [ %s; ] at 0x%02x (offset 0x%02x)\e[0m", \
		dump_token_list(line, "; "),
		GetRunfromPos(), GetFilePos());
	
	AssemblyResult *result = asmcore->Assemble(line);
	if (!result)
	{
		staterr("Assembly failed in asmcore of line '%s'", dump_token_list(line));
		return 1;
	}
	
	hexdump(result->bytes.Data(), result->bytes.Length());
	output.AppendData(result->bytes.Data(), result->bytes.Length());
	
	delete result;
	return 0;
}

bool Assembler::HandleAssemblerDirective(List<Token> *line, bool wasDeferred)
{
	const char *cmd = line->ItemAt(0)->text;
	
	if (!strcasecmp(cmd, ".filepos") || !strcasecmp(cmd, ".padto"))
	{
		// check for validity
		if (wasDeferred)
		{
			staterr("invalid to use labels as argument to %s directive", cmd);
			return 1;
		}
		else if (line->CountItems() != 2 || line->ItemAt(1)->type != TKN_NUMBER)
		{
			staterr("syntax error in %s directive '%s': correct usage: '%s [offset]'", cmd, dump_token_list(line), cmd);
			return 1;
		}
		
		// get the amount of padding we'll have to add
		int pos = line->ItemAt(1)->value;
		if (!strcasecmp(cmd, ".padto"))
			pos -= runfrom_delta;
		
		int paddingCount = pos - output.Length();
		if (paddingCount <= 0)
		{
			if (paddingCount == 0)
			{
				stat("%s: already at offset 0x%02x, directive has no effect", cmd, pos);
				return 0;
			}
			
			staterr("%s error: offset 0x%02x is already less than the length of file (0x%04x)",
				cmd, pos, output.Length());
			return 1;
		}
		
		stat("%s: advancing %d bytes to offset 0x%02x", cmd, paddingCount, pos);
		
		uint8_t *padding = (uint8_t *)malloc(paddingCount);
		memset(padding, 0xff, paddingCount);
		
		// advance the position to the given value
		output.AppendData(padding, paddingCount);
		free(padding);
	}
	else if (!strcasecmp(cmd, ".runfrom"))
	{
		// check for validity
		if (wasDeferred)
		{
			staterr("invalid to use labels as argument to .runfrom directive");
			return 1;
		}
		else if (line->CountItems() != 2 || line->ItemAt(1)->type != TKN_NUMBER)
		{
			staterr("syntax error in .runfrom directive '%s': correct usage: '.runat [address]'", dump_token_list(line));
			return 1;
		}
		
		// calculate the new runfrom_delta
		int pos = line->ItemAt(1)->value;
		runfrom_delta = pos - output.Length();
		
		stat(".runfrom set to 0x%02x (runfrom_delta = %s)", pos, neghex(runfrom_delta));
	}
	else if (!strcasecmp(cmd, ".db") || !strcasecmp(cmd, ".d8"))	// byte
	{
		if (HandleDB(line, 1))
			return 1;
	}
	else if (!strcasecmp(cmd, ".dw") || !strcasecmp(cmd, ".d16"))	// word
	{
		if (HandleDB(line, 2))
			return 1;
	}
	else if (/*!strcasecmp(cmd, ".da") ||*/ !strcasecmp(cmd, ".d24"))	// 24-bit address
	{
		if (HandleDB(line, 3))
			return 1;
	}
	else if (!strcasecmp(cmd, ".dl") || !strcasecmp(cmd, ".d32"))	// 32-bit long
	{
		if (HandleDB(line, 4))
			return 1;
	}
	else
	{
		staterr("unknown assembler directive '%s'", cmd);
		return 1;
	}
	
	return 0;
}

// handle ".db", ".dw", etc assembler commands
bool Assembler::HandleDB(List<Token> *line, int wordLength)
{
	staterr("STUB wordLength=%d", wordLength);
	return 1;
}

bool Assembler::HandleLabel(const char *labelName)
{
	if (labelName[0] != '.')	// look up global labels in global scope
		currentScope.Clear();
	
	// lookup the label and set it's address and offset, following label scoping
	Label *lbl = FindLabel(labelName, currentScope.String());
	if (!lbl)
	{	// this shouldn't happen
		staterr("internal error: label '%s' wasn't found in first pass", labelName);
		return 1;
	}
	
	lbl->address = GetRunfromPos();
	lbl->offset = GetFilePos();
	stat("Set label %s", lbl->Describe());
	
	if (labelName[0] != '.')
		currentScope.SetTo(labelName);
	
	return 0;
}

// go through the tokens and "defer" any TKN_WORDs which are labels by replacing them
// with a 16-bit dummy number, then add them to the list of lines to be reassembled
// at the end once we have found the location of all labels.
// OR, if the label address is already known, replace it with the label address
// and DON'T mark it for reassembly.
//
// line: original source of line
// deferred_out: returns a list of any DeferredLines that were added to deferredLines list, so caller can set predictedLength
// returns: a new line, with any labels either resolved or replaced with dummy numbers
List<Token> *Assembler::DeferLabels(List<Token> *line, List<DeferredLine> *deferred_out)
{
	staterr("DeferLabels in: '%s'", dump_token_list(line));
	auto *newline = new List<Token>();
	
	// iterate over tokens creating a new list.
	// we can skip the first token as it's either the assembler command or a mnemonic.
	for(int i=0;;i++)
	{
		Token *tkn = line->ItemAt(i);
		if (!tkn) break;
		
		if (i == 0 || tkn->type != TKN_WORD)
		{
			newline->AddItem(tkn->Duplicate());
			continue;
		}
		
		Label *lbl = FindLabel(tkn->text, (tkn->text[0] == '.') ? currentScope.String() : "");
		if (!lbl)
		{
			newline->AddItem(tkn->Duplicate());
			continue;
		}
		
		// if label address is known
		if (lbl->address >= 0)
		{
			Token *num = new Token(TKN_NUMBER);
			num->value = lbl->address;
			num->text = strdup(stprintf("0x%02x", num->value));
			newline->AddItem(num);
			
			staterr("Resolved label '%s' to 0x%02x", showLabel(lbl->name.String(), lbl->scope.String()), lbl->address);
		}
		else	// need to defer
		{
			staterr("Deferring label '%s'", showLabel(lbl->name.String(), lbl->scope.String()));
			
			// replace with dummy 16-bit value
			Token *num = new Token(TKN_NUMBER);
			num->value = 0xA5A5;
			num->text = strdup(stprintf("0x%02x", num->value));
			newline->AddItem(num);
			
			// add a defer record for this line so we can come back to it in next pass
			// ...unless we already have one (happens if the line contains more than one label)
			int offset = GetFilePos();
			if (!HaveDeferredLineForOffset(offset))
			{
				auto *rec = new DeferredLine();
				rec->line = DuplicateTokenList(line);
				rec->offset = offset;
				rec->predictedLength = -1;	// will be filled in after assembly
				
				deferredLines.AddItem(rec);
				deferred_out->AddItem(rec);
				
				stat("Added defer rec for offset 0x%02x", offset);
			}
			else
			{
				stat("Already have a defer rec for offset 0x%02x, not adding another", offset);
			}
		}
	}
	
	staterr("DeferLabels out: '%s'", dump_token_list(newline));
	return newline;
}

bool Assembler::HaveDeferredLineForOffset(int offset)
{
	for(int i=0;;i++)
	{
		auto *rec = deferredLines.ItemAt(i);
		if (!rec) break;
		
		if (rec->offset == offset)
			return true;
	}
	
	return false;
}

/*
void c------------------------------() {}
*/

bool Assembler::IsLabelDefinition(List<Token> *line)
{
	if (line->CountItems() == 2 && \
		line->ItemAt(1)->type == TKN_COLON && \
		line->ItemAt(0)->type == TKN_WORD)
	{
		return true;
	}
	
	return false;
}

Label *Assembler::FindLabel(const char *name, const char *scope)
{
static int startIndex = 0;

	// iterate through all labels and try to find the one requested.
	// but, we'll start by checking the label just after the last one that was looked up.
	// this is an optimization since it's likely the labels will be asked for in order
	// (during assembly).
	
	int numLabels = labels.CountItems();
	if (numLabels == 0)
		return NULL;
	
	int index = startIndex;
	for(;;)
	{
		Label *lbl = labels.ItemAt(index);
		//stat(" * checking label '%p' at index %d '%s'", \
			//lbl, index, (lbl != NULL) ? lbl->name.String() : "null");
		
		if (++index >= numLabels)
			index = 0;
		
		//stat("  * Check name='%s', scope='%s' against %s", name, scope, lbl->Describe());
		if (!strcasecmp(lbl->name.String(), name) && \
			!strcasecmp(lbl->scope.String(), scope))
		{
			startIndex = index;
			return lbl;
		}
		
		if (index == startIndex)
			break;
	}
	
	return NULL;
}

static const char *showLabel(const char *name, const char *scope)
{
	return stprintf("%s%s%s", \
		scope, (scope[0] ? "::" : ""), name);
}

const char *Label::Describe()
{
	if (address < 0)
	{
		return stprintf("< '%s': address unknown >", \
			showLabel(this->name.String(), this->scope.String()));
	}
	
	return stprintf("< '%s': address = 0x%02x, offset = 0x%02x >", \
		showLabel(this->name.String(), this->scope.String()),
		this->address,
		this->offset);
}

static const char *neghex(int value)
{
	if (value < 0)
		return stprintf("-0x%02x", -value);
	else
		return stprintf("0x%02x", value);
}
