if_com:
 link     a6,#-$80
 movem.l  d6/d7/a4/a5,-(sp)
 bsr      sav_errlv
 sf.b     d7				* d7.b = 0x00 (kein NOT)
 move.l   ARGV(a6),a5
if_nxtparm:
 addq.l   #4,a5               * nÑchster Parameter
 subq.w   #1,ARGC(a6)
 bne.b    i1
	; Syntaxfehler
 lea      syntax_ifs(pc),a0
 bsr      strcon
 bsr      inc_errlv
 bra      i100
	; weiter
i1:
 lea      not_s(pc),a1			* "NOT"
 move.l   (a5),a0
 bsr      upper_strcmp
 bne      i2
 st.b     d7					* d7.b = 0xff
 bra.b    if_nxtparm
i2:
 lea      errorlevel_s(pc),a1		* "ERRORLEVEL"
 move.l   (a5),a0
 bsr      upper_strcmp
 bne.b    i5
	; ERRORLEVEL
 cmpi.w   #3,ARGC(a6)
 blt.b    i5					* nur noch ein weiterer Parameter
	; ERRORLEVEL n
 move.l   4(a5),a0
 bsr      str_toi				* n
 cmp.w    errorlevel(pc),d0
 sls.b    d6					* n <= errorlevel => d6.b = 0xff
i3:
 addq.l   #4,a5
 subq.w   #1,ARGC(a6)
	; Hier springt auch das Kommando FOR rein.
	; zusÑtzliche Parameter:
	; d6.b = 0xff (Bedingung erfÅllt) oder 0x00 (nicht erfÅllt)
	; d7.b = 0xff (NOT) oder 0x00
exe_restzeile:
 eor.b    d7,d6          * d6 umdrehen, falls d7 = 0xff, sonst lassen
 beq.b    i4            * Damit ist d7 die Operation NOT auf 0 und 0xff
 move.l   a5,a1
 move.w   ARGC(a6),d0
 lea      -$80(a6),a0
 bsr      cat_strings
 move.l   PARGV(a6),-(sp)
 move.l   PARGC(a6),-(sp)
 pea      -$80(a6)
 move.w   BATCH(a6),-(sp)
 bsr      cmdline_exec        * Rest der Kommandozeile ausfÅhren
 adda.w   #14,sp
i4:
 bra.b    i100
i5:
 lea      exist_s(pc),a1			* "EXIST"
 move.l   (a5),a0
 bsr      upper_strcmp
 bne.b    i7
	; EXIST
 cmpi.w   #3,ARGC(a6)
 blt.b    i7					* zuwenige Argumente

 clr.l    -(sp)			* Attribute lesen, plus Dummy-Parameter
 move.l   4(a5),-(sp)		* Pfad
 gemdos   Fattrib
 adda.w   #$a,sp
 tst.l    d0				* Fehler
 sge.b    d6                  * d6 = 0xff, wenn RÅckgabewert >= 0
 bra.b    i3             * Weiter wie bei "errorlevel"
i7:
 cmpi.w   #4,ARGC(a6)
 blt.b    i100					* Fehlermeldung unterdrÅcken
 lea      equal_s(pc),a1			* "=="
 move.l   4(a5),a0
 bsr      upper_strcmp
 bne.b    i100           * Fehlermeldung unterdrÅcken
 subq.w   #1,ARGC(a6)    * Ausdruck "string1 == string2"
 move.l   8(a5),a1
 move.l   (a5)+,a0
 bsr      upper_strcmp
 seq      d6
 bra      i3
i100:
 movem.l  (sp)+,a4/a5/d7/d6
 unlk     a6
 rts
