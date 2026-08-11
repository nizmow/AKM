/*********************************************************************
*
* Dieses Modul enthÑlt die Bearbeitung aller Mausklicks, also 
* Verschieben und Aktivierung von Icons, Doppelklicks in Fenster usw.
*
*********************************************************************/

#include <vdi.h>
#include <tos.h>
#include "k.h"
#include <string.h>
#include <tosdefs.h>


void obj_malen(int wnr, int index);
void dsel_all( void );


extern OBJECT *adr_icons;
extern int scrx,scry,scrw,scrh,vdi_handle;

extern long    err_alert		(long e);
extern void 	redraw		(int wnr, GRECT *neu);
extern void	ob_sel		(OBJECT *tree, int which);
extern void	ob_dsel		(OBJECT *tree, int which);
extern int	selected		(OBJECT *tree, int which);
extern int 	icsel		(int *rwnr, int *objn);
extern int	rc_intersect	(GRECT *p1, GRECT *p2);
extern void 	objc_grect	(OBJECT *tree, int objn, GRECT *g);
extern void 	tree_sel_grect	(OBJECT *tree, GRECT *g);
extern void 	grect_toggle	(OBJECT *tree, GRECT *g);
extern int 	walk_sel		(int wnr, int (*tue)(int wnr, int obj));
extern int 	in_grect		(int x, int y, GRECT *g);
extern int 	find_obj		(OBJECT *tree, int x, int y);
extern int 	hdl_to_fnr	(int whdl);

/* Extern aus WINDOWS */

extern void upd_drives( void );
extern void dirty_info( int wnr );

/* Extern aus DCLICK */

extern int  dclick(int wnr, int obj, int kbsh);

/* Extern aus FILES */

extern int  qbreak( void );
extern long confirm(int wnr, int function, int ja, char *zielpfad);
extern void end_confirm( void );


/* Extern aus ALLG */

extern void Mgraf_mouse(int type);

/****************************************************************
*
* Der Benutzer hat an der Bildschirmposition (x_koor,y_koor) mit
* dem <knopf> einen <anzahl>- fachen Mausklick ausgefÅhrt.
*
****************************************************************/

