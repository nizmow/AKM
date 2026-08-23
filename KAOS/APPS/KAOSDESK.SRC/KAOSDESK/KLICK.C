/*********************************************************************
*
* Dieses Modul enthÑlt die Bearbeitung aller Mausklicks, also 
* Verschieben und Aktivierung von Icons, Doppelklicks in Fenster usw.
*
*********************************************************************/

#include <stdio.h>		/* for printf debugging */
#include <string.h>
#include <mgx_dos.h>	/* include before tos.h */
#include <tos.h>
#include <vdi.h>
#include <tosdefs.h>
#include "k.h"


/****************************************************************
*
* Malt alle Objekte in der Liste <mov_boxes[anz_boxes]>.
* Wird als Begrenzung <grenz> != NULL angegeben, wird verhindert,
*  daû die Objekte auûerhalb des Bildschirms liegen.
*
****************************************************************/

static int xrel, yrel;

void draw_boxes(OBJECT *tree, OBJECT *mov_boxes[], int num_boxes,
			 GRECT *grenz, int xoff, int yoff)
{
	int px, py;
	int pxy[10];


	vswr_mode(vdi_handle, MD_XOR);
	set_deflt_clip();
	vsl_type(vdi_handle, DASH);

	if (grenz)
	{
		if (grenz->g_x + xoff < desk_g.g_x)
			xoff = desk_g.g_x - grenz->g_x;
		if (grenz->g_y + yoff < desk_g.g_y)
			yoff = desk_g.g_y - grenz->g_y;
		if (grenz->g_x + grenz->g_w + xoff > desk_g.g_x + desk_g.g_w)
			xoff = desk_g.g_x + desk_g.g_w - (grenz->g_x + grenz->g_w);
		if (grenz->g_y + grenz->g_h + yoff > desk_g.g_y + desk_g.g_h)
			yoff = desk_g.g_y + desk_g.g_h - (grenz->g_y + grenz->g_h);
	}
	px = tree->ob_x + xoff;
	py = tree->ob_y + yoff;
	xrel = xoff;
	yrel = yoff;

	for	(; num_boxes > 0; num_boxes--,mov_boxes++)
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

		v_pline(vdi_handle,5,pxy);		/* Rechteck malen */
	}
	vswr_mode(vdi_handle, MD_REPLACE);
}


/****************************************************************
*
* Verschieben von selektierten Desktop- Icons um <x>,<y>
*
****************************************************************/

static int mvx,mvy;

static int mv_obj(int wnr, int obj)
{
	GRECT g;
	register OBJECT *tree = windows[wnr].pobj;

	objc_grect(tree, obj, &g);
	(tree+obj)->ob_x += mvx;
	(tree+obj)->ob_y += mvy;
	redraw(wnr, &g);
	draw_wind_obj(wnr, obj);
	return 0;
}

static void mv_dskt_icns(int x, int y)
{
	if (x == 0 && y == 0)
		return;
	mvx = x;
	mvy = y;
	walk_wind_sel_objs(0, mv_obj);
}


/****************************************************************
*
* Bringt ein neues Icon ins Desktop. Es soll ein Icon mit Typ
*  <type>, absoluter Position <x,y>, (optionalem) Buchstaben <c>
*  und (optionalem) Text <text>.
* Wenn es sich um eine Datei handelt, die aufs Desktop gezogen
*  wurde, wird <path> zugeordnet.
* RÅckgabe != 0, falls nicht erfolgreich.
*
****************************************************************/

