#ifndef AES_C_AES_H
#define AES_C_AES_H

#include <portab.h>
#include <tos.h>

typedef struct
{
    int g_x;
    int g_y;
    int g_w;
    int g_h;
} GRECT;

typedef struct
{
    char *te_ptext;
    char *te_ptmplt;
    char *te_pvalid;
    int   te_font;
    int   te_junk1;
    int   te_just;
    int   te_color;
    int   te_junk2;
    int   te_thickness;
    int   te_txtlen;
    int   te_tmplen;
} TEDINFO;

typedef struct
{
    int  *ib_pmask;
    int  *ib_pdata;
    char *ib_ptext;
    int   ib_char;
    int   ib_xchar;
    int   ib_ychar;
    int   ib_xicon;
    int   ib_yicon;
    int   ib_wicon;
    int   ib_hicon;
    int   ib_xtext;
    int   ib_ytext;
    int   ib_wtext;
    int   ib_htext;
} ICONBLK;

typedef struct cicon_data
{
    int   num_planes;
    int  *col_data;
    int  *col_mask;
    int  *sel_data;
    int  *sel_mask;
    struct cicon_data *next_res;
} CICON;

typedef struct cicon_blk
{
    ICONBLK monoblk;
    CICON  *mainlist;
} CICONBLK;

typedef struct
{
    int *bi_pdata;
    int  bi_wb;
    int  bi_hl;
    int  bi_x;
    int  bi_y;
    int  bi_color;
} BITBLK;

struct __parmblk;

typedef struct
{
    WORD cdecl (*ub_code)( struct __parmblk *parmblock );
    LONG ub_parm;
} USERBLK;

typedef struct
{
    unsigned long character   : 8;
    unsigned long framesize   : 8;
    unsigned long framecol    : 4;
    unsigned long textcol     : 4;
    unsigned long textmode    : 1;
    unsigned long fillpattern : 3;
    unsigned long interiorcol : 4;
} bfobspec;

typedef union obspecptr
{
    LONG              index;
    union obspecptr  *indirect;
    bfobspec          obspec;
    TEDINFO          *tedinfo;
    ICONBLK          *iconblk;
    CICONBLK         *ciconblk;
    BITBLK           *bitblk;
    USERBLK          *userblk;
    char             *free_string;
} OBSPEC;

typedef struct
{
    int    ob_next;
    int    ob_head;
    int    ob_tail;
    UWORD  ob_type;
    UWORD  ob_flags;
    UWORD  ob_state;
    OBSPEC ob_spec;
    int    ob_x;
    int    ob_y;
    int    ob_width;
    int    ob_height;
} OBJECT;

typedef struct __parmblk
{
    OBJECT *pb_tree;
    int     pb_obj;
    int     pb_prevstate;
    int     pb_currstate;
    int     pb_x, pb_y, pb_w, pb_h;
    int     pb_xc, pb_yc, pb_wc, pb_hc;
    LONG    pb_parm;
} PARMBLK;

typedef struct rshdr
{
    UWORD rsh_vrsn;
    UWORD rsh_object;
    UWORD rsh_tedinfo;
    UWORD rsh_iconblk;
    UWORD rsh_bitblk;
    UWORD rsh_frstr;
    UWORD rsh_string;
    UWORD rsh_imdata;
    UWORD rsh_frimg;
    UWORD rsh_trindex;
    UWORD rsh_nobs;
    UWORD rsh_ntree;
    UWORD rsh_nted;
    UWORD rsh_nib;
    UWORD rsh_nbb;
    UWORD rsh_nstring;
    UWORD rsh_nimages;
    UWORD rsh_rssize;
} RSHDR;

/* These records are shared with AES assembly and binary RSC files. */
_Static_assert(sizeof(GRECT) == 8, "GRECT ABI size");
_Static_assert(sizeof(TEDINFO) == 28, "TEDINFO ABI size");
_Static_assert(sizeof(ICONBLK) == 34, "ICONBLK ABI size");
_Static_assert(sizeof(CICON) == 22, "CICON ABI size");
_Static_assert(sizeof(CICONBLK) == 38, "CICONBLK ABI size");
_Static_assert(sizeof(BITBLK) == 14, "BITBLK ABI size");
_Static_assert(sizeof(OBJECT) == 24, "OBJECT ABI size");
_Static_assert(sizeof(PARMBLK) == 30, "PARMBLK ABI size");
_Static_assert(sizeof(RSHDR) == 36, "RSHDR ABI size");

