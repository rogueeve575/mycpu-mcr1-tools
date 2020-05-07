
// ALU unary operation modes
// (put the mode value into ALU_INPUT_2, then use ALU_MODE(UNARY))
enum UNARY
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
}
