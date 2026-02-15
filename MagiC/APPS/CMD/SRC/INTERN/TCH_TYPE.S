TIME      SET  -4
DATE      SET  -2

touch_file:
 link     a6,#-4
 move.l   d7,-(sp)
 clr.w    d0
 move.l   8(a6),a0
 bsr      open
 move.w   d0,d7                    * only regular files, no directories
 bge.b    tf_2
* error
 lea      kann_s(pc),a0
 bsr      strcon
 move.l   8(a6),a0
 bsr      strcon
 lea      nicht_offn_s(pc),a0
 bsr      strcon
 bsr      inc_errlv
 moveq    #1,d0
 bra.b    tf_50
tf_2:
 move.l	d+switch_bits(pc),d0	* command line switches
 bne.b	tf_silent		* there is only one
 move.l   8(a6),a0
 bsr      strcon
 bsr      crlf_con
tf_silent:
 gemdos   Tgettime
 addq.l   #2,sp
 move.w   d0,TIME(a6)
 gemdos   Tgetdate
 addq.l   #2,sp
 move.w   d0,DATE(a6)
 move.w   #1,-(sp)
 move.w   d7,-(sp)
 pea      TIME(a6)
 gemdos   Fdatime
 adda.w   #$a,sp
 move.w   d7,d0
 bsr      close
tf_50:
 move.l   (sp)+,d7
 unlk     a6
 rts


type_file:
 link     a6,#0
 movem.l  d6/d7/a5/a4,-(sp)
 bsr      memavail
 move.l   #1024,d6            * maximal 1k reservieren
 tst.l    d0
 beq.b    tyf_3               * kein Speicher frei => Fehler provozieren
 cmp.l    d6,d0               * mehr als 1k frei ?
 bhi.b    tyf_3               * trotzdem nur 1k holen
 move.l   d0,d6               * Sonst gesamten Speicher holen
tyf_3:
 move.l   d6,d0
 bsr      alloc_tpa           * Z=1, wenn Fehler
 beq.b    tyf_12			* error
 move.l   d0,a4               * a4 = Pufferadresse

 move.l	d+switch_bits(pc),d0	* command line switches
 bne.b	tyf_silent		* there is only one
 lea      datei_s(pc),a0
 bsr      strcon
 move.l   8(a6),a0
 bsr      strcon
 bsr      crlf_con
 bsr      crlf_con
tyf_silent:
 clr.w    d0
 move.l   8(a6),a0
 bsr      open
 move.l   d0,d7
 bge.b    tyf_2
 lea      kann_s(pc),a0
 bsr      strcon
 move.l   8(a6),a0
 bsr      strcon
 lea      nicht_offn_s(pc),a0
 bsr      strcon
tyf_12:
 bsr      inc_errlv
 moveq    #1,d0
 bra.b    tyf_50
tyf_1:
 move.l   a4,a0
 move.l   d0,d1
 moveq    #STDOUT,d0
 bsr      write
tyf_2:
 move.l   a4,a0
 move.l   d6,d1
 move.w   d7,d0
 bsr      read
 tst.l    d0
 bgt.b    tyf_1
 move.w   d7,d0
 bsr      close
 clr.w    d0
tyf_50:
 bsr      free_tpa
 movem.l  (sp)+,a4/a5/d7/d6
 unlk     a6
 rts


*
* int touch_com(uint16_t argc, char *argv[], ...)
* int type_com(uint16_t argc, char *argv[], ...)
*
* Alle Parameter werden Åber den Stack Åbergeben!
* Die weiteren Parameter sind nur fÅr den Batch-Betrieb.
* Darf d0-d2/a0-a2 verÑndern.
*

*    DATA
parm_table_s:
 DC.B	's',0
 	EVEN
*	TEXT

touch_com:
 lea      syntax_touch(pc),a0
 lea      touch_file(pc),a1		; Funktionszeiger fÅr TOUCH
 bra.b    t_com

type_com:
 lea      syntax_type(pc),a0
 lea      type_file(pc),a1		; Funktionszeiger fÅr TYPE
t_com:
 movem.l  a5/a4,-(sp)			; verringert Stack um 8
 move.l	a0,a5
 move.l   a1,a4

 addq.l   #4,SP_ARGV+8(sp)		; skip argv[0]
 subq.w   #1,SP_ARGC+8(sp)

* evaluate switches

 lea		SP_ARGV+8(sp),a0
 lea		SP_ARGC+8(sp),a1
 lea		parm_table_s(pc),a2
 bsr		eval_switches
 bmi		toty_syntax
 tst.w   	SP_ARGC+8(sp)
 beq		toty_syntax			; no (remaining) arguments

* loop over all arguments

toty_loop:
 movea.l  SP_ARGV+8(sp),a0
 movea.l  (a0),a5
 move.l   a4,a1
 moveq    #6,d0
 move.l   a5,a0
 bsr      for_all
 bne.b    toty_2
* file not found: print error message
 bsr      crlf_con
 move.l   a5,a0
 bsr      strcon
 lea      not_founds(pc),a0
 bsr      strcon
toty_2:
 addq.l   #4,SP_ARGV+8(sp)
 subq.w   #1,SP_ARGC+8(sp)
 bne.b    toty_loop
 bra.b	toty_end

toty_syntax:
 move.l   a5,a0
 bsr      strcon
 bsr      inc_errlv
 
* end loop

toty_end:
 movem.l  (sp)+,a5/a4
 rts
