
#include "main.h"
#include "compiler.fdh"
using namespace ALU;

static const char *show_romword(romword_t value, int num_binary_digits = 0);

Compiler::Compiler()
{
	t = NULL;
	inputs = NULL;
	outputs = NULL;
	rom = NULL;
	rom_metadata = NULL;
	
	Clear();
}

void Compiler::Clear()
{
	delete t; t = NULL;
	delete inputs; inputs = NULL;
	delete outputs; outputs = NULL;
	if (rom) { free(rom); rom = NULL; }
	if (rom_metadata) { free(rom_metadata); rom_metadata = NULL; }
	output_to_bitmask.Clear();
	
	opcodes.Clear();
	memset(opcode_lut, 0, sizeof(opcode_lut));
	
	next_opcode_num = 1;
	rom_size_bytes = 0;
	rom_size_words = 0;
	default_word = 0;
}

/*
void c------------------------------() {}
*/

bool Compiler::ParseFile(const char *fname)
{
	// compute base path of the file we're compiling for io defs and includes
	basePath.SetTo(GetFilePath(fname));
	stat("Compiling in base path '%s'", basePath.String());
	
	// initialize the tokenizer and tokenize the file
	stat("Tokenizing file %s...", fname);
	t = Preprocess_File(fname);
	if (!t) return 1;
	
	stat("Preprocessing complete.");
	
	#if 0
		stat("");
		stat("Tokens list after preprocessing:");
		for(int i=0;;i++)
		{
			Token *tkn = t->next_token();
			stat("%d. %s", i, tkn->Describe());
			if (tkn->type == TKN_EOF) break;
		}
		
		staterr("DEBUG STOP");
		return 1;
	#endif
	
	// read the tokens and find the input and output file definitions, which must be
	// at the start of the file. leave cursor past the definitions.
	stat(""); stat("Parsing source to load io defs.");
	if (load_io_defs())
		return 1;
	
	// compute the default word
	if (compute_default_word())
		return 1;
	
	// allocate the ROM and fill it with the default word
	rom_size_words = (1 << NUM_INPUT_PINS);
	rom_size_bytes = rom_size_words * sizeof(romword_t);
	rom = (romword_t *)malloc(rom_size_bytes);
	stat("Allocated ROM buffer of %d bytes (%d %d-bit words)", rom_size_bytes, rom_size_words, NUM_OUTPUT_PINS);
	
	stat("Filling ROM buffer with default word %s...", show_romword(default_word, 0));
	for(int i=0;i<rom_size_words;i++)
		rom[i] = default_word;
	
	// allocate space for the ROM metadata
	metadata_size_bytes = rom_size_words * sizeof(uint32_t);
	rom_metadata = (uint32_t *)malloc(metadata_size_bytes);
	memset(rom_metadata, 0xff, metadata_size_bytes);	// unused areas will show all controls
	stat("Allocated %d bytes for ROM metadata.", metadata_size_bytes);
	
	// compile source code, getting a raw list of microinstructions.
	// they will contain condition codes and bytecode, and will need to be placed into
	// the correct opcode and phase. Unspecified GOTOs are at their defaults and will
	// need to be automatically pointed to the next phase during propagation.
	stat("");
	stat("Compiling source code...");
	if (parse_source())
		return 1;
	
	stat("Compiled %d opcode%s.", opcodes.CountItems(), (opcodes.CountItems() == 1) ? "" : "s");
	stat("");
	
	// propagate the compiled microinstructions into the ROM
	stat("Propagating opcodes...");
	if (propagate_opcodes())
		return 1;
	
	// save the lookup table which will be used by the assembler and disassembler
	stat("");
	if (save_assembler_table())
		return 1;
	
	// save the completed ROM images
	stat("");
	if (save_rom())
		return 1;
	
	stat("");
	stat("Final stats:");
	int numOpcodes = opcodes.CountItems();
	int left = 256 - numOpcodes;
	stat(" Opcodes: %d (0x%02x)   Left: %d (0x%02x)", numOpcodes, numOpcodes, left, left);
	stat("");
	
	return 0;
}

