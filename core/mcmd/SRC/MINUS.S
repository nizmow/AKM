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
*        SR Z and N bits
*        also stored to d+switch_bits(pc)
*    (a0) changed
*    (a1) changed
*

eval_switches_plus:
 moveq	#'+',d0
 bra.b	_eval_switches

eval_switches:
 moveq	#'-',d0			; switch character
_eval_switches:
 movem.l	d3/d4/a3-a6,-(sp)
 move.l	a0,a6			; a6 is ptr to ptr table
 move.l	a1,a5			; a1 is ptr to argc
 move.l	a2,a4			; a4 is switch table
 moveq	#0,d3			; no switches found, yet
 move.l	d0,d4			; switch character

 tst.w   	(a5)				; at least one parameter?
 beq.b    evw_end			; no switches
 move.l	(a6),a0			; argv[]
 move.l   (a0),a0			; first argument string
 cmpi.b	#'\',(a0)
 bne.b	evw_no_bsl
 cmp.b    1(a0),d4			; switch character?
 bne.b    evw_no_bsl_minus
* first argument starts with "\-" -> remove "\" and stop evaluation
evw_remv_bsl_loop:
 move.b   1(a0),(a0)+
 bne.b    evw_remv_bsl_loop
 bra.b	evw_end			; stop evaluation
evw_no_bsl_minus:
 cmpi.b   #'\',1(a0)		; "\\"
 bne.b	evw_no_bsl
 cmp.b    2(a0),d4			; "\\-"
* first argument starts with "\\-" -> remove one "\" and stop evaluation
 beq.b    evw_remv_bsl_loop
evw_no_bsl:
 move.l	a0,a2

* switch loop. a0 points to current argument

evw_arg_loop:
 cmp.b    (a2)+,d4			; '-' (switch character)?
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
 lea		d+switch_bits(pc),a0
 move.l	d0,(a0)				; store to global variable
 movem.l  (sp)+,d3/d4/a3-a6
 rts
