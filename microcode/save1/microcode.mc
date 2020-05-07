/*
	make macro arguments passed as their full token,
	not just the word
*/

inputs inputs.def
outputs outputs.def

include "foreach.mc"
include "misc.mc"

// == CLR instruction ========================================================
// set a register to 0
macro op_CLR_reg $REG
	opcode CLR $REG
		@fetch
		IDBUS_SRC(CONST_00), LATCH($REG), GOTO(NEXTOP)
	end
endm

@foreach_reg op_CLR_reg

// clear a memory address to 0
opcode CLR parm16
	@fetch
	@fetchparm USERADDR_LO
	@fetchparm USERADDR_HI
	IDBUS_SRC(CONST_00), ADDRBUS_SRC(USERADDR), LATCH(MEMORY_WRITE), GOTO(NEXTOP)
end

// == SET instruction ========================================================
// set a register to 1
macro op_SET_reg $REG
	opcode SET $REG
		@fetch
		IDBUS_SRC(CONST_01), LATCH($REG), GOTO(NEXTOP)
	end
endm

@foreach_reg op_SET_reg

// set a memory address to 1
opcode SET parm16
	@fetch
	@fetchparm USERADDR_LO
	@fetchparm USERADDR_HI
	IDBUS_SRC(CONST_01), ADDRBUS_SRC(USERADDR), LATCH(MEMORY_WRITE), GOTO(NEXTOP)
end

// == MOV instruction ========================================================
// copy one register to another
opcode MOV regdst, regsrc
	@fetch
	@fetchparm PARM
	IDBUS_FROM(PARMSRC), LATCH_FROM(PARMDST), GOTO(NEXTOP)
end

/*macro op_MOV_reg_to_reg $DST, $SRC
	opcode MOV $DST, $SRC
		@fetch
		IDBUS_SRC($SRC), LATCH($DST), GOTO(NEXTOP)
	end
endm

@foreach_reg_reg op_MOV_reg_to_reg*/

// == LDI instruction =========================================================
// set a register to an immediate value
macro op_LDI $REG
	opcode LDI $REG, parm8
		@fetch
		ADDRBUS_SRC(PC), IDBUS_SRC(MEMORY_READ), LATCH($REG), INC_PC, GOTO(NEXTOP)
	end
endm

@foreach_reg op_LDI
@op_LDI SPL
@op_LDI SPH

// == LDM instruction ========================================================
// read the contents of a specified memory address into a register
macro op_LDM_immediate $DSTREG
	opcode LDM $DSTREG, parm16
		@fetch
		@fetchparm USERADDR_LO
		@fetchparm USERADDR_HI
		@fetchparm tr0
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH($DSTREG), GOTO(NEXTOP)
	end
endm

@foreach_reg op_LDM_immediate

// read the contents of the memory address contained in a register pair into a register
macro op_LDM_regpair $DSTREG, $PTRHI, $PTRLO
	opcode LDM $DSTREG, [$PTRHI:$PTRLO]
		@fetch
		IDBUS_SRC($PTRHI), LATCH(USERADDR_HI)
		IDBUS_SRC($PTRLO), LATCH(USERADDR_LO)
		ADDRBUS_SRC(USERADDR), IDBUS_SRC(MEMORY_READ), LATCH($DSTREG), GOTO(NEXTOP)
	end
endm

@foreach_reg_regpair op_LDM_regpair

// == STM instruction ========================================================

// write a register to a specified memory address
macro op_STM_reg_to_mem $REG
	opcode STM parm16, $REG
		@fetch
		@fetchparm USERADDR_LO
		@fetchparm USERADDR_HI
		IDBUS_SRC($REG), ADDRBUS_SRC(USERADDR), LATCH(MEMORY_WRITE), GOTO(NEXTOP)
	end
endm

@foreach_reg op_STM_reg_to_mem

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

// write an immediate value to a memory address
opcode STM parm16, parm8
	@fetch
	@fetchparm USERADDR_LO
	@fetchparm USERADDR_HI
	@fetchparm tr0
	IDBUS_SRC(tr0), ADDRBUS_SRC(USERADDR), LATCH(MEMORY_WRITE), GOTO(NEXTOP)
end

// == Math instructions ======================================================

// ADD, SUB, ADC, SBC, AND, INC, DEC, INC/DEC mem, ALU a register with function an immediate value or register
// each for all registers