/* object types */
#define G_BOX      20
#define G_TEXT     21
#define G_BOXTEXT  22
#define G_IMAGE    23
#define G_USERDEF  24
#define G_IBOX     25
#define G_BUTTON   26
#define G_BOXCHAR  27
#define G_STRING   28
#define G_FTEXT    29
#define G_FBOXTEXT 30
#define G_ICON     31
#define G_TITLE    32
#define G_CICON    33

/* object flags */
#define NONE       0x0000
#define SELECTABLE 0x0001
#define DEFAULT    0x0002
#define EXIT       0x0004
#define EDITABLE   0x0008
#define RBUTTON    0x0010
#define LASTOB     0x0020
#define TOUCHEXIT  0x0040
#define HIDETREE   0x0080
#define INDIRECT   0x0100

/* object states */
#define NORMAL     0x00
#define SELECTED   0x01
#define CROSSED    0x02
#define CHECKED    0x04
#define DISABLED   0x08
#define OUTLINED   0x10
#define SHADOWED   0x20

#define ROOT       0
#define MAX_DEPTH  8

/* editable text */
#define ED_START 0
#define ED_INIT  1
#define ED_CHAR  2
#define ED_END   3
#define ED_CRSR  100

#define TE_LEFT  0
#define TE_RIGHT 1
#define TE_CNTR  2

/* form */
#define FMD_START  0
#define FMD_GROW   1
#define FMD_SHRINK 2
#define FMD_FINISH 3

/* mouse forms */
#define ARROW      0
#define TEXT_CRSR  1
#define HOURGLASS  2
#define BUSYBEE    2
#define POINT_HAND 3
#define FLAT_HAND  4
#define THIN_CROSS 5
#define THICK_CROSS 6
#define OUTLN_CROSS 7
#define USER_DEF   255
#define M_OFF      256
#define M_ON       257

/* window components */
#define NAME     0x0001
#define CLOSER   0x0002
#define FULLER   0x0004
#define MOVER    0x0008
#define INFO     0x0010
#define SIZER    0x0020
#define UPARROW  0x0040
#define DNARROW  0x0080
#define VSLIDE   0x0100
#define LFARROW  0x0200
#define RTARROW  0x0400
#define HSLIDE   0x0800

#define WF_KIND      1
#define WF_NAME      2
#define WF_INFO      3
#define WF_WORKXYWH  4
#define WF_CURRXYWH  5
#define WF_PREVXYWH  6
#define WF_FULLXYWH  7
#define WF_HSLIDE    8
#define WF_VSLIDE    9
#define WF_TOP       10
#define WF_FIRSTXYWH 11
#define WF_NEXTXYWH  12
#define WF_RESVD     13
#define WF_NEWDESK   14
#define WF_HSLSIZE   15
#define WF_VSLSIZE   16
#define WF_SCREEN    17
#define WF_COLOR     18
#define WF_DCOLOR    19
#define WF_OWNER     20
#define WF_BEVENT    24
#define WF_BOTTOM    25

#define WC_BORDER 0
#define WC_WORK   1

#define END_UPDATE 0
#define BEG_UPDATE 1
#define END_MCTRL  2
#define BEG_MCTRL  3

/* events */
#define MU_KEYBD  0x0001
#define MU_BUTTON 0x0002
#define MU_M1     0x0004
#define MU_M2     0x0008
#define MU_MESAG  0x0010
#define MU_TIMER  0x0020

#define WM_REDRAW    20
#define WM_TOPPED    21
#define WM_CLOSED    22
#define WM_FULLED    23
#define WM_ARROWED   24
#define WM_HSLID     25
#define WM_VSLID     26
#define WM_SIZED     27
#define WM_MOVED     28
#define WM_NEWTOP    29
#define WM_UNTOPPED  30
#define WM_ONTOP     31

#define K_RSHIFT 0x0001
#define K_LSHIFT 0x0002
#define K_CTRL   0x0004
#define K_ALT    0x0008

#define GAI_MAGIC 0x0001

/* kernel AES entry points used by this slice */
void objc_offset( OBJECT *tree, WORD objnr, WORD *x, WORD *y );
WORD form_alert( WORD defbtn, const char *string );
WORD graf_mkstate( WORD *mx, WORD *my, WORD *mbutton, WORD *kstate );
WORD vq_gdos( void );
WORD graf_mouse( WORD form, void *data );
WORD graf_slidebox( OBJECT *tree, WORD parent, WORD objnr, WORD vert );
WORD wind_update( WORD mode );
WORD wind_close( WORD whdl );
WORD wind_delete( WORD whdl );
WORD wind_find( WORD x, WORD y );

#endif
