/**************************************************
*
* ALLGEMEINE INCLUDES UND DEFINITIONEN
*
**************************************************/


#define LANG_GER 1
#define LANG_FRA 2		/* not supported, yet */
#define LANG_ENG 3
#define LANG_SPA 4		/* not supported, yet */
#define LANG_ITA 5		/* not supported, yet */
#define LANG_SWE 6		/* not supported, yet */


#define LANGUAGE LANG_GER

#define MAGIXDESK 0		/* TRUE: MagiX- Desktop kreieren */
#define DEBUG 0

#include <mt_aes.h>

#if MAGIXDESK
#include "magxdesk.h"
#else
#include "kaosdesk.h"
#endif

#define TRUE   1
#define FALSE  0
#define EOS    '\0'
#define MIN(a,b) ((a < b) ? a : b)
#define MAX(a,b) ((a > b) ? a : b)
#define ABS(X) ((X>0) ? X : -X)
#define VA_START 0x4711


/**************************************************
*
* EIGENE FEHLERCODES
*
**************************************************/

#define ENOWND -100			/* kein Fenster verfÅgbar */


/**************************************************
*
* GLOBALE VARIABLEN
*
**************************************************/

#define SCREEN     0
#define FENSTERTYP (NAME+CLOSER+FULLER+MOVER+INFO+SIZER+UPARROW+DNARROW+VSLIDE+HSLIDE)

#define NUM_DRIVES      16			/* Maximale Anzahl Laufwerke */
#define NUM_PROGRAMS    14			/* Anzahl Dateien in "Programme" */
#define NUM_WINDOWS     5			/* Anzahl der Fenster */
#define PLUSICONS       10			/* Anzahl freier Desktop- Icons */
#define PLUSAPPS        10			/* noch anmeldbare Anwendungen */
#define NUM_FILES_TOTAL 600			/* Dateien insgesamt in allen Fenstern */
#define TTPLEN          63

#define PGMICN_OFFS 20
#define DATICN_OFFS 150

typedef struct
{
	int           number;		/* Eingangsreihenfolge */
	unsigned char flags;		/* Bit 0: "paût zu Muster" */
								/* Bit 1: versch (selected) */
								/* Bit 2: ist Programm */
	unsigned char icontyp;
	unsigned int  time;
	unsigned int  date;
	unsigned long filesize;
	char          filename[13];
	unsigned char attrib;
} MYDTA;

typedef struct
{
	char name[45];
} MYFNAME;

typedef struct
{
	int     handle;			/* Handle oder 0 				*/
	char    path[128];		/* "", wenn nicht geîffnet 		*/
	char    maske[13];		/* etwa "*.*" 					*/
	char    title[128];		/* Titelzeile 					*/
	char    info[60];		/* Infozeile 					*/
	GRECT   out;			/* Fenster- Gesamtmaûe 			*/
	GRECT   in;				/* Fenster- Innenmaûe 			*/
	int		realnum;		/* wahre Anzahl Dateien 		*/
	int		shownum;		/* mit Maske angezeigte Anzahl	*/
	int		too_much;		/* Flag fÅr "zuviele Dateien" 	*/
	int		cols;			/* Objekte/Zeile 				*/
	int		wlins;			/* Anzahl ganz sichtbarer Zeilen*/
	int		lins;			/* Anzahl Zeilen 				*/
	int		shift;			/* Nr. der obersten Zeile 		*/
	int     maxshift;		/* Maximales <shift> 			*/
	long    alloc_offs;		/* Offset auf Poolspeicher		*/
	long    alloc_size;		/* Grîûe des Poolspeicherblocks	*/
	MYDTA   *pmydta;		/* Daten 						*/
	OBJECT  *pobj;			/* Zeiger auf dargestellte Objekte */
	ICONBLK *picb;			/* Zeiger auf Icons 			*/
	MYFNAME *pstr;			/* Zeiger auf Textzeilen 		*/
} WINDOW;

typedef struct
{
	int     icontyp;		/* Bildtyp I_FLPDSK...	*/
	int     isdisk;			/* Buchstabe fÅr Disk-Icon oder 0 */
	char    text[80];		/* Text (Diskname, Pfad) */
	ICONBLK data;
} ICON;

typedef struct
{
	char path[80];			/* Dateiname mit vollst. Pfad	*/
	char ext1[4];			/* Erster Dateityp			*/
	char ext2[4];			/* Zweiter Dateityp			*/
	int typ;				/* 'GP', ' P', 'PP'			*/
							/* kleines 'p': single mode	*/
} APPLICATION;

extern char	*poolptr;		/* Zeiger auf Pool			*/
extern char	*poolfree;		/* Zeiger auf freien Teil	*/
extern int	poolsize;		/* Anzahl freier EintrÑge	*/

extern WINDOW fenster[NUM_WINDOWS + 1];
extern ICON *icon;
extern APPLICATION	*application;

extern char dirty_drives[NUM_DRIVES];
extern DISKINFO dinfo[NUM_DRIVES];

enum { OVERWRITE, BACKUP, CONFIRM };

