/* Tabulator width 4 */
/*******************************************************************
*
*             CHGRES_M.PRG
*             ============
*
* Authors:
*		Sven & Wilfried Behne (original, 1990s)
*		Thorsten Otto (decompilation)
*		Andreas Kromke (adaption for MagicOnLinux)
*
*
* written with Pure C V1.1
* project file: CHGRES_M.PRJ
*
* parameters: none
*
****************************************************************/

#include <tos.h>
#include <stdio.h>  /* for sprintf() */
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <mt_aes.h>

#include "de/chgres.h"

#if 1
#define DBGOUT printf
#else
define DBGOUT()
#endif

typedef char int8_t;
typedef unsigned char uint8_t;
typedef int int16_t;
typedef unsigned int uint16_t;
typedef long int32_t;
typedef unsigned long uint32_t;



#define SV_INQUIRE 0	/* inquire sysvar data, see mt_objc_sysvar() */
#define AD3DVAL 6		/* AES 4.0     */


int gl_apid;
static int aes_handle;
int gl_wchar;
int gl_hchar;
int gl_wbox;
int gl_hbox;
static char **rs_frstr;
static OBJECT **rs_trindex;

static int must_shutdown;

#define NUM_COLOUR_MODES 7
#define NUM_SCREEN_SIZES 14

static const uint32_t colour_modes[NUM_COLOUR_MODES][2] =
{
	{ 2, 0x0000 },
	{ 4, 0x0000 },
	{ 16, 0x0000 },
	{ 16, 0x0001 },
	{ 256, 0x0001 },
	{ 32 * 1024, 0x0003 },
	{ 16L * 1024L * 1024L, 0x0003 }
};

static const char *colour_mode_names[NUM_COLOUR_MODES] =
{
	"2 (mono)",
	"4",
	"16 (interleaved)",
	"16",
	"256",
	"32 k",
	"16 M"
};

static const uint16_t screen_sizes[NUM_SCREEN_SIZES][2] =
{
	{  320,  200 },
	{  512,  384 },
	{  640,  400 },
	{  640,  480 },
	{  800,  608 },
	{  720,  576 },
	{ 1024,  768 },
	{ 1280,  720 },
	{ 1280,  960 },		/* TODO: remove 1 */
	{ 1280, 1024 },
	{ 1600, 1200 },
	{ 1920, 1080 },
	{ 1920, 1200 },
	{ 2560, 1440 }
};

static char screen_sizes_names[NUM_SCREEN_SIZES][16];

static LBOX_ITEM screen_size_lbox_item_tab[NUM_SCREEN_SIZES];

struct MagiCVideoMode
{
	uint16_t length;		/* length of this struct, for forward compatibility */
	uint32_t magic;			/* 0x14534711, if valid */
	uint32_t ncolours;		/* 2,4,16,256,32768,65536,2^24 */
	uint16_t bits;			/* 1,2,4,8,16,32 */
	uint16_t flags;			/* Bit 0: packed, Bit 1: direct colour */
	uint16_t width;			/* screen width */
	uint16_t height;		/* screen height */
};

static struct MagiCVideoMode currMode;
static LIST_BOX *color_lbox;
static DIALOG *dialog;

/* dialogue status */
static int original_colour_mode = -1;
static int original_screen_size = -1;

static int dlg_curr_colour_mode = 0;    /* TODO: set to -1 */
static int dlg_curr_screen_size = -1;	/* TODO: set to -1 */


