        section .text
        xdef    xp_vq_scrninfo
        xdef    xp_vq_color
        xdef    xp_wmul
        xdef    xp_ldiv
        xref    _vq_scrninfo
        xref    _vq_color
        xref    W_mul_L
        xref    L_div_W

xp_vq_scrninfo:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        movea.l 16(sp),a1
        jsr     _vq_scrninfo
        movem.l (sp)+,d2/a2
        rts

xp_vq_color:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        move.w  18(sp),d0
        move.w  22(sp),d1
        jsr     _vq_color
        movem.l (sp)+,d2/a2
        rts

xp_wmul:
        move.w  6(sp),d0
        move.w  10(sp),d1
        jsr     W_mul_L
        rts

xp_ldiv:
        move.l  4(sp),d0
        move.w  10(sp),d1
        jsr     L_div_W
        rts

