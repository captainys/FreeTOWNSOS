;******************************************
;*
;*	OAK2FEP v1.04
;*
;*	90.07.31	Apend kancnvflg & enc_cnt Bug Fix By Ken
;*
;******************************************
	.286c
_TEXT	SEGMENT  BYTE PUBLIC 'CODE'
_TEXT	ENDS
_DATA	SEGMENT  WORD PUBLIC 'DATA'
_DATA	ENDS
CONST	SEGMENT  WORD PUBLIC 'CONST'
CONST	ENDS
_BSS	SEGMENT  WORD PUBLIC 'BSS'
_BSS	ENDS
DGROUP	GROUP	 _TEXT,	CONST,	_BSS,	_DATA

	ASSUME  CS: DGROUP, DS: DGROUP, ES: DGROUP

_TEXT      SEGMENT

;*******************************************
;*	ORICON40用にアセブルするなら
;*	LINE40を定義すること
;*	ゲェ失敗じゃ
;*	仮想VRAMがC800:1000以上に無い!
;*******************************************
;LINE40	equ	1
;*******************************************

TRUE	equ	1
FALSE	equ	0
;
;キーアドレス
;
sCAP	equ	55h
sHIRA	equ	56h
sKATA	equ	5Ah

sEIJI	equ	6Ah
sEIKO	equ	6Fh
sMUHEN	equ	57h
sHENKAN	equ	58h

sKANJI	equ	59h
sZENHAN	equ	71h

;
;モ－ドフラグ
;
mKANJI	equ	0002h
mROMA	equ	0008h
mZEN	equ	0020h
mKATA	equ	0080h
mHIRA	equ	0100h

;
;  かな漢字変換用構造体定義
;
Funcparm	struc
	wFunc		dw	?
	wMode		dw	?
	lpKkname	dd	far
	lpDataparm	dd	far
	wHandle		dw	?
	Reserved_f	db	2 dup(0)
Funcparm	ends

Kkname		struc
	wLevel		dw	?
	rgchName	db	8 dup(?)
	rgchVersion	db	4 dup(?)
	rgchRevision	db	4 dup(?)
	Reserved_k	db	14 dup(0)
Kkname		ends

Dataparm	struc
	wType		dw	0
	wScan		dw	0
	wAscii		dw	0
	wStatus		dw	0

	cchResult	dw	0
	lpchResult	dd	far

	cchMode		dw	0
	lpchMode	dd	far
	lpattrMode	dd	far

	cchSystem	dw	0
	lpchSystem	dd	far
	lpattrSystem	dd	far

	cchBuf		dw	0
	lpchBuf		dd	far
	lpattrBuf	dd	far
	cchBufCursor	dw	0
	cchBufAct	dw	0

	cchHomonym	dw	0
	lpchHomonym	dd	far
	cchHomonymAct	dw	0
	cchHomonymTotal	dw	0
	cchHomonymNum	dw	0

	fKanji		dw	?
	fNtype		dw	?

	Reserved_d	db	16 dup(0)
Dataparm	ends

;
;
;
MAX_X	equ	80
ifdef	LINE40
  MAX_Y	equ	39
else
  MAX_Y	equ	24
endif
;
;  データ領域
;

fnc	Funcparm	<>
knm	Kkname		<>
dat	Dataparm	<>
buf_size	equ	100
buf		db	8 dup(buf_size dup(0))
vct_90h		dd	far
vct_91h		dd	far
Slock		db	FALSE
modeW		dw	0056h or 8000h
modeB		dd	far
		dd	far
modeS		db	34h,41h, 21h,21h, ' ', 31h,51h, 3Eh,2Eh
modeA		db	1,0,7,0,3,0,7,0		;
		db	1,0,7,0,3,0,7,0
		db	0,0,7,0
		db	1,0,7,0,3,0,7,0
		db	1,0,7,0,3,0,7,0
old_cchS	dw	0
		db	4*buf_size dup(0)
old_cchB	dw	0
		db	4*buf_size dup(0)
cchR_p		dw	0
color		dd	0
bak_xy		dw	0
ent_xy		dw	0
dmy_key		db	0
k_kind		db	?
kancnvflg	db	0
key_tbl		db	16 dup(?)

enc_cnt		dw	0		;bug fixed at v1.00
enc_str		db	7 dup (0)

t1_msg	db	'単語登録　'
	db	'登録位置にカ－ソルを移動して実行キ－を押してください'
t1_msgL	equ	$-t1_msg
t1_atr	db	8 dup (8),t1_msgL-8 dup(1)
t2_msg	db	'単語登録　'
	db	'登録範囲を指定してください'
t2_msgL	equ	$-t2_msg
;
;  かな漢字変換関数
;
KKfunc	proc	near
	push	es
	push	ds
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	mov	ax,cs
	mov	ds,ax
	mov	es,ax

	mov	ax,fnc.wFunc
	mov	bx,offset fnc
	mov	cx,offset fnc
	mov	dx,offset fnc
	mov	di,offset fnc
	mov	si,offset fnc

	int	0EEh
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ds
	pop	es
	ret
KKfunc	endp

KKOpen	proc	near
	mov	fnc.wFunc,2
	mov	fnc.wHandle,1
	mov	ax,ds
	mov	word ptr fnc.lpKkname,offset knm
	mov	word ptr fnc.lpKkname+2,ax
	mov	word ptr fnc.lpDataparm,offset dat
	mov	word ptr fnc.lpDataparm+2,ax
	mov	dat.wType,1	;0にしてもシステムではやってくれない(バカ！)
	call	KKfunc
	ret
