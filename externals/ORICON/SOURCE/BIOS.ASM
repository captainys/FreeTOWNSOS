		.286p
_TEXT		SEGMENT  DWORD PUBLIC 'CODE'
_TEXT		ENDS
_DATA		SEGMENT  WORD PUBLIC 'DATA'
_DATA		ENDS
CONST		SEGMENT  WORD PUBLIC 'CONST'
CONST		ENDS
_BSS		SEGMENT  WORD PUBLIC 'BSS'
_BSS		ENDS
DGROUP		GROUP	 _TEXT, CONST,	_BSS,	_DATA

		ASSUME  CS: _TEXT, DS: DGROUP, ES: DGROUP

extrn		_BIOS_91:NEAR
extrn   	_OLD_X:WORD
extrn		_CUR_X:WORD
extrn		_CUR_Y:WORD
extrn		_CUR_OFF2:WORD
extrn		_CUR_SIZE2:WORD
extrn		_CUR_DSP_FLG:BYTE
extrn		_Act_Dsp:BYTE

GVRAM		equ	0C000h
CVRAM		equ	0C800h
KVRAM		equ	0CA00h
ANKCG		equ	0CB00h

MAX_X		equ	80
MAX_Y		equ	24
MAX_Y2		equ	25
CHRSIZ		equ	16
LINSIZ		equ	16

_DATA		segment
consol_path	db	"CON",0			; コンソ－ルデバイス名
null_path	db	"NUL",0			; NULデバイス名
_DATA		ends

_TEXT		SEGMENT

;*****************************
;*	Work Vram Eria
;****************************
dmy_ram		dw	(MAX_X*MAX_Y2) dup(0)
;*****************************
;*    割り込みワ－ク
;*****************************
ds_bak		dw	?
es_bak		dw	?
vct_off		dw	?
vct_seg		dw	?
;*********************
;*	VSYNC
;*********************
IRQ_CODE 	equ	0Bh
VS_TIME		equ	4		; 画面更新間隔の設定

vs_para		db	0,0
vs_off		dw	?
vs_seg		dw	?
old_off		dw	?
old_seg		dw	?
irq_map		db	0,0,0,0
vs_cnt		db	0

int_ent 	proc	far
		mov	dx,05CAh
		out	dx,al
		sti

		inc	cs:[vs_cnt]
		cmp	cs:[vs_cnt],VS_TIME
		jb	int_rt
		mov	cs:[vs_cnt],0

		mov	ds,cs:[ds_bak]
		mov	es,cs:[es_bak]
		cmp	byte ptr [_Act_Dsp],0	;割り込み禁止フラグ
		jne	int_rt

		mov	dx,05C8h
		in	al,dx
		test	al,80h
		jnz	int_go

		mov	ah,byte ptr [_CUR_X]
		mov	al,byte ptr [_CUR_Y]
		cmp	ax,[_OLD_X]
		jz	int_rt

int_go:		mov	byte ptr [_Act_Dsp],1	;多重割り込み防止
		call	_cflush
		mov	byte ptr [_Act_Dsp],0

		mov	ah,byte ptr [_CUR_X]
		mov	al,byte ptr [_CUR_Y]
		mov	[_OLD_X],ax

int_rt:		ret
int_ent 	endp

vs_set		proc	near
		push	ds

		mov	ah,01h
		mov	dl,IRQ_CODE
		int	0AEh
		mov	cs:[old_off],di
		mov	cs:[old_seg],ds

		push	cs
		pop	ds
		mov	di,offset int_ent
		mov	cs:[vs_off],di
		mov	cs:[vs_seg],ds
		mov	di,offset vs_para
		mov	ah,00h
		mov	dl,IRQ_CODE
		int	0AEh

		mov	ah,03h
		mov	di,offset irq_map
		int	0AEh
		or	byte ptr cs:[di+2],08h
		mov	ah,02h
		int	0AEh

		pop	ds
		ret
vs_set		endp

vs_reset 	proc	near
		push	ds

		push	cs
		pop	ds
		mov	ah,03h
		mov	di,offset irq_map
		int	0AEh
		and	byte ptr cs:[di+2],0F7h
		mov	ah,02h
		int	0AEh

		mov	di,cs:[old_off]
		mov	ds,cs:[old_seg]
		mov	ah,00h
		mov	dl,IRQ_CODE
		int	0AEh

		pop	ds
		ret
vs_reset 	endp
;******************************
;*    コンソ－ルBIOS(int 91h)
;******************************
bios_ent 	proc	far
		pusha
		push	ds	;+2
		push	es	;+0
		sti
		cld
		mov	bp,sp
		mov	ds,cs:[ds_bak]
		mov	es,cs:[es_bak]
		push	ss
		push	bp
		call	_BIOS_91
		add	sp,4
		pop	es
		pop	ds
		popa
		iret
