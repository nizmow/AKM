/*
	MT_AES.H

	GEM AES Definitions
*/


#if  !defined(__AES__)
#define __AES__
#define __MTAES__

#include <stdint.h>

typedef int32_t fixed;			/* 16.16 vorzeichenbehaftet */

/****** GEMparams *******************************************************/

typedef struct
{
	int16_t	*contrl;
	int16_t	*global;
	int16_t	*intin;
	int16_t	*intout;
	void	**addrin;
	void	**addrout;
} AESPB;

typedef struct
{
	int16_t	contrl[5];
	int16_t	intin[16];
	int16_t	intout[16];
	void	*addrin[16];
	void	*addrout[16];
} PARMDATA;

typedef struct		/* wird nur fÅr den Haupt-Thread verwendet */
{
	int16_t	contrl[15];
	int16_t	global[15];
	int16_t	intin[16];
	int16_t	intout[16];
	void	*addrin[16];
	void	*addrout[16];
} GEMPARBLK;

typedef struct
{
	int16_t	x;
	int16_t	y;
	int16_t	bstate;
	int16_t	kstate;
} EVNTDATA;

typedef struct
{
	int16_t	g_x;
	int16_t	g_y;
	int16_t	g_w;
	int16_t	g_h;
} GRECT;

/* Low Level Interface */

extern void _mt_aes(PARMDATA *d, int16_t *ctrldata, int16_t *global);

/* MultiTOS Drag&Drop definitions */

#define DD_OK		0
#define DD_NAK		1
#define DD_EXT		2
#define DD_LEN		3

#define DD_TIMEOUT	4000		/* Timeout in ms */

#define DD_NUMEXTS	8		/* Anzahl der Formate */
#define DD_EXTSIZE	32		/* LÑnge des Formatfelds */

#define DD_FNAME	"U:\\PIPE\\DRAGDROP.AA"
#define DD_NAMEMAX	128		/* maximale LÑnge eines Formatnamens */
#define DD_HDRMIN	9		/* minimale LÑnge des Drag&Drop-Headers */
#define DD_HDRMAX	(8 + DD_NAMEMAX)	/* maximale LÑnge */



extern int16_t	_app;
extern void	_crystal(AESPB *aespb);
extern GEMPARBLK _GemParBlk;

/****** Application definitions *****************************************/

/* extended appl_write structure */

typedef struct
{
	int16_t	dst_apid;
	int16_t	unique_flg;
	void	*attached_mem;
	int16_t	*msgbuf;
} XAESMSG;

extern void _appl_yield(void);								/* TOS */
extern int16_t MT_appl_init(int16_t *global);
extern int16_t MT_appl_read(int16_t ap_rid, int16_t ap_rlength, void *ap_rpbuff, int16_t *global);
extern int16_t MT_appl_write(int16_t ap_wid, int16_t ap_wlength, void *ap_wpbuff, int16_t *global);
extern int16_t MT_appl_find(char *ap_fpname, int16_t *global);
extern int16_t MT_appl_tplay(void *ap_tpmem, int16_t ap_tpnum, int16_t ap_tpscale, int16_t *global);
extern int16_t MT_appl_trecord(void *ap_trmem, int16_t ap_trcount, int16_t *global);
extern int16_t MT_appl_exit(int16_t *global);
extern int16_t MT_appl_search(int16_t ap_smode, char *ap_sname, int16_t *ap_stype, int16_t *ap_sid, int16_t *global);
extern void MT_appl_yield(int16_t *global);						/* GEM 2.x */


#define	AES_MOUSE 8
extern int16_t MT_appl_getinfo(int16_t ap_gtype, int16_t *ap_gout1, int16_t *ap_gout2, int16_t *ap_gout3, int16_t *ap_gout4, int16_t *global);

#define	appl_init() \
			MT_appl_init(0L)
#define	appl_read(a, b, c) \
			MT_appl_read(a, b, c, 0L)
#define	appl_write(a, b, c) \
			MT_appl_write(a, b, c, 0L)
#define	appl_find(a) \
			MT_appl_find(a, 0L)
#define	appl_tplay(a, b, c) \
			MT_appl_tplay(a, b, c, 0L)
#define	appl_trecord(a, b) \
			MT_appl_trecord(a, b, 0L)
#define	appl_exit() \
			MT_appl_exit(0L)
#define	appl_search(a, b, c, d) \
			MT_appl_search(a, b, c, d, 0L)
#define	appl_getinfo(a, b, c, d, e) \
			MT_appl_getinfo(a, b, c, d, e, 0L)
#define	appl_yield() \
			MT_appl_yield(0L)

#define	appl_bvset(disks, harddisks) /* Funktion ignorieren (GEM fÅr Dose): void appl_bvset(int16_t disks, int16_t harddisks); */

#define	vq_aes() (appl_init() >= 0)	/* int16_t	vq_aes(void); */


/****** Event definitions ***********************************************/

#define MU_KEYBD        0x0001
#define MU_BUTTON       0x0002
#define MU_M1           0x0004
#define MU_M2           0x0008
#define MU_MESAG        0x0010
#define MU_TIMER        0x0020

#define MN_SELECTED     10
#define WM_REDRAW       20
#define WM_TOPPED       21
#define WM_CLOSED       22
#define WM_FULLED       23
#define WM_ARROWED      24
#define WM_HSLID        25
#define WM_VSLID        26
#define WM_SIZED        27
#define WM_MOVED        28
#define WM_NEWTOP       29
#define WM_UNTOPPED      30                                 /* GEM  2.x    */
#define WM_ONTOP         31                                 /* AES 4.0     */
#define WM_BOTTOMED      33                                 /* AES 4.1     */
#define WM_ICONIFY       34                                 /* AES 4.1     */
#define WM_UNICONIFY     35                                 /* AES 4.1     */
#define WM_ALLICONIFY    36                                 /* AES 4.1     */
#define AC_OPEN		40
#define AC_CLOSE		41
#define CT_UPDATE		50
#define CT_MOVE		51
#define CT_NEWTOP		52
#define CT_KEY			53
#define AP_TERM          50                                 /* AES 4.0     */
#define AP_TFAIL         51                                 /* AES 4.0     */
#define AP_RESCHG        57                                 /* AES 4.0     */
#define SHUT_COMPLETED   60                                 /* AES 4.0     */
#define RESCH_COMPLETED  61                                 /* AES 4.0     */
#define AP_DRAGDROP      63                                 /* AES 4.0     */
#define SH_WDRAW         72                                 /* MultiTOS    */
#define SC_CHANGED       80                                 /* */
#define PRN_CHANGED		82
#define FNT_CHANGED		83
#define THR_EXIT		88							/* MagiC 4.5	*/
#define PA_EXIT          89                                 /* MagiC 3     */
#define CH_EXIT          90                                 /* MultiTOS    */
#define WM_M_BDROPPED    100                                /* KAOS 1.4    */
#define SM_M_SPECIAL     101                                /* MAG!X       */
#define SM_M_RES2        102                                /* MAG!X       */
#define SM_M_RES3        103                                /* MAG!X       */
#define SM_M_RES4        104                                /* MAG!X       */
#define SM_M_RES5        105                                /* MAG!X       */
#define SM_M_RES6        106                                /* MAG!X       */
#define SM_M_RES7        107                                /* MAG!X       */
#define SM_M_RES8        108                                /* MAG!X       */
#define SM_M_RES9        109                                /* MAG!X       */
#define WM_SHADED		22360						/* [WM_SHADED apid 0 win 0 0 0 0] */
#define WM_UNSHADED		22361						/* [WM_UNSHADED apid 0 win 0 0 0 0] */

/* SM_M_SPECIAL codes */

#define SMC_TIDY_UP		0							/* MagiC 2	*/
#define SMC_TERMINATE	1							/* MagiC 2	*/
#define SMC_SWITCH		2							/* MagiC 2	*/
#define SMC_FREEZE		3							/* MagiC 2	*/
#define SMC_UNFREEZE	4							/* MagiC 2	*/
#define SMC_RES5		5							/* MagiC 2	*/
#define SMC_UNHIDEALL	6							/* MagiC 3.1	*/
#define SMC_HIDEOTHERS	7							/* MagiC 3.1	*/
#define SMC_HIDEACT		8							/* MagiC 3.1	*/


/* Keybord states */

#define K_RSHIFT        0x0001
#define K_LSHIFT        0x0002
#define K_CTRL          0x0004
#define K_ALT           0x0008

typedef struct		/* Mausrechteck fÅr EVNT_multi() */
{
	int16_t	m_out;
	int16_t	m_x;
	int16_t	m_y;
	int16_t	m_w;
	int16_t	m_h;
} MOBLK;

typedef struct		/* Ereignisstruktur fÅr EVNT_multi(), Fensterdialoge, etc. */
{
	int16_t	mwhich;
	int16_t	mx;
	int16_t	my;
	int16_t	mbutton;
	int16_t	kstate;
	int16_t	key;
	int16_t	mclicks;
	int16_t	reserved[9];
	int16_t	msg[16];
} EVNT;

extern int16_t MT_evnt_keybd(int16_t *global);
extern int16_t MT_evnt_button(int16_t nclicks, int16_t bmask, int16_t bstate, EVNTDATA *ev, int16_t *global);
extern int16_t MT_evnt_mouse(int16_t flg_leave, GRECT *g, EVNTDATA *ev, int16_t *global);
extern int16_t MT_evnt_mesag(int16_t *ev_mgpbuff, int16_t *global);
extern int16_t MT_evnt_timer(uint32_t ms, int16_t *global);

extern int16_t MT_evnt_multi(
			int16_t evtypes,
			int16_t nclicks, int16_t bmask, int16_t bstate,
			int16_t flg1_leave, GRECT *g1,
			int16_t flg2_leave, GRECT *g2,
			int16_t *msgbuf,
			uint32_t ms,
			EVNTDATA *ev,
			int16_t *keycode,
			int16_t *nbclicks,
			int16_t *global);

