/*
	LIFE1 CPU main microcode source
	(L)ogic-gate (I)nstruction (F)etcher and (E)xecutor
	2018- Caitlin Shaw
*/

inputs inputs.def
outputs outputs.def

include "aluUnaryDefs.mc"
include "foreach.mc"
include "misc.mc"

// == CLR instruction ========================================================
// set a register to 0
macro op_CLR_reg $REG
	opcode CLR $REG
		@fetch
		IDBUS_SRC(VALUE), VALUE(0), LATCH($REG), GOTO(NEXTOP)
	end
endm

@foreach_reg op_CLR_reg

// clear a memory address to 0
opcode CLR parm16
	@fetch
	@fetchparm USERADDR_LO
	@fetchparm USERADDR_HI
	IDBUS_SRC(VALUE), VALUE(0), ADDRBUS_SRC(USERADDR), LATCH(MEMORY_WRITE), GOTO(NEXTOP)
end

// == SET instruction ========================================================
// set a register to 1
macro op_SET_reg $REG
	opcode SET $REG
		@fetch
		IDBUS_SRC(VALUE), VALUE(1), LATCH($REG), GOTO(NEXTOP)
	end
endm

@foreach_reg op_SET_reg

// set a memory address to 1
opcode SET parm16
	@fetch
	@fetchparm USERADDR_LO
	@fetchparm USERADDR_HI
	IDBUS_SRC(VALUE), VALUE(1), ADDRBUS_SRC(USERADDR), LATCH(MEMORY_WRITE), GOTO(NEXTOP)
end

// == MOV instruction ========================================================
// copy one register to another
opcode MOV dstreg, srcreg
	@fetch
	@fetchparm PARM
	IDBUS_FROM(PARMSRC), LATCH_FROM(PARMDST), GOTO(NEXTOP)
end

// 1-byte instruction for all general purpose registers
// this uses up a LOT of opcodes (56 of them!), but it may be worth it since it's
// such a common instruction. Saves 1 byte and executes in 2 cycles instead of 3.
macro op_MOV_reg2reg_short $DSTREG, $SRCREG
	opcode MOV $DSTREG, $SRCREG
		@fetch
		IDBUS_SRC($DSTREG), LATCH($SRCREG), GOTO(NEXTOP)
	end
endm

@foreach_reg_combination op_MOV_reg2reg_short

// == LDI instruction =========================================================
// set a register to an immediate value
macro op_LDI $REG
	opcode LDI $REG, parm8
		@fetch
		//@fetchparm $REG, GOTO(NEXTOP)
		ADDRBUS_SRC(PC), IDBUS_SRC(MEMORY_READ), LATCH($REG), INC_PC, GOTO(NEXTOP)
	end
endm

@foreach_reg op_LDI

// == LDM instruction ========================================================
// read the contents of a specified memory address into a register
macro op_LDM_absolute $DSTREG
	opcode LD $DSTREG, parm16
		@fetch
		@fetchparm USERADDR_LO
		@fetchparm USERADDR_HI
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH($DSTREG), GOTO(NEXTOP)
	end
endm

@foreach_reg op_LDM_absolute

// read memory at a specified offset from SP into a register
opcode LDM dstreg, [SP+parm8]
	@fetch
	@fetchparm ALU_INPUT_2
	
	// add the parameter to SP and get the result into USERADDR
	IDBUS_SRC(SPL), LATCH(ALU_INPUT_1)
	ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), LATCH(USERADDR_LO), LATCH_SREG
	if [ C ]
	{
		IDBUS_SRC(SPH), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(USERADDR_HI)
	}
	else
	{
		IDBUS_SRC(SPH), LATCH(USERADDR_HI)
	}
	
	// fetch from the computed location
	ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH_FROM(PARMDST), GOTO(NEXTOP)
end