/*************************************************************
*
* Ask MagiC emulator for current video mode
*
* Uses Vgetrgb() with an invalid colour index higher than 255
*
*************************************************************/
static void molGetVideoMode(void)
{
	unsigned i;

	currMode.length = (uint16_t) sizeof(currMode);
	(void) xbios(/*VgetRGB*/ 94, (uint16_t) 0xf000, (uint16_t) 0, &currMode);

	/*
	DBGOUT("vmode.length    = %u\n", currMode.length);
	DBGOUT("vmode.magic     = 0x%08lx\n", currMode.magic);
	DBGOUT("vmode.ncolours  = %lu\n", currMode.ncolours);
	DBGOUT("vmode.bits      = %u\n", currMode.bits);
	DBGOUT("vmode.flags     = 0x%04x\n", currMode.flags);
	DBGOUT("vmode.width     = %u\n", currMode.width);
	DBGOUT("vmode.height    = %u\n", currMode.height);
	*/

	/* search for colour mode in presets */
	for (i = 0; i < NUM_COLOUR_MODES; i++)
	{
		if ((colour_modes[i][0] == currMode.ncolours) &&
		    (colour_modes[i][1] == (uint32_t) currMode.flags))
		{
			original_colour_mode = i;
			dlg_curr_colour_mode = i;
			break;
		}
	}

	/* search for screen size in presets */
	for (i = 0; i < NUM_SCREEN_SIZES; i++)
	{
		if ((screen_sizes[i][0] == currMode.width) &&
		    (screen_sizes[i][1] == currMode.height))
		{
			original_screen_size = i;
			dlg_curr_screen_size = i;
			break;
		}
	}
}


/*************************************************************
*
* Initialise LISTBOX
*
*************************************************************/
static void initListBox(void)
{
	unsigned i;

	for (i = 0; i < NUM_SCREEN_SIZES; i++)
	{
		uint16_t w = screen_sizes[i][0];
		uint16_t h = screen_sizes[i][1];
		sprintf(screen_sizes_names[i], "%4u * %4u", w, h);

		if (i + 1 < NUM_SCREEN_SIZES)
		{
			screen_size_lbox_item_tab[i].next = &screen_size_lbox_item_tab[i + 1];
		}
		
		if (i == dlg_curr_screen_size)
		{
			screen_size_lbox_item_tab[i].selected = 1;
		}

		screen_size_lbox_item_tab[i].data1 = i;
		screen_size_lbox_item_tab[i].data2 = screen_sizes_names[i];
	}
}


#define N_ITEMS (CHGRES_BOX_LAST - CHGRES_BOX_FIRST + 1)
static int const ctrl_objs[5] =
{
	CHGRES_BOX, CHGRES_UP, CHGRES_DOWN, CHGRES_BACK, CHGRES_SLIDER
};

static int const objs[N_ITEMS] =
{
	CHGRES_BOX_FIRST,
	CHGRES_BOX_FIRST+1,
	CHGRES_BOX_FIRST+2,
	CHGRES_BOX_FIRST+3,
	CHGRES_BOX_FIRST+4,
	CHGRES_BOX_FIRST+5,
	CHGRES_BOX_FIRST+6,
	CHGRES_BOX_FIRST+7,
	CHGRES_BOX_FIRST+8,
	CHGRES_BOX_LAST
};

#ifndef OS_SELECTED
#define OS_SELECTED SELECTED
#define OS_DISABLED DISABLED
#define OS_SHADOWED SHADOWED
#define OS_NORMAL NORMAL
#define OS_CHECKED CHECKED
#define OF_SELECTABLE SELECTABLE
#define OF_DEFAULT DEFAULT
#define OF_HIDETREE HIDETREE
#define OF_FL3DBAK FL3DBAK
#define OF_LASTOB LASTOB
#define OF_NONE   NONE
#endif


