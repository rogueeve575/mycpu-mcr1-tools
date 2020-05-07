
#ifndef _EMULATOR_H
#define _EMULATOR_H

#define NUM_GP_REGS			16

#include "scrollwindow.h"
class MCScrollWindow;
class ASMScrollWindow;

struct GPRegDef
{
	const char *name;		// mnemonic name used in source code
	int latch_number;		// value of LATCH() output that latches it
	int idbus_number;		// value of IDBUS_SRC output that activates it
};

struct RegisterSet
{
public:
	uint16_t PC;
	uint16_t userAddr;
	
	uint8_t sreg;
	uint8_t latched_sreg;
	uint8_t alu_input[2];
	
	// general purpose registers (from microcode perspective) including r0l, r0h, tr0, SPL, LRH, etc
	uint8_t gp_regs[NUM_GP_REGS];
};

class Emulator
{
public:
	Emulator();
	~Emulator();
	
	bool init();				// initialize the emulator
	
	void reset();
	bool step();				// step one microcode phase
	bool step_instruction();	// step one assembly instruction
	
	bool initgui();
	void draw();
	void handlekey(int ch);
	
	const char *get_microcode_line(int phaseNum);
	const char *get_asm_line(int pc);
	
private:
	bool load_microcode();
	bool load_io_defs();
	bool load_rom(const char *fname, int address);
	
	bool handle_idbus_src(romword_t bytecode);
	bool handle_latch(romword_t bytecode);
	
	const char *disassemble_microcode(romword_t bytecode, uint32_t metadata=0xffffffff);
	
	void draw_registers();
	
private:
	friend class ASMScrollWindow;
	
	romword_t *microcode;
	int rom_size_words, rom_size_bytes;
	
	uint32_t *rom_metadata;
	int metadata_size_bytes;
	HashTable bitmask_to_output;
	
	uint8_t idbus;
	uint32_t addrbus;
	
	uint8_t opcode, parm;
	uint8_t phase;
	
	RegisterSet regs;
	uint8_t *memory;
	
	bool insstep_enabled;
	
	// GUI
	MCScrollWindow *mcWindow;
	ASMScrollWindow *asmWindow;
	ASMCore *asmcore;
	int registerAreaY;
	
	// microcode IO
	ControlDefList *inputs, *outputs;
	struct
	{
		struct	// convenience pointers to each input & output we'll need
		{
			ControlDef *opcode;
			ControlDef *phase;
			ControlDef *c, *v, *z;
		} in;
		
		struct
		{
			ControlDef *idbus_src;
			ControlDef *latch;
			ControlDef *addrbus_src;	// PC=0, USERADDR=1
			ControlDef *latch_sreg;		// active low
			ControlDef *inc_pc;			// active high
			ControlDef *_goto;			// GOTO()
			ControlDef *alu_mode;
			ControlDef *idbus_from;
			ControlDef *latch_from;
			ControlDef *portio;
			ControlDef *value;
		} out;
	} mcio;
};

namespace IDBUS
{
	enum
	{
		// on cards
		r0 = 0, r1 = 1, r2 = 2, r3 = 3,
		r4 = 4, r5 = 5, r6 = 6, r7 = 7,
		
		SPH = 9, SPL = 10,
		LRH = 11, LRL = 12,
		tr0 = 13, tr1 = 14,	// temporary registers for use by microcode
		//15 reserved, because LATCH needs a NONE
		
		MEMORY_READ = 16,	// gates data bus onto IDBUS, while asserting /RD
		ADDRBUS_HI = 17,	// gates MSB of address bus to data bus
		ADDRBUS_LO = 18,	// gates LSB of address bus to data bus
		ALU_RESULT = 19,	// output of ALU appears on IDBUS
		// reserved: 20-22
		VALUE = 23,			// put onto IDBUS a constant value stored in microcode and specified by the VALUE() control
	};
}

namespace LATCH
{
	enum
	{
		r0 = 0, r1 = 1, r2 = 2, r3 = 3,
		r4 = 4, r5 = 5, r6 = 6, r7 = 7,
		
		SPH = 9, SPL = 10,
		LRH = 11, LRL = 12,
		tr0 = 13, tr1 = 14,	// temporary registers for use by microcode
		NONE = 15,
		
		// everything after this has the MSB of the 5-bit code high.
		// that activates the 74138 on the main board (otherwise the lower 3 bits of LATCH is routed out to be decoded by register cards).
		OPCODE = 16,
		USERADDR_HI = 17,	// these two regs output to address bus and can be gated on by ADDRBUS_SRC,,,
		USERADDR_LO = 18,	// ...they can not be read back via the ID bus
		MEMORY_WRITE = 19,
		ALU_INPUT_1 = 20,
		ALU_INPUT_2 = 21,
		PC_LO = 22,
		PC_HI = 23,
		
		// this is identical to the LATCH code for OPCODE save for 1 bit (the 4th of the 5 bits).
		// two OR gates and a NOT gate can multiplex that bit to whether the latch will be activated on OPCODE, or on PARM.
		// PARM is used to specify source and/or destination registers for two-byte opcodes--
		// see the LATCH_FROM and IDBUS_FROM controls.
		PARM = 24
	};
}

namespace IDBUS_FROM
{
	enum
	{
		MICROCODE = 3,
		PARMDST = 2,
		PARMSRC = 1
	};
}

namespace LATCH_FROM
{
	enum
	{
		PARMDST = 0,
		MICROCODE = 1
	};
}

// ---------------------------------------

class MCScrollWindow : public ScrollWindow
{
public:
	MCScrollWindow(Emulator *emu, int x1, int y1, int x2, int y2)
		: ScrollWindow(x1, y1, x2, y2)
	{ this->emu = emu; }
	
	const char *GetItemAtIndex(int index)
	{
		return emu->get_microcode_line(index);
	}
	
private:
	Emulator *emu;
};


uint8_t ioport_read(uint32_t address);
void ioport_write(uint32_t address, uint8_t value);

#endif
