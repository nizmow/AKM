CON					EQU	2
RAW					EQU	5

	xref	app_window, pact_appl, vt_jmp, rawcon_jmp, stack_offset, vt_Bconstat, vt_Bconin, vt_Kbshift
	xdef	bios_disp, old_bios_vec
	xdef	old_xconstat, old_xconin, old_xconout_con, old_xconout_raw, old_xcostat_con, old_xcostat_raw
	xdef	xconstat, xconin, xconout_con, xconout_raw, xcostat_con, xcostat_raw
	xref	vt_app_Sconout, vt_app_Cconin, vt_getVDIESC, vt_inherit, vt_uninherit
	xdef	vt_sout_entry, vt_cin_entry, vt_geometry_entry, vt_inherit_entry, vt_uninherit_entry

                  DC.L 'XBRA'
                  DC.L 'VT52'
old_bios_vec:     DC.L 0
bios_disp:        move     usp,a0
                  btst     #5,(sp)        ; S bit in the high byte of saved SR
                  beq.s    bios_user
                  movea.l  sp,a0
                  adda.w   stack_offset,a0 ;Stack-Offset (6 bzw. 8 Bytes)
bios_user:        move.w	(a0)+,d0
						cmp.w		#11,d0
						bhi.s		old_bios
						add.w		d0,d0
						move.w	bios_tab(pc,d0.w),d0
						jmp		bios_tab(pc,d0.w)

bios_tab:			dc.w		old_bios-bios_tab	;0
						dc.w		Bconstat-bios_tab	;1
						dc.w		Bconin-bios_tab	;2
						dc.w		Bconout-bios_tab	;3
						dc.w		old_bios-bios_tab	;4
						dc.w		old_bios-bios_tab	;5
						dc.w		old_bios-bios_tab	;6
						dc.w		old_bios-bios_tab	;7
						dc.w		Bcostat-bios_tab	;8
						dc.w		old_bios-bios_tab	;9
						dc.w		old_bios-bios_tab	;10
						dc.w		Kbshift-bios_tab	;11

old_bios:         movea.l  old_bios_vec(pc),a0
                  jmp      (a0)

;WORD	Bconstat( WORD dev );
Bconstat:			cmp.w		#CON,(a0)		;Tastatur?
						bne.s		old_bios
						move.l	pact_appl,a0		;Adresse des des Zeigers auf die aktuelle Applikationsstruktur
						move.l	(a0),d1			;Zeiger auf aktuelle Applikationsstruktur oder NULL
						beq.s		old_bios			;NULL: keine Task aktiv
						move.l	d1,a0
						move.w	4(a0),d1			;AES-ID des aktuellen Prozesses
						add.w		d1,d1
						add.w		d1,d1
						lea		app_window,a0
						move.l	0(a0,d1.w),d1	;Zeiger auf die Fensterstruktur für den Proze× oder NULL
						beq.s		old_bios
						move.l	d1,a0				;Zeiger auf die Fensterstruktur
					
						move.l	a0,-(sp)
						jsr		vt_Bconstat		;C-Routine aufrufen
						addq.l	#4,sp
						rte

;LONG	Bconin( WORD dev );
Bconin:				cmp.w		#CON,(a0)		;Tastatur?
						bne.s		old_bios
						move.l	pact_appl,a0		;Adresse des des Zeigers auf die aktuelle Applikationsstruktur
						move.l	(a0),d1			;Zeiger auf aktuelle Applikationsstruktur oder NULL
						beq.s		old_bios			;NULL: keine Task aktiv
						move.l	d1,a0
						move.w	4(a0),d1			;AES-ID des aktuellen Prozesses
						add.w		d1,d1
						add.w		d1,d1
						lea		app_window,a0
						move.l	0(a0,d1.w),d1	;Zeiger auf die Fensterstruktur für den Proze× oder NULL
						beq.s		old_bios
						move.l	d1,a0				;Zeiger auf die Fensterstruktur
					
						move.l	a0,-(sp)
						jsr		vt_Bconin		;C-Routine Aufrufen
						addq.l	#4,sp
						rte
						
