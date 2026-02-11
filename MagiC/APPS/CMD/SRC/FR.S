err_1s:  DC.B  'Erreur g‚n‚rale',0
err_02s: DC.B  'Lecteur non prˆt',0
err_03s: DC.B  'Commande inconnue',0
err_04s: DC.B  'Erreur CRC',0
err_05s: DC.B  'Commande incorrecte',0
err_06s: DC.B  'Piste introuvable',0
err_07s: DC.B  'M‚dia inconnu',0
err_08s: DC.B  'Secteur introuvable',0
err_09s: DC.B  'Pas de papier',0
err_10s: DC.B  'Erreur d''‚criture',0
err_11s: DC.B  'Erreur de lecture',0
err_13s: DC.B  'Disque en lecture seule',0
err_14s: DC.B  'Changement de disque non autoris‚',0
err_15s: DC.B  'P‚riph‚rique inconnu',0
err_16s: DC.B  'Secteurs d‚fectueux',0
err_17s: DC.B  'Ins‚rez un autre disque!',0

err_32s: DC.B  'Num‚ro de fonction invalide',0
err_33s: DC.B  'Fichier introuvable',0
err_34s: DC.B  'Chemin introuvable',0
err_35s: DC.B  'Trop de fichiers ouverts',0
err_36s: DC.B  'AccŠs impossible',0
err_37s: DC.B  'Handle non valide',0
err_39s: DC.B  'M‚moire insuffisante',0
err_40s: DC.B  'Bloc m‚moire non valide',0
err_46s: DC.B  'Lecteur non valide',0
err_48s: DC.B  'Lecteur diff‚rent',0
err_49s: DC.B  'Aucun autre fichier',0
err_64s: DC.B  'Zone incorrecte',0
err_65s: DC.B  'Erreur interne',0
err_66s: DC.B  'Fichier non ex‚cutable',0
err_67s: DC.B  'Erreur Mshrink',0
err_69s: DC.B  'Exception 68000',0           * KAOS
toserrs: DC.B  'Erreur TOS',0

     IFEQ MAGIX

	* ATTENTION: set insert position!
change_s:			DC.B  'Veuillez ins‚rer le disque %: dans le lecteur A:!',0
diskerr_s:		DC.B  '  sur le disque %:',$d,$a
				DC.B  '[A]nnuler, [R]ecommencer, [I]gnorer ?',0
	ENDIF

out_of_envs:		DC.B  $d,$a,'Environnement plein',$d,$a,0
syntaxfehlers:		DC.B  $d,$a,'Syntaxe: SET [var=[valeur]]',$d,$a,0
	* ATTENTION: set insert position!
