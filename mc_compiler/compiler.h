
#ifndef _COMPILER_H
#define _COMPILER_H

struct MicroLine;
struct MicroIns;
class MicroCondition;

class Compiler
{
public:
	Compiler();
	~Compiler() { Clear(); }
	void Clear();
	
	bool ParseFile(const char *fname);
	
private:
	bool ParseTokens();
	
	bool load_io_defs();
	bool parse_io_def(const char *fname, int iotype);
	bool compute_default_word();
	
	bool parse_source();
	MicroIns *parse_instruction();
	MicroLine *parse_micro_line(MicroIns *ins);
	MicroCondition *read_expression();
	
	bool propagate_opcodes();
	bool save_rom();
	bool save_assembler_table();
	
	int find_free_opcode_num();
	
private:
	Tokenizer *t;
	ControlDefList *inputs, *outputs;
	
	List<MicroIns> opcodes;
	MicroIns *opcode_lut[256];
	uint8_t next_opcode_num;
	
	// the special inputs & outputs: PHASE, OPCODE, and GOTO
	ControlDef *ciPhase, *ciOpcode, *coGoto;
	
	romword_t *rom;				// buffer holding full contents of the microcode ROM(s)
	size_t rom_size_words;		// size of the rom buffer, in rombyte_t elements
	size_t rom_size_bytes;		// size of the rom buffer, in bytes
	romword_t default_word;		// a word that has all outputs set to defaults which the ROM is initially filled with
	DString basePath;
	
	// rom metadata used by the emulator, for more readable disassembly
	// this allows the emulator to know which outputs were specified
	// in source for each rom location, as opposed to being implicit defaults.
	uint32_t *rom_metadata;
	int metadata_size_bytes;
	HashTable output_to_bitmask;
};

// stores a definition of a single microinstruction
class MicroIns
{
public:
	~MicroIns()
	{
		lines.Clear();
		written_as.MakeEmpty();
	}
	
	List<MicroLine> lines;
	
	uint8_t opcodeNum;			// which opcode # this is in machine-language
	List<Token> written_as;		// stores the assembly-language mnemonic of this instruction
	
	const char *Describe();
};

class MicroCondition
{
public:
	~MicroCondition();
	
	MicroCondition *Duplicate()
	{
		MicroCondition *mc = new MicroCondition();
		mc->SetTo(this);
		return mc;
	}
	
	void SetTo(MicroCondition *other);
	void AddToken(Token *tkn);
	const char *Describe();
	
public:
	List<Token> expr;
};

// stores one phase of microcode
class MicroLine
{
public:
	MicroLine() { bytecode = 0; condition = 0; }
	~MicroLine() { delete condition; }
	
	romword_t bytecode;		// final compiled instruction code
	
	// if present, an expression which must be met in order for this opcode to be included.
	// this is how things such as conditionals from the SREG flag are implemented.
	MicroCondition *condition;
	
	// lists which outputs were specified in the source code (as opposed to being at their defaults)
	List<ControlDef> used_outputs;
	
	int FindUsedOutput(ControlDef *search_def);
	const char *Describe(bool showUnspecifiedOutputs = false, ControlDefList *outputs = NULL);
};

#endif