void mausknopf(int anzahl, int x_koor, int y_koor, int knopf, int kbsh)
{
	int windhdl,icnobj,wnr;
	int issel,dummy,dummy2;
	int x,y;
	OBJECT *tree;
	void move_icons(int x_koor, int y_koor, int wnr, OBJECT *tree,
				 int kbsh);


	/* Feststellen, ob in einem Fenster Icons angewÑhlt sind		*/
	/* ------------------------------------------------------------- */

	issel = icsel(&dummy, &dummy2);

	/* windhdl := Handle des angeklickten Fensters oder <= 0		*/
	/* icnobj := Objektnummer des Icons in diesem Fenster bzw. Desk.	*/
	/* tree   := Objektbaum des Fensters bzw. des Desktop			*/
	/* -------------------------------------------------------------	*/

	if	(0 < (windhdl = wind_find(x_koor,y_koor)))
		{
		wnr = hdl_to_fnr(windhdl);
		if	(wnr >= 0 && !in_grect(x_koor, y_koor, &(fenster[wnr].in)))
			wnr = -1;
		}
	else wnr = 0;

	if	(wnr >= 0)
		{
		tree = fenster[wnr].pobj;
		icnobj = find_obj(tree,x_koor,y_koor);
		}
	else return;

	/* 1. Fall: Kein Objekt wurde angeklickt. Nichts passiert. */
	/* ------------------------------------------------------- */

	if	(icnobj < 0)
		return;

	/* 2. Fall: Ein Hintergrund wurde angeklickt.			*/
	/*		  Alle Icons ggf. deselektieren				*/
	/*          Falls noch Maustaste gedrÅckt, Box ziehen		*/
	/* -------------------------------------------------------- */

	if	(icnobj == 0)
		{
		if	(issel & !(kbsh & (K_RSHIFT + K_LSHIFT)))
			dsel_all();
		graf_mkstate(&x, &y, &knopf, &dummy);
		if	(knopf & 1)
			{
			GRECT rub;

			rub.g_x = x_koor;
			rub.g_y = y_koor;
			Mgraf_mouse(POINT_HAND);
			graf_rubberbox(x_koor, y_koor, -32767, -32767, &rub.g_w, &rub.g_h);
			if	(rub.g_w < 0)
				{
				rub.g_x += rub.g_w;
				rub.g_w = -rub.g_w;
				}
			if	(rub.g_h < 0)
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
	/* -------------------------------------------------------- */

	if	(anzahl < 2 && selected(tree, icnobj))
		{
		if	(kbsh & (K_RSHIFT + K_LSHIFT))
			{
			ob_dsel(tree, icnobj);
			obj_malen(wnr, icnobj);
			dirty_info(wnr);
			return;
			}
		goto tst_move;
		}

	/* 4. Fall: nichtselektiertes Objekt wurde angeklickt		*/
	/*          oder irgendein Objekt wurde doppelgeklickt		*/
	/*          Bei nicht SHIFT alle anderen Objekte deselekt.	*/
	/* -------------------------------------------------------- */

	if	(!(kbsh & (K_RSHIFT + K_LSHIFT)))
		{
		dsel_all();
		ob_sel(tree, icnobj);
		}
	else {
		if	(anzahl != 2)
			(tree + icnobj) -> ob_state ^= SELECTED;
		else (tree + icnobj) -> ob_state |= SELECTED;
		}
	obj_malen(wnr, icnobj);
	dirty_info(wnr);

	if	(anzahl < 2)
		{
		tst_move:
		graf_mkstate(&x, &y, &knopf, &dummy);
		if	(knopf & 1)
			move_icons(x_koor, y_koor, wnr, tree, kbsh);
		return;
		}

	/* Ab hier nur noch Doppelklicks berÅcksichtigen */
	/* --------------------------------------------- */

	if	(anzahl == 2)
		{
		if	(dclick(wnr, icnobj, kbsh))
			{
			ob_dsel(tree, icnobj);
			obj_malen(wnr, icnobj);
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

void dsel_all( void )
{
	OBJECT *tree;
	GRECT g;
	int   wnr,dummy;
	register int i;


	while(icsel(&wnr, &dummy))
		{
		tree = fenster[wnr].pobj;
		tree_sel_grect(tree, &g);
		if	((i = (tree -> ob_head)) > 0)
			for	(; i <= (tree -> ob_tail); i++)
				if	(selected(tree, i))
					ob_dsel(tree, i);
		redraw(wnr, &g);
		if	(g.g_w > 0)
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
	OBJECT *tree = fenster[wnr].pobj;
	GRECT g;
	int   ownr,dummy;
	register int i,head;


	if	(wnr < 0)
		return;
	if	(icsel(&ownr, &dummy) && ownr != wnr)
		dsel_all();
	if	((head = (tree -> ob_head)) <= 0)
		return;
	for	(i = head; i <= (tree -> ob_tail); i++)
		{
		if	(0 == ((tree+i)->ob_flags & HIDETREE))
			(tree+i)->ob_state ^= SELECTED;
		}
	tree_sel_grect(tree, &g);
	for	(i = head; i <= (tree -> ob_tail); i++)
		{
		if	(0 == ((tree+i)->ob_flags & HIDETREE))
			(tree+i)->ob_state |= SELECTED;
		}
	redraw(wnr, &g);
	if	(g.g_w > 0)
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

void obj_malen(int wnr, int index)
{
	int		whdl;
	GRECT 	list,neu;
	OBJECT 	*tree;


	if	(index > fenster[wnr].shownum)
		return;
	whdl = fenster[wnr].handle;
	tree = fenster[wnr].pobj;
	objc_offset(tree, index, &(neu.g_x), &(neu.g_y));
	neu.g_w = (tree+index)->ob_width;
	neu.g_h = (tree+index)->ob_height;

	if	(((tree + index) -> ob_type == G_USERDEF) ||
		 ((tree + index) -> ob_state & WHITEBAK)
		)
		index = 0;
	wind_get(whdl,WF_FIRSTXYWH,&list.g_x,&list.g_y,&list.g_w,&list.g_h);
	do	{
		if	(rc_intersect(&neu,&list))
			objc_draw(tree,index,1,list.g_x,list.g_y,list.g_w,list.g_h);
		wind_get(whdl,WF_NEXTXYWH,&list.g_x,&list.g_y,&list.g_w,&list.g_h);
		}
	while(list.g_w > 0);
}


/****************************************************************
*
* Bringt ein neues Icon ins Desktop. Es soll ein Icon mit Typ
*  <typ>, absoluter Position <x,y>, (optionalem) Buchstaben <c>
*  und (optionalem) Text <text>.
* Wenn es sich um eine Datei handelt, die aufs Desktop gezogen
*  wurde, wird <path> zugeordnet.
* RÅckgabe != 0, falls nicht erfolgreich.
*
****************************************************************/

int make_icon(int typ, char c, char *icntext, char *path, int x, int y)
{
	register int i;
	register OBJECT *ob;
	extern char *get_name(char *path);
	extern void init_icnobj(OBJECT *o, ICONBLK *icn, int typ, char *text);


	for	(i = 0; i < n_icons && icon[i].icontyp; i++)
		;
	if	(i >= n_icons)
		{
#if		COUNTRY==FRG
		form_alert(1, "[3][Desktop ist voll!][Abbruch]");
#elif	COUNTRY==USA
		form_alert(1, "[3][Desktop full!][Cancel]");
#endif
		return(TRUE);
		}
	if	(path)
		{
		if	(strlen(path) > (80-15))
			{
			err_alert(EPTHOV);
			return(TRUE);
			}
		strcpy(icon[i].text, path);
		}
	else icon[i].text[0] = EOS;

	ob = fenster[0].pobj+i+1;
	icon[i].icontyp = typ;
	icon[i].isdisk  = c;
	if	(icntext)
		{
		strcat(icon[i].text, icntext);
		icntext = get_name(icon[i].text);
		}
	init_icnobj(ob, &icon[i].data, typ, icntext);
	if	(c)
		icon[i].data.ib_char = (c & 0x00ff) | 0x1000;
	ob->ob_x	    = x - fenster[0].pobj->ob_x;
	ob->ob_y	    = y - fenster[0].pobj->ob_y;
	obj_malen(0, i+1);
	return(FALSE);
}


/****************************************************************
*
* Nimmt ein Icon vom Desktop.
*
****************************************************************/

void kill_icon(int objnr)
{
	OBJECT *ob = fenster[0].pobj+objnr;
	ICON   *ic = icon+objnr-1;
	GRECT  g;


	ic -> icontyp = ic -> isdisk = 0;
	ob -> ob_flags = HIDETREE;
	objc_grect(fenster[0].pobj, objnr, &g);
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


	if	(f)
		*f = NULL;				/* per Default keine DTA */
	*path = EOS;
	if	(wnr == 0)
		{
		if	(obj == 0)			/* Desktop- Hintergrund */
			return(-1);
		icn = icon + (obj - 1);
		if	(icn->icontyp >= I_ORDNER) /* Ord,Prog,Dat,Bat */
			{
			strcpy(path, icn->text);
			if	(icn->icontyp == I_ORDNER)
				ispath = 1;
			}
		else {
			if	(!icn->isdisk)
				return(-1);
			path[0] = icn->isdisk;
			path[1] = ':';
			path[2] = EOS;
			ispath = 1;
			}
		}
	else {
		strcpy(path, fenster[wnr].path);
		if	(obj)
			{
			file = fenster[wnr].pmydta+obj-1;
			if	(f)
				*f = file;
			strcat(path, file-> filename);
			if	((file -> attrib) & F_SUBDIR)
				ispath = 1;
			}
		else ispath = 1;
		}

	if	(ispath && obj)
		strcat(path, "\\");
	return(ispath);
}


/****************************************************************
*
* Malt alle Objekte in der Liste <mov_boxes[anz_boxes]>.
* Wird als Begrenzung <grenz> != NULL angegeben, wird verhindert,
*  daû die Objekte auûerhalb des Bildschirms liegen.
*
****************************************************************/

int xrel, yrel;

void draw_boxes(OBJECT *tree, OBJECT *mov_boxes[], int anz_boxes,
			 GRECT *grenz, int xoff, int yoff)
{
	int px, py;
	int pxy[10];
	extern void set_deflt_clip( void );


	vswr_mode	(vdi_handle, MD_XOR);
	set_deflt_clip();
	vsl_type	(vdi_handle, DASH);
	if	(grenz)
		{
		if	(grenz->g_x + xoff < scrx)
			xoff = scrx - grenz->g_x;
		if	(grenz->g_y + yoff < scry)
			yoff = scry - grenz->g_y;
		if	(grenz->g_x + grenz->g_w + xoff > scrx + scrw)
			xoff = scrx + scrw - (grenz->g_x + grenz->g_w);
		if	(grenz->g_y + grenz->g_h + yoff > scry + scrh)
			yoff = scry + scrh - (grenz->g_y + grenz->g_h);
		}
	px = tree->ob_x + xoff;
	py = tree->ob_y + yoff;
	xrel = xoff;
	yrel = yoff;

	for	(; anz_boxes > 0; anz_boxes--,mov_boxes++)
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
* Verschieben von selektierten Objekten.
* <wnr>,<tree>		: Nummer und Baum der selektierten Objekte
* <x_koor>,<y_koor>	: Anklickpunkt in absoluten Koordinaten
*
****************************************************************/

void move_icons(int x_koor, int y_koor, int wnr, OBJECT *tree, int kbsh)
{
	GRECT g;
	GRECT *ptr_g;
	OBJECT **mov_boxes;
	int anz_boxes;
	int mouse_x, mouse_y, mstat, dummy;
	int old_x,old_y;
	register int i;
	OBJECT *zieltree;
	int zielwnr    , zielobj;
	int altwnr = -2, altobj = 0;
	OBJECT *alttree = NULL;
	extern int is_dest(int wnr, int objnr);
	void	mv_dskt_icns(int x, int y);
	void mk_dskt_icns(int wnr, int xrel, int yrel);
	void dl_icns	(int wnr);
	void prt_icns	(int wnr, int print);
	void cpmv_icns	(int wnr, char *zielpfad, int kbsh);


	/* Speicher holen */
	/* -------------- */

	mov_boxes = (OBJECT **) Malloc(ANZDATEIEN * sizeof(void *));
	if	(mov_boxes == NULL)
		{
		err_alert(ENSMEM);
		return;
		}

	/* Zeiger auf alle zu verschiebenden Icons */
	/* --------------------------------------- */

	if	((i = (tree -> ob_head)) > 0)
		{
		for	(anz_boxes = 0; i <= (tree -> ob_tail); i++)
			if	(selected(tree, i) && !(HIDETREE&(tree+i)->ob_flags))
				mov_boxes[anz_boxes++] = tree+i;
		}

	if	(wnr == 0 || anz_boxes == 1)
		{
		extern int gl_hwchar;

		tree_sel_grect	(tree, &g);
		ptr_g = &g;
		if	((anz_boxes == 1) && wnr)
			{
			if	(g.g_w > 12*gl_hwchar)
				g.g_w = 12*gl_hwchar;
			}
		}
	else ptr_g = NULL;

	Mgraf_mouse(M_OFF);
	Mgraf_mouse(FLAT_HAND);

	mouse_x = x_koor;
	mouse_y = y_koor;
	do	{
		draw_boxes(tree, mov_boxes, anz_boxes, ptr_g,
				 mouse_x-x_koor, mouse_y-y_koor);
		Mgraf_mouse(M_ON);

		old_x = mouse_x;
		old_y = mouse_y;
		do	{
			graf_mkstate(&mouse_x, &mouse_y, &mstat, &dummy);
			}
		while(mouse_x == old_x && mouse_y == old_y && (mstat & 1));
		Mgraf_mouse(M_OFF);
		draw_boxes(tree, mov_boxes, anz_boxes, ptr_g,
				 old_x-x_koor, old_y-y_koor);

		zielwnr = hdl_to_fnr(wind_find(mouse_x, mouse_y));
		if	(zielwnr >= 0)
			{
			zieltree = fenster[zielwnr].pobj;
			if	(in_grect(mouse_x, mouse_y, &(fenster[zielwnr].in)))
				{
				if	(0 < (zielobj = find_obj(zieltree, mouse_x, mouse_y)))
					{
					if	(zielwnr == wnr && selected(zieltree, zielobj))
						zielobj = -2;
					else if	(!is_dest(zielwnr, zielobj))
							zielobj = 0;
					}
				}
			else zielobj = 0;
			}
		else {
			zieltree = NULL;
			zielobj = -1;
			}

		if	(zieltree != alttree || zielobj != altobj)
			{
			if	(alttree && altobj > 0)
				{
				obj_malen(altwnr, altobj);
				}
			if	(zieltree && zielobj > 0)
				{
				ob_sel(zieltree, zielobj);
				obj_malen(zielwnr, zielobj);
				ob_dsel(zieltree, zielobj);
				}
			alttree = zieltree;
			altobj  = zielobj;
			altwnr  = zielwnr;
			}
		}
	while(mstat & 1);

	Mfree(mov_boxes);

	if	(zielobj > 0)
		{
		obj_malen(zielwnr, zielobj);
		}
	Mgraf_mouse(ARROW);
	Mgraf_mouse(M_ON);

	/* Die zu verschiebenden Icons lagen im Desktop, Zielfenster */
	/* ist ebenfalls das Desktop, Zielobjekt ist ungÅltig oder   */
	/* der Desktop- Hintergrund :							 */
	/*	ICONS AUF DEM DESKTOP VERSCHIEBEN					 */
	/* --------------------------------------------------------- */

	if	(wnr == 0 && zielwnr == 0 && (zielobj == 0 || zielobj == -2) )
		{
		mv_dskt_icns(xrel, yrel);
		return;
		}

	/* Zielobjekt ist ungÅltig (etwa fremdes Fenster oder mit    */
	/* einem der Quellobjekte identisch) :					 */
	/*	NICHTS TUN									 */
	/* --------------------------------------------------------- */

	if	(zielobj < 0)
		{
		if	(zielwnr == 0)
			Bconout(2,7);		/* pling */
		return;
		}

	/* Objekte sind von einem Fenster aufs Desktop gezogen 	 */
	/* worden, und zwar nicht in ein gÅltiges Zielobjekt,		 */
	/* sondern auf eine freie FlÑche :						 */
	/*	FALLS 1 OBJEKT, DESKTOP- ICON ERSTELLEN				 */
	/* --------------------------------------------------------- */

	if	(wnr > 0 && zielwnr == 0 && zielobj == 0)
		{
		if	(ptr_g)
			mk_dskt_icns(wnr, xrel, yrel);
#if		COUNTRY==FRG
		else form_alert(1, "[3][Immer nur EINE Datei|aufs Desktop ziehen!][Abbruch]");
#elif	COUNTRY==USA
		else form_alert(1, "[3][Only ONE file can be placed on|desktop at one time!][Abort]");
#endif
		return;
		}

	/* Objekte sind von irgendwoher in ein gÅltiges Objekt auf 	 */
	/* dem Desktop gezogen worden:						 */
	/*	PAPIERKORB	: LôSCHEN							 */
	/*	DRUCKER   	: DRUCKEN							 */
	/* --------------------------------------------------------- */

	if	(zielwnr == 0 && zielobj > 0)
		{
		if	(icon[zielobj-1].icontyp == I_PAPIER)
			{
			dl_icns(wnr);
			return;
			}
		if	(icon[zielobj-1].icontyp == I_DRUCKR)
			{
			prt_icns(wnr, TRUE);
			return;
			}
		}

	/*   KOPIEREN / VERSCHIEBEN							 */
	/* --------------------------------------------------------- */

		{
		char	zielpfad[130];


		if	(1 != obj_to_path(zielwnr, zielobj, zielpfad, NULL))
			{
			dclick(zielwnr, zielobj, kbsh &= ~K_CTRL);
			return;
			}
		cache_on(zielpfad);					/* fÅr Zieldisk  */
		if	(wnr > 0 && anz_boxes > 1)
			cache_on (fenster[wnr].path);		/* fÅr Quelldisk */
		cpmv_icns(wnr, zielpfad, kbsh);
		if	(wnr > 0 && anz_boxes > 1)
			cache_off(fenster[wnr].path);		/* fÅr Quelldisk */
		cache_off(zielpfad);				/* fÅr Zieldisk  */
		}
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
	register OBJECT *tree = fenster[wnr].pobj;

	objc_grect(tree, obj, &g);
	(tree+obj)->ob_x += mvx;
	(tree+obj)->ob_y += mvy;
	redraw(wnr, &g);
	obj_malen(wnr, obj);
	return(0);
}

void	mv_dskt_icns(int x, int y)
{
	if	(x == 0 && y == 0)
		return;
	mvx = x;
	mvy = y;
	walk_sel(0, mv_obj);
}


/****************************************************************
*
* Verschieben von selektierten Window- Icons vom Fenster <wnr>
*  um <x>,<y> auf das Desktop.
*
****************************************************************/

static int mk_obj(int wnr, int obj)
{
	int typ;
	MYDTA *file;
	register OBJECT *tree = fenster[wnr].pobj + obj;


	file = fenster[wnr].pmydta+obj-1;
	typ = file -> icontyp;
	if	(make_icon(typ, 0, file->filename, fenster[wnr].path,
			    tree->ob_x + mvx, tree -> ob_y + mvy))
		return(-1);
	return(0);
}

void mk_dskt_icns(int wnr, int xrel, int yrel)
{
	mvx = fenster[wnr].pobj -> ob_x + xrel;
	mvy = fenster[wnr].pobj -> ob_y + yrel;
	walk_sel(wnr, mk_obj);
}


/****************************************************************
*
* Alle selektierten Icons von <wnr> sind in den Papierkorb
* geschoben worden.
*
****************************************************************/

static int dli_obj(int wnr, int obj)
{
	if	(wnr == 0 && icon[obj-1].icontyp >= I_ORDNER)
		kill_icon(obj);
	return(0);
}

static int dl_obj(int wnr, int obj)
{
	int ret;
	char	path[128];
	extern int  delete(char *path, int ispath);

	if	(qbreak())
		return((int) EBREAK);
	if	(0 > (ret = obj_to_path(wnr, obj, path, NULL)))
		return(0);
	return(delete(path, ret));
}

void dl_icns(int wnr)
{
	if	(wnr < 0)
		return;
	if	(wnr > 0)
		cache_on (fenster[wnr].path);
	walk_sel(wnr, dli_obj);
	if	(!confirm(wnr, 0, status.cnfm_del, fenster[wnr].path))
		{
		Mgraf_mouse(HOURGLASS);
		walk_sel(wnr, dl_obj);
		Mgraf_mouse(ARROW);
		}
	end_confirm();
	if	(wnr > 0)
		cache_off(fenster[wnr].path);
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
	char	path[128];
	extern int  show_file(char *path);
	extern int  print_file(char *path);

	if	(qbreak())
		return((int) EBREAK);
	if	(0 != obj_to_path(wnr, obj, path, NULL))	/* nur Dateien */
		return(0);
	return((_prt) ? print_file(path) : show_file(path));
}

void prt_icns(int wnr, int print)
{
	extern void starten(char *path, int typ, int wnr, int obj, int kbsh);
	extern OBJECT *adr_prgrms;
	extern int suffixtyp(char *s);
	char *pgm;
	int  typ;


	if	(wnr < 0)
		return;

#if		COUNTRY==FRG
	if	(print && (2 == form_alert(1, "[2][Druckprogramm aufrufen?][OK|Abbruch]")))
#elif	COUNTRY==USA
	if	(print && (2 == form_alert(1, "[2][Execute print program?][OK|Cancel]")))
#endif
		return;

	pgm = ((adr_prgrms+PG_2+print)->ob_spec.tedinfo)->te_ptext;
	if	(*pgm)		/* Druck- bzw. Anzeigeprogramm angegeben */
		{
		typ = suffixtyp(pgm);
		if	(typ == 'PP')		/* .TTP -> .TOS */
			typ = '_P';
		starten(pgm, typ, 0, 0, 0);
		return;
		}

	_prt = print;
	walk_sel(wnr, prt_obj);
}


/****************************************************************
*
* Alle selektierten Icons von <wnr> sind in einen Zielpfad
* geschoben worden.
*
****************************************************************/

static char *zielp;
static int  mflag;

static int cpmv_obj(int wnr, int obj)
{
	int  ret;
	char path[128];
	extern int  copy_move(char *quellpfad, char *zielpfad, int move_flag, int ispath);


	if	(qbreak())
		return((int) EBREAK);
	if	(0 > (ret = obj_to_path(wnr, obj, path, NULL)))
		return(0);
	return(copy_move(path, zielp, mflag, ret));
}

void cpmv_icns(int wnr, char *zielpfad, int kbsh)
{
	zielp = zielpfad;
	mflag = (kbsh & K_CTRL) ? 1 : 0;
	if	(!confirm(wnr, 1+mflag, status.cnfm_copy, zielpfad))
		{
		Mgraf_mouse(HOURGLASS);
		walk_sel(wnr, cpmv_obj);
		Mgraf_mouse(ARROW);
		}
	end_confirm();
}