KKOpen	endp

KKClose	proc	near
	mov	fnc.wFunc,3
	mov	fnc.wMode,0
	mov	word ptr fnc.lpKkname,0
	mov	word ptr fnc.lpKkname+2,0
	mov	word ptr fnc.lpDataparm,0
	mov	word ptr fnc.lpDataparm+2,0
	call	KKfunc
	ret
KKClose	endp

KKInOut	proc	near
	mov	fnc.wFunc,4
	mov	fnc.wMode,0
	mov	word ptr fnc.lpKkname,0
	mov	word ptr fnc.lpKkname+2,0
	mov	ax,ds
	mov	word ptr fnc.lpDataparm,offset dat
	mov	word ptr fnc.lpDataparm+2,ax

	mov	word ptr dat.lpchResult,offset buf[0]
	mov	word ptr dat.lpchResult+2,ax
	mov	word ptr dat.lpchMode,offset buf[1*buf_size]
	mov	word ptr dat.lpchMode+2,ax
	mov	word ptr dat.lpattrMode,offset buf[2*buf_size]
	mov	word ptr dat.lpattrMode+2,ax
	mov	word ptr dat.lpchSystem,offset buf[3*buf_size]
	mov	word ptr dat.lpchSystem+2,ax
	mov	word ptr dat.lpattrSystem,offset buf[4*buf_size]
	mov	word ptr dat.lpattrSystem+2,ax
	mov	word ptr dat.lpchBuf,offset buf[5*buf_size]
	mov	word ptr dat.lpchBuf+2,ax
	mov	word ptr dat.lpattrBuf,offset buf[6*buf_size]
	mov	word ptr dat.lpattrBuf+2,ax
	mov	word ptr dat.lpchHomonym,offset buf[7*buf_size]
	mov	word ptr dat.lpchHomonym+2,ax
	call	KKfunc
	ret
KKInOut	endp

KKMode	proc	near
	mov	fnc.wFunc,5
	mov	word ptr fnc.lpKkname,0
	mov	word ptr fnc.lpKkname+2,0
	mov	ax,ds
	mov	word ptr fnc.lpDataparm,offset dat
	mov	word ptr fnc.lpDataparm+2,ax
	call	KKfunc
	ret
KKMode	endp

KKTrans	proc	near
	mov	fnc.wFunc,6
	mov	fnc.wMode,0
	mov	word ptr fnc.lpKkname,0
	mov	word ptr fnc.lpKkname+2,0
	mov	ax,ds
	mov	word ptr fnc.lpDataparm,offset dat
	mov	word ptr fnc.lpDataparm+2,ax
	mov	fnc.wHandle,1

	mov	byte ptr dat.wScan,0
	mov	byte ptr dat.wAscii,0
	mov	byte ptr dat.wStatus,0
	mov	word ptr dat.lpchMode,0
	mov	word ptr dat.lpchMode+2,0
	mov	word ptr dat.lpattrMode,0
	mov	word ptr dat.lpattrMode+2,0
	mov	word ptr dat.lpchSystem,offset buf[3*buf_size]
	mov	word ptr dat.lpchSystem+2,ax
	mov	word ptr dat.lpattrSystem,offset buf[4*buf_size]
	mov	word ptr dat.lpattrSystem+2,ax
;	mov	word ptr dat.lpchBuf,0
;	mov	word ptr dat.lpchBuf+2,0
;	mov	word ptr dat.lpattrBuf,0
;	mov	word ptr dat.lpattrBuf+2,0
	mov	word ptr dat.lpchHomonym,0
	mov	word ptr dat.lpchHomonym+2,0
	call	KKfunc
	ret
KKTrans	endp

;
;
;

get_vct90h	macro
		push	es
		push	bx
		mov	ax,3590h
		int	21h
		mov	word ptr vct_90h,bx
		mov	ax,es
		mov	word ptr vct_90h+2,ax
		pop	bx
		pop	es
		endm

set_vct90h	macro
		push	ds
		push	dx
		mov	dx,word ptr vct_90h
		mov	ax,word ptr vct_90h+2
		mov	ds,ax
		mov	ax,2590h
		int	21h
		pop	dx
		pop	ds
		endm

INT90H		macro
		pushf
		call	[vct_90h]
		endm

KYB_check	macro
		mov	ah,07h
		INT90H
		endm

KYB_read	macro	sw
		mov	ah,09h
		mov	al,sw
		INT90H
		endm

KYB_matrix	macro
		mov	di,offset key_tbl
		mov	ah,0Ah
		INT90H
		endm

KYB_KindKb	macro
		mov	ah,0FAh
		int	90h
		endm

KYB_setShift	macro
		mov	ah,0FCh
		INT90H
		endm

KYB_read2_s	proc 	near
kr2s_lp1:	call	KYBN_check
		cmp	dh,0FFh
		je	kr2s_lp1
		mov	ax,0900h
		INT90H
                push    ax
                push    bx
                push	cx
                push    dx
                mov	cl,bh
kr2s_apn31:     mov     ah,07h
                INT90H
                cmp     dh,0ffh
                je      kr2s_apn4

	        cmp     bh,cl
                jne     kr2s_apn4

                cmp     bh,4dh		;bug fixed at v0.99
                je      kr2s_apn32
                cmp     bh,4fh
                je      kr2s_apn32
                cmp     bh,50h
                je      kr2s_apn32
                cmp     bh,51h
                je      kr2s_apn32
		cmp	bh,4Bh
		je	kr2s_apn32
		cmp	bh,4Eh
		je	kr2s_apn32
                cmp     bh,48h
                je      kr2s_apn32
                cmp     bh,72h
                je      kr2s_apn32
                cmp     bh,73h
                jne     kr2s_apn4

