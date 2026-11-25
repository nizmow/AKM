DTA       SET  8
DATEINAME SET  4

* int attr(char *dateiname, DTA *)
* char *dateiname;
*  Setzt/Lîscht oder zeigt Attribute an

attr:
 lea      d+attrplus(pc),a0
 lea      d+attrminus(pc),a1
 move.l   DTA(sp),a2
 clr.w    d2
 move.b   $15(a2),d2          * Attribut
 move.b   (a0),d0
 or.b     (a1),d0
 beq.b    attr_10             * nur anzeigen
 or.b     (a0),d2             * Bits setzen
 move.b   (a1),d0
 not.b    d0
 and.b    d0,d2               * Bits lîschen
 cmp.b    $15(a2),d2          * hat sich Attribut geÑndert
 beq      attr_99             * nein => Ende
 move.w   d2,-(sp)
 move.w   #1,-(sp)
 move.l   DATEINAME+2+2(a7),-(sp)
 gemdos   Fattrib
 adda.w   #$a,sp
 move.l   d0,d2
 bge.b    attr_10
 bsr      print_err
 lea      auf_s(pc),a0
 bsr      strcon
 bra.b    attr_20
attr_10:
 move.w   d2,d0
 bsr      print_attr
attr_20:
 move.l   DATEINAME(sp),a0
 bsr      strstdout
 bsr      crlf_stdout
attr_99:
 clr.w    d0
 rts


*
* int attrib_com(uint16_t argc, char *argv[], ...)
*
* Alle Parameter werden Åber den Stack Åbergeben!
* Die weiteren Parameter sind nur fÅr den Batch-Betrieb.
* Darf d0-d2/a0-a2 verÑndern.
*

*    DATA
parm_table_rhsa:
 * use 8 (backspace) as dummy, to make sure 'a' will set bit 5, not 3
 DC.B	'rhs',8,8,'a',0		; bit 0:R, 1:H, 2:S, 5:A
* +DEBUG
	IF 0
gleich:
 DC.B	'=',0
* -DEBUG
	ENDIF
 	EVEN
*	TEXT

SPO	SET	12

attrib_com:
 movem.l	d3-d5,-(sp)			; add 12 to sp

 addq.l   #4,SP_ARGV+SPO(sp)		; skip argv[0]
 subq.w   #1,SP_ARGC+SPO(sp)
 moveq	#0,d3				; no "-" switches, yet
 moveq	#0,d4				; no "+" switches, yet

attr_sw_loop:

 move.w	SP_ARGC+SPO(sp),d5		; current argc

* evaluate '-' switches

 lea		SP_ARGV+SPO(sp),a0
 lea		SP_ARGC+SPO(sp),a1
 lea		parm_table_rhsa(pc),a2
 bsr		eval_switches
 bmi		attr_syntax
 or.l	d0,d3				; d3: add "minus" switches

* evaluate '+' switches

 lea		SP_ARGV+SPO(sp),a0
 lea		SP_ARGC+SPO(sp),a1
 lea		parm_table_rhsa(pc),a2
 bsr		eval_switches_plus
 bmi		attr_syntax
 or.l	d0,d4				; d4: add "plus" switches

 cmp.w	SP_ARGC+SPO(sp),d5		; was argc changed?
 bne.b	attr_sw_loop			; yes, look for more switches
 tst.w   	SP_ARGC+SPO(sp)
 beq		attr_syntax			; no (remaining) arguments
* store attrib switches to global variables
 lea      d+attrplus(pc),a0
 move.b   d4,(a0)
 move.b   d3,attrminus-attrplus(a0)

* loop over all remaining arguments, i.e. files

attr_loop:

* +DEBUG
	IF 0
 lea		gleich(pc),a0
 bsr      strcon
 moveq	#0,d0
 move.w	SP_ARGC+SPO(sp),d0
 bsr		lwrite_long
 lea		gleich(pc),a0
 bsr      strcon
 movea.l  SP_ARGV+SPO(sp),a0
 movea.l  (a0),a0        * Dateiname(nsmuster)
 bsr      strcon
 lea		gleich(pc),a0
 bsr      strcon
 	ENDIF
* -DEBUG

* prepare parameters for executing function
 lea      attr(pc),a1    * auszufÅhrende Routine
 moveq    #6,d0          * Dateityp = alle auûer Subdir
 movea.l  SP_ARGV+SPO(sp),a0
 movea.l  (a0),a0        * Dateiname(nsmuster)
 bsr      for_all
 bne.b    attr_found	* no problem
* "file not found"
 movea.l  SP_ARGV+SPO(sp),a0
 movea.l  (a0),a0        * Dateiname(nsmuster)
 bsr      strcon
 lea      not_founds(pc),a0
 bsr      strcon
 bsr      inc_errlv
attr_found:
 addq.l   #4,SP_ARGV+SPO(sp)
 subq.w   #1,SP_ARGC+SPO(sp)
 bgt.b    attr_loop
 bra.b    attr_end

* end loop

* print syntax, end command

attr_syntax:
 lea      syntaxattrs(pc),a0
 bsr      strcon
 bsr      inc_errlv
attr_end:
 movem.l  (sp)+,d3-d5
 rts