// read memory at (SP+reg) into a register
opcode LDM dstreg, [SP+srcreg]
	@fetch
	@fetchparm PARM
	IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_2)
	
	// add the parameter to SP and get the result into USERADDR
	IDBUS_SRC(SPL), LATCH(ALU_INPUT_1)
	ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), LATCH(USERADDR_LO), LATCH_SREG
	if [ C ]
	{
		IDBUS_SRC(SPH), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(USERADDR_HI)
	}
	else
	{
		IDBUS_SRC(SPH), LATCH(USERADDR_HI)
	}
	
	// fetch from the computed location
	ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH_FROM(PARMDST), GOTO(NEXTOP)
end

// read two bytes from an absolute address into a register pair
macro op_LDMW_absolute $DSTLO, $DSTHI
	opcode LDMW $DSTLO:$DSTHI, parm16
		@fetch
		@fetchparm tr0
		@fetchparm tr1
		
		// fetch first byte
		IDBUS_SRC(tr0), LATCH(USERADDR_LO)
		IDBUS_SRC(tr1), LATCH(USERADDR_HI)
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH($DSTLO)
		
		// increment the address and fetch 2nd byte
		IDBUS_SRC(tr0), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(USERADDR_LO), LATCH_SREG
		if [ C ]
		{
			IDBUS_SRC(tr1), LATCH(ALU_INPUT_1)
			ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(USERADDR_HI)
		}
		
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH($DSTHI), GOTO(NEXTOP)
	end
endm

@foreach_regpair op_LDMW_absolute

// read the contents of the memory address contained in a register pair into a register
opcode LDM dstreg, [srcreg:]
	@fetch
	ADDRBUS_SRC(PC), IDBUS_SRC(MEMORY_READ), LATCH(ALU_INPUT_1)		// read a 2nd copy of PARM
	@fetchparm PARM													// read PARM
	
	// copy byte in first register to LSB of address
	IDBUS_FROM(PARMSRC), LATCH(USERADDR_LO)
	
	// increment parm to point PARMSRC at next register
	IDBUS_SRC(ALU_RESULT), ALU_MODE(INC), LATCH(PARM)
	
	// copy byte in second register to MSB of address
	IDBUS_FROM(PARMSRC), LATCH(USERADDR_HI)
	
	// read resulting address into destination register
	ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH_FROM(PARMDST), GOTO(NEXTOP)
end

// == STM instruction ========================================================

// write a register to a specified immediate memory address
macro op_STM_reg_to_mem $REG
	opcode STM parm16, $REG
		@fetch
		@fetchparm USERADDR_LO
		@fetchparm USERADDR_HI
		IDBUS_SRC($REG), ADDRBUS_SRC(USERADDR), LATCH(MEMORY_WRITE), GOTO(NEXTOP)
	end
endm

@foreach_reg op_STM_reg_to_mem

/*
// write a register to the memory address contained in a register pair
macro op_STM_regpair $SRCREG, $PTRHI, $PTRLO
	opcode STM [$PTRHI:$PTRLO], $SRCREG
		@fetch
		IDBUS_SRC($PTRHI), LATCH(USERADDR_HI)
		IDBUS_SRC($PTRLO), LATCH(USERADDR_LO)
		IDBUS_SRC($SRCREG), LATCH(MEMORY_WRITE), GOTO(NEXTOP)
	end
endm

@foreach_reg_regpair op_STM_regpair
*/

// write an immediate value to a memory address
opcode STM parm16, parm8
	@fetch
	@fetchparm USERADDR_LO
	@fetchparm USERADDR_HI
	@fetchparm tr0
	IDBUS_SRC(tr0), ADDRBUS_SRC(USERADDR), LATCH(MEMORY_WRITE), GOTO(NEXTOP)
end