bool Compiler::propagate_opcodes()
{
	// we'll loop over all possible combinations of conditional inputs.
	// for each one, we'll go over all our opcodes.
	// for each opcode, we'll loop over each line.
	// we'll assign a phase to each line, skipping lines that have a condition which doesn't match the current one.
	// we'll automatically add GOTO(phase+1) for all lines which didn't specify a GOTO in the source.
	// using the phase, opcode #, and conditional states we'll determine an address and write
	// the compiled microline's bytecode to the correct address in the ROM.
	
	// first, get a list of all inputs which are conditionals.
	List<ControlDef> conditionals;
	for(int i=0;;i++)
	{
		ControlDef *input = inputs->list.ItemAt(i);
		if (!input) break;
		
		if (input->HasFlag(FlagConditional))
			conditionals.AddItem(input);
	}
	
	// based on the number of conditionals determine a number to increment to.
	// each bit in the incrementing index refers to the corresponding index in the conditonals list.
	// this lets us loop over all possibilities.
	int numConditionals = conditionals.CountItems();
	int conditionalMax = (1 << numConditionals) - 1;
	
	stat("   %d conditional input(s) found.", numConditionals);
	stat("   conditionalMax = %02x", conditionalMax);
	
	//stat("rom_size_words = 0x%04x", rom_size_words);
	
	// loop over all opcodes
	for(int opIndex=0;;opIndex++)
	{
		MicroIns *ins = opcodes.ItemAt(opIndex);
		if (!ins) break;
		
		stat("Propagating opcode %s...", ins->Describe());
		
		// for this opcode, loop over all possible combinations of opcode flags
		for(int condVal=0;condVal<=conditionalMax;condVal++)
		{
			// loop over each line of the current opcode and create a list of
			// microlines that match the current condition and thus will be included.
			List<MicroLine> lines;
			for(int i=0;;i++)
			{
				MicroLine *line = ins->lines.ItemAt(i);
				if (!line) break;
				
				if (!line->condition)
				{
					lines.AddItem(line);
				}
				else
				{
					// put all conditional flags into the variables to be passed to the RPN expression parser.
					HashTable vars;
					vars.AddValue("Z", (condVal & SREG::Z) ? 1 : 0);
					vars.AddValue("C", (condVal & SREG::C) ? 1 : 0);
					vars.AddValue("V", (condVal & SREG::V) ? 1 : 0);
					vars.AddValue("N", (condVal & SREG::N) ? 1 : 0);
					
					// check if it meets the condition
					int result = rpn_parse(&line->condition->expr, &vars);
					if (result != 0)
						lines.AddItem(line);
				}
			}
			
			dstat("   Propagating opcode 0x%02x, condition %02x (%s): %d active microlines...",
				ins->opcodeNum, condVal, padto(describe_sreg(condVal), 3), lines.CountItems());
			
			// output the bytecode of each active line to the correct address.
			for(int phase=0;;phase++)
			{
				MicroLine *line = lines.ItemAt(phase);
				if (!line) break;
				
				romword_t address = 0;
				
				// put the opcode and phase of the current line into the address.
				address = ciOpcode->SetValue(address, ins->opcodeNum);
				address = ciPhase->SetValue(address, phase);
				
				// put the conditionals into the address.
				for(int i=0;i<numConditionals;i++)
				{
					ControlDef *condInput = conditionals.ItemAt(i);
					bool value = (condVal & (1 << i)) ? 1 : 0;
					
					address = condInput->SetValue(address, value);
					//stat("%s = %d", condInput->name, value);
				}
				
				// fetch the bytecode for the current line.
				// if the line's source didn't include a specific GOTO, add one.
				romword_t bytecode = line->bytecode;
				if (line->FindUsedOutput(coGoto) < 0)
				{
					//stat("no goto, adding phase+1 %d", phase+1);
					int nextPhase = (phase + 1);
					if (nextPhase > ciPhase->maxValue()) nextPhase = 0;
					
					bytecode = coGoto->SetValue(bytecode, nextPhase);
				}
				
				// write the bytecode to the computed address.
				//staterr("writing %06x to address 0x%04x", bytecode, address);
				rom[address] = bytecode;
				
				// save metadata on which outputs were actually specified in source.
				uint32_t whichOutputsUsed = 0;
				for(int i=0;;i++)
				{
					ControlDef *output = line->used_outputs.ItemAt(i);
					if (!output) break;
					
					int bitmask = output_to_bitmask.LookupValue(output->name);
					if (bitmask == -1)
					{
						staterr("failure to generate metadata: output '%s' is not in output_to_bitmask");
						return 1;
					}
					
					whichOutputsUsed |= (uint32_t)bitmask;
					//stat("control %s bitmask %08x newValue %08x", output->name, (uint32_t)bitmask, whichOutputsUsed);
				}
				
				rom_metadata[address] = whichOutputsUsed;
			}
		}
	}
	
	return 0;
}

