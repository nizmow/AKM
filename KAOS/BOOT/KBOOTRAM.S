**********************************************************************
*
* BOOTLADER FöR KAOS.RAM ins FastRAM eines TT 4/8
* ======================
*
**********************************************************************

TOSLEN    EQU  $30000
fstrm_beg EQU  $49e           ; ___md
os_chksum EQU  $486           ; trp14ret


     INCLUDE "OSBIND.INC"

     TEXT
     SUPER

* In den Supervisormodus gehen und Maschine ST/TT bestimmen

     lea       stack,sp
     clr.l     -(sp)
     gemdos    Super
     addq.l    #6,sp

     move.l    _p_cookies,d0
     beq.b     is_st
     move.l    d0,a0
cookie_loop:
     move.l    (a0),d0             ; Kennung
     beq.b     is_st               ; Ende-Kennung
     addq.l    #8,a0
     cmpi.l    #'_MCH',d0
     bne.b     cookie_loop
     move.l    -4(a0),machine
is_st:

* NationalitÑt bestimmen und Titelzeile ausgeben

     move.l    _sysbase,a0
     move.l    os_base(a0),a0      ; RAM- Kopie enthÑlt kein os_palmode
     move.w    os_palmode(a0),d0
     lsr.w     #1,d0
     move.w    d0,nationality

     lea       titel,a0
     bsr       cconws_country

* prÅfen, ob ein RAM-TOS da ist

     move.l    _sysbase,a0
     move.l    4,a1                ; ROM- Startadresse
     cmp.l     os_start(a0),a1     ; vergleichen mit TOS- Startadresse
     bne       err                 ; RAMTOS aktiv !

* Speicherkonfiguration feststellen

     cmpi.l    #$00020000,machine  ; TT ?
     bcs.b     mem_st

* FÅr TT

     move.l    #$1000000,fstrm_beg
     cmpi.l    #$1357bd13,$5a8     ; fstrm_valid, TT-RAM gÅltig ?
     bne.b     ttm_no
     move.l    $5a4,a5
     cmpa.l    #$1200000,a5        ; FastRAM-Ende > 2MB?
     bcs.b     ttm_no              ; kein oder zuwenig FastRAM
     lea       $1000000,a0         ; Beginn TT-RAM
     bsr       mem_len
     move.l    a0,a5
     suba.l    #TOSLEN,a5
     move.l    a5,$5a4             ; Ende TT-RAM
     move.l    a5,a4
     bra       prf
ttm_no:
     lea       0,a0
     bsr       mem_len
     move.l    a0,a5
     suba.l    #TOSLEN,a5
     move.l    a5,a4
     suba.l    #$25900,a4          ; neuer Bildschirmspeicher
     bra       prf

* FÅr ST(E)

mem_st:
     move.b    memctrl,d0
     lsr.b     #2,d0
     bsr       mem_bank
     move.l    a0,a5               ; Bank #0
     move.b    memctrl,d0
     bsr       mem_bank            ; Bank #1

     cmpa.l    #the_label,a5       ; prÅfen, ob wir in der ersten Bank liegen
     bcs.b     the_label           ; Bank #1 muû existieren
     move.w    sr,-(sp)
     ori.w     #$700,sr
     move.l    #$12345678,d0
     move.l    (a5),d1
     move.l    d0,(a5)
     move.l    (a5),d2
     move.l    d1,(a5)
     move.w    (sp)+,sr
     cmp.l     d2,d0
     bne.b     no_bank1            ; kein RAM bei Bank #1
the_label:
     add.l     a0,a5               ; physikalisches Speicherende
no_bank1:

* a4 zeigt auf neue Bildschirmspeicheradresse

     suba.l    #TOSLEN,a5
     move.l    a5,a4
     suba.l    #$8000,a4           ; neuer Bildschirmspeicher

* PrÅfen, ob ein KAOS schon da ist

prf:
     cmpi.b    #$60,(a5)
     bne       do_load
     move.l    4(a5),d0
     clr.b     d0                  ; Lobyte lîschen
     cmp.l     a5,d0
     bne       do_load
     move.l    $14(a5),a0
     cmpi.l    #$87654321,(a0)
     bne       do_load

* KAOS schon da => weiter mit AUTO- Ordner ?

     move.l    a5,a6               ; hier liegt es
     moveq     #0,d0
     bsr       info_act
     move.w    d0,d6               ; Residentmodus merken
     bra       do_start

