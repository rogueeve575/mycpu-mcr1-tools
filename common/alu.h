
#ifndef _ALUCOMPILER_H
#define _ALUCOMPILER_H

#define ALU_NUM_INPUT_PINS		20
#define ALU_NUM_OUTPUT_PINS		16

class ALUCompiler
{
public:
	ALUCompiler();
	~ALUCompiler();
	
	bool Generate();
	
private:
	bool populate_alu_table();
	bool save_rom();
	
	bool load_io_defs(const char *fname, int iotype);
	
private:
	struct
	{
		// inputs
		ControlDef *input1;
		ControlDef *input2;
		ControlDef *mode;
		
		// outputs
		ControlDef *result;
		ControlDef *sreg;
	} io;
	
	ControlDefList *ctrlDefLists[2];
	
	uint16_t *alu_table;	// 8 bits of result plus 3 bits of SREG for every combination of inputs
	int alu_table_size_words;
	int alu_table_size_bytes;
};

namespace ALU
{
	namespace ALUMode
	{
		enum
		{
			ADD = 0,
			SUB = 1,
			ADD_AND_INC = 2,		// add inputs 1&2 and then add 1 to the result. used for carry during ADC.
			SUB_AND_DEC = 3,		// subtract inputs 1&2 and then subtract 1 from the result. used for carry during SBC.
			AND = 4,
			INC = 5,
			DEC = 6,
			UNARY = 7				// extended unary operations. ALU_INPUT_2 sets the mode; ALU_RESULT is an operation on input 1
		};
	}
	
	namespace UnaryMode
	{
		enum
		{
			// the operations start with 8 ops for the 8 possible combinations of Z,C,V.
			// These ops set SREG as requested, according to: Z=0x01 C=0x02 V=0x04
			// This allows to restore a previously-saved SREG register simply by passing it in as a unary operation #.
			SET_SREG_0, SET_SREG_1, SET_SREG_2, SET_SREG_3,
			SET_SREG_4, SET_SREG_5, SET_SREG_6, SET_SREG_7,
			
			SETBIT_0, SETBIT_1, SETBIT_2, SETBIT_3, SETBIT_4, SETBIT_5, SETBIT_6, SETBIT_7,
			CLRBIT_0, CLRBIT_1, CLRBIT_2, CLRBIT_3, CLRBIT_4, CLRBIT_5, CLRBIT_6, CLRBIT_7,
			
			// returns either 1 or 0 in both result and SREG Z depending upon whether the
			// specified bit is set in input1.
			GETBIT_0, GETBIT_1, GETBIT_2, GETBIT_3, GETBIT_4, GETBIT_5, GETBIT_6, GETBIT_7,
			
			MUL_START_LO, MUL_3_LO, MUL_5_LO, MUL_10_LO, MUL_15_LO, MUL_50_LO, MUL_60_LO, MUL_80_LO, MUL_100_LO, MUL_END_LO,
			MUL_START_HI, MUL_3_HI, MUL_5_HI, MUL_10_HI, MUL_15_HI, MUL_50_HI, MUL_60_HI, MUL_80_HI, MUL_100_HI, MUL_END_HI,
			
			DIV_START, DIV_3, DIV_5, DIV_10, DIV_15, DIV_50, DIV_60, DIV_80, DIV_100, DIV_END,
			MOD_START, MOD_3, MOD_5, MOD_10, MOD_15, MOD_50, MOD_60, MOD_80, MOD_100, MOD_END,
			
			// Logical shift left with carry out
			// For shifts greater than 1, the carry becomes the last bit shifted out.
			// The shift by 0 is a no-op, provided for convenience.
			LSL_0, LSL_1, LSL_2, LSL_3, LSL_4, LSL_5, LSL_6, LSL_7,
			
			// Logical shift right with carry out
			LSR_0, LSR_1, LSR_2, LSR_3, LSR_4, LSR_5, LSR_6, LSR_7,
			
			SWAP_NIBBLES,			// swaps the high and lo nibbles of input byte
			REVERSE_BIT_ORDER,		// reverses the bits to backwards order so LSB becomes MSB and vice versa
			NEGATE,					// returns the 2's complement negation of input
			INVERT,					// invert all bits (1's complement)
			SIN,
			
			LAST
		};
	}
	
	namespace SREG
	{
		enum	// SREG
		{
			Z = 0x01,
			C = 0x02,
			V = 0x04,
			N = 0x08,
			
			//I = 0x40,
			//P = 0x80
		};
	}
}

const char *describe_sreg(uint8_t sreg);

bool alu_compute(uint8_t input1, uint8_t input2, uint8_t mode,
				uint8_t *result_out, uint8_t *sreg_out);

#endif
