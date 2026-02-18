						.386p

						PUBLIC		MATH_SQRT

						ASSUME CS:CODE

CODE					SEGMENT



; unsigned int isqrt(unsigned int N)
; {
; 	if(0==N)
; 	{
; 		return 0;
; 	}
; 	unsigned int guess=N;
; 	while(N/guess<guess)
; 	{
; 		guess=(guess+N/guess)/2;
; 	}
; 	return guess;
; }
; Integer SQRT
; unsigned isqrt(unsigned int N)
MATH_SQRT				PROC
; ESP+12	N
; ESP+8		Return EIP

						PUSH	EDX	; ESP+4
						PUSH	ECX	; ESP

						MOV		ECX,[ESP+12]	; ECX=N==guess
						JECXZ	MATH_SQRT_EXIT

MATH_SQRT_LOOP:
						MOV		EAX,[ESP+12]
						XOR		EDX,EDX			; EDX|EAX=N
						DIV		ECX				; EAX=N/guess
						CMP		EAX,ECX
						JAE		MATH_SQRT_EXIT

						ADD		ECX,EAX
						SHR		ECX,1
						JMP		MATH_SQRT_LOOP

MATH_SQRT_EXIT:
						MOV		EAX,ECX
						POP		ECX
						POP		EDX
						RET

MATH_SQRT				ENDP


CODE					ENDS
						END
