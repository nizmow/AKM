/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _cpx_h
#define _cpx_h

#include <setjmp.h>
#include <mt_aes.h>
#include "global.h"


#define	PH_MAGIC 0x601a		/* Magic des Programmheaders */

struct program_header
{
	int16_t ph_branch;
	int32_t  ph_tlen;
	int32_t  ph_dlen;
	int32_t  ph_blen;
	int32_t  ph_slen;
	int32_t  ph_res1;
	int32_t  ph_prgflags;
	int16_t ph_absflag;
};

struct foobar
{
	int16_t	dummy;
	int16_t	*image;
};

struct cl_segm
{
	void	*text_seg;
	int32_t	len_text;
	void	*data_seg;
	int32_t	len_data;
	void	*bss_seg;
	int32_t	len_bss;
};

struct cpxhead
{
	unsigned int16_t magic;

	unsigned int16_t flags;
#define CPX_SETONLY		0x0001		/* Set Only CPX Flag 		*/
#define CPX_BOOTINIT	0x0002		/* Boot Initialization Flag	*/
#define CPX_RESIDENT	0x0004		/* RAM Resident Flag		*/
#define CPX_WAS_RESIDENT 0x8000		/* used internally by config.cpx */

	int32_t cpx_id;
	unsigned int16_t cpx_version;
	char i_text[14];
	unsigned int16_t icon[48];

	struct
	{
		unsigned i_color:	4;
		unsigned reserved:	4;
		unsigned i_char:	8;
	} i_info;

	char title_txt[18];

	struct
	{
		unsigned c_board:	4;
		unsigned c_text:	4;
		unsigned pattern:	4;
		unsigned c_back:	4;
	} t_info;

	char buffer[64];
	char reserved[306];
};

/* Mausparameter */
typedef struct
{
	int16_t x;
	int16_t y;
	int16_t buttons;
	int16_t kstate;
} MRETS;

/* Struktur zur Verwaltung von CPX-Modulen */
struct cpxlist
{
	char f_name[14];
	int16_t head_ok;
	int16_t segm_ok;
	struct cl_segm *segm;
	struct cpxlist *next;
	struct cpxhead header;
};

/*
 * exported functions to cpx (xcontrol parameter block)
 *
 * 32bit clean
 */

/* helper structs for 16bit argument alignment */
struct rsh_fix_args
{
	int16_t num_objs; int16_t num_frstr; int16_t num_frimg; int16_t num_tree;
	OBJECT *rs_object; TEDINFO *rs_tedinfo; char **rs_strings; ICONBLK *rs_iconblk;
	BITBLK *rs_bitblk; int32_t *rs_frstr; int32_t *rs_frimg; int32_t *rs_trindex; struct foobar *rs_imdope;
};
struct rsh_obfix_args
{
	OBJECT *tree; int16_t ob;
};
struct Popup_args
{
	char **items; int16_t no_items; int16_t slct; int16_t font; GRECT *up; GRECT *world;
};
struct Sl_size_args
{
	OBJECT *tree; int16_t base; int16_t slider;
	int16_t num_items; int16_t visible; int16_t direction; int16_t min_size;
};
struct Sl_xy_args
{
	OBJECT *tree; int16_t base; int16_t slider; int16_t value;
	int16_t min; int16_t max; void cdecl (*userdef)(void);
};
struct Sl_arrow_args
{
	OBJECT *tree; int16_t base; int16_t slider; int16_t obj; int16_t inc;
	int16_t min; int16_t max; int16_t *value; int16_t direction; void cdecl (*userdef)(void);
};
struct Sl_dragxy_args
{
	OBJECT *tree; int16_t base; int16_t slider;
	int16_t min; int16_t max; int16_t *value; void cdecl (*userdef)(void);
};
struct Xform_do_args
{
	OBJECT *tree; int16_t edit_obj; int16_t *msg;
};
struct Set_Evnt_Mask_args
{
	int16_t mask; MOBLK *m1; MOBLK *m2; int32_t evtime;
};
struct XGen_Alert_args
{
	int16_t alert_id;
};
struct MFsave_args
{
	int16_t flag; MFORM *mf;
};

struct xcpb
{
	int16_t handle;
	int16_t booting;
	int16_t reserved;
	int16_t SkipRshFix;

	struct cpxlist *cdecl (*get_cpx_list)(void);

	int16_t   cdecl (*save_header)(struct cpxlist *header);
	void    cdecl (*rsh_fix)(struct rsh_fix_args);
	void    cdecl (*rsh_obfix)(struct rsh_obfix_args);
	int16_t   cdecl (*Popup)(struct Popup_args);
	void    cdecl (*Sl_size)(struct Sl_size_args);
	void    cdecl (*Sl_x)(struct Sl_xy_args);
	void    cdecl (*Sl_y)(struct Sl_xy_args);
	void    cdecl (*Sl_arrow)(struct Sl_arrow_args);
	void    cdecl (*Sl_dragx)(struct Sl_dragxy_args);
	void    cdecl (*Sl_dragy)(struct Sl_dragxy_args);
	int16_t   cdecl (*Xform_do)(struct Xform_do_args);
	GRECT * cdecl (*GetFirstRect)(GRECT *prect);
	GRECT * cdecl (*GetNextRect)(void);
	void    cdecl (*Set_Evnt_Mask)(struct Set_Evnt_Mask_args);
	int16_t   cdecl (*XGen_Alert)(struct XGen_Alert_args);
	int16_t   cdecl (*CPX_Save)(void *ptr, int32_t bytes);
	void *  cdecl (*Get_Buffer)(void);
	int16_t   cdecl (*getcookie)(int32_t cookie, int32_t *p_value);