int make_icon(int type, char c, char *icntext, char *path, int x, int y)
{
	register int i;
	register OBJECT *ob;


	for	(i = 0; i < n_icons && icon[i].icon_type; i++)
		;

	if (i >= n_icons)
	{
#if LANGUAGE == LANG_GER
		form_alert(1, "[3][Desktop ist voll!][Abbruch]");
#elif LANGUAGE == LANG_ENG
		form_alert(1, "[3][Desktop full!][Cancel]");
#endif
		return TRUE;
	}

	if (path)
	{
		if (strlen(path) > (80-15))
		{
			err_alert(EPTHOV);
			return TRUE;
		}
		strcpy(icon[i].text, path);
	}
	else
	{
		icon[i].text[0] = EOS;
	}

	ob = windows[0].pobj+i+1;
	icon[i].icon_type = type;
	icon[i].isdisk  = c;

	if (icntext)
	{
		strcat(icon[i].text, icntext);
		icntext = get_name(icon[i].text);
	}

	init_icnobj(ob, &icon[i].data, type, icntext);
	if (c)
	{
		icon[i].data.ib_char = (c & 0x00ff) | 0x1000;
	}

	ob->ob_x = x - windows[0].pobj->ob_x;
	ob->ob_y = y - windows[0].pobj->ob_y;
	draw_wind_obj(0, i + 1);
	return FALSE;
}


/****************************************************************
*
* Verschieben von selektierten Window- Icons vom Fenster <wnr>
*  um <x>,<y> auf das Desktop.
*
****************************************************************/

static int mk_obj(int wnr, int obj)
{
	int type;
	MYDTA *file;
	register OBJECT *tree = windows[wnr].pobj + obj;


	file = windows[wnr].pmydta+obj-1;
	type = file->icon_type;
	if (make_icon(type, 0, file->filename, windows[wnr].path,
			    tree->ob_x + mvx, tree->ob_y + mvy))
		return -1;
	return 0;
}


static void mk_dskt_icns(int wnr, int xrel, int yrel)
{
	mvx = windows[wnr].pobj->ob_x + xrel;
	mvy = windows[wnr].pobj->ob_y + yrel;
	walk_wind_sel_objs(wnr, mk_obj);
}


/****************************************************************
*
* Verschieben von selektierten Objekten.
* <wnr>,<tree>		: Nummer und Baum der selektierten Objekte
* <x_koor>,<y_koor>	: Anklickpunkt in absoluten Koordinaten
*
****************************************************************/

