*
*
* Startmodul fÅr KAOSDESK/MAGIXDESK
*
*

	   XREF  main,_StkSize,memcpy,memset


	   XDEF  _BasPag,exit,_exit,_iconfile,errno,_StkLim


	   TEXT

 move.l	4(sp),a3				; Basepage
 move.l	_iconfile_len,d7
 bne.b	_st_no_icons			; Icons schon geladen!
 move.l	$10(a3),a5			; Beginn des Daten-Segments
 add.l	$14(a3),a5			; Ende des Daten-Segments
 cmpi.l	#'ICNS',-8(a5)
 bne.b	_st_no_icons
 cmpi.l	#'suec',-4(a5)
 bne.b	_st_no_icons
 move.l	-12(a5),d7			; LÑnge der Icondatei mit Magic
 suba.l	d7,a5				; a5 = Beginn der Icondatei
 move.l	$18(a3),a0			; Beginn BSS
 add.l	$1c(a3),a0			; Ende BSS
 suba.l	d7,a0
 move.l	a0,_iconfile
 move.l	a0,sp
 move.l	d7,d0
 move.l	a5,a1
;move.l	a0,a0
 jsr		memcpy
 move.l	d7,d0				; LÑnge Icons
 move.l	$1c(a3),d1			; LÑnge BSS
 cmp.l	d7,d1
 bls.b	_st_clr
 move.l	d7,d1
_st_clr:
 moveq	#0,d0				; Value
 move.l	a5,a0				; s
 jsr		memset
 move.l	d7,_iconfile_len

_st_no_icons:
 move.l	a3,_BasPag
 movea.l	$c(a3),a0				; LÑnge TEXT
 adda.l	$14(a3),a0			; LÑnge DATA
 adda.l	$1c(a3),a0			; LÑnge BSS
 adda.w	#$100,a0				; LÑnge PD
 move.l	a3,d0
 add.l	a0,d0
 and.b	#$fc,d0
 move.l	d0,d1
 sub.l	d7,d1				; ggf. Icondatei
 movea.l	d1,sp				; sp aufs Ende des BSS
 sub.l	#_StkSize-$100,d0
 move.l	d0,_StkLim
 move.l	a0,-(sp)
 move.l	a3,-(sp)
 clr.w	-(sp)
 move.w	#$4a,-(sp)
 trap	#1				; gemdos Mshrink
 lea 	$c(sp),sp
 jsr 	main
exit:
 move.w	d0,-(sp)
_exit:
 move.w	#$4c,-(sp)
 trap	#1				; gemdos Pterm

	   DATA

errno:
 DC.W	0
_iconfile:
 DC.L	0
_iconfile_len:
 DC.L	0

	   BSS
_BasPag:
 DS.L	1
_StkLim:
 DS.L	1

	   END

