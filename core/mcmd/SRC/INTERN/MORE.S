more_com:
 movem.l  d7/a5,-(sp)
 lea      _base+$80(pc),a5
 clr.w    d7
 bra.b    more_1
more_2:
 move.b   (a5),d0
 bsr      putchar
 cmpi.b   #$1a,(a5)           * EOF (CTRL-Z)
 beq.b    more_end
 cmpi.b   #LF,(a5)
 bne.b    more_3
 subq.w   #1,d7				; n„chste Bildschirmzeile?
 bhi.b    more_3				; ja, weitermachen
* auf Eingabe warten
 lea      mehrs(pc),a0
 bsr      strcon
 clr.w    -(sp)                    ; Platz fr 2 Bytes schaffen
 lea      (sp),a0                  ; Puffer
 moveq    #1,d1                    ; 1 Byte lesen
 moveq    #-1,d0                   ; CON:
 bsr      read
 move.w   (sp)+,d7                 ; Zeichen holen
 cmpi.w   #$0300,d7                ; CTRL-C ?
 beq.b    more_end                 ;  ja => Abbruch
 cmpi.w   #$7100,d7				; 'q'?
 beq.b    more_end                 ;  ja => Abbruch
 cmpi.w   #$5100,d7				; 'Q'?
 beq.b    more_end                 ;  ja => Abbruch
 lea      dellines(pc),a0		; Eingabe vom Bildschirm l”schen
 bsr      strcon
 cmpi.w   #$2000,d7                ; Leertaste ?
 beq.b    more_1                   ;  ja => eine Seite weiter
 moveq    #1,d7                    ; sonst  eine Zeile weiter
 bra.b    more_3
more_1:
 moveq    #22,d7				; Anzahl Bildschirmzeilen
more_3:
 move.l   a5,a0				; Prozež-Kommandozeile als Puffer
 moveq    #1,d1				; lies ein Byte
 moveq    #STDIN,d0				; lies von stdin
 bsr      read
 subq.l   #1,d0
 beq.b    more_2
more_end:
 movem.l  (sp)+,a5/d7
 rts
