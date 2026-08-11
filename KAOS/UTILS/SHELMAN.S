**********************************************************************
*
* SHELMAN (Shell- Manager fÅr KAOSDESK)
*
* installiert die GEM 2.x - Befehle shel_rdef/shel_wdef/appl_bvset,
* letzteren als Dummy.
* SHELMAN erlaubt, eine Defaultshell wie unter KAOS 1.4 anzumelden,
* KAOS 1.4 wird von SHELMAN emuliert.
*
* letzte énderung 27.09.92
*
**********************************************************************

/*
typedef struct
     {
     int  dummy;                   /* ein Nullwort               */
     long magic;                   /* 'SHEL', wenn ist Shell     */
     int  isfirst;                 /* erster Aufruf der Shell    */
     long lasterr;                 /* letzter Fehler             */
     int  wasgr;                   /* Programm war Grafikapp.    */
     } SHELTAIL;
*/

     INCLUDE "OSBIND.INC"


     TEXT
     SUPER

_base     EQU    *-$100

old_after EQU    _base+128         ; DS.L 1
istos14   EQU    _base+132         ; DS.B 1


 bra      main


**********************************************************************
*
* der neue Trap #1
*

xbra1:
 DC.B     'XBRA'                   ; Magische Zahlen
 DC.B     'SHMN'                   ; Programm-ID : Shell- Manager
 DS.L     1                        ; Platz fÅr alten Vektor

neu_trap1:
 btst     #5,(sp)
 beq      to_old1                  ; aus Usermode nicht abfangen
 lea      6(sp),a0                 ; fÅr 68000
 tst.w    cpu_typ
 beq.b    cpu_00
 addq.l   #2,a0                    ; fÅr 68010 usw.
cpu_00:
 cmpi.w   #Pexec,(a0)
 bne      to_old1                  ; kein Pexec    -> nicht abfangen

 tst.b    shel_name
 beq      to_old1                  ; keine Default- Applikation

 lea      2(sp),a0
 move.l   _sysbase,d0
 cmp.l    (a0),d0
 bcc      to_old1                  ; RÅcksprung vor OS  -> nicht abfangen
 add.l    #$30000,d0
 cmp.l    (a0),d0
 bcs      to_old1                  ; RÅcksprung nach OS -> nicht abfangen
 move.l   (a0),old_after           ; RÅcksprung vom Trap merken
 move.l   #after,(a0)              ; neuen Wert setzen
 clr.w    shelw_doex               ; doex = 0

to_old1:
 move.l   xbra1+8(pc),-(sp)
 rts

* Wird nach einem Pexec aus dem ROM aufgerufen

after:
 tst.b    shel_name                ; Programm angemeldet ?
 beq      normal                   ; nein, nichts tun
 move.l   d0,shel_tail+8           ; Programm- RÅckgabewert merken
 movem.l  a0/a1/a2/a5/d0/d1/d2,rett

* prÅfen, ob die Shell defekt ist oder fehlt, ggf. abmelden

 tst.w    d0
 bge.b    af_noerr                 ; Programm ist gelaufen
; Programm meldet Fehler
 lea      (sp),a0
 cmpi.l   #$004b0000,(a0)+
 bne.b    af_noerr                 ; Abfrage nur zur Sicherheit
 move.l   (a0)+,a1                 ; cmd
 move.l   (a0),a0                  ; tail
 lea      shel_tail(pc),a5
 moveq    #6-1,d1
af_tloop:
 cmp.b    (a0)+,(a5)+              ; 6 Bytes vergleichen
 dbne     d1,af_tloop
 bne.b    af_noerr                 ; war nicht die Shell
 lea      shel_name(pc),a0
 bsr      strcmp
 bne.b    af_noerr
 clr.b    shel_name                ; Shell abmelden
 bra      normal2

af_noerr:
 tst.w    shelw_doex               ; war schon ein shel_write ?
 beq      nachladen                ; nein, KAOSDESK laden
 tst.b    istos14
 bne      normal2                  ; unter TOS 1.4 nicht prÅfen, ob da
* PrÅfen, ob Datei existiert (fÅr TOS 1.2 nîtig)
 clr.w    -(sp)
 pea      shelw_cmd(pc)
 gemdos   Fattrib                  ; Attribut holen = sehen, ob ex.
 addq.w   #8,sp
 tst.l    d0
 bpl      normal2                  ; Datei existiert
 lea      notfounds(pc),a5
 bsr      wait
 bra.b    nowait                   ; sonst nachladen

