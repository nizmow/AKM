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
* LISTBOX: https://freemint.github.io/tos.hyp/de/lbox.html
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
#define DBGOUT(...)
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

#define N_ITEMS (CHGRES_BOX_LAST - CHGRES_BOX_FIRST + 1)
static const int ctrl_objs[5] =
{
	CHGRES_BOX, CHGRES_UP, CHGRES_DOWN, CHGRES_BACK, CHGRES_SLIDER
};

static const int objs[N_ITEMS] =
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


#define NUM_COLOUR_MODES 7
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

#define RES_ST_LOW 0
#define RES_ST_MID 1
#define RES_ST_HIGH 2

#define SIZE_ST_LOW 3
#define SIZE_ST_MID 5
#define SIZE_ST_HIGH 6

#define NUM_SCREEN_SIZES 18
static const uint16_t screen_sizes[NUM_SCREEN_SIZES][2] =
{
	/* Atari compatibility modes */
	{  320,  200 },		/* forces colour mode 2 */
	{  640,  200 },		/* forces colour mode 1 */
	{  640,  400 },		/* forces colour mode 0 */
	/* general modes */
	{  320,  200 },		/* SIZE_ST_LOW */
	{  512,  384 },
	{  640,  200 },		/* SIZE_ST_MID */
	{  640,  400 },		/* SIZE_ST_HIGH */
	{  640,  480 },
	{  800,  608 },
	{  720,  576 },
	{ 1024,  768 },
	{ 1280,  720 },
	{ 1280,  960 },
	{ 1280, 1024 },
	{ 1600, 1200 },
	{ 1920, 1080 },
	{ 1920, 1200 },
	{ 2560, 1440 }
};

static char screen_sizes_names[NUM_SCREEN_SIZES][22];

static LBOX_ITEM screen_size_lbox_item_tab[NUM_SCREEN_SIZES];

struct MagiCVideoMode
{
	uint16_t length;		/* length of this struct, for forward compatibility */
	uint32_t magic;			/* 0x14534711, if valid */
	uint32_t ncolours;		/* 2,4,16,256,32768,65536,2^24 */
	uint16_t bits;			/* 1,2,4,8,16,32 */
	uint16_t flags;			/* Bit 0: packed, Bit 1: direct colour, Bit 2: double vertical */
	uint16_t width;			/* screen width */
	uint16_t height;		/* screen height */
};

static struct MagiCVideoMode currMode;
static LIST_BOX *colour_lbox;
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
* Tell MagiC emulator the next video mode after warm boot
*
* Uses Vsetrgb() with an invalid colour index higher than 255
*
*************************************************************/
static long molSetVideoMode(void)
{
	long ret;

	switch(dlg_curr_colour_mode)
	{
		case 0:
			currMode.ncolours = 2;
			currMode.bits = 1;
			currMode.flags = 0;
			break;

		case 1:
			currMode.ncolours = 4;
			currMode.bits = 2;
			currMode.flags = 0;
			break;

		case 2:
			currMode.ncolours = 16;
			currMode.bits = 4;
			currMode.flags = 0;
			break;

		case 3:
			currMode.ncolours = 16;
			currMode.bits = 4;
			currMode.flags = 1;	/* packed */
			break;

		case 4:
			currMode.ncolours = 256;
			currMode.bits = 8;
			currMode.flags = 1;	/* packed */
			break;

		case 5:
			currMode.ncolours = 32 * 1024L;
			currMode.bits = 16;
			currMode.flags = 3;
			break;

		case 6:
			currMode.ncolours = 16 * 1024L * 1024L;
			currMode.bits = 32;
			currMode.flags = 3;
			break;
	}

	if (dlg_curr_screen_size >= 0)
	{
		currMode.width = screen_sizes[dlg_curr_screen_size][0];
		currMode.height = screen_sizes[dlg_curr_screen_size][1];
	}

	if (dlg_curr_screen_size == RES_ST_MID)
	{
		currMode.flags |= 4;	/* double scan */
	}

	ret = xbios(/*VsetRGB*/ 93, (uint16_t) 0xf000, (uint16_t) 0, &currMode);

	/*
	DBGOUT("vmode.length    = %u\n", currMode.length);
	DBGOUT("vmode.magic     = 0x%08lx\n", currMode.magic);
	DBGOUT("vmode.ncolours  = %lu\n", currMode.ncolours);
	DBGOUT("vmode.bits      = %u\n", currMode.bits);
	DBGOUT("vmode.flags     = 0x%04x\n", currMode.flags);
	DBGOUT("vmode.width     = %u\n", currMode.width);
	DBGOUT("vmode.height    = %u\n", currMode.height);
	*/

	return ret;
}


