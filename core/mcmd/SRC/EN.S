err_1s:  DC.B  'General error',0
err_02s: DC.B  'Drive not ready',0
err_03s: DC.B  'Unknown command',0
err_04s: DC.B  'CRC error',0
err_05s: DC.B  'Wrong command',0
err_06s: DC.B  'Track not found',0
err_07s: DC.B  'Unknown medium',0
err_08s: DC.B  'Sector not found',0
err_09s: DC.B  'No paper',0
err_10s: DC.B  'Write error',0
err_11s: DC.B  'Read error',0
err_13s: DC.B  'Disk write-protected',0
err_14s: DC.B  'Forbidden disk change',0
err_15s: DC.B  'Unknown device',0
err_16s: DC.B  'Defect sectors',0
err_17s: DC.B  'Insert other disk!',0

err_32s: DC.B  'Invalid function number',0
err_33s: DC.B  'File not found',0
err_34s: DC.B  'Path not found',0
err_35s: DC.B  'Too many open files',0
err_36s: DC.B  'Access denied',0
err_37s: DC.B  'Invalid handle',0
err_39s: DC.B  'Out of memory',0
err_40s: DC.B  'Invalid memory block',0
err_46s: DC.B  'Invalid drive',0
err_48s: DC.B  'Not the same volume',0
err_49s: DC.B  'No more files',0
err_64s: DC.B  'Invalid range',0
err_65s: DC.B  'Internal error',0
err_66s: DC.B  'File not executable',0
err_67s: DC.B  'Mshrink failure',0
err_69s: DC.B  '68000 Exception',0           * KAOS
toserrs: DC.B  'TOS error',0

     IFEQ MAGIX

	* ATTENTION: set insert position!
change_s:			DC.B  'Insert disk %: to drive A:!',0
diskerr_s:		DC.B  '  on drive %:',$d,$a
				DC.B  '[A]bort, [R]etry, [I]gnore ?',0
	ENDIF

syntax_echo_s:		DC.B  'Syntax: ECHO [ON|OFF]',CR,LF
				DC.B  '        ECHO [-n] [...]',CR,LF,0
out_of_envs:		DC.B  $d,$a,'Environment full',$d,$a,0
syntaxfehlers:		DC.B  $d,$a,'Syntax: SET [var=[value]]',$d,$a,0
	* ATTENTION: set insert position!