**********************************************************************
*
* Die Shell wird nachgeladen.
* Wenn nach einem TOS- Programm gewartet werden soll, wird
* vor dem Nachladen auf eine Taste gewartet
*

nachladen:
 move.w   shelw_isgr(pc),d0
 bne.b    nowait
 lea      waits(pc),a5
 bsr      wait                     ; auf Taste warten

nowait:
;move.l   #shel_name,addrin
;move.l   #shel_tail,addrin+4
;move.w   #1,intin
;move.w   #1,intin+2
;move.w   #1,intin+4
;move.w   #121,contrl
;move.w   #3,contrl+2
;move.w   #1,contrl+4
;move.w   #2,contrl+6
;move.w   #0,contrl+8
 move.l   #aespb,d1
 move.w   #$c8,d0

 move.l   usp,a5                   ; usp retten
 lea      u_stack(pc),a0
 move.l   a0,usp                   ; usp umsetzen
 trap     #2
 move.l   a5,usp                   ; usp zurÅck

 clr.w    shelw_doex
normal2:
 movem.l  rett,a0/a1/a2/a5/d0/d1/d2

normal:
 move.l   old_after(pc),-(sp)
 clr.l    old_after                ; nochmalige Verwendung verhindern
 rts

* void wait(a5)
*  Druckt die Zeichen in a5[] nach Device 2 (CON) und wartet

putch:
 move.w   d0,-(sp)
 move.w   #2,-(sp)
 bios     Bconout
 addq.l   #6,sp
wait:
 move.b   (a5)+,d0
 bne.b    putch
_wait:
 move.w   #2,-(sp)
 bios     Bconstat
 addq.w   #4,sp
 tst.w    d0
 bne.b    is_key
 DC.W     A_INIT
 btst.b   #0,MOUSE_BT+1(a0)
 bne.b    is_klick
 bra.b    _wait
is_key:
 move.w   #2,-(sp)
 bios     Bconin
 addq.w   #4,sp
is_klick:
 rts


**********************************************************************
*
* der neue Trap #13
*

xbra13:
 DC.B     'XBRA'                   ; Magische Zahlen
 DC.B     'SHMN'                   ; Programm-ID : Shell- Manager
 DS.L     1                        ; Platz fÅr alten Vektor

neu_trap13:
 btst.b   #5,(sp)                  ; Super ?
 beq      to_old13                 ; nein, user
 lea      6(sp),a0                 ; fÅr 68000
 tst.w    cpu_typ
 beq.b    cpu_00_13
 addq.l   #2,a0                    ; fÅr 68010 usw.
cpu_00_13:
 cmpi.l   #$00050101,(a0)          ; bios Setexc(etv_critic)
 bne      to_old13                 ; nein
 movea.l  $88,a0                   ; trap #2
xbra_loop:
 cmpi.l   #'XBRA',-12(a0)
 bne      install_2                ; nein
 cmpi.l   #'SHMN',-8(a0)
 beq      to_old13                 ; ja, bin drin
 movea.l  -4(a0),a0
 bra      xbra_loop                ; Kette verfolgen
install_2:
 move.l   $88,xbra2+8              ; alten GEM retten
 move.l   #neu_trap2,$88           ; neuen GEM setzen
to_old13:
 move.l   xbra13+8(pc),-(sp)
 rts


**********************************************************************
*
* der neue Trap #2
*

xbra2:
 DC.B     'XBRA'                   ; Magische Zahlen
 DC.B     'SHMN'                   ; Programm-ID : Shell- Manager
 DS.L     1                        ; Platz fÅr alten Vektor

neu_trap2:
 cmpi.w   #$c8,d0
 bne.b    to_old2
 move.l   d1,a0
 move.l   (a0),a0                  ; contrl
 cmpi.w   #$10,(a0)
 beq      gem2_fn
 cmpi.w   #$7e,(a0)
 beq      gem2_fn
 cmpi.w   #$7f,(a0)
 beq      gem2_fn
 cmpi.w   #$79,(a0)
 bne.b    to_old2
 bsr      my_shel_write
to_old2:
 move.l   xbra2+8(pc),-(sp)
 rts

gem2_fn:
 movem.l  a1/a2/d1/d2,-(sp)
 move.w   (a0),d0
 cmpi.w   #$10,d0
 beq      _she                     ; appl_bvset ist dummy
 move.l   d1,a1                    ; AesPb
 move.l   12(a1),a2                ; a2 = intout
 move.l   16(a1),a1                ; a1 = addrin
 move.l   (a1)+,a0                 ; cmd
 move.l   (a1),a1                  ; tail
 cmpi.w   #$7e,d0
 beq.b    _shr
 bsr      shel_wdef
 bra.b    _she
