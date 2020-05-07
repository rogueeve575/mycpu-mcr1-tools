
// -- simulated OR instruction
opcode OR dstreg, srcreg
	@fetch
	@fetchparm PARM
	// simulate a hardware OR using ANDs and NOTs,
	// based on the principle of these two "tricks":
	// A | B = ~(~A & ~B)
	// ~A = (0xFF - A)
	
	// get the negation of A and B into tr0 and tr1
	IDBUS_SRC(VALUE), VALUE(0xff), LATCH(ALU_INPUT_1)
	
	IDBUS_FROM(PARMDST), LATCH(ALU_INPUT_2)
	ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH(tr0)
	
	IDBUS_FROM(PARMSRC), LATCH(ALU_INPUT_2)
	ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH(tr1)
	
	// binary AND the two values
	IDBUS_SRC(tr0), LATCH(ALU_INPUT_1)
	IDBUS_SRC(tr1), LATCH(ALU_INPUT_2)
	ALU_MODE(AND), IDBUS_SRC(ALU_RESULT), LATCH(tr0)
	
	// invert the value and save to destination
	IDBUS_SRC(VALUE), VALUE(0xff), LATCH(ALU_INPUT_1)
	IDBUS_SRC(tr0), LATCH(ALU_INPUT_2)
	ALU_MODE(SUB), IDBUS_SRC(ALU_RESULT), LATCH_FROM(PARMDST), GOTO(NEXTOP)
end