/*************************************************************
*
* Popup
*
*************************************************************/
static int simple_popup
(
	OBJECT *tree, int obj_index,
	const char * const *names, int num_names,
	int selected
)
{
	OBJECT *obj;
	int i;
	int maxlen;
	OBJECT *newtree;

	obj = &tree[obj_index];
	for (i = maxlen = 0; i < num_names; i++)
	{
		int len = (int)strlen(names[i]);
		if (len > maxlen)
			maxlen = (int)len;
	}

	newtree = (OBJECT *) Malloc((num_names * 2 + 1) * sizeof(OBJECT));
	if (newtree != NULL)
	{
		OBJECT *o;
		int y;
		int sel_y;

		maxlen += 4;
		maxlen *= gl_wchar;
		if (maxlen < obj->ob_width)
			maxlen = obj->ob_width;
		o = newtree;
		o->ob_next = -1;  /* NIL */
		o->ob_type = G_BOX;
		o->ob_state = OS_SHADOWED;
		o->ob_spec.index = 0xff1000L;
		o->ob_flags = OF_FL3DBAK;
		if (num_names > 0)
		{
			o->ob_head = 1;
			o->ob_tail = (num_names * 2) - 1;
		} else
		{
			o->ob_flags |= OF_LASTOB;
			o->ob_head = o->ob_tail = -1;  /* NIL */
		}
		o->ob_width = maxlen;
		o->ob_height = num_names * gl_hchar;
		y = 0;
		sel_y = -obj->ob_height;
		for (i = 1; i <= num_names; i++)
		{
			o++;
			if (i < num_names)
				o->ob_next = (i * 2) + 1;
			else
				o->ob_next = ROOT;
			o->ob_head = o->ob_tail = i * 2;
			o->ob_flags = OF_FL3DBAK|OF_SELECTABLE;
			o->ob_type = G_IBOX;
			o->ob_spec.index = 0;
			o->ob_state = OS_NORMAL;
			o->ob_width = maxlen;
			o->ob_height = gl_hchar;
			o->ob_x = 0;
			o->ob_y = y;
			if ((selected + 1) == i)
			{
				sel_y = y;
				o->ob_state |= OS_CHECKED;
			}
			o++;
			o->ob_next = i * 2 - 1;
			o->ob_head = o->ob_tail = -1; /* NIL */
			o->ob_flags = OF_NONE;
			if (i == num_names)
				o->ob_flags |= OF_LASTOB;
			o->ob_type = G_STRING;
			o->ob_spec.free_string = (char *)names[i - 1];
			o->ob_state = OS_NORMAL;
			o->ob_width = 0;
			o->ob_height = gl_hchar;
			o->ob_x = gl_wchar * 2;
			o->ob_y = 0;
			y += gl_hchar;
		}

		objc_offset(tree, obj_index, &newtree[ROOT].ob_x, &newtree[ROOT].ob_y);
		newtree[ROOT].ob_y -= sel_y;
		selected = form_popup(newtree, 0, 0);
		if (selected > 0)
		{
			selected -= 1;
			selected >>= 1;
		} else
		{
			selected = -1;
		}

		Mfree(newtree);
	}
	return selected;
}


/*************************************************************
*
* Show dialogue for INFO, new resolution etc.
*
*************************************************************/
static int do_dialog(OBJECT *tree)
{
	GRECT gr;
	void *flydial;
	int lastcrsr;
	int ret;

	wind_update(BEG_UPDATE);
	wind_update(BEG_MCTRL);

	form_center(tree, &gr);
	form_xdial(FMD_START, &gr, &gr, &flydial);
	objc_draw(tree, ROOT, MAX_DEPTH, &gr);
	ret = form_xdo(tree, ROOT, &lastcrsr, NULL, flydial);
	ret &= 0x7fff;
	form_xdial(FMD_FINISH, &gr, &gr, &flydial);
	wind_update(END_MCTRL);
	wind_update(END_UPDATE);
	tree[ret].ob_state &= ~OS_SELECTED;
	return ret;
}


/*************************************************************
*
* LISTBOX callback
*
*************************************************************/
#pragma warn -par
static int cdecl lbox_set_item_cb
(
	LIST_BOX *box, OBJECT *tree,
	struct _lbox_item *item,
	WORD obj_index,
	void *user_data,
	GRECT *rect,
	WORD first
)
{
	/* (void) first; */
	char *text;
	const char *src;

	text = tree[obj_index].ob_spec.tedinfo->te_ptext;
	if (item != NULL)
	{
		if (item->selected)
			tree[obj_index].ob_state |= OS_SELECTED;
		else
			tree[obj_index].ob_state &= ~OS_SELECTED;
		src = (const char *) item->data2;
		if (*text)
			*text++ = ' ';
		while (*text && *src)
			*text++ = *src++;
	} else
	{
		tree[obj_index].ob_state &= ~OS_SELECTED;
	}

	while (*text)
		*text++ = ' ';
	return obj_index;
}
#pragma warn +par

static void redraw_obj(DIALOG *dlg, int obj)
{
	OBJECT *tree;
	GRECT gr;

	wdlg_get_tree(dlg, &tree, &gr);
	wind_update(BEG_UPDATE);
	wdlg_redraw(dlg, &gr, obj, MAX_DEPTH);
	wind_update(END_UPDATE);
}


