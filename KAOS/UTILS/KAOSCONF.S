**********************************************************************
*
* KAOSCONF
*
* erstellt		22.08.90
* letzte énderung	23.11.91
*
**********************************************************************

SCRP		EQU 0

_base		EQU	*-$100
NUM_OBS		EQU	18
FIRSTSTR		EQU	6
FIRSTBUT		EQU	12
OB_ABBRUCH	EQU	5
OB_SICHERN	EQU	4


	INCLUDE "OSBIND.INC"
	INCLUDE "AESBIND.INC"

 bra.b	init

default_value:
 DC.L	-1					; Defaultwert fÅr config

* Initialisierung
* ===============

init:
 lea 	_base+256(pc),sp		; eigener Stack (128 Bytes)
 lea 	d(pc),a6				; BSS- Bereich
 lea 	_base(pc),a5
 pea 	get_sysvars(pc)
 xbios	Supexec
 addq.l	#6,sp
 bmi.b	ver_err				; Version < 1.4
 move.l	default_value(pc),-(sp)	; Defaultdaten
 bmi.b	init_read 			; ungÅltig: lesen
 tst.l	p_parent(a5)
 beq.b	init_write			; bei ACC immer Status schreiben
* PRG
 move.w	#$c9,d0
 trap	#2
 tst.w	d0					; AES initialisiert ?
 beq.b	init_read 			; ja, Status lesen
init_write:
 move.w	#'EL',-(sp)
 bra.b	init_sc
init_read:
 move.w	#'AK',-(sp)
init_sc:
 gemdos	Sconfig
 addq.w	#8,sp
ver_err:
 tst.l	d0
 sge.b	is_kaos(a6)
 lea 	_base(pc),a1			; Basepage holen
 tst.l	p_parent(a1)
 seq.b	is_acc(a6)			; merken, wenn .ACC
 beq.b	init_acc

* 1. Fall: als .PRG gestartet

 lea 	pgmname+9(pc),a0
 move.b	#'p',(a0)+
 move.b	#'r',(a0)+
 move.b	#'g',(a0) 			; 'PRG' statt 'ACC'
 movea.w	#$100,a0				; ProgrammlÑnge + $100
 adda.l	$c(a1),a0
 adda.l	$14(a1),a0
 adda.l	$1c(a1),a0
 move.l	a0,-(sp)
 move.l	a1,-(sp)
 clr.w	-(sp)
 gemdos	Mshrink
 adda.w	#$c,sp
 tst.l	d0					; KAOS liefert Fehlermeldung bei Mshrink()
 bmi 	exit
 tst.b	is_kaos(a6)
 beq 	exit
 M_appl_init
 bmi 	exit
 bsr 	rsc_init
 bsr 	do_dialog 			; Dialog ausfÅhren
 M_appl_exit
exit:
 gemdos	Pterm0

* 2. Fall: Als .ACC gestartet

init_acc:

 M_appl_init
 bmi 	exit
 move.w	d0,intin(a6)			; schon fÅr menu_register
	IFNE	SCRP
 move.l	#scrpdir,addrin(a6)
 M_scrp_write
	ENDIF
 tst.b	is_kaos(a6)
 beq.b	again				; kein KAOS: nicht anmelden
 move.l	#appname,addrin(a6)
 M_menu_register
 bmi 	exit
 move.w	d0,menu_id(a6)
 bsr 	rsc_init

again:
 lea 	addrin(a6),a0
 lea 	ev_mgpbuff(a6),a1
 move.l	a1,(a0)
 M_evnt_mesag
 move.w	ev_mgpbuff(a6),d0
 cmpi.w	#AC_OPEN,d0
 bne.b	again
 tst.b	is_kaos(a6)
 beq.b	again				; kein KAOS: nichts tun

* Die folgenden Befehle sind speziell an Atari- MistGEM angepaût
 move.w	menu_id(a6),d0
 cmp.w	ev_mgpbuff+4*2(a6),d0	; 4 statt 3 wegen AES- Fehler
 bne.b	again

 bsr 	do_dialog
 bra.b	again


**********************************************************************
*
* AES Handler
*

_aes:
 lea 	control(a6),a0
 clr.l	(a0)
 clr.l	4(a0)
 movep	d0,5(a0)
 swap	d0
 movep	d0,1(a0)
 move.w	#$c8,d0
 lea 	aespb(pc),a0
 move.l	a0,d1
 trap	#2
 move.w	intout(a6),d0
 rts


**********************************************************************
*
* long get_sysvars( void )
*  MUû IM SUPERVISOR-MODUS AUSGEFöHRT WERDEN
*  Liest Betriebssystemversions- spezifische Daten
*  RÅckgabe negativ, wenn System nicht unterstÅtzt wird
*