;void	Bconout( WORD dev, WORD c );
Bconout:				move.w	(a0)+,d1			;Bios-Device
						move.w	(a0)+,d0			
						and.l		#$ff,d0			;Zeichen
						subq.w	#CON,d1
						bne.s		Bconout_raw

						move.l	pact_appl,a0		;Adresse des des Zeigers auf die aktuelle Applikationsstruktur
						move.l	(a0),d1			;Zeiger auf aktuelle Applikationsstruktur oder NULL
						beq		old_bios			;NULL: keine Task aktiv
						move.l	d1,a0
						move.w	4(a0),d1			;AES-ID des aktuellen Prozesses
						add.w		d1,d1
						add.w		d1,d1
						lea		app_window,a0
						move.l	0(a0,d1.w),d1	;Zeiger auf die Fensterstruktur für den Proze× oder NULL
						beq		old_bios
						move.l	d1,a0				;Zeiger auf die Fensterstruktur

												move.l	d0,-(sp)
						move.l	a0,-(sp)
						jsr		vt_jmp
						addq.l	#8,sp
						rte

Bconout_raw:		subq.w	#RAW-CON,d1
						bne		old_bios

						move.l	pact_appl,a0		;Adresse des des Zeigers auf die aktuelle Applikationsstruktur
						move.l	(a0),d1			;Zeiger auf aktuelle Applikationsstruktur oder NULL
						beq		old_bios			;NULL: keine Task aktiv
						move.l	d1,a0
						move.w	4(a0),d1			;AES-ID des aktuellen Prozesses
						add.w		d1,d1
						add.w		d1,d1
						lea		app_window,a0
						move.l	0(a0,d1.w),d1	;Zeiger auf die Fensterstruktur für den Proze× oder NULL
						beq		old_bios
						move.l	d1,a0				;Zeiger auf die Fensterstruktur

												move.l	d0,-(sp)
						move.l	a0,-(sp)
						jsr		rawcon_jmp
						addq.l	#8,sp
						rte

;LONG Bcostat( WORD dev );
Bcostat:				move.w	(a0),d0
						subq.w	#CON,d0
						beq		Bcostat_con
						subq.w	#RAW-CON,d0
						bne		old_bios
Bcostat_con:		moveq		#-1,d0			;Zeichen ist ausgegeben worden
						rte

;LONG	Kbshift( WORD mode );
Kbshift:				move.l	pact_appl,a0		;Adresse des des Zeigers auf die aktuelle Applikationsstruktur
						move.l	(a0),d1			;Zeiger auf aktuelle Applikationsstruktur oder NULL
						beq		old_bios			;NULL: keine Task aktiv
						move.l	d1,a0
						move.w	4(a0),d1			;AES-ID des aktuellen Prozesses
						add.w		d1,d1
						add.w		d1,d1
						lea		app_window,a0
						move.l	0(a0,d1.w),d1	;Zeiger auf die Fensterstruktur für den Proze× oder NULL
						beq		old_bios
						move.l	d1,a0				;Zeiger auf die Fensterstruktur

						move.l	a0,-(sp)
						jsr		vt_Kbshift
						addq.l	#4,sp
						rte

                  DC.L 'XBRA'
                  DC.L 'VT52'
old_xconstat:		DC.L 0
xconstat:			bsr		get_struct
						move.l	a0,d0
						beq.s		call_xconstat
						move.l	a0,-(sp)
					jsr		vt_Bconstat		;C-Routine aufrufen
					addq.l	#4,sp
					rts
call_xconstat:		move.l	old_xconstat(pc),a0
						jmp		(a0)
                  
                  DC.L 'XBRA'
                  DC.L 'VT52'
