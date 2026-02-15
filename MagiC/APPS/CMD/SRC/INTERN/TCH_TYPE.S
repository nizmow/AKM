*
* Find character in a list of valid switches, return its position or -1, sets SR flags
*
* input:
*    a0: table
*    d0: character to find
* output:
*    d0: bitmask or -1 on error
*    Z Flag
*
* changes a1
*

find_switch:
 move.l	a0,a1
fnd_sw_loop:
 tst.b	(a1)
 beq.b	fnd_sw_none		; not found, end of table
 cmp.b	(a1),d0			; match?
 beq.b	fnd_sw_end		; yes!
 addq.l	#1,a1			; next character
 bra.b	fnd_sw_loop
fnd_sw_end:
 suba.l	a1,a0			; match, get index
 move.l	a0,d0
 rts
fnd_sw_none:
 moveq.l	#-1,d0
 rts


*
* Evaluate command line switches of type -x or -xyz..
* De-escape escape ones like \-... or \\-...
*
* input:
*    a0: ptr to ptr to first argument, i.e. argv[1]
*    a1: ptr to number of remaining arguments
*	a2: ptr to string of valid switches
* output:
* 	d0: bit vector of found switches, -1 for error
*    (a0) changed
*    (a1) changed
*

eval_switches:
 movem.l	d3/a3-a6,-(sp)
 move.l	a0,a6			; a6 is ptr to ptr table
 move.l	a1,a5			; a1 is ptr to argc
 move.l	a2,a4			; a4 is switch table
 moveq	#0,d3			; no switches found, yet

 tst.w   	(a5)				; at least one parameter?
 beq.b    evw_end			; no switches
 move.l	(a6),a0			; argv[]
 move.l   (a0),a0			; first argument string
 cmpi.b	#'\',(a0)
 bne.b	evw_no_bsl
 cmpi.b   #'-',1(a0)
 bne.b    evw_no_bsl_minus
* first argument starts with "\-" -> remove "\" and stop evaluation
evw_remv_bsl_loop:
 move.b   1(a0),(a0)+
 bne.b    evw_remv_bsl_loop
 bra.b	evw_end			; stop evaluation
evw_no_bsl_minus:
 cmpi.b   #'\',1(a0)		; "\\"
 bne.b	evw_no_bsl
 cmpi.b   #'-',2(a0)		; "\\-"
* first argument starts with "\\-" -> remove one "\" and stop evaluation
 beq.b    evw_remv_bsl_loop
evw_no_bsl:
 move.l	a0,a2

* switch loop. a0 points to current argument

evw_arg_loop:
 cmpi.b   #'-',(a2)+		; "-"
 bne.b	evw_end			; no switch
evw_chr_loop:
 move.b	(a2)+,d0
 beq.b	evw_eos			; end of argument
 move.l	a4,a0
 bsr		find_switch
 bmi.b	evw_error			; invalid switch
 bset	d0,d3
 bra.b	evw_chr_loop
evw_eos:
* advance to next argv
 addq.l	#4,(a6)
 subq.w	#1,(a5)
 beq.b	evw_end			; no more aguments
 move.l	(a6),a2
 move.l	(a2),a2
 bra.b	evw_arg_loop		; next argument

evw_error:
 moveq	#-1,d3
evw_end:
 move.l	d3,d0
 movem.l  (sp)+,d3/a3-a6
 rts


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
 lea		d+switch_bits(pc),a0
 move.l	d0,(a0)				; store to global variable
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