get_sysvars:
 move.l	_sysbase,a1
 addq.l	#os_version,a1
 cmpi.w	#$0104,(a1)
 bcs.b	getsv_err

 lea 	s_ver+5(pc),a0
 moveq	#'0',d0
 add.b	(a1)+,d0
 move.b	d0,(a0)+
 addq.l	#1,a0				; '.' Åberlesen
 moveq	#'0',d0
 add.b	(a1),d0
 move.b	d0,(a0)

 move.l	_p_cookies,a0
 move.l	a0,d0
 beq.b	getsv_end 			; Zeiger ungÅltig
getcookie_loop:
 move.l	(a0)+,d1				; Cookiename holen
 beq.b	getsv_end 			; Ende der Cookie- Liste
 move.l	(a0)+,d0				; Wert
 cmpi.l	#'KAOS',d1
 bne.b	getcookie_loop
 lea 	s_ver+23(pc),a0
 moveq	#7,d2
getsv_datloop:
 move.w	d0,d1
 andi.b	#$f,d1
 addi.b	#'0',d1
 move.b	d1,-(a0)
 lsr.l	#4,d0
 cmpi.b	#'.',-1(a0)
 bne.b	getsv_nxtnum
 subq.l	#1,a0
getsv_nxtnum:
 dbra	d2,getsv_datloop

getsv_end:
 moveq	#0,d0
 rts
getsv_err:
 moveq	#EINVFN,d0
 rts


**********************************************************************
*
* void rsc_init( void )
*

rsc_init:
 moveq	#NUM_OBS-1,d7
 move.l	#rs_object,addrin(a6)
rsci_loop:
 move.w	d7,intin(a6)
 M_rsrc_obfix
 dbra	d7,rsci_loop
 rts


**********************************************************************
*
* Schreibt die aktuelle Konfiguration in die Dialogbox
* gibt in d0 die geholte Konfiguration zurÅck
*

cnf_to_dialog:
 lea 	rs_object(pc),a1
 move.w	#'AK',-(sp)
 gemdos	Sconfig
 addq.w	#4,sp
* erst die Bits holen, die eine Funktion ausschalten
 not.l	d0

 moveq	#FIRSTBUT+0,d1 		; Button 0
 moveq	#4,d2				; Fastload
 bsr 	_set

 moveq	#FIRSTBUT+2,d1 		; Button 2
 moveq	#6,d2				; smart redraw (keine AES- KompatibilitÑt)
 bsr 	_set

 moveq	#FIRSTBUT+3,d1 		; Button 3
 moveq	#7,d2				; Boxen
 bsr 	_set

 moveq	#FIRSTBUT+4,d1 		; Button 4
 moveq	#8,d2				; Halt
 bsr 	_set
 not.l	d0
* dann die Bits holen, die eine Funktion einschalten
 moveq	#FIRSTBUT+1,d1 		; Button 1
 moveq	#5,d2				; Kompat
 bsr 	_set

 moveq	#FIRSTBUT+5,d1 		; Button 5
 moveq	#10,d2				; menu_click
;bra 	_set

_set:
 mulu	#24,d1
 lea 	ob_state(a1,d1.l),a0
 btst	d2,d0
 beq.b	_set_off
 ori.w	#SELECTED,(a0)
 rts
_set_off:
 andi.w	#!SELECTED,(a0)
 rts


**********************************************************************
*
* ErhÑlt in d0 die aktuelle Konfiguration
* Setzt die aktuelle Konfiguration aus der Dialogbox
* gibt in d0 die gesetzte Konfiguration zurÅck
*

dialog_to_cnf:
 lea 	rs_object(pc),a1
* erst die Bits holen, die eine Funktion einschalten

 moveq	#FIRSTBUT+1,d1 		; Button 1
 moveq	#5,d2
 bsr 	_get

 moveq	#FIRSTBUT+5,d1 		; Button 5
 moveq	#10,d2				; menu_click
 bsr 	_get

* dann die Bits holen, die eine Funktion ausschalten
 not.l	d0

 moveq	#FIRSTBUT+0,d1 		; Button 0
 moveq	#4,d2				; Bit 4
 bsr 	_get

 moveq	#FIRSTBUT+2,d1 		; Button 2
 moveq	#6,d2
 bsr 	_get

 moveq	#FIRSTBUT+3,d1 		; Button 3
 moveq	#7,d2				; Boxen
 bsr 	_get

 moveq	#FIRSTBUT+4,d1 		; Button 4
 moveq	#8,d2				; Halt
 bsr 	_get

 not.l	d0

 move.l	d0,-(sp)
 move.w	#'EL',-(sp)
 gemdos	Sconfig
 addq.w	#4,sp
 move.l	(sp)+,d0
 rts

