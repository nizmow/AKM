#define NOTICE_MEM	512		/* Bytes pro Notiz */
#define EDITFELD 0			/* Objekt #0 ist G_EDIT */

extern void subobj_wdraw(void *d, int obj, int startob, int depth);
extern void options_dial_init_rsc( void );
extern WORD __CDECL hdl_options( struct HNDL_OBJ_args args );

extern int ncolours;
extern WINDOW *selected_window;
extern OBJECT *adr_options;
extern OBJECT *adr_colour;
extern void *d_options;
extern GRECT scrg;
extern char *notice_path;

extern struct prefs {
	short fontID;
	short fontH;
	short fontprop;
	char fontname[128];
	short colour;
	GRECT prefs_win;
} prefs;

/* Notiz-Fenster */

#define MAXWIDTH 40		/* Maximale Textbreite in Zeichen */
#define MOVER_HEIGHT 0	/* Höhe des MOVERs in Pixeln */
/* #define MOVER_HEIGHT 3	*/	/* Höhe des MOVERs in Pixeln */
#define MOVER_COLOUR LWHITE	/* Farbe des MOVERs */
#define NOTICE_W_KIND	0
extern void save_options( void );
extern void top_all_my_windows( void );
extern int dial_font( long *id, long *pt, short *mono, char *name );
extern void create_notice( int x, int y );
extern long save_notice( WINDOW *w, char *path );
extern void calc_size_notice_wind( WINDOW *w );
extern unsigned char *get_line(unsigned char *text,
						long n, long *lw );
extern void select_window( WINDOW *w );
extern void deselect_window( WINDOW *w );
extern long open_notice_wind( unsigned char *notice,
					short id,
					GRECT *g,
					short fontID, short font_is_prop, short fontH,
					short colour,
					WINDOW **pw );
long edit_notice_wind( unsigned char *notice, short id,
					WINDOW **pw );