* ôffnen der Datei: Handle d7

do_load:
     lea       puffer,a6
     clr.w     -(sp)
     pea       name
     gemdos    Fopen
     addq.l    #8,sp
     move.w    d0,d7
     bmi       err

* Ausgabe der Betriebssystemversion und Ladeadresse

     bsr       info_load
     move.w    d0,d6               ; Residentmodus merken

* Einlesen der gesamten Datei

     pea       (a6)
     move.l    #TOSLEN+$1c+4000,-(sp)
     move.w    d7,-(sp)
     gemdos    Fread
     adda.w    #12,sp
     move.l    d0,d5               ; d5 = DateilÑnge
     move.w    d7,-(sp)
     gemdos    Fclose
     addq.w    #4,sp
     tst.l     d5
     bmi       err
     tst.l     d0
     bmi       err
     cmpi.w    #$601a,(a6)
     bne       err

* öberprÅfung der Ladeadresse

     move.l    a5,d0
     btst      #0,d0
     bne       err2                ; ungerade Ladeadresse
     cmpa.l    a7,a4
     bcs       err2                ; Ladeadresse zu tief
     move.l    #$12345678,d0
     move.l    (a5),d1
     move.l    d0,(a5)
     move.l    (a5),d2
     move.l    d1,(a5)
     cmp.l     d2,d0
     bne       err2                ; kein RAM bei Ladeadresse
     move.l    a5,a0
     add.l     #TOSLEN-4,a0        ; letztes Langwort von KAOS
     move.l    (a0),d1
     move.l    d0,(a0)
     move.l    (a0),d2
     move.l    d1,(a0)
     cmp.l     d2,d0
     bne       err2                ; kein RAM bei Ende der Ladeadresse

* Auf Sound- Beendigung warten

wait_snd:
     moveq     #-1,d0
     move.l    d0,-(sp)
     xbios     Dosound
     addq.l    #6,sp
     tst.l     d0
     bne.b     wait_snd

* KAOS modifizieren, verschieben und starten

     cmpi.l    #$00020000,machine
     bcs.b     ver_st
     move.w    #$0300,os_version+$1c(a6)
ver_st:
     lea       $1c(a6),a1          ; a1 = Anfang des TOS
     move.l    a1,a2
     adda.l    #TOSLEN,a2          ; a2 = Ende des TOS
     move.b    1(a1),d0
     ext.w     d0                  ; Branch- Offset
     lea       6(a1,d0.w),a1
     cmpi.w    #$4e70,(a1)         ; "reset" ?
     bne.b     res_nores
     move.w    #$4e71,(a1)+        ; Befehl "reset" durch "nop" ersetzen
res_nores:
     sf        d1
res_loop:
     move.l    a1,a0               ; Test auf $31415926,$0426
     cmpi.l    #$31415926,(a0)+
     bne.b     res_endloop
     cmpi.w    #$0426,(a0)+
     bne.b     res_endloop
     move.b    #$60,(a0)           ; "bra" statt "bne"
     bra.b     clrscr
res_endloop:
     addq.l    #2,a1
     cmpa.l    a2,a1
     bcs.b     res_loop
;    clr       0                   ; fataler Fehler
clrscr:
     cmpa.l    #$1000000,a5
     bcc       no_scr

     move.l    a4,a0
     move.w    #$3fff,d0
loop:
     clr.l     (a0)+               ; neuen Bildschirm lîschen
     dbra      d0,loop

     move.w    #-1,-(sp)           ; Auflîsung unverÑndert
     move.l    a4,-(sp)
     move.l    a4,-(sp)            ; logische = physikalische Adresse
     xbios     Setscreen           ; Bildschirmspeicher verschieben
     adda.w    #12,sp
no_scr:

* KAOS relozieren

     move.l    2(a6),d0            ; LÑnge von TEXT
     add.l     6(a6),d0            ; LÑnge von DATA
     add.l     $e(a6),d0           ; LÑnge von SYM

     lea       $1c(a6,d0.l),a3     ; Beginn der Relocation- Daten
     lea       0(a6,d5.l),a2       ; Dateiende
     cmpa.l    a2,a3
     bcc.b     end_reloc
     lea       $1c(a6),a0
     move.l    (a3)+,d0            ; erstes Relocation- Langwort