typedef struct
{
	char sorttyp;
	char showtyp;
	char points;

	char is_1col;
	char is_groesse;
	char is_datum;
	char is_zeit;

	char blitstate;
	char cnfm_del;		/* Flag: Lîschen bestÑtigen				*/
	char cnfm_copy;		/* Flag: Kop./Versch. bestÑtigen			*/
	char mode_ovwr;		/* Modus (0=öb,1=Backup,2=Best.)			*/
	char check_free;	/* Freien Speicher auf Ziellaufwerk prÅfen	*/
	char use_fldl;
	char resident;
	char show_all;
	char clock;
	char use_cache;
	char dnam_init;
	char resvd1;
	char resvd2;
} DEFAULTS;

extern DEFAULTS status;

typedef struct
{
	char tmpdrv;
	char sides;
	char tracks;
	char sectors;
	char interlv;
	char trkincr;
	char sidincr;
	char clustsize;
} FMT_DEFAULTS;

extern FMT_DEFAULTS fmtstat;

typedef struct
{
	unsigned n_pgm_icons;
	unsigned n_dat_icons;
	unsigned n_pgm_names;
	unsigned n_dat_names;
} ICONFILE_HEADER;


/* ALLG */

#if MAGIXDESK==0
extern const char *err_string(int e);
#endif
extern char *get_name(char *path);
extern long err_alert(long e);
extern void Mgraf_mouse(int type);
extern int suffixtyp(char *s);
extern int fname_match(char *fname, char *fmask);
extern void date_to_str(char *s, unsigned int date);
extern void time_to_str(char *s, unsigned int time);
extern void fname_int(char *s, char *d);
extern void fname_ext(char *s, char *d);
extern void drv_to_str(char *s, char c);
extern char *print_ul(unsigned long num, unsigned long factor, char *p);
extern void objc_grect(OBJECT *tree, int objn, GRECT *g);
extern void tree_sel_grect(OBJECT *tree, GRECT *g);
extern void grect_toggle(OBJECT *tree, GRECT *g);
extern int find_obj(OBJECT *tree, int x, int y);
extern int rc_intersect	(GRECT *p1, GRECT *p2);
extern int walk_sel(int wnr, int (*tue)(int wnr, int obj));

/* KDSTART */

extern char *_iconfile;		/* aus dem Startmodul */

/* KAOSDESK */

extern USERBLK userblk;
extern int ap_id;
extern GRECT desk_g;			/* Maûe fÅr Hintergrund */
extern char	*err_file;
extern int deflt_topwnr;
extern int char_w, char_h;
extern int vdi_handle;
extern int n_icons, n_apps;
extern OBJECT *adr_icons;
extern OBJECT *adr_prgrms;
extern OBJECT *adr_hauptmen;
extern OBJECT *adr_cpmvdl;
extern OBJECT *adr_laufwe;
extern OBJECT *adr_einste;
extern OBJECT *adr_ttppar;
extern OBJECT *adr_format;
extern OBJECT *adr_fmtopt;
extern OBJECT *adr_cpydsk;
extern OBJECT *adr_about;
extern OBJECT *adr_datinf;
extern OBJECT *adr_ordinf;
extern int vdi_device;
extern int *aes_global;
extern int gl_hhchar;
extern int gl_hwchar;
extern const char *pgm_ver;
extern ICONFILE_HEADER iconfile_header;
extern int  *icf_data;		/* Zeiger auf Speicherblock	*/
extern int  *icf_pgmicn;		/* Daten fÅr Programmicons	*/
extern int  *icf_daticn;		/* Daten fÅr Dateiicons		*/
extern char *icf_pgms;		/* Programmnamen	*/
extern char *icf_fils;		/* Dateitypen		*/
#if MAGIXDESK == 0
extern int ismagix;		/* Multitasking - GEM */
extern void _ende(int code);
extern int isgem2;
extern int iskgem;		/* KAOS 1.4 */
#endif
extern int hdl_to_fnr(int whdl);
extern void get_char_dim(void);
extern int selected(OBJECT *tree, int which);
extern void	ob_sel(OBJECT *tree, int which);
extern void	ob_dsel(OBJECT *tree, int which);
extern void ob_sel_dsel(OBJECT *tree, int which, int sel);
extern void objs_hide(OBJECT *tree, ...);
extern void objs_unhide(OBJECT *tree, ...);
extern void set_deflt_clip(void);
extern void redraw(int wnr, GRECT *neu);
extern int icsel(int *rwnr, int *objn);
extern int in_grect(int x, int y, GRECT *g);
extern void close_all_wind(void);
extern void open_all_wind(void);
void subobj_draw(OBJECT *tree, int obj, int n, char *s);

/* KLICK */

extern int make_icon(int typ, char c, char *icntext, char *path, int x, int y);
extern void kill_icon(int objnr);
extern void dl_icns(int wnr);
extern void prt_icns(int wnr, int print);
extern void cpmv_icns(int wnr, char *zielpfad, int kbsh);
extern int is_dest(int wnr, int objnr);
extern void obj_malen(int wnr, int index);
extern void dsel_all(void);
extern void handle_mouse_button(int anzahl, const EVNTDATA *ev);
extern void sel_all(int wnr);
extern int obj_to_path(int wnr, int obj, char *path, MYDTA **f);

