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
