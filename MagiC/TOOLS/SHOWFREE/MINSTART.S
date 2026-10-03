*
*
* Minimum start code, without argc/argv
* However, the raw command line can be accessed via _BasPag.
*
*

        XREF  main,_StkSize,memcpy,memset


        XDEF  _BasPag,exit,_exit,errno,_StkLim


        TEXT

 move.l   4(sp),a3            ; Basepage
 move.l   a3,_BasPag
 movea.l  $c(a3),a0           ; length of TEXT segment
 adda.l   $14(a3),a0          ; length of DATA segment
 adda.l   $1c(a3),a0          ; length of BSS segment
 adda.w   #$100,a0            ; length of PD

 move.l   a3,d0
 add.l    a0,d0
 and.b    #$fc,d0
 move.l   d0,d1
 movea.l  d1,sp               ; sp to end of BSS segment
 sub.l    #_StkSize-$100,d0
 move.l   d0,_StkLim

 move.l   a0,-(sp)
 move.l   a3,-(sp)
 clr.w    -(sp)
 move.w   #$4a,-(sp)
 trap     #1                  ; gemdos Mshrink
 lea      $c(sp),sp

 jsr      main                ; run main() without parameters

exit:
 move.w   d0,-(sp)
_exit:
 move.w   #$4c,-(sp)
 trap     #1                  ; gemdos Pterm

        DATA

errno:
 DC.W     0

        BSS
_BasPag:
 DS.L     1
_StkLim:
 DS.L     1

        END

