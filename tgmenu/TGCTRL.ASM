						ASSUME	CS:CODE,DS:CODE
						.386p

; TGCTRL.COM ssssoooo
;   ssss Segment of the command buffer
;   oooo Offset of the command buffer


TSUGARU_STATUS			MACRO
						OUT		0EBh,AL
						ENDM

TSUGARU_BREAK			MACRO
						OUT		0EAh,AL
						ENDM


CODE					SEGMENT	USE16
						ORG		0100H

PSP_ENVSEG				EQU		02CH

STACK_PTR:

MAIN:
						CLD			; Don't touch it.

						PUSH	CS
						POP		DS

						PUSH	CS
						POP		ES

						PUSH	CS
						POP		SS
						MOV		SP,OFFSET STACK_PTR

						MOV		AH,62H		; Get Current PSP
						INT		21H
						MOV		CS:[PSP],BX

						MOV		FS,BX
						MOV		AX,FS:[PSP_ENVSEG]
						MOV		CS:[ENVSEG],AX

						MOV		GS,CS:[ENVSEG]
						MOV		SI,OFFSET COMSPEC_LABEL
						CALL	FIND_ENV_VAR

						TSUGARU_BREAK

						MOV		GS,CS:[ENVSEG]
						MOV		SI,OFFSET PATH_LABEL
						CALL	FIND_ENV_VAR

						TSUGARU_BREAK

						MOV		AH,19H
						INT		21H			; Get Currnent Drive
						ADD		AL,'A'
						MOV		CS:[CUR_DRV],AL



						MOV		SI,OFFSET TGMENU_EXP_SRC
						MOV		DI,OFFSET TGMENU_EXP
						MOV		AH,60H		; DOS TRUE NAME
						INT		21H




						MOV		AH,4CH		; Exit
						INT		21H

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Input
;   GS     ENV segment
; Return:
;   GS:BX  Proc File Name
FIND_PROC_FILE_NAME:
						XOR		BX,BX
						DEC		BX

FIND_ENV_SEG_0000:
						INC		BX
						CMP		WORD PTR GS:[BX],0000H
						JNE		FIND_ENV_SEG_0000

						ADD		BX,2

						RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

CAPITALIZE:
						CMP		AL,'a'
						JB		CAPITALIZE_DONE
						CMP		AL,'z'
						JA		CAPITALIZE_DONE
						ADD		AL,'A'-'a'
CAPITALIZE_DONE:
						RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Input
;  GS     ENV segment
;  DS:SI  Variable name + '=' eg. "PATH="
; return
;  GS:BX  Pointer following '='
;  EDX    Destroyed
;  EAX    Destroyed
FIND_ENV_VAR:
						XOR		BX,BX

FIND_ENV_VAR_OUTER_LOOP:
						PUSH	SI

FIND_ENV_VAR_INNER_LOOP:
						MOV		AL,GS:[BX]
						CALL	CAPITALIZE
						MOV		DL,AL
						LODSB
						CALL	CAPITALIZE
						CMP		DL,AL
						JNE		FIND_ENV_VAR_NEXT

						CMP		DL,'='
						JE		FOUND_ENV_VAR

						INC		BX
						JMP		SHORT FIND_ENV_VAR_INNER_LOOP

FIND_ENV_VAR_NEXT:
						POP		SI

FIND_ENV_VAR_SKIP_LOOP:
						INC		BX
						CMP		BYTE PTR GS:[BX],0
						JNE		SHORT FIND_ENV_VAR_SKIP_LOOP

						INC		BX
						CMP		BYTE PTR GS:[BX],0	; Two consecutive zeros means end of ENVSEG.
						JNE		FIND_ENV_VAR_OUTER_LOOP

						; Variable not found
						STC
						RET

FOUND_ENV_VAR:
						POP		SI
						INC		BX
						CLC
						RET



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PSP						DW		0
ENVSEG					DW		0
CUR_DRV					DB		0
OWN_DRV					DB		0

COMSPEC_LABEL			DB		"COMSPEC="
PATH_LABEL				DB		"PATH="

TGMENU_EXP_SRC			DB		"\TGMENU.EXP",0
TGMENU_EXP				DB		80 dup(0)


CODE					ENDS
						END		MAIN