reloc_loop:
     add.l     d0,a0
     move.l    a5,d0
     add.l     d0,(a0)             ; relozieren!
reloc_loop2:
     cmpa.l    a2,a3
     bhi.b     end_reloc           ; Dateiende!
     moveq     #0,d0
     move.b    (a3)+,d0
     beq.b     end_reloc           ; Ende der Tabelle
     cmpi.b    #1,d0
     bne.b     reloc_loop
     lea       254(a0),a0
     bra.b     reloc_loop2

end_reloc:
* Checksumme ermitteln
 lea      $1c(a6),a0
 lea      0(a0,d5.l),a1
 moveq    #0,d0
os_chkloop:
 add.l    (a0)+,d0
 cmpa.l   a0,a1
 bcs.b    os_chkloop
 move.l   d0,os_chksum

     ori.w     #$700,sr

     move.l    a5,a0
     lea       $1c(a6),a1
     bsr       toscpy

     cmpa.l    #$1000000,a5
     bcc.b     do_start
     move.l    a5,phystop          ; Systemvariablen setzen
     move.l    a4,_memtop

do_start:
     move.l    #$5555aaaa,$51a
     cmpi.b    #$1b,d6
     beq.b     no_reset            ; Esc => nicht resetfest machen
     move.l    #$31415926,resvalid
     move.l    a5,resvector
no_reset:
     jmp       (a5)                ; KAOS starten


err2:
     lea       fehler,a0
     bsr       cconws_country
err:
     gemdos    Pterm0


**********************************************************************
*
* void *mem_len(a0 = void *mem)
*
* prÅft, bis wohin der bei <mem> beginnende physikalische Speicher geht.
* Es wird alle 1024 Bytes ein Test-Langwort geschrieben
*
**********************************************************************

mem_len:
 move.w   sr,-(sp)
 ori.w    #$700,sr
 move.l   8,-(sp)                  ; Busfehlervektor retten
 move.l   #meml_ende,8
 move.l   sp,a2
 move.l   #$21071965,d0            ; Testmuster
 bra.b    meml_begin
meml_loop:
 lea      1024(a0),a0              ; nÑchste 1024 Bytes
meml_begin:
 lea      1024-4(a0),a1            ; Blockende (letzte 4 Bytes)
 move.l   (a1),d1                  ; alten Wert retten
 move.l   d0,(a1)                  ; Testwert schreiben
 move.l   (a1),d2                  ; Testwert lesen
 move.l   d1,(a1)                  ; alten Wert restaurieren
 cmp.l    d2,d0
 beq.b    meml_loop
meml_ende:
 move.l   a2,sp
 move.l   (sp)+,8
 move.w   (sp)+,sr
 rts


**********************************************************************
*
* int info_load
*
**********************************************************************

info_load:
     movem.l   d3/d4/d5/d6,-(sp)

     clr.w     -(sp)
     pea       name
     gemdos    Fsfirst
     addq.l    #8,sp
     tst.l     d0
     bmi       err

     gemdos    Fgetdta
     addq.l    #2,sp
     move.l    d0,a0

     lea       $1e(a0),a0
     bsr       cconws              ; Dateinamen ausgeben

     lea       tit_load,a0
     bsr       cconws_country

     gemdos    Cnecin              ; Taste zurÅckgeben
     addq.l    #2,sp
     movem.l   (sp)+,d3/d4/d5/d6
     rts


**********************************************************************
*
* int info_act
*
**********************************************************************

info_act:
     movem.l   d3/d4/d5/d6,-(sp)

     lea       tit_act,a0
     bsr       cconws_country

     gemdos    Cnecin              ; Taste zurÅckgeben
     addq.l    #2,sp
     movem.l   (sp)+,d3/d4/d5/d6
     rts


**********************************************************************
*
* void cconws_country(a0 = char *s)
*
* in <nationality> steht die NationalitÑt
* Aufbau der Zeichenkette, auf die a0 zeigt:
* char n1,n2,...,-1      NationalitÑten fÅr erste Zeichenkette
* char s1[]              erste Zeichenkette
* char n3,n4,...,-1      NationalitÑten fÅr zweite Zeichenkette
* char s2[]              zweite Zeichenkette
* char -1                Abschluû
* char defs[]            Defaultstring (i.a. englisch)
*
**********************************************************************