// ADD, SUB, and AND; reg to reg
macro op_math_2reg $ALUFUNC, $DST, $SRC
	opcode $ALUFUNC $DST, $SRC
		@fetch
		IDBUS_SRC($SRC), LATCH(ALU_INPUT_1)
		IDBUS_SRC($DST), LATCH(ALU_INPUT_2)
		ALU_MODE($ALUFUNC), IDBUS_SRC(ALU_RESULT), LATCH($DST), GOTO(NEXTOP)
	end
endm

macro do_op_math_2reg $DST, $SRC
	@op_math_2reg ADD, $DST, $SRC
	@op_math_2reg SUB, $DST, $SRC
	@op_math_2reg AND, $DST, $SRC
endm

@foreach_reg_reg do_op_math_2reg

// ADD, SUB, and AND; reg to immediate


// take care of INC and DEC
macro op_math_inc_dec $ALUFUNC, $REG
	opcode $ALUFUNC $REG
		@fetch
		IDBUS_SRC($REG), LATCH(ALU_INPUT_1)
		ALU_MODE($ALUFUNC), IDBUS_SRC(ALU_RESULT), LATCH($REG), GOTO(NEXTOP)
	end
endm

macro do_op_math_inc_dec $REG
	@op_math_inc_dec INC, $REG
	@op_math_inc_dec DEC, $REG
endm

@foreach_reg do_op_math_inc_dec
@do_op_math_inc_dec SPL
@do_op_math_inc_dec SPH

// == Additional bitwise instructions ========================================

// OR, XOR, SBR, CBR, SBM, CBM

// OR instruction
macro op_OR_reg_by_reg $DST, $SRC
	opcode OR $DST, $SRC
		@fetch
		// simulate a hardware OR using ANDs and NOTs,
		// based on the principle of these two "tricks":
		// A | B = ~(~A & ~B)
		// ~A = (0xFF - A)
		
		// get the negation of A and B into tr0 and tr1
		IDBUS_SRC(CONST_FF), LATCH(ALU_INPUT_1)
		
		IDBUS_SRC($DST), LATCH(ALU_INPUT_2)
		ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH(tr0)
		
		IDBUS_SRC($SRC), LATCH(ALU_INPUT_2)
		ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH(tr1)
		
		// binary AND the two values
		IDBUS_SRC(tr0), LATCH(ALU_INPUT_1)
		IDBUS_SRC(tr1), LATCH(ALU_INPUT_2)
		ALU_MODE(AND), IDBUS_SRC(ALU_RESULT), LATCH(tr0)
		
		// invert the value and save to destination
		IDBUS_SRC(CONST_FF), LATCH(ALU_INPUT_1)
		IDBUS_SRC(tr0), LATCH(ALU_INPUT_2)
		ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH($DST), GOTO(NEXTOP)
	end
endm

@foreach_reg_reg op_OR_reg_by_reg

// == Comparison instructions ================================================

// CMP sets SREG for a following conditional branch. It's equivalent to SUB with the result thrown away.
// compare a register with another register
macro op_CMP_reg_to_reg $R1, $R2
	opcode CMP $R1, $R2
		@fetch
		IDBUS_SRC($R1), LATCH(ALU_INPUT_1)
		IDBUS_SRC($R2), LATCH(ALU_INPUT_2)
		ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), GOTO(NEXTOP)
	end
endm

@foreach_reg_reg op_CMP_reg_to_reg

// compare a register with an immediate
macro op_CMP_reg_to_imm $REG
	opcode CMP $REG, parm8
		@fetch
		@fetchparm ALU_INPUT_2
		IDBUS_SRC($REG), LATCH(ALU_INPUT_1)
		ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), GOTO(NEXTOP)
	end
endm

@foreach_reg op_CMP_reg_to_imm

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
	if [ Z ]
	@condbranch_body
end

opcode BNE parm16		// !=
	@fetch
	if [ Z ! ]
	@condbranch_body
end

opcode BGT parm16		// > (unsigned)		- alias: BCS, Branch if Carry Set
	@fetch
	if [ C ]
	@condbranch_body
end

opcode BGE parm16		// >= (unsigned)
	@fetch
	if [ C Z || ]
	@condbranch_body
end

opcode BLT parm16		// < (unsigned)		- alias: BCC, Branch if Carry Clear
	@fetch
	if [ C ! ]
	@condbranch_body
end

opcode BLE parm16		// <= (unsigned)
	@fetch
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