kr2s_apn32:     mov     ax,0900h
                INT90H
                jmp     kr2s_apn31
kr2s_apn4:      
                pop     dx
                pop	cx
                pop     bx
                pop     ax
                ret
KYB_read2_s	endp

KYB_read2	macro
		call KYB_read2_s
		endm

key_test	macro	scan
		test	key_tbl[scan/8],(1 shl (scan mod 8))
		endm

key_break	macro	scan
		local	lp
lp:		    KYB_matrix
		    key_test	scan
		    jnz	short lp
		endm

putchar		macro	char
		mov	ah,1Dh
		mov	al,char
		int	91h
		endm

getatr		macro
		mov	di,offset color
		mov	ah,12h
		int	91h
		endm

setatr		macro
		mov	di,offset color
		mov	ah,11h
		int	91h
		endm

getcur		macro
		mov	ah,0Eh
		int	91h
		endm

setcur		macro
		mov	ah,0Dh
		int	91h
		endm

com_atr		macro
	  	mov	di,offset color
	  	mov	ah,10h
		int	91h
		xor	byte ptr[di+1],28h
		mov	ah,0Fh
		int	91h
		endm
;
; sjisto
;  bx:shift jis 漢字コード
;<return> bx:jis 漢字コード
;

sjisto	proc	near
	cmp	bh,9fh
	jbe	sjto1
	sub	bh,0B1h-071h
sjto1:  sub	bh,071h
	shl	bh,1
	inc	bh
	cmp	bl,7fh
	jbe	sjto2
	dec	bl
sjto2:	cmp	bl,9eh
	jb	sjto3
	sub	bl,7dh
	inc	bh
	jmp	sjto5
sjto3:	sub	bl,1fh
sjto5:	ret
sjisto	endp

;
; iskanji
;  bl:shift jis (上位)
;<return> C flug=1:漢字コード
;

iskanji		proc	near
		cmp	bl,81h
		jb 	short ik1_ed
		cmp	bl,0A0h
		jae	short ik1_sk1
		ret
ik1_sk1:	cmp	bl,0E0h
		jb	short ik1_ed
		cmp	bl,0FDh
		jae	short ik1_ed
		ret
ik1_ed:		clc
		ret
iskanji 	endp



putatr		proc	near
		push	di
		mov	al,28h			;attribute
		mov	ah,[di]			;color

		cmp	ah,7			;白=>紫反転
		jne	ps_sk10
		mov	ah,3
		jmp	ps_sk12
ps_sk10:	cmp	ah,8			;黒強調=>緑反転
		jne	short ps_sk11
		mov	ah,4
		jmp	ps_sk12
ps_sk11:	and	ah,07
		cmp	ah,1			;青=>白反転
		jne	short ps_sk12
		mov	ah,7

ps_sk12:	mov	di,offset color
		mov	word ptr[di+1],ax
		mov	ah,11h
		int	91h
		pop	di
		ret
putatr		endp


putstr		proc	near
		push	bp
		push	bx
		push	dx	;+4
		push	si	;+2
		push	di	;+0
		mov	bp,sp

		mov	cx,[di]
		mov	ax,[si]
		sub	cx,ax
		jbe	ps_2
		inc	cx		;bug fixed at v0.99
		inc	cx		;
		inc	cx		;漢字コード対策用
		cmp	al,0		;
		je	ps1_sk0		;
		inc	al		;
ps1_sk0:	add	dl,al
		lea	di,[si+2]
		shl	ax,2
		add	di,ax
ps1_lp1:	cmp	dl,80
		jbe	ps1_sk1
		sub	dl,80
		inc	dh
ps1_sk1:	mov	ah,14h
		int	91h
		mov	ah,10h
		int	91h
ps1_sk11:	mov	[di+3],bl
		mov	[di],bh
		add	di,4
		inc	dl
		loop	ps1_lp1

ps_2:		mov	di,[bp]
		mov	cx,[di]
		or	cx,cx
		jne	ps_sk01
		jmp	ps_sk1

ps_sk01:	mov	di,offset color
		mov	ah,12h
		int	91h
		push	word ptr[color+1]
		mov	di,[bp]

		mov	dx,[bp+4]		;bug fixed at v1.01
		mov	si,word ptr[di+2]
		mov	di,word ptr[di+6]
ps_lp1:
		cmp	dl,80		;bug fixed at v1.01
		jb	ps_sk09		;
		test	dl,01h		;
		jne	ps_sk08		;
		inc	dl		;
ps_sk08:	sub	dl,80		;
		inc	dh		;
ps_sk09:
		call	putatr
		inc	di
		
		mov	bl,[si]			;char:bug fixed at v1.01
		xor	bh,bh
		call	iskanji
		jnc	ps_sk120

		mov	bh,bl
		inc	si
		mov	bl,[si]
		call	sjisto
		push	bx
		mov	bl,bh
		mov	bh,1
		mov	ax,1300h
		int	91h
		pop	bx
		mov	bh,3
		inc	dl
		dec	cx
		call	putatr
		inc	di

