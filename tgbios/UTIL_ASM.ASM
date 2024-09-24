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
						STOSD

						MOV		ECX,[EBP+20]
						SHR		ECX,1
						AND		ECX,1
						STOSW

						MOV		ECX,[EBP+20]
						SHR		ECX,2
						AND		ECX,1
						STOSB

						POP		EDI
						POP		ECX
						POP		ES
						POP		EBP
						RET

MEMSETB_FAR				ENDP


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
						STOSD

						MOV		ECX,[EBP+20]
						AND		ECX,1
						STOSW

						POP		EDI
						POP		ECX
						POP		ES
						POP		EBP
						RET

MEMSETW_FAR				ENDP



MEMCPY_FAR				PROC
;void MEMCPY_FAR(_Far void *dst,const _Far void *src,unsigned int bytes);

						PUSH	ES   ; +16
						PUSH	DS   ; +12
						PUSH	EDI  ; +8
						PUSH	ESI  ; +4
						PUSH	ECX  ; +0

						MOV		EDI,[ESP+24]
						MOV		ES,[ESP+28]
						MOV		ESI,[ESP+32]
						MOV		DS,[ESP+36]
						CLD

						MOV		ECX,[ESP+40]
						SHR		ECX,2
						REP		MOVSD

						MOV		ECX,[ESP+40]
						SHR		ECX,1
						AND		ECX,1
						REP		MOVSW

						MOV		ECX,[ESP+40]
						AND		ECX,1
						REP		MOVSB

						POP		ECX
						POP		ESI
						POP		EDI
						POP		DS
						POP		ES
						RET

MEMCPY_FAR				ENDP



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




SET_SECOND_BYTE			PROC

						PUSH	EBP
						MOV		EBP,[ESP+8]
						MOV		AL,[ESP+12]
						MOV		[EBP+1],AL
						POP		EBP
						RET

SET_SECOND_BYTE			ENDP

SET_LOW_BYTE			PROC

						PUSH	EBP
						MOV		EBP,[ESP+8]
						MOV		AL,[ESP+12]
						MOV		[EBP],AL
						POP		EBP
						RET

SET_LOW_BYTE			ENDP

SET_LOW_WORD			PROC

						PUSH	EBP
						MOV		EBP,[ESP+8]
						MOV		AX,[ESP+12]
						MOV		[EBP],AX
						POP		EBP
						RET

SET_LOW_WORD			ENDP

SET_DWORD				PROC

						PUSH	EBP
						MOV		EBP,[ESP+8]
						MOV		EAX,[ESP+12]
						MOV		[EBP],EAX
						POP		EBP
						RET

SET_DWORD				ENDP


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

CODE					ENDS
						END
