						.386p

						PUBLIC		EGB_PUTBLOCK_16_MATTE

						ASSUME CS:CODE

CODE					SEGMENT
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
; ESP+44H	dstBytesPerLine
; ESP+40H	srcBytesPerLine
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
						SUB		[ESP+40H],EBP	; =srcBytesPerLine-xCount*2
						SUB		[ESP+44H],EBP	; =dstBytesPerLine-xCount*2
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
						ADD		ESI,[ESP+40H]
						ADD		EDI,[ESP+44H]

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
