/**************************************************
*
* ALLGEMEINE INCLUDES UND DEFINITIONEN
*
**************************************************/

#define MAGIXDESK 1		/* TRUE: MagiX- Desktop kreieren */
#define DEBUG	0

#include <aes.h>
#include <magix.h>

#if	MAGIXDESK
#include "magxdesk.h"
#else
#include "kaosdesk.h"
#endif

#define TRUE   1
#define FALSE  0
#define EOS    '\0'
#define NULL        ( ( void * ) 0L )
#define MIN(a,b) ((a < b) ? a : b)
#define MAX(a,b) ((a > b) ? a : b)
#define ABS(X) ((X>0) ? X : -X)
#define VA_START 0x4711
#define USA	0
#define FRG	1


/**************************************************
*
* EIGENE FEHLERCODES
*
**************************************************/

#define ENOWND		-100			/* kein Fenster verfÅgbar */


/**************************************************
*
* GLOBALE VARIABLEN
*
**************************************************/

extern int isgem2;
extern int iskgem;		/* KAOS 1.4 */
extern int isxgem;		/* MAGIX    */
extern int n_icons,n_apps;
extern char *get_name(char *path);	/* Aus ALLG.C */
extern long cache_on (char *path); /* aus SPEC.C */
extern long cache_off(char *path); /* aus SPEC.C */

#define SCREEN     0
#define FENSTERTYP (NAME+CLOSER+FULLER+MOVER+INFO+SIZER+UPARROW+DNARROW+VSLIDE+HSLIDE)

#define ANZDRIVES	16			/* Maximale Anzahl Laufwerke */
#define ANZPROGRAMS	14			/* Anzahl Dateien in "Programme" */
#define ANZFENSTER	5			/* Anzahl der Fenster */
#define PLUSICONS	10			/* Anzahl freier Desktop- Icons */
#define PLUSAPPS	10			/* noch anmeldbare Anwendungen */
#define ANZDATEIEN	600			/* Dateien insgesamt in allen Fenstern */
#define TTPLEN 63

typedef struct
{
	int			number;			/* Eingangsreihenfolge */
	unsigned char	flags;			/* Bit 0: "paût zu Muster" */
								/* Bit 1: versch (selected) */
								/* Bit 2: ist Programm */
	unsigned char	icontyp;
	unsigned int   time;
	unsigned int   date;
	unsigned long  filesize;
	char           filename[13];
	unsigned char  attrib;
} MYDTA;

typedef struct
{
	char		fname[45];
} MYFNAME;

typedef struct
{
	int		handle;			/* Handle oder 0 				*/
	char		path[128];		/* "", wenn nicht geîffnet 		*/
	char		maske[13];		/* etwa "*.*" 					*/
	char		title[128];		/* Titelzeile 					*/
	char		info[60];			/* Infozeile 					*/
	GRECT 	out;				/* Fenster- Gesamtmaûe 			*/
	GRECT	in;				/* Fenster- Innenmaûe 			*/
	int		realnum;			/* wahre Anzahl Dateien 			*/
	int		shownum;			/* mit Maske angezeigte Anzahl	*/
	int		too_much;			/* Flag fÅr "zuviele Dateien" 	*/
	int		cols;			/* Objekte/Zeile 				*/
	int		wlins;			/* Anzahl ganz sichtbarer Zeilen 	*/
	int		lins;			/* Anzahl Zeilen 				*/
	int		shift;			/* Nr. der obersten Zeile 		*/
	int		maxshift;			/* Maximales <shift> 			*/
	long		alloc_offs;		/* Offset auf Poolspeicher		*/
	long		alloc_size;		/* Grîûe des Poolspeicherblocks	*/
	MYDTA	*pmydta;			/* Daten 						*/
	OBJECT	*pobj;			/* Zeiger auf dargestellte Objekte */
	ICONBLK	*picb;			/* Zeiger auf Icons 			*/
	MYFNAME	*pstr;			/* Zeiger auf Textzeilen 		*/
} WINDOW;

typedef struct
{
	int 		icontyp;			/* Bildtyp I_FLPDSK...	*/
	int		isdisk;			/* Buchstabe fÅr Disk-Icon oder 0 */
	char		text[80];			/* Text (Diskname, Pfad) */
	ICONBLK	data;
} ICON;

typedef struct
{
	char		path[80];			/* Dateiname mit vollst. Pfad	*/
	char		ext1[4];			/* Erster Dateityp			*/
	char		ext2[4];			/* Zweiter Dateityp			*/
	int		typ;				/* 'GP', ' P', 'PP'			*/
							/* kleines 'p': single mode	*/
} APPLICATION;

extern char	*poolptr;			/* Zeiger auf Pool			*/
extern char	*poolfree;		/* Zeiger auf freien Teil	*/
extern int	poolsize;			/* Anzahl freier EintrÑge	*/

extern WINDOW		fenster[ANZFENSTER+1];
extern ICON		*icon;
extern APPLICATION	*application;

extern char dirty_drives[ANZDRIVES];
extern DISKINFO    dinfo[ANZDRIVES];

enum { OVERWRITE, BACKUP, CONFIRM };

typedef struct
{
	char		sorttyp;
	char 	showtyp;
	char		points;

	char 	is_1col;
	char 	is_groesse;
	char 	is_datum;
	char 	is_zeit;

	char		blitstate;
	char		cnfm_del;		/* Flag: Lîschen bestÑtigen				*/
	char		cnfm_copy;	/* Flag: Kop./Versch. bestÑtigen			*/
	char		mode_ovwr;	/* Modus (0=öb,1=Backup,2=Best.)			*/
	char		check_free;	/* Freien Speicher auf Ziellaufwerk prÅfen	*/
	char		use_fldl;
	char		resident;
	char 	show_all;
	char		clock;
	char		use_cache;
	char		dnam_init;
	char		resvd1;
	char		resvd2;
} DEFAULTS;

extern DEFAULTS status;

typedef struct
{
	char		tmpdrv;
	char		sides;
	char		tracks;
	char		sectors;
	char		interlv;
	char		trkincr;
	char		sidincr;
	char		clustsize;
} FMT_DEFAULTS;

extern FMT_DEFAULTS fmtstat;

typedef struct {
 unsigned		n_pgm_icons;
 unsigned		n_dat_icons;
 unsigned		n_pgm_names;
 unsigned		n_dat_names;
} ICONFILE_HEADER;

extern ICONFILE_HEADER iconfile_header;
extern int  *icf_data;		/* Zeiger auf Speicherblock	*/
extern int  *icf_pgmicn;		/* Daten fÅr Programmicons	*/
extern int  *icf_daticn;		/* Daten fÅr Dateiicons		*/
extern char *icf_pgms;		/* Programmnamen	*/
extern char *icf_fils;		/* Dateitypen		*/

#define PGMICN_OFFS	20
#define DATICN_OFFS 150