_shr:
 bsr      shel_rdef
_she:
 movem.l  (sp)+,a1/a2/d1/d2
 move.l   d1,a0                    ; AesPb
 move.l   12(a0),a0                ; a0 = intout
 move.w   #1,(a0)                  ; intout[0] = 1 (kein Fehler)
 moveq    #0,d0                    ; AES kommt immer mit d0=0 zurÅck
 rte


my_shel_write:
 movem.l  a1/a2/d0,-(sp)
 move.l   d1,a0
 move.l   8(a0),a1                 ; intin
 move.l   16(a0),a2                ; addrin
 lea      shelw_doex(pc),a0
 move.w   (a1)+,(a0)+              ; doex
 move.w   (a1)+,(a0)+              ; isgr
 move.w   (a1),(a0)+               ; isover
 move.l   (a2)+,a1                 ; cmd
 moveq    #128-1,d0
msw_loop1:
 move.b   (a1)+,(a0)+
 dbra     d0,msw_loop1
 move.l   (a2),a1                  ; tail
 moveq    #128-1,d0
msw_loop2:
 move.b   (a1)+,(a0)+
 dbra     d0,msw_loop2
 movem.l  (sp)+,a1/a2/d0
 rts


**********************************************************************
*
* void shel_rdef(a0 = char *cmd, a1 = char *dir)
*

shel_rdef:
 lea      shel_name(pc),a2
* Suche Zeiger hinter letztes ':' oder '\'
srd_merke:
 move.l   a2,d1                    ; Zeiger hinter ':' oder '\'
srd_loop1:
 move.b   (a2)+,d0
 beq.b    srd_endloop
 cmpi.b   #':',d0
 beq.b    srd_merke
 cmpi.b   #'\',d0
 beq.b    srd_merke
 bra.b    srd_loop1
srd_endloop:
 move.l   d1,a2                    ; Zeiger hinter letztes ':' oder '\'
srd_loop2:
 move.b   (a2)+,(a0)+              ; kopiere den Namen einschl. EOS
 bne.b    srd_loop2
 lea      shel_name(pc),a2
srd_loop3:
 cmpa.l   d1,a2                    ; Dateinamen erreicht ?
 bcc.b    srd_endloop3
 move.b   (a2)+,(a1)+
 bra.b    srd_loop3
srd_endloop3:
 clr.b    (a1)                     ; EOS des Pfads setzen
 rts


**********************************************************************
*
* void shel_wdef(a0 = char *cmd, a1 = char *dir)
*
* Kopiert <dir> nach shel_name und hÑngt <cmd> an. Ggf. wird ein '\'
* zwischen Pfad und Programmname ergÑnzt
*

shel_wdef:
 lea      shel_name(pc),a2
 tst.b    (a0)
 beq      swd_copy                 ; Name ist leer, Shell abmelden
 tst.b    (a1)
 beq.b    swd_name                 ; Pfad ist leer
swd_loop1:
 move.b   (a1)+,(a2)+              ; zunÑchst den Pfad kopieren
 bne.b    swd_loop1
 bra      swd_addname
swd_name:
 move.l   a0,a1                    ; cmd
swd_loop2:
 cmpi.b   #'\',(a1)
 beq.b    swd_copy
 cmpi.b   #':',(a1)
 beq.b    swd_copy
 tst.b    (a1)+
 bne.b    swd_loop2
 move.l   a0,-(sp)
 gemdos   Dgetdrv
 addq.l   #2,sp
 add.b    #'A',d0
 move.b   d0,(a2)+
 move.b   #':',(a2)+
 clr.w    -(sp)                    ; akt. Laufwerk
 pea      (a2)
 gemdos   Dgetpath
 addq.l   #8,sp
 move.l   (sp)+,a0
swd_addname:
 move.l   a0,a1
 lea      shel_name(pc),a0
 bra      add_name
swd_copy:
 move.b   (a0)+,(a2)+              ; dann den Namen anhÑngen
 bne.b    swd_copy
 rts


**********************************************************************
*
* d0 = char *add_name(a0 = char *path, a1 = char *name)
*
* <path> ist ein Pfad wie "C:\ACC" oder "C:\ACCS\" oder "C:".
* <name> ist "*.*" oder "*.prg" oder "datei.ext".
* gibt Zeiger auf bisheriges String- Ende zurÅck
*

add_name:
 tst.b    (a0)
 beq.b    adn_setslash
