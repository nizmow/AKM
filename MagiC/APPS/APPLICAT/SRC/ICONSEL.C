/*
*
* Enthält die spezifischen Routinen für das
* Fenster zur Icon-Auswahl
*
*/

#include "APPLICAT_GEM.H"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "IMPORT/GEMUT_MT.H"
#include "APPLICAT.H"
#include "WINDOWS.H"
#include "APPL.H"
#include "INF.H"
#include "APPLDATA.H"
#include "ICONSEL.H"
#include "ICA_DIAL.H"
#include "ICP_DIAL.H"
#include "SPC_DIAL.H"
#define  WXOFFS	3
#define  WYOFFS	1
#define  WXDIST	3
#define  WYDIST	1

/* Daten fürs Fenster */
static WINDOW iconwindow;

WINDOW *mywindow = NULL;


/****************************************************************
*
* Nach der Bearbeitung eines Einfach-Mausklicks müssen wir
* prüfen, ob die Maustaste noch gedrückt ist. Insbesondere auf
* schnellen Systemen müssen wir dem Anwender etwas Zeit lassen,
* die linke Maustaste wieder loszulassen, ansonsten wird immer
* ein "Drag" erkannt.
*
* Rückgabe TRUE bei "timeout", d.h. Maustaste noch gedrückt.
*
****************************************************************/

static int detect_mouse_drag(EVNT *w_ev)
{
	MOBLK m1 = {0, 0, 0, 0, 0};
	MOBLK m2 = {0, 0, 0, 0, 0};

	EVNT_multi(MU_BUTTON + MU_TIMER, 1, 1, 0, &m1, &m2,
			MOUSE_DRAG_DETECT_MS, w_ev);

	return (w_ev->mwhich & MU_TIMER) != 0;
}


/****************************************************************
*
* Für Drag & Drop von Objekten
*
* Malt alle Objekte in der Liste <mov_boxes[num_boxes]>.
* Wird als Begrenzung <glimit> != NULL angegeben, wird verhindert,
*  daß die Objekte außerhalb des Bildschirms liegen.
*
****************************************************************/

static int xrel, yrel;	/* TODO: unused? */

static void draw_boxes
(
	const OBJECT *tree,
	const OBJECT *mov_boxes[],
	int num_boxes,
	const GRECT *glimit,
	int xoff,
	int yoff
)
{
	int px, py;
	short pxy[10];


	vswr_mode(vdi_handle, MD_XOR);
	vsl_type(vdi_handle, DASH);

	if (glimit)
	{
		if (glimit->g_x + xoff < scrg.g_x)
			xoff = scrg.g_x - glimit->g_x;
		if (glimit->g_y + yoff < scrg.g_y)
			yoff = scrg.g_y - glimit->g_y;
		if (glimit->g_x + glimit->g_w + xoff > scrg.g_x + scrg.g_w)
			xoff = scrg.g_x + scrg.g_w - (glimit->g_x + glimit->g_w);
		if (glimit->g_y + glimit->g_h + yoff > scrg.g_y + scrg.g_h)
			yoff = scrg.g_y + scrg.g_h - (glimit->g_y + glimit->g_h);
	}

	px = tree->ob_x + xoff;
	py = tree->ob_y + yoff;
	xrel = xoff;
	yrel = yoff;

	for (; num_boxes > 0; num_boxes--, mov_boxes++)
	{
		pxy[0] = (*mov_boxes)->ob_x + px;
		pxy[1] = (*mov_boxes)->ob_y + py;

		pxy[2] = pxy[0] + (*mov_boxes)->ob_width;
		pxy[3] = pxy[1];

		pxy[4] = pxy[0] + (*mov_boxes)->ob_width;
		pxy[5] = pxy[1] + (*mov_boxes)->ob_height;

		pxy[6] = pxy[0];
		pxy[7] = pxy[1] + (*mov_boxes)->ob_height;

		pxy[8] = pxy[0];
		pxy[9] = pxy[1];

		v_pline(vdi_handle, 5, pxy);		/* Rechteck malen */
	}

	vswr_mode(vdi_handle, MD_REPLACE);
}


/****************************************************************
*
* Verschieben von selektierten Objekten.
* <wnr>,<tree>		: Nummer und Baum der selektierten Objekte
* <x_koor>,<y_koor>	: Anklickpunkt in absoluten Koordinaten
* KEIN BEG_UPDATE ausgeführt
*
****************************************************************/