extern void MT_EVNT_multi(int16_t evtypes, int16_t nclicks, int16_t bmask, int16_t bstate,
					MOBLK *m1, MOBLK *m2, uint32_t ms, EVNT *event, int16_t *global);

extern int16_t MT_evnt_dclick(int16_t ev_dnew, int16_t ev_dgetset, int16_t *global);

#define	evnt_keybd() \
			MT_evnt_keybd(0L)
#define	evnt_button_evntdata(a, b, c, d) \
			MT_evnt_button(a, b, c, d, 0L)
#define	evnt_mouse(a, b, c) \
			MT_evnt_mouse(a, b, c, 0L)
#define	evnt_mesag(a) \
			MT_evnt_mesag(a, 0L)
#define	evnt_timer(a) \
			MT_evnt_timer(a, 0L)
#define	evnt_multi(a, b, c, d, e, f, g, h, i, j, k, l, m) \
			MT_evnt_multi(a, b, c, d, e, f, g, h, i, j, k, l, m, 0L)
#define	EVNT_multi(a, b, c, d, e, f, g, h) \
			MT_EVNT_multi(a, b, c, d, e, f, g, h, 0L)
#define	evnt_dclick(a, b) \
			MT_evnt_dclick(a, b, 0L)

/****** Object definitions **********************************************/

#define G_BOX			20
#define G_TEXT			21
#define G_BOXTEXT		22
#define G_IMAGE		23
#define G_USERDEF		24
#define G_IBOX			25
#define G_BUTTON		26
#define G_BOXCHAR		27
#define G_STRING		28
#define G_FTEXT		29
#define G_FBOXTEXT		30
#define G_ICON			31
#define G_TITLE		32
#define G_CICON		33
#define G_SWBUTTON       34                                 /* MAG!X       */
#define G_POPUP          35                                 /* MAG!X       */
#define G_RESVD1		36							/* MagiC 3.1	*/
#define G_SHORTCUT		38							/* MagiC 6	*/


/* Object flags */

#define NONE         0x0000
 #define OF_NONE NONE         /* compatibility with newer source code */
#define SELECTABLE   0x0001
 #define OF_SELECTABLE SELECTABLE
#define DEFAULT      0x0002
 #define OF_DEFAULT DEFAULT
#define EXIT         0x0004
 #define OF_EXIT EXIT
#define EDITABLE     0x0008
 #define OF_EDITABLE EDITABLE
#define RBUTTON      0x0010
 #define OF_RBUTTON RBUTTON 
#define LASTOB       0x0020
 #define OF_LASTOB LASTOB
#define TOUCHEXIT    0x0040
 #define OF_TOUCHEXIT TOUCHEXIT
#define HIDETREE     0x0080
 #define OF_HIDETREE HIDETREE
#define INDIRECT     0x0100
 #define OF_INDIRECT INDIRECT
#ifndef FL3DMASK
#define FL3DMASK     0x0600
#define FL3DNONE     0x0000
#define FL3DIND      0x0200	/* 3D Indicator			  AES 4.0		*/
#define FL3DBAK      0x0400	/* 3D Background			  AES 4.0		*/
#define FL3DACT      0x0600	/* 3D Activator			  AES 4.0		*/
#endif
#define SUBMENU      0x0800
 #define OF_SUBMENU SUBMENU

/* Object states */

#define NORMAL   0x00
 #define OS_NORMAL NORMAL              /* compatibility with newer source code */
#define SELECTED 0x01
 #define OS_SELECTED SELECTED
#define CROSSED  0x02
 #define OS_CROSSED CROSSED 
#define CHECKED  0x04
 #define OS_CHECKED CHECKED 
#define DISABLED 0x08
 #define OS_DISABLED DISABLED
#define OUTLINED 0x10
 #define OS_OUTLINED OUTLINED
#define SHADOWED 0x20
 #define OS_SHADOWED SHADOWED
#define WHITEBAK 0x40  /* TOS     */
 #define OS_WHITEBAK WHITEBAK
#define DRAW3D   0x80  /* GEM 2.x */
 #define OS_DRAW3D DRAW3D

/* objc_sysvar */

#ifndef LK3DIND
#define LK3DIND      1                                      /* AES 4.0     */
#define LK3DACT      2                                      /* AES 4.0     */
#define INDBUTCOL    3                                      /* AES 4.0     */
#define ACTBUTCOL    4                                      /* AES 4.0     */
#define BACKGRCOL    5                                      /* AES 4.0     */
#define AD3DVALUE    6                                      /* AES 4.0     */
#define MX_ENABLE3D  10                                     /* MagiC 3.0   */
#endif


typedef struct	/* form_xdo definitions */
{
	char scancode;
	char nclicks;
	int16_t  objnr;
} SCANX;

typedef struct
{
	SCANX *unsh;
	SCANX *shift;
	SCANX *ctrl;
 	SCANX *alt;
	void  *resvd;
} XDO_INF;

typedef struct _xted /* scrollable textedit objects */
{
	char *xte_ptmplt;
	char *xte_pvalid;
	int16_t xte_vislen;
	int16_t xte_scroll;
} XTED;


/* Object colors */
#if !defined(__COLORS)
#define __COLORS /*
                 using both AES colours and BGI colours
                 is not possible
                 */

#define WHITE            0
 #define G_WHITE WHITE      /* compatibility with newer source code */
#define BLACK            1
 #define G_BLACK BLACK
#define RED              2
 #define G_RED RED
#define GREEN            3
#define BLUE             4
#define CYAN             5
#define YELLOW           6
#define MAGENTA          7
#define LWHITE           8
#define LBLACK           9
 #define G_LBLACK LBLACK
#define LRED            10
#define LGREEN          11
#define LBLUE           12
#define LCYAN           13
#define LYELLOW         14
#define LMAGENTA        15

#endif

#define ROOT             0
#define MAX_LEN         81              /* max string length */
#define MAX_DEPTH        8              /* max depth of search or draw */


#define IBM             3               /* font types */
#define SMALL           5


/* editable text justification */
#define TE_LEFT         0
#define TE_RIGHT        1
#define TE_CNTR         2

/* editable text field definitions */
#define ED_START 0
#define ED_INIT  1
#define ED_CHAR  2
#define ED_END   3
#define ED_CRSR  100 /* MAG!X       */
#define ED_DRAW  103 /* MAG!X 2.00  */


typedef struct orect
{
	struct orect *o_link;
	int16_t o_x;
	int16_t o_y;
	int16_t o_w;
	int16_t o_h;
} ORECT;

/* bfobspec.textmode/bfcolspec.textmode: */

#define TEXT_TRANSPARENT	0
#define TEXT_OPAQUE		1

/* TEDINFO.te_colour: */

typedef struct
{
	unsigned framecol    :  4;
	unsigned textcol     :  4;
	unsigned textmode    :  1;
	unsigned fillpattern :  3;
	unsigned interiorcol :  4;
} bfcolspec;

/* Object structures */

typedef struct
{
	char    *te_ptext;        /* ptr to text (must be 1st)    */
	char    *te_ptmplt;       /* ptr to template              */
	char    *te_pvalid;       /* ptr to validation            */
	int16_t te_font;          /* font                         */
	int16_t te_junk1;         /* junk int16_t                     */
	int16_t te_just;          /* justification: left, right...*/
	union {
		int16_t     	te_color;        /* colour information            */
		bfcolspec te_colspec;
	} te_ucolor;
	int16_t te_junk2;         /* junk int16_t                     */
	int16_t te_thickness;     /* border thickness             */
	int16_t te_txtlen;        /* text string length           */
	int16_t te_tmplen;        /* template string length       */
} TEDINFO;


typedef struct
{
	int16_t *ib_pmask;
	int16_t *ib_pdata;
	char    *ib_ptext;
	int16_t  ib_char;
	int16_t  ib_xchar;
	int16_t  ib_ychar;
	int16_t  ib_xicon;
	int16_t  ib_yicon;
	int16_t  ib_wicon;
	int16_t  ib_hicon;
	int16_t  ib_xtext;
	int16_t  ib_ytext;
	int16_t  ib_wtext;
	int16_t  ib_htext;
} ICONBLK;


typedef struct cicon_data {
	int16_t  num_planes;       /* number of planes in the following data */
	int16_t *col_data;        /* pointer to color bitmap in standard form */
	int16_t *col_mask;        /* pointer to single plane mask of col_data */
	int16_t *sel_data;        /* pointer to color bitmap of selected icon */
	int16_t *sel_mask;        /* pointer to single plane mask of selected icon */
	struct cicon_data *next_res;  /* pointer to next icon for a different resolution */
} CICON;

typedef struct cicon_blk {
     ICONBLK  monoblk;	       /* default monochrome icon */
     CICON    *mainlist;	       /* list of color icons for different resolutions */
} CICONBLK;

typedef struct
{
	int16_t *bi_pdata;        /* ptr to bit forms data        */
	int16_t  bi_wb;            /* width of form in bytes       */
	int16_t  bi_hl;            /* height in lines              */
	int16_t  bi_x;             /* source x in bit form         */
	int16_t  bi_y;             /* source y in bit form         */
	int16_t  bi_color;         /* foreground color             */
} BITBLK;


struct __parmblk;

#ifndef __STDC__        /*
                         *      using this structure is not possible
                         *      if ANSI keywords only is ON
                        */
typedef struct
{
	int16_t cdecl (*ub_code)(struct __parmblk *parmblock);
	int32_t ub_parm;
} USERBLK;

#endif

typedef struct
{
	unsigned character   :  8;
	signed   framesize   :  8;
	unsigned framecol    :  4;
	unsigned textcol     :  4;
	unsigned textmode    :  1;
	unsigned fillpattern :  3;
	unsigned interiorcol :  4;
} bfobspec;

typedef union obspecptr
{
	int32_t  index;
	union obspecptr *indirect;
	bfobspec obspec;
	TEDINFO  *tedinfo;
	ICONBLK  *iconblk;
	CICONBLK *ciconblk;
	BITBLK   *bitblk;
#ifndef __STDC__
	USERBLK  *userblk;
#endif
	char     *free_string;
} OBSPEC;


