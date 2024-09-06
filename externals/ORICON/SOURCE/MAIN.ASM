		.286
_TEXT		SEGMENT  WORD PUBLIC 'CODE'
_TEXT		ENDS
_DATA		SEGMENT  WORD PUBLIC 'DATA'
_DATA		ENDS
CONST		SEGMENT  WORD PUBLIC 'CONST'
CONST		ENDS
_BSS		SEGMENT  WORD PUBLIC 'BSS'
_BSS		ENDS
_INST		SEGMENT  WORD PUBLIC 'INST'
_INST		ENDS

DGROUP		GROUP	_TEXT, CONST, _BSS, _DATA, _INST

extrn		_setbios:near
extrn		_resetbios:near
extrn		His_init:near
extrn		His_end:near
extrn		_VDB_00:near
extrn		_cflush:near
extrn   	oak_set:near
extrn   	oak_reset:near

_DATA		segment

Exe_Param	dw	?			; Envp Seg	+0
		dw	?			; para offset	+2
		dw	?			;      seg	+4
		dw	offset DGROUP:Exe_fcb1	; FCB1 offset	+6
		dw	?			;      seg	+8
		dw	offset DGROUP:Exe_fcb2	; FCB2 offset	+10
		dw	?			;      seg	+12

Exe_fcb1	db	16 dup (0)
Exe_fcb2	db	16 dup (0)

Command		db	'C:\COMMAND.COM'
		db	128 dup (0)
Def_Param	db	0,0Dh

_DATA		ends

_TEXT		segment
		ASSUME CS: DGROUP, DS: DGROUP, ES: DGROUP

		org	02ch
EnvSeg		label	word

		org	080h
CmdsLine	label	word

		org	100h
entry:		jmp	main

_TEXT		ends

_INST		segment
		ASSUME CS: DGROUP, DS: DGROUP, ES: DGROUP

main		proc	near
		mov	ax,cs
		mov	ds,ax
		mov	es,ax
		cld

		call	Screen_init
		call	_setbios
		call	oak_set
		call	His_init
		call	Path_init
		call	PF_key_init

		mov	sp,offset DGROUP:Local_Stack

		mov	ax,cs
		mov	es,ax
		mov	bx,offset DGROUP:End_of_Prog
		add	bx,15
		shr	bx,4
		mov	ah,4Ah
		int	21h

		mov	di,offset DGROUP:Exe_Param
		mov	ax,EnvSeg
		mov	[di+0],ax

		mov	ax,ds
		mov	[di+4],ax
		mov	[di+8],ax
		mov	[di+12],ax

		mov	si,offset DGROUP:CmdsLine
		lodsb
		cmp	al,0
		je	Def_Fork
		mov	ah,al

main_1:		cmp	ah,0
		je	Def_Fork
		dec	ah
		lodsb
		cmp	al,0Dh
		je	Def_Fork
		cmp	al,' '
		je	main_1
		cmp	al,09h
		je	main_1

		mov	bx,offset DGROUP:Command
		jmp	main_5

main_2:		cmp	ah,0
		je	main_4
		dec	ah
		lodsb
		cmp	al,0Dh
		je	main_4
		cmp	al,' '
		je	main_3
		cmp	al,09h
		je	main_3
main_5:		mov	[bx],al
		inc	bx
		jmp	short main_2

main_3:		dec	si
		mov	[si],ah
		mov	[di+2],si
		mov	byte ptr [bx],0
		jmp	short Com_Fork

main_4:		mov	byte ptr [bx],0
Def_Fork:	mov	word ptr [di+2],offset DGROUP:Def_Param

Com_Fork:	push	ds
		pop	es
		mov	si,[di+2]
		mov	di,offset DGROUP:Exe_fcb1
		mov	ax,2901h
		int	21h
		mov	di,offset DGROUP:Exe_fcb2
		mov	ax,2901h
		int	21h

		mov	dx,offset DGROUP:Command
		mov	bx,offset DGROUP:Exe_Param
		mov	ax,4B00h
		int	21h

		call	His_end
		call	_resetbios
		call	oak_reset

		mov	ax,4C00h		; End of Oricon
		int	21h

