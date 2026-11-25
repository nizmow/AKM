	xdef	inc_sem

;Semaphore wenn möglich setzen und ihren Status zurückgeben
;Funktionsresultat:	0: Semaphore ist bereits gesetzt gewesen
;							1: Semaphore wurde gesetzt
;
;WORD	inc_sem( WORD *sem )
inc_sem:		move.l	4(sp),a0
				bset		#0,1(a0)
				seq		d0
				ext.l		d0
				rts
