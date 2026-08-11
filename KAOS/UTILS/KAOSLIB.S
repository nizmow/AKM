* Bibliotheksmodul zur Erg„nzung von TCGEMLIB.LIB, das die AES- Funktionen
* von GEM/3 und KAOS enth„lt


	XDEF		appl_yield			; GEM 2.x
	XDEF		_appl_yield
	XDEF		vq_aes
	XDEF		appl_bvset			; GEM 2.x
	XDEF		shel_rdef				; GEM 2.x
	XDEF		shel_wdef				; GEM 2.x
	XDEF		scrp_clear			; GEM 2.x
	XDEF		graf_xhandle			; KAOS 1.4
	XDEF		menu_unregister		; GEM 2.x
	XDEF		menu_click			; GEM/3
	XDEF		xgrf_stepcalc			; GEM 2.x
	XDEF		xgrf_2box				; GEM 2.x

	XREF		_GemParB

	OFFSET

CONTRL:	DS.W	15
GLOBAL:	DS.W	80
INTIN:	DS.W	128
INTOUT:	DS.W	45
PTSOUT:	DS.W	128
ADDRIN:	DS.L	128
ADDROUT:	DS.L	6
PTSIN:	DS.W	128

	   TEXT

**********************************************************************
*
* AES Handler
*

__aes:
 lea 	_GemParB+CONTRL,a0
 clr.l	(a0)
 clr.l	4(a0)
 movep	d0,5(a0)
 swap	d0
 movep	d0,1(a0)
 move.w	#$c8,d0
 move.l	#aespb,d1
 trap	#2
 lea 	_GemParB+INTOUT,a0
 move.w	(a0)+,d0
 rts

aespb:
 DC.L	_GemParB+CONTRL
 DC.L	_GemParB+GLOBAL
 DC.L	_GemParB+INTIN
 DC.L	_GemParB+INTOUT
 DC.L	_GemParB+ADDRIN
 DC.L	_GemParB+ADDROUT


**********************************************************************
*
* int appl_yield (void )
*

appl_yield:
 move.l	#$11000100,d0
 bra 	__aes


**********************************************************************
*
* int _appl_yield (void )
*

_appl_yield:
 move.w	#$c9,d0
 trap	#2
 rts


*********************************************************************
*
* int vq_aes ( void )
*

vq_aes:
 move.w	#$c9,d0
 trap	#2
 cmpi.w	#$c9,d0
 sne		d0
 ext.w	d0
 rts


**********************************************************************
*
* int appl_bvset( d0 = int dsk, d1 = int hrd )
*

appl_bvset:
 lea 	_GemParB+INTIN,a0
 move.w	d0,(a0)+
 move.w	d1,(a0)
 move.l	#$10020100,d0
 bra 	__aes


**********************************************************************
*
* int shel_rdef( a0 = char *fname, a1 = char *dir )
*

shel_rdef:
 move.l	#$7e000102,d0
__shel_xdef:
 move.l	a0,_GemParB+ADDRIN
 move.l	a1,_GemParB+ADDRIN+4
 bra 	__aes


**********************************************************************
*
* int shel_wdef( a0 = char *fname, a1 = char *dir )
*

shel_wdef:
 move.l	#$7f000002,d0
 bra 	__shel_xdef


**********************************************************************
*
* int scrp_clear( void )
*

scrp_clear:
 move.l	#$52000100,d0
 bra 	__aes


**********************************************************************
*
* int menu_unregister ( d0 = int menu_id )
*

menu_unregister:
 move.w	d0,_GemParB+INTIN
 move.l	#$24010100,d0
 bra 	__aes


**********************************************************************
*
* int menu_click( d0 = int val, d1 = int setit )
*

menu_click:
 lea		_GemParB+INTIN,a0
 move.w	d0,(a0)+
 move.w	d1,(a0)
 move.l	#$25020100,d0
 bra		__aes


**********************************************************************
*
* int xgrf_stepcalc( d0 = int orgw, d1 = int orgh,
*				 d2 = int x, int y, int w, int h,
*				 a0 = int *cx, a1 = int *cy,
*				 int	*cnt, int *xstep, int *ystep )
*

xgrf_stepcalc:
 move.l	a1,-(sp)
 move.l	a0,-(sp)
 lea 	_GemParB+INTIN,a0
 move.w	d0,(a0)+
 move.w	d1,(a0)+
 move.w	d2,(a0)+
 move.l	$c(sp),(a0)+
 move.w	$10(sp),(a0)
 move.l	#$82060600,d0
 bsr 	__aes
 movea.l	(sp)+,a1
 move.w	(a0)+,(a1)			; *cx = intout[1]
 movea.l	(sp)+,a1
 move.w	(a0)+,(a1)			; *cy = intout[2]
 movea.l	$a(sp),a1
 move.w	(a0)+,(a1)			; *cnt = intout[3]
 movea.l	$e(sp),a1
 move.w	(a0)+,(a1)			; *xstep = intout[4]
 movea.l	$12(sp),a1
 move.w	(a0),(a1)				; *ystep = intout[5]
 rts


**********************************************************************
*
* int xgrf_2box( d0 = int startx, d1 = int starty,
*			  d2 = int startw, int starth,
*			  int corners, int cnt, int xstep, int ystep, int doubled )
*

xgrf_2box:
 lea 	_GemParB+INTIN,a0
 lea		8(sp),a1
 move.l	(a1)+,(a0)+			; intin[0] = cnt
							; intin[1] = xstep
 move.w	(a1)+,(a0)+			; intin[2] = ystep
 move.w	6(sp),(a0)+			; intin[3] = corners
 move.w	(a1),(a0)+			; intin[4] = doubled
 move.w	d0,(a0)+				; intin[5] = startx
 move.w	d1,(a0)+				; intin[6] = starty
 move.w	d2,(a0)+				; intin[7] = startw
 move.w	4(sp),(a0)			; intin[8] = starth
 move.l	#$83090100,d0
 bra 	__aes


	   END