/*************************************************************
*
* LISTBOX callback
*
*************************************************************/
#pragma warn -par
static void cdecl lbox_select_item_cb
(
	LIST_BOX *box, OBJECT *tree,
	struct _lbox_item *item,
	void *user_data,
	WORD obj_index,
	WORD last_state
)
{
	/* (void) last_state; */
	DIALOG *dialog = (DIALOG *) user_data;

	if (item != NULL)
	{
		if (dlg_curr_colour_mode == -1)
		{
			tree[CHGRES_OK].ob_state &= ~OS_DISABLED;
			tree[CHGRES_OK].ob_flags |= OF_SELECTABLE;
			tree[CHGRES_OK].ob_flags |= OF_DEFAULT;
			redraw_obj(dialog, CHGRES_OK);
		}
		#if 0
		if (res->base.flags & FLAG_INFO)
		{
			tree[CHGRES_INFO].ob_state &= ~OS_DISABLED;
			tree[CHGRES_INFO].ob_flags |= OF_SELECTABLE;
		} else
		{
			tree[CHGRES_INFO].ob_state |= OS_DISABLED;
			tree[CHGRES_INFO].ob_flags &= ~OF_SELECTABLE;
		}
		#endif
		redraw_obj(dialog, CHGRES_INFO);
		dlg_curr_colour_mode = item->data1;
	}
}
#pragma warn +par


/*************************************************************
*
* Update popup in main dialogue to show selected colour mode
*
*************************************************************/
static void update_colour_mode(void)
{
	rs_trindex[MAIN_DIALOG][CHGRES_COLORS].ob_spec.free_string =
			 (char *) colour_mode_names[dlg_curr_colour_mode];
}



#if 0
static int count_res(struct res *res)
{
	int count;

	count = 0;
	while (res != NULL)
	{
		res = res->base.next;
		count++;
	}
	return count;
}


static int cmp_res(const void *res1, const void *res2)
{
	const struct res *r1 = *(const void *const *)res1;
	const struct res *r2 = *(const void *const *)res2;
	if ((r1->base.virt_hres > r2->base.virt_hres) ||
     ((r1->base.virt_hres >= r2->base.virt_hres &&
      ((r1->base.virt_vres > r2->base.virt_vres ||
       ((r1->base.virt_vres >= r2->base.virt_vres &&
        ((r1->base.hres > r2->base.hres ||
         ((r1->base.hres >= r2->base.hres &&
          ((r1->base.vres > r2->base.vres ||
           ((r1->base.vres >= r2->base.vres &&
            (((r1->base.flags & FLAG_INFO) != 0)) && (((r2->base.flags & FLAG_INFO) == 0 ||
             r1->freq > r2->freq)))))))))))))))))
	    return 1;
	return -1;
}


static struct res *sort_restab(struct res *res)
{
	struct res **table;
	int count;
	struct res **respp;
	struct res *ret;

	count = count_res(res);
	ret = res;
	table = (struct res **)Malloc(count * sizeof(*table));
	if (table != NULL)
	{
		respp = table;
		while (res != NULL)
		{
			*respp++ = res;
			res = res->base.next;
		}
		qsort(table, count, sizeof(*table), cmp_res);
		ret = table[0];
		respp = table;
		while (count > 1)
		{
			respp[0]->base.next = respp[1];
			respp++;
			count--;
		}
		respp[0]->base.next = NULL;
		Mfree(table);
	}
	return ret;
}


static void set_items(void)
{
	lbox_set_items(color_lbox, (LBOX_ITEM *)res_tab);
	lbox_update(color_lbox, NULL);
}


