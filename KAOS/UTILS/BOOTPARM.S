

 text 
Begin:

 move.l  4(sp),a0
 lea     $81(a0),a0     ;pointer auf commandline nach a0
 move.b  (a0),d0
 tst.b   d0
 bne.b   _begin         ;wenn keine parameter nur werte ausgeben
 move.w  #1,noparm
 bra     readhd

_begin:
 moveq.l #0,d0
 move.l  d0,d1
 move.w  d0,noparm
 move.b  (a0),d2
 cmpi.b  #'?',d2
 bne.b   id_loop
 bra     helpout
 addq    #1,d1

id_loop:
 move.b  0(a0,d1.w),d2
 sub.b   #'0',d2
 cmpi.b  #9,d2
 bhi.b   id_end
 move.l  d0,d3
 lsl.l   #1,d3
 lsl.l   #3,d0
 add.l   d3,d0
 ext.w   d2
 ext.l   d2
 add.l   d2,d0
 addq    #1,d1
 bra.b   id_loop
id_end:
 tst.l   d1
 beq.b   readhd
 cmpi.l  #79,d0
 bhi.b   readhd
 ext.w	d0
 move.w  d0,delayin
 bra.b   hddelay


readhd:
 move.w  #-1,delayin

hddelay:
 move.w  delayin,-(sp)        ;setzen
 move.l  #$c0003,-(sp)        ;config,cmosparm
 trap    #13                  ;Bios
 addq.l  #6,sp                ;
 move.w  d0,delayout

 lea     copymsg(pc),a0
 bsr     message

 tst.w   noparm
 beq.b   _out

 lea noparmmsg(pc),a0
 bsr message

_out:
 clr.l   -(sp)
 move.w  #$20,-(sp)           ;Super
 trap    #1
 addq.l  #6,sp

 move.l  d0,sysstack

 bsr     chk_rtclock
 bcc.b   _printout2

_printout1:
 lea     noclk(pc),a0
 bra.b   printout

_printout2:
 lea     message1(pc),a0

printout:
 bsr     message

 move.l  sysstack,-(sp)
 move.w  #$20,-(sp)           ;User
 trap    #1
 addq.l  #6,sp

 move.w  delayout,d0
 bsr     printdec

 lea     message2(pc),a0
 bsr     message

 bra.b   key

helpout:
 lea     copymsg(pc),a0
 bsr     message

 lea     helpmsg(pc),a0
 bsr     message

key:
 lea     keymsg(pc),a0
 bsr     message

 move.w  #8,-(sp)
 trap    #1
 addq.l  #2,sp

exit:
 clr.w   -(sp) 	           ;PTERM0
 trap    #1

printdec:
 moveq.l #1,d2
 lea    zehner(pc),a1
 move.b d0,d1

d_loop:
 move.b 0(a1,d2.w),d3
 moveq  #$30,d4

d_verglei:
 cmp.b  d3,d1
 bcs.b  d_ausgabe
 sub.b  d3,d1
 addq   #1,d4
 bra.b  d_verglei

d_ausgabe:
d_aus:
 move.w  d4,-(sp)
 move.w  #2,-(sp)
 trap    #1
 addq.l  #4,sp

d_next:
 subq    #1,d2
 bpl.b   d_loop
 rts

*
* PrÅft, ob Mega- Uhr da ist
*
* Carry gesetzt, wenn nicht da
* zerstîrt d0/d1/d2/a0/a1
*

chk_rtclock:
 lea 	 $fffffc20,a0
 move.l  8,d2
 movea.l sp,a1
 move.l	 #no_clk,8

* die Alarmzeit wird nicht zerstîrt

 ori.b   #%00001001,27(a0)      ; Bank 1 anwÑhlen
 move.l  d2,8					; kein Buserror, reparieren
 movep.w 5(a0),d2				; rette alte Alarm-Minuten Einer
 move.w  #$a05,d0				; beschreibe Alarm-Minuten Einer
 movep.w d0,5(a0)
 movep.w 5(a0),d1				; hole Alarm-Minuten Einer
 movep.w d2,5(a0)				; repariere Alarm-Minuten Einer
 and.w   #%0000111100001111,d1  ; maskiere MÅll weg
 cmp.w   d0,d1                  ; Chip enthielt den Testwert ?
 bne.b   _no_clk				; Nein: Uhr nicht aktiv, Error !
 ori.b   #%00000001,1(a0)       ;
 ori.b   #%00001000,27(a0)      ;
 andi.b  #%11111110,27(a0)		; selektiere wieder Bank 0

;-----------------------------------
 andi.b  #$fe,ccr               ; Carry lîschen
;-----------------------------------
 rts

no_clk:
 movea.l	a1,sp
 move.l	d2,8
_no_clk:
 ori.b 	#1,ccr				   ; Carry setzen
 rts

message:
 move.l  a0,-(sp)
 move.w  #9,-(sp)
 trap    #1
 addq.l  #6,sp
 rts


; data

zehner:
 dc.b 1,10

  even

copymsg:
 dc.b ' CMOS Parametereinstellung fÅr   ',$d,$a
 dc.b ' Bootparameter unter KAOSø 1.4.2 ',$d,$a
 dc.b ' Version 1.00   Copyright Ω 1991 ',$d,$a
 dc.b ' elCo, D-3000 Hannover 1         ',$d,$a,$a,0

  even

message1:
 dc.b ' Gespeicherte Bootparametersumme: ',0

  even

message2:
 dc.b $d,$a,0

  even

noclk:
 dc.b ' Keine Mega (kompatible) Uhr vorhanden !',$d,$a
 dc.b ' ROM-Bootparametersumme: ',0

  even

helpmsg:
 dc.b ' Aufruf: BOOTPARM value',$d,$a
 dc.b ' value = Summe der Bootparameter',$d,$a
 dc.b ' diese setzt sich wie folgt zusammen:',$d,$a
 dc.b ' Wertigkeit 1 = Registerausgabe statt',$d,$a
 dc.b '                ',39,'Bomben',39,$d,$a
 dc.b ' Wertigkeit 2 = Booten zuerst von DMA',$d,$a
 dc.b '                dann von Floppy',$d,$a
 dc.b ' Wertigkeit 4 = Floppy-Laufwerk nach',$d,$a
 dc.b '                Booten deselektieren',$d,$a
 dc.b ' Wertigkeit 8 = AusfÅhrbaren Disketten-',$d,$a
 dc.b '                Bootsektor melden',$d,$a
 dc.b ' Beispiel: BOOTPARM 5',$d,$a
 dc.b ' = Laufwerk nach Booten aus und',$d,$a
 dc.b '   Registerausgabe statt Bomben',$d,$a,$a
 dc.b '        ?      diese Hilfe',$d,$a
 dc.b ' BOOTPARM ohne Parameter gibt nur die',$d,$a
 dc.b ' aktuelle Parametersumme aus.',$d,$a,0

  even

noparmmsg:
 dc.b ' Parameter mit ',39,'bootparm ?',39,$d,$a,$a,0
  even

keymsg:
 dc.b $a,' weiter mit beliebiger Taste',$1b,'f',$d,$a,0 


  bss

noparm:
 ds.w 1

delayin:
 ds.w 1

delayout:
 ds.w 1

sysstack:
 ds.l 1

      END

