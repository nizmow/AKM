err_1s:  DC.B  'Allgemeiner Fehler',0
err_02s: DC.B  'Laufwerk nicht bereit',0
err_03s: DC.B  'Unbekanntes Kommando',0
err_04s: DC.B  'CRC- Fehler',0
err_05s: DC.B  'Kommando falsch',0
err_06s: DC.B  'Spur nicht gefunden',0
err_07s: DC.B  'Unbekanntes Medium',0
err_08s: DC.B  'Sektor nicht gefunden',0
err_09s: DC.B  'Kein Papier',0
err_10s: DC.B  'Schreibfehler',0
err_11s: DC.B  'Lesefehler',0
err_13s: DC.B  'Disk schreibgeschÅtzt',0
err_14s: DC.B  'Unerlaubter Diskwechsel',0
err_15s: DC.B  'Unbekanntes GerÑt',0
err_16s: DC.B  'Defekte Sektoren',0
err_17s: DC.B  'Andere Disk einlegen!',0

err_32s: DC.B  'UngÅltige Funktionsnummer',0
err_33s: DC.B  'Datei nicht gefunden',0
err_34s: DC.B  'Pfad nicht gefunden',0
err_35s: DC.B  'Zuviele geîffnete Dateien',0
err_36s: DC.B  'Zugriff unmîglich',0
err_37s: DC.B  'UngÅltiges Handle',0
err_39s: DC.B  'Zuwenig Speicher',0
err_40s: DC.B  'UngÅltiger Speicherblock',0
err_46s: DC.B  'UngÅltiges Laufwerk',0
err_48s: DC.B  'Nicht dasselbe Laufwerk',0
err_49s: DC.B  'Keine weiteren Dateien',0
err_64s: DC.B  'Falscher Bereich',0
err_65s: DC.B  'Interner Fehler',0
err_66s: DC.B  'Datei nicht ausfÅhrbar',0
err_67s: DC.B  'Mshrink- Fehler',0
err_69s: DC.B  '68000 Exception',0           * KAOS
toserrs: DC.B  'TOS Fehler',0

     IFEQ MAGIX

change_s:			DC.B  'Bitte Disk %: in Laufwerk A: einlegen!',0
diskerr_s:		DC.B  '  auf Laufwerk %:',$d,$a
				DC.B  '[A]bbruch, [W]iederholen, [I]gnorieren ?',0
	ENDIF
	
syntax_echo_s:		DC.B  'Syntax: ECHO [ON|OFF]',CR,LF
				DC.B  '        ECHO [-n] [...]',CR,LF,0
