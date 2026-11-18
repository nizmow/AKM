/*
*
* Enthält die spezifischen Routinen für den Dialog
* "Spezielle Icons auswählen"
*
*/

#include "APPLICAT_GEM.H"
#include <string.h>
#include <stdlib.h>
#include "IMPORT/GEMUT_MT.H"
#include "APPLICAT.H"
#include "APPL.H"
#include "INF.H"
#include "APPLDATA.H"
#include "ICONSEL.H"
#include "SPC_DIAL.H"


/* Das "Fenster" */

static void *sbox;

/* static int visible_len = -1; */
/*	static int selected_spec = -1;	*/




/*********************************************************************
*
* Umrechnungen
*
*********************************************************************/

static int showindex_to_icnobj(int index)
{
	static const WORD icons[] =
	{ SPCICON1, SPCICON2, SPCICON3, SPCICON4, SPCICON5 };

	return icons[index];
}
static int icnobj_to_showindex(int objnr)
{
	switch(objnr)
	{
		case SPCICON1: return 0;
		case SPCICON2: return 1;
		case SPCICON3: return 2;
		case SPCICON4: return 3;
		case SPCICON5: return 4;
	}
	return -1;
}


/*********************************************************************
*
* Auswahl- und Setzroutinen für die Scrollbox
*
*********************************************************************/

static void cdecl select_item(struct SLCT_ITEM_args args)
{
	(void) args;
}

static WORD cdecl set_item(struct SET_ITEM_args args)
{
	OBJECT *tree = args.tree;
	LBOX_ITEM *item = args.item;
	WORD index = args.obj_index;
	struct spc_file *myspc = (struct spc_file *) item;
	OBJECT *dob;
	OBJECT *sob;
/*	int len;	*/
	struct icon *ic;
	int ic_height,ob_height;


	if (!item)
	{
		dob = tree+(tree+index)->ob_head;
		dob->ob_spec.tedinfo->te_ptext[1] = EOS;
		dob->ob_flags &= ~TOUCHEXIT;
		dob = tree+dob->ob_next;
		dob->ob_flags |= HIDETREE;
		return index;
	}

	ob_height = tree[index].ob_height;

	/* Text */

	dob = tree+index;
	strcpy(dob->ob_spec.tedinfo->te_ptext + 1, myspc->name);
/*	len = (int) strlen(myspc->name);	*/
	if (myspc->sel)
		ob_sel(dob, 0);
	else
		ob_dsel(dob, 0);
/*	dob->ob_width = len * gl_hwchar;	*/
	ic = icnx + myspc->iconnr;

	/* Icons */

	dob = tree+dob->ob_head;

	if (item->selected)
		dob->ob_state &= ~WHITEBAK;
	else
		dob->ob_state |= WHITEBAK;

/*
	if (myspc->sel_icon)
		ob_sel(dob, 0);
	else
		ob_dsel(dob, 0);
*/
	sob = rscx[ic->rscfile].adr_icons + ic->objnr;
	ic_height = sob->ob_spec.ciconblk->monoblk.ib_hicon + 8;
	dob->ob_y = ob_height - ic_height - 2;
	dob->ob_spec = sob->ob_spec;
	dob->ob_type = sob->ob_type;
	dob->ob_width = sob->ob_width;
	dob->ob_height = sob->ob_height;
/*
	if (rect)
	{
		rect->g_x += xpos_textob;
		rect->g_w = visible_len*gl_hwchar;
	}
*/
	return index;
}


/*********************************************************************
*
* Verkette die Objekte
*
*********************************************************************/

static LBOX_ITEM *cat_spcs(void)
{
	register int i;
	register struct spc_file *spc;
	LBOX_ITEM *sc;

	/* Verkette die "spc_file"-Strukturen */
	for	(i = 0, spc = spcx; i < spcn - 1; i++, spc++)
	{
		spc->next = spc + 1;
	}
	if (spcn)
	{
		sc = (LBOX_ITEM *) spcx;
		spcx[spcn - 1].next = NULL;
	}
	else
		sc = NULL;
	return sc;
}


/*********************************************************************
*
* Initialisierung der Objektklasse "Spezialicon auswählen"
*
*********************************************************************/