main		endp

		db	256 dup (?)
Local_Stack	label	word
		public	End_of_Prog
End_of_Prog	label	word

Pal_Tbl		db	000h,000h,000h
		db	0B0h,000h,000h
		db	000h,0B0h,000h
		db	0B0h,0B0h,000h
		db	000h,000h,0B0h
		db	0B0h,000h,0B0h
		db	000h,0B0h,0B0h
		db	0B0h,0B0h,0B0h
		db	0B0h,0B0h,0B0h
		db	0F0h,000h,000h
		db	000h,0F0h,000h
		db	0F0h,0F0h,000h
		db	000h,000h,0F0h
		db	0F0h,000h,0F0h
		db	000h,0F0h,0F0h
		db	0F0h,0F0h,0F0h

CRT_Reg_Tbl	dw	0040h,0320h,0000h,0000h
		dw	035Fh,0000h,0010h,0000h
		dw	036Fh,009Ch,031Ch,009Ch
		dw	031Ch,0040h,0360h,0040h
		dw	0360h,0000h,009Ch,0000h
		dw	0050h,0000h,009Ch,0000h
		dw	0050h,004Ah,0001h,0000h
		dw	003Fh,0003h,0000h,0150h

Palet_init	proc	near
		sub	cl,cl
		mov	si,offset DGROUP:Pal_Tbl
Pal_init_1:	mov	al,cl
		mov	dx,0FD90h
		out	dx,al
		lodsb
		mov	dx,0FD92h
		out	dx,al
		lodsb
		mov	dx,0FD94h
		out	dx,al
		lodsb
		mov	dx,0FD96h
		out	dx,al
		inc	cl
		cmp	cl,16
		jb	Pal_init_1
		ret	
Palet_init	endp

;
; SI = Data Address
;
CRTC_set	proc	near
		sub	cl,cl
CRTC_set_1:	mov	al,cl
		mov	dx,0440h
		out	dx, al
		lodsw
		mov	dx,0442h
		out	dx, ax
		inc	cl
		cmp	cl,32
		jb	CRTC_set_1
		ret	
CRTC_set	endp

;
; AL = Val
;
CRTC_onoff	proc	near
		push	ax
		mov	al,01Ch
		mov	dx,0440h
		out	dx,al
		pop	ax
		mov	dx,0443h
		out	dx,al
		ret
CRTC_onoff	endp

;
; SI = Data Address
;
CRTC_init	proc	near
		mov	al,0
		call	CRTC_onoff
		call	CRTC_set
		mov	al,byte ptr [CRT_Reg_Tbl+01Ch*2]
		or	al,80h
		call	CRTC_onoff
		ret
CRTC_init	endp

;
; AX = Val
;
Video_init	proc	near
		push	ax
		mov	al,0
		mov	dx,0448h
		out	dx,al
		pop	ax
		mov	dx,044Ah
		out	dx,al

		mov	al,1
		mov	dx,0448h
		out	dx,al
		mov	al,ah
		mov	dx,044Ah
		out	dx,al
		ret
Video_init	endp

Screen_init	proc	near

		mov	al,0
		mov	dx,0FDA0h
		out	dx,al

		mov	si,offset DGROUP:CRT_Reg_Tbl
		call	CRTC_init

		mov	ax,0815h
		call	Video_init

		call	Palet_init
		call	_VDB_00
		call	_cflush

		mov	al,0Fh
		mov	dx,0FDA0h
		out	dx,al
		ret
Screen_init	endp

