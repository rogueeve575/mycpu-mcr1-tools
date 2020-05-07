
#include "main.h"
#include "main.fdh"

//#define GENERATE_ALU

int main(int argc, char **argv)
{
	const char *fname = "../microcode/microcode.mc";
	if (argc >= 2)
	{
		if (argc == 2 && argv[1][0] != '-')
		{
			fname = argv[1];
		}
		else
		{
			stat("usage: %s {filename}", argv[0]);
			stat("\t if omitted, filename defaults to %s\n", fname);
			return -1;
		}
	}
	
	if (!file_exists(fname))
	{
		staterr("file not found: '%s'", fname);
		return 1;
	}
	
	// compile the microcode into rom images
	stat("Compiling source file '%s'", fname);
	
	Compiler *compiler = new Compiler();
	if (compiler->ParseFile(fname))
	{
		staterr("\e[1;31m-- compile failed --\e[0m");
		delete compiler;
		return 1;
	}
	
	stat("\e[1;32m-- compilation complete --\e[0m");
	delete compiler;
	
	// generate ALU tables
	#ifdef GENERATE_ALU
	stat("\nCompiling ALU tables...\n");
	
	ALUCompiler *alu = new ALUCompiler();
	if (alu->Generate())
	{
		staterr("\e[1;31m-- ALU generation failed --\e[0m");
		delete alu;
		return 1;
	}
	
	stat("\e[1;32m-- ALU generation complete --\e[0m");
	delete alu;
	#else
	stat("Skipping generation of ALU tables.");
	#endif
	
	return 0;
}
