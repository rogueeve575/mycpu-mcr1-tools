
// prefix code of nearly every intruction. Fetches the next instruction into opcode and increments PC.
macro fetch
	ADDRBUS_SRC(PC), IDBUS_SRC(MEMORY_READ), LATCH(OPCODE), INC_PC
endm

// fetch byte at PC into specified register and increment PC
macro fetchparm $REG
	ADDRBUS_SRC(PC), IDBUS_SRC(MEMORY_READ), LATCH($REG), INC_PC
endm

// fetch byte at PC into specified register and leave PC alone. Good for when you want two copies of a parameter.
macro fetchparm_noinc $REG
	ADDRBUS_SRC(PC), IDBUS_SRC(MEMORY_READ), LATCH($REG)
endm

macro dec_sp
	IDBUS_SRC(SPL), LATCH(ALU_INPUT_1)
	ALU_MODE(DEC), LATCH(SPL), LATCH_SREG
	if [ C ]
	{
		IDBUS_SRC(SPH), LATCH(ALU_INPUT_1)
		ALU_MODE(DEC), LATCH(SPH), LATCH_SREG
	}
endm

macro inc_sp
	IDBUS_SRC(SPL), LATCH(ALU_INPUT_1)
	ALU_MODE(INC), LATCH(SPL), LATCH_SREG
	if [ C ]
	{
		IDBUS_SRC(SPH), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), LATCH(SPH), LATCH_SREG
	}
endm

macro push $ARG
	IDBUS_SRC(SPL), LATCH(USERADDR_LO)
	IDBUS_SRC(SPH), LATCH(USERADDR_HI)
	ADDRBUS_SRC(USERADDR), IDBUS_SRC($ARG), LATCH(MEMORY_WRITE)
	@dec_sp
endm

macro pop $ARG
	@inc_sp
	IDBUS_SRC(SPL), LATCH(USERADDR_LO)
	IDBUS_SRC(SPH), LATCH(USERADDR_HI
	ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH($ARG)
endm

macro fetch_sreg $ARG
	// Z=1 C=2 V=4 N=8
	// there sre 15 possible values that SREG can have.
	// we'll just provide code to return every one of them.
	
	if [Z 0 = C 0 = && V 0 = && N 0 = &&]	// 0
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(0), LATCH($ARG)
	}
	if [Z 1 = C 0 = && V 0 = && N 0 = &&]	// 1
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(1), LATCH($ARG)
	}
	if [Z 0 = C 1 = && V 0 = && N 0 = &&]	// 2
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(2), LATCH($ARG)
	}
	if [Z 1 = C 1 = && V 0 = && N 0 = &&]	// 3
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(3), LATCH($ARG)
	}
	if [Z 0 = C 0 = && V 1 = && N 0 = &&]	// 4
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(4), LATCH($ARG)
	}
	if [Z 1 = C 0 = && V 1 = && N 0 = &&]	// 5
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(5), LATCH($ARG)
	}
	if [Z 0 = C 1 = && V 1 = && N 0 = &&]	// 6
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(6), LATCH($ARG)
	}
	if [Z 1 = C 1 = && V 1 = && N 0 = &&]	// 7
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(7), LATCH($ARG)
	}
	
	if [Z 0 = C 0 = && V 0 = && N 1 = &&]	// 8
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(8), LATCH($ARG)
	}
	if [Z 1 = C 0 = && V 0 = && N 1 = &&]	// 9
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(9), LATCH($ARG)
	}
	if [Z 0 = C 1 = && V 0 = && N 1 = &&]	// 10
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(10), LATCH($ARG)
	}
	if [Z 1 = C 1 = && V 0 = && N 1 = &&]	// 11
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(11), LATCH($ARG)
	}
	if [Z 0 = C 0 = && V 1 = && N 1 = &&]	// 12
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(12), LATCH($ARG)
	}
	if [Z 1 = C 0 = && V 1 = && N 1 = &&]	// 13
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(13), LATCH($ARG)
	}
	if [Z 0 = C 1 = && V 1 = && N 1 = &&]	// 14
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(14), LATCH($ARG)
	}
	if [Z 1 = C 1 = && V 1 = && N 1 = &&]	// 15
	{
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(VALUE), VALUE(15), LATCH($ARG)
	}
endm

macro push_sreg
	IDBUS_SRC(SPL), LATCH(USERADDR_LO)
	IDBUS_SRC(SPH), LATCH(USERADDR_HI)
	@fetch_sreg MEMORY_WRITE
	@dec_sp
endm

// ---------------------------------------

// Resets the CPU. This opcode is forced into the OPCODE register
// on hardware reset.
opcode 0x00 RST
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	// fetch PC from reset vector
	IDBUS_SRC(VALUE), VALUE(0xff), LATCH(USERADDR_HI)
	IDBUS_SRC(VALUE), VALUE(0xfe), LATCH(USERADDR_LO)
	ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH(PC_LO)
	IDBUS_SRC(VALUE), VALUE(0xff), LATCH(USERADDR_LO)
	ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH(PC_HI)

	IDBUS_SRC(VALUE), VALUE(0x7f), LATCH(SPH)
	IDBUS_SRC(VALUE), VALUE(0xff), LATCH(SPL)
	
	ADDRBUS_SRC(PC), IDBUS_SRC(MEMORY_READ), LATCH(OPCODE), GOTO(0)