bios_ent 	endp
;**************************************************************
;
;	ファイルのリダイレクト処理
;
; al	=	ファイルのオ－プンモ－ド
; bx	=	ファイルパス番号
; dx	=	NEWデバイスネ－ム
;
;**************************************************************
reopen		proc	near
		push	ax
		mov	ah,3Eh				; CLOSE
		int	21h
		pop	ax
		mov	ah,3Dh				; OPEN
		int	21h
		ret
reopen		endp
;*********************************************
;*    コンソ－ルBIOSベクタ(int 91h) &
;*    タイマ割り込みの設定
;*********************************************
		PUBLIC	_setbios
_setbios 	PROC 	NEAR
		push	ds
		push	es

		mov     al,0				; 標準入出力を
		mov     bx,0				; コンソ－ルデバイス
		mov	dx,offset DGROUP:consol_path	; にリダイレクトする
		call    reopen				; 入力
		mov     al,1				;
		mov     bx,1				;
		call    reopen				; 出力
		mov     al,1				;
		mov     bx,2				;
		call    reopen				; エラ－出力

		mov	cs:[ds_bak],ds
		mov	cs:[es_bak],es

		mov	ax,3591h		; int 91h
		int	21h
		mov	ax,es
		mov	cs:[vct_seg],ax
		mov	cs:[vct_off],bx

		mov	dx,offset bios_ent
		push	cs
		pop	ds
		mov	ax,2591h		; int 91h
		int	21h

		call	vs_set

		pop	es
		pop	ds
		ret	
_setbios 	endp
;***************************************
;*    ベクタ及び割り込みの解除 & 終了
;***************************************
		PUBLIC	_resetbios
_resetbios 	PROC NEAR
		push	ds
		push	es

		mov	al,0				; 標準入出力を
		mov	bx,0				; NULにリダイレクト
		mov	dx,offset DGROUP:null_path	;
		call	reopen				; 入力
		mov	al,1				;
		mov	bx,1				;
		call	reopen				; 出力
		mov	al,1				;
		mov	bx,2				;
		call	reopen				; エラ－出力

		call	vs_reset

		mov	ax,cs:[vct_seg]
		mov	ds,ax
		mov	dx,cs:[vct_off]
		mov	ax,2591h
		int	21h

		pop	es
		pop	ds
		ret	
_resetbios 	endp
;**********************************************
;*	colset(char *vram,int color,int byte);
;**********************************************
		PUBLIC	_colset
_colset		proc	near
		push	bp
		mov	bp,sp
		push	es
		push	di

		cld
		mov	ax,CVRAM
		mov	es,ax
		mov	di,[bp+4]
		sub	al,al
		mov	ah,[bp+6]
		mov	cx,[bp+8]
		shr	cx,1
		je	colst1
	rep	stosw

colst1:		pop	di
		pop	es
		pop	bp
		ret
_colset		endp
;*****************************************************
;*	vramcpy(char *des,char *src,int byte);
;*****************************************************
		PUBLIC	_vramcpy
_vramcpy 	proc	near
		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	di
		push	si

		cld
		mov	ax,CVRAM
		mov	ds,ax
		mov	es,ax
		mov	cx,[bp+8]
		shr	cx,1
		je	vfcp1

		mov	di,[bp+4]
		mov	si,[bp+6]
;		mov	byte ptr [_Act_Dsp],1	;漢字表示が乱れるなら
	rep	movsw

		mov	ax,KVRAM
		mov	ds,ax
		mov	es,ax
		mov	di,[bp+4]
		mov	si,[bp+6]
		mov	cx,[bp+8]
		shr	cx,1
	rep	movsw
;		mov	byte ptr [_Act_Dsp],0	; このフラグを操作

vfcp1:		pop	si
		pop	di
		pop	es
		pop	ds
		pop	bp
		ret
_vramcpy 	endp
;*****************************************************
;*	vramcpy(char *src,char *des,int byte);
;*****************************************************
		PUBLIC	_vramrcpy
_vramrcpy 	proc	near
		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	di
		push	si

		std
		mov	ax,CVRAM
		mov	ds,ax
		mov	es,ax
		mov	cx,[bp+8]
		shr	cx,1
		je	vrcp1

		mov	di,[bp+4]
		mov	si,[bp+6]
	rep	movsw

		mov	ax,KVRAM
		mov	ds,ax
		mov	es,ax
		mov	di,[bp+4]
		mov	si,[bp+6]
		mov	cx,[bp+8]
	rep	movsw

