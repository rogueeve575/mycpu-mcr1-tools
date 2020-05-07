
//#define DEBUG
#include "main.h"
#include "emulator.h"
#include "emulator.fdh"
using namespace ALU;

static const char *microcode_fname_fmt = "../microcode/mcrom%d.bin";
static const char *microcode_metadata_fname = "../microcode/mc_metadata.dat";


Emulator::Emulator()
{
	memset(&regs, 0, sizeof(regs));
	microcode = NULL;
	inputs = NULL;
	outputs = NULL;
	asmcore = NULL;
	mcWindow = NULL;
	asmWindow = NULL;
	insstep_enabled = false;
}

Emulator::~Emulator()
{
	if (microcode)
		free(microcode);
	
	if (memory)
		free(memory);
	
	delete inputs;
	delete outputs;
	delete asmcore;
	delete mcWindow;
	delete asmWindow;
}

/*
void c------------------------------() {}
*/

bool Emulator::init()
{
	stat("");
	
	// allocate the memory to hold the microcode
	rom_size_words = (1 << NUM_INPUT_PINS);
	rom_size_bytes = rom_size_words * sizeof(romword_t);
	microcode = (romword_t *)malloc(rom_size_bytes);
	stat("Allocated microcode buffer of %d bytes (%d %d-bit words).", rom_size_bytes, rom_size_words, NUM_OUTPUT_PINS);
	
	// allocate space for the ROM metadata
	metadata_size_bytes = rom_size_words * sizeof(uint32_t);
	rom_metadata = (uint32_t *)malloc(metadata_size_bytes);
	stat("Allocated %d bytes for ROM metadata.", metadata_size_bytes);
	
	uint32_t mem_alloc_size = (1 << 16) + 32;	// add a few extra bytes to avoid buffer overflows when disassembling
	memory = (uint8_t *)malloc(mem_alloc_size);
	memset(memory, 0xff, mem_alloc_size);
	stat("Allocated main memory: %d bytes.", mem_alloc_size);
	
	stat(""); stat("Initializing disassembly core...");
	asmcore = new ASMCore();
	if (asmcore->LoadTable("../assembler/asmtable.dat"))
		return 1;
	
	// clear memory and load test program
	if (load_rom("../assembler/tests/test.bin", 0xC000))
		return 1;
	
	stat(""); stat("Loading microcode...");
	if (load_microcode())
		return 1;
	
	stat(""); stat("Loading IO defs...");
	if (load_io_defs())
		return 1;
	
	reset();
	
	return 0;
}

void Emulator::reset()
{
	stat(""); stat("- Resetting CPU -");
	memset(&regs, 0, sizeof(regs));
	
	// scramble some regs for power-up
	regs.PC = 0xA5A5;
	
	idbus = 0xA5;
	addrbus = 0x55BEEF;
	parm = 0xFF;
	
	// simulate RESET condition
	opcode = 0x00;		// RESET opcode
	phase = 0;			// clear phase register
	
	if (mcWindow) mcWindow->SetIndex(phase);
	if (asmWindow) asmWindow->SetPC(regs.PC);
}

bool Emulator::step_instruction()
{
	dstat("- Step Instruction -");
	insstep_enabled = true;
	while(insstep_enabled)
	{
		if (step()) return 1;
	}
}