typedef struct
{
	int16_t  ob_next;          /* -> object's next sibling     */
	int16_t  ob_head;          /* -> head of object's children */
	int16_t  ob_tail;          /* -> tail of object's children */
	uint16_t ob_type;          /* object type: BOX, CHAR,...   */
	uint16_t ob_flags;         /* object flags                 */
	uint16_t ob_state;         /* state: SELECTED, OPEN, ...   */
	OBSPEC   ob_spec;          /* "out": -> anything else      */
	int16_t  ob_x;             /* upper left corner of object  */
	int16_t  ob_y;             /* upper left corner of object  */
	int16_t  ob_width;         /* object width                 */
	int16_t  ob_height;        /* object height                */
} OBJECT;


typedef struct __parmblk
{
	OBJECT  *pb_tree;
	int16_t  pb_obj;
	int16_t  pb_prevstate;
	int16_t  pb_currstate;
	int16_t  pb_x, pb_y, pb_w, pb_h;
	int16_t  pb_xc, pb_yc, pb_wc, pb_hc;
	int32_t  pb_parm;
} PARMBLK;


typedef struct
{
	OBJECT  *mn_tree;
	int16_t  mn_menu;
	int16_t  mn_item;
	int16_t  mn_scroll;
	int16_t  mn_keystate;
} MENU;


typedef struct
{
	int32_t Display;
	int32_t Drag;
	int32_t Delay;
	int32_t Speed;
	int16_t Height;
} MN_SET;


typedef struct
{
	char    *string;          /* etwa "TOS|KAOS|MAG!X"          */
	int16_t  num;              /* Nr. der aktuellen Zeichenkette */
	int16_t  maxnum;           /* maximal erlaubtes <num>        */
} SWINFO;

typedef struct
{
	OBJECT  *tree;            /* Popup- MenÅ                    */
	int16_t obnum;            /* aktuelles Objekt von <tree>    */
} POPINFO;

/****** Menu definitions ************************************************/

/* menu_bar modes */

#define MENU_HIDE  0    /* TOS   */
#define MENU_SHOW  1    /* TOS   */
#define MENU_INSTL 100  /* MAG!X */

extern int16_t MT_menu_bar(OBJECT *me_btree, int16_t me_bshow, int16_t *global);
extern int16_t MT_menu_icheck(OBJECT *me_ctree, int16_t me_citem, int16_t me_ccheck, int16_t *global);
extern int16_t MT_menu_ienable(OBJECT *me_etree, int16_t me_eitem, int16_t me_eenable, int16_t *global);
extern int16_t MT_menu_tnormal(OBJECT *me_ntree, int16_t me_ntitle, int16_t me_nnormal, int16_t *global);
extern int16_t MT_menu_text(OBJECT *tree, int16_t objnr, const char *text, int16_t *global);
extern int16_t MT_menu_register(int16_t apid, const char *text, int16_t *global);
extern int16_t MT_menu_popup(MENU *me_menu, int16_t me_xpos, int16_t me_ypos, MENU *me_mdata, int16_t *global);
extern int16_t MT_menu_attach(int16_t me_flag, OBJECT *me_tree, int16_t me_item, MENU *me_mdata, int16_t *global);
extern int16_t MT_menu_istart(int16_t me_flag, OBJECT *me_tree, int16_t me_imenu, int16_t me_item, int16_t *global);
extern int16_t MT_menu_settings(int16_t flag, MN_SET *values, int16_t *global);
extern int16_t MT_menu_unregister(int16_t menuid, int16_t *global);
extern int16_t MT_menu_click(int16_t val, int16_t setit, int16_t *global);                 /* GEM 3.x     */

#define	menu_bar(a, b) \
			MT_menu_bar(a, b, 0L)
#define	menu_icheck(a, b, c) \
			MT_menu_icheck(a, b, c, 0L)
#define	menu_ienable(a, b, c) \
			MT_menu_ienable(a, b, c, 0L)
#define	menu_tnormal(a, b, c) \
			MT_menu_tnormal(a, b, c, 0L)
#define	menu_text(a, b, c) \
			MT_menu_text(a, b, c, 0L)
#define	menu_register(a, b) \
			MT_menu_register(a, b, 0L)
#define	menu_popup(a, b, c, d) \
			MT_menu_popup(a, b, c, d, 0L)
#define	menu_attach(a, b, c, d) \
			MT_menu_attach(a, b, c, d, 0L)
#define	menu_istart(a, b, c, d) \
			MT_menu_istart(a, b, c, d, 0L)
#define	menu_settings(a, b) \
			MT_menu_settings(a, b, 0L)
#define	menu_unregister(a) \
			MT_menu_unregister(a, 0L)
#define	menu_click(a, b) \
			MT_menu_click(a, b, 0L)



/* Object prototypes */

extern int16_t MT_objc_add(OBJECT *ob_atree, int16_t ob_aparent, int16_t ob_achild, int16_t *global);
extern int16_t MT_objc_delete(OBJECT *ob_dltree, int16_t ob_dlobject, int16_t *global);
extern int16_t MT_objc_draw(OBJECT *tree, int16_t start, int16_t depth, const GRECT *g, int16_t *global);
extern int16_t MT_objc_find(OBJECT *ob_ftree, int16_t ob_fstartob, int16_t ob_fdepth, int16_t ob_fmx, int16_t ob_fmy, int16_t *global);
extern int16_t MT_objc_offset(OBJECT *ob_oftree, int16_t ob_ofobject, int16_t *ob_ofxoff, int16_t *ob_ofyoff, int16_t *global);
extern int16_t MT_objc_order(OBJECT *ob_ortree, int16_t ob_orobject, int16_t ob_ornewpos, int16_t *global);
extern int16_t MT_objc_edit(OBJECT *ob_edtree, int16_t ob_edobject, int16_t ob_edchar, int16_t *ob_edidx, int16_t ob_edkind, int16_t *global);
extern int16_t MT_objc_xedit(OBJECT *tree, int16_t objnr, int16_t key, int16_t *cursor_xpos, int16_t subfn, GRECT *r, int16_t *global);
extern int16_t MT_objc_change(OBJECT *tree, int16_t objnr, int16_t resvd, const GRECT *g, int16_t newstate, int16_t redraw, int16_t *global);
extern int16_t MT_objc_sysvar(int16_t mode, int16_t which, int16_t ival1, int16_t ival2, int16_t *oval1, int16_t *oval2, int16_t *global);	/* AES 4.0 */

extern void MT_objc_wdraw(OBJECT *tree, int16_t object, int16_t depth, GRECT *clip,
					int16_t windowhandle, int16_t *global);
extern void MT_objc_wchange(OBJECT *tree, int16_t object, int16_t newstate,
					GRECT *clip, int16_t windowhandle,
					int16_t *global);
extern int16_t MT_graf_wwatchbox(OBJECT *tree, int16_t object, int16_t instate,
					int16_t outstate,	int16_t windowhandle,
					int16_t *global);
extern int16_t MT_form_wbutton(OBJECT *tree, int16_t object, int16_t nclicks,
					int16_t *nextob, int16_t windowhandle,
					int16_t *global);
extern int16_t MT_form_wkeybd(OBJECT *tree, int16_t object, int16_t nextob,
					int16_t ichar, int16_t *onextob, int16_t *ochar,
					int16_t windowhandle,
					int16_t *global);
extern int16_t MT_objc_wedit(OBJECT *tree, int16_t object, int16_t edchar,
					int16_t *didx, int16_t kind, int16_t windowhandle,
					int16_t *global);

#define	objc_add(a, b, c) \
			MT_objc_add(a, b, c, 0L)
#define	objc_delete(a, b) \
			MT_objc_delete(a, b, 0L)
#define	objc_draw_grect(a, b, c, d) \
			MT_objc_draw(a, b, c, d, 0L)
#define	objc_find(a, b, c, d, e) \
			MT_objc_find(a, b, c, d, e, 0L)
#define	objc_offset(a, b, c, d) \
			MT_objc_offset(a, b, c, d, 0L)
#define	objc_order(a, b, c) \
			MT_objc_order(a, b, c, 0L)
#define	objc_edit(a, b, c, d, e) \
			MT_objc_edit(a, b, c, d, e, 0L)
#define	objc_xedit(a, b, c, d, e, f) \
			MT_objc_xedit(a, b, c, d, e, f, 0L)
#define	objc_change_grect(a, b, c, d, e, f) \
			MT_objc_change(a, b, c, d, e, f, 0L)
#define	objc_sysvar(a, b, c, d, e, f) \
			MT_objc_sysvar(a, b, c, d, e, f, 0L)

#define	objc_wdraw(a, b, c, d, e) \
			MT_objc_wdraw(a, b, c, d, e, 0L)
#define	objc_wchange(a, b, c, d, e) \
			MT_objc_wchange(a, b, c, d, e, 0L)
#define	graf_wwatchbox(a, b, c, d, e) \
			MT_graf_wwatchbox(a, b, c, d, e, 0L)
#define	form_wbutton(a, b, c, d, e) \
			MT_form_wbutton(a, b, c, d, e, 0L)
#define	form_wkeybd(a, b, c, d, e, f, g) \
			MT_form_wkeybd(a, b, c, d, e, f, g, 0L)
#define	objc_wedit(a, b, c, d, e, f) \
			MT_objc_wedit(a, b, c, d, e, f, 0L)

/****** Form definitions ************************************************/

#define FMD_START  0
#define FMD_GROW   1
#define FMD_SHRINK 2
#define FMD_FINISH 3