ps_sk120:	mov	ax,1300h
		int	91h
		inc	si
		inc	dl
		loop	ps_lp1

		pop	word ptr[color+1]
		mov	di,offset color
		mov	ah,11h
		int	91h

ps_sk1:		mov	di,[bp]
		mov	si,[bp+2]
		mov	ax,[di]
		mov	cx,[si]
		mov	[si],ax
		sub	cx,ax
		jbe	short ps_end
		inc	cx		;bug fixed at v0.99
		inc	cx		;
		mov	dx,[bp+4]
		add	dl,al
		lea	di,[si+2]
		shl	ax,2
		add	di,ax
ps_lp2:		cmp	dl,80
		jbe	ps3_sk1
		sub	dl,80
		inc	dh
ps3_sk1:	mov	bl,[di+3]
		mov	bh,[di]
		mov	ax,1300h
		int	91h
		push	[di+2]
		mov	byte ptr[di+3],0
		mov	ah,0Fh
		int	91h
		pop	[di+2]
		add	di,4
		inc	dl
		loop	ps_lp2

ps_end:		pop	di
		pop	si
		pop	dx
		pop	bx
		pop	bp
		ret
putstr		endp



clrstr		proc	near
		mov	cx,[si]
		or	cx,cx
		je	short cs_end

		inc	cx		;bug fixed at v0.99
		inc	cx		;
		push	bx
		lea	di,[si+2]
cs_lp2:		cmp	dl,80
		jbe	cs3_sk1
		sub	dl,80
		inc	dh
cs3_sk1:	mov	bl,[di+3]
		mov	bh,[di]
		mov	ax,1300h
		int	91h
		mov	byte ptr[di+3],0
		mov	ah,0Fh
		int	91h
		add	di,4
		inc	dl
		loop	cs_lp2
		pop	bx
cs_end:		ret
clrstr		endp


mode_chg	proc	near

		push	di
		push	ax
		push	cx

		test	modeW,01h
		  jz	short krn_edsk0
		    mov		modeW,8055h
		    test	bl,02h
		    jz		short krn_edsk0
		      xor	modeW,0C0h

krn_edsk0:	test	modeW,0008h
		jne	short krn_edsk1
		  test	modeW,180h
		  jz	short krn_edsk1
		  or	bl,02h
		  mov	al,bl
		 jmp	short krn_edsk2

krn_edsk1:	  and	bl,0FDh
		  mov	al,bl
krn_edsk2:	  KYB_setShift

		test	modeW,0002h
		je	short krnc_sk11
		  mov	word ptr modeS[0],4134h		;'漢'
		 jmp	short krnc_sk2
krnc_sk11:	  mov	word ptr modeS[0],2121h		;'  '
krnc_sk2:	test	modeW,0008h
		je	short krnc_sk21
		  mov	modeS[4],'R'
		 jmp	short krnc_sk3
krnc_sk21:	  mov	modeS[4],' '
krnc_sk3:	test	modeW,0020h
		je	short krnc_sk31
		  mov	word ptr modeS[2],3441h		;'全'
		 jmp	short krnc_sk4
krnc_sk31:	  mov	word ptr modeS[2],2121h		;'  '
krnc_sk4:	test	modeW,0040h
		je	short krnc_sk42
		  mov	word ptr modeS[5],5131h		;'英'
		  test	bl,01h
		  je	short krnc_sk411
		    mov	word ptr modeS[7],6742h		;'大'
		   jmp	short krnc_sk5
krnc_sk411:	    mov	word ptr modeS[7],2E3Eh		;'小'
		   jmp	short krnc_sk5
krnc_sk42:	test	modeW,0080h
		je	short krnc_sk43
		  mov	word ptr modeS[5],2B25h		;'カナ'
		  mov	word ptr modeS[7],4A25h
		 jmp	short krnc_sk5
krnc_sk43:	  mov	word ptr modeS[5],2B24h		;'かな'
		  mov	word ptr modeS[7],4A24h

krnc_sk5:	mov	cx,9
		mov	dl,72+80h
		mov	di,offset modeB
		mov	ax,1F01h
		int	91h
		mov	ax,modeW
		mov	fnc.wMode,ax
		call	KKMode

		pop	cx
		pop	ax
		pop	di
		ret
mode_chg	endp

;
;
;

KYBN_check	proc	near
		KYB_matrix
		KYB_check
		cmp	dh,0FFh
		je	krn_sk1

		cmp	dh,80h		;PF key?
		jne	kcn_sk1

;
; For oyayubi shift
;
		cmp	bh,57h		;
		je	kcn_sk0
		cmp	bh,58h		;
		jne	kcn_sk01
kcn_sk0:	cmp	k_kind,1	;JIS?
		je	kcn_sk01
 	     	and	modeW,803Fh
 	     	or	modeW,100h
		push	dx		; Bug Fix By Ken
		push	bx
		call	mode_chg
		pop	bx
		pop	dx
		jmp	kcn_sk2		; Bug Fix at v1.03
kcn_sk01:	jmp	kcn_sk3


kcn_sk1:	test	modeW,0020h	;全角?
		jne	kcn_sk2
		test	modeW,0040h	;英数入力モード?
		jne	kcn_sk3
		test	modeW,0108h	;ローマ字入力モード or ひらがな?
		jne	kcn_sk2
		jmp	kcn_sk3		;かな入力モード and かたかな.

kcn_sk2:	mov	ah,0FFh		;変換の必要有り
		ret
kcn_sk3:	mov	ah,0		;変換の必要なし
		ret