bool Emulator::step()
{
	dstat(""); dstat("- Step -");
	dstat("Opcode=0x%02x Phase=%d Parm=%02x SREG=%s", opcode, phase, parm, describe_sreg(regs.sreg));
	
	if (phase == 0)
		asmWindow->SetPC(regs.PC);
	
	// use the current value of the microcode inputs to determine the address
	// and then fetch the bytecode from that address
	uint16_t address = 0;
	address = mcio.in.opcode->SetValue(address, opcode);
	address = mcio.in.phase->SetValue(address, phase);
	if (regs.latched_sreg & SREG::Z) address = mcio.in.z->SetValue(address, 1);
	if (regs.latched_sreg & SREG::C) address = mcio.in.c->SetValue(address, 1);
	if (regs.latched_sreg & SREG::V) address = mcio.in.v->SetValue(address, 1);
	romword_t bytecode = microcode[address];
	
	dstat("Current microcode address 0x%04x, bytecode %06x", address, bytecode);
	dstat("%s", disassemble_microcode(bytecode));
	
	// set the address bus as specified by ADDRBUS_SRC
	if (mcio.out.addrbus_src->IsActive(bytecode))
		addrbus = regs.userAddr;
	else
		addrbus = regs.PC;
	
	dstat("Address bus: 0x%04X", addrbus);
	
	// read IDBUS_SRC and put the appropriate value onto the ID Bus
	if (handle_idbus_src(bytecode)) return 1;
	
	// latch anything specified by the LATCH output
	if (handle_latch(bytecode)) return 1;
	
	// if LATCH_SREG is asserted, copy sreg to latched_sreg
	if (mcio.out.latch_sreg->IsActive(bytecode))
		regs.latched_sreg = regs.sreg;
	
	// if INC_PC is asserted, increment the PC
	if (mcio.out.inc_pc->IsActive(bytecode))
		regs.PC++;
	
	// go to the next phase: set the current phase to the value of the GOTO output
	phase = mcio.out._goto->GetValue(bytecode);
	
	mcWindow->SetIndex(phase);
	if (phase == 0)
	{
		asmWindow->SetPC(regs.PC);
		insstep_enabled = false;
	}
	
	return 0;
}

bool Emulator::handle_idbus_src(romword_t bytecode)
{
	romword_t idbus_src;
	int idbus_from = mcio.out.idbus_from->GetValue(bytecode);
	switch(idbus_from)
	{
		case IDBUS_FROM::MICROCODE:
			idbus_src = mcio.out.idbus_src->GetValue(bytecode);
		break;
		
		case IDBUS_FROM::PARMDST:
			idbus_src = (parm & 0xf0) >> 4;
		break;
		
		case IDBUS_FROM::PARMSRC:
			idbus_src = (parm & 0x0f);
		break;
		
		default:
			staterr("unexpected value %02x for IDBUS_FROM", idbus_from);
			return 1;
	}
	
	dstat("idbus_src = %d (%s) from %d", \
		idbus_src, mcio.out.idbus_src->enumEntries.ValueToKeyName(idbus_src), idbus_from);
	
	idbus = 0xfe;
	
	if (idbus_src < NUM_GP_REGS)		// general purpose registers
	{
		idbus = regs.gp_regs[idbus_src];
		return 0;
	}
	
	switch(idbus_src)
	{
		case IDBUS::MEMORY_READ:
		{
			if (mcio.out.portio->IsActive(bytecode))
				idbus = ioport_read(addrbus);
			else
				idbus = memory[addrbus];
		}
		break;
		
		case IDBUS::ADDRBUS_HI:
		{
			idbus = (addrbus & 0xff00) >> 8;
		}
		break;
		
		case IDBUS::ADDRBUS_LO:
		{
			idbus = (addrbus & 0xff);
		}
		break;
		
		case IDBUS::ALU_RESULT:
		{
			uint8_t mode = mcio.out.alu_mode->GetValue(bytecode);
			if (alu_compute(regs.alu_input[0], regs.alu_input[1], mode, &idbus, &regs.sreg))
			{
				staterr("ALU error encountered");
				return 1;
			}
		}
		break;
		
		case IDBUS::VALUE:
			idbus = mcio.out.value->GetValue(bytecode);
		break;
		
		default:
			staterr("unhandled IDBUS_SRC %d", idbus_src);
			idbus = 0xff;
		break;
	}
	
	return 0;
}