disk_in_lw_s:		DC.B  $d,$a,' Disk in drive % ',0
keinname_s:		DC.B  'is unnamed',0
ist_s:			DC.B  'is ',0
syntaxattrs:		DC.B  'Syntax: ATTRIB [[+|-]{rash}] file(s)',CR,LF,0
pfadzutiefs:		DC.B  $d,$a,'Path too deep',$d,$a,0
pfads:			DC.B  $d,$a,'Path: ',0
kbinsg_s:			DC.B  '  kB total available',$d,$a,0
bytes_in_s:		DC.B  '  bytes in ',0
hauptsp_s:		DC.B  '  bytes RAM',$d,$a,0
nbytesvers:		DC.B  ' hidden or system file(s)',$d,$a,0
nbytesbens:		DC.B  ' user file(s)',$d,$a,0
nverzeichns:		DC.B  '  directory/ies',$d,$a,0
zeile_gekuerzt_s:	DC.B  $d,$a,'Command line truncated',$d,$a,0
nbytesfreis:		DC.B  '  bytes free',$d,$a,0
nkbfree:			DC.B  '  kB free',$d,$a,0
nmbfree:			DC.B  '  MB free',$d,$a,0
rens:			DC.B  'MV ',0
err_rens:			DC.B  ' => Error : ',0
isidents:			DC.B  ' Source is same as destination',$d,$a,0
copys:			DC.B  'COPY ',0
auf_s:			DC.B  ' on ',0
zielvolls:		DC.B  ' Destination volume full',$d,$a,0
syntaxrens:		DC.B  'Syntax: MV source destination',$d,$a,0
syntaxcopys:		DC.B  'Syntax: COPY [-t] source destination',$d,$a,0
unerlcopy_s:		DC.B  'Invalid COPY',$d,$a,0
unerlren_s:		DC.B  'Invalid MV',$d,$a,0
ndateiens:		DC.B  ' file(s) copied',$d,$a,0
akt_date_is:		DC.B  $d,$a,'Current date is ',0
give_dates:		DC.B  $d,$a,'New date (dd/mm/yy) : ',0
akt_time_is:		DC.B  $d,$a,'Current time is ',0
give_times:		DC.B  $d,$a,'New time (hh:mm:ss) : ',0
falsches_forms:	DC.B  $d,$a,'Wrong format',$d,$a,0
loesch_s:			DC.B  $d,$a,'Delete ',0
auswahl_s:		DC.B  ' (Y/N/G/A) ? ',0
global_s:			DC.B  'Global',0
nein_s:			DC.B  'No',0
ja_s:			DC.B  'Yes',0
quit_s:			DC.B  'Abort',0
dels:			DC.B  $d,$a,'DEL ',0
syntax_dels:		DC.B  $d,$a,'Syntax: DEL [-n] file1 ...',$d,$a,0
space3_s:			DC.B  '   ',0
dir_zeichens:		DC.B  '   <DIR>',0
verzchn_vons:		DC.B  ' Content of ',0
n_dbytes:			DC.B  '  bytes in',0
n_dateiens:		DC.B  ' file(s)',0
echo_ists:		DC.B  $d,$a,'ECHO is ',0
rmd_syntaxs:		DC.B  $d,$a,'Syntax: MD/RD directory',$d,$a,0
ren_syntaxs:		DC.B  $d,$a,'Syntax: REN source name',$d,$a,0
kein_pfads:		DC.B  'No path',0
datei_s:			DC.B  $d,$a,'file: ',0
kann_s:			DC.B  $d,$a,'Cannot open ',0
nicht_offn_s:		DC.B  ' ',$d,$a,0
syntax_touch:		DC.B  $d,$a,'Syntax: TOUCH [-s] filename(s)',$d,$a,0
syntax_type:		DC.B  $d,$a,'Syntax: TYPE [-s] filename(s)',$d,$a,0
status_s:			DC.B  $d,$a,'Flag is ',0
error_verifys:		DC.B  'Invalid state',$d,$a,0
syntax_sorts:		DC.B  $d,$a,'Syntax: SORT -C -R -n',$d,$a,0
sort_memerrs:		DC.B  $d,$a,'SORT: insufficient memory',$d,$a,0
sort_volume_full_s:	DC.B  'SORT: Destination volume full',$d,$a,0
syntax_finds:		DC.B  $d,$a,'Syntax: FIND string',$d,$a,0
mehrs:			DC.B  $d,'--more--',0
syntax_ifs:		DC.B  $d,$a,'Error in IF expression',$d,$a,0
syntax_shifts:		DC.B  $d,$a,'SHIFT: missing operand',$d,$a,0
lbl_not_found_s:	DC.B  $d,$a,'GOTO: label not found',$d,$a,0
for_error_s:		DC.B  $d,$a,'FOR: syntax error or nesting',$d,$a,0
pipe_errs:		DC.B  CR,LF,' Pipe error',0
redir_errs:		DC.B  CR,LF,' Error while redirecting STD',0
batch_errs:		DC.B  $d,$a,'BATCH nesting overflow',$d,$a,0
exec_fehler1_s:	DC.B  $d,$a,'EXEC error =>',0
exec_fehler2_s:	DC.B  '<=',$d,$a,0
pfad_ueberl_s:		DC.B  $d,$a,'Path overflow',0
kmd_nicht_gef_s:	DC.B  $d,$a,'Command not found',$d,$a,0

nach_art_s:		DC.B  ' by type',0
nach_dat_s:		DC.B  ' by date/time',0
nach_grs_s:		DC.B  ' by size',0
nach_nix_s:		DC.B  ' unsorted',0
bytes_free_s:		DC.B  ' bytes free',$d,$a,$a,0

taste_drueckens:	DC.B  'Press key ',0
not_founds:		DC.B  '  not found'
keine_dateiens:	DC.B  $d,$a,' No files',0