	int16_t Country_Code;

	void cdecl (*MFsave)(struct MFsave_args);
};

/*
 * CPX interface entry points
 *
 * 32bit clean
 */

typedef struct
{
	int16_t cdecl (*cpx_call)(GRECT *rect, DIALOG * /* COPS extension */);
	void  cdecl (*cpx_draw)(GRECT *clip);
	void  cdecl (*cpx_wmove)(GRECT *work);
	void  cdecl (*cpx_timer)(int16_t *quit);
	void  cdecl (*cpx_key)(int16_t kstate, int16_t key, int16_t *quit);
	void  cdecl (*cpx_button)(MRETS *mrets, int16_t nclicks, int16_t *quit);
	void  cdecl (*cpx_m1)(MRETS *mrets, int16_t *quit);
	void  cdecl (*cpx_m2)(MRETS *mrets, int16_t *quit);
	int16_t cdecl (*cpx_hook)(int16_t event, int16_t *msg, MRETS *mrets, int16_t *key, int16_t *nclicks);
	void  cdecl (*cpx_close)(int16_t flag);
} CPXINFO;


/* Documented alert message numbers for XGen_Alert  */
#define XAL_SAVE_DEFAULTS	0
#define XAL_MEM_ERR		1
#define XAL_FILE_ERR		2
#define XAL_FILE_NOT_FOUND	3
/* Undocumented alert message numbers for XGen_Alert  */
#define XAL_NO_NODES_ERR	4
#define XAL_RELOAD_CPXS		5
#define XAL_UNLOAD_CPX		6
#define XAL_NO_RELOAD		7
#define XAL_SAVE_HEADER		8
#define XAL_FILE_NOT_CPX	9
#define XAL_NO_SOUND_DMA	10	/* used by sound.cpx/fsound.cpx */
#define XAL_SHUTDOWN		11

#define MFSAVE      1
#define MFRESTORE   0

#ifndef	CT_KEY
#define CT_KEY  53
#endif

#define	CPXD_INVALID	0x8000	/* signalisiert ungueltigen CPX_DESC-Eintrag */
#define	CPXD_AUTOSTART	0x0002
#define	CPXD_INACTIVE	0x0001

typedef	struct cpx_desc
{
	struct cpx_desc *next;	/* Zeiger auf die naechste CPX-Beschreibung oder 0L */

	void	*start_of_cpx;	/* Startadresse des CPX im Speicher */
	void	*end_of_cpx;	/* Endadresse des CPX im Speicher */

	jmp_buf	jb;		/* longjmp buffer for interrupted Xform_do */
	char	*stack;		/* temporary malloced() stack */

	CPXINFO	*info;		/* Zeiger auf CPXINFO-Struktur die bei cpx_call() zurueckgeliefert wird */

	DIALOG	*dialog;	/* Zeiger auf die Dialogbeschreibung oder 0L, wenn das CPX nicht offen ist */
	OBJECT	*tree;		/* ist bei Form-CPX 0L, bis cpx_form_do() aufgerufen wird */
	int16_t	whdl;		/* Handle des Dialogfensters */
	int16_t	window_x;	/* Position des Dialogfensters */
	int16_t	window_y;

	int16_t	is_evnt_cpx;	/* 0: Form-CPX 1: Event-CPX */
	int16_t	button;		/* wird von handle_form_cpx() zurueckgeliefert */
	int16_t	*msg;		/* Zeiger auf den bei Xform_do() uebergebenen Messagebuffer */

	int16_t	edit_obj;
	int16_t	cursor_idx;
	BOOLEAN	cursor;

	int16_t	obfix_cnt;
	int16_t	box_width;
	int16_t	box_height;
	GRECT	size;		/* Dialogausmasse und -position */
	OBJECT	empty_tree[2];	/* IBOX fuer leeren Objektbaum am Anfang,
				   wegen WDIALOG-Fehler aus 2 Objekten bestehend */

	struct cl_segm segm;

	int16_t	icon_x;		/* Iconposition innerhalb des Hauptfensters */
	int16_t	icon_y;

	int	selected;
	int16_t	flags;

	/* cpx read/write accessible data */

	GRECT	redraw_area;	/* Bereich den ein CPX nach WM_REDRAW neuzeichnen soll */
	GRECT	dirty_area;	/* Bereich eines neuzuzeichnenden Rechtecks */
	int16_t	mask;		/* zusaetzliche Ereignismaske fuer evnt_multi() */
	MOBLK	m1;		/* Mausrechteck fuer evnt_multi() */
	MOBLK	m2;		/* Mausrechteck fuer evnt_multi() */
	int32_t	time;		/* Timerintervall fuer evnt_multi() */

	struct cpxlist old;	/* Beschreibung fuer get_cpx_list */
	struct xcpb xctrl_pb;	/* Parameterblock fuer das CPX */

	/* cpx name */
	char	file_name[0];

} CPX_DESC;

#endif /* _cpx_h */
