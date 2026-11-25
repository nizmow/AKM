#ifndef EDITOB_H
#define EDITOB_H

#include "EDIT_GEM.H"

#define EDITERR_BUFFER_FULL (-200L)

typedef struct {
	LONG magic;
	unsigned char *buf;
	unsigned char *tmplt;
	unsigned char *valid;
	LONG buflen;
	LONG curr_tlen;
	LONG nlines;
	LONG yscroll;
	WORD ob_w;
	WORD ob_h;
	WORD autowrap;
	WORD max_linew;
	WORD tabwidth;
	WORD bcolour;
	WORD tcolour;
	WORD fontID;
	WORD fontH;
	WORD fontPix;
	WORD mono;
	WORD dirty;
	WORD charW;
	WORD charH;
	WORD lvis;
	WORD leff;
	struct _xed_li {
		unsigned char *line;
		LONG len;
		WORD lextra;
	} *lines;
	WORD xscroll;
	unsigned char *tcurs;
	WORD curs_hidecnt;
	WORD ccurs_x;
	WORD ccurs_y;
	WORD pcurs_x;
	WORD pcurs_y;
	WORD pcursth_x;
	unsigned char *bsel;
	unsigned char *esel;
} EDITINFO;

extern WORD __CDECL xeditob_userdef(PARMBLK *pb);
extern EDITINFO *xedit_create(void);
extern WORD xedit_open(OBJECT *ob, EDITINFO *xi);
extern void xedit_close(EDITINFO *xi);
extern void xedit_delete(EDITINFO *xi);
extern WORD xedit_evnt(OBJECT *tree, WORD obj, WORD whdl, EVNT *ev, EDITINFO *xi, LONG *errcode);
extern WORD xedit_cursor(OBJECT *tree, WORD obj, WORD whdl, WORD show, EDITINFO *xi);
extern void xedit_set_buf(EDITINFO *xi, unsigned char *buf, LONG buflen);
extern void xedit_set_format(EDITINFO *xi, WORD tabwidth, WORD autowrap);
extern WORD xedit_set_font(EDITINFO *xi, WORD fontID, WORD fontH, WORD fontPix, WORD mono);
extern unsigned char *xedit_get_cursor(EDITINFO *xi);
extern WORD xedit_set_cursor(OBJECT *tree, WORD obj, WORD whdl, EDITINFO *xi, unsigned char *s);
extern void xedit_set_scroll_and_cpos(EDITINFO *xi, WORD xscroll, LONG yscroll, unsigned char *cyscroll, unsigned char *curpos, WORD cx, WORD cy);
extern WORD xedit_scroll(OBJECT *tree, WORD obj, WORD whdl, EDITINFO *xi, LONG yscroll, WORD xscroll);
extern WORD xedit_resized(OBJECT *ob, EDITINFO *xi, WORD *oldrh, WORD *newrh);
extern WORD init_ftextob(OBJECT *tree, WORD obj, USERBLK *ub, TEDINFO *te);

#endif