_get:
 mulu	#24,d1
 btst	#0,ob_state+1(a1,d1.l)	; SELECTED ?
_get_o:
 beq.b	_get_off
 bset	d2,d0
 rts
_get_off:
 bclr	d2,d0
 rts


**********************************************************************
*
* Der Dialog selbst
*

do_dialog:
 suba.w	#40,sp				; 80 Bytes lokale Variablen
 lea 	(sp),a5				; a5 auf lokale Variablen
 lea 	intin(a6),a4

 move.w	#BEG_UPDATE,(a4)
 M_wind_update

 lea 	rs_object(pc),a3

* Maus auf Pfeil umschalten

 clr.w	(a4) 				; ARROW == 0
 M_graf_mouse

 bsr 	cnf_to_dialog
 move.l	d0,d7				; aktuelle Konfiguration

 move.l	a3,addrin(a6)
 M_form_center
 lea 	intout+2(a6),a0
 move.l	(a0)+,(a5)
 move.l	(a0),4(a5)

 moveq	#FMD_START,d0
 bsr 	_form_dial

 lea 	(a4),a0
 clr.w	(a0)+				; ROOT
 move.w	#MAX_DEPTH,(a0)+
 move.l	(a5),(a0)+
 move.l	4(a5),(a0)
 M_objc_draw

 M_form_do

 andi.w	#$7f,d0				; Doppelklick- Bit lîschen
 move.w	d0,d6				; d6 ist Button- Nummer
 mulu	#24,d0
 clr.w	ob_state(a3,d0.w)		; ob_state = NORMAL
 cmpi.w	#OB_ABBRUCH,d6
 beq 	dial_sichern			; Abbruch

* Auswerten der SELECTED- Flags

 move.l	d7,d0				; aktuelle Konfiguration
 bsr 	dialog_to_cnf
 move.l	d0,d7				; neuer Wert

dial_sichern:
 moveq	#FMD_FINISH,d0
 bsr 	_form_dial
 subq.w	#OB_SICHERN,d6
 bne.b	dial_end
 lea 	pgmname(pc),a0
 lea 	(a5),a1
dial_strc:
 move.b	(a0)+,(a1)+
 bne.b	dial_strc
 move.l	a5,addrin(a6)
 M_shel_find					; Wir suchen uns selbst
 beq.b	dial_end				; nicht gefunden
 move.w	#1,-(sp)
 move.l	a5,-(sp)
 gemdos	Fopen
 addq.w	#8,sp
 move.w	d0,d6
 bmi.b	dial_end				; Fehler und Devices absÑgen
 clr.w	-(sp)
 move.w	d6,-(sp)
 moveq	#$1c+2,d0
 move.l	d0,-(sp)
 gemdos	Fseek
 adda.w	#10,sp
 move.l	d7,(a5)
 pea 	(a5)
 moveq	#4,d0
 move.l	d0,-(sp)
 move.w	d6,-(sp)
 gemdos	Fwrite
 addq.w	#2,sp
 gemdos	Fclose
 adda.w	#12,sp
dial_end:

 move.w	#END_UPDATE,(a4)
 M_wind_update

 adda.w	#40,sp				; lokalen Speicher freigeben
 rts



_form_dial:
 lea 	(a4),a0
 move.w	d0,(a0)+
 clr.l	(a0)+
 clr.l	(a0)+
 move.l	(a5),(a0)+
 move.l	4(a5),(a0)+
 M_form_dial
 rts
 


**********************************************************************
**********************************************************************
*
*	   DATA

	IF	COUNTRY=FRG
bit4:	DC.B "Fastload:",0
bit5:   	DC.B "KompatibilitÑt zu TOS:",0
bit6:   	DC.B "Smart Redraw:",0
bit7:	DC.B "Grow- und Shrinkboxen:",0
bit8:	DC.B "Halt nach TOS- Programmen:",0
bit10:	DC.B "Pulldown- MenÅs:",0
s_title:	DC.B "KAOS - Konfigurationsschalter",0
s_ok:	DC.B "OK",0
s_sicher: DC.B "Sichern",0
s_abbr:	DC.B "Abbruch",0
s_but:	DC.B "Ein",0
s_ver:	DC.B "KAOS x.y vom ??.??.????",0
	ENDIF
	IF	(COUNTRY=UK)|(COUNTRY=USA)