static void show_info(struct res *res)
{
	OBJECT *tree;
	long linesize;
	short orig_hres;
	short orig_vres;

	tree = rs_trindex[INFO_DIALOG];
	sprintf(tree[INFO_HRES].ob_spec.free_string, "%4d", res->base.hres);
	sprintf(tree[INFO_VRES].ob_spec.free_string, "%4d", res->base.vres);
	sprintf(tree[INFO_FREQ].ob_spec.free_string, "%4d", res->base.flags & FLAG_INFO ? res->freq : 0);
	sprintf(tree[INFO_VIRT_HRES].ob_spec.tedinfo->te_ptext, "%4d", res->base.virt_hres);
	sprintf(tree[INFO_VIRT_VRES].ob_spec.tedinfo->te_ptext, "%4d", res->base.virt_vres);
	if (do_dialog(tree) == INFO_OK && !magicpc)
	{
		orig_hres = res->base.virt_hres;
		orig_vres = res->base.virt_vres;

		res->base.virt_hres = atoi(tree[INFO_VIRT_HRES].ob_spec.tedinfo->te_ptext);
		res->base.virt_vres = atoi(tree[INFO_VIRT_VRES].ob_spec.tedinfo->te_ptext);
		if ((res->base.virt_hres & 15) != 0 || (res->base.virt_vres & 15) != 0)
		{
			res->base.virt_hres &= -16;
			res->base.virt_vres &= -16;
			do_dialog(rs_trindex[ERR_VIRTUAL_RES]);
		}
		if (res->base.virt_hres < res->base.hres)
			res->base.virt_hres = res->base.hres;
		if (res->base.virt_vres < res->base.vres)
			res->base.virt_vres = res->base.vres;
		if (res->planes >= 24)
			linesize = (long)res->base.virt_hres * 4;
		else if (res->planes >= 15)
			linesize = (long)res->base.virt_hres * 2;
		else
			linesize = ((long)res->base.virt_hres * res->planes) >> 3;
		if (linesize > 32700)
		{
			res->base.virt_hres = orig_hres;
			res->base.virt_vres = orig_vres;
			do_dialog(rs_trindex[ERR_RES_TOO_LARGE]);
		} else
		{
			if ((linesize * res->base.virt_vres) > 1048576L) /* FIXME: hard coded */
			{
				res->base.virt_hres = orig_hres;
				res->base.virt_vres = orig_vres;
				do_dialog(rs_trindex[ERR_RES_TOO_LARGE]);
			}
		}
		sprintf(res->base.desc, "%4d * %4d, ET 4000", res->base.virt_hres, res->base.virt_vres);
		if (res->base.hres != res->base.virt_hres || res->base.vres != res->base.virt_vres)
		{
			strcat(res->base.desc, ", ");
			strcat(res->base.desc, rs_frstr[FS_VIRTUAL]);
		}
		res_tab = sort_restab(res_tab);
		set_items();
		redraw_obj(dialog, CHGRES_BOX);
	}
}


static void new_res(void)
{
	OBJECT *tree;
	struct res *res;

	/* TODO: nvdivga.inf currently cannot be rewritten */
	if (!magicpc)
		return;
	tree = rs_trindex[NEW_RES];
	sprintf(tree[NEW_HRES].ob_spec.tedinfo->te_ptext, "%4d", 640);
	sprintf(tree[NEW_VRES].ob_spec.tedinfo->te_ptext, "%4d", 480);
	if (do_dialog(tree) == NEWRES_OK)
	{
		res = (struct res *)Malloc(sizeof(*res));
		if (res != NULL)
		{
			res->base.selected = FALSE;
			res->base.desc = res->descbuf;
			res->base.vmode = SVEXT|0x200|STMODES|OVERSCAN|7;
			res->base.flags = FLAG_INFO;
			res->planes = valid_planes[bpp_index];
			switch (res->planes)
			{
			case 8:
				res->base.rez = nvdipc_driver_ids[0];
				break;
			case 15:
				res->base.rez = nvdipc_driver_ids[1];
				break;
			case 16:
				res->base.rez = nvdipc_driver_ids[2];
				break;
			case 32:
				res->base.rez = nvdipc_driver_ids[3];
				break;
			}
			res->base.virt_hres = atoi(tree[NEW_HRES].ob_spec.tedinfo->te_ptext);
			res->base.virt_vres = atoi(tree[NEW_VRES].ob_spec.tedinfo->te_ptext);
			if ((res->base.virt_hres & 15) != 0 || (res->base.virt_vres & 7) != 0)
			{
				res->base.virt_hres &= -16;
				res->base.virt_vres &= -8;
				do_dialog(rs_trindex[ERR_VIRTUAL_RES]);
			}
			if (res->base.virt_hres < 512)
				res->base.virt_hres = 512;
			if (res->base.virt_vres < 384)
				res->base.virt_vres = 384;
			res->base.hres = res->base.virt_hres;
			res->base.vres = res->base.virt_vres;
			res->freq = 0;
			res->mode_offset = 0;
			sprintf(res->base.desc, "%4d * %4d", res->base.virt_hres, res->base.virt_vres);
			res->base.next = res_tab;
			res_tab = sort_restab(res);
			possible_resolutions[bpp_index] = res_tab;
			set_items();
			lbox_set_asldr(color_lbox, 0, NULL);
			redraw_obj(dialog, CHGRES_BOX);
		}
	}
}