bool Emulator::handle_latch(romword_t bytecode)
{
	romword_t latch;
	int latch_from = mcio.out.latch_from->GetValue(bytecode);
	switch(latch_from)
	{
		case LATCH_FROM::MICROCODE:
			latch = mcio.out.latch->GetValue(bytecode);
		break;
		
		case LATCH_FROM::PARMDST:
			latch = (parm & 0xf0) >> 4;
		break;
		
		default:
			staterr("unexpected value %02x for LATCH_FROM", latch_from);
			return 1;
	}
	
	dstat("latch = %d (%s) from %d", \
		latch, mcio.out.latch->enumEntries.ValueToKeyName(latch), latch_from);
	
	if (latch < NUM_GP_REGS)		// general purpose registers
	{
		regs.gp_regs[latch] = idbus;
		return 0;
	}
	
	switch(latch)
	{
		case LATCH::MEMORY_WRITE:
		{
			if (mcio.out.portio->IsActive(bytecode))
					ioport_write(addrbus, idbus);
				else
					memory[addrbus] = idbus;
		}
		break;
		
		case LATCH::USERADDR_HI:
		{
			regs.userAddr &= 0x00FF;
			regs.userAddr |= (int)idbus << 8;
		}
		break;
		
		case LATCH::USERADDR_LO:
		{
			regs.userAddr &= 0xFF00;
			regs.userAddr |= idbus;
		}
		break;
		
		case LATCH::ALU_INPUT_1: regs.alu_input[0] = idbus; break;
		case LATCH::ALU_INPUT_2: regs.alu_input[1] = idbus; break;
		
		case LATCH::PC_LO:
		{
			regs.PC &= 0xFF00;
			regs.PC |= idbus;
		}
		break;
		
		case LATCH::PC_HI:
		{
			regs.PC &= 0x00FF;
			regs.PC |= (int)idbus << 8;
		}
		break;
		
		case LATCH::OPCODE: opcode = idbus; break;
		case LATCH::PARM: parm = idbus; break;
		
		case LATCH::NONE: break;
		
		default:
			staterr("unhandled LATCH(%d)", latch);
			return 1;
	}
	
	return 0;
}

/*
void c------------------------------() {}
*/

bool Emulator::load_microcode()
{
	int num_files = NUM_OUTPUT_PINS / 8;
	stat("Reading %d ROM images...", num_files);
	
	int shift = 0;
	for(int fileNo=0;fileNo<num_files;fileNo++)
	{
		char fname[MAXPATHLEN];
		sprintf(fname, microcode_fname_fmt, fileNo+1);
		
		stat("  Loading %s shift=%d", fname, shift);
		
		FILE *fp = fopen(fname, "rb");
		if (!fp)
		{
			staterr("unable to open microcode file %s", fname);
			return 1;
		}
		
		for(int address=0;address<rom_size_words;address++)
		{
			int ch = fgetc(fp);
			microcode[address] |= (ch << shift);
		}
		
		fclose(fp);
		shift += 8;
	}
	
	stat("Loading metadata...");
	FILE *fp = fopen(microcode_metadata_fname, "rb");
	if (!fp)
	{
		staterr("unable to open metadata file %s", microcode_metadata_fname);
		return 1;
	}
	
	if (fgetc(fp) != 'K') { staterr("metadata: invalid signature byte"); return 1; }
	fread(rom_metadata, metadata_size_bytes, 1, fp);
	
	fclose(fp);
	
	stat("Microcode loaded successfully.");
	return 0;
}

bool Emulator::load_io_defs(void)
{
	{
		ControlDefParser parser;
		inputs = parser.ParseFile("../microcode/inputs.def", INPUT, NUM_INPUT_PINS);
		if (!inputs)
			return 1;
	}
	
	{
		ControlDefParser parser;
		outputs = parser.ParseFile("../microcode/outputs.def", OUTPUT, NUM_OUTPUT_PINS);
		if (!outputs)
			return 1;
	}
	
	// map controls to variables
	if (inputs->MapToVars(
				"OPCODE", 	&mcio.in.opcode,
				"PHASE", 	&mcio.in.phase,
				"C", 		&mcio.in.c,
				"V", 		&mcio.in.v,
				"Z", 		&mcio.in.z,
				NULL
	)) return 1;
	
	if (outputs->MapToVars(
				"IDBUS_SRC", 	&mcio.out.idbus_src,
				"LATCH", 		&mcio.out.latch,
				"ADDRBUS_SRC", 	&mcio.out.addrbus_src,
				"LATCH_SREG", 	&mcio.out.latch_sreg,
				"LATCH_SREG",	&mcio.out.portio,	// shared w/ LATCH_SREG
				"INC_PC", 		&mcio.out.inc_pc,
				"GOTO", 		&mcio.out._goto,
				"ALU_MODE",		&mcio.out.alu_mode,
				"IDBUS_FROM",	&mcio.out.idbus_from,
				"LATCH_FROM",	&mcio.out.latch_from,
				"VALUE", 		&mcio.out.value,
				NULL
	)) return 1;
	
	return 0;
}