out_of_envs:		DC.B  $d,$a,'Environment voll',$d,$a,0
syntaxfehlers:		DC.B  $d,$a,'Syntax: SET [var=[wert]]',$d,$a,0
disk_in_lw_s:		DC.B  $d,$a,' Disk in Laufwerk % ',0
keinname_s:		DC.B  'hat keinen Namen',0
ist_s:			DC.B  'ist ',0
syntaxattrs:		DC.B  'Syntax: ATTRIB [[+|-]{rash}] datei(en)',CR,LF,0
pfadzutiefs:		DC.B  $d,$a,'Pfad zu tief',$d,$a,0
pfads:			DC.B  $d,$a,'Pfad: ',0
kbinsg_s:			DC.B  '  kB insgesamt verfÅgbar',$d,$a,0
bytes_in_s:		DC.B  '  Bytes in ',0
hauptsp_s:		DC.B  '  Bytes RAM',$d,$a,0
nbytesvers:		DC.B  ' versteckten/System- Datei(en)',$d,$a,0
nbytesbens:		DC.B  ' Benutzerdatei(en)',$d,$a,0
nverzeichns:		DC.B  '  Verzeichnis(sse)',$d,$a,0
zeile_gekuerzt_s:	DC.B  $d,$a,'Kommandozeile gekÅrzt',$d,$a,0
nbytesfreis:		DC.B  '  Bytes frei',$d,$a,0
nkbfree:			DC.B  '  kB frei',$d,$a,0
nmbfree:			DC.B  '  MB frei',$d,$a,0
rens:			DC.B  'MV ',0
err_rens:			DC.B  ' => Fehler : ',0
isidents:			DC.B  ' Quelle und Ziel identisch',$d,$a,0
copys:			DC.B  'COPY ',0
auf_s:			DC.B  ' auf ',0
zielvolls:		DC.B  ' Ziel-Volume voll',$d,$a,0
syntaxrens:		DC.B  'Syntax: MV Quelle Ziel',$d,$a,0
syntaxcopys:		DC.B  'Syntax: COPY [-t] Quelle Ziel',$d,$a,0
unerlcopy_s:		DC.B  'UngÅltiges COPY',$d,$a,0
unerlren_s:		DC.B  'UngÅltiges MV',$d,$a,0
ndateiens:		DC.B  ' Datei(en) kopiert',$d,$a,0
akt_date_is:		DC.B  $d,$a,'Aktuelles Datum ist ',0
give_dates:		DC.B  $d,$a,'Neues Datum (tt/mm/jj) : ',0
akt_time_is:		DC.B  $d,$a,'Aktuelle Zeit ist ',0
give_times:		DC.B  $d,$a,'Neue Zeit (hh:mm:ss) : ',0
falsches_forms:	DC.B  $d,$a,'Falsches Format',$d,$a,0
loesch_s:			DC.B  $d,$a,'Lîsche ',0
auswahl_s:		DC.B  ' (J/N/G/A) ? ',0
global_s:			DC.B  'Global',0
nein_s:			DC.B  'Nein',0
ja_s:			DC.B  'Ja',0
quit_s:			DC.B  'Abbruch',0
dels:			DC.B  $d,$a,'DEL ',0
syntax_dels:		DC.B  $d,$a,'Syntax: DEL [-n] datei1 ...',$d,$a,0
space3_s:			DC.B  '   ',0
dir_zeichens:		DC.B  '   <DIR>',0
verzchn_vons:		DC.B  ' Verzeichnis von ',0
n_dbytes:			DC.B  '  Bytes in',0
n_dateiens:		DC.B  ' Datei(en)',0
echo_ists:		DC.B  $d,$a,'ECHO ist ',0
rmd_syntaxs:		DC.B  $d,$a,'Syntax: MD/RD Verzeichnis',$d,$a,0
ren_syntaxs:		DC.B  $d,$a,'Syntax: REN Quelle Name',$d,$a,0
kein_pfads:		DC.B  'Kein Pfad',0
datei_s:			DC.B  $d,$a,'Datei: ',0
kann_s:			DC.B  $d,$a,'Kann ',0
nicht_offn_s:		DC.B  ' nicht îffnen',$d,$a,0
syntax_touch:		DC.B  $d,$a,'Syntax: TOUCH [-s] datei1 ...',$d,$a,0
syntax_type:		DC.B  $d,$a,'Syntax: TYPE [-s] datei1 ...',$d,$a,0
status_s:			DC.B  $d,$a,'Flag ist ',0
error_verifys:		DC.B  'UngÅltiger Zustand',$d,$a,0
syntax_sorts:		DC.B  $d,$a,'Syntax: SORT -C -R -n',$d,$a,0
sort_memerrs:		DC.B  $d,$a,'SORT: Zuwenig RAM',$d,$a,0
sort_volume_full_s:	DC.B  'SORT: Ziel-Volume voll',$d,$a,0
syntax_finds:		DC.B  $d,$a,'Syntax: FIND text',$d,$a,0
mehrs:			DC.B  $d,'--Mehr--',0
syntax_ifs:		DC.B  $d,$a,'Fehler in IF- Ausdruck',$d,$a,0
syntax_shifts:		DC.B  $d,$a,'SHIFT: Operand fehlt',$d,$a,0
lbl_not_found_s:	DC.B  $d,$a,'GOTO: Label nicht gefunden',$d,$a,0
for_error_s:		DC.B  $d,$a,'FOR: Syntaxfehler oder Schachtelung',$d,$a,0
pipe_errs:		DC.B  CR,LF,' Pipe- Fehler',0
redir_errs:		DC.B  CR,LF,' Fehler beim Umlenken von STD',0
batch_errs:		DC.B  $d,$a,'BATCH zu tief verschachtelt',$d,$a,0
exec_fehler1_s:	DC.B  $d,$a,'EXEC Fehler =>',0
exec_fehler2_s:	DC.B  '<=',$d,$a,0
pfad_ueberl_s:		DC.B  $d,$a,'PfadÅberlauf',0
kmd_nicht_gef_s:	DC.B  $d,$a,'Kommando nicht gefunden',$d,$a,0

nach_art_s:		DC.B  ' nach Art',0
nach_dat_s:		DC.B  ' nach Datum',0
nach_grs_s:		DC.B  ' nach Grîûe',0
nach_nix_s:		DC.B  ' unsortiert',0
bytes_free_s:		DC.B  ' Bytes frei',$d,$a,$a,0

taste_drueckens:	DC.B  'Taste drÅcken ',0
not_founds:		DC.B  ' nicht gefunden',CR,LF,0
keine_dateiens:	DC.B  $d,$a,' Keine Dateien',0