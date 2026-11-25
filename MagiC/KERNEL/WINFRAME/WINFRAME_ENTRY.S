        section .slb_header,code
        xdef    slb_header
        xdef    __text
        xref    slb_init
        xref    slb_exit
        xref    slb_open
        xref    slb_close

slb_header:
__text:
        dc.l    $70004afc
        dc.l    slb_name
        dc.l    1
        dc.l    0
        dc.l    slb_init
        dc.l    slb_exit
        dc.l    slb_open
        dc.l    slb_close
        dc.l    0
        dcb.l   8,0
        dc.l    0
slb_name:
        dc.b    'WINFRAME.SLB',0
        even

        section .text
        xref    wbm_create
        xref    wbm_skind
        xref    wbm_ssize
        xref    wbm_sslid
        xref    wbm_sstr
        xref    wbm_sattr
        xref    wbm_calc
        xref    wbm_obfind
        xdef    win_wbm_create
        xdef    win_wbm_skind
        xdef    win_wbm_ssize
        xdef    win_wbm_sslid
        xdef    win_wbm_sstr
        xdef    win_wbm_sattr
        xdef    win_wbm_calc
        xdef    win_wbm_obfind

win_wbm_create:
        move.l  a0,-(sp)
        jsr     wbm_create
        addq.l  #4,sp
        rts

win_wbm_skind:
        move.l  a0,-(sp)
        jsr     wbm_skind
        addq.l  #4,sp
        rts

win_wbm_ssize:
        move.l  a0,-(sp)
        jsr     wbm_ssize
        addq.l  #4,sp
        rts

win_wbm_sslid:
        move.l  d0,-(sp)
        move.l  a0,-(sp)
        jsr     wbm_sslid
        addq.l  #8,sp
        rts

win_wbm_sstr:
        move.l  a0,-(sp)
        jsr     wbm_sstr
        addq.l  #4,sp
        rts

win_wbm_sattr:
        move.l  d0,-(sp)
        move.l  a0,-(sp)
        jsr     wbm_sattr
        addq.l  #8,sp
        rts

win_wbm_calc:
        move.l  a0,-(sp)
        move.l  d0,-(sp)
        jsr     wbm_calc
        addq.l  #8,sp
        rts

win_wbm_obfind:
        move.l  d1,-(sp)
        move.l  d0,-(sp)
        move.l  a0,-(sp)
        jsr     wbm_obfind
        lea     12(sp),sp
        rts
