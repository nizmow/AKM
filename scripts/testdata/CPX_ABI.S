        section .text,code
        xdef    _start
        xdef    cpx_init
        xref    cpx_xform_do
        xref    cpx_rsh_obfix
        xref    cpx_xgen_alert
        xref    cpx_call2
        xref    cpx_ctrl_timeslice
        xref    cpx_sl_arrow

_start:
        ; Each caller uses the C stack layout expected by the public bridge.
        move.l  #0x5a5aa5a5,-(sp)
        move.l  sp,saved_sp
        move.l  #0x33333333,d3
        move.l  #0x44444444,d4
        move.l  #0x55555555,d5
        move.l  #0x66666666,d6
        move.l  #0x77777777,d7
        move.l  #0x33333333,a3
        move.l  #0x44444444,a4
        move.l  #0x55555555,a5
        move.l  #0x66666666,a6
        move.l  #0x11223344,d2
        move.l  #0x55667788,a2
        pea     xform_msg
        move.l  #0x1234,-(sp)
        pea     0x23456789
        pea     native_xform
        jsr     cpx_xform_do
        lea     16(sp),sp
        cmpi.w  #0x1357,d0
        bne     fail1
        cmpi.l  #0x11223344,d2
        bne     fail2
        cmpa.l  #0x55667788,a2
        bne     fail3
        bsr     check_state

        move.l  #0x11223344,d2
        move.l  #0x55667788,a2
        pea     0x3456789a
        pea     0x23456789
        pea     native_obfix
        jsr     cpx_rsh_obfix
        lea     12(sp),sp
        cmpi.l  #0x11223344,d2
        bne     fail5
        cmpa.l  #0x55667788,a2
        bne     fail6
        bsr     check_state

        move.l  #0x11223344,d2
        move.l  #0x55667788,a2
        pea     0x1234
        pea     native_alert
        jsr     cpx_xgen_alert
        lea     8(sp),sp
        cmpi.w  #0xfedc,d0
        bne     fail7
        cmpi.l  #0x11223344,d2
        bne     fail8
        cmpa.l  #0x55667788,a2
        bne     fail9
        bsr     check_state

        move.l  #0x11223344,d2
        move.l  #0x55667788,a2
        move.l  #0x5a5aa5a5,-(sp)
        pea     0x76543210
        pea     0x12345678
        pea     native_call2
        jsr     cpx_call2
        lea     12(sp),sp
        cmpi.l  #0x5a5aa5a5,(sp)
        bne     fail15
        addq.l  #4,sp
        cmpi.l  #0x89abcdef,d0
        bne     fail10
        cmpi.l  #0x11223344,d2
        bne     fail11
        cmpa.l  #0x55667788,a2
        bne     fail12
        bsr     check_state

        ; zero- and one-argument callbacks use the documented zero filler.
        clr.l   -(sp)
        clr.l   -(sp)
        pea     native_zero
        jsr     cpx_call2
        lea     12(sp),sp
        cmpi.l  #0x2468ace0,d0
        bne     fail13
        bsr     check_state
        clr.l   -(sp)
        pea     0x10203040
        pea     native_one
        jsr     cpx_call2
        lea     12(sp),sp
        cmpi.l  #0xfedcba98,d0
        bne     fail14
        bsr     check_state

        move.l  #0x11223344,d2
        move.l  #0x55667788,a2
        pea     0x11223344
        pea     native_timeslice
        jsr     cpx_ctrl_timeslice
        lea     8(sp),sp
        cmpi.l  #0x90abcdef,d0
        bne     fail17
        cmpi.l  #0x11223344,d2
        bne     fail18
        cmpa.l  #0x55667788,a2
        bne     fail19
        bsr     check_state

        move.l  #0x11223344,d2
        move.l  #0x55667788,a2
        clr.l   -(sp)               ; userdef
        pea     1                   ; direction = horizontal
        pea     arrow_value
        pea     10                  ; max
        pea     1                   ; min
        pea     1                   ; inc
        pea     1                   ; obj
        pea     2                   ; slider
        pea     1                   ; base
        pea     arrow_tree
        pea     native_sl_arrow
        jsr     cpx_sl_arrow
        lea     44(sp),sp
        cmpi.l  #0x11223344,d2
        bne     fail29
        cmpa.l  #0x55667788,a2
        bne     fail30
        bsr     check_state
        moveq   #0,d1
        bra     exit
fail15: moveq #15,d1
        bra exit
fail17: moveq #17,d1
        bra exit
fail18: moveq #18,d1
        bra exit
fail19: moveq #19,d1
        bra exit
fail29: moveq #29,d1
        bra exit
fail30: moveq #30,d1
        bra exit

fail1:  moveq #1,d1
        bra exit
fail2:  moveq #2,d1
        bra exit
fail3:  moveq #3,d1
        bra exit
fail5:  moveq #5,d1
        bra exit
fail6:  moveq #6,d1
        bra exit
