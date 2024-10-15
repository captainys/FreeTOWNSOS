						.386p

						ASSUME CS:CODE
						PUBLIC	MEMSETB_FAR
						PUBLIC	MEMSETW_FAR
						PUBLIC	MEMCPY_FAR

						PUBLIC	MOVSB_FAR	; It does use MOVSB only.  Good for copy to wave RAM.

						PUBLIC	SET_SECOND_BYTE
						PUBLIC	SET_LOW_BYTE
						PUBLIC	SET_LOW_WORD
						PUBLIC	SET_DWORD

						PUBLIC	__SET_RPVECTP
						PUBLIC	__GET_RVECT
						PUBLIC	__GET_PVECT
						PUBLIC	__SET_RVECT
						PUBLIC	__SET_PVECT

						PUBLIC	CALL_SNDINT_HANDLER

						PUBLIC	MULDIV
						PUBLIC	IMULDIV
						PUBLIC	MUL_SHR


TSUGARU_BREAK			MACRO
						OUT		0EAH,AL
						ENDM

TSUGARU_STATUS			MACRO
						OUT		0EBH,AL
						ENDM


CODE					SEGMENT

TOWNSIO_VM_HOST_IF_CMD_STATUS	EQU		2386H
TOWNSIO_VM_HOST_IF_DATA      	EQU		2387H

TOWNS_VMIF_CMD_NOP           	EQU		00H 
TOWNS_VMIF_CMD_CAPTURE_CRTC  	EQU		01H // Capture CRTC. Followed by two bytes mode0H and mode1.
TOWNS_VMIF_CMD_PAUSE         	EQU		02H // Pause VM
TOWNS_VMIF_CMD_EXIT_VM       	EQU		03H // Exit the VM.  Return code taken from the data queu.

TOWNS_VMIF_CMD_FILE_RXRDY    	EQU		04H
TOWNS_VMIF_CMD_FILE_ACK      	EQU		05H

TOWNS_VMIF_TFR_END           	EQU		00H
TOWNS_VMIF_TFR_HOST_TO_VM    	EQU		01H
TOWNS_VMIF_TFR_VM_TO_HOST    	EQU		02H

TOWNS_VMIF_TFR_NOERROR       	EQU		00H
TOWNS_VMIF_TFR_WRITE_ERROR   	EQU		80H


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


MEMSETB_FAR				PROC
; MEMSETB_FAR(_Far void *ptr,unsigned int value,unsigned int count);
; [EBP+20] count
; [EBP+16] value
; [EBP+12] SELECTOR
; [EBP+8]  OFFSET
; [EBP+4]  EIP
; [EBP]    EBP
						PUSH	EBP
						MOV		EBP,ESP
						PUSH	ES
						PUSH	ECX
						PUSH	EDI

						MOV		AL,[EBP+16]
						MOV		AH,AL
						SHL		EAX,8
						MOV		AL,AH
						SHL		EAX,8
						MOV		AL,AH

						LES		EDI,[EBP+8]
						CLD

						MOV		ECX,[EBP+20]
						SHR		ECX,2
						REP STOSD

						MOV		ECX,[EBP+20]
						SHR		ECX,1
						AND		ECX,1
						REP STOSW

						MOV		ECX,[EBP+20]
						AND		ECX,1
						REP STOSB

						POP		EDI
						POP		ECX
						POP		ES
						POP		EBP
						RET

MEMSETB_FAR				ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


MEMCPY_FAR				PROC
; MEMSETB_FAR(_Far void *dst,_Far void *src,unsigned int bytes);
; [ESP+40] Count
; [ESP+36] src SELECTOR
; [ESP+32] src OFFSET
; [ESP+28] dst SELECTOR
; [ESP+24] dst OFFSET
; [ESP+20] EIP
; [ESP+16] DS
; [ESP+12] ES
; [ESP+8]  EDI
; [ESP+4]  ESI
; [ESP]    ECS
						PUSH	DS
						PUSH	ES
						PUSH	EDI
						PUSH	ESI
						PUSH	ECX

						LES		EDI,[ESP+24]
						LDS		ESI,[ESP+32]
						MOV		ECX,[ESP+40]
						CLD

						SHR		ECX,2
						REP MOVSD

						MOV		ECX,[ESP+40]
						SHR		ECX,1
						AND		ECX,1
						REP MOVSW

						MOV		ECX,[ESP+40]
						AND		ECX,1
						REP MOVSB

						POP		ECX
						POP		ESI
						POP		EDI
						POP		ES
						POP		DS
						RET