// RET LR		return from a BRL instuction
opcode RET LR
	@fetch
	IDBUS_SRC(LRH), LATCH(PC_HI)
	IDBUS_SRC(LRL), LATCH(PC_LO), GOTO(NEXTOP)
end

// JSR			jump and push return address to stack
// RTS			return from a JSR instruction

// == Stack instructions =====================================================

// == Misc instructions ======================================================

// PUSH, POP
// PUSH/POP SREG
// MOV SPL/SPH, R0
// MOV R0, SPL/SPH
// LDM any_reg, [SP+/-constant]
// STM [SP+/-constant], any_reg
// MEMSET
// MEMCPY

endfile

define 0xE0 SET CS	// CS = 0
	IDBUS_SRC(CONST_00), LATCH(PAGESEL), GOTO(NEXTOP)
end

// DS = 1

opcode 0xE2
written as SET ES	// ES = 2
	IDBUS_SRC(CONST_02), LATCH(ALU_INPUT_1)
	ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(PAGESEL), GOTO(NEXTOP)
end

macro ldm_macro $DESTREG
	opcode written as LDM $DESTREG, (mem16)
		// load the address into the TEMPADDR reg, then read each byte
		IDBUS_SRC(PC_HI), LATCH(TEMPADDR_HI)
		IDBUS_SRC(PC_LO), LATCH(TEMPADDR_LO)
		ADDRBUS_SRC(TEMPADDR), USE_CS, IDBUS_SRC(MEMORY_READ), LATCH($DESTREG.l), INC_PC
		IDBUS_SRC(PC_HI), LATCH(TEMPADDR_HI)
		IDBUS_SRC(PC_LO), LATCH(TEMPADDR_LO), INC_PC
		ADDRBUS_SRC(TEMPADDR), USE_CS, IDBUS_SRC(MEMORY_READ), LATCH($DESTREG.h), GOTO(NEXTOP)
	end
endm

#define ALL_REGS16	(r0, r1, r2, r3, r4)
#define ALL_REGS8	(r0l, r1l, r2l, r3l, r4l, r0h, r1h, r2h, r3h, r4h)

#for @REG in ALL_REGS16:
	!ldm_macro @REG
#next

endfile

// writes REG2 bytes starting at address REG1 to the byte contained in REG3.
// REG3 should be an 8-bit register such as r0l or r2h.
macro memset_macro $ADDR, $COUNT, $SETBYTE
	:memset $ADDR:reg16, $COUNT:reg16, $SETBYTE:reg8
		// add the count to the address to find the address of the byte immediately
		// after the last byte we'll set. Store the end pointer in tr0.
		ID_SRC($ADDRl), LATCH(ALU_INPUT_1)
		ID_SRC($COUNTl), LATCH(ALU_INPUT_2)
		ID_SRC(ALU_RESULT), ALU_MODE(ADD), LATCH(tr0l), LATCH_SREG
		ID_SRC($ADDRh), LATCH(ALU_INPUT_1)
		ID_SRC($COUNTh), LATCH(ALU_INPUT_2)
		IF [ $C ] {
			ID_SRC(ALU_RESULT), ALU_MODE(ADD_AND_INC), LATCH(tr0h)
		} else {
			ID_SRC(ALU_RESULT), ALU_MODE(ADD), LATCH(tr0h)
		}
		
		IDBUS_SRC(t0h), LATCH(TEMPADDR_HI)		// set the high byte. we'll only update it when a carry decrements it
@loop:
		// check if we've finished the last byte
		// first check the low byte. If it's equal check the high byte, else don't bother.
		ID_SRC($ADDRl), LATCH(ALU_INPUT_1)
		ID_SRC(tr0l), LATCH(ALU_INPUT_2)
		ALU_MODE(SUB), ID_SRC(ALU_RESULT), LATCH_SREG, GOTO @notDone IF [ $Z ! ]
		// low bytes were equal! check high byte
		ID_SRC($ADDRh), LATCH(ALU_INPUT_1)
		ID_SRC(tr0h), LATCH(ALU_INPUT_2)
		// compare high byte and if equal, we're done--go to next instruction by jumping to phase 0
		ALU_MODE(SUB), ID_SRC(ALU_RESULT), LATCH_SREG, GOTO 0 IF [ $Z ]