cconws_country:
 move.w   d4,-(sp)
 move.w   nationality,d4
 bsr      _chk_nat
 bne.b    cconws_country
 move.w   (sp)+,d4
cconws:
 move.l   a0,-(sp)
 gemdos   Cconws
 addq.l   #6,sp
 rts

_chk_nat:
 move.b   (a0)+,d0
 bmi.b    _chk_ende                ; Abschluûbyte, Default verwenden
_chk_nxt:
 cmp.b    d0,d4                    ; unsere NationalitÑt ?
 beq.b    _chk_found
 move.b   (a0)+,d0                 ; nÑchste NationalitÑt
 bge.b    _chk_nxt                 ; weiter vergleichen
_chk_nxtstr:
 tst.b    (a0)+                    ; Zeichenkette Åberspringen
 bne.b    _chk_nxtstr
 moveq    #1,d0                    ; nicht gefunden
 rts
_chk_found:
 tst.b    (a0)+
 bge.b    _chk_found
_chk_ende:
 moveq    #0,d0                    ; gefunden, a0 ausdrucken
 rts


**********************************************************************
*
* a0 = long mem_bank(d0 = char conf)
*
**********************************************************************

mem_bank:
     lea       $20000,a0           ; 128k fÅr Bank #0
     move.b    d0,d1
     andi.b    #$3,d1
     beq.b     isbank
     adda.l    a0,a0
     adda.l    a0,a0               ; 512k fÅr Bank #0
     subq.b    #1,d1
     beq.b     isbank
     adda.l    a0,a0
     adda.l    a0,a0               ; 2MB fÅr Bank #0
isbank:
     rts

**********************************************************************
*
* void toscpy(a0 = void *dst, a1 = void *src)
*
**********************************************************************

toscpy:
 move.w   #(TOSLEN/8)-1,d0
 cmpa.l   a0,a1
 bhi      cpy_uloop
 beq      cpy_end
 adda.l   #TOSLEN,a1
 adda.l   #TOSLEN,a0
cpy_dloop:
 move.l   -(a1),-(a0)
 move.l   -(a1),-(a0)
 dbra     d0,cpy_dloop
cpy_end:
 rts
cpy_uloop:
 move.l   (a1)+,(a0)+
 move.l   (a1)+,(a0)+
 dbra     d0,cpy_uloop
 rts


     DATA

titel:    DC.B -1
          DC.B CR,LF,'KAOSø/MAGIX- BOOTER, Ω 1990-91 Andreas Kromke',CR,LF,0

fehler:   DC.B FRG,SWG,-1
          DC.B 'KBOOTRAM: Falsche Ladeadresse (kein RAM dort vorhanden ?)',0
          DC.B -1
          DC.B 'KBOOTRAM: Invalid load address (no RAM at load address ?)',0

tit_load: DC.B FRG,SWG,-1
          DC.B ' wird gebootet',CR,LF,LF
          DC.B 'Leertaste:   laden und resetfest machen',CR,LF
          DC.B 'Esc:         laden',CR,LF
          DC.B '^C:          nicht laden',CR,LF,LF,0
          DC.B -1
          DC.B ' will be booted',CR,LF,LF
          DC.B 'Space:       load and make reset resident',CR,LF
          DC.B 'Esc:         load',CR,LF
          DC.B "^C:          don't load",CR,LF,LF,0

tit_act:  DC.B FRG,SWG,-1
          DC.B 'inaktives Betriebssystem im RAM',CR,LF,LF
          DC.B 'Leertaste:   resetfest machen und starten',CR,LF
          DC.B 'Esc:         nur starten',CR,LF
          DC.B '^C:          nicht starten',CR,LF,LF,0
          DC.B -1
          DC.B 'inactive operating system in RAM',CR,LF,LF
          DC.B 'Space:       make reset resident and start',CR,LF
          DC.B 'Esc:         only start',CR,LF
          DC.B "^C:          don't start",CR,LF,LF,0

     EVEN
machine:  DC.L  0          ; Maschine per Default ein ST
nationality:
          DC.W 0
name:     DC.B '\*.ram',0

     BSS

     EVEN

     DS.L      200
stack:

puffer:
     DS.B      TOSLEN+$1c+4000

     END
