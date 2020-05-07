
#ifndef _ASMCORE_H
#define _ASMCORE_H

// this module provides routines for loading and accessing the assembler table
// which is output by the microcode compiler. it is used by both the
// assembler and disassembler.

class InstructionDef
{
public:
	InstructionDef() {
		opcodeNum = -1;
		usesParm = false;
		length = 0;
	}
	
	~InstructionDef() {
		opcodeNum = -1;
		tokens.Clear();
	}
	
	const char *Describe();
	
public:
	uint8_t opcodeNum;
	bool usesParm;			// 1 for a two-byte opcode that uses parmdst/parmsrc
	int length;				// total length of the instruction in bytes
	
	// tokens making up the mnemonic. note that this may not be the exact mnemonic you see in assembly,
	// it's resolved in a "best match" manner similar to C++ overrides. Some tokens such as parmhi
	// or mem16 are placeholders for a certain TYPE of token that would be seen in the assembly source
	// if this is the opcode for you.
	List<Token> tokens;
};

class AssemblyResult
{
public:
	AssemblyResult() { insdef = NULL; }
	
	InstructionDef *insdef;		// holds the InstructionDef for the instruction assembled
	DBuffer bytes;				// holds the machine code for the assembled instruction
};

class DisassemblyResult
{
public:
	DisassemblyResult() { insdef = NULL; address = 0; }
	
	InstructionDef *insdef;		// holds the InstructionDef for the instruction disassembled
	DString dasmtext;			// holds the disassembled text
	DString hextext;			// holds a formatted hex representation of the instruction's machine code
	int length;					// length of the instruction
	int address;				// calling program can use this to track what address this ins was at if it wants
};

namespace DASMFlags
{
	enum
	{
		IncludeHex = 0x01,
		IgnoreErrors = 0x02,
		IncludeAddress = 0x04	// works only with FormatDisassemblyResult if calling program has set the address after disassembly
	};
}

class ASMCore
{
public:
	ASMCore();
	~ASMCore();
	
	bool LoadTable(const char *fname);
	
public:
	DisassemblyResult *Disassemble(const uint8_t *ins, uint8_t flags = 0);	// ins points to the start of the instruction in memory
	int Disassemble(const uint8_t *ins, DString *out, uint8_t flags = 0);
	bool FormatDisassemblyResult(DisassemblyResult *result, DString *out, uint8_t flags = 0);
	
	AssemblyResult *Assemble(const char *line, int lineLength = -1);
	AssemblyResult *Assemble(List<Token> *tokens);
	
	bool IsValidMnemonic(const char *mnemonic);

public:
	inline InstructionDef *LookupOpcode(uint8_t op) { return opcode_to_insdef[op]; }
	inline InstructionDef *OpcodeAt(int index) { return opcodes.ItemAt(index); }
	inline int NumOpcodes() { return opcodes.CountItems(); }
	
private:
	DisassemblyResult *_Disassemble(const uint8_t *ins);
	
	// given a list of tokens containing a line of assembly, find the best matching InstructionDef
	InstructionDef *MatchInstruction(List<Token> *tokens);
	bool DoesInstructionMatch(List<Token> *candidates, List<Token> *deftokens);
	bool CanTokensMatch(Token *candidate, Token *deftoken);
	
	bool LoadOpcodeList(FILE *fp);
	bool LoadParmValues(FILE *fp);
	
	// list of known instructions, along with a lookup table for fast backwards matching
	// from an opcode number to an ASMIns
	List<InstructionDef> opcodes;
	InstructionDef *opcode_to_insdef[256];
	
	// valid register names to replace dstreg/srcreg keywords, and their associated numeric code
	HashTable parmValues;
};


#endif