bool Compiler::save_rom()
{
	int num_files = NUM_OUTPUT_PINS / 8;
	stat("Writing %d ROM images...", num_files);
	
	romword_t mask = 0xFF;
	int shift = 0;
	
	for(int fileNo=0;fileNo<num_files;fileNo++)
	{
		DString fname(basePath);
		path_combine(&fname, stprintf("mcrom%d.bin", fileNo+1));
		
		stat("Saving %s...", fname.String());
		
		FILE *fp = fopen(fname.String(), "wb");
		if (!fp)
		{
			staterr("unable to open output file %s", fname.String());
			return 1;
		}
		
		for(int address=0;address<rom_size_words;address++)
		{
			int ch = (rom[address] & mask) >> shift;
			/*if (rom[address] != default_word)
			{
				stat("address %04x file %d word %06x byte %02x",
					address, fileNo, rom[address], ch);
			}*/
			
			fputc(ch, fp);
		}
		
		fclose(fp);
		mask <<= 8;
		shift += 8;
	}
	
	// save metadata on which outputs were used
	DString fname(basePath);
	path_combine(&fname, "mc_metadata.dat");
	stat("Saving metadata %s...", fname.String());
	
	FILE *fp = fopen(fname.String(), "wb");
	if (!fp)
	{
		staterr("unable to open %s", fname.String());
		return 1;
	}
	
	fputc('K', fp);
	fwrite(rom_metadata, metadata_size_bytes, 1, fp);
	
	fputl(outputs->list.CountItems(), fp);
	for(int i=0;;i++)
	{
		ControlDef *output = outputs->list.ItemAt(i);
		if (!output) break;
		
		fwrite(output->name, strlen(output->name) + 1, 1, fp);
	}
	
	fclose(fp);
	
	stat("ROM save complete.");
	return 0;
}

bool Compiler::save_assembler_table(void)
{
	DString fname("../assembler/");
	path_combine(&fname, "asmtable.dat");
	
	stat("Saving assembler table: %s...", fname.String());
	
	if (opcodes.CountItems() > 255)
	{
		staterr("Too many opcodes! There can only up to 255, but there are %d.", opcodes.CountItems());
		return 1;
	}
	
	FILE *fp = fopen(fname.String(), "wb");
	if (!fp)
	{
		staterr("unable to open output file %s", fname.String());
		return 1;
	}
	
	// signature
	fputc('A', fp);
	fputc('T', fp);
	
	// version
	fputc(1, fp);
	
	// start of opcode definition section
	fputc('O', fp);
	
	// # of instructions to follow
	fputc(opcodes.CountItems(), fp);
	
	// main table:
	for(int i=0;;i++)
	{
		MicroIns *ins = opcodes.ItemAt(i);
		if (!ins) break;
		
		if (ins->written_as.CountItems() > 255)
		{
			staterr("Too many tokens in opcode: < %s >", ins->Describe());
			fclose(fp);
			return 1;
		}
		
		fputc(':', fp);								// start of opcode marker
		fputc(ins->opcodeNum, fp);					// opcode number
		fputc(ins->written_as.CountItems(), fp);	// number of mnemonic tokens to follow
		for(int j=0;;j++)
		{
			Token *tkn = ins->written_as.ItemAt(j);
			if (!tkn) break;
			
			if (tkn->type == TKN_PIN)
			{
				staterr("What is a %s doing in instruction <%s>?", tkn->Describe(), ins->Describe());
				fclose(fp);
				return 1;
			}
			
			if (tkn->type >= 0xFF)
			{
				fputc(0xff, fp);
				fputl(tkn->type, fp);
			}
			else
			{
				fputc(tkn->type, fp);
			}
			
			if (tkn->type == TKN_NUMBER)
				fputl(tkn->value, fp);
			else
				fwritestring(tkn->text, fp);
		}
	}
	
	fputc('E', fp);		// end of section
	
	// start of valid register names to replace parmdst/parmsrc in 2-byte opcodes,
	// and their associated values that should be put into the machine code.
	// there are 16 possible values but there may be unused "holes".
	int maxParmNames = 16;
	fputc('R', fp);
	
	ControlDef *idbus_src = outputs->LookupByName("IDBUS_SRC");
	if (!idbus_src)
	{
		staterr("unable to find IDBUS_SRC control to enumerate register names");
		return 1;
	}
	
	for(int i=0;i<maxParmNames;i++)
	{
		const char *regName = idbus_src->enumEntries.ValueToKeyName(i);
		//staterr("Writing regName %d = '%s'", i, regName);
		
		if (regName)
		{
			fputc(':', fp);
			fputc(i, fp);
			fwritestring(regName, fp);
		}
	}
	
	fputc('E', fp);		// end of section
	
	fputc('E', fp);		// end of file
	fclose(fp);
	
	stat("Assembler table written successfully.");
	return 0;
}