/*************************************************************
*
* Initialise LISTBOX with colour modes and resolutions
*
*************************************************************/
static void initListBox(void)
{
	unsigned i;

	for (i = 0; i < NUM_SCREEN_SIZES; i++)
	{
		uint16_t w = screen_sizes[i][0];
		uint16_t h = screen_sizes[i][1];
		sprintf(screen_sizes_names[i], "%4u * %4u", w, h);	/* 12 characters */
		if (i == RES_ST_LOW)
		{
			strcat(screen_sizes_names[i], " (ST low)");		/* +9 characters */
		}
		else
		if (i == RES_ST_MID)
		{
			strcat(screen_sizes_names[i], " (ST mid)");		/* +9 characters */
		}
		else
		if (i == RES_ST_HIGH)
		{
			strcat(screen_sizes_names[i], " (ST high)");	/* +10 characters */
		}

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


/*************************************************************
*
* redraw a single dialogue OBJECT
*
*************************************************************/
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
* Popup for colour modes
* return selected index or -1 on error
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
	OBJECT *o;
	int y;
	int sel_y;


	obj = &tree[obj_index];
	for (i = maxlen = 0; i < num_names; i++)
	{
		int len = (int)strlen(names[i]);
		if (len > maxlen)
			maxlen = (int)len;
	}

	newtree = (OBJECT *) Malloc((num_names * 2 + 1) * sizeof(OBJECT));
	if (newtree == NULL)
	{
		#if 0
		DBGOUT("ERR: out of memory\n");
		#endif
		return -1;
	}

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
* Update popup in main dialogue to show selected colour mode
*
*************************************************************/
static void update_colour_mode(void)
{
	rs_trindex[MAIN_DIALOG][CHGRES_COLORS].ob_spec.free_string =
			 (char *) colour_mode_names[dlg_curr_colour_mode];
}


/*************************************************************
*
* LISTBOX callback to initialise a GEM object with a listbox item
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


/*************************************************************
*
* LISTBOX callback for selecting an item
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
	int new_colour_mode = dlg_curr_colour_mode;

	if (item != NULL)
	{
		dlg_curr_screen_size = item->data1;

		/*
		if (dlg_curr_colour_mode == -1)
		{
			tree[CHGRES_OK].ob_state &= ~OS_DISABLED;
			tree[CHGRES_OK].ob_flags |= OF_SELECTABLE;
			tree[CHGRES_OK].ob_flags |= OF_DEFAULT;
			redraw_obj(dialog, CHGRES_OK);
		}
		*/

		if ((dlg_curr_screen_size == RES_ST_LOW) &&
		    (dlg_curr_colour_mode != 2))
		{
			new_colour_mode = 2;
		}
		else
		if ((dlg_curr_screen_size == RES_ST_MID) &&
		    (dlg_curr_colour_mode != 1))
		{
			new_colour_mode = 1;
		}
		else
		if ((dlg_curr_screen_size == RES_ST_HIGH) &&
		    (dlg_curr_colour_mode != 0))
		{
			new_colour_mode = 0;
		}

		if (new_colour_mode != dlg_curr_colour_mode)
		{
			dlg_curr_colour_mode = new_colour_mode;
			update_colour_mode();
			redraw_obj(dialog, CHGRES_COLORS);
		}
	}
}
#pragma warn +par


/*************************************************************
*
* WDIALOG callback
*
* return zero to leave dialogue
*
*************************************************************/
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
		return 0;	/* dialogue has been closed */
	}

	index = obj;
	if (clicks > 1)
	{
		index |= 0x8000;	/* double click on object */
	}

	/* handle listbox items and control objects */

	if (lbox_do(colour_lbox, index) == -1)
	{
		obj = CHGRES_OK;	/* TODO: double click on item? */
	}

	switch (obj)
	{
		case CHGRES_COLORS:
			index = simple_popup(tree, CHGRES_COLORS,
							 colour_mode_names, NUM_COLOUR_MODES,
							 dlg_curr_colour_mode);
			if (index >= 0)
			{
				int new_screen_size = dlg_curr_screen_size;

				/* update colour mode popup title */

				dlg_curr_colour_mode = index;
				update_colour_mode();
				redraw_obj(dialog, CHGRES_COLORS);
				
				/* update listbox, if necessary */
				
				if ((dlg_curr_screen_size == RES_ST_LOW) &&
				    (dlg_curr_colour_mode != 2))
				{
					new_screen_size = SIZE_ST_LOW;
				}
				else
				if ((dlg_curr_screen_size == RES_ST_MID) &&
				    (dlg_curr_colour_mode != 1))
				{
					new_screen_size = SIZE_ST_MID;
				}
				else
				if ((dlg_curr_screen_size == RES_ST_HIGH) &&
				    (dlg_curr_colour_mode != 0))
				{
					new_screen_size = SIZE_ST_HIGH;
				}

				if (new_screen_size != dlg_curr_screen_size)
				{
					/* deselect current entry */
					screen_size_lbox_item_tab[dlg_curr_screen_size].selected = 0;
					dlg_curr_screen_size = new_screen_size;

					/* select new entry */
					screen_size_lbox_item_tab[dlg_curr_screen_size].selected = 1;
					lbox_update(colour_lbox, NULL);
					redraw_obj(dialog, CHGRES_BOX);
				}
			}
			break;

		case CHGRES_OK:
			must_shutdown = 1;
			return 0;	/* end dialogue */

		case CHGRES_CANCEL:
			must_shutdown = 0;
			return 0;	/* end dialogue */
		}

	return 1;
}
#pragma warn +par