old_xconin:			DC.L 0
xconin:				bsr		get_struct
						move.l	a0,d0
						beq.s		call_xconin
						move.l	a0,-(sp)
					jsr		vt_Bconin		;C-Routine Aufrufen
					addq.l	#4,sp
					rts
call_xconin:		move.l	old_xconin(pc),a0
						jmp		(a0)
						
                  DC.L 'XBRA'
                  DC.L 'VT52'
old_xconout_con:	DC.L 0
xconout_con:		bsr		get_struct
						move.l	a0,d0
						beq.s		call_xconout_con
						move.w	6(sp),d0
						and.l		#$ff,d0
						move.l	d0,-(sp)
						move.l	a0,-(sp)
						jsr		vt_jmp
						addq.l	#8,sp
						rts
call_xconout_con:	move.l	old_xconout_con(pc),a0
						jmp		(a0)

                  DC.L 'XBRA'
                  DC.L 'VT52'
old_xconout_raw:	DC.L 0
xconout_raw:		bsr		get_struct
						move.l	a0,d0
						beq.s		call_xconout_raw
						move.w	6(sp),d0
						and.l		#$ff,d0
						move.l	d0,-(sp)
						move.l	a0,-(sp)
						jsr		rawcon_jmp
						addq.l	#8,sp
						rts
call_xconout_raw:	move.l	old_xconout_raw(pc),a0
						jmp		(a0)

                  DC.L 'XBRA'
                  DC.L 'VT52'
old_xcostat_con:	DC.L 0
xcostat_con:		bsr		get_struct
						move.l	a0,d0
						beq.s		call_xcostat_con
						moveq		#-1,d0			;Zeichen ist ausgegeben worden						
						rts
call_xcostat_con:	move.l	old_xcostat_con(pc),a0
						jmp		(a0)

                  DC.L 'XBRA'
                  DC.L 'VT52'
old_xcostat_raw:	DC.L 0
xcostat_raw:		bsr		get_struct
						move.l	a0,d0
						beq.s		call_xcostat_raw
						moveq		#-1,d0			;Zeichen ist ausgegeben worden
						rts
call_xcostat_raw:	move.l	old_xcostat_raw(pc),a0
						jmp		(a0)

;Zeiger auf die Fensterstruktur oder NULL zurückliefern
;Vorgaben:
;Register d0 und a0 werden verändert
;Eingaben:
;-
;Ausgaben:
;a0.l Zeiger auf die Fensterstruktur oder NULL
get_struct:			move.l	pact_appl,a0		;Adresse des des Zeigers auf die aktuelle Applikationsstruktur
						move.l	(a0),a0			;Zeiger auf aktuelle Applikationsstruktur oder NULL
						move.l	a0,d0
						beq.s		get_struct_exit ;NULL: keine Task aktiv
						move.w	4(a0),d0			;AES-ID des aktuellen Prozesses
						add.w		d0,d0
						add.w		d0,d0
						lea		app_window,a0
						move.l	0(a0,d0.w),a0	;Zeiger auf die Fensterstruktur für den Proze× oder NULL
get_struct_exit:	rts


; VTSYS calls by MagiC DOS/AES: registers in, D0 result. GCC expects
; 4-byte stack slots, including for the 16-bit application IDs.
vt_sout_entry:	move.l	d0,-(sp)
					move.l	a1,-(sp)
					move.l	a0,-(sp)
					jsr		vt_app_Sconout
					lea		12(sp),sp
					rts
vt_cin_entry:	move.l	a0,-(sp)
					jsr		vt_app_Cconin
					addq.l	#4,sp
					rts
vt_geometry_entry:	move.l	a0,-(sp)
					jsr		vt_getVDIESC
					addq.l	#4,sp
					rts
vt_inherit_entry:	move.l	d1,-(sp)
					move.l	d0,-(sp)
					jsr		vt_inherit
					addq.l	#8,sp
					rts
vt_uninherit_entry:	move.l	d0,-(sp)
					jsr		vt_uninherit
					addq.l	#4,sp
					rts
