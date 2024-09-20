						.386p

						ASSUME CS:CODE
						PUBLIC	MEMSETB_FAR
						PUBLIC	MEMSETW_FAR

						PUBLIC	SET_SECOND_BYTE
						PUBLIC	SET_LOW_BYTE
						PUBLIC	SET_LOW_WORD
						PUBLIC	SET_DWORD


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


CODE					ENDS
						END