bool Emulator::load_rom(const char *fname, int address)
{
	stat("Loading ROM image %s at 0x%04x...", fname, address);
	FILE *fp = fopen(fname, "rb");
	if (!fp)
	{
		staterr("unable to open '%s': %s", fname, strerror(errno));
		return 1;
	}
	
	for(;;)
	{
		int ch = fgetc(fp);
		if (ch < 0) break;
		
		if (address > 0xFFFF)
		{
			staterr("ROM too large to fit in memory");
			fclose(fp);
			return 1;
		}
		
		memory[address++] = ch;
	}
	
	fclose(fp);
	return 0;
}

/*
void c------------------------------() {}
*/

// used by debugger
const char *Emulator::get_microcode_line(int phaseNum)
{
	if (phaseNum < 0 || phaseNum > mcio.in.phase->maxValue())
		return stprintf("(invalid microcode phase %d)", phaseNum);
	
	uint16_t address = 0;
	address = mcio.in.opcode->SetValue(address, opcode);
	address = mcio.in.phase->SetValue(address, phaseNum);
	if (regs.latched_sreg & SREG::Z) address = mcio.in.z->SetValue(address, 1);
	if (regs.latched_sreg & SREG::C) address = mcio.in.c->SetValue(address, 1);
	if (regs.latched_sreg & SREG::V) address = mcio.in.v->SetValue(address, 1);
	
	const char *disasm = disassemble_microcode(microcode[address], rom_metadata[address]);
	
	return stprintf("%s%d. %04x: %s",
		(phaseNum < 10) ? " " : "",
		phaseNum, address,
		disasm);
}