@notDone:
		
		// write the byte to the current memory address
		IDBUS_SRC(tr0l), LATCH(TEMPADDR_LO)
		ADDRBUS_SRC(TEMPADDR), ID_SRC($SETBYTE), WRITE_MEMORY
		
		// increment the current memory address in t0 and keep looping
		IDBUS_SRC(tr0l), LATCH(ALU_INPUT_1)
		ALU_MODE(DEC), ID_SRC(ALU_RESULT), LATCH(tr0l), LATCH_SREG, GOTO @loop IF [ $C ! ]
		IDBUS_SRC(tr0h), LATCH(ALU_INPUT_1)
		ALU_MODE(DEC), ID_SRC(ALU_RESULT), LATCH(tr0h)
		IDBUS_SRC(tr0h), LATCH(TEMPADDR_HI), GOTO @loop	// update the high byte of the address we're writing to
	end
endm

#for &ADDR in ( r0, r1, r2, r3, r4 )
	#for &COUNT in ( r0, r1, r2, r3, r4 )
		#if [ &ADDR &COUNT != ]
			#for &SETBYTE in ( r0, r1, r2, r3 )
				#if [ &SETBYTE &COUNT != &SETBYTE &ADDR != && ]
					#for &LH in ( l, h )
						!memset_macro &ADDR, &COUNT, &SETBYTE&LH
					#next
				#next
			#next
		#endif
	#next
#next

endfile

dim $mode
for $mode = 15 to 0 step -1
	IDBUS_SRC($mode)
next

nop

let $i := 4
loop
	ID_SRC(r0), LATCH($i)
	should ( i 0 = ) then
	{
		break;
	}
	else
	{
		let $i := ($i 1 +)
	}
next

endfile

:adc_if_test
	ID_SRC(r0), LATCH(ALU_INPUT_1)
	ID_SRC(r1), LATCH(ALU_INPUT_2), LATCH_SREG
	if ( C 1 = )
	{
		ALU_MODE(ADD_AND_INC), ID_SRC(ALU_RESULT), LATCH(r0)
	}
	else
	{
		ALU_MODE(ADD), ID_SRC(ALU_RESULT), LATCH(r0)
	}
	NEXTOP
end

endfile

include "foreach.mc"

// this performs the instruction fetch.
// prepended to all normal instructions so that you don't need to write
// it over and over.
// note that it is actually the fetch code located in the PREVIOUS instruction
// which is responsible for running each instruction.
:_fetch_prefix
	ADDRBUS_SRC(PC), IDBUS_SRC(READ_MEMORY), LATCH(OPCODE)
	INC_PC
end

:call [parm16]
	// read call address into TEMPADDR and increment PC past it to next instruction/return address
	ADDRBUS_SRC(PC), IDBUS_SRC(READ_MEMORY), LATCH(TEMPADDR_HI), INC_PC
	ADDRBUS_SRC(PC), IDBUS_SRC(READ_MEMORY), LATCH(TEMPADDR_LO), INC_PC
	
	// save old PC (i.e. return address) into temp reg 1 & 2
	ADDRBUS_SRC(PC), IDBUS_SRC(ADDR_HI), LATCH(TEMPREG1)
	ADDRBUS_SRC(PC), IDBUS_SRC(ADDR_LO), LATCH(TEMPREG2)
	
	// set new PC from TEMPADDR (jump to the subroutine)
	ADDRBUS_SRC(TEMPADDR), IDBUS_SRC(ADDR_HI), LATCH(PC_HI)
	ADDRBUS_SRC(TEMPADDR), IDBUS_SRC(ADDR_LO), LATCH(PC_LO)
	
	// load SP into TEMPADDR (HI=FF, LO=SP)
	IDBUS_SRC(CONST_FF), LATCH(TEMPADDR_HI)
	IDBUS_SRC(SP), LATCH(TEMPADDR_LO)
	
	// write TEMPREG1 (saved PC HI) to SP location through TEMPADDR
	ADDRBUS_SRC(TEMPADDR), IDBUS_SRC(TEMPREG1), WRITE_MEMORY
	
	// decrement SP
	IDBUS_SRC(SP), LATCH(ALU_INPUT_1)
	ALU_MODE(DEC), IDBUS_SRC(ALU_RESULT), LATCH(SP)
	
	// update TEMPADDR LO with new SP, then write TEMPREG2 (saved PC LO)
	// to SP location through TEMPADDR
	IDBUS_SRC(SP), LATCH(TEMPADDR_LO)
	ADDRBUS_SRC(TEMPADDR), IDBUS_SRC(TEMPREG2), WRITE_MEMORY
	
	// decrement SP one final time
	IDBUS_SRC(SP), LATCH(ALU_INPUT_1)
	ALU_MODE(DEC), IDBUS_SRC(ALU_RESULT), LATCH(SP)
	
	// finally, done.
	NEXTOP

