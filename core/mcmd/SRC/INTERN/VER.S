ver_com:
	; MagiC-, KAOS- oder TOS-Version
 lea      kaos_s(pc),a0            * 'MagiC v' oder 'KAOS v'
 move.b   is_kaos(pc),d0
 bne.b    ver_1
 lea      tos_s(pc),a0
ver_1:
 bsr      strstdout
 move.l   d+c_sysbase(pc),a0
 move.w   os_version(a0),d0
 move.w   os_gendatg(a0),-(sp)
 bsr.b    print_ver                * BS-Versionsnummer, z.B. "3.0"
 moveq    #' ',d0
 bsr      putchar
 move.w   (sp)+,d0
 suba.w   #10,sp
 lea      (sp),a0
 bsr      _date_to_str
 lea      (sp),a0
 bsr      strstdout                * BS-Erstelldatum, "z.B. 26/03/26"
 adda.w   #10,sp
	; GEMDOS-Version
 lea      gemdos_s(pc),a0          * "GEMDOS v"
 bsr      strstdout
 gemdos   Sversion
 addq.l   #2,sp
 ror.w    #8,d0                    * Low/High - Byte vertauschen
 bsr.b    print_ver                * GEMDOS-Versionsnummer z.B. "0.32"
 bsr      crlf_stdout
	; MCMD- oder CMD-Version
	IFF ACC
 lea      titels(pc),a0
	IFNE MAGIX
 clr.b    17(a0)				; Variante for MagiC
	ELSE
 clr.b    19(a0)				; Variante fÅr KAOS und TOS
	ENDIF					; IFNE MAGIX
 bsr      strstdout
	ENDIF					; IFF ACC
	IF ACC
 lea      aes_s(pc),a0
 bsr      strstdout
 move.w   d+global(pc),d0
 bsr.b    print_ver
	ENDIF					; IF ACC
 bra      crlf_stdout

print_ver:
 move.w   d0,-(sp)
 lsr.w    #8,d0
 bsr.b    prv_1                    * oberste Ziffer ausgeben
 moveq    #'.',d0                  * '.' ausgeben
 bsr      putchar
 move.w   (sp)+,d0                 * unterste Ziffer ausgeben
prv_1:
 andi.l   #$ff,d0
 bsr      lwrite_long
 rts
 