static void move_icons(int x_koor, int y_koor, int wnr, OBJECT *tree, int kbsh)
{
	GRECT g;
	GRECT *ptr_g;
	OBJECT **mov_boxes;
	int num_boxes;
	EVNTDATA ev;
	int old_x,old_y;
	register int i;
	OBJECT *dest_tree;
	int dest_wnr, dest_obj;
	int old_wnr = -2, old_obj = 0;
	OBJECT *old_tree = NULL;


	/* Speicher holen */
	/* -------------- */

	mov_boxes = (OBJECT **) Malloc(NUM_FILES_TOTAL * sizeof(void *));
	if (mov_boxes == NULL)
	{
		err_alert(ENSMEM);
		return;
	}

	/* Zeiger auf alle zu verschiebenden Icons */
	/* --------------------------------------- */

	if ((i = (tree->ob_head)) > 0)
	{
		for	(num_boxes = 0; i <= (tree->ob_tail); i++)
			if (selected(tree, i) && !(HIDETREE&(tree + i)->ob_flags))
				mov_boxes[num_boxes++] = tree+i;
	}

	if (wnr == 0 || num_boxes == 1)
	{
		tree_sel_grect(tree, &g);
		ptr_g = &g;
		if ((num_boxes == 1) && wnr)
		{
			if (g.g_w > 12 * gl_hwchar)
				g.g_w = 12 * gl_hwchar;
		}
	}
	else
		ptr_g = NULL;

	Mgraf_mouse(M_OFF);
	Mgraf_mouse(FLAT_HAND);

	ev.x = x_koor;
	ev.y = y_koor;
	do
	{
		draw_boxes(tree, mov_boxes, num_boxes, ptr_g, ev.x - x_koor, ev.y - y_koor);
		Mgraf_mouse(M_ON);

		old_x = ev.x;
		old_y = ev.y;

		do
		{
			graf_mkstate(&ev);
		}
		while ((ev.x == old_x) && (ev.y == old_y) && (ev.bstate & 1));

		Mgraf_mouse(M_OFF);
		draw_boxes(tree, mov_boxes, num_boxes, ptr_g,
				 old_x - x_koor, old_y - y_koor);

		dest_wnr = hdl_to_fnr(wind_find(ev.x, ev.y));
		if (dest_wnr >= 0)
		{
			dest_tree = windows[dest_wnr].pobj;
			if (in_grect(ev.x, ev.y, &(windows[dest_wnr].in)))
			{
				if (0 < (dest_obj = find_obj(dest_tree, ev.x, ev.y)))
				{
					if (dest_wnr == wnr && selected(dest_tree, dest_obj))
						dest_obj = -2;
					else
					if (!is_dest(dest_wnr, dest_obj))
						dest_obj = 0;
				}
			}
			else
				dest_obj = 0;
		}
		else
		{
			dest_tree = NULL;
			dest_obj = -1;
		}

		if ((dest_tree != old_tree) || (dest_obj != old_obj))
		{
			if (old_tree && (old_obj > 0))
			{
				draw_wind_obj(old_wnr, old_obj);
			}
			if (dest_tree && dest_obj > 0)
			{
				ob_sel(dest_tree, dest_obj);
				draw_wind_obj(dest_wnr, dest_obj);
				ob_dsel(dest_tree, dest_obj);
			}
			old_tree = dest_tree;
			old_obj  = dest_obj;
			old_wnr  = dest_wnr;
		}
	}
	while (ev.bstate & 1);

	Mfree(mov_boxes);

	if (dest_obj > 0)
	{
		draw_wind_obj(dest_wnr, dest_obj);
	}

	Mgraf_mouse(ARROW);
	Mgraf_mouse(M_ON);

	/* Die zu verschiebenden Icons lagen im Desktop, Zielfenster */
	/* ist ebenfalls das Desktop, Zielobjekt ist ungÅltig oder   */
	/* der Desktop- Hintergrund :							 */
	/*	ICONS AUF DEM DESKTOP VERSCHIEBEN					 */
	/* --------------------------------------------------------- */

	if ((wnr == 0) && (dest_wnr == 0) && (dest_obj == 0 || dest_obj == -2) )
	{
		mv_dskt_icns(xrel, yrel);
		return;
	}

	/* Zielobjekt ist ungÅltig (etwa fremdes Fenster oder mit    */
	/* einem der Quellobjekte identisch) :					 */
	/*	NICHTS TUN									 */
	/* --------------------------------------------------------- */

	if (dest_obj < 0)
	{
		if (dest_wnr == 0)
			Bconout(2,7);		/* pling */
		return;
	}

	/* Objekte sind von einem Fenster aufs Desktop gezogen 	 */
	/* worden, und zwar nicht in ein gÅltiges Zielobjekt,		 */
	/* sondern auf eine freie FlÑche :						 */
	/*	FALLS 1 OBJEKT, DESKTOP- ICON ERSTELLEN				 */
	/* --------------------------------------------------------- */

	if ((wnr > 0) && (dest_wnr == 0) && (dest_obj == 0))
	{
		if (ptr_g)
			mk_dskt_icns(wnr, xrel, yrel);
		else
#if LANGUAGE == LANG_GER
			form_alert(1, "[3][Immer nur EINE Datei|aufs Desktop ziehen!][Abbruch]");
#elif LANGUAGE == LANG_ENG
			form_alert(1, "[3][Only ONE file can be placed on|desktop at one time!][Abort]");
#endif
		return;
	}

	/* Objekte sind von irgendwoher in ein gÅltiges Objekt auf 	 */
	/* dem Desktop gezogen worden:						 */
	/*	PAPIERKORB	: LôSCHEN							 */
	/*	DRUCKER   	: DRUCKEN							 */
	/* --------------------------------------------------------- */

	if ((dest_wnr == 0) && (dest_obj > 0))
	{
		if (icon[dest_obj - 1].icon_type == I_PAPIER)
		{
			dl_icns(wnr);
			return;
		}
		if (icon[dest_obj - 1].icon_type == I_DRUCKR)
		{
			prt_icns(wnr, TRUE);
			return;
		}
	}

	/*   KOPIEREN / VERSCHIEBEN							 */
	/* --------------------------------------------------------- */

	{
		char dest_path[130];


		if (1 != obj_to_path(dest_wnr, dest_obj, dest_path, NULL))
		{
			dclick(dest_wnr, dest_obj, kbsh &= ~K_CTRL);
			return;
		}

		cache_on(dest_path);					/* fÅr Zieldisk  */
		if ((wnr > 0) && (num_boxes > 1))
			cache_on(windows[wnr].path);		/* fÅr Quelldisk */

		cpmv_icns(wnr, dest_path, kbsh);

		if ((wnr > 0) && (num_boxes > 1))
			cache_off(windows[wnr].path);		/* fÅr Quelldisk */

		cache_off(dest_path);				/* fÅr Zieldisk  */
	}
}