;
; For JIS keybord
;
krn_sk1:	  key_test	sCAP
		  jz	short krn_sk2
		    key_break	sCAP
		   jmp	krn_end1

krn_sk2:	  key_test	sHIRA
		  jz	short krn_sk3
		    test	bl,10h		;CTRL
		    jz	short krn_sk22
		      test	Slock,TRUE
		      jnz	krn_sk21
		      xor	modeW,0Ch
krn_sk21:	      key_break	sHIRA
		     jmp	krn_end1
krn_sk22:	      test	Slock,TRUE
		      jnz	krn_sk23
	 	     xor	modeW,140h
krn_sk23:	      test	modeW,100h
		      je	short krn_sk21
		       and	modeW,813Fh
		     jmp	short krn_sk21
krn_sk3:	  key_test	sKATA
		  jz	short krn_sk4
		      test	Slock,TRUE
		      jnz	krn_sk32
		    xor		modeW,80h
		    test	modeW,80h
		    je		short krn_sk31
		      and	modeW,80BFh
		     jmp	short krn_sk32
krn_sk31:	      and	modeW,803Fh
		      or	modeW,40h
krn_sk32:	      key_break	sKATA
		      jmp	krn_end1

;
; For oyayubi shift keybord
;
krn_sk4:
		  key_test	sEIJI
		  jz	short krn_sk5
		    test	bl,10h		;CTRL
		    jz	short krn_sk42
		      test	Slock,TRUE
		      jnz	krn_sk41
		      xor	modeW,0Ch
krn_sk41:	      key_break	sEIJI
		     jmp	krn_end1
krn_sk42:	      test	Slock,TRUE
		      jnz	krn_sk41
	 	     and	modeW,803Fh
	 	     or		modeW,040h
		     jmp	short krn_sk41

krn_sk5:	  key_test	sEIKO
		  jz	short krn_sk6
		    test	Slock,TRUE
		    jnz		krn_sk52
		    test	modeW,040h
		    jne		krn_sk52
	 	    and		modeW,803Fh
	 	    or		modeW,080h
krn_sk52:	    key_break	sKATA
		    jmp	short krn_end1

;
;
;
krn_sk6:	  key_test	sZENHAN
		  jz	krn_sk7
		      test	Slock,TRUE
		      jnz	krn_sk61
		    xor	modeW,30h
krn_sk61:	      key_break	sZENHAN
		    jmp	krn_end1
krn_sk7:	  key_test	sKANJI
		  jz	short krn_end2
		      test	Slock,TRUE
		      jnz	krn_sk71
		    xor		modeW,03h
krn_sk71:	    key_break	sKANJI

krn_end1:	call	mode_chg
		mov	dx,0FFFFh
		mov	bh,0FFh
krn_end2:	ret
KYBN_check	endp



enter_word	proc	near

		test	modeW,01h
		jz	short ew_ent
		jmp	ew_end3

ew_ent:		push	bp
		push	dx
		push	bx
		push	ax

		mov	Slock,TRUE
	mov	ax,ds
	mov	byte ptr dat.wType,1
	mov	word ptr dat.lpchResult,offset buf[0]
	mov	word ptr dat.lpchResult+2,ax
	mov	dat.cchResult,0

		getcur
		push	dx
		mov	bp,sp
		mov	bak_xy,dx

		mov	dx,1901h
		mov	di,offset dat.cchSystem
		mov	word ptr[di],t1_msgL
		mov	word ptr[di+2],offset t1_msg
		mov	word ptr[di+6],offset t1_atr
		mov	si,offset old_cchS
		call	putstr

ew_lp1:		mov	dx,bak_xy
		setcur
		KYB_read2
		mov	dx,bak_xy
		cmp	bh,4Fh		;left
		jne	ew_sk1
		  dec	dl
		  cmp	dl,1
		  jae	ew_sk01
		    mov	dl,MAX_X
ew_sk01:	  mov	ah,14h
		  int	91h
		  cmp	bh,3
		  jne	ew_sk51
		    dec	dl
		  jmp	ew_sk51
ew_sk1:		cmp	bh,51h		;right
		jne	ew_sk2
		  inc	dl
		  cmp	dl,MAX_X
		  jbe	ew_sk5
		    mov	dl,1
		  jmp	ew_sk5
ew_sk2:		cmp	bh,4Dh		;up
		jne	ew_sk3
		  dec	dh
		  cmp	dh,1
		  jae	ew_sk5
		    mov	dh,MAX_Y
		  jmp	ew_sk5
ew_sk3:		cmp	bh,50h		;down
		jne	ew_sk4
		  inc	dh
		  cmp	dh,MAX_Y
		  jbe	ew_sk5
		    mov	dh,1
		  jmp	ew_sk5
ew_sk4:		cmp	bh,72h		;[取消]
		jne	ew_sk41
		jmp	ew_end
ew_sk41:	cmp	bh,73h		;[実行]
		je	ew_next1
ew_sk5:		mov	ah,14h
		int	91h
		cmp	bh,3
		jne	ew_sk51
		  inc	dl
ew_sk51:	mov	bak_xy,dx
		jmp	ew_lp1

ew_next1:	mov	dx,bak_xy
		mov	ent_xy,dx

		mov	dx,1901h
		mov	di,offset dat.cchSystem
		mov	word ptr[di],t2_msgL
		mov	word ptr[di+2],offset t2_msg
		mov	word ptr[di+6],offset t1_atr
		mov	si,offset old_cchS
		call	putstr

		xor	cx,cx
