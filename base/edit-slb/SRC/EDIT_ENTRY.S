        section .slb_header,code
        xdef slb_header
        xref    slb_init
        xref    slb_exit
        xref    slb_open
        xref    slb_close
        xref    editor_fn210
        xref    editor_fn211
        xref    editor_fn212
        xref    editor_fn213
        xref    editor_fn214
        xref    editor_fn215
        xref    editor_fn216
        xref    editor_fn217
        xdef fn210_entry,fn211_entry,fn212_entry,fn213_entry
        xdef fn214_entry,fn215_entry,fn216_entry,fn217_entry
slb_header:
        dc.l    $70004afc
        dc.l    slb_name
        dc.l    9
        dc.l    0
        dc.l    slb_init
        dc.l    slb_exit
        dc.l    slb_open
        dc.l    slb_close
        dc.l    0
        dcb.l   8,0
        dc.l    0
slb_name:
        dc.b    'EDITOBJC.SLB',0
        even

        section .text
fn210_entry:
        move.l  a0,-(sp)
        jsr     editor_fn210
        addq.l  #4,sp
        rts
fn211_entry:
        move.l  a0,-(sp)
        jsr     editor_fn211
        addq.l  #4,sp
        rts
fn212_entry:
        move.l  a0,-(sp)
        jsr     editor_fn212
        addq.l  #4,sp
        rts
fn213_entry:
        move.l  a0,-(sp)
        jsr     editor_fn213
        addq.l  #4,sp
        rts
fn214_entry:
        move.l  a0,-(sp)
        jsr     editor_fn214
        addq.l  #4,sp
        rts
fn215_entry:
        move.l  a0,-(sp)
        jsr     editor_fn215
        addq.l  #4,sp
        rts
fn216_entry:
        move.l  a0,-(sp)
        jsr     editor_fn216
        addq.l  #4,sp
        rts
fn217_entry:
        move.l  a0,-(sp)
        jsr     editor_fn217
        addq.l  #4,sp
        rts