:ret
	// increment SP
	IDBUS_SRC(SP), LATCH(ALU_INPUT_1)
	ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(SP)
	
	// load SP into TEMPADDR
	IDBUS_SRC(CONST_FF), LATCH(TEMPADDR_HI)
	IDBUS_SRC(SP), LATCH(TEMPADDR_LO)
	
	// fetch PC LO from first SP location
	ADDRBUS_SRC(TEMPADDR), IDBUS_SRC(READ_MEMORY), LATCH(PC_LO)
	
	// update new SP into ALU and increment again
	IDBUS_SRC(SP), LATCH(ALU_INPUT_1)
	ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(SP)
	
	// fetch PC HI from second SP location
	IDBUS_SRC(SP), LATCH(TEMPADDR_LO)
	ADDRBUS_SRC(TEMPADDR), IDBUS_SRC(READ_MEMORY), LATCH(PC_HI)
	
	NEXTOP
end

//
// ------ MOVE, LOAD, and STORE INSTRUCTIONS
//

// CLR		clear a register to 0
// MOV		copy value from one register to another
// LDI		load immediate
// LDIW		load immediate 16-bit value into register pair
// LDM		load from memory
// STM		store to memory
// LDW		load 16-bit value from memory into register pair
// STW		store 16-bit value from register pair to memory
// PUSH		push register or SREG onto stack
// POP		pop register or SREG from stack

// -- clr

MACRO clear_reg $REG
	:clr $REG
		IDBUS_SRC(CONST_FF), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH($REG)
		NEXTOP
	end
endm

MACRO clear_2regs $REG1, $REG2
	:clr $REG1:$REG2
		IDBUS_SRC(CONST_FF), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH($REG1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH($REG2)
		NEXTOP
	end
endm

!foreach_reg clear_reg
!clear_2regs r0, r1
!clear_2regs r1, r2
!clear_2regs r2, r3
!clear_2regs r3, r4

// -- mov

MACRO mov_reg2reg $DST $SRC
	:mov $DST, $SRC
		IDBUS_SRC($SRC), LATCH($DST)
		NEXTOP
	end
ENDM

!for_all_reg_combinations_incl_sp mov_reg2reg

:push SREG
	// Z=1 C=2 V=4
	// there sre 8 possible values that SREG can have.
	// we'll just provide code to return every one of them.
	
	LATCH_SREG
	
	// load SP into TEMPADDR (HI=FF, LO=SP)
	IDBUS_SRC(CONST_FF), LATCH(TEMPADDR_HI)
	IDBUS_SRC(SP), LATCH(TEMPADDR_LO)
	
	if (Z 0 = C 0 = && V 0 = &&)	// 0
	{
		IDBUS_SRC(CONST_FF), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), ADDRBUS_SRC(TEMPADDR), WRITE_MEMORY
	}
	if (Z 1 = C 0 = && V 0 = &&)	// 1
	{
		IDBUS_SRC(CONST_FF), LATCH(ALU_INPUT_1)
		IDBUS_SRC(CONST_02), LATCH(ALU_INPUT_2)
		ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), ADDRBUS_SRC(TEMPADDR), WRITE_MEMORY
	}
	if (Z 0 = C 1 = && V 0 = &&)	// 2
	{
		IDBUS_SRC(CONST_02), ADDRBUS_SRC(TEMPADDR), WRITE_MEMORY
	}
	if (Z 1 = C 1 = && V 0 = &&)	// 3
	{
		IDBUS_SRC(CONST_02), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), ADDRBUS_SRC(TEMPADDR), WRITE_MEMORY
	}
	if (Z 0 = C 0 = && V 1 = &&)	// 4
	{
		IDBUS_SRC(CONST_02), LATCH(ALU_INPUT_1)
		IDBUS_SRC(CONST_02), LATCH(ALU_INPUT_2)
		ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), ADDRBUS_SRC(TEMPADDR), WRITE_MEMORY
	}
	if (Z 1 = C 0 = && V 1 = &&)	// 5
	{
		IDBUS_SRC(CONST_02), LATCH(ALU_INPUT_1)
		IDBUS_SRC(CONST_02), LATCH(ALU_INPUT_2)
		ALU_MODE(ADD_AND_INC), IDBUS_SRC(ALU_RESULT), ADDRBUS_SRC(TEMPADDR), WRITE_MEMORY
	}
	if (Z 0 = C 1 = && V 1 = &&)	// 6
	{
		IDBUS_SRC(CONST_02), LATCH(ALU_INPUT_1)
		IDBUS_SRC(CONST_02), LATCH(ALU_INPUT_2)
		ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), LATCH(TEMPREG1)
		IDBUS_SRC(TEMPREG1), LATCH(ALU_INPUT_1)
		ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), ADDRBUS_SRC(TEMPADDR), WRITE_MEMORY
	}
	if (Z 1 = C 1 = && V 1 = &&)	// 7
	{
		IDBUS_SRC(CONST_02), LATCH(ALU_INPUT_1)
		IDBUS_SRC(CONST_02), LATCH(ALU_INPUT_2)
		ALU_MODE(ADD), IDBUS_SRC(ALU_RESULT), LATCH(TEMPREG1)
		IDBUS_SRC(TEMPREG1), LATCH(ALU_INPUT_1)
		ALU_MODE(ADD_AND_INC), IDBUS_SRC(ALU_RESULT), ADDRBUS_SRC(TEMPADDR), WRITE_MEMORY
	}
	
	// decrement SP
	IDBUS_SRC(SP), LATCH(ALU_INPUT_1)
	ALU_MODE(DEC), IDBUS_SRC(ALU_RESULT), LATCH(SP)
	
	// restore original contents of SREG that our ALU usage messed up
	IDBUS_SRC($DSTREG), LATCH(ALU_INPUT_1)
	ALU_MODE(UNARY_EX), IDBUS_SRC(ALU_RESULT)
	
	NEXTOP
