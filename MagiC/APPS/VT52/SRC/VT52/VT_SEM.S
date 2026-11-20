	xdef	inc_sem

;Semaphore wenn möglich setzen und ihren Status zurückgeben
;Funktionsresultat:	0: Semaphore ist bereits gesetzt gewesen
;							1: Semaphore wurde gesetzt
;
;WORD	inc_sem( WORD *sem )
inc_sem:		bset		#0,1(a0)
				seq		d0
				ext.w		d0
				rts