end

// generate an interrupt request.
// this opcode is forced into the OPCODE register when an interrupt
// is pending and a new instruction is about to start (technically,
// when we enter T0 state and NEXT_MIP is 0).
opcode 0x80 IRQ
	// push the PC onto the stack, high byte first then low byte,
	// so that it appears little-end in memory.
	
	// determine what the status register is and write it to SP.
	// decrement the SP again.
	
	// jump to the interrupt vector
end

// == SBR/CBR instructions ===================================================

// -- SBR		set bit in register
// -- CBR		clear bit in register
// I put them here because of the unusual implementation of the "SBR/CBR reg, reg" version,
// which uses the immediate version's code and thus needs to have it's opcode number hardcoded.
// This way, the opcode # will be less likely to change as the microcode is updated, preventing
// accidental breakages of those instructions.

// set or clear bit in register according to an immediate value
macro op_SBR_CBR_immediate $MNEMONIC, $SHIFT, $UNARYOP
	opcode $MNEMONIC dstreg, $SHIFT
		@fetch
		ADDRBUS_SRC(PC), IDBUS_SRC(MEMORY_READ), LATCH(PARM), INC_PC, GOTO(10)
		
		// gap for register version
		NOP		// 2
		NOP		// 3
		NOP		// 4
		NOP		// 5
		NOP		// 6
		NOP		// 7
		NOP		// 8
		IDBUS_SRC(ALU_RESULT), ALU_MODE(ADD), LATCH(OPCODE)		// 9- entry point of register version
		
		// 10- shared code between immediate and register versions
		IDBUS_FROM(PARMDST), LATCH(ALU_INPUT_1)
		IDBUS_SRC(VALUE), VALUE($UNARYOP), LATCH(ALU_INPUT_2)
		IDBUS_SRC(ALU_RESULT), ALU_MODE(UNARY), LATCH_FROM(PARMDST), GOTO(NEXTOP)
	end
endm

@op_SBR_CBR_immediate SBR, 0, UNARY>>SETBIT_0
@op_SBR_CBR_immediate SBR, 1, UNARY>>SETBIT_1
@op_SBR_CBR_immediate SBR, 2, UNARY>>SETBIT_2
@op_SBR_CBR_immediate SBR, 3, UNARY>>SETBIT_3
@op_SBR_CBR_immediate SBR, 4, UNARY>>SETBIT_4
@op_SBR_CBR_immediate SBR, 5, UNARY>>SETBIT_5
@op_SBR_CBR_immediate SBR, 6, UNARY>>SETBIT_6
@op_SBR_CBR_immediate SBR, 7, UNARY>>SETBIT_7

@op_SBR_CBR_immediate CBR, 0, UNARY>>CLRBIT_0
@op_SBR_CBR_immediate CBR, 1, UNARY>>CLRBIT_1
@op_SBR_CBR_immediate CBR, 2, UNARY>>CLRBIT_2
@op_SBR_CBR_immediate CBR, 3, UNARY>>CLRBIT_3
@op_SBR_CBR_immediate CBR, 4, UNARY>>CLRBIT_4
@op_SBR_CBR_immediate CBR, 5, UNARY>>CLRBIT_5
@op_SBR_CBR_immediate CBR, 6, UNARY>>CLRBIT_6
@op_SBR_CBR_immediate CBR, 7, UNARY>>CLRBIT_7

// set bit in register according to another register
// unusual experimental implementation--we'll use the srcreg, which is ignored by the immediate
// versions of the instructions, to calculate the opcode number of the corresponding immediate version
// according to the contents of the register, then switch OPCODE to that opcode.
macro op_SBR_CBR_reg $MNEMONIC, $JMPOPNUM
	opcode $MNEMONIC dstreg, srcreg
		@fetch
		@fetchparm PARM
		
		// AND the bit value by 7 to ensure it is valid
		IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_1)
		IDBUS_SRC(VALUE), VALUE(7), LATCH(ALU_INPUT_2)
		IDBUS_SRC(ALU_RESULT), ALU_MODE(AND), LATCH(tr0)
		
		// get the opcode number of the corresponding immediate version, and switch opcodes
		// to the immediate version
		IDBUS_SRC(tr0), LATCH(ALU_INPUT_1)
		IDBUS_SRC(VALUE), VALUE($JMPOPNUM), LATCH(ALU_INPUT_2), GOTO(9)
		
		NOP		// 7
		NOP		// 8
		
		IDBUS_SRC(ALU_RESULT), ALU_MODE(ADD), LATCH(OPCODE)	// switch to corresponding immediate version of opcode
	end
endm

// VALUE is opcode number of "SBR/CBR dstreg, 0". Currently must be maintained if it changes,
// which is why it's in here, so they can be among the first opcode numbers.
@op_SBR_CBR_reg SBR, 0x01
@op_SBR_CBR_reg CBR, 0x09

// -- reserved opcode 0xff
opcode 0xff FF_RESERVED
	@fetch
	GOTO(NEXTOP)
end

opcode 0xf0 NOP
	@fetch
	GOTO(NEXTOP)
end
