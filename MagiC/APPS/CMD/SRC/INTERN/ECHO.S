*
* int echo_com(uint16_t argc, char *argv[], ...)
*
* Alle Parameter werden ber den Stack bergeben!
* Die weiteren Parameter sind nur fr den Batch-Betrieb.
* Darf d0-d2/a0-a2 ver„ndern.
*
echo_com:
 movem.l  d7/a5,-(sp)			; stack minus 8
 sf       d7					; default: do not suppress CR/LF
echo_7:
 lea      is_echo(pc),a5
 cmpi.w   #2,SP_ARGC+8(sp)		* Anzahl Parameter
 bge.b    echo_2				* mindestens zwei
* argc ist 1, also keine Argumente
 lea      echo_ists(pc),a0
 bsr      strstdout
 lea      ons(pc),a0               * "ON"
 tst.b    (a5)
 bne.b    echo_1
 lea      offs(pc),a0              * "OFF"
echo_1:
 bsr      strstdout
 bra.b    echo_6
* argc ist mindestens 2, d.h. es gibt Argumente
echo_2:
 movea.l  SP_ARGV+8(sp),a0		* argv[]
 move.l   4(a0),a1				* argv[1] ist das erste Argument
 cmpi.b   #'-',(a1)+			* beginnt mit "-"?
 bne.b    echo_5				* nein
 move.b   (a1)+,d0				* Zeichen nach dem "-"
 bsr      d0_upper				* in Grožbuchstaben
 cmpi.b   #'N',d0				* "N" oder "n"?
 bne.b    echo_5				* nein
 tst.b    (a1)					* "-nquark"?
 bne.b    echo_5				* ja, Parameter nicht als Schalter
* Erstes Argument: "-n" : CR/LF unterdrcken
 addq.l   #4,SP_ARGV+8(sp)
 subq.w   #1,SP_ARGC+8(sp)
 st       d7					; suppress CR/LF
 bra.b    echo_7				; fange von vorn an
echo_5:
 move.l   4(a0),a0
 bsr      is_off_on				; Parameter ist "ON" oder "OFF"?
 tst.w    d0
 bmi.b    echo_4                   * nein => Text ausgeben
 sne      (a5)                     * ja => ECHO-Status merken
 bra.b    echo_6

* loop over all arguments

echo_loop:
 addq.l   #4,SP_ARGV+8(sp)
 movea.l  SP_ARGV+8(sp),a0
 move.l   (a0),a0
 bsr      strstdout
 cmpi.w   #1,SP_ARGC+8(sp)
 beq.b    echo_4
 moveq    #' ',d0
 bsr      putchar
echo_4:
 subq.w   #1,SP_ARGC+8(sp)
 bne.b    echo_loop

* end loop

echo_6:
 tst.b    d7				; suppress CR/LF
 bne.b    echo_8			; yes
 bsr      crlf_stdout		; no, print CR/LF
echo_8:
 movem.l  (sp)+,a5/d7
 rts
