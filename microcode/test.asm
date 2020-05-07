
; calculate prime numbers from 0 - 255, displaying them on the 7-segment.

.org 0
	nop
	jmp start

start:
	clr r3		; r3 will hold the current number we're checking for primeness.
loop:


is_prime:		; returns nonzero if r0 if r3 is prime, 0 otherwise
	; if it's even, easy case out
	mov r0, r3
	andi r0, #1
	bnz .not_even
	ret			; it's even so not prime. r0 is already 0. return.
.not_even:

	; divide by each odd value starting at 3, and if we find a remainder
	; of 0, we know it's not prime.
	ldi r1, #3
.check_loop:
	mov r0, r3
	jsr divide_remainder
	cmp r0, #0
	beq .not_proven_unprime
	ret			; it's not prime, again r0 is already conveniently 0, return.
.not_proven_unprime:
	add r1, #2
	cmp r1, r3
	blt .check_loop

	; nothing could do it. it's a prime!
	set r0
	ret

; divide r0 by r1, returning the remainder in r1.
divide_remainder:
.loop:
	sub r0, r1
	bncz .loop
	bz .remainder_was_0
	add r0, r1	; put back the amount we went over
.remainder_was_0:
	ret