ew_lp2:		mov	dx,bak_xy
		setcur
		KYB_read2
		mov	dx,bak_xy
		  push	bx
		  mov	di,word ptr dat.lpchResult
		  add	di,cx
		  mov	ah,14h
		  int	91h
		  test	bh,1
		  jne	ew_sk73
		  mov	[di],bl
		  jmp	ew_sk6001

ew_sk73:	  push	dx
		  push	bx
		  inc	dl
		  mov	ah,14h
		  int	91h
		  pop	dx
		  mov	dh,bl
		  mov	ah,03
		  int	0AFh
		  xchg	dh,dl
		  mov	[di],dx
		  pop	dx
ew_sk6001:	  pop	bx

		cmp	bh,4Fh		;left
		jne	ew_sk7
		  dec	dl
		  dec	cx
		  cmp	cx,0
		  jns	ew_sk601
		  inc	dl
		  inc	cx
		  jmp	ew_sk9
ew_sk601:	  cmp	dl,1
		  jae	ew_sk61
		    mov	dl,MAX_X
		    dec	dh
ew_sk61:	  com_atr
		  cmp	byte ptr[di],3
		  jne	ew_sk62
		    dec	dl
		    dec	cx
		    com_atr
ew_sk62:	  jmp	ew_sk9
ew_sk7:		cmp	bh,51h		;right
		jne	ew_sk8
		  com_atr
		  inc	dl
		  inc	cx
		  cmp	cx,buf_size
		  jbe	ew_sk701
		  dec	dl
		  dec	cx
		  com_atr
		  jmp	ew_sk9
ew_sk701:	  cmp	dl,MAX_X
		  jbe	ew_sk71
		    mov	dl,1
		    inc	dh
ew_sk71:  	  mov	di,offset color
	  	  mov	ah,10h
		  int	91h
		  cmp	byte ptr[di],3
		  jne	ew_sk9
		    xor	byte ptr[di+1],28h
		    mov	ah,0Fh
		    int	91h
		    inc	dl
		    inc	cx
		  jmp	ew_sk9
ew_sk8:		cmp	bh,72h		;[取消]
		jne	ew_sk81
		jmp	ew_end
ew_sk81:	cmp	bh,73h		;[実行]
		je	ew_next2
ew_sk9:		mov	bak_xy,dx
		jmp	ew_lp2

ew_next2:	mov	dat.cchResult,cx
		mov	dx,ent_xy
		setcur
ew_lp3:		com_atr
		inc	dl
		cmp	dl,MAX_X
		jbe	ew_lp31
		  mov	dl,1
		  inc	dh
ew_lp31:	loop	ew_lp3

		cmp	dat.cchResult,0
		je	ew_end
		call	KKTrans

ew_lp4:		mov	dx,1901h
		mov	di,offset dat.cchSystem
		mov	si,offset old_cchS
		call	putstr
		mov	dx,ent_xy
		mov	di,offset dat.cchBuf
		mov	si,offset old_cchB
		call	putstr
		mov	dx,[bp]
		setcur

		KYB_read2
		mov	byte ptr dat.wType,0
		mov	byte ptr dat.wScan,bh
		mov	byte ptr dat.wAscii,dl
		mov	byte ptr dat.wStatus,bl
		call	KKInOut
		cmp	dat.cchSystem,0
		jne	ew_lp4

ew_end:		mov	dx,1901h
		mov	si,offset old_cchS
		call	clrstr
		mov	old_cchS,0
		mov	dx,ent_xy
		mov	si,offset old_cchB
		call	clrstr
		mov	old_cchB,0
	mov	dat.cchSystem,0
	mov	word ptr dat.lpchSystem,offset buf[3*buf_size]
	mov	word ptr dat.lpchSystem+2,ax
	mov	word ptr dat.lpattrSystem,offset buf[4*buf_size]
	mov	word ptr dat.lpattrSystem+2,ax

		pop	dx
		setcur
		mov	Slock,FALSE
		pop	ax
		pop	bx
		pop	dx
		pop	bp
ew_end3:	ret
enter_word	endp

;
;
;
key_bios	proc	far
		cmp	ah,09h		;key_read
		je	b09_n
		cmp	ah,07h		;key_check
		je	b07
		jmp	cs:[vct_90h]

b09_n:		jmp	b09

b07:		push	bp	;+16
		push	es	;+14
		push	ds	;+12
		push	si	;+10
		push	di	;+8
		push	dx	;+6
		push	cx	;+4
		push	bx	;+2
		push	ax	;+0
		mov	bp,sp
		mov	ax,cs
		mov	ds,ax
		mov	es,ax
		sti

		cmp	dat.cchResult,0
		je	b07_sk11
		mov	cx,cchR_p
		cmp	cx,dat.cchResult
		jae	short b07_sk1
		mov	si,word ptr dat.lpchResult
		add	si,cchR_p
		xor	dh,dh
		mov	dl,[si]
		mov	bh,byte ptr dat.wScan	;bug fixed at v1.01
		cmp	enc_cnt,0
		je	v101_fx1
		xor	bh,bh
v101_fx1:	mov	bl,byte ptr dat.wStatus
		mov	al,byte ptr dat.cchResult
		sub	al,byte ptr cchR_p
		jmp	b07_sk3

b07_sk1:	mov	cchR_p,0
		mov	dat.cchResult,0
		mov	al,0
		mov	dx,0FFFFh
		mov	bx,0FF00h
		jmp	b07_sk3

