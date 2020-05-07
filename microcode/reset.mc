
// code to execute during RESET. it runs over and over until the CPU
// is taken out of reset. it doesn't matter what OPCODE says for this code.
// additionally the fetch preamble is not prepended.
//
// when we come out of reset we could either
//	* have special logic that ensures that PHASE is reset to 0, or...
//	the deal is we don't know what phase we will be in when we come out of
//	reset. And so coming out of reset is going to dump us randomly into the
//	middle of the microcode for whatever's in OPCODE. So:
//		a special opcode called RESET (0xff) is defined. This opcode
//		has a first part which is identical to the reset code, but then has an
//		additional part. The additional part consists of a NOP sled which
//		gets us towards the very end of the possible instruction phases--
//		past the microcode of the first instruction. Then it fetches the
//		opcode at the PC -- switching us into the first opcode's microcode.
//		Since we are past the end of it's microcode we will run down a NOP sled
//		of unused phases until the phase wraps around, at which point we will
//		re-fetch it and execute it as normal.
//		To be sure this works we can recommend putting a particular instruction
//		which has a short microcode, such as NOP, as the first instruction at
//		the reset vector.
//
//	Note that it happens to work out that RESET actually is able to be executed
//	as a regular program instruction, and _does_ work to cause a software
//	reset.
//
:_reset_code
	// two instructions as placeholders for where the fetch would normally be.
	// this allows RESET to be executed as a regular opcode.
	NOP
	NOP
	// load the RESET opcode (0xFF)
	IDBUS_SRC(CONST_FF), LATCH(OPCODE)
	// get a constant 0 by subtracting FF from FF in the ALU.
	IDBUS_SRC(CONST_FF), LATCH(ALU_INPUT_1)
	IDBUS_SRC(CONST_FF), LATCH(ALU_INPUT_2)
	// set the PC to 0000.
	ALU_MODE(ALU_SUB), IDBUS_SRC(ALU_RESULT), LATCH(PC_LO)
	ALU_MODE(ALU_SUB), IDBUS_SRC(ALU_RESULT), LATCH(PC_HI)
	// over and over until we come out of reset.
	NOP
	NEXTOP
end

:reset [bytecode=0xff] [nofetch]
	// two instructions as placeholders for where the fetch would normally be.
	// this allows RESET to be executed as a regular opcode.
	NOP
	NOP
	// load the RESET opcode (0xFF)
	IDBUS_SRC(CONST_FF), LATCH(OPCODE)
	// get a constant 0 by subtracting FF from FF in the ALU.
	IDBUS_SRC(CONST_FF), LATCH(ALU_INPUT_1)
	IDBUS_SRC(CONST_FF), LATCH(ALU_INPUT_2)
	// set the PC to 0000.
	ALU_MODE(ALU_SUB), IDBUS_SRC(ALU_RESULT), LATCH(PC_LO)
	ALU_MODE(ALU_SUB), IDBUS_SRC(ALU_RESULT), LATCH(PC_HI)
	// NOP sled to get us past microcode of first instruction.
	NOP	// 7
	NOP	// 8
	NOP	// 9
	NOP	// 10
	NOP	// 11
	NOP	// 12
	NOP	// 13
	NOP	// 14
	NOP	// 15
	NOP	// 16
	NOP	// 17
	NOP	// 18
	NOP	// 19
	NOP	// 20
	NOP	// 21
	NOP	// 22
	NOP	// 23
	NOP	// 24
	NOP	// 25
	NOP	// 26
	NOP	// 27
	NOP	// 28
	NOP	// 29
	NOP	// 30
	// fetch first instruction - we will switch microcodes at this point,
	// roll over the phase counter, and execute first instruction.
	ADDRBUS_SRC(PC), IDBUS_SRC(READ_MEMORY), LATCH(OPCODE)
