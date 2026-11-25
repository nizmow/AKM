        section .text
        xdef    legacy_xp0
        xdef    legacy_load0
        xdef    legacy_load1
        xdef    legacy_load2
        xdef    legacy_load3
        xdef    legacy_raster

legacy_xp0:
        movem.l d2/a2-a3,-(sp)
        movea.l 16(sp),a2
        movea.l 20(sp),a0
        movea.l 24(sp),a1
        movea.l 28(sp),a3
        move.l  a3,-(sp)
        move.l  a1,-(sp)
        move.w  #4,-(sp)
        clr.l   -(sp)
        move.l  a0,-(sp)
        jsr     (a2)
        lea     18(sp),sp
        movem.l (sp)+,d2/a2-a3
        rts

legacy_load0:
        movem.l d2/a2-a3,-(sp)
        movea.l 16(sp),a2
        movea.l 20(sp),a0
        movea.l 24(sp),a1
        movea.l 28(sp),a3
        move.l  a3,-(sp)
        move.l  a1,-(sp)
        move.w  #4,-(sp)
        clr.l   -(sp)
        move.l  a0,-(sp)
        jsr     (a2)
        lea     18(sp),sp
        movem.l (sp)+,d2/a2-a3
        rts

legacy_load1:
        movem.l d2/a2-a3,-(sp)
        movea.l 16(sp),a2
        movea.l 20(sp),a0
        movea.l 24(sp),a1
        move.l  a1,-(sp)
        move.w  #2,-(sp)
        move.l  #1,-(sp)
        move.l  a0,-(sp)
        jsr     (a2)
        lea     14(sp),sp
        movem.l (sp)+,d2/a2-a3
        rts
legacy_load2:
        movem.l d2-d4/a2-a3,-(sp)
        movea.l 24(sp),a2
        movea.l 28(sp),a0
        move.l  32(sp),d0
        move.l  36(sp),d1
        move.w  42(sp),d2
        move.w  46(sp),d3
        move.w  50(sp),d4
        move.w  d4,-(sp)
        move.w  d3,-(sp)
        move.w  d2,-(sp)
        move.l  d1,-(sp)
        move.l  d0,-(sp)
        move.w  #7,-(sp)
        move.l  #2,-(sp)
        move.l  a0,-(sp)
        jsr     (a2)
        lea     24(sp),sp
        movem.l (sp)+,d2-d4/a2-a3
        rts

legacy_load3:
        movem.l d2-d4/a2-a3,-(sp)
        movea.l 24(sp),a2
        movea.l 28(sp),a0
        move.w  34(sp),d0
        move.l  36(sp),d1
        move.w  42(sp),d2
        move.w  46(sp),d3
        move.w  d3,-(sp)
        move.w  d2,-(sp)
        move.l  d1,-(sp)
        move.w  d0,-(sp)
        move.w  #5,-(sp)
        move.l  #3,-(sp)
        move.l  a0,-(sp)
        jsr     (a2)
        lea     20(sp),sp
        movem.l (sp)+,d2-d4/a2-a3
        rts

legacy_raster:
        movem.l d2-d4/a2-a3,-(sp)
        movea.l 24(sp),a2
        move.l  28(sp),d0
        move.l  32(sp),d1
        move.w  38(sp),d2
        movea.l 40(sp),a0
        movea.l 44(sp),a1
        move.l  a1,-(sp)
        move.l  a0,-(sp)
        move.w  d2,-(sp)
        move.l  d1,-(sp)
        move.l  d0,-(sp)
        jsr     (a2)
        lea     18(sp),sp
        movem.l (sp)+,d2-d4/a2-a3
        rts
