conterm           EQU $0484               ;CON-Attributes
bell_hook         EQU $05ac               ;Routine für Glocke

	xdef	call_vt_bel

;BEL, Klingelzeichen
call_vt_bel:  	 	btst     #2,conterm.w   ;Glocke an ?
                  beq.s    cursor_exit
                  move.l   bell_hook.w,-(sp)
cursor_exit:		rts
