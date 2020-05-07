
#include "common.h"
#include "alu.h"
#include "alu.fdh"
using namespace ALU;

ALUCompiler::ALUCompiler()
{
	alu_table = NULL;
	alu_table_size_bytes = 0;
	alu_table_size_words = 0;
	
	memset(&io, 0, sizeof(io));		// set all pointers to IO controls to null
	ctrlDefLists[0] = NULL;
	ctrlDefLists[1] = NULL;
}

ALUCompiler::~ALUCompiler()
{
	if (alu_table)
		free(alu_table);
	
	delete ctrlDefLists[0];
	delete ctrlDefLists[1];
}

/*
void c------------------------------() {}
*/

bool ALUCompiler::Generate()
{
	if (load_io_defs("../microcode/alu_inputs.def", INPUT)) return 1;
	if (load_io_defs("../microcode/alu_outputs.def", OUTPUT)) return 1;
	
	// allocate ALU table
	alu_table_size_words = (1 << ALU_NUM_INPUT_PINS);
	alu_table_size_bytes = alu_table_size_words * sizeof(*alu_table);
	
	alu_table = (uint16_t *)malloc(alu_table_size_bytes);
	memset(alu_table, 0xff, alu_table_size_bytes);
	
	stat("Allocated %s bytes (%s words) for ALU table", \
		comma_number(alu_table_size_bytes), comma_number(alu_table_size_words));
	
	// populate the ALU table
	if (populate_alu_table())
		return 1;
	
	if (save_rom())
		return 1;
	
	return 0;
}

bool ALUCompiler::populate_alu_table()
{
uint8_t result, sreg;

	int numModes = (1 << io.mode->numPins);
	stat("%d ALU modes", numModes);
	
	//alu_compute(107, 7, ALUMode::SUB, &result, &sreg);
	//stat("result: %d, sreg: %s", result, describe_sreg(sreg));
	
	for(int mode = 0; mode < numModes; mode++)
	{
		stat("Generating ALU mode %d...", mode);
		
		for(int input2=0;input2<256;input2++)
		for(int input1=0;input1<256;input1++)
		{
			alu_compute(input1, input2, mode, &result, &sreg);
			//stat("result: 0x%02X, sreg: %s", result, describe_sreg(sreg));
			
			// find the address to put this result in the ROM
			romword_t address = 0;
			address = io.input1->SetValue(address, input1);
			address = io.input2->SetValue(address, input2);
			address = io.mode->SetValue(address, mode);
			
			// find the value to place there
			romword_t value = 0xffff;
			value = io.result->SetValue(value, result);
			value = io.sreg->SetValue(value, sreg);
			
			//stat("address = %08x", address);
			//stat("value = %04x", value);
			
			if ((value & 0xffff) != value)
			{
				staterr("ALU result doesn't fit into 16 bits: result=%08x, input1=%d, input2=%d, mode=%d", \
					result, input1, input2, mode);
				return 1;
			}
			
			if (address >= alu_table_size_words)
			{
				staterr("ALU address outside of table: address=%08x, input1=%d, input2=%d, mode=%d", \
					address, input1, input2, mode);
				return 1;
			}
			
			// place the value into the table
			alu_table[address] = (uint16_t)value;
		}
	}
	
	stat("ALU table populated successfully.");
}