end

:pop SREG
	// increment SP
	IDBUS_SRC(SP), LATCH(ALU_INPUT_1)
	ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(SP)
	
	// load SP into TEMPADDR
	IDBUS_SRC(CONST_FF), LATCH(TEMPADDR_HI)
	IDBUS_SRC(SP), LATCH(TEMPADDR_LO)
	
	// fetch new SREG from SP and set SREG using ALU special function
	// (note; if byte is outside range of 0-7, result is undefined)
	ADDRBUS_SRC(TEMPADDR), IDBUS_SRC(READ_MEMORY), LATCH(ALU_INPUT_1)
	ALU_MODE(UNARY_EX), IDBUS_SRC(ALU_RESULT)
	
	NEXTOP
end

//!foreach_reg push_reg
//!foreach_reg pop_reg

//
// ------ MATH INSTRUCTIONS
//

// ADD		add two registers or an immediate to a register
// SUB		sub two registers or an immediate from a register
// AND		logical AND two registers or a register and an immediate
// INC		increment register or memory location
// DEC		decrement register or memory location
// ADC		add with carry
// SBC		subtract with carry

// These opcodes have side-effect of affecting the other flags
// rather that just the one targeted to be set.
// CLC		clear carry flag
// SEC		set carry flag
// CLZ		clear zero flag
// SEZ		set zero flag

MACRO math_reg2reg $ALU_MODE, $DST, $SRC
	:$ALU_MODE $DST, $SRC
		ALU_MODE($ALU_MODE), ID_SRC($DST), LATCH(ALU_INPUT_1)
		ALU_MODE($ALU_MODE), ID_SRC($SRC), LATCH(ALU_INPUT_2)
		ALU_MODE($ALU_MODE), ID_SRC(ALU_RESULT), LATCH($DST)
		
		NEXTOP
	end
ENDM

MACRO math_reg_with_imm $ALU_MODE, $DST
	:$ALU_MODE $DST, [imm8]
		
		ID_SRC(READ_MEMORY), LATCH(ALU_INPUT_2)
		ALU_MODE($ALU_MODE), ID_SRC($DST), LATCH(ALU_INPUT_1), INC_PC
		ALU_MODE($ALU_MODE), ID_SRC(ALU_RESULT), LATCH($DST)
		
		NEXTOP
	end
ENDM

MACRO adc_reg2reg $DST, SRC
	:ADC $DST, SRC
		ID_SRC($DST), LATCH(ALU_INPUT_1)
		ID_SRC($SRC), LATCH(ALU_INPUT_2), LATCH_SREG
		if (C)
		{
			ALU_MODE(ADD_AND_INC), ID_SRC(ALU_RESULT), LATCH($DST)
		}
		else
		{
			ALU_MODE(ADD), ID_SRC(ALU_RESULT), LATCH($DST)
		}
		NEXTOP
	end