pf_key_tbl	db	01h,80h, 2,1Bh,'S'	;PF1
		db	02h,80h, 2,1Bh,'T'	;PF2
		db	03h,80h, 2,1Bh,'U'	;PF3
		db	04h,80h, 2,1Bh,'V'	;PF4
		db	05h,80h, 2,1Bh,'W'	;PF5
		db	06h,80h, 2,1Bh,'E'	;PF6
		db	07h,80h, 2,1Bh,'J'	;PF7
		db	08h,80h, 2,1Bh,'P'	;PF8
		db	09h,80h, 2,1Bh,'Q'	;PF9
		db	0ah,80h, 2,1Bh,'Z'	;PF10

		db	0bh,80h, 2,1Bh,'S'	;PF11
		db	1dh,80h, 2,1Bh,'T'	;PF12
		db	21h,80h, 2,1Bh,'U'	;PF13
		db	22h,80h, 2,1Bh,'V'	;PF14
		db	23h,80h, 2,1Bh,'W'	;PF15
		db	24h,80h, 2,1Bh,'E'	;PF16
		db	25h,80h, 2,1Bh,'J'	;PF17
		db	26h,80h, 2,1Bh,'P'	;PF18
		db	27h,80h, 2,1Bh,'Q'	;PF19
		db	28h,80h, 2,1Bh,'Z'	;PF20

		db	12h,00h, 2,1Bh,'P'	;[INS]
		db	7fh,00h, 2,1Bh,'V'	;[DEL]
		db	18h,80h, 0		;[半角/全角]
		db	1ch,80h, 0		;[かな漢字]
		db	0,0

Start_msg	db	'FM-Towns FMR50 コンソ－ルシュミレ－タ v1.09'
		db	0Dh,0Ah
		db	'Programmed by K.MIYAZAKI'
		db	0Dh,0Ah
		db	'Asistant programmer N.YAMAZAKI'
		db	0Dh,0Ah
		db	0Dh,0Ah
		db	'FM-Towns OAK/2 フロントエンドプロセッサー v1.04a'
		db	0Dh,0Ah
		db	'Programmed by N.YAMAZAKI'
		db	0Dh,0Ah
		db	'Investigated by K.MIYAZAKI Thanks a lot.'
		db	0Dh,0Ah
		db	'$'

PF_key_init	proc	near
		mov	di,offset DGROUP:pf_key_tbl
		xor	cx,cx
key_set:	mov	dx,word ptr [di]
		cmp	dx,0
		je	key_st_e
		add	di,2
		mov	cl,[di]
		inc	di
		mov	ax,0E00h
		int	90h
		add	di,cx
		jmp	key_set

key_st_e:	mov	dx,offset DGROUP:Start_Msg
		mov	ah,09h
		int	21h
		ret
PF_key_init	endp

extrn		_path_top:near

Path_init	proc	near
		ASSUME DS: nothing, ES: DGROUP

		push	ds
		push	es

		mov	ax,ds
		mov	es,ax

		mov	ds,es:[EnvSeg]
		mov	si,0
		mov	di,offset DGROUP:_path_top

_Path_1:	cmp	byte ptr ds:[si],0
		je	_Path_4

		mov	ax,ds:[si]
		and	ax,0DFDFh
		cmp	ax,'A'*256+'P'
		jne	_Path_3
		mov	ax,ds:[si+2]
		and	ax,0DFDFh
		cmp	ax,'H'*256+'T'
		jne	_Path_3
		cmp	byte ptr ds:[si+4],'='
		jne	_Path_3

		add	si,5
_Path_2:	cmp	byte ptr ds:[si],0
		je	_Path_4
		cmp	di,offset DGROUP:_path_top+127
		jnb	_Path_4
		movsb
		jmp	_Path_2

_Path_3:	lodsb
		cmp	al,0
		jne	_Path_3
		jmp	_Path_1

_Path_4:	mov	byte ptr es:[di],0
		pop	es
		pop	ds
		ret
		ASSUME DS: DGROUP, ES: nothing

Path_init	endp

_INST		ends
		end	entry