void spc_dial_init_rsc(void)
{
	int /* dummy, */ i;
	static WORD ctrl_objs[5] =
	{DF_BK, DF_UP, DF_DOWN, DF_BSL, DF_SLID};
	static WORD objs[5] =
	{SPC1,SPC2,SPC3,SPC4,SPC5};


	if (!is_3d)
	{
		adr_spc_dialog[ctrl_objs[3]].ob_spec.obspec.fillpattern = 1;
		for	(i = 1; i < 5; i++)
			adr_spc_dialog[ctrl_objs[i]].ob_spec.obspec.framesize = 1;
	}
/*
	visible_len = (int) strlen(adr_spc_dialog[SPC1].ob_spec.tedinfo->te_ptext + 1);
*/
	sbox = lbox_create(adr_spc_dialog,
					select_item,
					set_item,
					cat_spcs(),	/* Items */
					5,		/* Anzahl sichtbarer Einträge */
					0,		/* erster sichtbarer Eintrag */
					ctrl_objs,
					objs,
					LBOX_VERT+LBOX_REAL+LBOX_SNGL+LBOX_TOGGLE,
					20,		/* Scrollverzögerung */
					NULL,	/* user data */
					NULL,
					0,0,0,0
					);
	if (!sbox)
		Pterm((int) ENSMEM);
}


/*********************************************************************
*
* Scrolle den Inhalt des "Fensters" so, daß die Zeile für
* das "special object" <key> oben sichtbar ist.
*
*********************************************************************/

static void scroll_win_key(DIALOG *d, long key)
{
	register struct spc_file *spc;
	register int n;

	for	(n = 0,spc = spcx; n < spcn; n++,spc++)
		if (spc->key == key)
			goto found;
	return;
	found:
	if ((n > 0) && (n > spcn - 5))
		n = spcn - 5;
	if (n < 0)
		n = 0;
	lbox_scroll_to(sbox, n, NULL, NULL);
	if (d)
		subobj_wdraw(d, DF_BK, DF_BK, MAX_DEPTH);
}


/*********************************************************************
*
* Das Icon für Object <myspc> hat sich geändert.
*
*********************************************************************/

static void chg_icon(int myspc, int iconnr)
{
	OBJECT *sob,*dob;
	int n;
	struct icon *ic;
	register int i;


	if (!d_spc)
		return;
	ic = icnx + iconnr;
	sob = rscx[ic->rscfile].adr_icons + ic->objnr;
	n = lbox_get_first(sbox);
	for	(i = 0; (i < 5) && (i + n < spcn); i++)
	{
		if (i + n != myspc)
			continue;
		dob = adr_spc_dialog + showindex_to_icnobj(i);
		ob_dsel(dob, 0);
		dob->ob_spec = sob->ob_spec;
		dob->ob_type = sob->ob_type;
		dob->ob_width	= sob->ob_width;
		dob->ob_height	= sob->ob_height;
		subobj_wdraw(d_spc, showindex_to_icnobj(i),
					0, MAX_DEPTH);
	}
}


/*********************************************************************
*
* Alle selektierten Icons als <iconnr> setzen
*
* Wird aufgerufen beim Doppelklick auf ein Icon im Fenster "Icons"
*
*********************************************************************/

void spc_dial_set_icon(int iconnr)
{
	register int i;


	for	(i = 0; i < spcn; i++)
	{
		if (spcx[i].sel)
		{
			spcx[i].iconnr = iconnr;
			chg_icon(i, iconnr);
		}
	}
}


/*********************************************************************
*
* Behandelt die Drag-Operation eines Icons auf ein Fenster.
*
*********************************************************************/

static void spc_set_icon(int iconnr, int objnr)
{
	int spc;

	spc = icnobj_to_showindex(objnr);
	if (spc >= 0)
	{
		spc += lbox_get_first(sbox);
		spcx[spc].iconnr = iconnr;
		chg_icon(spc, iconnr);
	}
}

static void spc_draw_subobj(int objnr)
{
	if (d_spc)
	subobj_wdraw(d_spc, objnr ,0, 8);
}

