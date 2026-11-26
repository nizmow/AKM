#ifndef MGFORMAT_GEM_H
#define MGFORMAT_GEM_H

/* Public MiNTLib/GEMlib types and the small MagiC DOS compatibility layer
 * used by MGFORMAT.  Keep this application-local; the shared GEMUT_MT_SDK.H
 * header owns only the common MT GEM utility contract. */
#include "GEMUT_MT_SDK.H"
#include "GEMUT_MT.H"
#include <mint/ostruct.h>

#define RMODE_RD 0
#define RMODE_WR 1

#ifndef F_VOLUME
#define F_VOLUME FA_VOLUME
#endif
#ifndef FA_VOLUME
#define FA_VOLUME 0x08
#endif

/* GEMDOS error codes; these are negative results, not positive errno values. */
#ifndef EDRVNR
#define EDRVNR (-2L)
#endif
#ifndef EUNCMD
#define EUNCMD (-3L)
#endif
#ifndef EBADRQ
#define EBADRQ (-5L)
#endif
#ifndef EMEDIA
#define EMEDIA (-7L)
#endif
#ifndef EINVFN
#define EINVFN (-32L)
#endif
#ifndef EFILNF
#define EFILNF (-33L)
#endif
#ifndef EACCDN
#define EACCDN (-36L)
#endif
#ifndef ENSMEM
#define ENSMEM (-39L)
#endif
#ifndef ELOCKED
#define ELOCKED (-58L)
#endif
#ifndef EBREAK
#define EBREAK (-68L)
#endif

#define SC_DOSVARS 2

/* Legacy GEMDOS DTA layout expected by MGFORMAT. */
typedef struct {
	char d_reserved[21];
	unsigned char d_attrib;
	unsigned short d_time;
	unsigned short d_date;
	unsigned long d_length;
	char d_fname[14];
} DTA;

typedef _BPB BPB;

/* MagiC extended appl_write envelope. */
typedef struct {
	WORD dst_apid;
	WORD unique_flg;
	void *attached_mem;
	WORD *msgbuf;
} XAESMSG;

/* MagiC Sconfig(SC_DOSVARS) layout.  Only err_to_str is consumed here, but
 * retaining the pointer offsets avoids depending on a private header. */
typedef struct {
	long res0;
	int *dos_time;
	int *dos_date;
	long res1;
	long res2;
	long res3;
	void *act_pd;
	void *res4;
	short res5;
	void *res6;
	long *res7;
	void (*resv_intmem)(void);
	long (*etv_critic)(void);
	char *((*err_to_str)(char e));
	long res8;
	long res9;
	long res10;
} DOSVARS;

_Static_assert(sizeof(DTA) == 44, "legacy DTA layout");
_Static_assert(offsetof(DTA, d_fname) == 30, "legacy DTA filename offset");
_Static_assert(offsetof(DOSVARS, err_to_str) == 50, "MagiC DOSVARS layout");


/* MagiC's native wdlg API uses AES functions 160-167.  GEMlib's wdlg_*
 * wrappers are pdlg/XaAES entry points (AES 200-207), which MagiC maps to
 * PDLG.SLB and are not available in this environment.  Keep the application
 * on the documented MagiC API and pass the caller's AES global explicitly. */
#undef wdlg_create
#undef wdlg_open
#undef wdlg_close
#undef wdlg_delete
#undef wdlg_get_tree
#undef wdlg_get_handle
#undef wdlg_evnt
#undef wdlg_redraw
#undef wdlg_set_iconify
#undef wdlg_set_uniconify

DIALOG *mg_wdlg_create(HNDL_OBJ handle_exit, OBJECT *tree, void *user_data,
                       WORD code, void *data, WORD flags, WORD *global);
WORD mg_wdlg_open(DIALOG *dialog, const char *title, WORD kind,
                  WORD x, WORD y, WORD code, void *data, WORD *global);
WORD mg_wdlg_close(DIALOG *dialog, WORD *x, WORD *y, WORD *global);
WORD mg_wdlg_delete(DIALOG *dialog, WORD *global);
WORD mg_wdlg_get_tree(DIALOG *dialog, OBJECT **tree, GRECT *r, WORD *global);
WORD mg_wdlg_get_handle(DIALOG *dialog, WORD *global);
WORD mg_wdlg_set_iconify(DIALOG *dialog, GRECT *g, const char *title,
                         OBJECT *tree, WORD obj, WORD *global);
WORD mg_wdlg_set_uniconify(DIALOG *dialog, GRECT *g, const char *title,
                           OBJECT *tree, WORD *global);
WORD mg_wdlg_evnt(DIALOG *dialog, EVNT *events, WORD *global);
void mg_wdlg_redraw(DIALOG *dialog, GRECT *rect, WORD obj, WORD depth,
                    WORD *global);

#define wdlg_create(a,b,c,d,e,f) mg_wdlg_create((a),(b),(c),(d),(e),(f),aes_global)
#define wdlg_open(a,b,c,d,e,f,g) mg_wdlg_open((a),(b),(c),(d),(e),(f),(g),aes_global)
#define wdlg_close(a,b,c) mg_wdlg_close((a),(b),(c),aes_global)
#define wdlg_delete(a) mg_wdlg_delete((a),aes_global)
#define wdlg_get_tree(a,b,c) mg_wdlg_get_tree((a),(b),(c),aes_global)
#define wdlg_get_handle(a) mg_wdlg_get_handle((a),aes_global)
#define wdlg_set_iconify(a,b,c,d,e) mg_wdlg_set_iconify((a),(b),(c),(d),(e),aes_global)
#define wdlg_set_uniconify(a,b,c,d) mg_wdlg_set_uniconify((a),(b),(c),(d),aes_global)
#define wdlg_evnt(a,b) mg_wdlg_evnt((a),(b),aes_global)
#define wdlg_redraw(a,b,c,d) mg_wdlg_redraw((a),(b),(c),(d),aes_global)

#endif