adn_loop:
 tst.b    (a0)+
 bne.b    adn_loop
 subq.l   #1,a0                    ; a0 auf EOS
 move.l   a0,d0                    ; bisheriges Ende zurÅckgeben
 cmpi.b   #'\',-1(a0)
 beq.b    adn_cat
 cmpi.b   #':',-1(a0)
 beq.b    adn_cat
adn_setslash:
 move.b   #'\',(a0)+
adn_cat:
 move.b   (a1)+,(a0)+
 bne.b    adn_cat
 rts


* int strcmp(a0 = char *string1, a1 = char *string2)
*  RÅckgabe: string1-string2 (als Vektoren)

str2:
 tst.b    -1(a0)
 bne.b    strcmp
 moveq    #0,d0
 bra.b    str3
strcmp:
 cmp.b    (a1)+,(a0)+
 beq.b    str2
 move.b   -1(a0),d0
 sub.b    -1(a1),d0
 ext.w    d0
str3:
 rts


notfounds:
 DC.B     CR,LF,'Programm fÅr EXEC nicht gefunden'
waits:
 DC.B     ESC,'e',CR,LF,'Taste drÅcken : ',0

     EVEN

shelw_doex:
 DC.W     0                        ; doex
shelw_isgr:
 DC.W     0                        ; isgr
 DC.W     0                        ; isover
shelw_cmd:
 DS.B     128                      ; cmd
shelw_tail:
 DS.B     128                      ; tail
shel_name:
 DCB.B    128,0

aespb:
 DC.L     contrl
 DC.L     global
 DC.L     intin
 DC.L     intout
 DC.L     addrin
 DC.L     addrout

contrl:
 DC.W     $79                      ; shel_write
 DC.W     3                        ; len(intin)
 DC.W     1                        ; len(intout)
 DC.W     2                        ; len(addrin)
 DC.W     0                        ; len(addrout)
global:
 DS.W     30
intin:
 DC.W     1                        ; doex
 DC.W     1                        ; isgr
 DC.W     1                        ; isover
 DC.W     0
intout:
 DS.W     4
addrin:
 DC.L     shel_name
 DC.L     shel_tail
addrout:
 DS.L     1
rett:
 DS.L     7

 DS.W     100
u_stack:

shel_tail:
 DC.W     0                        ; dummy, ein Nullwort
 DC.L     'SHEL'
 DC.W     0                        ; isfirst (immer 0 unter TOS/KAOS)
 DC.L     0                        ; lasterr
 DC.W     0                        ; wasgr

ende:

************* nichtresidenter Teil ***********************************

main:
* Supervisormodus ein
 clr.l    -(sp)
 gemdos   Super
 addq.w   #6,sp
 move.l   d0,-(sp)
 move.l   _sysbase,a0
 cmpi.w   #$0104,os_version(a0)
 scc.b    istos14
 bcs.b    ver_ok                   ; unter 1.4 immer installieren
 move.w   #'AK',-(sp)              ; auslesen
 gemdos   Sconfig
 addq.w   #4,sp
 tst.l    d0
 bmi.b    ver_ok
 gemdos   Pterm0                   ; SHELMAN fÅr KAOS 1.4 nicht nîtig
ver_ok:
* nachsehen, ob schon installiert
 move.l   $84,a0
 lea      -12(a0),a1               ; ggf. Zeiger auf XBRA
 cmpi.l   #'XBRA',(a1)+
 bne.b    inst
 cmpi.l   #'SHMN',(a1)
 beq      inst_err
* neuen Trap #1 installieren
inst:
 move.l   a0,xbra1+8
 move.l   #neu_trap1,$84
* neuen Trap #13 installieren
 move.l   $b4,xbra13+8
 move.l   #neu_trap13,$b4
* wieder in den Usermodus
 gemdos   Super
 addq.w   #6,sp
* Titelzeile
 pea      hallos(pc)
 gemdos   Cconws
 addq.w   #6,sp
* Environment freigeben
 move.l   _base+$2c(pc),-(sp)
 gemdos   Mfree
 addq.w   #6,sp
* Resident beenden
 clr.w    -(sp)
 move.l   #ende-_base,-(sp)
 gemdos   Ptermres


inst_err:
 pea      errs2(pc)
 gemdos   Cconws
 addq.w   #6,sp
 gemdos   Super
 addq.w   #6,sp
 move.w   #ERROR,-(sp)
 gemdos   Pterm

hallos:   DC.B CR,LF,LF,TAB,TAB,'SHELL- MANAGER V2.11 FöR KAOSDESK',CR,LF
          DC.B TAB,TAB,'Copyright (C) 1989-91 Andreas Kromke',CR,LF,LF,0
errs2:    DC.B 'Schon installiert',0

     END