/****************************************************************
*
* Nach der Bearbeitung eines Einfach-Mausklicks mÅssen wir
* prÅfen, ob die Maustaste noch gedrÅckt ist. Insbesondere auf
* schnellen Systemen mÅssen wir dem Anwender etwas Zeit lassen,
* die linke Maustaste wieder loszulassen, ansonsten wird immer
* ein "Drag" erkannt.
*
* RÅckgabe TRUE bei "timeout", d.h. Maustaste noch gedrÅckt.
*
****************************************************************/

static int detect_mouse_drag(void)
{
	EVNT w_ev;

#if 0
	long last_clicktime = getHz200();
	long clicktime;
#endif
	w_ev.mwhich = evnt_multi(
                      MU_BUTTON + MU_TIMER,
					  1,			/* keine Doppelklicks */
					  1,			/* nur linke Maustaste */
					  0,			/* linke Maustaste losgelassen */
					  0,NULL,		/* kein 1. Rechteck */
					  0,NULL,		/* kein 2. Rechteck */
					  NULL,			/* keine "message" */
					  MOUSE_DRAG_DETECT_MS,		/* ms */
					  (EVNTDATA *) &(w_ev.mx),
					  &w_ev.key,
					  &w_ev.mclicks
					  );

#if 0
	clicktime = getHz200();
	printf("\x1b" "H %lu->%lu (%lu ms) mwhich=0x%04x   \n",
			last_clicktime, clicktime,
			(clicktime - last_clicktime) * 5,
			w_ev.mwhich);
#endif
	return (w_ev.mwhich & MU_TIMER) != 0;
}


/****************************************************************
*
* Der Benutzer hat einen Mausklick ausgefÅhrt.
*
****************************************************************/