extern int16_t MT_form_do(OBJECT *tree, int16_t startob, int16_t *global);
extern int16_t MT_form_xdo(OBJECT *tree, int16_t startob, int16_t *lastcrsr, XDO_INF *tabs, void *flydial, int16_t *global);	/* MAG!X */
extern int16_t MT_form_dial(int16_t subfn, const GRECT *lg, const GRECT *bg, int16_t *global);
extern int16_t MT_form_xdial(int16_t subfn, const GRECT *lg, const GRECT *bg, void **flyinf, int16_t *global);
extern int16_t MT_form_alert(int16_t fo_adefbttn, const char *fo_astring, int16_t *global);
extern int16_t MT_form_error(int16_t fo_enum, int16_t *global);
extern int16_t MT_form_center(OBJECT *tree, GRECT *g, int16_t *global);
extern int16_t MT_form_keybd(OBJECT *fo_ktree, int16_t fo_kobject,
					int16_t fo_kobnext, int16_t fo_kchar,
					int16_t *fo_knxtobject, int16_t *fo_knxtchar, int16_t *global);
extern int16_t MT_form_button(OBJECT *fo_btree, int16_t fo_bobject, int16_t fo_bclicks, int16_t *fo_bnxtobj, int16_t *global);
extern int16_t MT_form_popup(OBJECT *tree, int16_t x, int16_t y, int16_t *global);		/* MAG!X */

typedef	int16_t	(cdecl init_popup_tf)(OBJECT *tree, int16_t scrollpos, int16_t nlines, void *param);
extern int16_t MT_xfrm_popup /* MagiC 5.03 */
(
	OBJECT *tree,
	int16_t x,
	int16_t y,
	int16_t firstscrlob,
	int16_t lastscrlob,
	int16_t nlines,
	init_popup_tf *init_f,
	void *param,
	int16_t *lastscrlpos,
	int16_t *global
);
extern int16_t MT_form_xerr(long errcode, const char *errfile, int16_t *global);        /* MAG!X */

#define	form_do(a, b) \
			MT_form_do(a, b, 0L)
#define	form_xdo(a, b, c, d, e) \
			MT_form_xdo(a, b, c, d, e, 0L)
#define	form_dial_grect(a, b, c) \
			MT_form_dial(a, b, c, 0L)
#define	form_xdial_grect(a, b, c, d) \
			MT_form_xdial(a, b, c, d, 0L)
#define	form_alert(a, b) \
			MT_form_alert(a, b, 0L)
#define	form_error(a) \
			MT_form_error(a, 0L)
#define	form_center_grect(a, b) \
			MT_form_center(a, b, 0L)
#define	form_keybd(a, b, c, d, e, f) \
			MT_form_keybd(a, b, c, d, e, f, 0L)
#define	form_button(a, b, c, d) \
			MT_form_button(a, b, c, d, 0L)
#define	form_popup(a, b, c) \
			MT_form_popup(a, b, c, 0L)
#define	xfrm_popup(a, b, c, d, e, f, g, h, i) \
			MT_xfrm_popup(a, b, c, d, e, f, g, h, i, 0L)
#define	form_xerr(a, b) \
			MT_form_xerr(a, b, 0L)

/****** Graph definitions ************************************************/


/* Mouse forms */

#define	ARROW				0
#define	TEXT_CRSR		1
#define	HOURGLASS		2
#define	BUSYBEE			2
#define	POINT_HAND		3
#define	FLAT_HAND		4
#define	THIN_CROSS		5
#define	THICK_CROSS		6
#define	OUTLN_CROSS		7
#define	USER_DEF			255
#define	M_OFF				256
#define	M_ON				257
#define	M_SAVE			258
#define	M_RESTORE		259

/* Mouse form definition block */

typedef struct mfstr
{
	int16_t mf_xhot;
	int16_t mf_yhot;
	int16_t mf_nplanes;
	int16_t mf_fg;
	int16_t mf_bg;
	int16_t mf_mask[16];
	int16_t mf_data[16];
} MFORM;



extern int16_t MT_graf_rubberbox(int16_t gr_rx, int16_t gr_ry, int16_t gr_minwidth,
                    int16_t gr_minheight, int16_t *gr_rlastwidth,
                    int16_t *gr_rlastheight, int16_t *global);
extern int16_t MT_graf_dragbox(int16_t w, int16_t h, int16_t begx, int16_t begy, GRECT *g, int16_t *endx, int16_t *endy, int16_t *global);
extern int16_t MT_graf_movebox(int16_t gr_mwidth, int16_t gr_mheight, int16_t gr_msourcex, int16_t gr_msourcey, int16_t gr_mdestx, int16_t gr_mdesty, int16_t *global);
extern int16_t MT_graf_growbox(const GRECT *startg, const GRECT *endg, int16_t *global);
extern int16_t MT_graf_shrinkbox(const GRECT *endg, const GRECT *startg, int16_t *global);
extern int16_t MT_graf_watchbox(const OBJECT *tree, int16_t obj, int16_t instate, int16_t outstate, int16_t *global);
extern int16_t MT_graf_slidebox(OBJECT *gr_slptree, int16_t gr_slparent, int16_t gr_slobject, int16_t gr_slvh, int16_t *global);
extern int16_t MT_graf_handle
(
	int16_t *gr_hwchar,
	int16_t *gr_hhchar,
	int16_t *gr_hwbox,
	int16_t *gr_hhbox,
	int16_t *global
);
extern int16_t MT_graf_xhandle    /* KAOS 1.4    */
(
	int16_t *wchar,
	int16_t *hchar,
	int16_t *wbox,
	int16_t *hbox,
	int16_t *dev,
	int16_t *global
);
extern int16_t MT_graf_mouse(int16_t gr_monumber, MFORM *gr_mofaddr, int16_t *global);
extern int16_t MT_graf_mkstate(EVNTDATA *ev, int16_t *global);

#define	graf_rubbox(a, b, c, d, e, f) \
			MT_graf_rubberbox(a, b, c, d, e, f, 0L)
#define	graf_dragbox(a, b, c, d, e, f, g) \
			MT_graf_dragbox(a, b, c, d, e, f, g, 0L)
#define	graf_movebox(a, b, c, d, e, f) \
			MT_graf_movebox(a, b, c, d, e, f, 0L)
#define	graf_growbox_grect(a, b) \
			MT_graf_growbox(a,b,0L)
#define	graf_shrinkbox_grect(a, b) \
			MT_graf_shrinkbox(a, b, 0L)
#define	graf_watchbox(a, b, c, d) \
			MT_graf_watchbox(a, b, c, d, 0L)
#define	graf_slidebox(a, b, c, d) \
			MT_graf_slidebox(a, b, c, d, 0L)
#define	graf_handle(a, b, c, d) \
			MT_graf_handle(a, b, c, d, 0L)
#define	graf_xhandle(a, b, c, d, e) \
			MT_graf_xhandle(a, b, c, d, e, 0L)
#define	graf_mouse(a, b) \
			MT_graf_mouse(a, b, 0L)
#define	graf_mkstate_evntdata(a) \
			MT_graf_mkstate(a, 0L)

/****** Scrap definitions ***********************************************/

int16_t MT_scrp_read(char *sc_rpscrap, int16_t *global);
int16_t MT_scrp_write(char *sc_wpscrap, int16_t *global);
int16_t MT_scrp_clear(int16_t *global);                               /* GEM 2.x */

#define	scrp_read(a) \
			MT_scrp_read(a, 0L)
#define	scrp_write(a) \
			MT_scrp_write(a, 0L)
#define	scrp_clear() \
			MT_scrp_clear(0L)

/****** File selector definitions ***************************************/


extern int16_t MT_fsel_input(char *path, char *name, int16_t *button, int16_t *global);
extern int16_t MT_fsel_exinput(char *path, char *name, int16_t *button, char *label, int16_t *global);

#define	fsel_input(a, b, c) \
			MT_fsel_input(a, b, c, 0L)
#define	fsel_exinput(a, b, c, d) \
			MT_fsel_exinput(a, b, c, d, 0L)

/****** Window definitions **********************************************/

#define NAME    0x0001
#define CLOSER  0x0002
#define FULLER  0x0004
#define MOVER   0x0008
#define INFO    0x0010
#define SIZER   0x0020
#define UPARROW 0x0040
#define DNARROW 0x0080
#define VSLIDE  0x0100
#define LFARROW 0x0200
#define RTARROW 0x0400
#define HSLIDE  0x0800
#define HOTCLOSEBOX 0x1000                             /* GEM 2.x */
#define BACKDROP    0x2000                             /* KAOS 1.4 */
#define ICONIFIER   0x4000                             /* AES 4.1 */
#define SMALLER     ICONIFIER


/* AES wind_s/get()- Modes */

#define WF_KIND          1
#define WF_NAME          2
#define WF_INFO          3
#define WF_WORKXYWH      4
#define WF_CURRXYWH      5
#define WF_PREVXYWH      6
#define WF_FULLXYWH      7
#define WF_HSLIDE        8
#define WF_VSLIDE        9
#define WF_TOP			10
#define WF_FIRSTXYWH	11
#define WF_NEXTXYWH		12
#define WF_RESVD		13
#define WF_NEWDESK		14
#define WF_HSLSIZE		15
#define WF_VSLSIZE		16
#define WF_SCREEN		17
#define WF_COLOR		18
#define WF_DCOLOR		19
#define WF_OWNER		20
#define WF_BEVENT		24
#define WF_BOTTOM		25
#define WF_ICONIFY       26                                 /* AES 4.1     */
#define WF_UNICONIFY     27                                 /* AES 4.1     */
#define WF_UNICONIFYXYWH 28                                 /* AES 4.1     */
#define WF_M_BACKDROP    100                                /* KAOS 1.4    */
#define WF_M_OWNER       101                                /* KAOS 1.4    */
#define WF_M_WINDLIST    102                                /* KAOS 1.4    */
#define WF_MINXYWH       103                                /* MagiC 6     */
#define WF_INFOXYWH      104							/* MagiC 6.10	*/
#define WF_SHADE		0x575d						/* WINX 2.3	*/
#define WF_STACK		0x575e						/* WINX 2.3	*/
#define WF_TOPALL		0x575f						/* WINX 2.3	*/
#define WF_BOTTOMALL	0x5760						/* WINX 2.3	*/

/* wind_set(WF_DCOLOR) */

