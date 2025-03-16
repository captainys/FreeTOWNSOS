						.386p
						ASSUME CS:CODE

						PUBLIC	SND_SETGLOBAL_GAMEDEV_TYPE

TGBIOS_SEG				EQU		0110H

SNDBIOS_ENTRY			EQU		080H

CODE					SEGMENT

; void SND_SetGlobal_Gamedev_Type(int port,int devType);
SND_SETGLOBAL_GAMEDEV_TYPE			PROC

; EBP+C Device Type
; EBP+8 PORT
; EBP+4	EIP
; EBP	Prev EBP

						PUSH	EBP
						MOV		EBP,ESP
						PUSH	EAX
						PUSH	EDX
						PUSH	FS


						; AH=E0H
						; DH=Port Number 0 or 1
						; DL=Device Type

						MOV		DX,TGBIOS_SEG
						MOV		FS,DX

						MOV		AH,0E0H
						MOV		DL,[EBP+8]
						MOV		DH,[EBP+12]

						CALL	PWORD PTR FS:[SNDBIOS_ENTRY]

						POP		FS
						POP		EDX
						POP		EAX
						MOV		ESP,EBP
						POP		EBP
						RET

SND_SETGLOBAL_GAMEDEV_TYPE			ENDP

CODE					ENDS

						END