b07_sk11:	call	KYBN_check
		cmp	word ptr dat.cchBuf,0	;未確定文字列があるか？
		jne	b07_sk2
		cmp	dh,0FFh
		je	b07_sk3
;		cmp	dx,8015h	;単語登録?
		cmp	bh,6Dh		;単語登録? (Bug Fix By Ken)
		jne	b07_sk12
		mov	ax,0900h
		INT90H
		call	enter_word
		mov	al,0
		mov	dx,0FFFFh
		mov	bx,0FF00h
		jmp	b07_sk3
b07_sk12:	cmp	ah,0FFh		;変換の必要ありか？
		jne	b07_sk3		;無い。

b07_sk2:	mov	al,1
		mov	bx,0
		mov	dx,0

b07_sk3:	mov	[bp],al
		mov	[bp+2],bx
		mov	[bp+6],dx
		jmp	kr_end23

b09:		push	bp	;+16
		push	es	;+14
		push	ds	;+12
		push	si	;+10
		push	di	;+8
		push	dx	;+6
		push	cx	;+4
		push	bx	;+2
		push	ax	;+0
		mov	bp,sp
		mov	ax,cs
		mov	ds,ax
		mov	es,ax
		sti

		mov	cx,cchR_p
		cmp	cx,dat.cchResult
		jae	short kr_sk1
		mov	si,word ptr dat.lpchResult
		add	si,cx
		xor	dh,dh
		mov	dl,[si]
		mov	bh,byte ptr dat.wScan	;bug fixed at v1.01
		cmp	enc_cnt,0
		je	v101_fx2
		xor	bh,bh
		dec	enc_cnt
v101_fx2:	mov	bl,byte ptr dat.wStatus
		mov	[bp+2],bx
		mov	[bp+6],dx
		inc	cchR_p
		jmp	kr_end23

kr_sk1:		cmp	kancnvflg,0		; Apend OAKLIB by Ken
		je	ken_fix1
		mov	kancnvflg,0

		cmp	byte ptr[bp],1		;No wait key in?
		jne	ken_fix1

		mov	bh,0FFH
		mov	bl,byte ptr dat.wStatus
		mov	dx,0FFFFh
		jmp	ken_fix2		; Return to Null Char

ken_fix1:	mov	cchR_p,0
		mov	dat.cchResult,0

		getatr
		getcur
		mov	bak_xy,dx

		cmp	word ptr dat.cchSystem,0
		je	kr_sk101	;bug fixed at v1.00
		mov	dx,1901h
		mov	di,offset dat.cchSystem
		mov	si,offset old_cchS
		call	putstr

kr_sk101:	cmp	word ptr dat.cchBuf,0	;bug fixed at v1.01
		je	short kr_sk11
		mov	di,offset dat.cchBuf
		mov	si,offset old_cchB
		call	putstr

		mov	dx,bak_xy
		add	dl,byte ptr dat.cchBufCursor
		cmp	dl,80		;bug fixed at v1.01
		jb	kr_sk11		;
		test	dl,01h		;
		jne	kr_sk102	;
		inc	dl		;
kr_sk102:	sub	dl,80		;
		inc	dh		;
kr_sk11:	setcur

kr_lp1:		call	KYBN_check
		cmp	dh,0FFh			;No key in?
		jne	short kr_sk2		;No.
		cmp	byte ptr[bp],1		;No wait key in?
		jne	short kr_lp1		;No.
kr_lp11:	cmp	word ptr dat.cchBuf,0
		jne	short kr_lp1

kr_lp12:
		cmp	bh,57h			;変換,無変換を返さない
		je	kr_lp1			;ようにした  by Ken
		cmp	bh,58h
		je	kr_lp1
ken_fix2:	mov	[bp+2],bx
		mov	[bp+6],dx
		jmp	kr_end23

kr_sk2:		push	ax
		mov	ax,0900h		;dummy read
		INT90H
		pop	ax
		cmp	word ptr dat.cchBuf,0	;未確定文字列があるか？
		jne	kr_sk22
;		cmp	dx,8015h	;単語登録?
		cmp	bh,6Dh		;単語登録? (Bug Fix By Ken)
		jne	kr_sk21
		call	enter_word
		mov	dx,0FFFFh
		mov	bx,0FF00h
		jmp	kr_lp1
kr_sk21:	cmp	ah,0FFh		;変換の必要ありか？
		jne	kr_lp12		;無。

kr_sk22:	mov	byte ptr dat.wType,0
		mov	byte ptr dat.wScan,bh
		mov	byte ptr dat.wAscii,dl
		mov	byte ptr dat.wStatus,bl

		mov	si,offset enc_str	;bug fixed at v1.00
		mov	enc_cnt,1
		mov	[si],dl
		inc	si

;Bug Fixed by K.MIYAZAKI
;   Thanks a lot!!
;
                mov     ah,1eh
                cmp     bh,4dh
                je      ken_apn2
                mov     ah,1dh
                cmp     bh,4fh
                je      ken_apn2
                mov     ah,1fh
                cmp     bh,50h
                je      ken_apn2
                mov     ah,1ch
                cmp     bh,51h
                je      ken_apn2

		mov	ah,7Fh
		cmp	bh,4Bh
		je	ken_apn2
		mov	ah,0Bh
		cmp	bh,4Eh
		je	ken_apn2

                mov     ah,1bh
                cmp     bh,48h
                je      ken_apn2
                cmp     bh,72h
                je      ken_apn2
                cmp     bh,73h
                je      ken_apn2	;bug fixed at v1.04a

		cmp	dh,80h		;bug fixed at v1.04a
		jne	ken_apn5	;
		mov	ah,1Eh		;
		cmp	bh,58h		;
		je	ken_apn2	;
		mov	ah,1Fh		;
		cmp	bh,57h		;
		jne	ken_apn5	;