/* DKLICK */

extern APPLICATION *find_defapp(char *pgmpath);
extern void init_vt52(void);
extern void exit_vt52(void);
extern void str_conout(char *s, int len);
extern APPLICATION *file_to_defapp(char *filepath);
extern void opn_newwnd(char *path);
extern int dclick(int wnr, int obj, int kbsh);
extern long setpath(char *program, int stdpath, int isover);
extern unsigned char waitkey(void);
extern int show_file(char *path);
extern int print_file(char *path);
extern int dclick(int wnr, int obj, int kbsh);
extern void starten(char *path, int typ, int wnr, int obj, int kbsh);
extern void exit_vt52(void);
extern int obj_typ(int wnr, int obj, char *path, MYDTA **f, int *typ);
#if	MAGIXDESK
extern void res_exec(void);
#else
extern long	res_exec(int doex, int isgr, char *program,
		  			    COMMAND *param, char *envptr);
#endif

/* WINDOWS */

extern int top_wnr(void);
extern void topped(int whdl);
extern void untopped(int whdl);
extern void closed(int whdl, int kstate);
extern void	fulled(int wnr);
extern void	arrowed(int wnr, int arrow);
extern void	hslid(int whdl);
extern void	vslid(int whdl, int newpos);
extern void	sized(int whdl, GRECT *g);
extern void	moved(int whdl, GRECT *g);
extern void	newtop(int whdl);
extern void	close_immed(int wnr);
extern void	upd_show(void);
extern void	upd_is(void);
extern void	upd_sort(void);
extern void	upd_col(void);
extern void upd_drives(void);
extern void upd_infos(void);
extern void	upd_path(int wnr, int free_flag);
extern void upd_mask(int wnr);
extern long opn_wnd(int wnr, int fast);
extern void init_icnobj(OBJECT *o, ICONBLK *icn, int typ, char *text);
extern int find_iconfile_dat(char *fname);
extern int find_iconfile_pgm(char *fname);
extern void get_syshdr(void);
extern void set_dname(int lw, char *dname);
extern void dirty_info( int wnr );

/* SHELSORT.O */

extern void shelsort(void *base, size_t count, size_t size, int (*compar)());

/* SPEC */

#if	MAGIXDESK == 0
extern void set_parent_paths(void);
#endif
extern void free_drive(int drv);
extern int change_res(int dev, int txt);
extern long label(int lw, char *name);
extern long read_bootsec
(
	int lw, long *ser, int *bps, int *spc,
	int *res, int *nfats, int *ndirs, int *nsects,
	int *media, int *spf, int *spt, int *nsides,
	int *nhid, int *exec
);
extern long diskcopy
(
	int slw, int dlw, int tmp, int sid, int trk, int spt,
	int fmt, void (*feedb)(char *action, int n)
);
extern long format_floppy
(
	int dev, unsigned sid, unsigned trk, unsigned spt, unsigned clu,
	int inter, int vtrk, int vsid,
	void (*counter)(int n), int is_logic, char *dname
);
extern long getHz200(void);
extern unsigned get_dclick_interval_ms(void);
extern char chg_msg[];
extern long cache_on (char *path);
extern long cache_off(char *path);
extern void set_colors(void);
extern long long MDfree( DISKINFO *ds, int driveno);

/* DEFAULTS */

extern void save_status(int is_inf);
#if MAGIXDESK
extern void	reload_status(int drv);
#else
extern void	reload_status(int is_inf, int drv);
#endif
extern char inf_name[];
extern void load_icons(void);

/* DIALOGS */

extern int dial_datexi(char *name, int move_flag, int *do_redraw);
extern int dial_ordexi(char *name, int *do_redraw);
extern int dial_ttppar(char *pgm, char *par);
extern long memsize(void);
extern void dial_about(void);
extern void dial_info(void);
extern void dial_neuord(void);
extern void dial_format(void);
extern void dial_maske(void);
extern void dial_laufwe(void);
extern void dial_anwndg(void);
extern void dial_einste(void);
extern void dial_arbsic(void);
extern void dial_cpydsk(void);
extern void dial_prgrms(void);

/* FILES */

extern int copy_move_files(char *quell, char *ziel, int move_flag, int ispath);
extern int delete_files(char *path, int ispath);
extern int qbreak(void);
extern long confirm(int wnr, int function, int ja, char *zielpfad);
extern void end_confirm(void);
extern long read_dname(int lw, char *name);
extern int disk_type(int drvnr);
extern void set_dirty(long err, char *path, char val);
extern long GDcreate(char *path, char *name);
extern long walk_path(char *path, long *clusters, long *nbytes, long *hbytes,
			int *folders, int *nfiles, int *hfiles,
			int attr_msk, long clsize,
			long (*before_cb)(char *path, DTA *file),
			long (*after_cb) (char *path, DTA *file));