// write a register into memory at a specified offset from SP
opcode STM [SP+parm8], srcreg
	@fetch
	@fetchparm PARM				// get srcreg
	@fetchparm ALU_INPUT_2		// get the absolute offset from SP
	
	// add the parameter to SP and get the result into USERADDR
	IDBUS_SRC(SPL), LATCH(ALU_INPUT_1)
	ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), LATCH(USERADDR_LO), LATCH_SREG
	if [ C ]
	{
		IDBUS_SRC(SPH), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(USERADDR_HI)
	}
	else
	{
		IDBUS_SRC(SPH), LATCH(USERADDR_HI)
	}
	
	// store to the computed location
	ADDRBUS_SRC(USERADDR), LATCH(MEMORY_WRITE), IDBUS_FROM(PARMSRC), GOTO(NEXTOP)
end

// write a register into memory at (SP+reg)
opcode STM [SP+dstreg], srcreg
	@fetch
	IDBUS_FROM(PARMDST), LATCH(ALU_INPUT_2)
	
	// add the parameter to SP and get the result into USERADDR
	IDBUS_SRC(SPL), LATCH(ALU_INPUT_1)
	ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), LATCH(USERADDR_LO), LATCH_SREG
	if [ C ]
	{
		IDBUS_SRC(SPH), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(USERADDR_HI)
	}
	else
	{
		IDBUS_SRC(SPH), LATCH(USERADDR_HI)
	}
	
	// store to the computed location
	ADDRBUS_SRC(USERADDR), LATCH(MEMORY_WRITE), IDBUS_FROM(PARMSRC), GOTO(NEXTOP)
end

// == Math instructions, register-to-register ================================

// -- ADD, SUB, and AND
macro math_reg_to_reg $MATHOP
	opcode $MATHOP dstreg, srcreg
		@fetch
		@fetchparm PARM
		IDBUS_FROM(PARMDST), LATCH(ALU_INPUT_1)
		IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_2)
		ALU_MODE($MATHOP), IDBUS_SRC(ALU_RESULT), LATCH_FROM(PARMDST), GOTO(NEXTOP)
	end
endm

@math_reg_to_reg ADD
@math_reg_to_reg SUB
@math_reg_to_reg AND

// -- simulated OR instruction
opcode OR dstreg, srcreg
	@fetch
	@fetchparm PARM
	// simulate a hardware OR using addition.
	// if we AND out all the bits that we are going to set, then there
	// can be no carries and then '+' operator will be equivalent to OR.
	// A | B = (A & NOT B) + B
	// NOT B = (0xFF - B)
	
	// calculate negation of B
	IDBUS_SRC(VALUE), VALUE(0xff), LATCH(ALU_INPUT_1)
	IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_2)
	ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH(ALU_INPUT_2)
	
	// calculate A AND ~B - mask all bits set in B out of A
	IDBUS_FROM(PARMDST), LATCH(ALU_INPUT_1)
	ALU_MODE(AND), IDBUS_SRC(ALU_RESULT), LATCH(ALU_INPUT_1)
	
	// load original non-inverted B back and add it to
	// the masked-off A to get the result
	IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_2)
	ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), LATCH_FROM(PARMDST), GOTO(NEXTOP)
end