void handle_mouse_button(int nclicks, const EVNTDATA *ev)
{
	int windhdl,icnobj,wnr;
	int issel, dummy, dummy2;
	OBJECT *tree;


	/* Feststellen, ob in einem Fenster Icons angewÑhlt sind */
	/* ----------------------------------------------------- */

	issel = icsel(&dummy, &dummy2);

	/* windhdl := Handle des angeklickten Fensters oder <= 0 */
	/* icnobj := Objektnummer des Icons in diesem Fenster bzw. Desk. */
	/* tree   := Objektbaum des Fensters bzw. des Desktop */
	/* -------------------------------------------------- */

	if (0 < (windhdl = wind_find(ev->x, ev->y)))
	{
		wnr = hdl_to_fnr(windhdl);
		if (wnr >= 0 && !in_grect(ev->x, ev->y, &(windows[wnr].in)))
			wnr = -1;
	}
	else
		wnr = 0;

	if (wnr >= 0)
	{
		tree = windows[wnr].pobj;
		icnobj = find_obj(tree, ev->x, ev->y);
	}
	else
		return;

	/* 1. Fall: Kein Objekt wurde angeklickt. Nichts passiert. */
	/* ------------------------------------------------------- */

	if (icnobj < 0)
		return;

	/* 2. Fall: Ein Hintergrund wurde angeklickt. */
	/*          Alle Icons ggf. deselektieren */
	/*          Falls noch Maustaste gedrÅckt, Box ziehen */
	/* -------------------------------------------------- */

	if (icnobj == 0)
	{
		if (issel & !(ev->kstate & (K_RSHIFT + K_LSHIFT)))
		{
			dsel_all();
		}

		if (detect_mouse_drag())
		{
			GRECT rub;

			rub.g_x = ev->x;
			rub.g_y = ev->y;
			Mgraf_mouse(POINT_HAND);
			graf_rubbox(ev->x, ev->y, -32767, -32767, &rub.g_w, &rub.g_h);
			if (rub.g_w < 0)
			{
				rub.g_x += rub.g_w;
				rub.g_w = -rub.g_w;
			}
			if (rub.g_h < 0)
			{
				rub.g_y += rub.g_h;
				rub.g_h = -rub.g_h;
			}
			Mgraf_mouse(ARROW);
			grect_toggle(tree, &rub);
			redraw(wnr, &rub);
			dirty_info(wnr);
		}
		return;
	}

	/* 3. Fall: selektiertes Objekt wurde einmal angeklickt	*/
	/* ---------------------------------------------------- */

	if ((nclicks < 2) && selected(tree, icnobj))
	{
		if (ev->kstate & (K_RSHIFT + K_LSHIFT))
		{
			ob_dsel(tree, icnobj);
			draw_wind_obj(wnr, icnobj);
			dirty_info(wnr);
			return;
		}

		goto tst_move;
	}

	/* 4. Fall: nichtselektiertes Objekt wurde angeklickt		*/
	/*          oder irgendein Objekt wurde doppelgeklickt		*/
	/*          Bei nicht SHIFT alle anderen Objekte deselekt.	*/
	/* -------------------------------------------------------- */

	if (!(ev->kstate & (K_RSHIFT + K_LSHIFT)))
	{
		dsel_all();
		ob_sel(tree, icnobj);
	}
	else
	{
		if (nclicks != 2)
			tree[icnobj].ob_state ^= SELECTED;
		else
			tree[icnobj].ob_state |= SELECTED;
	}

	draw_wind_obj(wnr, icnobj);
	dirty_info(wnr);

	if (nclicks < 2)
	{
		tst_move:
		if (detect_mouse_drag())
		{
			move_icons(ev->x, ev->y, wnr, tree, ev->kstate);
		}
		return;
	}

	/* Ab hier nur noch Doppelklicks berÅcksichtigen */
	/* --------------------------------------------- */

	if (nclicks == 2)
	{
		if (dclick(wnr, icnobj, ev->kstate))
		{
			ob_dsel(tree, icnobj);
			draw_wind_obj(wnr, icnobj);
			dirty_info(wnr);
		}
	}
}


/****************************************************************
*
* Alle Objekte in Fenstern und auf dem Desktop werden
* deselektiert und ggf. neu gezeichnet.
*
****************************************************************/

void dsel_all(void)
{
	OBJECT *tree;
	GRECT g;
	int   wnr,dummy;
	register int i;


	while (icsel(&wnr, &dummy))
	{
		tree = windows[wnr].pobj;
		tree_sel_grect(tree, &g);
		if ((i = (tree->ob_head)) > 0)
			for	(; i <= (tree->ob_tail); i++)
				if (selected(tree, i))
					ob_dsel(tree, i);
		redraw(wnr, &g);
		if (g.g_w > 0)
			dirty_info(wnr);
	}
}