static void delete_res(void)
{
	struct res *res;
	struct res **respp;
	OBJECT *tree;

	/* TODO: nvdivga.inf currently cannot be rewritten */
	if (!magicpc)
		return;
	res = (struct res *)lbox_get_slct_item(color_lbox);
	if (res->base.flags & FLAG_INFO)
	{
		if (do_dialog(rs_trindex[RES_DELETE]) == RES_DELETE_OK)
		{
			respp = &res_tab;
			while (*respp != NULL)
			{
				if (*respp == res)
				{
					*respp = res->base.next;
					Mfree(res);
					cur_res = NULL;
					tree = rs_trindex[MAIN_DIALOG];
					tree[CHGRES_OK].ob_state |= OS_DISABLED;
					tree[CHGRES_OK].ob_flags &= ~OF_SELECTABLE;
					tree[CHGRES_OK].ob_flags &= ~OF_DEFAULT;
					tree[CHGRES_INFO].ob_state |= OS_DISABLED;
					tree[CHGRES_INFO].ob_flags &= ~OF_SELECTABLE;
					break;
				}
				respp = &(*respp)->base.next;
			}
			possible_resolutions[bpp_index] = res_tab;
			set_items();
			lbox_set_asldr(color_lbox, 0, NULL);
			redraw_obj(dialog, ROOT);
		}
	}
}
#endif


/* WDIALOG callback */

#pragma warn -par
static int cdecl hdl_obj(DIALOG *dialog, EVNT *events, WORD obj, WORD clicks, void *data)
{
	int index;
	OBJECT *tree;
	GRECT gr;

	wdlg_get_tree(dialog, &tree, &gr);

	if (obj < 0)
	{
		if (obj != HNDL_CLSD)
			return 1;
		return 0;
	}
	index = obj;
	if (clicks > 1)
		index |= 0x8000;
	if (lbox_do(color_lbox, index) == -1)
		obj = CHGRES_OK;
	switch (obj)
	{
	case CHGRES_BOX_FIRST:
	case CHGRES_BOX_FIRST+1:
	case CHGRES_BOX_FIRST+2:
	case CHGRES_BOX_FIRST+3:
	case CHGRES_BOX_FIRST+4:
	case CHGRES_BOX_FIRST+5:
	case CHGRES_BOX_FIRST+6:
	case CHGRES_BOX_FIRST+7:
	case CHGRES_BOX_FIRST+8:
	case CHGRES_BOX_LAST:
		#if 0
		if ((events != NULL) && (events->kstate == K_CTRL))
			delete_res();
		#endif
		break;
	case CHGRES_COLORS:
		index = simple_popup(tree, CHGRES_COLORS,
							 colour_mode_names, NUM_COLOUR_MODES,
							 dlg_curr_colour_mode);
		if (index >= 0)
		{
			dlg_curr_colour_mode = index;
			update_colour_mode();
			redraw_obj(dialog, CHGRES_COLORS);
		}

#if 0
		if (index >= 0)
		{
			bpp_index = popup_to_index(index);
			if (bpp_index >= 0)
			{
				res_tab = possible_resolutions[bpp_index];
				set_bpp(bpp_index);
				set_items();
				lbox_set_asldr(color_lbox, 0, NULL);
				cur_res = (struct res *)lbox_get_slct_item(color_lbox);
				if (cur_res != NULL)
				{
					tree[CHGRES_OK].ob_state &= ~OS_DISABLED;
					tree[CHGRES_OK].ob_flags |= OF_SELECTABLE;
					tree[CHGRES_OK].ob_flags |= OF_DEFAULT;
					if (cur_res->base.flags & FLAG_INFO)
					{
						tree[CHGRES_INFO].ob_state &= ~OS_DISABLED;
						tree[CHGRES_INFO].ob_flags |= OF_SELECTABLE;
					} else
					{
						tree[CHGRES_INFO].ob_state |= OS_DISABLED;
						tree[CHGRES_INFO].ob_flags &= ~OF_SELECTABLE;
					}
				} else
				{
					tree[CHGRES_OK].ob_state |= OS_DISABLED;
					tree[CHGRES_OK].ob_flags &= ~OF_SELECTABLE;
					tree[CHGRES_OK].ob_flags &= ~OF_DEFAULT;
					tree[CHGRES_INFO].ob_state |= OS_DISABLED;
					tree[CHGRES_INFO].ob_flags &= ~OF_SELECTABLE;
				}
				redraw_obj(dialog, ROOT);
			}
		}
#endif
		break;
	case CHGRES_INFO:
#if 0
		show_info(cur_res);
		tree[CHGRES_INFO].ob_state &= ~OS_SELECTED;
		redraw_obj(dialog, CHGRES_INFO);
#endif
		break;
	case CHGRES_NEW:
#if 0
		new_res();
		tree[CHGRES_NEW].ob_state &= ~OS_SELECTED;
		redraw_obj(dialog, CHGRES_NEW);
#endif
		break;
	case CHGRES_OK:
		must_shutdown = 1;
		return 0;
	case CHGRES_CANCEL:
		must_shutdown = 0;
		return 0;
	}
	return 1;
}
#pragma warn +par