// -- simulated XOR instruction
opcode XOR dstreg, srcreg
	@fetch
	@fetchparm PARM
	// simulate a hardware XOR
	// uses the code for OR and this DeMorgan theorem:
	// A ^ B = (A | B) & ~(A AND B)
	// btw it also equals: (!X AND Y) OR (X AND !Y)
	
	// -- first tackle the (A | B). It's just like the OR instruction.
	// calculate negation of B
	IDBUS_SRC(VALUE), VALUE(0xff), LATCH(ALU_INPUT_1)
	IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_2)
	ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH(ALU_INPUT_2)
	
	// calculate A AND ~B - mask all bits set in B out of A
	IDBUS_FROM(PARMDST), LATCH(ALU_INPUT_1)
	ALU_MODE(AND), IDBUS_SRC(ALU_RESULT), LATCH(ALU_INPUT_1)
	
	// load original non-inverted B back and add it to
	// the masked-off A to get the result
	// save the result of the (A | B) part of the equation in tr0
	IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_2)
	ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), LATCH(tr0)
	
	// -- now let's do (A AND B). This one is easy.
	// B is already loaded from calculation above so just reload A.
	IDBUS_FROM(PARMDST), LATCH(ALU_INPUT_1)
	ALU_MODE(AND), IDBUS_SRC(ALU_RESULT), LATCH(ALU_INPUT_2)
	
	// we have A AND B in ALU_INPUT_2. Negate it now.
	IDBUS_SRC(VALUE), VALUE(0xff), LATCH(ALU_INPUT_1)
	ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH(ALU_INPUT_2)
	
	// we have ~(A AND B) in ALU_INPUT_2. Let's AND with the (A | B)
	// result from earlier to get final result and our XOR operation.
	IDBUS_SRC(tr0), LATCH(ALU_INPUT_1)
	ALU_MODE(AND), IDBUS_SRC(ALU_RESULT), LATCH_FROM(PARMDST), GOTO(NEXTOP)
end

// -- ADC and SBC
opcode ADC dstreg, srcreg
	@fetch
	@fetchparm PARM
	IDBUS_FROM(PARMDST), LATCH(ALU_INPUT_1), LATCH_SREG
	IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_2)
	if [ C ]
	{
		ALU_MODE(ADD_AND_INC), IDBUS_SRC(ALU_RESULT), LATCH_FROM(PARMDST), GOTO(NEXTOP)
	}
	else
	{
		ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), LATCH_FROM(PARMDST), GOTO(NEXTOP)
	}
end

opcode SBC dstreg, srcreg
	@fetch
	@fetchparm PARM
	IDBUS_FROM(PARMDST), LATCH(ALU_INPUT_1), LATCH_SREG
	IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_2)
	if [ C ]
	{
		ALU_MODE(SUB_AND_DEC), IDBUS_SRC(ALU_RESULT), LATCH_FROM(PARMDST), GOTO(NEXTOP)
	}
	else
	{
		ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH_FROM(PARMDST), GOTO(NEXTOP)
	}
end

// -- INC and DEC
macro create_inc $REG
	opcode INC $REG
		@fetch
		IDBUS_SRC($REG), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH($REG), GOTO(NEXTOP)
	end
endm

macro create_dec $REG
	opcode DEC $REG
		@fetch
		IDBUS_SRC($REG), LATCH(ALU_INPUT_1)
		ALU_MODE(DEC), IDBUS_SRC(ALU_RESULT), LATCH($REG), GOTO(NEXTOP)
	end
endm

@foreach_reg create_inc
@foreach_reg create_dec

// -- extended ALU instruction
// perform the operation "parm8" on srcreg and place the result in dstreg.
// dstreg and srcreg can also be the same-- but this instruction provides the
// ability to output to a different register than the source, unlike most
// other instructions, since it doesn't cost instruction length or cycle count.
opcode ALU dstreg, srcreg, parm8
	@fetch
	@fetchparm PARM
	@fetchparm ALU_INPUT_2
	IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_1)
	ALU_MODE(UNARY), IDBUS_SRC(ALU_RESULT), LATCH_FROM(PARMDST), GOTO(NEXTOP)
end

// == Comparison instructions ================================================

// CMP - Compare register with register
opcode CMP dstreg, srcreg
	@fetch
	@fetchparm PARM
	IDBUS_FROM(PARMDST), LATCH(ALU_INPUT_1)
	IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_2)
	ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH(NONE), GOTO(NEXTOP)
end

// CMP - Compare register with immediate
macro op_CMP_immediate $REG
	opcode CMP $REG, parm8
		@fetch
		@fetchparm ALU_INPUT_2
		IDBUS_SRC($REG), LATCH(ALU_INPUT_1)
		ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH(NONE), GOTO(NEXTOP)
	end