ENDM

MACRO adc_regwith0 $REG
	:ADC $REG
		LATCH_SREG
		
		if (C)
		{
			ID_SRC($REG), LATCH(ALU_INPUT_1)
			ALU_MODE(INC), ID_SRC(ALU_RESULT), LATCH($REG)
		}
		
		NEXTOP
	end
ENDM

!for_all_reg_combinations_incl_sp "math_reg2reg ADD, "
!for_all_reg_combinations_incl_sp "math_reg2reg SUB, "
!for_all_reg_combinations_incl_sp "math_reg2reg AND, "
!for_all_regs_single "math_reg_with_imm ADD, "
!for_all_regs_single "math_reg_with_imm SUB, "
!for_all_regs_single "math_reg_with_imm AND, "
!for_all_reg_combinations "adc_reg2reg"

!for_all_reg_combinations "math_reg2reg UNARY_EX, "
!for_all_regs_single "math_reg_with_imm UNARY_EX, "

// multiply RX by RY through repetitive addition and leave a 16-bit
// result in r0:r1. You should set up with a "clr r0:r1"
MACRO multiply_2regs $RX, $RY
	:mul $RX, $RY
	
	end
ENDM

//
// ------ COMPARISON & BRANCH INSTRUCTIONS
//

// CMP		compare two registers
// CPI		compare a register with an immediate
// CPM		compare a register with the value at a memory address
// CPMI		compare the value at a memory address with an immediate

// JMP		always branch to specified addres
// BEQ		branch if Z flag set
// BNE		branch if Z flag clear
// ... do the gt, ge, lt, le, and signed versions

:jmp	[mem]
	ID_SRC(READ_MEMORY), LATCH(TEMPREG1), INC_PC
	ID_SRC(READ_MEMORY), LATCH(PC_LO)
	ID_SRC(TEMPREG1), LATCH(PC_HI)
	NEXTOP
end

MACRO cond_branch	$NAME, $CONDITION
	:$NAME [mem]
		ID_SRC(READ_MEMORY), LATCH(TEMPREG1), INC_PC, LATCH_SREG
		IF ($CONDITION)
		{
			ID_SRC(READ_MEMORY), LATCH(PC_LO)
			ID_SRC(TEMPREG1), LATCH(PC_HI)
			NEXTOP
		}
		else
		{
			INC_PC
			NEXTOP
		}
	end
endm

cond_branch		"beq"	"Z=1"
cond_branch		"bne"	"Z=0"


// load a 16-bit value from TEMPADDR into the designated two registers
MACRO !loadmem16_to_reg $DSTHI $DSTLO
	// read first byte
	ADDRBUS_SRC(TEMPADDR), ID_SRC(READ_MEMORY), LATCH($DSTHI)
	
	// 16-bit increment of TEMPADDR using carry
	ADDRBUS_SRC(TEMPADDR), ID_SRC(ADDR_LO), LATCH(ALU_INPUT_1)
	ALU_MODE(INC), ID_SRC(ALU_RESULT), LATCH(TEMPADDR_LO), LATCH_SREG
	IF (LATCHED_C)
	{
		ADDRBUS_SRC(TEMPADDR), ID_SRC(ADDR_HI), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), ID_SRC(ALU_RESULT), LATCH(TEMPADDR_HI)
	}
	
	// read second byte
	ADDRBUS_SRC(TEMPADDR), ID_SRC(READ_MEMORY), LATCH($DSTLO)
ENDM

// jump to the 16-bit address stored in the specified memory location.
=IJMP [parm16]
	ADDRBUS_SRC(PC), ID_SRC(READ_MEMORY), LATCH(TEMPADDR_HI), INC_PC
	ADDRBUS_SRC(PC), ID_SRC(READ_MEMORY), LATCH(TEMPADDR_LO)
	!loadmem16_to_reg PC_HI, PC_LO
	NEXTOP

// jump to the given address.
=JMP [parm16]
	ADDRBUS_SRC(PC), ID_SRC(READ_MEMORY), LATCH(TEMPREG1), INC_PC
	ADDRBUS_SRC(PC), ID_SRC(READ_MEMORY), LATCH(PC_LO)
	ID_SRC(TEMPREG1), LATCH(PC_HI)
	NEXTOP