/*************************************************************
*
* main screen mode dialogue
*
*************************************************************/
static int select_res(void)
{
	OBJECT *tree;
	EVNT events;
	int dummy;
	static const GRECT empty_grect = { 0, 0, 0, 0 };

	tree = rs_trindex[MAIN_DIALOG];

	/* Hide unused "Info.." and "New.." buttons */

	tree[CHGRES_INFO].ob_flags |= OF_HIDETREE;
	tree[CHGRES_NEW].ob_flags |= OF_HIDETREE;

/*
	if (dlg_curr_colour_mode != -1)
	{
		tree[CHGRES_OK].ob_state &= ~OS_DISABLED;
		tree[CHGRES_OK].ob_flags |= OF_SELECTABLE;
		tree[CHGRES_OK].ob_flags |= OF_DEFAULT;
		tree[CHGRES_INFO].ob_state &= ~OS_DISABLED;
		tree[CHGRES_INFO].ob_flags |= OF_SELECTABLE;
	} else
	{
		tree[CHGRES_OK].ob_state |= OS_DISABLED;
		tree[CHGRES_OK].ob_flags &= ~OF_SELECTABLE;
		tree[CHGRES_OK].ob_flags &= ~OF_DEFAULT;
		tree[CHGRES_INFO].ob_state |= OS_DISABLED;
		tree[CHGRES_INFO].ob_flags &= ~OF_SELECTABLE;
	}
	*/
	#if 0
	if (magicpc)
	{
	} else
	{
		tree[CHGRES_INFO].ob_flags &= ~OF_HIDETREE;
		tree[CHGRES_NEW].ob_flags |= OF_HIDETREE;
	}
	#endif

	update_colour_mode();
	dialog = wdlg_create(hdl_obj, tree, NULL, 0, NULL, 0);
	if (dialog != NULL)
	{
		color_lbox = lbox_create(
							tree,
							lbox_select_item_cb, lbox_set_item_cb,
							screen_size_lbox_item_tab,
							CHGRES_BOX_LAST - CHGRES_BOX_FIRST + 1,
							0,
			ctrl_objs, objs,
			LBOX_VERT|LBOX_AUTO|LBOX_AUTOSLCT|LBOX_REAL|LBOX_SNGL, 40,
			dialog, dialog, 0, 0, 0, 0);
		if (color_lbox != NULL)
		{
			if (wdlg_open(dialog, rs_frstr[FS_CHANGE_RES], NAME|CLOSER|MOVER, -1, -1, 0, NULL))
			{
				do
				{
					events.mwhich = evnt_multi(
										MU_KEYBD | MU_BUTTON | MU_MESAG, /* event types */
										2, 1, 1, /* nclicks, bmask, bstate */
										0, &empty_grect,
										0, &empty_grect,
										events.msg,			/* msgbuf */
										0UL,					/* ms */
										(EVNTDATA *) &events.mx,
										&events.key,
										&events.mclicks);
				} while (wdlg_evnt(dialog, &events) != 0);
				wdlg_close(dialog, &dummy, &dummy);
			}
			lbox_delete(color_lbox);
		}
		wdlg_delete(dialog);
	}
	return must_shutdown;
}


