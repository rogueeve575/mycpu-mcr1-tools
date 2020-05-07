
#include "main.h"
#include "main.fdh"

static const uint8_t testcode[] =
{
	0x01,				// NOP at reset vector
	0x14,				// CLR r0
	0x1b,				// CLR r7
	0x12, 0x70,			// SBR r7, r0
	0x8a,				// INC r0
	0x9c, 0x08,			// CMP r0, 8
	0xa5, 0x03, 0x00,	// BNE 0x0003
	
	0xff				// signal for end of code
};

int main(int argc, char **argv)
{
	stat("Assembler started");
	
	if (argc != 2)
	{
		staterr("usage: %s [source file name]", argv[0]);
		return 1;
	}
	
	char *fname = argv[1];
	if (!file_exists(fname))
	{
		staterr("input file not found: '%s'", fname);
		return 1;
	}
	
	auto *assembler = new Assembler();
	int result = assembler->Assemble_File(fname);
	delete assembler;
	stat("");
	
	if (result)
	{
		staterr("Assembly failed.");
		return 1;
	}
	
	stat("Assembly success!");
	return 0;
}