/*
void c------------------------------() {}
*/

bool Compiler::load_io_defs()
{
	delete inputs; inputs = NULL;
	delete outputs; outputs = NULL;
	
	while(!inputs || !outputs)
	{
		int lineNo = t->get_current_line();
		Token *tkn = t->next_token();
		//stat("%d. %s", lineNo, tkn->Describe());
		
		if (tkn->type == TKN_EOL) continue;
		
		if (tkn->type == TKN_INPUTS || tkn->type == TKN_OUTPUTS)
		{
			Token *tFile = t->next_token();
			//stat("found inputs or outputs def: %s", tFile->Describe());
			
			if (!t->expect_token(tFile, TKN_WORD))
				return 1;
			
			// get full path to file
			DString fname(basePath);
			path_combine(&fname, tFile->text);
			
			// set up whether we're doing input or output
			ControlDefList **list;
			int iotype;
			if (tkn->type == TKN_INPUTS)
			{
				list = &inputs;
				iotype = INPUT;
			}
			else
			{
				list = &outputs;
				iotype = OUTPUT;
			}
			
			if (*list != NULL)
			{
				staterr("%s file defined more than once", DescribeIOType(iotype));
				return 1;
			}
			
			// compile it
			stat("");
			
			ControlDefParser parser;
			*list = parser.ParseFile(fname.String(), iotype, (iotype == INPUT) ? NUM_INPUT_PINS : NUM_OUTPUT_PINS);
			if (!*list)
				return 1;
		}
		else if (tkn->type == TKN_EOL)
		{
			staterr("unexpected end of file");
			return 1;
		}
		else
		{
			staterr("unexpected token %s on line %d", tkn->Describe(), lineNo);
			
			stat("\tYour microcode file must begin with the 'inputs' and 'outputs' definitions");
			stat("\tbefore any other statements.");
			
			return 1;
		}
	}
	
	// do some final checks and processing
	if (inputs->list.CountItems() == 0)
	{
		staterr("no inputs defined.");
		return 1;
	}
	else if (outputs->list.CountItems() == 0)
	{
		staterr("no outputs defined.");
		return 1;
	}
	
	ciPhase = inputs->LookupByName("PHASE");
	if (!ciPhase)
	{
		staterr("unable to find special input PHASE");
		return 1;
	}
	
	ciOpcode = inputs->LookupByName("OPCODE");
	if (!ciOpcode)
	{
		staterr("unable to find special input OPCODE");
		return 1;
	}
	
	coGoto = outputs->LookupByName("GOTO");
	if (!coGoto)
	{
		staterr("unable to find special output GOTO");
		return 1;
	}
	
	// populate the output_to_bitmask table, used to make metadata for emulator
	uint32_t bitmask = 0x01;
	for(int i=0;;i++)
	{
		ControlDef *output = outputs->list.ItemAt(i);
		if (!output) break;
		
		output_to_bitmask.AddValue(output->name, bitmask);
		bitmask <<= 1;
		if (!bitmask)
		{
			staterr("too many outputs: ran out of bits while populating output_to_bitmask");
			return 1;
		}
	}
	
	stat("");
	stat("Input/output defs parsed successfully.");
	stat("");
}