int spc_get_target_obj(int x, int y, int whdl, OBJECT **tree,
			WORD *objnr, void (**set_icon)(int iconnr, int objnr),
			void (**drawfn)(int objnr))
{
	GRECT dummy;

	if (!d_spc)
		return FALSE;		/* Objekt ungültig */

	if (whdl != wdlg_get_handle(d_spc))
		return FALSE;

	wdlg_get_tree(d_spc, tree, &dummy);
	*objnr = objc_find(*tree, 0, 8, x, y);

	if (icnobj_to_showindex(*objnr) >= 0)
	{
		*set_icon = spc_set_icon;
		*drawfn = spc_draw_subobj;
		return TRUE;
	}

	return FALSE;
}


/*********************************************************************
*
* Behandelt die Exit- Objekte des Icondialogs
* Das Exit-Objekt <objnr> wurde mit <clicks> Klicks angewählt.
*
* objnr = -1:	Initialisierung.
*			d->user_data und d->dialog_tree initialisieren!
*			<data> ist Zeiger auf Argumentzeiger,
*			d.h.		char *data[2]
*			<clicks> ist Parameter, z.Zt. immer 0
*		-2:	Nachricht int data[8] wurde übergeben
* 		-3:	Fenster wurde durch Closebutton geschlossen.
*		-4:	Programm wurde beendet.
*
* Rückgabe:	0	Dialog schließen
*			< 0	Fehlercode
*
*********************************************************************/


WORD cdecl spc_wdlg_cb(struct HNDL_OBJ_args args)
{
	DIALOG *d = args.dialog;
	short exitbutton = args.obj;
	short nclicks = args.clicks;
	void *data = args.data;
	OBJECT *tree;
	long key;


	/* 1. Fall: Dialog soll geöffnet werden */
	/* ------------------------------------ */

	tree = adr_spc_dialog;

	if (exitbutton == HNDL_INIT)
	{
		if (d_spc)			/* Dialog ist schon geöffnet ! */
			return 0;		/* create verweigern */

		if (is_multiwindow)
			objs_disable(tree, DF_OK, DF_CN, 0);

		if ((data) && (*((char **) data)))
		{
			memcpy(&key, *((char **) data), sizeof(long));
			scroll_win_key(NULL, key);
		}
	
		return 1;
	}

	/* 3. Fall: Dialog soll geschlossen werden */
	/* --------------------------------------- */

	if (exitbutton == HNDL_CLSD)	/* Wenn Dialog geschlossen werden soll... */
	{
		close_dialog:
		d_spc = NULL;
		return 0;		/* ...dann schließen wir ihn auch */
	}

	if (exitbutton < 0)
		return 1;

	/* 4. Fall: Exitbutton wurde betätigt */
	/* ---------------------------------- */

		/* Doppelklick auf Icon */
		/* -------------------- */

	if ((nclicks == 2) && (icnobj_to_showindex(exitbutton) >= 0))
	{
		open_iconsel();
		return 1;
	}


	if (nclicks != 1)
		goto ende;

	/* Scrollbox angewählt */
	/* ------------------- */

	if (exitbutton == SPCICON1)
		exitbutton = SPC1;
	if (exitbutton == SPCICON2)
		exitbutton = SPC2;
	if (exitbutton == SPCICON3)
		exitbutton = SPC3;
	if (exitbutton == SPCICON4)
		exitbutton = SPC4;
	if (exitbutton == SPCICON5)
		exitbutton = SPC5;
	if ((exitbutton == DF_UP) ||
		 (exitbutton == DF_DOWN) ||
		 (exitbutton == DF_BSL) ||
		 (exitbutton == DF_SLID) ||
		 (exitbutton == SPC1) ||
		 (exitbutton == SPC2) ||
		 (exitbutton == SPC3) ||
		 (exitbutton == SPC4) ||
		 (exitbutton == SPC5))
	{
		lbox_do(sbox, exitbutton);

		return 1;
	}


		/* Buttons zur Dialogbeendigung */
		/* ---------------------------- */

	if (exitbutton == DF_OK)			/* OK */
	{
		save_dialog_xy(d);
		if (put_inf())
			goto ende;				/* Fehler bei INF */
		goto close_dialog;
	}

	if (exitbutton == DF_CN)			/* Abbruch */
	{
		save_dialog_xy(d);
		goto close_dialog;
	}

	ende:
	ob_dsel(tree, exitbutton);
	subobj_wdraw(d, exitbutton, exitbutton, 1);
	return 1;		/* weiter */
}