vrcp1:		cld
		pop	si
		pop	di
		pop	es
		pop	ds
		pop	bp
		ret
_vramrcpy 	endp
;*********************************
;*	int sjisto(int code);
;*********************************
		PUBLIC	_sjisto
_sjisto 	proc	near
		push	bp
		mov	bp,sp
		mov	ax,[bp+4]
		shl	ah,1
		cmp	al,080h
		adc	ax,1f61h
		add	al,7fh
		jc	short knf1
		add	al,0a2h
knf1:		and	ah,7fh
		pop	bp
		ret
_sjisto		endp
;*************************
;*	Beep();
;*************************
		PUBLIC	_Beep
_Beep		proc	near
		push	dx
		push	cx
		mov	dx,0FF98h
		in	al,dx
		mov	cx,5000		; x 10us
		int	0FDh
		out	dx,al
		pop	cx
		pop	dx
		ret
_Beep		ENDP
;********************************************
;*
;*	AsinKey(int ch,int byte,char *str)
;*
;********************************************
		public	_AsinKey
_AsinKey 	proc	near
		push	bp
		mov	bp,sp
		push	di

		mov	ax,0E00h	; Assin String
		mov	dx,[bp+4]	; Key Code
		mov	cx,[bp+6]	; Str Count
		mov	di,[bp+8]	; Str Addr
		int	90h		; Key BIOS

		pop	di
		pop	bp
		ret
_AsinKey 	endp
;********************************************
;*
;*	KeyBufIns(short int *key,int len)
;*
;********************************************
		public	_KeyBufIns
_KeyBufIns 	proc	near
		push	bp
		mov	bp,sp
		push	di

		mov	ax,0B00h	; Insert Key Buffer
		mov	cx,[bp+6]	; Str Count
		mov	di,[bp+4]	; Str Addr
		int	90h		; Key BIOS

		pop	di
		pop	bp
		ret
_KeyBufIns 	endp
;****************************************
; Wrt_Ank
;   al:char ah:attr
;   cx:x    bx:y
;****************************************
		align	4
_Wrt_Ank 	proc	near
		push	ds
		push	si
		push	es
		push	di
		push	cx
		push	bx

		mov	dx,ax
		sub	dh,dh		; char=>ANKCG address [si]
		shl	dx,4		; * 16
		mov	si,dx
		mov	di,bx
		add	di,cx

		mov	bl,ah
		mov	cl,bl		;cl
		and	bx,7

		test	cl,20h		;cl
		je	$I147
		or	bl,8		;at

$I147:		test	cl,18h		;cl
		je	$I148
		dec	bh		;bk bh=0ffh

$I148:		mov	ax,ANKCG
		mov	ds,ax

		mov	ax,GVRAM
		mov	es,ax

		mov	dx,0FF99h
		mov	al,01h
		out	dx,al

		mov	al,0Fh
		mov	dx,0FF81h
		out	dx,al
		mov	cx,80-1
		cmp	al,bl
		je	$Sclr

		sub	al,al

		REPT	16
		stosb
		add	di,cx
		endm

		sub	di,80*16
		mov	al,bl
		out	dx,al

$Sclr:		or	bh,bh
		jne	$FF100

		REPT	16
		movsb
		add	di,cx
		endm
		jmp	$FF101

		align	4
$FF100:		REPT	16
		lodsb
		xor	al,bh
		stosb
		add	di,cx
		endm

$FF101:		mov	dx,0FF99h
		xor	al,al
		out	dx,al

		pop	bx
		pop	cx
		pop	di
		pop	es
		pop	si
		pop	ds
		ret	
_Wrt_Ank	ENDP
;************************************
; Wrt_Kan
;   dx:漢字(反転注意)	ah:attr
;   cx:x          	bx:y
;************************************
		align	4
_Wrt_Kan 	PROC NEAR
		push	es
		push	di
		push	cx
		push	bx

		mov	di,bx
		add	di,cx		;di=VRAM_Address
		mov	cl,ah

		mov	bx,cx
		mov	cx,dx
		xchg	ch,cl

;****** Bug Fix Start *******
		cmp	ch,70h
		jb	Not_Cnv
		cmp	ch,74h
		ja	Not_Cnv
		cmp	cl,60h
		jb	Not_Cnv
		cmp	cl,7Eh
		ja	Not_Cnv
		add	cx,07C0h
		jmp	Cnv_End

		align	4
Not_Cnv:	cmp	ch,75h
		je	Ext_Cnv
		cmp	ch,76h
		jne	Cnv_End
Ext_Cnv:	sub	ch,75h
		sub	cl,21h
		mov	al,94
		mul	ch
		xor	ch,ch
		add	ax,cx
		mov	dx,ax
		shr	dx,5 	; dx / 32
		and	ax,1Fh	; ax % 32
		mov	ch,dl	; dx * 256
		mov	cl,al
		add	cx,0900h
