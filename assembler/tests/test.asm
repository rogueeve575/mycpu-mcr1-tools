
.filepos 0x0
.runfrom 0xC000

RESET:
	NOP				// NOP at reset vector
	JMP	main

.padto 0xC010
	
main:
	SET r7			// r7 is bit we're setting, we'll start at 1 and "wrap around"
	CLR r0			// r0 will get each bit set one at a time
.L1:
	SBR r0, r7		// set bit in r0
	INC r7			// go on to next bit
	CMP r7, 9		// until we have wrapped around to first bit...
	BNE .L1			// ...keep looping

forever:
	NOP
	JMP	forever
	
/*forever:
	NOP
.L1:
	NOP
	JMP .L1			// infinite loop, start all over again
	
	.db 34, "This is a string", 34, 0
	.dw 0x1234, "Unicode string?"
	.db unquoted_string		// will need to add that "quoted" parameter to Token to handle the invalidity of this
	.db labelname
*/
/*
//.org 0x00
.runat 0x00
//reset_vector:
	NOP
	LDI	r0, 0x10
	LDI	r1, 0x08
//.addloop:
	ADD	r0, r1
	BLT 0x0005///.addloop
	
	CLR r0
	INC r1
	JMP 0x0005//.addloop
*/
