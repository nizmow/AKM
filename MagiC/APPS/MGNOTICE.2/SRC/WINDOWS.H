#include "WGLOBALS.H"

#define WFLAG_ICONIFIED	1

struct _window;

typedef struct _wind_scroll_info
{
	long		n;			/* Anzahl Zeilen/Spalten */
	long		shift;		/* Nr der obersten Zeile, linksten Spalte */
	int		nvis;		/* Anzahl sichtbarer Zeilen/Spalten */
	long		maxshift;		/* Abhängig von n und nvis */
	void		(*set_shift)	(struct _window *w, long shift,
							int is_horiz);
	int		pixelsize;	/* Pixel pro Objekt */
	void		(*arrange)	(struct _window *w, int is_horiz);
} WINDSCROLLINFO;

typedef struct _window
{
	short		handle;			/* Handle oder 0 				*/
	short		flags;			/* Bit 0: Ikonifiziert			*/
	void		(*key)		(struct _window *w, short kstate, short key);
	void		(*button)		(struct _window *w, int nclicks, EVNTDATA *ev );
	void		(*message)	(struct _window *w, short kstate, short message[8]);
	void		(*open)		(struct _window *w);
	void		(*close)		(struct _window *w, int kstate);
	void		(*iconified)	(struct _window *w, GRECT *g);
	void		(*uniconified)	(struct _window *w, GRECT *g, int unhide);
	void		(*alliconified)(struct _window *w, GRECT *g, int hide);
	void		(*fulled)		(struct _window *w);
	void		(*sized)		(struct _window *w, GRECT *g);
	void		(*moved)		(struct _window *w, GRECT *g);
	int		(*arrange)	(struct _window *w);
	char		title[128];		/* Titelzeile 					*/
	char		info[60];			/* Infozeile 					*/
	GRECT 	out;				/* Fenster- Gesamtmaße 			*/
	GRECT	in;				/* Fenster- Innenmaße 			*/
	/* hier Benutzerdaten */
	XEDITINFO	*xedit;
	OBJECT	tree;			/* nur ein OBJECT */
	short		id_code;			/* eindeutige Nummer */
	short		selected;			/* Fenster selektiert ? */
	short		position_dirty;	/* muß noch gespeichert werden */
	char		*buf;			/* Puffer */
	long		bufsize;			/* Puffergröße */
	short		tabwidth;
	short		fontH;
	short		fontID;
	short		fontprop;			/* proportionaler Zeichensatz */
	short		charW;
	short		charH;
	short		bcolour;			/* Hintergrundfarbe */
	short		tcolour;			/* Textfarbe */
} WINDOW;

typedef union _wmesag 
{
	short		message[8];
	struct	{
		short	code;
		short	dest_apid;
		short	is_zero;
		short	whdl;
		GRECT g;
			}
			msg;
} WMESAG;

_Static_assert(sizeof(WMESAG) == 16, "WMESAG must be eight AES words");


/* Für Fenster */
extern int	top_whdl( void );
extern WINDOW *windows[NWINDOWS];
extern WINDOW **new_window( void );
extern WINDOW **find_slot_window( WINDOW *myw );
extern WINDOW *whdl2window(int whdl);
extern void init_window( WINDOW *w);
extern int update_window(WINDOW *w);