void move_icons(int x_koor, int y_koor, OBJECT *tree, int objnr)
{
	short clip_rect[4];
	GRECT g;
	GRECT *ptr_g;
	const OBJECT *mov_boxes[1];
	EVNTDATA ev;
	int old_x,old_y;
	OBJECT *target_tree;
	int target_whdl;
	short target_obj;
	short prev_obj = 0;
	OBJECT *prev_tree = NULL;
	void (*set_icon)(int iconnr, int objnr);
	void (*drawfn)(int objnr);
	void (*prev_drawfn)(int objnr) = NULL;


	mov_boxes[0] = tree + objnr;		/* Tabelle aller Objekte */
	ptr_g = &g;
	objc_grect(tree, objnr, ptr_g);	/* Hülle aller Objekte */

	wind_update(BEG_UPDATE);
	graf_mouse(M_OFF, NULL);
	graf_mouse(FLAT_HAND, NULL);

	clip_rect[0] = scrg.g_x;
	clip_rect[1] = scrg.g_y;
	clip_rect[2] = scrg.g_x + scrg.g_w - 1;
	clip_rect[3] = scrg.g_y + scrg.g_h - 1;
	vs_clip	(vdi_handle, TRUE, clip_rect);

	ev.x = x_koor;
	ev.y = y_koor;
	do
	{
		vs_clip	(vdi_handle, TRUE, clip_rect);
		draw_boxes(tree, mov_boxes, 1, ptr_g,
				 ev.x-x_koor, ev.y-y_koor);
		graf_mouse(M_ON, NULL);

		old_x = ev.x;
		old_y = ev.y;

		do
		{
			graf_mkstate(&ev.x, &ev.y, &ev.bstate, &ev.kstate);
		}
		while ((ev.x == old_x) && (ev.y == old_y) && (ev.bstate & 1));

		graf_mouse(M_OFF, NULL);
		draw_boxes(tree, mov_boxes, 1, ptr_g,
				 old_x - x_koor, old_y - y_koor);

		target_whdl = wind_find(ev.x, ev.y);
		if (!ica_get_target_obj(ev.x, ev.y, target_whdl, &target_tree,
				&target_obj, &set_icon, &drawfn) &&
			 !icp_get_target_obj(ev.x, ev.y, target_whdl, &target_tree,
				&target_obj, &set_icon, &drawfn) &&
			 !spc_get_target_obj(ev.x, ev.y, target_whdl, &target_tree,
				&target_obj, &set_icon, &drawfn))
		{
			target_tree = NULL;
			target_obj = -1;
		}

		if ((target_tree != prev_tree) || (target_obj != prev_obj))
		{
			if (prev_tree && prev_obj > 0)
			{
				prev_drawfn(prev_obj);
			}

			if (target_tree && target_obj > 0)
			{
				ob_sel(target_tree, target_obj);
				drawfn(target_obj);
				ob_dsel(target_tree, target_obj);
			}

			prev_tree = target_tree;
			prev_obj  = target_obj;
			prev_drawfn = drawfn;
		}
	}
	while (ev.bstate & 1);

	if (target_obj > 0)
	{
		drawfn(target_obj);
		set_icon(objnr - 1, target_obj);
	}

	graf_mouse(ARROW, NULL);
	graf_mouse(M_ON, NULL);
	wind_update(END_UPDATE);
}


/*******************************************************************
*
* Taste
*	Cursor up        : 0x4800
*	Shift Cursor up  : 0x4838
*	Cursor down      : 0x5000
*	Shift Cursor down: 0x5032
*	Page up          : 0x4900
*	Page up          : 0x5100
*
*******************************************************************/

static void hdl_key_iconsel(WINDOW *w, int, int key)
{
	short message[8];
	int msg4 = -1;
	/*
	printf("0x%04x ", key);
	*/

	switch(key)
	{
		case 0x4800:
			msg4 = WA_UPLINE;
			break;

		case 0x5000:
			msg4 = WA_DNLINE;
			break;

		case 0x4900:
		case 0x4838:
			msg4 = WA_UPPAGE;
			break;

		case 0x5100:
		case 0x5032:
			msg4 = WA_DNPAGE;
			break;
	}

	/* schicke mir selbst eine Nachricht */
	if (msg4 >= 0)
	{
		message[0] = WM_ARROWED;
		message[1] = ap_id;
		message[2] = 0;
		message[3] = w->handle;
		message[4] = msg4;
		message[5] =
		message[6] = message[7] = 0;
		appl_write(ap_id, 16, message);
	}
}


/*******************************************************************
*
* Mausklick
*
*******************************************************************/

static void button_iconsel(WINDOW *w, int,
				int x, int y, int, int nclicks)
{
	short objnr;
	EVNT ev_drag;
	int mouse_drag;


	objnr = find_obj(w->tree, x, y);
	if (objnr <= 0)
	{
		wind_set_int(w->handle, WF_TOP, 0);
		return;		/* kein Objekt unter dem Mausklick */
	}

	if (nclicks == 2)
	{
		/* Doppelklick */
		
		/* user feedback */
		ob_sel(w->tree, objnr);
		window_draw_obj(w->handle, w->tree, objnr);
		evnt_timer(200);
		ob_dsel(w->tree, objnr);
		window_draw_obj(w->handle, w->tree, objnr);

		/* Icon zuweisen */
		ica_dial_set_icon(objnr - 1);	/* Icon -> "Registrierte Anwendungen" */
		icp_dial_set_icon(objnr - 1);	/* Icon -> "Registrierte Pfade" */
		spc_dial_set_icon(objnr - 1);	/* Icon -> "Standard-Icons" */
		return;
	}

	mouse_drag = detect_mouse_drag(&ev_drag);
	if (mouse_drag)
	{
		move_icons(x, y, w->tree, objnr);
		return;
	}

	if (selected(w->tree, objnr))
		ob_dsel(w->tree, objnr);
	else
		ob_sel(w->tree, objnr);

	window_draw_obj(w->handle, w->tree, objnr);
}


