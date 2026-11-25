#define N_OBJS 16

#define TOP_WINOBJS	(NAME+CLOSER+FULLER+BACKDROP+ICONIFIER)
#define RGT_WINOBJS (UPARROW+DNARROW+VSLIDE)
#define BOT_WINOBJS (LFARROW+RTARROW+HSLIDE)

/* WINDOW-Struktur für MagiC-Kernel */

typedef struct {
	WORD state;
	WORD attr;
	void *own;			/* (APPL *) */
	WORD kind;			/* von wind_create() */
	char	*name;			/* Zeiger auf Titelzeile */
	char	*info;			/* Zeiger auf Infozeile	*/
	GRECT curr;
	GRECT prev;
	GRECT full;
	GRECT work;
	GRECT overall;			/* Umriß */
	GRECT unic;
	GRECT min;			/* Minimale Größe */
	WORD	oldheight;		/* alte Höhe vor Shading	*/
	WORD	hslide;		 	/* horizontale Schieberposition */
	WORD vslide; 			/* vertikale Schieberposition */
	WORD hslsize;			/* horizontale Schiebergröße */
	WORD vslsize;			/* vertikale Schiebergröße */
	void *wg;				/* Rechteckliste */
	void *nextwg;			/* nächstes Rechteck der Liste */
	WORD	whdl;
	OBJECT tree[N_OBJS];
	WORD is_sizer;
	WORD is_info;
	WORD is_rgtobjects;
	WORD is_botobjects;
	TEDINFO ted_name;
	TEDINFO ted_info;
} WININFO;

_Static_assert(sizeof(OBJECT) == 24, "OBJECT size");
_Static_assert(offsetof(WININFO, tree) == 94, "WININFO tree offset");
_Static_assert(offsetof(WININFO, is_sizer) == 478, "WININFO sizer flag offset");
_Static_assert(offsetof(WININFO, ted_name) == 486, "WININFO name TEDINFO offset");
_Static_assert(offsetof(WININFO, ted_info) == 514, "WININFO info TEDINFO offset");
_Static_assert(sizeof(WININFO) == 542, "WININFO size");

/* Bits von state */

#define OPENED 1
#define COVERED 2
#define ACTIVE 4
#define LOCKED 8
#define ICONIFIED 32
#define SHADED 64

/* Übergabe-Struktur zum Einklinken */

typedef struct {
	WORD		version;		/* Versionsnummer der Struktur */
	LONG		wsizeof;		/* Größe der WINDOW-Struktur */
	WORD		whshade;		/* Höhe eines ge-shade-ten Fensters */
	void		(*wbm_create)( WININFO *w );
	void		(*wbm_skind)( WININFO *w );
	void		(*wbm_ssize)( WININFO *w );
	void		(*wbm_sslid)( WININFO *w, WORD vertical );
	void		(*wbm_sstr)( WININFO *w );
	void		(*wbm_sattr)( WININFO *w, WORD chbits );
	void		(*wbm_calc)( WORD kind, WORD *fg );
	WORD		(*wbm_obfind)( WININFO *w, WORD x, WORD y );
} WINFRAME_HANDLER;

_Static_assert(offsetof(WINFRAME_HANDLER, wsizeof) == 2, "handler size offset");
_Static_assert(offsetof(WINFRAME_HANDLER, wbm_create) == 8, "handler callback offset");
_Static_assert(sizeof(WINFRAME_HANDLER) == 40, "handler table size");

/* Übergabe-Struktur für globale Fenster-Einstellungen */

typedef struct {
	WORD		flags;
	WORD		h_inw;
	void		*finfo_inw;
} WINFRAME_SETTINGS;

_Static_assert(offsetof(WINFRAME_SETTINGS, finfo_inw) == 4, "settings font offset");

/* Bits von flags */

#define NO_BDROP 1

extern WINFRAME_SETTINGS *settings;
extern WORD h_inw;
extern OBJECT *adr_window;
