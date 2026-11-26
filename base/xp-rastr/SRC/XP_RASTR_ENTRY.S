        section .slb_header,code
        xdef    slb_header
        xdef    __text
        xref    slb_init
        xref    slb_exit
        xref    slb_open
        xref    slb_close
        xref    slb_fn0
        xref    slb_fn1

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
        dc.l    2
        dc.l    slb_fn0
        dc.l    slb_fn1
slb_name:
        dc.b    'XP_RASTR.SLB',0
        even
