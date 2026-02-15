*
* int echo_com(uint16_t argc, char *argv[], ...)
*
* Alle Parameter werden ber den Stack bergeben!
* Die weiteren Parameter sind nur fr den Batch-Betrieb.
* Darf d0-d2/a0-a2 ver„ndern.
*

*    DATA
parm_table_n:
 DC.B	'n',0
 	EVEN
*	TEXT

echo_com:
 addq.l   #4,SP_ARGV(sp)		; skip argv[0]
 subq.w   #1,SP_ARGC(sp)

* evaluate switches

 lea		SP_ARGV(sp),a0
 lea		SP_ARGC(sp),a1
 lea		parm_table_n(pc),a2
 bsr		eval_switches
 bmi		echo_syntax
 tst.w   	SP_ARGC(sp)
 beq		echo_print_on_off		; no (remaining) arguments

* check for ECHO on/off

 cmpi.w	#1,SP_ARGC(sp)			; exactly one argument
 bne.b	echo_loop				; no, normal operation
 movea.l  SP_ARGV(sp),a0			* argv[]
 move.l	(a0),a0				* argument
 bsr      is_off_on				; Parameter ist "ON" oder "OFF"?
 tst.w    d0
 bmi.b	echo_loop

* ECHO ON/OFF

 lea      is_echo(pc),a2			; ECHO on/off state
 sne      (a2)                     * ja => ECHO-Status merken
 bra		echo_print_on_off

* loop over remaining argument, text output

echo_loop:
 movea.l  SP_ARGV(sp),a0
 move.l   (a0),a0
 bsr      strstdout
 cmpi.w   #1,SP_ARGC(sp)
 beq.b    echo_was_last
 moveq    #' ',d0			* not last one
 bsr      putchar
echo_was_last:
 addq.l   #4,SP_ARGV(sp)
 subq.w   #1,SP_ARGC(sp)
 bne.b    echo_loop
 bra.b	echo_end

* end loop

* no arguments: tell ON/OFF status

echo_print_on_off:
 lea      echo_ists(pc),a0
 bsr      strstdout
 lea      ons(pc),a0               * "ON"
 move.b   is_echo(pc),d0			; ECHO on/off state
 bne.b    echo_1
 lea      offs(pc),a0              * "OFF"
echo_1:
 bsr      strstdout
 bra.b    echo_end

echo_syntax:
 lea		syntax_echo_s(pc),a0
 bsr      strcon
 bsr      inc_errlv

echo_end:
 move.l	d+switch_bits(pc),d0	; suppress CR/LF
 bne.b    echo_8				; yes
 bsr      crlf_stdout			; no, print CR/LF
echo_8:
 rts