ken_apn2:
	        cmp     old_cchS,0
                jne     ken_apn3
                cmp     old_cchB,0
                jne     ken_apn3
                jmp     kr_lp12
ken_apn3:       
                mov     byte ptr dat.wAscii,ah

                push    ax
                push    bx
                push    dx
ken_apn31:      mov     ah,07h
                INT90H
                cmp     dh,0ffh
                je      ken_apn4
                cmp     bh,byte ptr dat.wScan
                jne     ken_apn4
                mov     ax,0900h
                INT90H
		mov	[si],dl		;bug fixed at v1.00
		inc	si
		inc	enc_cnt
		jmp	ken_apn31

ken_apn4:       
                pop     dx
                pop     bx
                pop     ax
ken_apn5:       
;
;
;

		call	KKInOut

		cmp	[dat.cchResult],0	;No result string?
		jne	kr_sk3			;No.

		mov	dx,1901h
		mov	di,offset dat.cchSystem
		mov	si,offset old_cchS
		call	putstr

		mov	dx,bak_xy
		mov	di,offset dat.cchBuf
		mov	si,offset old_cchB
		call	putstr

		mov	dx,bak_xy
		add	dl,byte ptr dat.cchBufCursor
		cmp	dl,80		;bug fixed at v1.01
		jb	kr_sk23		;
		test	dl,01h		;
		jne	kr_sk230	;
		inc	dl		;
kr_sk230:	sub	dl,80		;
		inc	dh		;
kr_sk23:	setcur

		mov	kancnvflg,1		; Apend OAKLIB by Ken

		jmp	kr_lp1

kr_sk3:
;		cmp	dat.cchBuf,0		;bug fixed at v1.00
;		jne	kr_sk301

		mov	di,word ptr[dat.lpchResult]
		add	di,dat.cchResult
		dec	di
		mov	si,offset enc_str
		mov	cx,enc_cnt

		mov	al,byte ptr dat.wAscii	;キーコードの復元が必要か？
		cmp	al,[di]
		je	kr_sk309	; Do Cnvert
		dec	enc_cnt		; エンコ－ドの禁止 by Ken
		jmp	kr_sk301

kr_sk309:	repnz	movsb
		mov	cx,dat.cchResult
		add	cx,enc_cnt
		dec	cx
		mov	dat.cchResult,cx

kr_sk301:	mov	si,word ptr[dat.lpchResult]
		cmp	dl,[si]
		je	short kr_sk31

kr_sk4:		mov	dl,[si]		;bug fixed at v1.01
		xor	dh,dh
		xor	bh,bh

kr_sk31:	mov	[bp+2],bx
		mov	[bp+6],dx
		jmp	kr_end1

		cmp	old_cchS,0
		je	kr_end1
		mov	dx,1901h
		mov	si,offset old_cchS
		call	clrstr
		mov	old_cchS,0
kr_end1:	cmp	old_cchB,0
		je	kr_end12
		mov	dx,bak_xy
		mov	si,offset old_cchB
		call	clrstr
		mov	old_cchB,0
kr_end12:	mov	dx,bak_xy
		setcur

kr_end2:	mov	cchR_p,1
		mov	ax,dat.cchResult	;bug fixed at v1.01
		sub	ax,enc_cnt
		dec	ax
		jns	kr_end20
		xor	ax,ax
kr_end20:	mov	enc_cnt,ax
kr_end23:	pop	ax
		pop	bx
		pop	cx
		pop	dx
		pop	di
		pop	si
		pop	ds
		pop	es
		pop	bp
		xor	ah,ah
		iret
key_bios	endp

;
;
;
		public	oak_set
oak_set		proc	near
		mov	ax,cs
		mov	ds,ax
		mov	es,ax
		mov	word ptr modeB[0],offset modeS	;System line
		mov	word ptr modeB[2],ax
		mov	word ptr modeB[4],offset modeA
		mov	word ptr modeB[6],ax
		mov	ah,0
		int	91h

		KYB_KindKb
		mov	k_kind,al

		call	KKOpen			;oak/2 open

		mov	ax,0F900h		; key bios status dsp off
		int	90h

		mov	cx,9			;print mode
		mov	dl,72+80h
		mov	di,offset modeB
		mov	ax,1F01h
		int	91h

		mov	ah,07h			;set shift mode
		int	90h
		mov	ax,modeW
		mov	fnc.wMode,ax
		call	KKMode
		mov	byte ptr dat.wType,0		;リフレッシュ
		mov	byte ptr dat.wScan,0
		mov	byte ptr dat.wAscii,0
		mov	byte ptr dat.wStatus,0
		call	KKInOut
		mov	dat.cchResult,0
		mov	dat.cchSystem,0
		mov	dat.cchBuf,0
		mov	old_cchS,0
		mov	old_cchB,0
		mov	cchR_p,0

		get_vct90h
		mov	dx,offset key_bios
		mov	ax,2590h
		int	21h

		ret
oak_set		endp

		public	oak_reset
oak_reset	proc	near
		set_vct90h
		call	KKClose
		mov	ax,0F901h		;ステータス表示on
		int	90h
		ret
oak_reset	endp

_TEXT	ENDS
		end