bool Compiler::compute_default_word()
{
	// start with all bits 1
	default_word = 0;
	for(int i=0;i<NUM_OUTPUT_PINS;i++)
		default_word |= (1 << (romword_t)i);
	
	//stat("initial default_word %s", show_romword(default_word, NUM_OUTPUT_PINS));
	
	// loop through all the outputs and apply their default value to the default word
	for(int i=0;;i++)
	{
		ControlDef *output = outputs->list.ItemAt(i);
		if (!output) break;
		
		//stat("Applying defaultValue %d for output %s to default word", output->defaultValue, output->Describe());
		default_word = output->SetValue(default_word, output->defaultValue);
		//stat("new default_word %s", show_romword(default_word, NUM_OUTPUT_PINS));
	}
	
	stat("Found default word: %s", show_romword(default_word, NUM_OUTPUT_PINS));
	return 0;
}

int Compiler::find_free_opcode_num()
{
	uint8_t start = next_opcode_num;
	for(;;)
	{
		if (opcode_lut[next_opcode_num] == NULL)
			return next_opcode_num++;
		
		if (++next_opcode_num == start)
		{
			staterr("out of available opcode numbers");
			return -1;
		}
	}
}

/*
void c------------------------------() {}
*/

bool Compiler::parse_source()
{
	opcodes.Clear();
	
	for(;;)
	{
		int lineNo = t->get_current_line();
		Token *tkn = t->next_token();
		stat("parse_source: %s", tkn->Describe());
		
		if (tkn->type == TKN_EOF) break;
		if (tkn->type == TKN_EOL) continue;
		
		if (tkn->type == TKN_WORD && !strcasecmp(tkn->text, "opcode"))
		{
			MicroIns *ins = parse_instruction();
			if (!ins) return 1;
			
			stat(" : opcode %s: %d microlines", ins->Describe(), ins->lines.CountItems());
		}
		else
		{
			staterr("unexpected token on line %d: %s", lineNo, tkn->Describe());
			return 1;
		}
	}
	
	return 0;
}