bool alu_compute(uint8_t input1, uint8_t input2, uint8_t mode, \
						uint8_t *result_out, uint8_t *sreg_out)
{
const int muldiv_tbl[] = { 0, 3, 5, 10, 15, 50, 60, 80, 100 };
bool no_auto_sreg = false;
uint8_t result = 0, sreg = 0;
uint16_t iresult;

	// implement ALU modes. For all but unary mode, V & Z flags are handled automatically after the switch.
	// each case is responsible for implementing the C flag.
	switch(mode)
	{
		case ALUMode::ADD:
		{
			iresult = (uint16_t)input1 + (uint16_t)input2;
			if (iresult & 0xff00) sreg |= SREG::C;
			result = iresult & 0xFF;
		}
		break;
		
		case ALUMode::ADD_AND_INC:
		{
			iresult = ((uint16_t)input1 + (uint16_t)input2) + 1;
			if (iresult & 0xff00) sreg |= SREG::C;
			result = iresult & 0xFF;
		}
		break;
		
		case ALUMode::SUB:
		{
			iresult = (uint16_t)input1 - (uint16_t)input2;
			if (iresult & 0xff00) sreg |= SREG::C;
			result = iresult & 0xFF;
		}
		break;
		
		case ALUMode::SUB_AND_DEC:
		{
			iresult = ((uint16_t)input1 - (uint16_t)input2) - 1;
			if (iresult & 0xff00) sreg |= SREG::C;
			result = iresult & 0xFF;
		}
		break;
		
		case ALUMode::AND:
		{
			result = (input1 & input2);
		}
		break;
		
		case ALUMode::INC:
		{
			result = input1 + 1;
			if (result == 0) sreg |= SREG::C;
		}
		break;
		
		case ALUMode::DEC:
		{
			result = input1 - 1;
			if (result == 0xff) sreg |= SREG::C;
		}
		break;
		
		case ALUMode::UNARY:
		{
			if (input2 >= UnaryMode::SET_SREG_0 && input2 <= UnaryMode::SET_SREG_7)
			{
				sreg = (input2 - UnaryMode::SET_SREG_0);
				result = sreg;
				no_auto_sreg = true;
			}
			else if (input2 >= UnaryMode::SETBIT_0 && input2 <= UnaryMode::SETBIT_7)
			{
				result = input1 | (1 << (input2 - UnaryMode::SETBIT_0));
			}
			else if (input2 >= UnaryMode::CLRBIT_0 && input2 <= UnaryMode::CLRBIT_7)
			{
				result = input1 & ~(1 << (input2 - UnaryMode::CLRBIT_0));
			}
			else if (input2 >= UnaryMode::GETBIT_0 && input2 <= UnaryMode::GETBIT_7)
			{
				result = (input1 & (1 << input2 - UnaryMode::GETBIT_0)) != 0;
			}
			else if (input2 > UnaryMode::MUL_START_LO && input2 < UnaryMode::MUL_END_LO)
			{
				int value = (int)input1 * muldiv_tbl[input2 - UnaryMode::MUL_START_LO];
				if (value > 0xffff) sreg |= SREG::C;
				result = (value & 0xff);
			}
			else if (input2 > UnaryMode::MUL_START_HI && input2 < UnaryMode::MUL_END_HI)
			{
				int value = (int)input1 * muldiv_tbl[input2 - UnaryMode::MUL_START_HI];
				if (value > 0xffff) sreg |= SREG::C;
				result = (value & 0xff00) >> 8;
			}
			else if (input2 > UnaryMode::DIV_START && input2 < UnaryMode::DIV_END)
			{
				result = input1 / muldiv_tbl[input2 - UnaryMode::DIV_START];
			}
			else if (input2 > UnaryMode::MOD_START && input2 < UnaryMode::MOD_END)
			{
				result = input1 % muldiv_tbl[input2 - UnaryMode::MOD_START];
			}
			else if (input2 == UnaryMode::LSL_0 || input2 == UnaryMode::LSR_0)
			{
				result = input1;
			}
			else if (input2 >= UnaryMode::LSL_1 && \
					input2 <= UnaryMode::LSL_7)
			{
				int shift = (input2 - UnaryMode::LSL_1);
				if (input1 & (1 << (7 - shift))) sreg |= SREG::C;
				result = input1 << shift;
			}
			else if (input2 >= UnaryMode::LSR_1 && \
					input2 <= UnaryMode::LSR_7)
			{
				int shift = (input2 - UnaryMode::LSR_1);
				if (input1 & (1 << shift)) sreg |= SREG::C;
				result = input1 >> shift;
			}
			else switch(input2)
			{
				case UnaryMode::SWAP_NIBBLES:
				{
					result = (input1 & 0xf0) >> 4;
					result |= (input1 & 0x0f) << 4;
				}
				break;
				
				case UnaryMode::REVERSE_BIT_ORDER:
				{
					int mask1 = 0x80;
					int mask2 = 0x01;
					result = 0;
					while(mask1)
					{
						if (input1 & mask1) result |= mask2;
						mask1 >>= 1;
						mask2 <<= 1;
					}
				}
				break;
				
				case UnaryMode::NEGATE:
				{
					int8_t temp = (int8_t)input1;
					temp = -temp;
					result = (uint8_t)temp;
				}
				break;
				
				case UnaryMode::INVERT:
				{
					result = input1 ^ 0xff;
				}
				break;
				
				default:
				{
					// an unused unary mode was specified
					result = 0xff;
					sreg = 7;
					no_auto_sreg = true;
				}
				break;
			}
		}
		break;
		
		default:
			staterr("unhandled ALU mode %d", mode);
			return 1;
	}
	
	if (!no_auto_sreg)
	{
		if ((result & 0x80) != (input1 & 0x80)) sreg |= SREG::V;		// V=sign changed
		if (result == 0) sreg |= SREG::Z;	// Z set if result = 0
	}
	
	if (result_out) *result_out = result;
	if (sreg_out) *sreg_out = sreg;
	return 0;
}


/*
void c------------------------------() {}
*/

bool ALUCompiler::load_io_defs(const char *fname, int iotype)
{
	ControlDefParser *parser = new ControlDefParser();
	ControlDefList *controls = parser->ParseFile(fname, iotype, \
					(iotype == INPUT) ? ALU_NUM_INPUT_PINS : ALU_NUM_OUTPUT_PINS);
	delete parser;
	
	if (!controls)
	{
		delete controls;
		return 1;
	}
	
	// map the controls defined in the file to their variables
	stat("Finding needed %ss...", DescribeIOType(iotype));
	if (iotype == INPUT)
	{
		if (controls->MapToVars(
				"INPUT_1", &io.input1,
				"INPUT_2", &io.input2,
				"MODE", &io.mode,
				NULL
		)) return 1;
	}
	else
	{
		if (controls->MapToVars(
				"RESULT", &io.result,
				"SREG", &io.sreg,
				NULL
		)) return 1;
	}
	
	// save the ControlDefList so that we can delete it and the controldefs in destructor
	ctrlDefLists[iotype] = controls;
	stat("");
	return 0;
}

bool ALUCompiler::save_rom()
{
const char *fname = "../microcode/alu.bin";
FILE *fp;
	
	fp = fopen(fname, "wb");
	if (!fp)
	{
		staterr("unable to open output file: %s: %s", fname, strerror(errno));
		return 1;
	}
	
	for(int i=0;i<alu_table_size_words;i++)
	{
		uint16_t value = alu_table[i];
		fputc((value & 0xff00) >> 8, fp);
		fputc(value & 0xff, fp);
	}
	
	fclose(fp);
	
	stat("saved %s: %d words.", fname, alu_table_size_words);
	return 0;
}

/*
void c------------------------------() {}
*/

const char *describe_sreg(uint8_t sreg)
{
	if (sreg == 0)
		return "-";
	
	DString str;
	if (sreg & SREG::Z) str.AppendChar('Z');
	if (sreg & SREG::C) str.AppendChar('C');
	if (sreg & SREG::V) str.AppendChar('V');
	
	return str.StaticString();
}