#define W_BOX       0
#define W_TITLE     1
#define W_CLOSER    2
#define W_NAME      3
#define W_FULLER    4
#define W_INFO      5
#define W_DATA      6
#define W_WORK      7
#define W_SIZER     8
#define W_VBAR      9
#define W_UPARROW  10
#define W_DNARROW  11
#define W_VSLIDE   12
#define W_VELEV    13
#define W_HBAR     14
#define W_LFARROW  15
#define W_RTARROW  16
#define W_HSLIDE   17
#define W_HELEV    18
#define W_SMALLER  19                                 /* AES 4.1     */
#define W_BOTTOMER 20                                 /* MagiC 3     */

/* wind_set(WF_BEVENT) */

#define BEVENT_WORK 0x0001						/* AES 4.0	*/
#define BEVENT_INFO 0x0002						/* MagiC 6	*/

/* Window Messages */

#define WA_UPPAGE 0
#define WA_DNPAGE 1
#define WA_UPLINE 2
#define WA_DNLINE 3
#define WA_LFPAGE 4
#define WA_RTPAGE 5
#define WA_LFLINE 6
#define WA_RTLINE 7

#define WC_BORDER 0                     /* wind calc flags */
#define WC_WORK   1

#define END_UPDATE 0                    /* update flags */
#define BEG_UPDATE 1
#define END_MCTRL  2
#define BEG_MCTRL  3

extern int16_t MT_wind_create(int16_t kind, const GRECT *maxsize, int16_t *global);
extern int16_t MT_wind_open(int16_t whdl, GRECT *g, int16_t *global);
extern int16_t MT_wind_close(int16_t whdl, int16_t *global);
extern int16_t MT_wind_delete(int16_t whdl, int16_t *global);
extern int16_t MT_wind_get(int16_t whdl, int16_t subfn, int16_t *g1, int16_t *g2, int16_t *g3, int16_t *g4, int16_t *global);
extern int16_t MT_wind_get_grect(int16_t whdl, int16_t subfn, GRECT *g, int16_t *global);
extern int16_t MT_wind_get_ptr(int16_t whdl, int16_t subfn, void **v, int16_t *global);
extern int16_t MT_wind_get_int(int16_t whdl, int16_t subfn, int16_t *g1, int16_t *global);
extern int16_t MT_wind_set(int16_t whdl, int16_t subfn, int16_t g1, int16_t g2, int16_t g3, int16_t g4, int16_t *global);
extern int16_t MT_wind_set_string(int16_t whdl, int16_t subfn, const char *s, int16_t *global);
extern int16_t MT_wind_set_grect(int16_t whdl, int16_t subfn, GRECT *g, int16_t *global);
extern int16_t MT_wind_set_ptr_int(int16_t whdl, int16_t subfn, void *s, int16_t i, int16_t *global);
extern int16_t MT_wind_set_int(int16_t whdl, int16_t subfn, int16_t g1, int16_t *global);
extern int16_t MT_wind_find(int16_t wi_fmx, int16_t wi_fmy, int16_t *global);
extern int16_t MT_wind_update(int16_t wi_ubegend, int16_t *global);
extern int16_t MT_wind_calc(int16_t subfn, int16_t kind, const GRECT *in_g, GRECT *out_g, int16_t *global);
extern void	    MT_wind_new(int16_t *global);

#define	wind_create_grect(a, b) \
			MT_wind_create(a, b, 0L)
#define	wind_open_grect(a, b) \
			MT_wind_open(a, b, 0L)
#define	wind_close(a) \
			MT_wind_close(a, 0L)
#define	wind_delete(a) \
			MT_wind_delete(a,0L)
#define	wind_get(a, b, c, d, e, f) \
			MT_wind_get(a, b, c, d, e, f, 0L)
#define	wind_get_grect(a, b, c) \
			MT_wind_get_grect(a, b, c, 0L)
#define	wind_get_ptr(a, b, c) \
			MT_wind_get_ptr(a, b, c, 0L)
#define	wind_get_int(a, b, c) \
			MT_wind_get_int(a, b, c, 0L)
#define	wind_set(a, b, c, d, e, f) \
			MT_wind_set(a, b, c, d, e, f, 0L)
#define	wind_set_str(a, b, c) \
			MT_wind_set_string(a, b, c, 0L)
#define	wind_set_int(a, b, c) \
			MT_wind_set_int(a, b, c, 0L)
#define	wind_set_grect(a, b, c) \
			MT_wind_set_grect(a, b, c, 0L)
#define	wind_set_ptr_int(a, b, c, d) \
			MT_wind_set_ptr_int(a, b, c, d, 0L)
#define	wind_find(a, b) \
			MT_wind_find(a, b, 0L)
#define	wind_update(a) \
			MT_wind_update(a, 0L)
#define	wind_calc_grect(a, b, c, d) \
			MT_wind_calc(a, b, c, d, 0L)

/****** Resource definitions ********************************************/

                                /* data structure types */
#define R_TREE      0
#define R_OBJECT    1
#define R_TEDINFO   2
#define R_ICONBLK   3
#define R_BITBLK    4
#define R_STRING    5              /* gets pointer to free strings */
#define R_IMAGEDATA 6              /* gets pointer to free images */
#define R_OBSPEC    7
#define R_TEPTEXT   8              /* sub ptrs in TEDINFO */
#define R_TEPTMPLT  9
#define R_TEPVALID  10
#define R_IBPMASK   11              /* sub ptrs in ICONBLK */
#define R_IBPDATA   12
#define R_IBPTEXT   13
#define R_BIPDATA   14              /* sub ptrs in BITBLK */
#define R_FRSTR     15              /* gets addr of ptr to free strings */
#define R_FRIMG     16              /* gets addr of ptr to free images  */

typedef struct rshdr
{
	uint16_t rsh_vrsn;
	uint16_t rsh_object;
	uint16_t rsh_tedinfo;
	uint16_t rsh_iconblk;    /* list of ICONBLKS */
	uint16_t rsh_bitblk;
	uint16_t rsh_frstr;
	uint16_t rsh_string;
	uint16_t rsh_imdata;     /* image data */
	uint16_t rsh_frimg;
	uint16_t rsh_trindex;
	uint16_t rsh_nobs;       /* counts of various structs */
	uint16_t rsh_ntree;
	uint16_t rsh_nted;
	uint16_t rsh_nib;
	uint16_t rsh_nbb;
	uint16_t rsh_nstring;
	uint16_t rsh_nimages;
	uint16_t rsh_rssize;     /* total bytes in resource */
} RSHDR;

extern int16_t MT_rsrc_load(char *filename, int16_t *global);
extern int16_t MT_rsrc_free(int16_t *global);
extern int16_t MT_rsrc_gaddr(int16_t type, int16_t index, void *addr, int16_t *global);
extern int16_t MT_rsrc_saddr(int16_t type, int16_t index, void *o, int16_t *global);
extern int16_t MT_rsrc_obfix(OBJECT *re_otree, int16_t re_oobject, int16_t *global);
extern int16_t MT_rsrc_rcfix(RSHDR *rsh, int16_t *global);

#define	rsrc_load(a) \
			MT_rsrc_load(a, 0L)
#define	rsrc_free() \
			MT_rsrc_free(0L)
#define	rsrc_gaddr(a, b, c) \
			MT_rsrc_gaddr(a, b, c, 0L)
#define	rsrc_saddr(a, b, c) \
			MT_rsrc_saddr(a, b, c, 0L)
#define	rsrc_obfix(a, b) \
			MT_rsrc_obfix(a, b, 0L)
#define	rsrc_rcfix(a) \
			MT_rsrc_rcfix(a, 0L)

/****** Shell definitions ***********************************************/

/* tail for default shell */

typedef struct
{
	int16_t  dummy;                   /* ein Nullwort               */
	uint32_t magic;                   /* 'SHEL', wenn ist Shell     */
	int16_t  isfirst;                 /* erster Aufruf der Shell    */
	int32_t  lasterr;                 /* letzter Fehler             */
	int16_t  wasgr;                   /* Programm war Grafikapp.    */
} SHELTAIL;

/* shel_write modes for parameter "isover" */

#define SHW_IMMED        0                                  /* PC-GEM 2.x  */
#define SHW_CHAIN        1                                  /* TOS         */
#define SHW_DOS          2                                  /* PC-GEM 2.x  */
#define SHW_PARALLEL     100                                /* MAG!X       */
#define SHW_SINGLE       101                                /* MAG!X       */

/* shel_write modes for parameter "doex" */

#define SHW_NOEXEC       0
#define SHW_EXEC         1
#define SHW_EXEC_ACC	3							/* AES 3.3	*/
#define SHW_SHUTDOWN     4                                  /* AES 3.3     */
#define SHW_RESCHNG      5                                  /* AES 3.3     */
#define SHW_BROADCAST    7                                  /* AES 4.0     */
#define SHW_INFRECGN     9                                  /* AES 4.0     */
#define SHW_AESSEND      10                                 /* AES 4.0     */
#define SHW_THR_CREATE	20							/* MagiC 4.5	*/

/* extended shel_write() modes and parameter structure */

#define SHW_XMDLIMIT	256
#define SHW_XMDNICE		512
#define SHW_XMDDEFDIR	1024
#define SHW_XMDENV		2048
#define SHW_XMDFLAGS	4096

typedef struct {
	char *command;
	long limit;
	long nice;
	char *defdir;
	char *env;
	long flags;		/* ab MagiC 6 */
} XSHW_COMMAND;

typedef struct {
	int32_t cdecl (*proc)(void *par);
	void *user_stack;
	uint32_t stacksize;
	int16_t mode;		/* immer auf 0 setzen! */
	int32_t res1;		/* immer auf 0L setzen! */
} THREADINFO;

extern int16_t MT_shel_read(char *sh_rpcmd, char *sh_rptail, int16_t *global);
extern int16_t MT_shel_write(int16_t sh_wdoex, int16_t sh_wisgr, int16_t sh_wiscr,
                char *sh_wpcmd, char *sh_wptail, int16_t *global);
