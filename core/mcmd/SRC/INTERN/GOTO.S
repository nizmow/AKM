*
* GOTO label
* argc muž 2 sein
*
* lokale Variablen:
*  -90(a6) : Zeilenpuffer, 90 Zeichen
*

goto_com:
 link     a6,#-130
 movem.l  a5/a4,-(sp)
 bsr      sav_errlv
	; Fehlerbehandlung bei Parametern
 move.w   BATCH(a6),d0        * Batch-Modus?
 beq.b    goto_end            * interaktiver Modus -> GOTO ignorieren
 cmpi.w   #2,ARGC(a6)
 blt.b    goto_lbl_not_found
 lea      -130(a6),a5          * a5 = Zeilenpuffer
	; Lesezeiger zum Anfang der Batch-Datei
 clr.w    -(sp)
 move.w   d0,-(sp)            * Handle der Batchdatei
 clr.l    -(sp)               * Dateianfang
 gemdos   Fseek
 adda.w   #$a,sp
 bra.b    goto_nxtline
goto_loop:
 move.l   a5,a0
 bsr      skip_sep
 move.l   d0,a4
 cmpi.b   #':',(a4)+
 bne.b    goto_nxtline
 move.l   a4,a0
 bsr      search_sep
 clr.b    (a0)
 movea.l  ARGV(a6),a0
 move.l   4(a0),a1
 move.l   a4,a0
 bsr      upper_strcmp   * Ist es das richtige Label ?
 beq.b    goto_end         * Gefunden !!
goto_nxtline:
 move.l   a5,a0
 move.w   BATCH(a6),d0   * Batch- Handle
 bsr      read_str
 tst.w    d0
 beq.b    goto_loop
goto_lbl_not_found:
 lea      lbl_not_found_s(pc),a0
 bsr      strcon
goto_end:
 movem.l  (sp)+,a5/a4
 unlk     a6
 rts
