#ifndef MGVIEW_WINDOWS_H
#define MGVIEW_WINDOWS_H

#include "GLOBALS.H"

#define WFLAG_ICONIFIED 1

struct _window;

typedef struct _wind_scroll_info
{
	long n;
	long shift;
	int nvis;
	long maxshift;
	void (*set_shift)(struct _window *w, long shift, int is_horiz);
	int pixelsize;
	void (*arrange)(struct _window *w, int is_horiz);
} WINDSCROLLINFO;

typedef struct _window
{
	int handle;
	int flags;
	void (*key)(struct _window *w, int kstate, int key);
	void (*button)(struct _window *w, int kstate, int x, int y,
			int button, int nclicks);
	void (*message)(struct _window *w, int kstate, short message[8]);
	void (*snap)(struct _window *w);
	void (*open)(struct _window *w);
	void (*close)(struct _window *w, int kstate);
	void (*iconified)(struct _window *w, GRECT *g);
	void (*uniconified)(struct _window *w, GRECT *g, int unhide);
	void (*alliconified)(struct _window *w, GRECT *g, int hide);
	void (*fulled)(struct _window *w);
	void (*arrowed)(struct _window *w, int arrow);
	void (*hslid)(struct _window *w, int pos);
	void (*vslid)(struct _window *w, int pos);
	void (*sized)(struct _window *w, GRECT *g);
	void (*moved)(struct _window *w, GRECT *g);
	void (*draw)(struct _window *w, GRECT *g);
	int (*arrange)(struct _window *w);
	char title[128];
	char info[60];
	GRECT out;
	GRECT in;
	WINDSCROLLINFO vscroll;
	WINDSCROLLINFO hscroll;
	long user_fsize;
	unsigned char *user_file;
	unsigned char **user_lines;
	int user_tabsize;
} WINDOW;

typedef union _wmesag
{
	short message[8];
	struct
	{
		short code;
		short dest_apid;
		short is_zero;
		short whdl;
		GRECT g;
	} msg;
} WMESAG;

_Static_assert(sizeof(WMESAG) == 16, "WMESAG must be 16 bytes");

extern int top_whdl(void);
extern WINDOW *windows[NWINDOWS];
extern WINDOW **new_window(void);
extern WINDOW **find_slot_window(WINDOW *myw);
extern WINDOW *whdl2window(int whdl);
extern void window_calc_slider(WINDOW *w, int is_horiz);
extern void init_window(WINDOW *w);
extern int update_window(WINDOW *w);

#endif /* MGVIEW_WINDOWS_H */
