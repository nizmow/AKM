	xdef	vt_vdi

;void	vt_vdi( VDIPB *pb )
vt_vdi:	move.l	a2,-(sp)
			move.l	8(sp),d1
			moveq		#115,d0
			trap		#2
			move.l	(sp)+,a2
			rts