 LISTCHAR 15
; --------------------------------------------------------
;	Set Environment	1.5
;
; --------------------------------------------------------

 OPT	O+
 OUTPUT	C:\AUTO\SETENV.PRG

BELL		=  7
CR		= 13
LF		= 10
ESC		= 27
WHITESPACE 	= 32

EnvPtr		= 44
ParentPD	= 36

_bootdev  = $446
the_env	  = $4BE
_sysbase  = $4F2
_shell_p  = $4F6
end_os	  = $4FA
exec_os	  = $4FE

SUCCESS	  = 0
XB_OLD	  = 4

Cconws	 = $09
Fopen	 = $3D
Fread	 = $3F
Fclose	 = $3E
Ptermres = $31
Pterm	 = $4C
Supexec	 = $26

GEMDOS	MACRO	funcID,stackval
	move	#\1,-(A7)
	trap	#1

	IFC	'','\2'
	MEXIT
	ENDC
	
	IFNE	(\2)<9
	addq.l	#\2,A7
	ELSEIF
	lea	\2(A7),A7
	ENDC
	ENDM

XBIOS	MACRO	funcID,stackval
	move	#\1,-(A7)
	trap	#14
	
	IFNE	(\2)<9
 	addq.l	#\2,A7
 	ELSEIF
 	lea	\2(A7),A7
 	ENDC
 	ENDM
 

	SECTION	TEXT
init_1:
	bra	init_2
	DC.L	'XBRA','SENV',0

environ_ins:
	move.L	4(A7),A5
	lea	envbuf(PC),A0
	move.L	A0,EnvPtr(A5)
	move.L	environ_ins-XB_OLD(PC),A0
	jmp	(A0)


init_3:
	move	#1024*2-1,D0		; L”sche EnvBuf und FileBuf...
	move.l	A5,A0
clr_env:
	clr.B	(A0)+
	dbra	D0,clr_env

	pea	1024(A5)		; Fread(D7,1024L,FileBuf)
	pea	1024\W
	move	D7,-(A7)
	GEMDOS	Fread,12

	move	D7,-(A7)		; Fclose(D7)
	GEMDOS	Fclose,4

	move.l	A5,A0
	lea	1024(A5),A1
	moveq	#0,D1
	st	D1
	sf	D2
	move	#1022-1,D0

copy_env:
	cmp.B	#WHITESPACE,(A1)	; Kopiere EnvVars...
	blt.s	clr_char

	seq	D2
	beq.s	no_space

	move.L	A0,A2
no_space:
	move.B	(A1)+,(A0)+
	st	D1
	bra.s	nextchar
clr_char:
	tst.B	D1
	beq.S	no_clr

	tst.B	D2
	beq.S	no_skip

	move.L	A2,A0
no_skip:
	clr.B	(A0)+
	sf	D1
	sf	D2
no_clr:	addq.L	#1,A1
nextchar:
	dbra	D0,copy_env

	move	#1022-14-1,D0
	move.l	A5,A0

chk_EOS:
	tst.B	(A0)+
	bne.s	not_EOS
	tst.B	(A0)
not_EOS:
	dbeq	D0,chk_EOS

	tst	D0			;  Aussreichend	Platz ?
	bmi.s	no_envsize		;  Ja, fge ENVSIZE= hinzu...

	lea	envsizstr(PC),A1
copy_siz:
	move.B	(A1)+,(A0)+
	bne.s	copy_siz

no_envsize:
	move	#1024,A0

	clr	-(A7)
	pea	(init_2-init_1)+256(A0)
	GEMDOS	Ptermres

envsizstr:
	DC.B	'ENVSIZE=0x0400',0
	DC.L	'XBRA','SENV',0
envbuf:
init_2:
	pea	msg_load(PC)
	GEMDOS	Cconws,6

*	pea	setbootdev(PC)
*	XBIOS	Supexec,6
* ILLEGAL
	clr	-(A7)			; D7 = Fopen(envfile,M_READ)...
	pea	envfile(pc)
	GEMDOS	Fopen,8
	move	D0,D7
	bmi	open_err

	pea	setvecs(PC)
	XBIOS	Supexec,6

	lea	envbuf(PC),A5
	bra	init_3

open_err:
	pea	opn_err(pc)
	GEMDOS	Cconws,6

	move	d0,-(A7)
	GEMDOS	Pterm

setvecs:
	lea	environ_ins-XB_OLD(PC),A0
	move.L	exec_os\W,(A0)+
	move.L	A0,exec_os\W

	lea	envbuf-XB_OLD(PC),A0
	move.L	the_env\W,(A0)+
	move.L	A0,the_env\W
	clr.l	_shell_p\w
	rts

*setbootdev:
*	move	_bootdev\w,d0
*	lea	envfile(PC),a0
*	add.b	d0,(a0)
*	rts

	SECTION DATA

msg_load:
	DC.B	' ',ESC,'p',CR,LF
	DC.B	' Set Environment 1.5 (SETENV) ',ESC,'q',CR,LF
	DC.B	'  ½ Andreas Grož 28-Apr-1990',CR,LF,0
opn_err:
	DC.B	BELL,' SETENV: Cannot open '
envfile: 
	DC.B	'\GEMSYS\DESKTOP.ENV',0

	END