fail7:  moveq #7,d1
        bra exit
fail8:  moveq #8,d1
        bra exit
fail9:  moveq #9,d1
        bra exit
fail10: moveq #10,d1
        bra exit
fail11: moveq #11,d1
        bra exit
fail12: moveq #12,d1
        bra exit
fail13: moveq #13,d1
        bra exit
fail14: moveq #14,d1
exit:   moveq #1,d0
        trap #0

; Called after each bridge. Account for this routine's own return address.
check_state:
        move.l  sp,d1
        addq.l  #4,d1
        cmp.l   saved_sp,d1
        bne     fail15
        cmpi.l  #0x5a5aa5a5,4(sp)
        bne     fail15
        cmpi.l  #0x11223344,d2
        bne     fail16
        cmpa.l  #0x55667788,a2
        bne     fail16
        cmpi.l  #0x33333333,d3
        bne     fail16
        cmpi.l  #0x44444444,d4
        bne     fail16
        cmpi.l  #0x55555555,d5
        bne     fail16
        cmpi.l  #0x66666666,d6
        bne     fail16
        cmpi.l  #0x77777777,d7
        bne     fail16
        cmpa.l  #0x33333333,a3
        bne     fail16
        cmpa.l  #0x44444444,a4
        bne     fail16
        cmpa.l  #0x55555555,a5
        bne     fail16
        cmpa.l  #0x66666666,a6
        bne     fail16
        rts
fail16: moveq   #16,d1
        bra     exit

; Native callbacks validate stack arguments and stack guard, then clobber D2/A2.
native_xform:
        cmpi.l  #0x23456789,4(sp)
        bne cbfail1
        cmpi.w  #0x1234,8(sp)
        bne cbfail1
        cmpi.l  #xform_msg,10(sp)
        bne cbfail1
        move.l  #0xaabbccdd,d2
        move.l  #0xdeadbeef,a2
        move.l  #0x1357,d0
        rts
native_obfix:
        cmpi.l  #0x23456789,4(sp)
        bne cbfail2
        cmpi.w  #0x789a,8(sp)
        bne cbfail2
        move.l  #0xaabbccdd,d2
        move.l  #0xdeadbeef,a2
        moveq   #0,d0
        rts
native_alert:
        cmpi.w  #0x1234,4(sp)
        bne cbfail3
        move.l  #0xaabbccdd,d2
        move.l  #0xdeadbeef,a2
        move.l  #0xfffffedc,d0
        rts
native_call2:
        cmpi.l  #0x12345678,4(sp)
        bne cbfail4
        cmpi.l  #0x76543210,8(sp)
        bne cbfail4
        move.l  #0xaabbccdd,d2
        move.l  #0xdeadbeef,a2
        move.l  #0x89abcdef,d0
        rts
native_zero:
        cmpi.l  #0,4(sp)
        bne cbfail5
        move.l  #0xaabbccdd,d2
        move.l  #0xdeadbeef,a2
        move.l  #0x2468ace0,d0
        rts
native_one:
        cmpi.l  #0x10203040,4(sp)
        bne cbfail6
        move.l  #0xaabbccdd,d2
        move.l  #0xdeadbeef,a2
        move.l  #0xfedcba98,d0
        rts
native_timeslice:
        cmpi.l  #0x11223344,d0
        bne cbfail7
        move.l  #0xaabbccdd,d2
        move.l  #0xdeadbeef,a2
        move.l  #0x90abcdef,d0
        rts
native_sl_arrow:
        cmpi.l  #arrow_tree,4(sp)
        bne cbfail8
        cmpi.w  #1,8(sp)            ; base
        bne cbfail8
        cmpi.w  #2,10(sp)           ; slider
        bne cbfail8
        cmpi.w  #1,12(sp)           ; obj
        bne cbfail8
        cmpi.w  #1,14(sp)           ; inc
        bne cbfail8
        cmpi.w  #1,16(sp)           ; min
        bne cbfail8
        cmpi.w  #10,18(sp)          ; max
        bne cbfail8
        cmpi.l  #arrow_value,20(sp)
        bne cbfail8
        cmpi.w  #1,24(sp)           ; horizontal direction
        bne cbfail8
        move.l  #0xaabbccdd,d2
        move.l  #0xdeadbeef,a2
        rts
cbfail1: moveq #21,d1
        bra exit
cbfail2: moveq #22,d1
        bra exit
cbfail3: moveq #23,d1
        bra exit
cbfail4: moveq #24,d1
        bra exit
cbfail5: moveq #25,d1
        bra exit
cbfail6: moveq #26,d1
        bra exit
cbfail7: moveq #27,d1
        bra exit
cbfail8: moveq #28,d1
        bra exit

        section .data,data
saved_sp:
        dc.l    0
xform_msg:
        dc.l    0xabcdef01
arrow_value:
        dc.w    5
arrow_tree:
        dc.l    0

        ; Link-only entry required by cpx_entry.S; never called.
cpx_init:
        rts
