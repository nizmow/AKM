**********************************************************************
*
* BOOTFIX
*
* erstellt        17.09.90
* letzte Žnderung
*
* wird im AUTO- Ordner gestartet und jubelt dem Urprozež einen Parent
* unter, um den Fehler einiger Programme zu beheben, die durch
* Z„hlen ihrer Vorfahren feststellen, ob sie im AUTO- Ordner laufen.
*
* Ich stelle folgenden Test als Alternative zur Diskussion:
*
*    vq_aes:
*         move.w    #$c9,d0
*         trap      #2
*         tst.w     d0
*         seq.b     d0
*         ext.w     d0
*         rts
*
**********************************************************************


     INCLUDE "OSBIND.INC"

 move.w   #$c9,d0
 trap     #2
 tst.w    d0
 beq      err                      ; AES ist da!

 move.l   4(sp),a6                 ; Basepage
 move.l   $2c(a6),-(sp)            ; Environment
 gemdos   Mfree                    ;  freigeben
 addq.l   #6,sp

 move.l   $24(a6),d0
 beq.b    err                      ; habe keinen Parent!
 move.l   d0,a0                    ; a0 ist der Parent
 tst.l    $24(a0)                  ; Parent hat Parent
 bne.b    err                      ; ja, Fehler

* Wir haben genau einen Vorg„nger, der Parent hat keinen Parent

 lea      128(a6),a5               ; zweiten Teil der Basepage...
 move.l   a5,a1
 moveq    #32-1,d1
clr_loop:
 clr.l    (a1)+                    ; l”schen
 dbra     d1,clr_loop

 move.l   a5,$24(a0)               ; ...als Parent des Parent
 move.l   #$fffffefd,$30(a5)       ; Standardhandles...
 move.w   #$ffff,$34(a5)           ; ...auf Ger„te

 clr.w    -(sp)
 move.l   #256,-(sp)
 gemdos   Ptermres

err:
 move.w   #-1,-(sp)
 gemdos   Pterm

     END