extern int16_t MT_shel_get(char *sh_gaddr, uint16_t sh_glen, int16_t *global);
extern int16_t MT_shel_put(char *sh_paddr, uint16_t sh_plen, int16_t *global);
extern int16_t MT_shel_find(char *sh_fpbuff, int16_t *global);
extern int16_t MT_shel_envrn(char **sh_epvalue, char *sh_eparm, int16_t *global);
extern void	MT_shel_rdef(char *cmd, char *dir, int16_t *global);               /* GEM 2.x     */
extern void	MT_shel_wdef(char *cmd, char *dir, int16_t *global);               /* GEM 2.x     */

#define	shel_read(a, b) \
			MT_shel_read(a, b, 0L)
#define	shel_write(a, b, c, d, e) \
			MT_shel_write(a, b, c, d, e, 0L)
#define	shel_get(a, b) \
			MT_shel_get(a, b, 0L)
#define	shel_put(a, b) \
			MT_shel_put(a, b, 0L)
#define	shel_find(a) \
			MT_shel_find(a, 0L)
#define	shel_envrn(a, b) \
			MT_shel_envrn(a, b, 0L)
#define	shel_rdef(a, b) \
			MT_shel_rdef(a, b, 0L)
#define	shel_wdef(a, b) \
			MT_shel_wdef(a, b, 0L)

/****** dummies ***********************************************/

#define	xgrf_stepcalc(a, b, c, d, e, f, g, h, i, j, k)	/* Funktion ignorieren (GEM fÅr Dose): int16_t xgrf_stepcalc(int16_t orgw, int16_t orgh, int16_t xc, int16_t yc, int16_t w, int16_t h, int16_t *cx, int16_t *cy, int16_t *stepcnt, int16_t *xstep, int16_t *ystep); */
#define	xgrf_2box(a, b, c, d, e, f, g, h, i)			/* Funktion ignorieren (GEM fÅr Dose): int16_t xgrf_2box(int16_t xc, int16_t yc, int16_t w, int16_t h, int16_t corners, int16_t stepcnt, int16_t xstep, int16_t ystep, int16_t doubled); */

#if (WDLG_DIRECT_CALL == 0)

/****** Wdialog definitions ***********************************************/

typedef	void	*DIALOG;
						
typedef	int16_t	(cdecl hndl_obj_tf)(DIALOG *dialog, EVNT *events, int16_t obj, int16_t clicks, void *data);

extern DIALOG *MT_wdlg_create
(
	hndl_obj_tf *handle_exit,
	OBJECT *tree,
	void *user_data,
	int16_t code,
	void *data,
	int16_t flags,
	int16_t *global
);
extern int16_t MT_wdlg_open(DIALOG *dialog, int8_t *title, int16_t kind, int16_t x, int16_t y, int16_t code, void *data, int16_t *global);
#if MT_OLDWAY
extern int16_t MT_wdlg_close(DIALOG *dialog, int16_t *global);
#else
extern int16_t MT_wdlg_close(DIALOG *dialog, int16_t *x, int16_t *y, int16_t *global);
#endif
extern int16_t MT_wdlg_delete(DIALOG *dialog, int16_t *global);

extern int16_t MT_wdlg_get_tree(DIALOG *dialog, OBJECT **tree, GRECT *r, int16_t *global);
extern int16_t MT_wdlg_get_edit(DIALOG *dialog, int16_t *cursor, int16_t *global);
extern void	*MT_wdlg_get_udata(DIALOG *dialog, int16_t *global);
extern int16_t MT_wdlg_get_handle(DIALOG *dialog, int16_t *global);

extern int16_t MT_wdlg_set_edit(DIALOG *dialog, int16_t obj, int16_t *global);
extern int16_t MT_wdlg_set_tree(DIALOG *dialog, OBJECT *tree, int16_t *global);
extern int16_t MT_wdlg_set_size(DIALOG *dialog, GRECT *size, int16_t *global);
extern int16_t MT_wdlg_set_iconify(DIALOG *dialog, GRECT *g, char *title, OBJECT *tree, int16_t obj, int16_t *global);
extern int16_t MT_wdlg_set_uniconify(DIALOG *dialog, GRECT *g, char *title, OBJECT *tree, int16_t *global);

extern int16_t MT_wdlg_evnt(DIALOG *dialog, EVNT *events, int16_t *global);
extern void	MT_wdlg_redraw(DIALOG *dialog, GRECT *rect, int16_t obj, int16_t depth, int16_t *global);

#define	wdlg_create(a, b, c, d, e, f) \
			MT_wdlg_create(a, b, c, d, e, f, 0L)
#define	wdlg_open(a, b, c, d, e, f, g) \
			MT_wdlg_open(a, b, c, d, e, f, g, 0L)
#if MT_OLDWAY
#define	wdlg_close(a) \
			MT_wdlg_close(a, 0L)
#else
#define	wdlg_close(a, b, c) \
			MT_wdlg_close(a, b, c, 0L)
#endif
#define	wdlg_delete(a) \
			MT_wdlg_delete(a, 0L)
#define	wdlg_get_tree(a, b, c) \
			MT_wdlg_get_tree(a, b, c, 0L)
#define	wdlg_get_edit(a, b) \
			MT_wdlg_get_edit(a, b, 0L)
#define	wdlg_get_udata(a) \
			MT_wdlg_get_udata(a, 0L)
#define	wdlg_get_handle(a) \
			MT_wdlg_get_handle(a, 0L)
#define	wdlg_set_edit(a, b) \
			MT_wdlg_set_edit(a, b, 0L)
#define	wdlg_set_tree(a, b) \
			MT_wdlg_set_tree(a, b, 0L)
#define	wdlg_set_size(a, b) \
			MT_wdlg_set_size(a, b, 0L)
#define	wdlg_set_iconify(a, b, c, d, e) \
			MT_wdlg_set_iconify(a, b, c, d, e, 0L)
#define	wdlg_set_uniconify(a, b, c, d) \
			MT_wdlg_set_uniconify(a, b, c, d, 0L)
#define	wdlg_evnt(a, b) \
			MT_wdlg_evnt(a, b, 0L)
#define	wdlg_redraw(a, b, c, d) \
			MT_wdlg_redraw(a, b, c, d, 0L)


/* Definitionen fÅr <flags> */
#define	WDLG_BKGD	1				/* Hintergrundbedienung zulassen */

/* Funktionsnummern fÅr <obj> bei handle_exit(...) */
#define	HNDL_INIT	-1				/* Dialog initialisieren */
#define	HNDL_MESG	-2				/* Dialog initialisieren */
#define	HNDL_CLSD	-3				/* Dialogfenster wurde geschlossen */
#define	HNDL_OPEN	-5				/* Dialog-Initialisierung abschlieûen (zweiter Aufruf am Ende von wdlg_init) */
#define	HNDL_EDIT	-6				/* Zeichen fÅr ein Edit-Feld ÅberprÅfen */
#define	HNDL_EDDN	-7				/* Zeichen wurde ins Edit-Feld eingetragen */
#define	HNDL_EDCH	-8				/* Edit-Feld wurde gewechselt */
#define	HNDL_MOVE	-9				/* Dialog wurde verschoben */
#define	HNDL_TOPW	-10				/* Dialog-Fenster ist nach oben gekommen */
#define	HNDL_UNTP	-11				/* Dialog-Fenster ist nicht aktiv */

/****** Listbox definitions ***********************************************/
typedef	void	*LIST_BOX;

typedef	void	(cdecl *SLCT_ITEM)(LIST_BOX *box, OBJECT *tree, struct _lbox_item *item, void *user_data, int16_t obj_index, int16_t last_state);
typedef	int16_t	(cdecl *SET_ITEM)(LIST_BOX *box, OBJECT *tree, struct _lbox_item *item, int16_t obj_index, void *user_data, GRECT *rect, int16_t first);

typedef struct	_lbox_item
{
	struct _lbox_item *next;			/* Zeiger auf den nÑchsten Eintrag in der Liste */
	int16_t	selected;					/* gibt an, ob das Objekt selektiert ist */

	int16_t	data1;					/* Daten fÅr das Programm... */
	void	*data2;
	void	*data3;

} LBOX_ITEM;

#define	LBOX_VERT	1				/* Listbox mit vertikalem Slider */
#define	LBOX_AUTO	2				/* Auto-Scrolling */
#define	LBOX_AUTOSLCT	4			/* automatische Darstellung beim Auto-Scrolling */
#define	LBOX_REAL	8				/* Real-Time-Slider */
#define	LBOX_SNGL	16				/* nur ein anwÑhlbarer Eintrag */
#define	LBOX_SHFT	32				/* Mehrfachselektionen mit Shift */
#define	LBOX_TOGGLE	64			/* Status eines Eintrags bei Selektion wechseln */
#define	LBOX_2SLDRS	128			/* Listbox hat einen hor. und einen vertikalen Slider */

/* #defines fÅr Listboxen mit nur einem Slider */
#define	lbox_get_visible \
			lbox_get_avis

#define	lbox_get_first \
			lbox_get_afirst
			
#define	lbox_set_slider \
			lbox_set_asldr

#define	lbox_scroll_to \
			lbox_ascroll_to
			
extern LIST_BOX *MT_lbox_create(OBJECT *tree, SLCT_ITEM slct, SET_ITEM set, LBOX_ITEM *items, int16_t visible_a, int16_t first_a,
						  int16_t *ctrl_objs, int16_t *objs, int16_t flags, int16_t pause_a, void *user_data, void *dialog,
						  int16_t visible_b, int16_t first_b, int16_t entries_b, int16_t pause_b, int16_t *global);

extern void	MT_lbox_update(LIST_BOX *box, GRECT *rect, int16_t *global);
extern int16_t MT_lbox_do(LIST_BOX *box, int16_t obj, int16_t *global);
extern int16_t MT_lbox_delete(LIST_BOX *box, int16_t *global);