MicroIns *Compiler::parse_instruction()
{
	// allocate an opcode number for this instruction
	int opcodeNum;
	if (t->peek_next_token()->type == TKN_NUMBER)
	{
		opcodeNum = t->next_token()->value;
		if (opcode_lut[opcodeNum])
		{
			staterr("can't assign instruction to opcode 0x%02x: that opcode number is already in use", opcodeNum);
			return NULL;
		}
	}
	else
	{
		opcodeNum = find_free_opcode_num();
		if (opcodeNum < 0)
			return NULL;
	}
	
	// allocate the microinstruction
	auto *ins = new MicroIns();
	ins->opcodeNum = opcodeNum;
	
	//stat("Reading definition for opcode 0x%02x...", ins->opcodeNum);
	
	// read in the "written_as" definition
	for(;;)
	{
		Token *tkn = t->next_token();
		if (tkn->type == TKN_EOL) break;
		if (tkn->type == TKN_EOF)
		{
			staterr("unexpected end of file parsing opcode definition");
			delete ins;
			return NULL;
		}
		
		//stat("  written_as: '%s'", tkn->Describe());
		ins->written_as.AddItem(tkn);
	}
	
	// read in the microinstruction lines
	stat("Parsing source code for opcode 0x%02x...", ins->opcodeNum);
	List<MicroCondition> conditionStack;
	
	for(;;)
	{
		Token *tkn = t->peek_next_token();
		//staterr("peek_next_token = %s", tkn->Describe());
		
		if (tkn->type == TKN_END)
		{
			t->next_token();
			
			if (conditionStack.CountItems())
			{
				staterr("mismatched braces: IF statement never closed in opcode 0x%02x", ins->opcodeNum);
				delete ins;
				return NULL;
			}
			
			break;	// end of instruction
		}
		else if (tkn->type == TKN_EOL)
		{
			t->next_token();
			continue;	// ignore
		}
		else if (tkn->type == TKN_EOF)
		{
			staterr("unexpected end of file parsing source for opcode 0x%02x", ins->opcodeNum);
			delete ins;
			return NULL;
		}
		else if (tkn->type == TKN_IF)
		{
			//stat("Reading IF statement...");
			t->next_token();
			
			MicroCondition *cond = read_expression();
			if (!cond)
			{
				staterr("failed to parse IF expression for opcode 0x%02x", ins->opcodeNum);
				delete ins;
				return NULL;
			}
			
			//stat("Got condition: %s", cond->Describe());
			
			Token *openBrace = t->next_token_except_eol();
			if (openBrace->type != TKN_OPEN_CURLY_BRACE)
			{
				staterr("expected: '{' after IF expression");
				delete cond;
				delete ins;
				return NULL;
			}
			
			conditionStack.AddItem(cond);
			continue;
		}
		else if (tkn->type == TKN_CLOSE_CURLY_BRACE)
		{
			t->next_token();
			if (conditionStack.CountItems() == 0)
			{
				staterr("mismatched braces: found '}' when not inside IF statement");
				delete ins;
				return NULL;
			}
			
			//stat("found end of IF statement");
			MicroCondition *cond = conditionStack.RemoveItem(conditionStack.CountItems() - 1);
			
			// check for "else"
			if (t->next_token_except_eol()->type == TKN_ELSE)
			{
				//staterr("ELSE found");
				
				// negate the condition and carry on
				Token neg(TKN_EXCLAMATION, "!");
				cond->AddToken(&neg);
				conditionStack.AddItem(cond);
				
				Token *openBrace = t->next_token_except_eol();
				if (openBrace->type != TKN_OPEN_CURLY_BRACE)
				{
					staterr("expected: '{' after ELSE expression");
					delete cond;
					delete ins;
					return NULL;
				}
			}
			else
			{
				t->back_token();
				delete cond;
			}
			
			continue;
		}
		else if (tkn->type != TKN_WORD)
		{
			staterr("line %d: expected: microcode source line or END; but got '%s'", t->get_current_line(), tkn->Describe());
			delete ins;
			return NULL;
		}
		
		// parse the line of microcode source into a skeleton MicroLine structure containing the bytecode
		MicroLine *line = parse_micro_line(ins);
		if (!line)
		{
			delete ins;
			return NULL;
		}
		
		// apply the current conditional status, if any, to the microline
		if (conditionStack.CountItems())
			line->condition = conditionStack.LastItem()->Duplicate();
		else
			line->condition = NULL;
		
		//stat(" : MICROLINE %d, bytecode=%s:", ins->lines.CountItems(), show_romword(line->bytecode));
		//stat("    %s", line->Describe());
		
		// add the microline into the current instruction
		ins->lines.AddItem(line);
	}
	
	//staterr("DEBUG STOP");exit(1);
	
	opcode_lut[ins->opcodeNum] = ins;
	opcodes.AddItem(ins);
	
	return ins;
}