/*************************************************************
*
* Fine arrange some GUI objects if AES supports 3D effects
*
*************************************************************/
static void fix_3d(OBJECT *tree)
{
	tree[CHGRES_UP].ob_spec.obspec.framesize = 1;		/* list box up arrow */
	tree[CHGRES_DOWN].ob_spec.obspec.framesize = 1;
	tree[CHGRES_SLIDER].ob_spec.obspec.framesize = 1;
	tree[CHGRES_BACK].ob_spec.obspec.interiorcol = 1;	/* list box slider background */
	tree[CHGRES_BACK].ob_spec.obspec.fillpattern = 1;
}


/*************************************************************
*
* Fine arrange some GUI objects
*
* Note that this would not be necessary if the Resource
* Editor was able to write the upper byte of the positions
* accordingly as -128..+127 pixels offset.
*
*************************************************************/
static void fix_rsc(void)
{
	int dummy;
	OBJECT *tree;
	RSHDR *rs_hdr;

	/* get header of loaded resource */
	WORD *aes_global = _GemParBlk.global;
	rs_hdr = *((void **) &aes_global[7]);

	rs_trindex = (OBJECT **)(((char *) rs_hdr + rs_hdr->rsh_trindex));
	rs_frstr = (char **)(((char *)rs_hdr + rs_hdr->rsh_frstr));

	/* move scroll arrows one pixel up to align them to the list box */
	tree = rs_trindex[MAIN_DIALOG];
	tree[CHGRES_UP].ob_y -= 1;
	tree[CHGRES_DOWN].ob_y -= 1;

	/* move colour icon down */
	tree[CHGRES_ICON].ob_y += (tree[CHGRES_ICON].ob_height - tree[CHGRES_ICON].ob_spec.iconblk->ib_hicon) / 2;

	/* adjustments for newer AES versions. TODO: Can this be done in Resource Editor? */
	if (objc_sysvar(SV_INQUIRE, AD3DVAL, 0, 0, &dummy, &dummy) == 0)
	{
		fix_3d(tree);
	}
}


/*************************************************************
*
* Pass control to SHUTDOWN to execute resolution change
*
* switch resolution:
*
*	SHUTDOWN dev [txth]
*
* switch Falcon resolution:
*
*	SHUTDOWN dev txth xdv
*
* txth is text height, usually 0.
*
*************************************************************/
static int start_shutdown(void)
{
	char buf[64];
	buf[0] = (char) sprintf(&buf[1], "%d 0 %d", dlg_curr_colour_mode, 0x4711);
	shel_write(1, 1, 1, "SHUTDOWN.PRG", buf); /* BUG: return value ignored */
	return 0;
}


/*************************************************************
*
* main function
*
*************************************************************/
int main(void)
{
	if (appl_init() < 0)
	{
		DBGOUT("appl_init() failed\n");
		return -1;
	}

	aes_handle = graf_handle(&gl_wchar, &gl_hchar, &gl_wbox, &gl_hbox);
	graf_mouse(ARROW, NULL);
	if (!rsrc_load("CHGRES.RSC"))
	{
		DBGOUT("rsrc_load() failed\n");
		appl_exit();
		return -2;
	}

	/* do some fine adjustments of GUI objects */
	fix_rsc();

	molGetVideoMode();
	initListBox();

	if (select_res())
	{
		if (start_shutdown() == 0)
		{
			do_dialog(rs_trindex[ERR_RESCHG]);
		} else
		{
			/*
			change_magx_inf(cur_res->base.rez, cur_res->base.vmode);
			*/
		}
	}

	rsrc_free();
	appl_exit();
	return 0;
}
