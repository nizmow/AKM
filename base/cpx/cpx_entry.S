        section .cpx_entry,code
        xdef    cpx_entry
        xref    cpx_init
cpx_entry:
        jmp     cpx_init

        section .text
        xdef    cpx_xform_do
        xdef    cpx_rsh_obfix
        xdef    cpx_xgen_alert
        xdef    cpx_call2
        xdef    cpx_ctrl_timeslice
        xdef    cpx_sl_x
        xdef    cpx_sl_arrow
        xdef    cpx_sl_dragx
        xdef    cpx_sl_dragy
        xdef    cpx_mfsave

; short cpx_xform_do(void *fn, OBJECT *tree, long edit, short *msg)
; Classic CPX callbacks take a 16-bit edit argument and 32-bit pointers.
; D2/A2 are callee-saved for the C caller and may be clobbered by the classic
; callback, so preserve them around the bridge.
cpx_xform_do:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        move.l  16(sp),a1
        move.w  22(sp),d1
        move.l  24(sp),d2
        move.l  d2,-(sp)
        move.w  d1,-(sp)
        move.l  a1,-(sp)
        jsr     (a0)
        lea     10(sp),sp
        movem.l (sp)+,d2/a2
        rts

; void cpx_rsh_obfix(void *fn, OBJECT *tree, long ob)
cpx_rsh_obfix:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        move.l  16(sp),a1
        move.w  22(sp),d1
        move.w  d1,-(sp)
        move.l  a1,-(sp)
        jsr     (a0)
        addq.l  #6,sp
        movem.l (sp)+,d2/a2
        rts

; short cpx_xgen_alert(void *fn, long id)
cpx_xgen_alert:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        move.w  18(sp),d0
        move.w  d0,-(sp)
        jsr     (a0)
        addq.l  #2,sp
        movem.l (sp)+,d2/a2
        rts

; long cpx_call2(void *fn, long a1, long a2)
; Standard 32-bit callback bridge: preserves D2/A2 around the classic call.
; Callers pass a zero filler argument for zero/one-argument callbacks.
cpx_call2:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        move.l  16(sp),d0
        move.l  20(sp),d1
        move.l  d1,-(sp)
        move.l  d0,-(sp)
        jsr     (a0)
        addq.l  #8,sp
        movem.l (sp)+,d2/a2
        rts

; long cpx_ctrl_timeslice(void *fn, long settings)
; MagiC's AESVARS ctrl_timeslice takes the packed value in D0 and returns D0.
cpx_ctrl_timeslice:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        move.l  16(sp),d0
        jsr     (a0)
        movem.l (sp)+,d2/a2
        rts

; void cpx_sl_x(void *fn, OBJECT *tree, short base, short slider,
;               short value, short min, short max, void *userdef)
cpx_sl_x:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        lea     -18(sp),sp
        move.l  34(sp),0(sp)
        move.w  40(sp),4(sp)
        move.w  44(sp),6(sp)
        move.w  48(sp),8(sp)
        move.w  52(sp),10(sp)
        move.w  56(sp),12(sp)
        move.l  58(sp),14(sp)
        jsr     (a0)
        lea     18(sp),sp
        movem.l (sp)+,d2/a2
        rts

; void cpx_sl_arrow(void *fn, OBJECT *tree, short base, short slider,
;                   short obj, short inc, short min, short max,
;                   short *value, short direction, void *userdef)
cpx_sl_arrow:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        lea     -26(sp),sp
        move.l  42(sp),0(sp)
        move.w  48(sp),4(sp)
        move.w  52(sp),6(sp)
        move.w  56(sp),8(sp)
        move.w  60(sp),10(sp)
        move.w  64(sp),12(sp)
        move.w  68(sp),14(sp)
        move.l  70(sp),16(sp)
        move.w  76(sp),20(sp)
        move.l  78(sp),22(sp)
        jsr     (a0)
        lea     26(sp),sp
        movem.l (sp)+,d2/a2
        rts

; void cpx_sl_dragx(void *fn, OBJECT *tree, short base, short slider,
;                   short min, short max, short *value, void *userdef)
cpx_sl_dragx:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        lea     -20(sp),sp
        move.l  36(sp),0(sp)
        move.w  42(sp),4(sp)
        move.w  46(sp),6(sp)
        move.w  50(sp),8(sp)
        move.w  54(sp),10(sp)
        move.l  56(sp),12(sp)
        move.l  60(sp),16(sp)
        jsr     (a0)
        lea     20(sp),sp
        movem.l (sp)+,d2/a2
        rts

; void cpx_sl_dragy(void *fn, OBJECT *tree, short base, short slider,
;                   short min, short max, short *value, void *userdef)
cpx_sl_dragy:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        lea     -20(sp),sp
        move.l  36(sp),0(sp)
        move.w  42(sp),4(sp)
        move.w  46(sp),6(sp)
        move.w  50(sp),8(sp)
        move.w  54(sp),10(sp)
        move.l  56(sp),12(sp)
        move.l  60(sp),16(sp)
        jsr     (a0)
        lea     20(sp),sp
        movem.l (sp)+,d2/a2
        rts

; void cpx_mfsave(void *fn, short flag, MFORM *mf)
cpx_mfsave:
        movem.l d2/a2,-(sp)
        movea.l 12(sp),a0
        move.w  18(sp),d0
        move.l  20(sp),a1
        subq.l  #6,sp
        move.w  d0,0(sp)
        move.l  a1,2(sp)
        jsr     (a0)
        addq.l  #6,sp
        movem.l (sp)+,d2/a2
        rts