MicroLine *Compiler::parse_micro_line(MicroIns *ins)
{
int value;		// int isn't technically right, but I can't see us having a multiplex output with enough pins to overflow an int

	romword_t bytecode = default_word;
	
	List<ControlDef> used_outputs;
	int lineNo = t->get_current_line();
	
	for(;;)
	{
		Token *tknOutput = t->next_token();
		if (tknOutput->type == TKN_EOL) break;
		
		if (tknOutput->type == TKN_EOF)
		{
			staterr("unexpected end of file parsing micro line for opcode 0x%02x", ins->opcodeNum);
			return NULL;
		}
		
		//stat("\nbytecode %s micro_line: '%s'", show_romword(bytecode), tknOutput->Describe());
		
		// lookup the token to see if it's a known output
		if (tknOutput->type != TKN_WORD)
		{
			staterr("line %d: expected: output control name, got %s", lineNo, tknOutput->Describe());
			return NULL;
		}
		
		ControlDef *output = outputs->LookupByName(tknOutput->text);
		if (!output)
		{
			if (!strcasecmp(tknOutput->text, "nop"))
				goto skip;
			
			staterr("line %d: unknown output control name: '%s'", lineNo, tknOutput->text);
			return NULL;
		}
		else if (used_outputs.FindItem(output) >= 0)
		{
			staterr("line %d: output '%s' specified more than once", lineNo, tknOutput->text);
			return NULL;
		}
		
		//stat("Found output '%s'", output->Describe());
		
		// determine the value to apply to the output.
		value = 0;
		
		// peek and see if the next token is an open paren.
		if (t->peek_next_token()->type == TKN_OPEN_PAREN)
		{
			t->next_token();		// skip the open paren
			Token *tknValue = t->next_token();
			//stat("Parens found. tknValue: '%s'", tknValue->Describe());
			
			if (tknValue->type == TKN_NUMBER)
			{
				value = tknValue->value;
			}
			else if (tknValue->type == TKN_WORD)
			{
				value = output->enumEntries.LookupValue(tknValue->text);
				if (value < 0)
				{
					staterr("output '%s' has no enum named '%s' on line %d", output->name, tknValue->text, lineNo);
					return NULL;
				}
			}
			else
			{
				staterr("line %d: expected: number or enum, got %s", lineNo, tknValue->Describe());
			}
			
			// read in the close paren
			Token *tkn = t->next_token();
			if (tkn->type != TKN_CLOSE_PAREN)
			{
				staterr("line %d: expected ')', got %s", lineNo, tkn->Describe());
				return NULL;
			}
		}
		else	// no parens
		{
			if (output->HasFlag(FlagMultiplex))
			{
				staterr("line %d: output %s is multiplex and should be specified with paren syntax", lineNo, output->name);
				return NULL;
			}
			
			// if the output is active low, use a value of 0. otherwise use the maximum value for the control.
			if (output->HasFlag(FlagActiveLow))
				value = 0;
			else
				value = output->maxValue();
		}
		
		// verify the value is within valid range of the output
		if (value < 0 || value > output->maxValue())
		{
			staterr("value %d out of range for control '%s': max value %d",
				value, output->name, (uint32_t)output->maxValue());
			return NULL;
		}
		
		// apply the current output to the current value within the bytecode.
		//stat("setting output to value %d", value);
		bytecode = output->SetValue(bytecode, value);
		used_outputs.AddItem(output);
		
skip: ;
		//stat("new bytecode %s", show_romword(bytecode));
		
		// read in the token after the output designation
		Token *tkn = t->next_token();
		
		if (tkn->type == TKN_COMMA) { }		// skip commas
		else if (tkn->type == TKN_EOL)
		{	// end of microcode line
			break;
		}
		else
		{
			staterr("line %d: unexpected token after '%s': %s", lineNo, output->name, tkn->Describe());
			return NULL;
		}
	}
	
	//stat("Final bytecode: %s", show_romword(bytecode));
	
	auto *line = new MicroLine();
	line->bytecode = bytecode;
	line->used_outputs.AddList(&used_outputs);
	
	return line;
}

MicroCondition *Compiler::read_expression()
{
	if (!t->expect_token(TKN_OPEN_SQUARE_BRACKET))
		return NULL;
	
	auto *cond = new MicroCondition();
	for(;;)
	{
		Token *tkn = t->next_token();
		
		if (tkn->type == TKN_CLOSE_SQUARE_BRACKET)
		{
			break;
		}
		else if (tkn->type == TKN_EOL || tkn->type == TKN_EOF)
		{
			staterr("unexpected end of %s while reading expression", (tkn->type == TKN_EOF) ? "file":"line");
			delete cond;
			return NULL;
		}
		else
		{
			//stat("adding to expression: %s", tkn->Describe());
			cond->AddToken(tkn);
		}
	}
	
	if (cond->expr.CountItems() == 0)
	{
		staterr("empty expression not valid");
		delete cond;
		return NULL;
	}
	
	return cond;
}

/*
void c------------------------------() {}
*/

MicroCondition::~MicroCondition()
{
	expr.Clear();
}
	
void MicroCondition::SetTo(MicroCondition *other)
{
	expr.Clear();
	for(int i=0;;i++)
	{
		Token *tkn = other->expr.ItemAt(i);
		if (!tkn) break;
		
		AddToken(tkn);
	}
}