Cnv_End:
;******* Bug Fix End ********

		xchg	ch,cl
		mov	ax,cx
		mov	cx,bx
		mov	dx,0FF94h
		out	dx,al
		inc	dx
		mov	al,ah
		out	dx,al

		mov	bl,cl	;cl
		and	bx,7

		test	cl,20h	;cl
		je	$I247
		or	bl,8			;at

$I247:		test	cl,18h	;cl
		je	$I248
		dec	bh			;bk bh=0ffh

$I248:		mov	ax,GVRAM
		mov	es,ax
		mov	al,0Fh
		mov	dx,0FF81h
		out	dx,al
		mov	cx,80-2
		cmp	al,bl
		je	$Sclr_K

		sub	ax,ax

$F251:		REPT	16
		stosw
		add	di,cx
		endm

		sub	di,80*16
		mov	al,bl
		out	dx,al

$Sclr_K:	mov	dx,0FF96h+1
		and	bh,bh
		je	$FF200
		jmp	$F277

		align	4
$FF200:		REPT	16
		dec	dx
		insb
		inc	dx
		insb
		add	di,cx
		endm
		jmp	$FF201

		align	4

$F277:		REPT	16
		dec	dx
		in	al,dx
		xor	al,bh
		stosb
		inc	dx
		in	al,dx
		xor	al,bh
		stosb
		add	di,cx
		endm

$FF201:		pop	bx
		pop	cx
		pop	di
		pop	es
		ret	
_Wrt_Kan	ENDP
;*****************************
; locate
;*****************************
		PUBLIC	_locate
_locate		proc	near
		push	es
		mov	cx,[_CUR_X]
		mov	bx,[_CUR_Y]
		mov	ax,MAX_X
		mul	bx
		add	ax,cx
		shl	ax,1
		add	ax,offset dmy_ram
		mov	di,ax
		xor	byte ptr cs:[di+1],0FFh

		shl	bx,4		; y *= 16
		add	bx,[_CUR_OFF2]
		mov	ax,80

		mul	bx		; y *= 80
		add	ax,cx		; y + x
		mov	di,ax

		mov	ax,GVRAM
		mov	es,ax
		mov	dx,0FF81h
		mov	al,00000001b
		call	loca_sub
		mov	al,01000010b
		call	loca_sub
		mov	al,10000100b
		mov	al,10000100b
		call	loca_sub
		mov	al,11001000b
		call	loca_sub
		pop	es
		ret	

loca_sub:	out	dx,al
		push	di
		mov	cx,[_CUR_SIZE2]

loca1:		not	byte ptr es:[di]
		add	di,80
		loop	loca1
		pop	di
		ret
_locate		ENDP
;***********************************************
;仮想ＶＲＡＭ部（ＤＭＹＣＯＮ　Ｖｅｒ２．０３）
;                             ｂｙ （山）
;***********************************************
		public	_cflush
_cflush		proc 	near
		push	es
		push	ds
		cld
		mov	ax,CVRAM	; cp
		mov	ds,ax
		xor	si,si

		mov	ax,cs		; dp
		mov	es,ax
		mov	di,offset dmy_ram

		xor	cx,cx		; x:cx
		xor	bx,bx		; y:bx
		align	4

cf_lp1:		mov	ax,[si]
		test	ah,40h
		je	short cf_ank

		mov	dx,[si+2000h]
		cmp	dh,es:[di+0]
		jne	short cf_knj
		cmp	dl,es:[di+2]
		jne	short cf_knj
		cmp	ah,es:[di+1]
		je	short cf_ked

cf_knj:		mov	es:[di+1],ah
		mov	es:[di+0],dh
		mov	es:[di+2],dl
		mov	byte ptr es:[di+3],0BFh
		call	_Wrt_Kan
cf_ked:		add	si,4
		add	di,4
		add	cx,2
		jmp	short cf_end
		align	4

cf_ank:		cmp	ax,es:[di]
		stosw
		je	short cf_aed
		call	_Wrt_Ank
cf_aed:		add	si,2
		inc	cx

cf_end:		cmp	cx,MAX_X
		jb	short cf_lp1
		sub	cx,MAX_X
		add	bx,80*LINSIZ
		cmp	bx,MAX_Y2*80*LINSIZ
		jb	short cf_lp1

		pop	ds
		pop	es
		cmp	byte ptr [_CUR_DSP_FLG],0
		jne	short cf_end2
		call	_locate
cf_end2:	ret
_cflush		endp

_TEXT		ENDS
END