/****************************************************************
*
* Alle Objekte in einem Fenster oder auf dem Desktop werden
* selektiert und ggf. neu gezeichnet.
*
****************************************************************/

void sel_all(int wnr)
{
	OBJECT *tree = windows[wnr].pobj;
	GRECT g;
	int   ownr,dummy;
	register int i,head;


	if (wnr < 0)
		return;
	if (icsel(&ownr, &dummy) && ownr != wnr)
		dsel_all();
	if ((head = (tree->ob_head)) <= 0)
		return;
	for	(i = head; i <= (tree->ob_tail); i++)
	{
		if (0 == ((tree+i)->ob_flags & HIDETREE))
			(tree+i)->ob_state ^= SELECTED;
	}
	tree_sel_grect(tree, &g);
	for	(i = head; i <= (tree->ob_tail); i++)
	{
		if (0 == ((tree+i)->ob_flags & HIDETREE))
			(tree+i)->ob_state |= SELECTED;
	}
	redraw(wnr, &g);
	if (g.g_w > 0)
		dirty_info(wnr);
}


/****************************************************************
*
* Das Objekt mit der Objektnummer <index> wird neu gemalt.
* (da es sich entweder verschoben hat oder der Status sich
*  geÑndert hat).
* Das Objekt liegt im Fenster mit Nummer <wnr> bzw. auf dem
* Desktop, wenn <wnr> = 0;
*
****************************************************************/

void draw_wind_obj(int wnr, int index)
{
	int whdl;
	GRECT list, neu;
	OBJECT *tree;


	if (index > windows[wnr].shownum)
		return;

	whdl = windows[wnr].handle;
	tree = windows[wnr].pobj;
	objc_offset(tree, index, &(neu.g_x), &(neu.g_y));
	neu.g_w = (tree+index)->ob_width;
	neu.g_h = (tree+index)->ob_height;

	if (((tree + index)->ob_type == G_USERDEF) || ((tree + index)->ob_state & WHITEBAK))
	{
		index = 0;
	}

	wind_get_grect(whdl, WF_FIRSTXYWH, &list);
	do
	{
		if (rc_intersect(&neu,&list))
		{
			objc_draw(tree, index, 1, &list);
		}
		wind_get_grect(whdl, WF_NEXTXYWH, &list);
	}
	while (list.g_w > 0);
}


/****************************************************************
*
* Nimmt ein Icon vom Desktop.
*
****************************************************************/

void kill_icon(int objnr)
{
	OBJECT *ob = windows[0].pobj + objnr;
	ICON   *ic = icon + objnr - 1;
	GRECT  g;


	ic->icon_type = ic->isdisk = 0;
	ob->ob_flags = HIDETREE;
	objc_grect(windows[0].pobj, objnr, &g);
	redraw(0, &g);
}


/****************************************************************
*
* Gibt zu einem Objekt den Pfad und ggf. die DTA zurÅck.
* RÅckgabe  0, falls Datei
*           1, falls Pfad (Disk oder Ordner oder Fenster)
*		 -1, sonst
*
****************************************************************/

int obj_to_path(int wnr, int obj, char *path, MYDTA **f)
{
	ICON  *icn;
	MYDTA *file;
	int   ispath = 0;


	if (f)
		*f = NULL;				/* per Default keine DTA */
	*path = EOS;
	if (wnr == 0)
	{
		if (obj == 0)			/* Desktop- Hintergrund */
			return -1;
		icn = icon + (obj - 1);

		if (icn->icon_type >= I_ORDNER) /* Ord,Prog,Dat,Bat */
		{
			strcpy(path, icn->text);
			if (icn->icon_type == I_ORDNER)
				ispath = 1;
		}
		else
		{
			if (!icn->isdisk)
				return -1;
			path[0] = icn->isdisk;
			path[1] = ':';
			path[2] = EOS;
			ispath = 1;
		}
	}
	else
	{
		strcpy(path, windows[wnr].path);
		if (obj)
		{
			file = windows[wnr].pmydta+obj-1;
			if (f)
				*f = file;
			strcat(path, file-> filename);
			if ((file->attrib) & F_SUBDIR)
				ispath = 1;
		}
		else
			ispath = 1;
	}

	if (ispath && obj)
		strcat(path, "\\");

	return ispath;
}