/*******************************************************************
*
* Initialisierung
*
*******************************************************************/

void init_iconsel(void)
{
	WINDEFPOS *w;

	w = def_wind_pos(IDENT_ICONS);
	if (w)
	{
		iconwindow.out = w->g;
	}
	else
	{
		iconwindow.out = scrg;
	}
}


/*******************************************************************
*
* Initialisierung
*
*******************************************************************/

static void close_iconsel(WINDOW *w, int)
{
	WINDEFPOS *wd;
	WINDOW **sw;

	wd = def_wind_pos(IDENT_ICONS);
	if (wd)
		wd->g = iconwindow.out;

	wind_close(w->handle);
	wind_delete(w->handle);
	Mfree(w->tree);
	sw = find_slot_window(w);
	mywindow = NULL;
	*sw = NULL;
}


/*******************************************************************
*
* Fenster öffnen
*
*******************************************************************/

int open_iconsel(void)
{
	WINDOW **w;
	register OBJECT *o,*sob;
	ICONBLK *icb;
	register int i;


	if (mywindow)		/* schon geöffnet */
	{
		wind_set(mywindow->handle, WF_TOP, 0,0,0,0);
		return 0;	/* OK */
	}
	w = new_window();
	if (!w)
		return -1;		/* kein Slot */
	iconwindow.handle = wind_create_grect(NAME+CLOSER+FULLER+MOVER+
						SIZER+UPARROW+DNARROW+VSLIDE,
						&scrg);

	iconwindow.tree = (OBJECT *) Malloc(sizeof(OBJECT) * (icnn + 1));
	if (!iconwindow.tree)
		return -3;

	if (iconwindow.handle < 0)
	{
		Mfree(iconwindow.tree);
		Rform_alert(1, ALRT_ERRWINDOPEN, NULL);
		return -2;		/* kein AES-Fenster mehr übrig */
	}

	/* Baum aufbauen */

	o = iconwindow.tree;
	/* Objekt der weißen Hintergrundbox */
	o->ob_next = -1;
	o->ob_type = G_BOX;
	o->ob_state = NORMAL;
	o->ob_spec.index = (long) (WHITE);
	if (icnn > 0)
	{
		o->ob_flags = NONE;
		o->ob_head = 1;
		o->ob_tail = icnn;
	}
	else
	{
		o->ob_flags = LASTOB;
		o->ob_head = o->ob_tail = -1;
	}

	iconwindow.max_hicon = 0;
	for (i = 1; i <= icnn; i++)
	{
		o++;
		o->ob_next = (i < icnn) ? (i + 1) : (0);
		o->ob_head = o->ob_tail = -1;
		o->ob_flags = (i < icnn) ? NONE : LASTOB;
		sob = rscx[icnx[i-1].rscfile].adr_icons + icnx[i - 1].objnr;

		icb = NULL;
		if (sob->ob_type == G_USERDEF)
			icb = (ICONBLK *) sob->ob_spec.userblk->ub_parm;
		else
		if (sob->ob_type == G_ICON)
			icb = sob->ob_spec.iconblk;
		else
		if (sob->ob_type == G_CICON)
			icb = &sob->ob_spec.ciconblk->monoblk;
		else
			icb = NULL;

		if (icb && (icb->ib_hicon > iconwindow.max_hicon))
			iconwindow.max_hicon = icb->ib_hicon;

		o->ob_type   = sob->ob_type;
		o->ob_width  = sob->ob_width;
		o->ob_height = sob->ob_height;
		o->ob_spec   = sob->ob_spec;
		o->ob_state  = NORMAL+WHITEBAK;
	}

	init_window(&iconwindow);
	iconwindow.flags = 0;
	iconwindow.is_1col = FALSE;
	iconwindow.showw = 72;
	iconwindow.showh = iconwindow.max_hicon + 8;
	iconwindow.shownum = icnn;
	iconwindow.xoffs = WXOFFS;
	iconwindow.yoffs = WYOFFS;
	iconwindow.xdist = WXDIST;
	iconwindow.ydist = WYDIST;
	iconwindow.hdl_key = hdl_key_iconsel;
	iconwindow.button = button_iconsel;
	iconwindow.close = close_iconsel;
	*w = mywindow = &iconwindow;
	wind_set_str(iconwindow.handle, WF_NAME,
				Rgetstring(STR_WTIT_ICONS, NULL));
	iconwindow.open(&iconwindow);
	wind_set(iconwindow.handle, WF_BEVENT, 0x0001, 0, 0, 0);
	return 0;
}