extern int16_t MT_lbox_cnt_items(LIST_BOX *box, int16_t *global);
extern OBJECT	*MT_lbox_get_tree(LIST_BOX *box, int16_t *global);
extern int16_t MT_lbox_get_avis(LIST_BOX *box, int16_t *global);
extern void	*MT_lbox_get_udata(LIST_BOX *box, int16_t *global);
extern int16_t MT_lbox_get_afirst(LIST_BOX *box, int16_t *global);
extern int16_t MT_lbox_get_slct_idx(LIST_BOX *box, int16_t *global);
extern LBOX_ITEM	*MT_lbox_get_items(LIST_BOX *box, int16_t *global);
extern LBOX_ITEM	*MT_lbox_get_item(LIST_BOX *box, int16_t n, int16_t *global);
extern LBOX_ITEM *MT_lbox_get_slct_item(LIST_BOX *box, int16_t *global);
extern int16_t MT_lbox_get_idx(LBOX_ITEM *items, LBOX_ITEM *search, int16_t *global);
extern int16_t MT_lbox_get_bvis(LIST_BOX *box, int16_t *global);
extern int16_t MT_lbox_get_bentries(LIST_BOX *box, int16_t *global);
extern int16_t MT_lbox_get_bfirst(LIST_BOX *box, int16_t *global);

extern void	MT_lbox_set_asldr(LIST_BOX *box, int16_t first, GRECT *rect, int16_t *global);
extern void	MT_lbox_set_items(LIST_BOX *box, LBOX_ITEM *items, int16_t *global);
extern void	MT_lbox_free_items(LIST_BOX *box, int16_t *global);
extern void	MT_lbox_free_list(LBOX_ITEM *items, int16_t *global);
extern void	MT_lbox_ascroll_to(LIST_BOX *box, int16_t first, GRECT *box_rect, GRECT *slider_rect, int16_t *global);
extern void	MT_lbox_set_bsldr(LIST_BOX *box, int16_t first, GRECT *rect, int16_t *global);
extern void	MT_lbox_set_bentries(LIST_BOX *box, int16_t entries, int16_t *global);
extern void	MT_lbox_bscroll_to(LIST_BOX *box, int16_t first, GRECT *box_rect, GRECT *slider_rect, int16_t *global);


#define	lbox_create(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
			MT_lbox_create(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, 0L)
#define	lbox_update(a, b) \
			MT_lbox_update(a, b, 0L)
#define	lbox_do(a, b) \
			MT_lbox_do(a, b, 0L)
#define	lbox_delete(a) \
			MT_lbox_delete(a, 0L)

#define	lbox_cnt_items(a) \
			MT_lbox_cnt_items(a, 0L)
#define	lbox_get_tree(a) \
			MT_lbox_get_tree(a, 0L)
#define	lbox_get_avis(a) \
			MT_lbox_get_avis(a, 0L)
#define	lbox_get_udata(a) \
			MT_lbox_get_udata(a, 0L)
#define	lbox_get_afirst(a) \
			MT_lbox_get_afirst(a, 0L)
#define	lbox_get_slct_idx(a) \
			MT_lbox_get_slct_idx(a, 0L)
#define	lbox_get_items(a) \
			MT_lbox_get_items(a, 0L)
#define	lbox_get_item(a, b) \
			MT_lbox_get_item(a, b, 0L)
#define	lbox_get_slct_item(a) \
			MT_lbox_get_slct_item(a, 0L)
#define	lbox_get_idx(a, b) \
			MT_lbox_get_idx(a, b, 0L)
#define	lbox_get_bvis(a) \
			MT_lbox_get_bvis(a, 0L)
#define	lbox_get_bentries(a) \
			MT_lbox_get_bentries(a, 0L)
#define	lbox_get_bfirst(a) \
			MT_lbox_get_bfirst(a, 0L)

#define	lbox_set_asldr(a, b, c) \
			MT_lbox_set_asldr(a, b, c, 0L)
#define	lbox_set_items(a, b) \
			MT_lbox_set_items(a, b, 0L)
#define	lbox_free_items(a) \
			MT_lbox_free_items(a, 0L)
#define	lbox_free_list(a) \
			MT_lbox_free_list(a, 0L)
#define	lbox_ascroll_to(a, b, c, d) \
			MT_lbox_ascroll_to(a, b, c, d, 0L)
#define	lbox_set_bsldr(a, b, c) \
			MT_lbox_set_bsldr(a, b, c, 0L)
#define	lbox_set_bentries(a, b) \
			MT_lbox_set_bentries(a, b, 0L)
#define	lbox_bscroll_to(a, b, c, d) \
			MT_lbox_bscroll_to(a, b, c, d, 0L)

/****** font selector definitions ***********************************************/

typedef	void	*FNT_DIALOG;

typedef	void	(cdecl *UTXT_FN)(int16_t x, int16_t y, int16_t *clip_rect, int32_t id, int32_t pt, int32_t ratio, int8_t *string);

typedef struct _fnts_item
{
	struct	_fnts_item	*next;	/* Zeiger auf den nÑchsten Font oder 0L (Ende der Liste) */
	UTXT_FN	display;				/* Zeiger auf die Anzeige-Funktion fÅr applikationseigene Fonts */
	int32_t		id;					/* ID des Fonts, >= 65536 fÅr applikationseigene Fonts */
	int16_t		index;				/* Index des Fonts (falls VDI-Font) */
	int8_t		mono;				/* Flag fÅr Ñquidistante Fonts */
	int8_t		outline;				/* Flag fÅr Vektorfont */
	int16_t		npts;				/* Anzahl der vordefinierten Punkthîhen */
	int8_t		*full_name;			/* Zeiger auf den vollstÑndigen Namen */
	int8_t		*family_name;			/* Zeiger auf den Familiennamen */
	int8_t		*style_name;			/* Zeiger auf den Stilnamen */
	int8_t		*pts;				/* Zeiger auf Feld mit Punkthîhen */
	int32_t		reserved[4];			/* reserviert, mÅssen 0 sein */
} FNTS_ITEM;

/* Definitionen fÅr <font_flags> bei fnts_create() */

#define FNTS_BTMP		1				/* Bitmapfonts anzeigen */
#define FNTS_OUTL		2				/* Vektorfonts anzeigen */
#define FNTS_MONO		4				/* Ñquidistante Fonts anzeigen */
#define FNTS_PROP		8				/* proportionale Fonts anzeigen */

/* Definitionen fÅr <dialog_flags> bei fnts_create() */
#define FNTS_3D		1				/* 3D-Design benutzen */

/* Definitionen fÅr <button_flags> bei fnts_open() */
#define FNTS_SNAME		0x01		/* Checkbox fÅr die Namen selektieren */
#define FNTS_SSTYLE		0x02		/* Checkbox fÅr die Stile selektieren */
#define FNTS_SSIZE		0x04		/* Checkbox fÅr die Hîhe selektieren */
#define FNTS_SRATIO		0x08		/* Checkbox fÅr das VerhÑltnis Breite/Hîhe selektieren */

#define FNTS_CHNAME		0x0100	/* Checkbox fÅr die Namen anzeigen */
#define FNTS_CHSTYLE	0x0200	/* Checkbox fÅr die Stile anzeigen */
#define FNTS_CHSIZE		0x0400	/* Checkbox fÅr die Hîhe anzeigen */
#define FNTS_CHRATIO	0x0800	/* Checkbox fÅr das VerhÑltnis Breite/Hîhe anzeigen */
#define FNTS_RATIO		0x1000	/* VerhÑltnis Breite/Hîhe einstellbar */
#define FNTS_BSET		0x2000	/* Button "setzen" anwÑhlbar */
#define FNTS_BMARK		0x4000	/* Button "markieren" anwÑhlbar */

/* Definitionen fÅr <button> bei fnts_evnt() */

#define FNTS_CANCEL		1		/* "Abbruch" wurde angewÑhlt */
#define FNTS_OK		2		/* "OK" wurde gedrÅckt */
#define FNTS_SET		3		/* "setzen" wurde angewÑhlt */
#define FNTS_MARK		4		/* "markieren" wurde betÑtigt */
#define FNTS_OPT		5		/* der applikationseigene Button wurde ausgewÑhlt */

extern FNT_DIALOG	*MT_fnts_create(int16_t vdi_handle, int16_t no_fonts, int16_t font_flags, int16_t dialog_flags, int8_t *sample, int8_t *opt_button, int16_t *global);
extern int16_t MT_fnts_delete(FNT_DIALOG *fnt_dialog, int16_t vdi_handle, int16_t *global);
extern int16_t MT_fnts_open(FNT_DIALOG *fnt_dialog, int16_t button_flags, int16_t x, int16_t y, int32_t id, int32_t pt, int32_t ratio, int16_t *global);
#if MT_OLDWAY
extern int16_t MT_fnts_close(FNT_DIALOG *fnt_dialog, int16_t *global);
#else
extern int16_t MT_fnts_close(FNT_DIALOG *fnt_dialog, int16_t *x, int16_t *y, int16_t *global);
#endif

extern int16_t MT_fnts_get_no_styles(FNT_DIALOG *fnt_dialog, int32_t id, int16_t *global);
extern int32_t	MT_fnts_get_style(FNT_DIALOG *fnt_dialog, int32_t id, int16_t index, int16_t *global);
extern int16_t MT_fnts_get_name(FNT_DIALOG *fnt_dialog, int32_t id, int8_t *full_name, int8_t *family_name, int8_t *style_name, int16_t *global);
extern int16_t MT_fnts_get_info(FNT_DIALOG *fnt_dialog, int32_t id, int16_t *mono, int16_t *outline, int16_t *global);

extern int16_t MT_fnts_add(FNT_DIALOG *fnt_dialog, FNTS_ITEM *user_fonts, int16_t *global);
extern void	MT_fnts_remove(FNT_DIALOG *fnt_dialog, int16_t *global);
extern int16_t MT_fnts_update(FNT_DIALOG *fnt_dialog, int16_t button_flags, int32_t id, int32_t pt, int32_t ratio, int16_t *global);

