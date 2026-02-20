/*
* global definitions
*/

#ifndef WORD
#include <portab.h>
#endif

#define PROGRAM_VERSION_STR "v1.01"

#define MAX_NAMELEN 34
#define MAX_PATHLEN 128
#define MAX_PGMN 300
#define MAX_DATN 300
#define MAX_PTHN 200
#define MAX_SPCN 12
#define MAX_LINN 600
#define MAX_RSCN 30
#define MAX_ICNN 500
#define MAXWINDEFPOS 10

/* Each window has its identification, that is also stored in APPLICAT.INF */

#define IDENT_APPLICATIONS   "APPLICATIONS"
#define IDENT_REGISTER_APP   "REGISTER_APPLICATION"  /*"ANW_ANMELDEN"*/
#define IDENT_PATHS          "PATHS"
#define IDENT_SPECIAL        "SPECIAL"
#define IDENT_ICONS          "ICONS"
#define IDENT_EDIT_FILE_TYPE "EDIT_FILE_TYPE"		/*"DATEITYP_EDIT"*/
#define IDENT_REGISTER_PATH  "REGISTER_PATH"		/*"PFAD_ANMELDEN"*/

/* Section names */

#define SECTION_PATHS        "<Paths>"				/*"[<Pfade>]"*/
#define SECTION_SPECIAL      "<Special>"			/*"[<Spezial>]"*/

/* other */

#define RSC_FNAME_INTERN     "<internal>"			/*"<intern>"*/
#define APP_NAME_FREE        "<free>"				/*"<frei>"*/

typedef struct
{
	char name[32];				/* SchlÅssel (Fenstername) */
	GRECT g;					/* Position */
} WINDEFPOS;

struct dialog_userdata
{
	char *ident;
	int mode;					/* merken, ob Aktion bei HNDL_OPEN */
};

struct iconfile
{
	char fname[MAX_NAMELEN];	/* Dateiname */
	OBJECT *adr_icons;			/* Baum mit Icons */
	int nicons;					/* Anzahl Icons */
	int firsticon;				/* Index in der Icontabelle */
};

struct dat_file
{
	int sel;					/* selektiert */
/*	int sel_icon;		*/		/* Icon selektiert */
	char name[MAX_NAMELEN];		/* Dateityp, "" = unbenutzt */
	char rscname[MAX_NAMELEN];	/* Name der RSC-Datei */
	int rscindex;				/* Nummer des Icons in rsc */
	int iconnr;					/* interne Iconnummer */
	int pgm;					/* zugehîrige Applikation */
	int next;					/* fÅr Verkettung, bis -1 */
};

struct pgm_file
{
	int sel;					/* selektiert */
/*	int sel_icon;		*/		/* Icon selektiert */
	char name[MAX_NAMELEN];	/* Name ohne Ext/ "" = unbenutzt */
	char rscname[MAX_NAMELEN];	/* Name der RSC-Datei */
	int  rscindex;				/* Nummer des Icons in rsc */
	int iconnr;					/* interne Iconnummer */
	char path[MAX_PATHLEN];		/* Voller Pfad */
	int config;					/* Konfigurationsbits */
	long memlimit;				/* Speicherlimit */
	int ntypes;					/* Anzahl angemeldeter Dateien */
	int types;					/* verkettete Liste */
};

struct pth_file
{
	struct pth_file *next;		/* Verkettung fÅr scrollbox */
	WORD sel;					/* selektiert */
/*	int sel_icon;		*/		/* Icon selektiert */
	char path[MAX_PATHLEN];		/* Voller oder rel. Pfad */
	char rscname[MAX_NAMELEN];	/* Name der RSC-Datei */
	int rscindex;				/* Nummer des Icons in rsc */
	int iconnr;					/* interne Icon-Nummer */
};

struct spc_file
{
	struct spc_file *next;		/* Verkettung fÅr scrollbox */
	int sel;					/* selektiert */
/*	int sel_icon;		*/		/* Icon selektiert */
	long key;					/* SchlÅssel fÅr DAT-Datei */
	char name[MAX_NAMELEN];		/* Name in der Dialogbox */
	char rscname[MAX_NAMELEN];	/* Name der RSC-Datei */
	int rscindex;				/* Nummer des Icons in rsc */
	int iconnr;					/* interne Icon-Nummer */
};

struct icon
{
	int rscfile;				/* fÅr versch. Dateien */
	int objnr;					/* Objektnummer */
/*	int sel_pgm;	*/			/* als Programm-Icon selektiert */
/*	int sel_dat;	*/			/* als Datei-Icon selektiert */
};

struct zeile
{
	struct zeile *next;			/* Verkettung fÅr scrollbox */
	WORD sel;					/* selektiert */
	int pgmnr;					/* Nummer des Programmnamens */
	int reldatnr;				/* angemeldete Datei 0..ntypes-1 */
								/*  nix angemeldet: -1 */
	int datnr;					/* absolute Nummer der Datei */
								/*  nix angemeldet: -1 */
};

extern int is_multiwindow;		/* alle Fenster sind offen */

#ifdef NWINDOWS
extern WINDOW *mywindow;
#endif
extern int get_deficonnr(long key);
extern void subobj_wdraw(void *d, int obj, int startob, int depth);
extern void fname_ext(char *s, char *d);
extern int extract_apname(char *path, char *name);
extern DIALOG *xy_wdlg_init(
			HNDL_OBJ hndl_obj,
			OBJECT *tree,
			char *ident,
			int code,
			void *data,
			int title_code
			);
extern void save_dialog_xy(DIALOG *d);
extern long err_alert(long e);

extern int  rsrc_gtree(int gindex, OBJECT **tree);
extern int spcn;
extern int pthn;
extern int datn;
extern int pgmn;
extern int icnn;
extern int linn;
extern int rscn;
extern struct pgm_file	pgmx[MAX_PGMN];
extern struct dat_file	datx[MAX_DATN];
extern struct pth_file	pthx[MAX_PTHN];
extern struct spc_file	spcx[MAX_SPCN];
extern struct icon		icnx[MAX_ICNN];
extern struct zeile		linx[MAX_LINN];
extern struct iconfile	rscx[MAX_RSCN];

extern int is_3d;

extern void error(int id);		/* show alert from resource string */
extern char buf512[512];
