        section .text
        xdef    loadimg_call_raster

; WORD loadimg_call_raster(fn, words, len, planes, src, dst)
; xp_rasterC expects an 18-byte stack record and may clobber D2.
loadimg_call_raster:
        move.l  d2,-(sp)
        movea.l sp,a1
        movea.l 8(a1),a0
        move.l  28(a1),-(sp)
        move.l  24(a1),-(sp)
        move.w  22(a1),-(sp)
        move.l  16(a1),-(sp)
        move.l  12(a1),-(sp)
        jsr     (a0)
        lea     18(sp),sp
        move.l  (sp)+,d2
        rts