extern int16_t MT_fnts_evnt(FNT_DIALOG *fnt_dialog, EVNT *events, int16_t *button, int16_t *check_boxes, int32_t *id, int32_t *pt, int32_t *ratio, int16_t *global);
extern int16_t MT_fnts_do(FNT_DIALOG *fnt_dialog, int16_t button_flags, int32_t id_in, int32_t pt_in, int32_t ratio_in, int16_t *check_boxes, int32_t *id, int32_t *pt, int32_t *ratio, int16_t *global);


#define	fnts_create(a, b, c, d, e, f) \
			MT_fnts_create(a, b, c, d, e, f, 0L)
#define	fnts_delete(a, b) \
			MT_fnts_delete(a, b, 0L)
#define	fnts_open(a, b, c, d, e, f, g) \
			MT_fnts_open(a, b, c, d, e, f, g, 0L)
#if MT_OLDWAY
#define	fnts_close(a) \
			MT_fnts_close(a, 0L)
#else
#define	fnts_close(a, b, c) \
			MT_fnts_close(a, b, c, 0L)
#endif

#define	fnts_get_no_styles(a, b) \
			MT_fnts_get_no_styles(a, b, 0L)
#define	fnts_get_style(a, b, c) \
			MT_fnts_get_style(a, b, c, 0L)
#define	fnts_get_name(a, b, c, d, e) \
			MT_fnts_get_name(a, b, c, d, e, 0L)
#define	fnts_get_info(a, b, c, d) \
			MT_fnts_get_info(a, b, c, d, 0L)

#define	fnts_add(a, b) \
			MT_fnts_add(a, b, 0L)
#define	fnts_remove(a) \
			MT_fnts_remove(a, 0L)
#define	fnts_update(a, b, c, d, e) \
			MT_fnts_update(a, b, c, d, e, 0L)

#define	fnts_evnt(a, b, c, d, e, f, g) \
			MT_fnts_evnt(a, b, c, d, e, f, g, 0L)
#define	fnts_do(a, b, c, d, e, f, g, h, i) \
			MT_fnts_do(a, b, c, d, e, f, g, h, i, 0L)

#endif	/* WDLG_DIRECT_CALL */

/****** file selector definitions ***********************************************/
/* Sortiermodi */

#define SORTBYNAME	0
#define SORTBYDATE	1
#define SORTBYSIZE	2
#define SORTBYTYPE	3
#define SORTBYNONE	4
#define SORTDEFAULT -1				/* MagiC 6.10 */

/* Flags fÅr Dateiauswahl */

#define DOSMODE	1
#define NFOLLOWSLKS	2
#define GETMULTI	8

/* fslx_set_flags */

#define SHOW8P3	1

#ifndef __TOS
#define XATTR void
#endif

typedef	void	*FSLX_DIALOG;

typedef int16_t (cdecl XFSL_FILTER)(char *path, char *name, XATTR *xa);

extern FSLX_DIALOG MT_fslx_open(
			char *title,
			int16_t x, int16_t y,
			int16_t	*handle,
			char *path, int16_t pathlen,
			char *fname, int16_t fnamelen,
			char *patterns,
			XFSL_FILTER *filter,
			char *paths,
			int16_t sort_mode,
			int16_t flags,
			int16_t *global);

extern int16_t MT_fslx_evnt(
			FSLX_DIALOG fsd,
			EVNT *events,
			char *path,
			char *fname,
			int16_t *button,
			int16_t *nfiles,
			int16_t *sort_mode,
			char **pattern, int16_t *global);

extern void * MT_fslx_do(
			char *title,
			char *path, int16_t pathlen,
			char *fname, int16_t fnamelen,
			char *patterns,
			XFSL_FILTER *filter,
			char *paths,
			int16_t *sort_mode,
			int16_t flags,
			int16_t *button,
			int16_t *nfiles,
			char **pattern, int16_t *global);

extern int16_t MT_fslx_getnxtfile(FSLX_DIALOG fsd, char *fname, int16_t *global);
extern int16_t MT_fslx_close(FSLX_DIALOG fsd, int16_t *global);
extern int16_t MT_fslx_set_flags(int16_t flags, int16_t *oldval, int16_t *global);

#define	fslx_open(a,b,c,d,e,f,g,h,i,j,k,l,m) \
			MT_fslx_open(a,b,c,d,e,f,g,h,i,j,k,l,m,0L)
#define	fslx_evnt(a,b,c,d,e,f,g,h) \
			MT_fslx_evnt(a,b,c,d,e,f,g,h,0L)
#define	fslx_do(a,b,c,d,e,f,g,h,i,j,k,l,m) \
			MT_fslx_do(a,b,c,d,e,f,g,h,i,j,k,l,m,0L)
#define	fslx_close(a) \
			MT_fslx_close(a, 0L)
#define	fslx_getnxtfile(a, b) \
			MT_fslx_getnxtfile(a, b, 0L)
#define	fslx_set_flags(a, b) \
			MT_fslx_set_flags(a, b, 0L)

/************************************************************************/
#if (WDLG_DIRECT_CALL == 0)

#include	"prdialog.h"	/* Struktur zum EinfÅgen eigener Drucker und Unterdialoge */
#include	"prsettng.h"	/* PRN_SETTINGS-Struktur */

typedef	void *PRN_DIALOG;

PRN_DIALOG	*MT_pdlg_create(int16_t dialog_flags, int16_t *global);
int16_t MT_pdlg_delete(PRN_DIALOG *prn_dialog, int16_t *global);
int16_t MT_pdlg_open(PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings, int8_t *document_name, int16_t option_flags, int16_t x, int16_t y, int16_t *global);
int16_t MT_pdlg_close(PRN_DIALOG *prn_dialog, int16_t *x, int16_t *y, int16_t *global);

int32_t	MT_pdlg_get_setsize(int16_t *global);

int16_t MT_pdlg_add_printers(PRN_DIALOG *prn_dialog, DRV_INFO *drv_info, int16_t *global);
int16_t MT_pdlg_remove_printers(PRN_DIALOG *prn_dialog, int16_t *global);
int16_t MT_pdlg_update(PRN_DIALOG *prn_dialog, int8_t *document_name, int16_t *global);
int16_t MT_pdlg_add_sub_dialogs(PRN_DIALOG *prn_dialog, PDLG_SUB *sub_dialogs, int16_t *global);
int16_t MT_pdlg_remove_sub_dialogs(PRN_DIALOG *prn_dialog, int16_t *global);
PRN_SETTINGS *MT_pdlg_new_settings(PRN_DIALOG *prn_dialog, int16_t *global);
int16_t MT_pdlg_free_settings(PRN_SETTINGS *settings, int16_t *global);
int16_t MT_pdlg_dflt_settings(PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings, int16_t *global);
int16_t MT_pdlg_validate_settings(PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings, int16_t *global);
int16_t MT_pdlg_use_settings(PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings, int16_t *global);
int16_t MT_pdlg_save_default_settings(PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings, int16_t *global);

int16_t MT_pdlg_evnt(PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings, EVNT *events, int16_t *button, int16_t *global);
int16_t MT_pdlg_do(PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings, int8_t *document_name, int16_t option_flags, int16_t *global);


#define	pdlg_create(a) \
			MT_pdlg_create(a, 0L)
#define	pdlg_delete(a) \
			MT_pdlg_delete(a, 0L)
#define	pdlg_open(a, b, c, d, e, f) \
			MT_pdlg_open(a, b, c, d, e, f, 0L)
#define	pdlg_close(a, b, c) \
			MT_pdlg_close(a, b, c, 0L)

#define	pdlg_get_setsize() \
			MT_pdlg_get_setsize(0L)

#define	pdlg_add_printer(a, b) \
			MT_pdlg_add_printers(a, b, 0L)
#define	pdlg_remove_printers(a) \
			MT_pdlg_remove_printers(a, 0L)
#define	pdlg_update(a, b) \
			MT_pdlg_update(a, b, 0L)
#define	pdlg_add_sub_dialogs(a, b) \
			MT_pdlg_add_sub_dialogs(a, b, 0L)
#define	pdlg_remove_sub_dialogs(a) \
			MT_pdlg_remove_sub_dialogs(a, 0L)
#define	pdlg_new_settings(a) \
			MT_pdlg_new_settings(a, 0L)
#define	pdlg_free_settings(a) \
			MT_pdlg_free_settings(a, 0L)
#define	pdlg_dflt_settings(a, b) \
			MT_pdlg_dflt_settings(a, b, 0L)
#define	pdlg_validate_settings(a, b) \
			MT_pdlg_validate_settings(a, b, 0L)
#define	pdlg_use_settings(a, b) \
			MT_pdlg_use_settings(a, b, 0L)
#define	pdlg_save_default_settings(a, b) \
			MT_pdlg_save_default_settings(a, b, 0L)

#define	pdlg_evnt(a, b, c, d) \
			MT_pdlg_evnt(a, b, c, d, 0L)
#define	pdlg_do(a, b, c, d) \
			MT_pdlg_do(a, b, c, d, 0L)

/* <dialog_flags> fÅr pdlg_create() */
#define	PDLG_3D			0x0001

/* <option_flags> fÅr pdlg_open/do() */
#define	PDLG_PREFS		0x0000			/* Einstelldialog anzeigen */
#define	PDLG_PRINT		0x0001			/* Druckdialog anzeigen */

#define	PDLG_ALWAYS_COPIES	0x0010			/* immer Kopien anbieten */
#define	PDLG_ALWAYS_ORIENT	0x0020			/* immer Querformat anbieten */
#define	PDLG_ALWAYS_SCALE	0x0040			/* immer Skalierung anbieten */

#define	PDLG_EVENODD		0x0100			/* Option fÅr gerade und ungerade Seiten anbieten */

/* <button> fÅr pdlg_evnt()/pdlg_do */
#define	PDLG_CANCEL	1					/* "Abbruch" wurde angewÑhlt */
#define	PDLG_OK		2					/* "OK" wurde gedrÅckt */


#endif	/* WDLG_DIRECT_CALL */

/************************************************************************/

#include	<mt_edit.h>		/* Editor-Funktionen */

/************************************************************************/
#endif