void MicroCondition::AddToken(Token *tkn)
{
	expr.AddItem(tkn->Duplicate());
}

const char *MicroCondition::Describe()
{
	if (!expr.CountItems())
		return "[ no condition ]";
	
	DString str;
	str.AppendString("[ ");
	for(int i=0;i<expr.CountItems();i++)
	{
		Token *tkn = expr.ItemAt(i);
		
		str.AppendString(tkn->text);
		str.AppendChar(' ');
	}
	
	str.AppendChar(']');
	return str.StaticString();
}

/*
void c------------------------------() {}
*/

int MicroLine::FindUsedOutput(ControlDef *search_def)
{
	for(int i=used_outputs.CountItems()-1;i>=0;i--)
	{
		ControlDef *def = used_outputs.ItemAt(i);
		if (def == search_def)
			return i;
	}
	
	return -1;
}

// if showUnspecifiedOutputs is false, shows only the outputs that were written in the source code.
// if true, it shows the full function of the line, and you must pass a list of all the outputs in outputs.
const char *MicroLine::Describe(bool showUnspecifiedOutputs, ControlDefList *outputs)
{
	DString str("<");
	
	if (showUnspecifiedOutputs && !outputs)
		staterr("showUnspecifiedOutputs is true, but no outputs were passed");
	
	if (!showUnspecifiedOutputs || outputs)
	{
		for(int i=0;;i++)
		{
			ControlDef *output;
			if (showUnspecifiedOutputs)
				output = outputs->list.ItemAt(i);
			else
				output = used_outputs.ItemAt(i);
			if (!output) break;
			
			int value = output->GetValue(bytecode);
			
			if (output->HasFlag(FlagMultiplex))
			{
				if (str.Length() > 1) str.AppendString(", ");
				str.AppendString(output->name);
				str.AppendChar('(');
				
				const char *enumName = output->enumEntries.ValueToKeyName(value);
				if (enumName)
					str.AppendString(enumName);
				else
					str.AppendString(stprintf("%d", value));
				
				str.AppendChar(')');
			}
			else
			{
				if (value != output->HasFlag(FlagActiveLow))
				{
					if (str.Length() > 1) str.AppendString(", ");
					str.AppendString(output->name);
				}
			}
		}
	}
	else
	{
		str.AppendString(show_romword(bytecode));
	}
	
	if (str.Length() == 1)
		str.AppendString("nop");
	
	str.AppendString("> condition ");
	if (!condition)
	{
		str.AppendString("none");
	}
	else
	{
		str.AppendString(condition->Describe());
	}
	
	return str.StaticString();
}

const char *MicroIns::Describe()
{
	DString str;
	str.AppendString(stprintf("0x%02x: '", opcodeNum));
	
	for(int i=0;i<written_as.CountItems();i++)
	{
		Token *tkn = written_as.ItemAt(i);
		if (i && tkn->type != TKN_COMMA) str.AppendChar(' ');
		
		if (tkn->text)
			str.AppendString(tkn->text);
		else
			str.AppendString(tkn->DescribeShort());
	}
	
	str.AppendChar('\'');
	return str.StaticString();
}

static const char *show_romword(romword_t value, int num_binary_digits)
{
char fmt[32];

	int numHexDigits = num_binary_digits / 4;
	if ((num_binary_digits % 4) != 0) numHexDigits++;
	if (numHexDigits <= 0) numHexDigits = 6;
	sprintf(fmt, "0x%%%02dx", numHexDigits);
	
	if (num_binary_digits <= 0)
		return stprintf(fmt, value);
	
	if (num_binary_digits > sizeof(romword_t) * 8)
		num_binary_digits = sizeof(romword_t) * 8;
	
	DString str;
	
	str.AppendChar('[');
	str.AppendString(stprintf(fmt, value));
	str.AppendChar(' ');
	
	for(int i=num_binary_digits-1;i>=0;i--)
	{
		if ((value & (1 << i)))
			str.AppendChar('1');
		else
			str.AppendChar('0');
	}
	
	str.AppendChar(']');
	return str.StaticString();
}

static const char *padto(const char *input, int padlen)
{
	DString str(input);
	while(str.Length() < padlen) str.AppendChar(' ');
	return str.StaticString();
}