/*************************************************************
*
* main screen mode dialogue
* return -1 on error
*
*************************************************************/
static int select_res(void)
{
	OBJECT *tree;
	EVNT events;
	int dummy;
	static const GRECT empty_grect = { 0, 0, 0, 0 };
	tree = rs_trindex[MAIN_DIALOG];

	/*
	* create WDIALOG and LISTBOX
	*/

	dialog = wdlg_create(hdl_obj, tree, NULL, 0, NULL, 0);
	if (dialog == NULL)
	{
		#if 0
		DBGOUT("wdlg_create() failed\n");
		#endif
		return -1;
	}

	colour_lbox = lbox_create(
						tree,
						lbox_select_item_cb, lbox_set_item_cb,
						screen_size_lbox_item_tab,
						CHGRES_BOX_LAST - CHGRES_BOX_FIRST + 1,
						0,
						ctrl_objs, objs,
						LBOX_VERT|LBOX_AUTO|LBOX_AUTOSLCT|LBOX_REAL|LBOX_SNGL, 40,
						dialog, dialog, 0, 0, 0, 0);
	if (colour_lbox == NULL)
	{
		#if 0
		DBGOUT("lbox_create() failed\n");
		#endif
		wdlg_delete(dialog);
		return -1;
	}

	/*
	* Initialise dialogue with current settings
	*/

	if (dlg_curr_screen_size >= 0)
	{
		lbox_scroll_to(colour_lbox, dlg_curr_screen_size, NULL, NULL);
	}

	/* Hide unused "Info.." and "New.." buttons */

	tree[CHGRES_INFO].ob_flags |= OF_HIDETREE;
	tree[CHGRES_NEW].ob_flags |= OF_HIDETREE;

	update_colour_mode();

	/*
	* Open and run dialogue
	*/

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
		}
		while (wdlg_evnt(dialog, &events) != 0);

		wdlg_close(dialog, &dummy, &dummy);
	}

	lbox_delete(colour_lbox);
	wdlg_delete(dialog);

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
* For the emulator the resolution change does not work, thus
* a warmboot is initiated, with parameter "-w".
*
*************************************************************/
static void start_shutdown(void)
{
	char buf[64];
	#if 0

	/* initiate resolution change */
	buf[0] = (char) sprintf(&buf[1], "%d 0 %d", dlg_curr_colour_mode, 0x4711);

	#else

	/* initiate warmboot */
	buf[0] = (char) sprintf(&buf[1], "-w");

	#endif

	shel_write(1, 1, 1, "SHUTDOWN.PRG", buf); /* BUG: return value ignored */
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
		#if 0
		DBGOUT("appl_init() failed\n");
		#endif
		return -1;
	}

	aes_handle = graf_handle(&gl_wchar, &gl_hchar, &gl_wbox, &gl_hbox);
	graf_mouse(ARROW, NULL);
	if (!rsrc_load("CHGRES.RSC"))
	{
		#if 0
		DBGOUT("rsrc_load() failed\n");
		#endif
		appl_exit();
		return -2;
	}

	/* do some fine adjustments of GUI objects */
	fix_rsc();

	molGetVideoMode();
	initListBox();

	if (select_res())
	{
		if ((dlg_curr_colour_mode != original_colour_mode) ||
		    (dlg_curr_screen_size != original_screen_size))
		{
			/* tell new video mode to emulator */
			if (molSetVideoMode() == 0)
			{
				start_shutdown();
			}
			else
			{
				do_dialog(rs_trindex[ERR_RESCHG]);
			}
		}
	}

	rsrc_free();
	appl_exit();
	return 0;
}
