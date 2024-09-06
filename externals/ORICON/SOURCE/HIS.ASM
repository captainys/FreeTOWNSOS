;
;	Command Line History Input TSR
;
;	1991.07.12	make by Ken
;
		.286
_TEXT		SEGMENT  WORD PUBLIC 'CODE'
_TEXT		ENDS
_DATA		SEGMENT  WORD PUBLIC 'DATA'
_DATA		ENDS
CONST		SEGMENT  WORD PUBLIC 'CONST'
CONST		ENDS
_BSS		SEGMENT  WORD PUBLIC 'BSS'
_BSS		ENDS

DGROUP		GROUP	_TEXT, CONST, _BSS, _DATA

		ASSUME CS: DGROUP, DS: DGROUP, SS: DGROUP

extrn		_input:near
extrn		_dta_bak:dword
extrn		_dta:near

_DATA		segment
UnGetBuf	db	0FFh
_DATA		ends

_TEXT		segment

Vect_21		dd	far

		public	_PUTC
_PUTC		proc	near
		push	bp
		mov	bp,sp
		mov	dl,[bp+4]
		mov	ah,02h
		int	21h
		pop	bp
		ret
_PUTC		endp

		public	_PUTS
_PUTS		proc	near
		push	bp
		mov	bp,sp
		push	bx
		push	dx

		mov	ah,02h
		mov	bx,[bp+4]
		mov	cx,[bp+6]
		cmp	cx,0
		jle	_PUTS_5

_PUTS_1:	mov	dl,[bx]
		inc	bx
		cmp	dl,0
		je	_PUTS_5
		cmp	dl,0Ah
		jne	_PUTS_2
		mov	dl,0Dh
		jmp	_PUTS_3

_PUTS_2:	cmp	dl,20h
		jnb	_PUTS_3
		mov	dl,'.'

_PUTS_3:	int	21h
		loop	_PUTS_1

_PUTS_5:	pop	dx
		pop	bx
		pop	bp
		ret
_PUTS		endp

		public	_BEEP
_BEEP		proc	near
		push	dx
		mov	ah,02h
		mov	dl,07h
		int	21h
		pop	dx
		ret
_BEEP		endp

		public	_FLUSH
_FLUSH		proc	near
		ret
_FLUSH		endp

		public	_BAKSPC
_BAKSPC		proc	near
		push	bp
		mov	bp,sp
		push	dx

		mov	cx,[bp+4]
		cmp	cx,0
		jle	_BAKSPC_2

		mov	ah,02h
		mov	dl,08h

_BAKSPC_1:	int	21h
		loop	_BAKSPC_1

_BAKSPC_2:	pop	dx
		pop	bp
		ret
_BAKSPC		endp

		public	_REPCHR
_REPCHR		proc	near
		push	bp
		mov	bp,sp
		push	dx

		mov	cx,[bp+6]
		cmp	cx,0
		jle	_REPCHR_2

		mov	ah,02h
		mov	dl,[bp+4]

_REPCHR_1:	int	21h
		loop	_REPCHR_1

_REPCHR_2:	pop	dx
		pop	bp
		ret
_REPCHR		endp

		public	_GETCH
_GETCH		proc	near
		mov	al,UnGetBuf
		cmp	al,0FFh
		jne	GetCh_1			; UnGetBuf TRUE ?
		mov	ah,08h			; Non Echo Get Char
		int	21h
GetCh_1:	mov	UnGetBuf,0FFh
		sub	ah,ah
		ret
_GETCH		endp

		public	_UNGETCH
_UNGETCH	proc	near
		push	bp
		mov	bp,sp
		mov	al,[bp+4]
		mov	UnGetBuf,al
		pop	bp
		ret
_UNGETCH	endp

		public	_tolow
_tolow		proc	near
		push	bp
		mov	bp,sp
		mov	ax,[bp+4]
		sub	ah,ah
		cmp	al,'A'
		jb	_tolow_1
		cmp	al,'Z'
		ja	_tolow_1
		or	al,20h
_tolow_1:	pop	bp
		ret
_tolow		endp

		public	_strlow
_strlow		proc	near
		push	bp
		mov	bp,sp
		push	bx

		mov	bx,[bp+4]

_strlow_1:	mov	al,[bx]
		cmp	al,0
		je	_strlow_3
		cmp	al,'A'
		jb	_strlow_2
		cmp	al,'Z'
		ja	_strlow_2
		or	al,20h
_strlow_2:	mov	[bx],al
		inc	bx
		jmp	_strlow_1

_strlow_3:	pop	bx
		pop	bp
		ret
_strlow		endp

		public	_DTA_init	
_DTA_init	proc	near
		push	es
		push	bx

		mov	ah,2Fh
		int	21h
		mov	word ptr [_dta_bak],bx
		mov	word ptr [_dta_bak+2],es

		mov	ah,1Ah
		mov	dx,offset DGROUP:_dta
		int	21h

		pop	bx
		pop	es
		ret
_DTA_init	endp

		public	_DTA_end
_DTA_end	proc	near
		push	ds

		mov	ah,1Ah
		mov	dx,word ptr [_dta_bak]
		mov	ax,word ptr [_dta_bak+2]
		mov	ds,ax
		int	21h

		pop	ds
		ret
_DTA_end	endp

		public	_farst_call
_farst_call	proc	near
		push	bp
		mov	bp,sp

		mov	ah,4Eh
		mov	cx,21h
		mov	dx,[bp+4]
		int	21h
		mov	ax,1
		jnc	_farst_1
		sub	ax,ax

_farst_1:	pop	bp
		ret
_farst_call	endp

		public	_next_call
_next_call	proc	near
		mov	ah,4Fh
		int	21h
		mov	ax,1
		jnc	_next_1
		sub	ax,ax
_next_1:	ret
_next_call	endp

;*******************************************************************

His_Go		proc	near
		pusha
		push	ds
		push	es

		sti
		push	ds
		push	dx

		mov	bx,dx
		mov	al,ds:[bx]
		sub	ah,ah

		push	cs
		pop	ds

		push	ax
		call	_input
		add	sp,2

		mov	si,ax
		pop	di
		pop	es

		mov	bx,di
		add	di,2
		sub	ah,ah

His_Go_1:	lodsb
		or	al,al
		je	His_Go_3
		cmp	al,0Ah
		jne	His_Go_2
		mov	al,0Dh
		stosb
		jmp	His_Go_3

His_Go_2:	cmp	ah,es:[bx]
		jnb	His_Go_3
		stosb
		inc	ah
		jmp	short His_Go_1

His_Go_3:	mov	es:[bx+1],ah

		pop	es
		pop	ds
		popa
		iret
His_Go		endp

Trap_21		proc	near
		cmp	ah,0Ah			; Dos Line Input Call
		je	His_Go
		jmp	cs:[Vect_21]
Trap_21		endp

;*******************************************************************

		public	His_init
His_init	proc	near
		push	ds

		mov	ax,3521h		; Get 21h Int Vect
		int	21h
		mov	word ptr cs:[Vect_21],bx
		mov	ax,es
		mov	word ptr cs:[Vect_21+2],ax

		push	cs
		pop	ds
		mov	dx,offset DGROUP:Trap_21
		mov	ax,2521h		; Set Now Vect
		int	21h

		pop	ds
		ret

His_init	endp

		public	His_end
His_end		proc	near

		push	ds

		lds	dx,cs:[Vect_21]
		mov	ax,2521h		; Set 21h int Vect
		int	21h

		pop	ds
		ret

His_end		endp

_TEXT		ends
		end