MEMCPY_FAR				ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


MEMSETW_FAR				PROC
; MEMSETW_FAR(_Far void *ptr,unsigned int value,unsigned int count);
; [EBP+20] count
; [EBP+16] value
; [EBP+12] SELECTOR
; [EBP+8]  OFFSET
; [EBP+4]  EIP
; [EBP]    EBP
						PUSH	EBP
						MOV		EBP,ESP
						PUSH	ES
						PUSH	ECX
						PUSH	EDI

						MOV		AX,[EBP+16]
						SHL		EAX,16
						MOV		AX,[EBP+16]

						LES		EDI,[EBP+8]
						CLD

						MOV		ECX,[EBP+20]
						SHR		ECX,1
						REP STOSD

						MOV		ECX,[EBP+20]
						AND		ECX,1
						REP STOSW

						POP		EDI
						POP		ECX
						POP		ES
						POP		EBP
						RET

MEMSETW_FAR				ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


MOVSB_FAR				PROC
;void MOVSB_FAR(_Far void *dst,_Far void *src,unsigned int count);
						PUSH	ES   ; +16
						PUSH	DS   ; +12
						PUSH	EDI  ; +8
						PUSH	ESI  ; +4
						PUSH	ECX  ; +0

						MOV		EDI,[ESP+24]
						MOV		ES,[ESP+28]
						MOV		ESI,[ESP+32]
						MOV		DS,[ESP+36]
						MOV		ECX,[ESP+40]
						CLD
						REP		MOVSB

						POP		ECX
						POP		ESI
						POP		EDI
						POP		DS
						POP		ES
						RET
MOVSB_FAR				ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


SET_SECOND_BYTE			PROC

						PUSH	EBP
						MOV		EBP,[ESP+8]
						MOV		AL,[ESP+12]
						MOV		[EBP+1],AL
						POP		EBP
						RET

SET_SECOND_BYTE			ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


SET_LOW_BYTE			PROC

						PUSH	EBP
						MOV		EBP,[ESP+8]
						MOV		AL,[ESP+12]
						MOV		[EBP],AL
						POP		EBP
						RET

SET_LOW_BYTE			ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


SET_LOW_WORD			PROC

						PUSH	EBP
						MOV		EBP,[ESP+8]
						MOV		AX,[ESP+12]
						MOV		[EBP],AX
						POP		EBP
						RET

SET_LOW_WORD			ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


SET_DWORD				PROC

						PUSH	EBP
						MOV		EBP,[ESP+8]
						MOV		EAX,[ESP+12]
						MOV		[EBP],EAX
						POP		EBP
						RET

SET_DWORD				ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


__SET_RPVECTP			PROC
; __SET_RPVECTP(int INTNum,_Far void (*func)(void))
						PUSH	EBP
						MOV		EBP,ESP
						PUSH	ECX
						PUSH	EDX
						PUSH	DS

						MOV		AX,2506H	; Set INT to Always Gain Control in Protected Mode
						MOV		CL,[EBP+08H]
						MOV		DS,[EBP+0CH]
						MOV		EDX,[EBP+10H]
						INT		21H

						POP		DS
						POP		EDX
						POP		ECX
						POP		EBP
						RET
__SET_RPVECTP			ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


__GET_RVECT				PROC
; unsigned long __GET_RVECT(int INTNum)
						PUSH	ECX
						PUSH	EBX

						MOV		CL,[ESP+0CH]
						MOV		AX,2503H
						INT		21H
						MOV		EAX,EBX

						POP		EBX
						POP		ECX
						RET
__GET_RVECT				ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


__GET_PVECT				PROC
; _Far void *__GET_PVECT(int INTNum)
						PUSH	ECX
						PUSH	EBX
						PUSh	ES

						MOV		AX,2502h
						MOV		CL,[ESP+10H]
						INT		21H

						MOV		DX,ES
						MOV		EAX,EBX

						POP		ES
						POP		EBX
						POP		ECX

						; High-C apparently returns a Far pointer by DS:EAX

						RET
__GET_PVECT				ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


__SET_RVECT				PROC
; void __SET_RVECT(int INTNum,unsigned long Handler)
						PUSH	ECX
						PUSH	EBX

						MOV		EBX,[ESP+10H]
						MOV		CL,[ESP+0CH]
						MOV		AX,2505H
						INT		21H

						POP		EBX
						POP		ECX
						RET