endm

@foreach_reg op_CMP_immediate

// CMPM - Compare register with memory
opcode CMPM dstreg, parm16
	@fetch
	@fetchparm PARM
	@fetchparm USERADDR_LO
	@fetchparm USERADDR_HI
	
	IDBUS_FROM(PARMDST), LATCH(ALU_INPUT_1)
	IDBUS_SRC(MEMORY_READ), ADDRBUS_SRC(USERADDR), LATCH(ALU_INPUT_2)
	ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH(NONE), GOTO(NEXTOP)
end

// == Conditional branch instructions ========================================

macro condbranch_body
	{
		@fetchparm tr1
		@fetchparm tr0
		IDBUS_SRC(tr0), LATCH(PC_HI)
		IDBUS_SRC(tr1), LATCH(PC_LO), GOTO(NEXTOP)
	}
	else
	{
		INC_PC
		INC_PC, GOTO(NEXTOP)
	}
endm

opcode BEQ parm16		// ==
	@fetch
	LATCH_SREG
	if [ Z ]
	@condbranch_body
end
//alias BZ BEQ

opcode BNE parm16		// !=
	@fetch
	LATCH_SREG
	if [ Z ! ]
	@condbranch_body
end
//alias BNZ BNE

opcode BGT parm16		// > (unsigned)		- alias: BCS, Branch if Carry Set
	@fetch
	LATCH_SREG
	if [ C ]
	@condbranch_body
end
//alias BCS BGT

opcode BGE parm16		// >= (unsigned)
	@fetch
	LATCH_SREG
	if [ C Z || ]
	@condbranch_body
end

opcode BLT parm16		// < (unsigned)		- alias: BCC, Branch if Carry Clear
	@fetch
	LATCH_SREG
	if [ C ! ]
	@condbranch_body
end
//alias BCC BLT

opcode BLE parm16		// <= (unsigned)
	@fetch
	LATCH_SREG
	if [ C Z || ! ]
	@condbranch_body
end

// == Unconditional branch instructions ======================================

// JMP			jump to a direct address
opcode JMP parm16
	@fetch
	@fetchparm tr1
	@fetchparm tr0
	IDBUS_SRC(tr0), LATCH(PC_HI)
	IDBUS_SRC(tr1), LATCH(PC_LO), GOTO(NEXTOP)
end

// JMP r0:r1	jump to the address contained in a register pair

// BLR			save return address in LR (link register) and jump
opcode BLR parm16
	@fetch
	@fetchparm tr1
	@fetchparm tr0
	ADDRBUS_SRC(PC), IDBUS_SRC(ADDRBUS_HI), LATCH(LRH)
	ADDRBUS_SRC(PC), IDBUS_SRC(ADDRBUS_LO), LATCH(LRL)
	IDBUS_SRC(tr0), LATCH(PC_HI)
	IDBUS_SRC(tr1), LATCH(PC_LO), GOTO(NEXTOP)
end

// RTLR			return from a BRL instuction (equivalent to mov pc, lr)
opcode RTLR
	@fetch
	IDBUS_SRC(LRH), LATCH(PC_HI)
	IDBUS_SRC(LRL), LATCH(PC_LO), GOTO(NEXTOP)
end

// JSR			jump and push return address to stack
// RTS			return from a JSR instruction

// == Stack instructions =====================================================

// PUSH a register
macro op_PUSH $REG
	opcode PUSH $REG
		@fetch
		
		// write REG to memory pointed to by SP
		IDBUS_SRC(SPH), LATCH(USERADDR_HI)
		IDBUS_SRC(SPL), LATCH(USERADDR_LO)
		IDBUS_SRC($REG), LATCH(MEMORY_WRITE), ADDRBUS_SRC(USERADDR)
		
		// decrement SP
		IDBUS_SRC(SPL), LATCH(ALU_INPUT_1)
		ALU_MODE(DEC), IDBUS_SRC(ALU_RESULT), LATCH(SPL), LATCH_SREG
		if [ C ]
		{
			IDBUS_SRC(SPH), LATCH(ALU_INPUT_1)
			ALU_MODE(DEC), IDBUS_SRC(ALU_RESULT), LATCH(SPH), GOTO(NEXTOP)
		}
		else
		{
			GOTO(NEXTOP)
		}
	end
