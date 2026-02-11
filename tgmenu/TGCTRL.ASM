						ASSUME	CS:CODE,DS:CODE
						.386p

; TGCTRL.COM ssssoooo
;   ssss Segment of the command buffer
;   oooo Offset of the command buffer

CODE					SEGMENT	USE16
						ORG		0100H

PSP_ENVSEG				EQU		02CH

STACK_PTR:

MAIN:
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

						MOV		AH,19H
						INT		21H			; Get Currnent Drive
						ADD		AL,'A'
						MOV		CS:[CUR_DRV],AL




						MOV		SI,OFFSET TGMENU_EXP_SRC
						MOV		DI,OFFSET TGMENU_EXP
						MOV		AH,60H		; DOS TRUE NAME
						INT		21H


						OUT		0EAh,AL


						MOV		AH,4CH		; Exit
						INT		21H


PSP						DW		0
CUR_DRV					DB		0
OWN_DRV					DB		0

TGMENU_EXP_SRC			DB		"\\TGMENU.EXP",0
TGMENU_EXP_DRV			DB		'?',':'
TGMENU_EXP				DB		80 dup(0)


CODE					ENDS
						END		MAIN
