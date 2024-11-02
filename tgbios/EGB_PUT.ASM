						.386p

						PUBLIC		EGB_PUTBLOCK_16_MATTE
						PUBLIC		EGB_PUTBLOCK_16_PSET

						ASSUME CS:CODE

CODE					SEGMENT



; extern void EGB_PUTBLOCK_16_PSET(
; 	_Far unsigned char *dstPtr,
; 	_Far unsigned char *srcPtr,
; 	unsigned int xCount,
; 	unsigned int yCount,
; 	unsigned int dstBytesPerLine,
; 	unsigned int srcBytesPerLine);

;					for(y=p0.y; y<=p1.y; ++y)
;					{
;						MEMCPY_FAR(vram,src,transferBytesPerLine);
;						src+=srcBytesPerLine;
;						vram+=scrnMode->bytesPerLine;
;					}
EGB_PUTBLOCK_16_PSET	PROC
; ESP+40H	srcBytesPerLine
; ESP+3CH	dstBytesPerLine
; ESP+38H	yCount
; EXP+34H	xCount
; ESP+2CH	srcPtr
; ESP+24H	dstPtr
; ESP+20H	Return EIP
; ESP+1CH	DS
; ESP+18H	ES
; ESP+14H	EBP
; ESP+10H	ESI
; ESP+0CH	EDI
; ESP+08H	EBX
; ESP+04H	ECX
; ESP		EDX

						PUSH	DS
						PUSH	ES
						PUSH	EBP
						PUSH	ESI
						PUSH	EDI
						PUSH	EBX
						PUSH	ECX
						PUSH	EDX

						LES		EDI,[ESP+24H] ; dstPtr
						LDS		ESI,[ESP+2CH] ; srcPtr

						CLD
						MOV		EDX,[ESP+38H] ; yCount
						MOV		EBP,[ESP+34H] ; xCount

						MOV		EAX,[ESP+40H] ; srcBytesPerLine
						MOV		EBX,[ESP+3CH] ; dstBytesPerLine

						SHL		EBP,1
						SUB		EAX,EBP
						SUB		EBX,EBP
						SHR		EBP,1

EGB_PUTBLOCK_16_PSET_YLOOP:
						MOV		ECX,EBP

						SHR		ECX,1
						REP		MOVSD

						JNC		EGB_PUTBLOCK_16_PSET_NEXT
						MOVSW

EGB_PUTBLOCK_16_PSET_NEXT:
						ADD		ESI,EAX
						ADD		EDI,EBX
						DEC		EDX
						JNE		EGB_PUTBLOCK_16_PSET_YLOOP

						POP		EDX
						POP		ECX
						POP		EBX
						POP		EDI
						POP		ESI
						POP		EBP
						POP		ES
						POP		DS
						RET

EGB_PUTBLOCK_16_PSET	ENDP



; EGB_PUTBLOCK_16_MATTE(dstPtr,srcPtr,xCount,p1.y+1-p0.y,srcBytesPerLine,scrnMode->bytesPerLine,transparentColor);

;						for(y=p0.y; y<=p1.y; ++y)
;						{
;							register _Far unsigned short *srcPtr,*dstPtr;
;							srcPtr=(_Far unsigned short *)src;
;							dstPtr=(_Far unsigned short *)vram;
;							for(count=xCount; 0<count; --count)
;							{
;								if(*srcPtr!=transparentColor)
;								{
;									*dstPtr=*srcPtr;
;								}
;								++dstPtr;
;								++srcPtr;
;							}
;							src+=srcBytesPerLine;
;							vram+=scrnMode->bytesPerLine;
;						}

; ESP+48H	transparentColor
; ESP+44H	srcBytesPerLine
; ESP+40H	dstBytesPerLine
; ESP+3CH	yCount
; ESP+38H	xCount
; ESP+30H	_Far srcPtr
; ESP+28H	_Far dstPtr
; ESP+24H	Return EIP
; ESP+20H	DS
; ESP+1CH	ES
; ESP+18H	ESI
; ESP+14H	EDI
; ESP+10H	EDX
; ESP+0CH	ECX
; ESP+08H	EBX
; ESP+04H	EBP
; ESP+00H	EFLAGS

EGB_PUTBLOCK_16_MATTE	PROC
						PUSH	DS
						PUSH	ES
						PUSH	ESI
						PUSH	EDI
						PUSh	EDX
						PUSH	ECX
						PUSH	EBX
						PUSH	EBP
						PUSHFD

						LES		EDI,[ESP+28H] ; dstPtr
						LDS		ESI,[ESP+30H] ; srcPtr

						MOV		BX,[ESP+48H] ; transparentColor

						MOV		EBP,[ESP+38H] ; xCount
						MOV		EDX,[ESP+3CH] ; yCount
						CLD

						SHL		EBP,1
						SUB		[ESP+40H],EBP	; =dstBytesPerLine-xCount*2
						SUB		[ESP+44H],EBP	; =srcBytesPerLine-xCount*2
						SHR		EBP,1

EGB_PUTBLOCK_16_MATTE_YLOOP:
						MOV		ECX,EBP

EGB_PUTBLOCK_16_MATTE_XLOOP:
						LODSW
						CMP		AX,BX
						JE		EGB_PUTBLOCK_16_MATTE_TRANS
						STOSW
						LOOP	EGB_PUTBLOCK_16_MATTE_XLOOP
						JMP		EGB_PUTBLOCK_16_MATTE_NEXTY

EGB_PUTBLOCK_16_MATTE_TRANS:
						ADD		EDI,2
						LOOP	EGB_PUTBLOCK_16_MATTE_XLOOP

EGB_PUTBLOCK_16_MATTE_NEXTY:
						ADD		EDI,[ESP+40H]
						ADD		ESI,[ESP+44H]

						DEC		EDX
						JNE		EGB_PUTBLOCK_16_MATTE_YLOOP

						POPFD
						POP		EBP
						POP		EBX
						POP		ECX
						POP		EDX
						POP		EDI
						POP		ESI
						POP		ES
						POP		DS
						RET
EGB_PUTBLOCK_16_MATTE	ENDP


CODE					ENDS
						END