const char *Emulator::disassemble_microcode(romword_t bytecode, uint32_t metadata)
{
//metadata=0xffffffff;
	if (metadata == 0)
		return "NOP";
	
	DString str;
	for(int i=0;;i++)
	{
		ControlDef *output = outputs->list.ItemAt(i);
		if (!output) break;
		if (!(metadata & (1 << i))) continue;
		
		int value = output->GetValue(bytecode);
		
		if (output->HasFlag(FlagMultiplex))
		{
			if (str.Length() != 0) str.AppendString(", ");
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
	
	return str.StaticString();
}

/*
void c------------------------------() {}
*/

bool Emulator::initgui()
{
	stat("Initializing emulator GUI");
	
	// initialize the two disassembly windows
	int xsplit = conwidth() - 40;
	int ysplit = conheight() - DEBUG_NUM_LINES - 3 - 7;
	mcWindow = new MCScrollWindow(this, 0, 0, xsplit, ysplit);
	mcWindow->SetTitle("Microcode");
	mcWindow->SetNumItems(mcio.out._goto->maxValue() + 1);
	mcWindow->SetIndex(phase);
	
	asmWindow = new ASMScrollWindow(this, xsplit+1, 0, conwidth() - 1, ysplit);
	asmWindow->SetPC(regs.PC);
	
	registerAreaY = ysplit + 2;
	
	return 0;
}

void Emulator::handlekey(int ch)
{
	switch(ch)
	{
		case 13:
			step();
		break;
		
		case ' ':
		case 'i':
			step_instruction();
		break;
		
		case KEY_UPARROW:
			mcWindow->SetSelectionIndex(mcWindow->GetSelectionIndex() - 1);
			mcWindow->Draw();
		break;
		
		case KEY_DOWNARROW:
			mcWindow->SetSelectionIndex(mcWindow->GetSelectionIndex() + 1);
			mcWindow->Draw();
		break;
		
		case 27:
			stat("Exiting program");
			app_quitting = true;
		break;
		
		default:
			stat("Unhandled keypress %d", ch);
		break;
	}
}

void Emulator::draw()
{
	coninhibitupdate(true);
	
	mcWindow->Draw();
	asmWindow->Draw();
	draw_registers();
	
	coninhibitupdate(false);
}

void Emulator::draw_registers()
{
	textcolor(14);
	textbg(0);
	
	const int spacing = 16;
	const int col1 = 2;
	const int col2 = col1 + spacing + 4;
	const int col3 = col2 + spacing + 3;
	const int col4 = col3 + spacing;
	const int col5 = col4 + spacing;
	//const int col6 = col5 + spacing;
	
	gotoxy(col1, registerAreaY+0); cstatnocr("OPCODE: %02X", opcode);
	textcolor(6); cstatnocr(" [%01X<<%01X]", parm>>4, parm&0x0f); textcolor(14);
	gotoxy(col1, registerAreaY+1); cstatnocr("PHASE: %d ", phase);
	
	gotoxy(col1, registerAreaY+3); cstatnocr("ALU_INPUT_1: %02x", regs.alu_input[0]);
	gotoxy(col1, registerAreaY+4); cstatnocr("ALU_INPUT_2: %02x", regs.alu_input[1]);
	//gotoxy(col2, registerAreaY+4); cstatnocr("USERADDR: %04x", regs.userAddr);
	
	#define showreg(col, Yoffs, regName, fmt, reg) \
	do	\
	{	\
		gotoxy(col, registerAreaY+Yoffs);	\
		textcolor(11); cstatnocr(regName);	\
		textcolor(3); cstatnocr(fmt, reg);	\
	} while(0)
	
	//showreg(col2, 0, "PC: ", "%04x", regs.PC);
	textcolor(11);
	gotoxy(col2, registerAreaY+0); cstatnocr("PC    : %04x", regs.PC);
	
	showreg(col2, 1, "USRADR: ", "%04x", regs.userAddr);
	
	showreg(col2, 3, "SREG  : ", "%s   ", describe_sreg(regs.sreg));
	showreg(col2, 4, "LATSR : ", "%s   ", describe_sreg(regs.latched_sreg));
	
	
	showreg(col3, 0, "r0: ", "%02x", regs.gp_regs[IDBUS::r0]);
	showreg(col3, 1, "r1: ", "%02x", regs.gp_regs[IDBUS::r1]);
	showreg(col3, 2, "r2: ", "%02x", regs.gp_regs[IDBUS::r2]);
	showreg(col3, 3, "r3: ", "%02x", regs.gp_regs[IDBUS::r3]);
	showreg(col3, 4, "r4: ", "%02x", regs.gp_regs[IDBUS::r4]);
	
	showreg(col4, 0, "r5: ", "%02x", regs.gp_regs[IDBUS::r5]);
	showreg(col4, 1, "r6: ", "%02x", regs.gp_regs[IDBUS::r6]);
	showreg(col4, 2, "r7: ", "%02x", regs.gp_regs[IDBUS::r7]);
	
	textcolor(11);
	gotoxy(col5, registerAreaY+0); cstatnocr("SP: %02x%02x", regs.gp_regs[IDBUS::SPH], regs.gp_regs[IDBUS::SPL]);
	gotoxy(col5, registerAreaY+1); cstatnocr("LR: %02x%02x", regs.gp_regs[IDBUS::LRH], regs.gp_regs[IDBUS::LRL]);
	
	showreg(col5, 3, "tr0: ", "%02x", regs.gp_regs[IDBUS::tr0]);
	showreg(col5, 4, "tr1: ", "%02x", regs.gp_regs[IDBUS::tr1]);
	
	/*
			r0l = 0, r1l = 1, r2l = 2, r3l = 3, r4l = 4,
		tr0 = 5, tr1 = 6,	// temporary registers for use by microcode
		SPL = 7,			// lower stack register
		
		r0h = 8, r1h = 9, r2h = 10, r3h = 11, r4h = 12,
		LRL = 13, LRH = 14,	// link register to store return address of jsr. return by using mov pc, lr
		SPH = 15,			// upper stack register (the stack always stores in CS page)
	*/
}

/*
void c------------------------------() {}
*/

uint8_t ioport_read(uint32_t address)
{
	staterr("STUB: read from IO port %06x", address);
	return 0xA5;
}

void ioport_write(uint32_t address, uint8_t value)
{
	staterr("STUB: write %02x to IO port %06x", value, address);
}