__SET_RVECT				ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


__SET_PVECT				PROC
; void __SET_PVECT(int INTNum,_Far void *func)
						PUSH	ECX
						PUSH	EDX
						PUSh	DS

						MOV		CL,[ESP+10H]
						MOV		EDX,[ESP+14H]
						MOV		DS,[ESP+18H]
						MOV		AX,2504h
						INT		21H

						POP		DS
						POP		EDX
						POP		ECX

						RET
__SET_PVECT				ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


; void CALL_SNDINT_HANDLER(_Far void *SSESP,_Far void *CSEIP,unsigned int DS,unsigned int ES,unsigned int FS,unsigned int GS)
CALL_SNDINT_HANDLER		PROC
						; TSUGARU_BREAK

						PUSH	FS
						PUSH	GS
						PUSH	ESI
						PUSH	EBP

						CMP		WORD PTR [ESP+18H],0
						JE		CALL_SNDINT_NO_STACK_CHANGE

; ESP+30H	GS
; ESP+2CH	FS
; ESP+28H	ES
; ESP+24H	DS
; ESP+20H	CS
; ESP+1CH	EIP
; ESP+18H	SS
; ESP+14H	ESP
; ---- Input Parameters ----
; ESP+10H	FS
; ESP+0CH	GS
; ESP+08H	ESI
; ESP+04H	EBP
; ESP		Return EIP

						MOV		SI,SS
						MOV		DS,SI
						MOV		ESI,ESP		; DS:ESI is current stack pointer

						LSS		ESP,[ESP+14H]

						PUSH	DWORD PTR [ESI+30H]
						PUSH	DWORD PTR [ESI+2CH]
						PUSH	DWORD PTR [ESI+28H]
						PUSH	DWORD PTR [ESI+24H]
						PUSH	DWORD PTR [ESI+20H]
						PUSH	DWORD PTR [ESI+1CH]
						PUSH	DS
						PUSH	ESI
						MOV		EBP,ESP
						JMP		CALL_SNDINT_STACK_DID_CHANGE

; Now
; ---- Input Parameters ----
; EBP		PrevSSESP
; EBP+08H	CSEIP
; EBP+10H	DS
; EBP+14H	ES
; EBP+18H	FS
; EBP+1CH	GS

CALL_SNDINT_NO_STACK_CHANGE:
						LEA		EBP,[ESP+14H]
						MOV		[EBP],ESP
						MOV		[EBP+4],SS


CALL_SNDINT_STACK_DID_CHANGE:

						MOV		DS,[EBP+10H]
						MOV		ES,[EBP+14H]
						MOV		FS,[EBP+18h]
						MOV		GS,[EBP+1CH]
						CALL	FWORD PTR [EBP+8]

						LSS		ESP,[EBP]

						POP		EBP
						POP		ESI
						POP		GS
						POP		FS
						RET
CALL_SNDINT_HANDLER		ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


IMULDIV					PROC
; int x=IMULDIV(a,b,c); 
; [ESP+16] C
; [ESP+12] B
; [ESP+8]  A
; [ESP+4]  EIP
; [ESP]    EDX
						PUSH	EDX

						MOV		EAX,[ESP+8]
						IMUL	DWORD PTR [ESP+12]
						IDIV	DWORD PTR [ESP+16]

						POP		EDX
						RET

IMULDIV					ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


MULDIV					PROC
; unsigd int x=MULDIV(a,b,c); 
; [ESP+16] C
; [ESP+12] B
; [ESP+8]  A
; [ESP+4]  EIP
; [ESP]    EDX
						PUSH	EDX

						MOV		EAX,[ESP+8]
						IMUL	DWORD PTR [ESP+12]
						IDIV	DWORD PTR [ESP+16]

						POP		EDX
						RET

MULDIV					ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


MUL_SHR					PROC
; unsigned int x=MUL_SHR(a,b,shift);
; [ESP+20] shift
; [ESP+16] B
; [ESP+12] A
; [ESP+8]  EIP
; [ESP+4]  EDX
; [ESP]    ECX
						PUSH	EDX
						PUSH	ECX

						MOV		EAX,[ESP+12]
						MOV		EDX,[ESP+16]
						MUL		EDX

						MOV		ECX,[ESP+20]
						SHRD	EAX,EDX,CL

						POP		ECX
						POP		EDX
						RET
MUL_SHR					ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


CODE					ENDS
						END
