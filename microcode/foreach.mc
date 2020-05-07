
macro foreach_reg $MACRONAME
	@$MACRONAME r0
	@$MACRONAME r1
	@$MACRONAME r2
	@$MACRONAME r3
	@$MACRONAME r4
	@$MACRONAME r5
	@$MACRONAME r6
	@$MACRONAME r7
endm

macro foreach_regpair $MACRONAME
	@$MACRONAME r0, r1
	@$MACRONAME r1, r2
	@$MACRONAME r2, r3
	@$MACRONAME r3, r4
	@$MACRONAME r4, r5
	@$MACRONAME r6, r7
endm

macro foreach_reg_except_r0
	@$MACRONAME r1
	@$MACRONAME r2
	@$MACRONAME r3
	@$MACRONAME r4
	@$MACRONAME r5
	@$MACRONAME r6
	@$MACRONAME r7
endm

macro foreach_reg_combination $MACRONAME
	@$MACRONAME r0, r1
	@$MACRONAME r0, r2
	@$MACRONAME r0, r3
	@$MACRONAME r0, r4
	@$MACRONAME r0, r5
	@$MACRONAME r0, r6
	@$MACRONAME r0, r7
	
	@$MACRONAME r1, r0
	@$MACRONAME r1, r2
	@$MACRONAME r1, r3
	@$MACRONAME r1, r4
	@$MACRONAME r1, r5
	@$MACRONAME r1, r6
	@$MACRONAME r1, r7
	
	@$MACRONAME r2, r0
	@$MACRONAME r2, r1
	@$MACRONAME r2, r3
	@$MACRONAME r2, r4
	@$MACRONAME r2, r5
	@$MACRONAME r2, r6
	@$MACRONAME r2, r7
	
	@$MACRONAME r3, r0
	@$MACRONAME r3, r1
	@$MACRONAME r3, r2
	@$MACRONAME r3, r4
	@$MACRONAME r3, r5
	@$MACRONAME r3, r6
	@$MACRONAME r3, r7
	
	@$MACRONAME r4, r0
	@$MACRONAME r4, r1
	@$MACRONAME r4, r2
	@$MACRONAME r4, r3
	@$MACRONAME r4, r5
	@$MACRONAME r4, r6
	@$MACRONAME r4, r7
	
	@$MACRONAME r5, r0
	@$MACRONAME r5, r1
	@$MACRONAME r5, r2
	@$MACRONAME r5, r3
	@$MACRONAME r5, r4
	@$MACRONAME r5, r6
	@$MACRONAME r5, r7
	
	@$MACRONAME r6, r0
	@$MACRONAME r6, r1
	@$MACRONAME r6, r2
	@$MACRONAME r6, r3
	@$MACRONAME r6, r4
	@$MACRONAME r6, r5
	@$MACRONAME r6, r7
	
	@$MACRONAME r7, r0
	@$MACRONAME r7, r1
	@$MACRONAME r7, r2
	@$MACRONAME r7, r3
	@$MACRONAME r7, r4
	@$MACRONAME r7, r5
	@$MACRONAME r7, r6
endm

endfile

/*
macro foreach_reg $MACRONAME
	@$MACRONAME r0
	@$MACRONAME r1
	@$MACRONAME r2
	@$MACRONAME r3
	@$MACRONAME r4
endm

macro foreach_reg_incl_sp $MACRONAME
	@$MACRONAME r0
	@$MACRONAME r1
	@$MACRONAME r2
	@$MACRONAME r3
	@$MACRONAME r4
	@$MACRONAME sp
endm

macro for_all_reg_combinations $MACRONAME
	@$MACRONAME r0, r1
	@$MACRONAME r0, r2
	@$MACRONAME r0, r3
	@$MACRONAME r0, r4
	@$MACRONAME r1, r0
	@$MACRONAME r1, r2
	@$MACRONAME r1, r3
	@$MACRONAME r1, r4
	@$MACRONAME r2, r0
	@$MACRONAME r2, r1
	@$MACRONAME r2, r3
	@$MACRONAME r2, r4
	@$MACRONAME r3, r0
	@$MACRONAME r3, r1
	@$MACRONAME r3, r2
	@$MACRONAME r3, r4
	@$MACRONAME r4, r0
	@$MACRONAME r4, r1
	@$MACRONAME r4, r2
	@$MACRONAME r4, r3
endm

macro for_all_reg_combinations_incl_sp $MACRONAME
	@$MACRONAME r0, r1
	@$MACRONAME r0, r2
	@$MACRONAME r0, r3
	@$MACRONAME r0, r4
	@$MACRONAME r0, sp
	@$MACRONAME r1, r0
	@$MACRONAME r1, r2
	@$MACRONAME r1, r3
	@$MACRONAME r1, r4
	@$MACRONAME r1, sp
	@$MACRONAME r2, r0
	@$MACRONAME r2, r1
	@$MACRONAME r2, r3
	@$MACRONAME r2, r4
	@$MACRONAME r2, sp
	@$MACRONAME r3, r0
	@$MACRONAME r3, r1
	@$MACRONAME r3, r2
	@$MACRONAME r3, r4
	@$MACRONAME r3, sp
	@$MACRONAME r4, r0
	@$MACRONAME r4, r1
	@$MACRONAME r4, r2
	@$MACRONAME r4, r3
	@$MACRONAME r4, sp
	@$MACRONAME sp, r0
	@$MACRONAME sp, r1
	@$MACRONAME sp, r2
	@$MACRONAME sp, r3
	@$MACRONAME sp, r4
endm
*/
