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
PSP_FCB1				EQU		05CH
PSP_FCB2				EQU		06CH


STACK_PTR:

MAIN:
						CLI
						CLD			; Don't touch it.

						PUSH	CS
						POP		DS

						PUSH	CS
						POP		ES

						PUSH	CS
						POP		SS
						MOV		SP,OFFSET STACK_PTR

						MOV		BX,OFFSET END_OF_PROGRAM
						ADD		BX,000FH
						SHR		BX,4
						MOV		AH,4AH	; Resize memory block.  Resize self.
						INT		21H

						STI



						MOV		AH,62H		; Get Current PSP
						INT		21H


						MOV		FS,BX				; FS is PSP.  Don't touch it.
						MOV		GS,FS:[PSP_ENVSEG]	; GS is ENVSEG.  Don't touch it.


						MOV		AH,19H
						INT		21H			; Get Currnent Drive
						ADD		AL,'A'
						MOV		CS:[CUR_DRV],AL

						MOV		SI,OFFSET TGMENU_EXP_SRC
						MOV		DI,OFFSET TGMENU_EXP
						MOV		AH,60H		; DOS TRUE NAME
						INT		21H

						MOV		SI,OFFSET FREE386_SRC
						MOV		DI,OFFSET FREE386_COM
						MOV		AH,60H		; DOS TRUE NAME
						INT		21H

						MOV		SI,OFFSET RUN386_SRC
						MOV		DI,OFFSET RUN386_EXE
						MOV		AH,60H		; DOS TRUE NAME
						INT		21H


MAIN_LOOP:
						TSUGARU_BREAK
						CALL	FIND_FREE386_OR_RUN386
						JC		EXIT_PROGRAM

						; DS:DX is FREE386 or RUN386 full-path file name.
						PUSH	DX

						MOV		DI,OFFSET EXE_PARAM+1
						MOV		SI,OFFSET TGMENU_EXP
						CALL	STRCPY

						MOV		WORD PTR ES:[DI-1],' '

						MOV		DX,CS
						CALL	USTOX
						MOV		BYTE PTR [DI+4],' '
						MOV		DX,OFFSET DOSXCALLBUF
						ADD		DI,5
						CALL	USTOX
						MOV		BYTE PTR [DI+4],0

						; DI=DI+4-(EXE_PARAM+1)=DI-(EXE_PARAM-3)
						SUB		DI,OFFSET EXE_PARAM-3
						MOV		BX,DI
						MOV		CS:[EXE_PARAM],BL

						MOV		CS:[ENVSEG_COPY],GS

						MOV		WORD PTR CS:[EXE_PARAM_PTR],OFFSET EXE_PARAM
						MOV		WORD PTR CS:[EXE_PARAM_PTR+2],CS

						MOV		WORD PTR CS:[EXE_FIRST_FCB],PSP_FCB1
						MOV		WORD PTR CS:[EXE_FIRST_FCB+2],CS

						MOV		WORD PTR CS:[EXE_SECOND_FCB],PSP_FCB2
						MOV		WORD PTR CS:[EXE_SECOND_FCB+2],CS

						MOV		BX,OFFSET EXE_PARAM_BLOCK
						POP		DX		; From FIND_FREE386_OR_RUN386
						MOV		AX,4B00H
						INT		21H

						TSUGARU_BREAK


EXIT_PROGRAM:
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
;  CF=0  GS:BX  Pointer following '='
;  CF=1  Var not found
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

; Input:
;   DS=CS
;   ES=CS
;   GS=ENVSEG
;   DTA=CS:DOSDTA
; Return:
;   DS:DX  Exe file name.
FIND_FREE386_OR_RUN386:
						MOV		DX,OFFSET DOSDTA
						MOV		AH,1AH		; Set Disk Transfer Area  Needed for FIND_FIRST
						INT		21H			; May change after calling an executable.

						MOV		AX,4E00H	; FIND_FIRST
						XOR		CX,CX		; attrib
						MOV		DX,OFFSET FREE386_COM
						INT		21H
						JNC		FIND_FREE386_OR_RUN386_FOUND

						MOV		AX,4E00H	; FIND_FIRST
						XOR		CX,CX		; attrib
						MOV		DX,OFFSET RUN386_EXE
						INT		21H
						JNC		FIND_FREE386_OR_RUN386_FOUND

						MOV		DX,OFFSET FREE386_SRC
						CALL	FIND_EXE_FROM_PATH
						JNC		FIND_FREE386_OR_RUN386_FOUND

						MOV		DX,OFFSET RUN386_SRC
						CALL	FIND_EXE_FROM_PATH
						JNC		FIND_FREE386_OR_RUN386_FOUND

						; Not Found
						STC
						RET