disk_in_lw_s:		DC.B  $d,$a,'Le disque dans le lecteur % ',0
keinname_s:		DC.B  'a pas de nom',0
ist_s:			DC.B  'est ',0
syntaxattrs:		DC.B  'Syntaxe: ATTRIB [[+|-]{rash}] fichier(s)',CR,LF,0
pfadzutiefs:		DC.B  $d,$a,'Chemin trop profond',$d,$a,0
pfads:			DC.B  $d,$a,'Chemin: ',0
kbinsg_s:			DC.B  '  kO disponibles au total',$d,$a,0		* TODO: or Ko?
bytes_in_s:		DC.B  '  Octets dans ',0
hauptsp_s:		DC.B  '  Octets m‚moire principale',$d,$a,0
nbytesvers:		DC.B  ' Fichier(s) cach‚(s)/systŠme',$d,$a,0
nbytesbens:		DC.B  ' Fichier(s) utilisateur',$d,$a,0
nverzeichns:		DC.B  '  R‚pertoire(s)',$d,$a,0
zeile_gekuerzt_s:	DC.B  $d,$a,'Ligne de commande tronqu‚e',$d,$a,0
nbytesfreis:		DC.B  '  Octets libres',$d,$a,0
nkbfree:			DC.B  '  kO libres',$d,$a,0		* TODO: or Ko?
nmbfree:			DC.B  '  MO libres',$d,$a,0
rens:			DC.B  'MV ',0
err_rens:			DC.B  ' => Erreur : ',0
isidents:			DC.B  ' Source et destination identiques',$d,$a,0
copys:			DC.B  'COPY ',0
auf_s:			DC.B  ' sur ',0
zielvolls:		DC.B  ' Disque de destination plein',$d,$a,0
syntaxrens:		DC.B  'Syntaxe: MV Source Destination',$d,$a,0
syntaxcopys:		DC.B  'Syntaxe: COPY [-t] Source Destination',$d,$a,0
unerlcopy_s:		DC.B  'COPY invalide',$d,$a,0
unerlren_s:		DC.B  'MV invalide',$d,$a,0
ndateiens:		DC.B  ' Fichier(s) copi',$d,$a,0
akt_date_is:		DC.B  $d,$a,'La date actuelle est ',0
give_dates:		DC.B  $d,$a,'Nouvelle date (jj/mm/aa) : ',0
akt_time_is:		DC.B  $d,$a,'L',$27,'heure actuelle est ',0
give_times:		DC.B  $d,$a,'Nouvelle heure (hh:mm:ss) : ',0
falsches_forms:	DC.B  $d,$a,'Format incorrect',$d,$a,0
loesch_s:			DC.B  $d,$a,'Effacer ',0
auswahl_s:		DC.B  ' (O/N/G/A) ? ',0
global_s:			DC.B  'Global',0
nein_s:			DC.B  'Non',0
ja_s:			DC.B  'Oui',0
quit_s:			DC.B  'Annuler',0
dels:			DC.B  $d,$a,'DEL ',0
syntax_dels:		DC.B  $d,$a,'Syntaxe: DEL [-n] fichier1 ...',$d,$a,0
space3_s:			DC.B  '   ',0
dir_zeichens:		DC.B  '   <DIR>',0
verzchn_vons:		DC.B  ' R‚pertoire de ',0
n_dbytes:			DC.B  '  Octets dans',0
n_dateiens:		DC.B  ' fichier(s)',0
echo_ists:		DC.B  $d,$a,'ECHO est ',0
rmd_syntaxs:		DC.B  $d,$a,'Syntaxe: MD/RD R‚pertoire',$d,$a,0
ren_syntaxs:		DC.B  $d,$a,'Syntaxe: REN Source Nom',$d,$a,0
kein_pfads:		DC.B  'Aucun chemin',0
datei_s:			DC.B  $d,$a,'Fichier: ',0
kann_s:			DC.B  $d,$a,'Ouverture de ',0
nicht_offn_s:		DC.B  ' impossible',$d,$a,0
syntax_ts:		DC.B  $d,$a,'Syntaxe: TYPE|TOUCH Nom(s) de fichier',$d,$a,0
status_s:			DC.B  $d,$a,'Le Flag est ',0
error_verifys:		DC.B  'Etat invalide',$d,$a,0
syntax_sorts:		DC.B  $d,$a,'Syntaxe: SORT -C -R -n',$d,$a,0
sort_memerrs:		DC.B  $d,$a,'SORT: M‚moire insuffisante',$d,$a,0
sort_zielvolls:	DC.B  'SORT: Disque destination plein',$d,$a,0
syntax_finds:		DC.B  $d,$a,'Syntaxe: FIND chaŒne',$d,$a,0
mehrs:			DC.B  $d,'--Plus--',0
syntax_ifs:		DC.B  $d,$a,'Erreur d''expression IF',$d,$a,0
syntax_shifts:		DC.B  $d,$a,'SHIFT: Op‚rande manquante',$d,$a,0
lbl_not_found_s:	DC.B  $d,$a,'GOTO: Balise non trouv‚e',$d,$a,0
for_error_s:		DC.B  $d,$a,'FOR: Erreur de syntaxe ou d',$27,'imbrication',$d,$a,0
pipe_errs:		DC.B  CR,LF,' Erreur de Pipe',0
redir_errs:		DC.B  CR,LF,' Erreur lors de la redirection de STD',0
batch_errs:		DC.B  $d,$a,'BATCH trop imbriqu‚',$d,$a,0
exec_fehler1_s:	DC.B  $d,$a,'Erreur EXEC =>',0
exec_fehler2_s:	DC.B  '<=',$d,$a,0
pfad_ueberl_s:		DC.B  $d,$a,'D‚bordement de chemin',0
kmd_nicht_gef_s:	DC.B  $d,$a,'Commande introuvable',$d,$a,0

nach_art_s:		DC.B  ' par type',0
nach_dat_s:		DC.B  ' par date',0
nach_grs_s:		DC.B  ' par taille',0
nach_nix_s:		DC.B  ' pon tri‚  ',0
bytes_free_s:		DC.B  ' Octets libres',$d,$a,$a,0

taste_drueckens:	DC.B  'Pressez une touche ',0
not_founds:		DC.B  '  introuvable'
keine_dateiens:	DC.B  $d,$a,' Aucun fichier',0