// this instruction implements a memset() of up to 256 bytes entirely
// in microcode.
// r0:r1 contains the starting address to memset.
// r2 contains the value to write the bytes to.
// r4 contains the number of bytes to set.
// on exit, r0:r1 will contain the address immediately after the written block,
// r4 will contain 00, and r2 will be unchanged.
// calling this function with an r4 of 00 will result in writing 256 bytes.
:memset [nofetch]
	if (LATCHED_Z)
	{	// we're done load next instruction
		ADDRBUS_SRC(PC), IDBUS_SRC(READ_MEMORY), LATCH(OPCODE)
		INC_PC
	}
	else
	{	// shame to waste these two cycles. too bad we can't get
		// part of the loop into them.
		NOP
		NOP
	}
	
	// note that we enter the instruction here, having run the fetch from some
	// previous opcode. Thus the state of Z will not affect us.
	
	// load r0:r1 into TEMPADDR
	IDBUS_SRC(r0), LATCH(TEMPADDR_HI)
	IDBUS_SRC(r1), LATCH(TEMPADDR_LO)
	
	// write r2 to the current memory location
	IDBUS_SRC(r2), ADDRBUS_SRC(TEMPADDR), WRITE_MEMORY
	
	// 16-bit increment of r0:r1
	IDBUS_SRC(r1), LATCH(ALU_INPUT_1)
	ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(r1), LATCH_SREG
	if (LATCHED_C)
	{
		IDBUS_SRC(r0), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(r0)
	}
	
	// decrement remaining count in r4 and update Z reg, which will
	// determine our behavior after the NEXTOP.
	IDBUS_SRC(r4), LATCH(ALU_INPUT_1)
	ALU_MODE(DEC), IDBUS_SRC(ALU_RESULT), LATCH(r4), LATCH_SREG
	NEXTOP

// this exciting instruction implements a memcpy of up to 256 bytes entirely
// in microcode.
//	r0:r1	source address
//	r2:r3	dest address
//	r4		count
// on exit, r0:r1 will contain the address immediately after the written block,
// r4 will contain 00, and r2 will be unchanged.
// calling this function with an r4 of 00 will result in writing 256 bytes.
=MEMCPY
	if (LATCHED_Z)
	{	// we're done load next instruction
		ADDRBUS_SRC(PC), IDBUS_SRC(READ_MEMORY), LATCH(OPCODE)
		INC_PC
	}
	else
	{	// shame to waste these two cycles. too bad we can't get
		// part of the loop into them (we can't because we enter
		// instruction at phase 3).
		NOP
		NOP
	}
	
	// note that we enter the instruction here, having run the fetch from some
	// previous opcode. Thus the state of Z will not affect us.
	
	// load r0:r1 into TEMPADDR read from source address
	IDBUS_SRC(r0), LATCH(TEMPADDR_HI)
	IDBUS_SRC(r1), LATCH(TEMPADDR_LO)
	ADDRBUS_SRC(TEMPADDR), IDBUS_SRC(READ_MEMORY), LATCH(TEMPREG1)
	
	// load r2:r3 into TEMPADDR write to destination address
	IDBUS_SRC(r2), LATCH(TEMPADDR_HI)
	IDBUS_SRC(r3), LATCH(TEMPADDR_LO)
	ADDRBUS_SRC(TEMPADDR), IDBUS_SRC(TEMPREG1), WRITE_MEMORY
	
	// 16-bit increment of r0:r1
	IDBUS_SRC(r1), LATCH(ALU_INPUT_1)
	ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(r1), LATCH_SREG
	if (LATCHED_C)
	{
		IDBUS_SRC(r0), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(r0)
	}
	
	// 16-bit increment of r2:r3
	IDBUS_SRC(r3), LATCH(ALU_INPUT_1)
	ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(r3), LATCH_SREG
	if (LATCHED_C)
	{
		IDBUS_SRC(r2), LATCH(ALU_INPUT_1)
		ALU_MODE(INC), IDBUS_SRC(ALU_RESULT), LATCH(r2)
	}
	
	// decrement remaining count in r4 and update Z reg, which will
	// determine our behavior after the NEXTOP.
	IDBUS_SRC(r4), LATCH(ALU_INPUT_1)
	ALU_MODE(DEC), IDBUS_SRC(ALU_RESULT), LATCH(r4), LATCH_SREG
	NEXTOP
*/