endm

@foreach_reg op_PUSH

// POP a register
macro op_POP $REG
	opcode POP $REG
		@fetch
		
		// increment SP
		IDBUS_SRC(SPL), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(SPL), LATCH_SREG
		if [ C ]
		{
			IDBUS_SRC(SPH), LATCH(ALU_INPUT_1)
			ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(SPH)
		}
		
		// read REG from memory pointed to by SP
		IDBUS_SRC(SPH), LATCH(USERADDR_HI)
		IDBUS_SRC(SPL), LATCH(USERADDR_LO)
		IDBUS_SRC(MEMORY_READ), ADDRBUS_SRC(USERADDR), LATCH($REG), GOTO(NEXTOP)
	end
endm

@foreach_reg op_POP

// == Fetching and setting status register ===================================

// PUSH SREG
opcode PUSH SREG
	@fetch
	
	@fetch_sreg tr0
	@push tr0
	
	// restore original contents of SREG that our ALU usage messed up
	IDBUS_SRC(tr0), LATCH(ALU_INPUT_1)
	ALU_MODE(UNARY), IDBUS_SRC(ALU_RESULT), GOTO(NEXTOP)
end

// POP SREG
opcode POP SREG
	@fetch
	
	// increment SP
	@inc_sp
	
	// load SP into USERADDR
	IDBUS_SRC(SPH), LATCH(USERADDR_HI)
	IDBUS_SRC(SPL), LATCH(USERADDR_LO)
	
	// fetch new SREG from stack and set SREG using ALU special function
	// (note; if value of byte popped is outside valid range of 0-15,
	// result of POP SREG is undefined)
	ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH(ALU_INPUT_1)
	ALU_MODE(UNARY), IDBUS_SRC(ALU_RESULT), GOTO(NEXTOP)
end

// == Misc instructions ======================================================

// fills countreg bytes with the contents of valuereg beginning at the specified absolute address.
// if countreg is 0, fills 256 bytes. countreg will be 0 after the instruction completes.
//
// the order of operands is exactly like the C function:
//	memset addr, valuereg, countreg
///////////////////// BROKEN fixme
opcode MEMSET parm16
	@fetch
	@fetchparm PARM		// get which regs will be valuereg & countreg
	@fetchparm tr1		// LSB of start addr
	@fetchparm tr0		// MSB of start addr
	
	// write r0 to (tr0:tr1)
	IDBUS_SRC(tr0), LATCH(USERADDR_HI)
	IDBUS_SRC(tr1), LATCH(USERADDR_LO)
	IDBUS_SRC(r0), ADDRBUS_SRC(USERADDR), LATCH(MEMORY_WRITE)
	
	// decrement r1 counter and exit if it becomes 0
	IDBUS_SRC(r1), LATCH(ALU_INPUT_1)
	ALU_MODE(DEC), IDBUS_SRC(ALU_RESULT), LATCH(r1), LATCH_SREG
	if [ Z ]
	{
		GOTO(NEXTOP)
	}
	else
	{
		// setup for increment below. needed here because we are about to latch sreg again
		// and so phases need to be in sync regardless of which branch was taken here
		IDBUS_SRC(tr1), LATCH(ALU_INPUT_1)
	}
	
	// increment address
	ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(tr1), LATCH_SREG
	if [ C ]
	{
		IDBUS_SRC(SPH), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(SPH), GOTO(3)
	}
	
	GOTO(3)
end