FIND_FREE386_OR_RUN386_FOUND:
						CLC
						RET


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Input
;   DS:DX  Executable file name
;   DS=CS
;   ES=CS
;   DTA=CS:DOSDTA
; Output
;   CF=0   DS:DX points to EXE_FILE
;   CF=1   not found
FIND_EXE_FROM_PATH:
						PUSH	DX

						; Input
						;  GS     ENV segment
						;  DS:SI  Variable name + '=' eg. "PATH="
						MOV		SI,OFFSET PATH_LABEL
						CALL	FIND_ENV_VAR
						; return
						;  GS:BX  Pointer following '='

						POP		DX
						JC		FIND_EXE_FROM_PATH_NOT_FOUND

FIND_EXE_FROM_PATH_OUTER_LOOP:
						MOV		DI,OFFSET EXE_FILE

FIND_EXE_FROM_PATH_COPY_PATH_LOOP:
						MOV		AL,GS:[BX]
						CMP		AL,';'
						JE		FIND_EXE_FROM_PATH_COPY_PATH_END
						CMP		AL,0
						JE		FIND_EXE_FROM_PATH_COPY_PATH_END

						STOSB
						INC		BX
						JMP		SHORT FIND_EXE_FROM_PATH_COPY_PATH_LOOP

FIND_EXE_FROM_PATH_COPY_PATH_END:
						CMP		DI,OFFSET EXE_FILE
						JE		FIND_EXE_FROM_PATH_COPY_PATH_NEXT

						CMP		BYTE PTR CS:[DI-1],'/'
						JE		FIND_EXE_FROM_PATH_SLASH
						CMP		BYTE PTR CS:[DI-1],'\'
						JE		FIND_EXE_FROM_PATH_SLASH

						MOV		AL,'\'
						STOSB

FIND_EXE_FROM_PATH_SLASH:
						MOV		SI,DX

FIND_EXE_FROM_PATH_APPEND_LOOP:
						LODSB
						STOSB
						CMP		AL,0
						JNE		FIND_EXE_FROM_PATH_APPEND_LOOP

						PUSH	DX
						MOV		DX,OFFSET DOSDTA
						MOV		AH,1AH		; Set Disk Transfer Area  Needed for FIND_FIRST
						INT		21H			; May change after calling an executable.

						MOV		AX,4E00H	; FIND_FIRST
						XOR		CX,CX		; attrib
						MOV		DX,OFFSET EXE_FILE
						MOV		CX,016H
						INT		21H
						POP		DX

						JNC		FIND_EXE_FROM_PATH_COPY_PATH_FOUND

FIND_EXE_FROM_PATH_COPY_PATH_NEXT:
						CMP		BYTE PTR GS:[BX],0
						JE		FIND_EXE_FROM_PATH_NOT_FOUND
						INC		BX
						JMP		FIND_EXE_FROM_PATH_OUTER_LOOP

FIND_EXE_FROM_PATH_COPY_PATH_FOUND:
						CLC
						MOV		DX,OFFSET EXE_FILE
						RET

FIND_EXE_FROM_PATH_NOT_FOUND:
						STC
						RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

NIBBLETOX:
						CMP		AL,10
						JAE		NIBBLE_A_TO_F
						ADD		AL,'0'
						RET
NIBBLE_A_TO_F:
						ADD		AL,'A'-10
						RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Unsigned Short to Hex
; Input:
;   DX
;   DS:DI Location of Hex string
;   DX will be destroyed.
;   DI is preserved.
USTOX:
						MOV		BX,3
USTOXLOOP:
						MOV		AL,DL
						AND		AL,0FH
						CALL	NIBBLETOX
						MOV		DS:[DI+BX],AL
						SHR		DX,4
						DEC		BX
						JNS		USTOXLOOP

						RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Input:
;	DS:SI SRC
;   ES:DI DST
; Output
;   DS:SI After \0 of SRC
;   ES:DI After \0 of DST
STRCPY:
						LODSB
						STOSB
						CMP		AL,0
						JNE		STRCPY
						RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

CUR_DRV					DB		0
OWN_DRV					DB		0

COMSPEC_LABEL			DB		"COMSPEC="
PATH_LABEL				DB		"PATH="

TGMENU_EXP_SRC			DB		"TGMENU.EXP",0
TGMENU_EXP				DB		80 dup(0)

FREE386_SRC				DB		"FREE386.COM",0
FREE386_COM				DB		80 dup(0)

RUN386_SRC				DB		"RUN386.EXE",0
RUN386_EXE				DB		80 dup(0)

DOSDTA					DB		80 dup(0)

EXE_PARAM_BLOCK:
ENVSEG_COPY				DW		0			; Leave zero to use this process's ENVSEG
EXE_PARAM_PTR			DD		0
EXE_FIRST_FCB			DD		0
EXE_SECOND_FCB			DD		0
						DD		0			; Usage unknown
						DD		0			; Usage unknown

DOSXCALLBUF				DB		0			; What to do
EXE_FILE				DB		80 dup(0); Command file name
EXE_PARAM				DB		80 dup(0)	; 



END_OF_PROGRAM:

CODE					ENDS
						END		MAIN
