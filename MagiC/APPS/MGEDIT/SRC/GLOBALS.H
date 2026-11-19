#ifndef MGEDIT_GLOBALS_H
#define MGEDIT_GLOBALS_H

#include "MGEDIT_GEM.H"

#define NWINDOWS 64
#define WFLAG_ICONIFIED 1
#define WFLAG_SHADED 2
#define EDITFELD 0

typedef struct _window {
	short handle;
	short flags;
	short save_active;
	char title[129]; /* 127-byte path, dirty marker and terminator. */
	GRECT out;
	GRECT in;
	char path[128];
	char *buf;
	long nlines;
	long yscroll;
	short yvis;
	short ncols;
	short xscroll;
	short xvis;
	long bufsize;
	short dirty;
	OBJECT tree;
	XEDITINFO *xedit;
	short fontID;
	short fontH;
	short fontprop;
	short tcolour;
	short bcolour;
	short tabwidth;
} WINDOW;

typedef union _wmesag {
	short message[8];
	struct {
		short code;
		short dest_apid;
		short is_zero;
		short whdl;
		GRECT g;
	} msg;
} WMESAG;

_Static_assert(sizeof(WMESAG) == 16, "WMESAG must be eight AES words");

extern int close_file(WINDOW *w);
extern int top_whdl(void);
extern WINDOW *windows[NWINDOWS];
extern WINDOW **new_window(void);
extern WINDOW *whdl2window(int whdl);
extern WINDOW **find_slot_window(WINDOW *myw);
extern WINDOW *open_new_window(char *path);
extern void window_message(WINDOW *w, int kstate, short message[8]);
extern void window_set_slider(WINDOW *w);
extern void close_window(WINDOW *w);
extern int update_window(WINDOW *w, GRECT *g);
extern void subobj_wdraw(void *d, int obj, int startob, int depth);
extern void options_dial_init_rsc(void);
extern WORD __CDECL hdl_options(struct HNDL_OBJ_args args);
extern void prefs_were_changed(void);
extern int ncolours;
extern OBJECT *adr_options;
extern OBJECT *adr_colour;
extern void *d_options;
extern GRECT scrg;

extern struct prefs {
	long bufsize;
	short fontID;
	short fontH;
	short fontprop;
	char fontname[128];
	short tcolour;
	short bcolour;
	short tabwidth;
	GRECT prefs_win;
} prefs;

#define MAXWIDTH 40
#define MOVER_HEIGHT 3
#define MOVER_COLOUR G_LWHITE
#define EDITOR_W_KIND (NAME+CLOSER+FULLER+MOVER+SIZER+UPARROW+DNARROW+VSLIDE+HSLIDE+LFARROW+RTARROW)
extern void save_options(void);
extern short dial_font(long *id, long *pt, short *mono, char *name);
extern int scan_values(char **s, int count, short *values);
extern void print_values(char *s, int count, const short *values);

#endif
