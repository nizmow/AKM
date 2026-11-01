XDEF _emu_dbgout

; Prozedur aufrufen. a0 auf Zeiger, a1 ist Parameter.

_emu_dbgout:
 DC.W $00c0
 rts