bit4:	DC.B "Fastload:",0
bit5:	DC.B "Compatibility to TOS:",0
bit6:	DC.B "Smart Redraw:",0
bit7:	DC.B "Grow- and shrinkboxes:",0
bit8:	DC.B "Stop after TOS programs:",0
bit10:	DC.B "Pulldown menus:",0
s_title:	DC.B "KAOS - Configuration switches",0
s_ok:	DC.B "OK",0
s_sicher: DC.B "Save",0
s_abbr:	DC.B "Cancel",0
s_but:	DC.B "On",0
s_ver:	DC.B "KAOS x.y     ??.??.????",0
	ENDIF

	EVEN

rs_object:
 DC.W	-1,1,NUM_OBS-1,G_BOX,NONE,OUTLINED 		;  0: umgebende Box
 DC.L	$21100
 DC.W	0,0,39,0x050e

 DC.W	2,-1,-1,G_BUTTON,NONE,OUTLINED+SHADOWED 	;  1: Titelzeile
 DC.L	s_title
 DC.W	5,1,30,1

 DC.W	3,-1,-1,G_STRING,NONE,DISABLED			;  2: Version
 DC.L	s_ver
 DC.W	8,0x040a,25,1

 DC.W	4,-1,-1,G_BUTTON,SELECTABLE+DEFAULT+EXIT,NORMAL	; 3: 'OK'
 DC.L	s_ok
 DC.W	4,0x040c,8,1

 DC.W	5,-1,-1,G_BUTTON,SELECTABLE+EXIT,NORMAL 	;  4: 'Sichern'
 DC.L	s_sicher
 DC.W	16,0x040c,8,1

 DC.W	6,-1,-1,G_BUTTON,SELECTABLE+EXIT,NORMAL 	;  5: 'Abbruch'
 DC.L	s_abbr
 DC.W	27,0x040c,8,1

 DC.W	7,-1,-1,G_STRING,NONE,NORMAL
 DC.L	bit4
 DC.W	2,0x0003,9,1

 DC.W	8,-1,-1,G_STRING,NONE,NORMAL					; 7:
 DC.L	bit5
 DC.W	2,0x0104,22,1

 DC.W	9,-1,-1,G_STRING,NONE,NORMAL					; 8:
 DC.L	bit6
 DC.W	2,0x0205,22,1

 DC.W	10,-1,-1,G_STRING,NONE,NORMAL					; 9:
 DC.L	bit7
 DC.W	2,0x0306,22,1

 DC.W	11,-1,-1,G_STRING,NONE,NORMAL 				; 10:
 DC.L	bit8
 DC.W	2,0x0407,26,1

 DC.W	12,-1,-1,G_STRING,NONE,NORMAL 				; 11:
 DC.L	bit10
 DC.W	2,0x0508,26,1


 DC.W	13,-1,-1,G_BUTTON,SELECTABLE,NORMAL			; 12:
 DC.L	s_but
 DC.W	30,0x0003,7,1

 DC.W	14,-1,-1,G_BUTTON,SELECTABLE,NORMAL			; 13:
 DC.L	s_but
 DC.W	30,0x0104,7,1

 DC.W	15,-1,-1,G_BUTTON,SELECTABLE,NORMAL			; 14:
 DC.L	s_but
 DC.W	30,0x0205,7,1

 DC.W	16,-1,-1,G_BUTTON,SELECTABLE,NORMAL			; 15:
 DC.L	s_but
 DC.W	30,0x0306,7,1

 DC.W	17,-1,-1,G_BUTTON,SELECTABLE,NORMAL			; 16:
 DC.L	s_but
 DC.W	30,0x0407,7,1

 DC.W	0,-1,-1,G_BUTTON,LASTOB+SELECTABLE,NORMAL		; 17:
 DC.L	s_but
 DC.W	30,0x0508,7,1


 EVEN

aespb:	DC.L 	(d+control)
		DC.L 	(d+global)
		DC.L 	(d+intin)
		DC.L 	(d+intout)
		DC.L 	(d+addrin)
		DC.L 	(d+addrout)

	IFNE	SCRP
scrpdir:	DC.B 	'C:\GEMSYS\GEMSCRAP',0
	ENDIF

appname:	DC.B 	'  KAOSCONF',0
pgmname:	DC.B 	'KAOSCONF.ACC',0

	EVEN

************ BEGINN DES BSS ***********

d:
	OFFSET	0

is_acc:		DS.B 	1
is_kaos:		DS.B 	1
	EVEN
ap_id:		DS.W 	1
menu_id:		DS.W 	1
xdesk:		DS.W 	1
ydesk:		DS.W 	1
wdesk:		DS.W 	1
hdesk:		DS.W 	1
control:		DS.W 	5
global:		DS.W 	15
intin:		DS.W 	16
intout:		DS.W 	16
addrin:		DS.L 	2
addrout:		DS.L 	1
ev_mgpbuff:	DS.W 	8
bsslen:

	   BSS

	DS.B  bsslen


	END