/****************************************************************
*
* Alle selektierten Icons von <wnr> sind in den Papierkorb
* geschoben worden.
*
****************************************************************/

static int dli_obj(int wnr, int obj)
{
	if (wnr == 0 && icon[obj - 1].icon_type >= I_ORDNER)
		kill_icon(obj);
	return 0;
}

static int dl_obj(int wnr, int obj)
{
	int ret;
	char path[128];

	if (qbreak())
		return (int) EBREAK;

	if (0 > (ret = obj_to_path(wnr, obj, path, NULL)))
		return 0;

	return delete_files(path, ret);
}

void dl_icns(int wnr)
{
	if (wnr < 0)
		return;
	if (wnr > 0)
		cache_on (windows[wnr].path);

	walk_wind_sel_objs(wnr, dli_obj);
	if (!confirm(wnr, 0, settings.cnfm_del, windows[wnr].path))
	{
		Mgraf_mouse(HOURGLASS);
		walk_wind_sel_objs(wnr, dl_obj);
		Mgraf_mouse(ARROW);
	}

	end_confirm();
	if (wnr > 0)
	{
		cache_off(windows[wnr].path);
	}
}


/****************************************************************
*
* Alle selektierten Icons von <wnr> sind in den Drucker
* geschoben worden.
* Bei <print> == 0 wird angezeigt
* Bei <print> == 1 wird ausgedruckt.
*
****************************************************************/

static int _prt;

static int prt_obj(int wnr, int obj)
{
	char path[128];

	if (qbreak())
		return (int) EBREAK;

	if (0 != obj_to_path(wnr, obj, path, NULL))	/* nur Dateien */
		return 0;

	return (_prt) ? print_file(path) : show_file(path);
}

void prt_icns(int wnr, int print)
{
	char *pgm;
	int type;


	if (wnr < 0)
		return;

#if LANGUAGE == LANG_GER
	if (print && (2 == form_alert(1, "[2][Druckprogramm aufrufen?][OK|Abbruch]")))
#elif LANGUAGE == LANG_ENG
	if (print && (2 == form_alert(1, "[2][Execute print program?][OK|Cancel]")))
#endif
		return;

	pgm = tree_programs[PG_2 + print].ob_spec.tedinfo->te_ptext;
	if (*pgm)		/* Druck- bzw. Anzeigeprogramm angegeben */
	{
		type = suffix_type(pgm);
		if (type == 'PP')		/* .TTP -> .TOS */
			type = '_P';
		start_program(pgm, type, 0, 0, 0);
		return;
	}

	_prt = print;
	walk_wind_sel_objs(wnr, prt_obj);
}


/****************************************************************
*
* Alle selektierten Icons von <wnr> sind in einen Zielpfad
* geschoben worden.
*
****************************************************************/

static char *_dest_path;
static int _mflag;

static int cpmv_obj(int wnr, int obj)
{
	int  ret;
	char path[128];


	if (qbreak())
		return (int) EBREAK;

	if (0 > (ret = obj_to_path(wnr, obj, path, NULL)))
		return 0;

	return copy_move_files(path, _dest_path, _mflag, ret);
}

void cpmv_icns(int wnr, char *dest_path, int kbsh)
{
	_dest_path = dest_path;
	_mflag = (kbsh & K_CTRL) ? 1 : 0;
	if (!confirm(wnr, 1 + _mflag, settings.cnfm_copy, dest_path))
	{
		Mgraf_mouse(HOURGLASS);
		walk_wind_sel_objs(wnr, cpmv_obj);
		Mgraf_mouse(ARROW);
	}
	end_confirm();
}