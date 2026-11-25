/*
*
* Enthält die spezifischen Routinen für den Dialog
* "Anwendungen/Dateitypen auswählen" bzw. "Registrierte Programme"
*
*/

#include "APPLICAT_GEM.H"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "GEMUT_MT.H"
#include "APPLICAT.H"
#include "WINDOWS.H"
#include "APPL.H"
#include "INF.H"
#include "ICA_DIAL.H"
#include "APPLDATA.H"
#include "ANW_DIAL.H"
#include "TYP_DIAL.H"
#include "ICONSEL.H"


#define NLINES 5
#define PGMS 0
#define DFLS 1
#define PDFLS 2

static WORD ctrl_objs1[NLINES] =
	{PN_BK, PN_UP, PN_DOWN, PN_BSL, PN_SLID};
static WORD objs1[NLINES] =
	{PRG1,PRG2,PRG3,PRG4,PRG5};
static WORD ctrl_objs2[NLINES] =
	{DN_BK, DN_UP, DN_DOWN, DN_BSL, DN_SLID};
static WORD objs2[NLINES] =
	{DAT1,DAT2,DAT3,DAT4,DAT5};

/*
 Wenn links ein Programm ausgewählt wurde, enthält die
 rechte Listbox die zugehörigen Dateitypen.
*/
struct curr_pgm_file_type
{
	struct curr_pgm_file_type *next;
	WORD selected;
	int datnr;
};

/* Die vier "Fenster" */

/*
 PDFLS:	Ist aktiv, wenn keine Applikation ausgewählt ist.

 DFLS:	Ist aktiv, wenn eine Applikation angewählt ist, und
		enthält die zugeordneten Dateitypen
*/

/* Diese Variablen sollten eigentlich alle nach struct <dialog_userdata> */
static LIST_BOX *sbox1,*sbox2;
static struct item_line *selected_pgmline = NULL;
static struct item_line *selected_datline = NULL;
static struct curr_pgm_file_type *selected_curr_pgm_file_type = NULL;
static struct dat_file  *selected_datfile = NULL;
static struct pgm_file  *selected_program = NULL;
static int selected_pgmnr = -1;
static int selected_datnr = -1;
static struct curr_pgm_file_type *sel_dfiles = NULL;
static int visible_pgmlen = -1;
static int visible_datlen = -1;

/* globale Variable! */
char *def_txt;		/* ->hdl_typ */


/*********************************************************************
*
* Umrechnungen
*
*********************************************************************/

static int showindex_to_pgmicnobj(int index)
{
	static const WORD icons[] = {PICON1,PICON2,PICON3,PICON4,PICON5};

	return icons[index];
}
static int showindex_to_daticnobj(int index)
{
	static const WORD icons[] = {DICON1,DICON2,DICON3,DICON4,DICON5};

	return icons[index];
}
static int pgmicnobj_to_showindex(int objnr)
{
	switch(objnr)
	{
		case PICON1: return 0;
		case PICON2: return 1;
		case PICON3: return 2;
		case PICON4: return 3;
		case PICON5: return 4;
	}
	return -1;
}
static int daticnobj_to_showindex(int objnr)
{
	switch(objnr)
	{
		case DICON1: return 0;
		case DICON2: return 1;
		case DICON3: return 2;
		case DICON4: return 3;
		case DICON5: return 4;
	}
	return -1;
}
static int pgmobj_to_showindex(int objnr)
{
	switch(objnr)
	{
		case PRG1: return 0;
		case PRG2: return 1;
		case PRG3: return 2;
		case PRG4: return 3;
		case PRG5: return 4;
	}
	return -1;
}
static int datobj_to_showindex(int objnr)
{
	switch(objnr)
	{
		case DAT1: return 0;
		case DAT2: return 1;
		case DAT3: return 2;
		case DAT4: return 3;
		case DAT5: return 4;
	}
	return -1;
}


/*********************************************************************
*
* Gibt den <n>-ten angemeldeten Dateityp für Programm <pgm>.
* Dreht die Richtung um, weil die zuletzt angemeldeten Typen
* vorn in der Liste stehen.
*
*********************************************************************/

static int pgm_filetype(int pgm, int n)
{
	int dat;

	dat = pgmx[pgm].ntypes;
	n = dat - n - 1;
	if ((n >= dat) || (n < 0))
		return -1;
	dat = pgmx[pgm].types;
	while (n)
	{
		dat = datx[dat].next;
		n--;
	}
	return dat;
}


/*********************************************************************
*
* Macht alle Scrollobjekte eines Fensters auswählbar
*
*********************************************************************/

static void enable_scroll(DIALOG *d, WORD *objs)
{
	register int i;

	for (i = 1; i <= 4; i++)
	{
		adr_ica_dialog[objs[i]].ob_state &= ~DISABLED;
		adr_ica_dialog[objs[i]].ob_flags |= TOUCHEXIT;
		if (d)
			subobj_wdraw(d, objs[i], 0, MAX_DEPTH);
	}
}


/*********************************************************************
*
* Macht alle Scrollobjekte eines Fensters nicht-auswählbar
*
*********************************************************************/

static void disable_scroll(DIALOG *d, WORD *objs)
{
	register int i;

	for (i = 1; i <= 4; i++)
	{
		adr_ica_dialog[objs[i]].ob_state |= DISABLED;
		adr_ica_dialog[objs[i]].ob_flags &= ~TOUCHEXIT;
		if (d)
			subobj_wdraw(d, objs[i], 0, MAX_DEPTH);
	}
}


/*********************************************************************
*
* Scrolle den Inhalt des "Fensters" so, daß die Zeile für
* die Applikation <app_name> oben sichtbar ist.
*
* Wenn <app_name> nicht gefunden, Rückgabe -1
* Fehler: Rückgabe -2
* sonst: Rückgabe der Programm-Nummer
*
*********************************************************************/

static int scroll_win_app(DIALOG *d, char *ap_path)
{
	char fname[MAX_NAMELEN];
	register struct pgm_file *pgm;
	register struct item_line *line;
	register int n,i;


	n = extract_apname(ap_path, fname);
	if (n < 0)
		return -2;
	for (n = 0,pgm = pgmx; n < pgmn; n++,pgm++)
		if (!stricmp(pgm->name, fname))
			goto found;
	return -1;
	found:
	i = n;
	for (n = 0,line = linx; n < linn; n++,line++)
		if (line->pgmnr == i)
			break;
	if ((n > 0) && (n > linn - NLINES))
		n = linn - NLINES;
	if (n < 0)
		n = 0;
	pgm->sel = TRUE;
	lbox_scroll_to(sbox1, n, NULL, NULL);
	if (d)
		subobj_wdraw(d, PN_BK, PN_BK, MAX_DEPTH);

	return i;
}


/*********************************************************************
*
* Sucht einen Dateityp
* Wenn <dat_name> nicht gefunden, Rückgabe -1
* sonst: Rückgabe der Datei-Nummer
*
*********************************************************************/

static int scroll_win_dat(DIALOG *d, char *datname)
{
	register int j,k;
	register struct dat_file *dat;

	for (j = 0,dat = datx; j < datn; j++,dat++)
	{
		if (!stricmp(dat->name, datname))
			goto found;
	}
	return -1;

	found:
	if (dat->pgm == selected_pgmnr)
	{
		for (k = 0; k < selected_program->ntypes; k++)
		{
			if (j == sel_dfiles[k].datnr)
			{
				lbox_scroll_to(sbox2, k, NULL, NULL);
				if (d)
					subobj_wdraw(d, DN_BK, DN_BK, MAX_DEPTH);
				break;
			}
		}
	}
	else
	{
		if (!selected_program)
		{
			for (k = 0; k < linn; k++)
			{
				if (linx[k].datnr == j)
				{
					lbox_scroll_to(sbox2, k, NULL, NULL);
					if (d)
						subobj_wdraw(d, DN_BK, DN_BK, MAX_DEPTH);
					break;
				}
			}
		}
	}
	return j;
}


/*********************************************************************
*
* "select callback" für Programme-Scrollbox
*
* item->selected enthält bereits den neuen Selektions-Status,
* der vorherige wird in <last_state> übergeben.
*
* Man beachte, daß linke und rechte Scrollbox dieselben
* LBOX_ITEMs verwenden, also auch denselben <selected>-Status.
*
*********************************************************************/

static void cdecl scrollbox_pgm_select_cb(struct SLCT_ITEM_args args)
{
	LBOX_ITEM *item = args.item;
	struct item_line *line = (struct item_line *) item;

	if (line->selected)
	{
		selected_pgmline = line;
	}
	else
	{
		selected_pgmline = NULL;
	}
}


/*********************************************************************
*
* "select callback" für Dateitypen-Scrollbox
*
* item->selected enthält bereits den neuen Selektions-Status,
* der vorherige wird in <last_state> übergeben.
*
*********************************************************************/

static void cdecl scrollbox_dat_select_cb(struct SLCT_ITEM_args args)
{
	LBOX_ITEM *item = args.item;
	if (selected_program)
	{
		struct curr_pgm_file_type *spd = (struct curr_pgm_file_type *) item;

		if (item->selected)
		{
			selected_curr_pgm_file_type = spd;
		}
		else
		{
			selected_curr_pgm_file_type = NULL;
		}
	}
	else
	{
		struct item_line *line = (struct item_line *) item;

		/*
		* Man beachte, daß linke und rechte Scrollbox dieselben
		* LBOX_ITEMs verwenden, also auch denselben <selected>-Status,
		* aber nur, wenn links kein Programm ausgewählt ist.
		*/

		if (item->selected)
		{
			selected_datline = line;
		}
		else
		{
			selected_datline = NULL;
		}
	}
}


/*********************************************************************
*
* "set callback" für Programme-Scrollbox
*
*********************************************************************/

static WORD cdecl scrollbox_pgm_set_cb(struct SET_ITEM_args args)
{
	LIST_BOX *box = args.box;
	OBJECT *tree = args.tree;
	LBOX_ITEM *item = args.item;
	WORD index = args.obj_index;
	struct item_line *line = (struct item_line *) item;
	GRECT *redr;
	struct pgm_file *mypgm;
	OBJECT *dob;
	OBJECT *sob;
	int len;
	struct icon *ic;
	int ic_height,ob_height;


	/* Scrolling synchronisieren */
	/* ------------------------- */

	if (sbox2 && !selected_program)
	{
		len = lbox_get_first(box);
		if (len != lbox_get_first(sbox2))
		{
			if (d_ica)
				redr = (GRECT *) &(adr_ica_dialog->ob_x);
			else
				redr = NULL;
			lbox_scroll_to(sbox2, len, redr, redr);
		}
	}


	if ((!item) || ((line->reldatnr != -1) && (line->reldatnr != 0)))
	{
		dob = tree+index;
		dob->ob_spec.tedinfo->te_ptext[1] = EOS;
		dob->ob_flags &= ~TOUCHEXIT;
		dob = tree+dob->ob_head;
		dob->ob_flags |= HIDETREE;
		return index;
	}

	ob_height = tree[index].ob_height;

	/* Text */

	mypgm = pgmx+line->pgmnr;
	dob = tree+index;
	dob->ob_flags &= ~HIDETREE;
	dob->ob_flags |= TOUCHEXIT;
	dob->ob_state &= ~DISABLED;
	strncpy(dob->ob_spec.tedinfo->te_ptext + 1,
			mypgm->name, visible_pgmlen);
/*	len = (int) strlen(dob->ob_spec.tedinfo->te_ptext + 1);	*/
	if (mypgm->sel)
/*	if (item->selected)	*/
	{
		ob_sel(dob, 0);
	}
	else
	{
		if (selected_program)
		{
			dob->ob_flags &= ~TOUCHEXIT;
			dob->ob_state |= DISABLED;
		}
		ob_dsel(dob, 0);
	}
/*	dob->ob_width = len * gl_hwchar;	*/
	ic = icnx + mypgm->iconnr;

	/* Icons */

	dob = tree+dob->ob_head;
	if (mypgm->name[0] == '<')
	{
		dob->ob_flags |= HIDETREE;
		return index;
	}
	dob->ob_flags &= ~HIDETREE;
	dob->ob_flags |= TOUCHEXIT;

	if ((!mypgm->sel) && (selected_program))
		dob->ob_flags &= ~TOUCHEXIT;

	if (mypgm->sel)
		dob->ob_state &= ~WHITEBAK;
	else
		dob->ob_state |= WHITEBAK;
/*
	if (mypgm->sel_icon)
		ob_sel(dob, 0);
	else
		ob_dsel(dob, 0);
*/
	sob = rscx[ic->rscfile].adr_icons + ic->objnr;
	ic_height = sob->ob_spec.ciconblk->monoblk.ib_hicon+8;
	dob->ob_y = ob_height - ic_height - 2;
	dob->ob_spec	= sob->ob_spec;
	dob->ob_type	= sob->ob_type;
	dob->ob_width	= sob->ob_width;
	dob->ob_height	= sob->ob_height;

/*
	if (rect)
	{
		rect->g_x += xpos_textob;
		rect->g_w = visible_pgmlen*gl_hwchar;
	}
*/
	return index;
}


/*********************************************************************
*
* "set callback" für Dateitypen-Scrollbox
*
*********************************************************************/

static WORD cdecl scrollbox_dat_set_cb(struct SET_ITEM_args args)
{
	OBJECT *tree = args.tree;
	LBOX_ITEM *item = args.item;
	WORD index = args.obj_index;
	struct item_line *line = (struct item_line *) item;
	struct curr_pgm_file_type *sd = (struct curr_pgm_file_type *) item;
	struct dat_file *mydat;
	OBJECT *dob;
	OBJECT *sob;
	int len;
	struct icon *ic;
	int ic_height,ob_height;


	if (item)
		len = (selected_program) ? sd->datnr : line->datnr;
	else
		len = -1;
	if ((!item) || (!item->selected))
		tree[index].ob_state &= ~SELECTED;

	if (len == -1)
		tree[index].ob_flags &= ~TOUCHEXIT;
	else
		tree[index].ob_flags |= TOUCHEXIT;

	if ((!item) || (len == -1))
	{
		dob = tree+index;
		dob->ob_spec.tedinfo->te_ptext[1] = EOS;
		dob = tree+dob->ob_head;
		dob->ob_flags |= HIDETREE;
		return index;
	}

	ob_height = tree[index].ob_height;

	/* Text */

	mydat = datx+len;
	dob = tree+index;
	dob->ob_flags &= ~HIDETREE;
	strncpy(dob->ob_spec.tedinfo->te_ptext + 1,
			mydat->name, visible_datlen);
/*	len = (int) strlen(dob->ob_spec.tedinfo->te_ptext + 1); */
	if (item->selected)
		ob_sel(dob, 0);
	else
		ob_dsel(dob, 0);
/*	dob->ob_width = len * gl_hwchar;	*/
	ic = icnx + mydat->iconnr;

	/* Icons */

	dob = tree+dob->ob_head;
	dob->ob_flags &= ~HIDETREE;
	if (item->selected)
		dob->ob_state &= ~WHITEBAK;
	else
		dob->ob_state |= WHITEBAK;
/*
	if (mydat->sel_icon)
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
		rect->g_x += xpos_textob2;
		rect->g_w = visible_datlen*gl_hwchar;
	}
*/
	return index;
}


/*********************************************************************
*
* Verkette sämtliche LBOX_ITEM-Objekte
*
* Man beachte, daß die beiden parallelen Scrollboxen, für Programme
* und Dateitypen, dieselben LBOX_ITEMs verwenden. Sie teilen sich
* also auch das <selected>-Attribut. Es muß sichergestellt werden,
* daß die Boxen immer parallel laufen und daß die linke Box
* verriegelt wird, während die rechte bearbeitet wird.
*
*********************************************************************/

static LBOX_ITEM *cat_lines(void)
{
	register int i;
	register struct item_line *line;
	LBOX_ITEM *sc;

	/* Verkette die "item_line"-Strukturen */
	for (i = 0,line = linx; i < linn - 1; i++,line++)
	{
		line->next = line + 1;
	}
	if (linn)
	{
		sc = (LBOX_ITEM *) linx;
		linx[linn - 1].next = NULL;
	}
	else
	{
		sc = NULL;
	}

	return sc;
}


/*********************************************************************
*
* Initialisierung der Objektklasse "Icon auswählen"
*
*********************************************************************/

void ica_dial_init_rsc(void)
{
/*	int dummy;	*/
	register int i;
	LBOX_ITEM *sc;


	sc = cat_lines();

	if (!is_3d)
	{
		for (i = 1; i < 5; i++)
		{
			adr_ica_dialog[ctrl_objs1[i]].ob_spec.obspec.framesize =
			adr_ica_dialog[ctrl_objs2[i]].ob_spec.obspec.framesize = 1;
		}

		adr_ica_dialog[ctrl_objs1[3]].ob_spec.obspec.fillpattern =
		adr_ica_dialog[ctrl_objs2[3]].ob_spec.obspec.fillpattern = 1;
	}

	adr_ica_dialog[DEL_PGM].ob_state |= DISABLED;
	adr_ica_dialog[NEU_DAT].ob_state |= DISABLED;
	adr_ica_dialog[DEL_DAT].ob_state |= DISABLED;

	adr_ica_dialog[BUT_VERSION].ob_spec.free_string = PROGRAM_VERSION_STR;

	enable_scroll(NULL, ctrl_objs1);
	disable_scroll(NULL, ctrl_objs2);

	visible_pgmlen = (int) strlen(adr_ica_dialog[PRG1].ob_spec.tedinfo->te_ptext + 1);
	visible_datlen = (int) strlen(adr_ica_dialog[DAT1].ob_spec.tedinfo->te_ptext + 1);

	/* Scrollbox für Programme */
	sbox1 = lbox_create(adr_ica_dialog,
					scrollbox_pgm_select_cb,
					scrollbox_pgm_set_cb,
					sc,	/* Items */
					5,		/* Anzahl sichtbarer Einträge */
					0,		/* erster sichtbarer Eintrag */
					ctrl_objs1,
					objs1,
					LBOX_VERT+LBOX_REAL+LBOX_SNGL+LBOX_TOGGLE,
					20,		/* Scrollverzögerung */
					NULL,	/* user data */
					NULL,
					0,0,0,0
					);
	if (!sbox1)
		Pterm((int) ENSMEM);

	/* Scrollbox für Dateitypen */
	sbox2 = lbox_create(adr_ica_dialog,
					scrollbox_dat_select_cb,
					scrollbox_dat_set_cb,
					sc,		/* Items */
					5,		/* Anzahl sichtbarer Einträge */
					0,		/* erster sichtbarer Eintrag */
					ctrl_objs2,
					objs2,
					LBOX_VERT+LBOX_REAL+LBOX_SNGL+LBOX_TOGGLE,
					20,		/* Scrollverzögerung */
					NULL,	/* user data */
					NULL,
					0,0,0,0
					);
	if (!sbox2)
		Pterm((int) ENSMEM);
}


/*********************************************************************
*
* Setzt für alle Zeilen der Scrollboxen das Attribut
* item_line->sel korrekt.
* Außerdem werden <selected_pgmline> und <selected_datline>
* neu bestimmt.
*
*********************************************************************/

static void sync_sel(void)
{
	register int i, n;
	register struct item_line *z;
	register struct curr_pgm_file_type *sd;


	selected_pgmline = selected_datline = NULL;
	for (i = 0, z = linx; i < linn; i++, z++)
	{
		z->selected = FALSE;
		if (!z->reldatnr || z->reldatnr == -1)
		{
			if (pgmx[z->pgmnr].sel)
			{
				selected_pgmline = z;
				z->selected = TRUE;
			}
		}
		if (z->datnr != -1)
		{
			if (datx[z->datnr].sel)
			{
				selected_datline = z;
				z->selected = TRUE;
			}
		}
	}

	selected_curr_pgm_file_type = NULL;
	if (sel_dfiles && selected_program)
	{
		n = selected_program->ntypes;
		for (i = 0, sd = sel_dfiles; i < n; i++, sd++)
		{
			sd->selected = FALSE;
			if (datx[sd->datnr].sel)
			{
				sd->selected = TRUE;
				selected_curr_pgm_file_type = sd;
			}
		}
	}
}


/*********************************************************************
*
* Initialisiert die linke Scrollbox, die mit den Programmen,
* und setzt, wenn <line> nicht -1 ist, den Scrollbalken so, daß
* Zeile <line> die erste sichtbare ist.
*
*********************************************************************/

static void setup_lbox_programs(int line)
{
	if (line < 0)
	{
		line = lbox_get_first(sbox1);
	}

	lbox_set_items(sbox1, (pgmn) ? (LBOX_ITEM *) linx : NULL);
	lbox_set_slider(sbox1, line, NULL);
	lbox_update(sbox1, NULL);
	if (d_ica)
	{
		subobj_wdraw(d_ica, PN_BK, PN_BK, MAX_DEPTH);
		subobj_wdraw(d_ica, PN_BSL, PN_BSL, MAX_DEPTH);
	}
}


/*********************************************************************
*
* Initialisiert die rechte Scrollbox, die mit den Dateitypen,
* und setzt, wenn <line> nicht -1 ist, den Scrollbalken so, daß
* Zeile <line> die erste sichtbare ist.
*
*********************************************************************/

static void setup_lbox_datafiles(int line)
{
	if (line < 0)
		line = lbox_get_first(sbox1);
	lbox_set_items(sbox2, (pgmn) ? (LBOX_ITEM *) linx : NULL);
	lbox_set_slider(sbox2, line, NULL);
	if (d_ica)
	{
		lbox_update(sbox2, NULL);
		subobj_wdraw(d_ica, DN_BK, DN_BK, MAX_DEPTH);
		subobj_wdraw(d_ica, DN_BSL, DN_BSL, MAX_DEPTH);
	}
}


/*********************************************************************
*
* Trägt die zum selektierten Programm gehörigen
* Dateitypen in die zweite Scrollbox ein.
*
*********************************************************************/

static void setup_spdfiles(int line)
{
	register int i, n;
	register struct curr_pgm_file_type *sd;


	selected_curr_pgm_file_type = NULL;
	if (sel_dfiles)
		free(sel_dfiles);

	n = selected_program->ntypes;
	if (n)
	{
		sel_dfiles = malloc(n * sizeof(struct curr_pgm_file_type));
		if (sel_dfiles)
		{
			for (i = 0, sd = sel_dfiles; i < n; i++,sd++)
			{
				sd->selected = FALSE;
				if (i < n - 1)
					sd->next = sd + 1;
				else
					sd->next = NULL;
				sd->datnr = pgm_filetype(selected_pgmnr, i);
				if (datx[sd->datnr].sel)
				{
					sd->selected = TRUE;
					selected_curr_pgm_file_type = sd;
				}
			}
		}
		else
		{
			err_alert(ENSMEM);
		}
	}
	else
	{
		sel_dfiles = NULL;
	}

	if (line < 0)
		line = lbox_get_first(sbox2);
	lbox_set_items(sbox2, (LBOX_ITEM *) sel_dfiles);
	lbox_set_slider(sbox2, line, NULL);

	lbox_update(sbox2, NULL);
	if (d_ica)
	{
		subobj_wdraw(d_ica, DN_BK, DN_BK, MAX_DEPTH);
		subobj_wdraw(d_ica, DN_BSL, DN_BSL, MAX_DEPTH);
	}
}


/*********************************************************************
*
* Trägt einen neuen Dateityp ein.
* Die Dateitypen werden so sortiert, daß die Typen mit "*.<ext>"
* hinten stehen.
*
*********************************************************************/

int insert_dat(struct pgm_file *pgm, struct dat_file *dat)
{
	register struct pgm_file *mypgm;
	register struct dat_file *mydat;
	register struct item_line *z;
	int *prev;
	register int pgmnr,j,k,l;
	int newdat;
	int newline;


	if (linn >= MAX_LINN)
	{
		Rform_alert(1, ALRT_OVERFLOW, NULL);
		return -3;				/* Überlauf */
	}

	/* Suche <pgm> in pgmx[] */
	/* --------------------- */

	for (pgmnr = 0,mypgm = pgmx; pgmnr < pgmn; pgmnr++,mypgm++)
	{
		if (!stricmp(mypgm->name, pgm->name))
			goto found;
	}
	Rform_alert(1, ALRT_APP_INVALID, NULL);
	return -1;		/* nicht mehr gefunden! */
	found:

	/* Suche <dat.name> in datx[] */
	/* -------------------------- */

	for (j = 0,mydat = datx; j < datn; j++,mydat++)
	{
		if (!stricmp(mydat->name, dat->name))
		{
			char s[200];

			sprintf(s, Rgetstring(ALRT_FTYPE_INUSE, NULL),
					dat->name,
					(pgmx + (mydat->pgm))->name);
			form_alert(1, s);
			return -1;
		}
	}

	/* dat neu eintragen */
	/* ----------------- */

	for (j = 0,mydat = datx; j < datn; j++,mydat++)
	{
		if (!mydat->name[0])
		{
			newdat = j;
			goto ok_new;
		}
	}

	if (datn >= MAX_DATN)
		return -2;		/* Überlauf */
	newdat = datn;
	datn++;

	ok_new:
	datx[newdat] = *dat;
	dat = datx + newdat;		/* dat = Zeiger auf neuen Eintrag */

	/* dat in die Verkettung für dat->pgm eintragen 			*/
	/* wird als <k-te> angemeldete Datei zu mypgm eingetragen	*/
	/* --------------------------------------------------------	*/

	j = mypgm->types;
	k = mypgm->ntypes;
	prev = &(mypgm->types);
	while (j >= 0)
	{
		mydat = datx+j;
		if ((dat->name[0] == '*') && (mydat->name[0] != '*'))
			break;	/* erster Typ, der mit '*' anfängt */
		if ((dat->name[0] != '*') && (mydat->name[0] == '*'))
			goto weiter5;
		if (stricmp(dat->name, mydat->name) >= 0)
			break;
		weiter5:
		j = mydat->next;
		prev = &(mydat->next);
		k--;
	}

	dat->pgm = pgmnr;
	dat->next = j;
	*prev = newdat;
	mypgm->ntypes++;

	/* ggf. einsortieren in linx[] */
	/* --------------------------- */

	for (j = 0, z = linx; j < linn; j++,z++)
	{
		if (z->pgmnr != pgmnr)
			continue;
		if (z->datnr == -1)	/* noch kein Dateityp */
			break;
		j += k;		/* rel. Pos der neuen Datei */
		z += k;
		for (l = linn; l > j; l--)
			linx[l] = linx[l - 1];
		linn++;
		z->selected = FALSE;
		break;
	}

	z->pgmnr = pgmnr;
	z->datnr = newdat;
	z->reldatnr = k;
	newline = j;
	j++;
	while ((j < linn) && (linx[j].pgmnr == pgmnr))
	{
		linx[j].reldatnr++;
		j++;
	}

	if (selected_program)
		newline = -1;
	cat_lines();
	setup_lbox_programs(newline);
	if (!selected_program)
	{
		setup_lbox_datafiles(-1);
	}
	else
	{
		if (selected_pgmnr == pgmnr)
		{
			for (j = 0; j < mypgm->ntypes; j++)
			{
				if (newdat == pgm_filetype(pgmnr, j))
				{
					newline = j;
					break;
				}
			}
		}
		setup_spdfiles(newline);
	}

	return 0;
}


/*********************************************************************
*
* Löscht einen Dateityp.
*
*********************************************************************/

int delete_dat(int dat)
{
	register int i,k;
	int pgm;
	int *prev;


	pgm = datx[dat].pgm;

	/* aussortieren aus linx[] */
	/* ----------------------- */

	for (i = 0; i < linn; i++)
	{
		if (linx[i].datnr == dat)
		{
			if ((linx[i].reldatnr == 0) &&
				 ((i + 1 >= linn) || (linx[i + 1].pgmnr != pgm)))
			{
				linx[i].datnr = linx[i].reldatnr = -1;
				goto no_del;
			}
			for (k = i; k < linn - 1; k++)
				linx[k] = linx[k + 1];
			for (k = i; (k < linn) && (linx[k].pgmnr == pgm); k++)
				linx[k].reldatnr--;
			goto ok2;
		}
	}

	exit(-1);			/* ??? */

	ok2:
	linn--;

	no_del:
	cat_lines();
	setup_lbox_programs(-1);

	/* Dateityp aus Liste für pgm entfernen */
	/* ------------------------------------ */

	for (prev = &(pgmx[pgm].types); *prev >= 0;
			prev = &datx[*prev].next)
	{
		if (*prev == dat)
		{
			*prev = datx[*prev].next;	/* ausklinken */
			goto ok3;
		}
	}

	exit(-1);			/* ??? */
	ok3:
	pgmx[pgm].ntypes--;

	/* Dateityp entfernen */
	/* ------------------ */

	datx[dat].name[0] = EOS;

	if (selected_pgmnr == pgm)
	{
		setup_spdfiles(-1);
	}
	else
	{
		if (selected_program)
			return 0;		/* ?!? */
		setup_lbox_datafiles(-1);
	}

	return 0;
}


/*********************************************************************
*
* Ändert einen Dateityp
*
*********************************************************************/

int change_dat(struct pgm_file *pgm, struct dat_file *dat,
				char *newname)
{
	register int i,j;
	struct dat_file dat_new;
	DIALOG *d_icons_prev;


	/* Suche <pgm> in pgmx[] */
	/* --------------------- */

	for (i = 0; i < pgmn; i++)
	{
		if (!stricmp(pgmx[i].name, pgm->name))
			goto found;
	}
	Rform_alert(1, ALRT_APP_INVALID, NULL);
	return -1;		/* nicht mehr gefunden! */
	found:

	/* Suche <dat.name> in datx[] */
	/* -------------------------- */

	for (j = 0; j < datn; j++)
	{
		if (!stricmp(datx[j].name, dat->name))
			goto found2;
	}
	Rform_alert(1, ALRT_DAT_INVALID, NULL);
	return -1;		/* nicht mehr gefunden! */

	found2:

	/* dat ändern */
	/* ---------- */

	dat_new = *dat;
	strcpy(dat_new.name, newname);
	d_icons_prev = d_ica;
	d_ica = NULL;		/* Redraw verhindern */
	delete_dat(j);
	d_ica = d_icons_prev;
	insert_dat(pgm, &dat_new);
	return 0;
}


/*********************************************************************
*
* Registriert ein neues Programm oder ändert ein registriertes
*
*********************************************************************/

int insert_pgm(struct pgm_file *pgm)
{
	register int k;
	register int pgmx_pos;
	int pgmx_free_pos;
	register int linx_pos;


	/* Sicherheitstests */
	/* ---------------- */

	if (pgmx[pgmn].ntypes != 0)
		return -1;			/* ??? */
	if (pgmn < 1)
		return -1;			/* ??? */

	/* Einsortieren in pgmx[] */
	/* ---------------------- */

	pgmx_free_pos = -1;					/* noch kein freier Eintrag in pgmx[] */
	for (pgmx_pos = 0; pgmx_pos < pgmn; pgmx_pos++)
	{
		if (!stricmp(pgmx[pgmx_pos].name, pgm->name))
		{
			/* Das Programm ist bereits in der Liste. Wir ändern es. */

			/* erst in pgmx[] ändern... */
			strcpy(pgmx[pgmx_pos].path, pgm->path);
			pgmx[pgmx_pos].config = pgm->config;
			pgmx[pgmx_pos].memlimit = pgm->memlimit;

			/* ... , dann in linx[] suchen und ändern */
			for (linx_pos = 0; linx_pos < linn; linx_pos++)
			{
				if (linx[linx_pos].pgmnr == pgmx_pos)
				{
					/*
					 * Das Programm liegt bei pgmx[pgmx_pos] und linx[linx_pos].
					 * Wir scrollen die Liste so, daß die Programmzeile oben sichtbar ist.
					 */
					goto redraw;
				}
			}

			/*
			 * TODO: Unter welchen Umständen können wir hier hin gelangen?
			 *       Wenn das Programm in pgmx[] ist, müßte es auch in linx[] sein.
			 * <linx_pos> ist jetzt <linn>
			 */

			Cconws("APPLICAT: INTERNAL ERROR #1");
			linx_pos = -1;
			goto redraw;			/* Name gefunden */
		}

		/* Wir haben das Programm noch nicht gefunden, aber suchen nebenbei nach freien Einträgen */

		if (!pgmx[pgmx_pos].name[0])		/* freier Eintrag */
		{
			pgmx_free_pos = pgmx_pos;	/* wir merken uns den letzten freien Eintrag */
		}
	}

	/*
	 * Wir haben das Programm in der Liste pgmx[]
	 * der registrierten Programme nicht gefunden.
	 * Wenn <pgmx_free_pos> >= 0, dann ist hier ein freier Eintrag.
	 */

	if (pgmx_free_pos >= 0)
	{
		pgmx_pos = pgmx_free_pos;					/* freier Eintrag */
	}
	else
	{
		if (pgmx_pos >= MAX_PGMN)
			return -2;			/* Überlauf */
		pgmn++;		/* Tabelle verlängern */
	}

	/*
	 * Bei pgmx[pgmx_pos] ist ein freier Eintrag, wo wir das neue
	 * Programm reinschreiben. Wenn die Liste verlängert werden mußte,
	 * ist das bereits oben passiert.
	 */

	pgm->ntypes = 0;
	pgm->types = -1;
	pgm->sel = /* pgm->sel_icon = */ FALSE;
	pgmx[pgmx_pos] = *pgm;

	/* ggf. einsortieren in linx[] */
	/* --------------------------- */

	if (linn >= MAX_LINN)
		return -3;				/* Überlauf */

	for (linx_pos = 0; linx_pos < linn; linx_pos++)
	{
		if ((pgmx[linx[linx_pos].pgmnr].name[0] == '<') ||
			 (stricmp(pgmx[linx[linx_pos].pgmnr].name, pgm->name) > 0))
		{
			/* Tabelleneinträge j..linn-1 verschieben nach j+1..linn */
			for (k = linn; k > linx_pos; k--)
			{
				linx[k] = linx[k - 1];
			}
			goto ok2;
		}
	}

	ok2:

	/* linx[] ist um einen Eintrag, bei <linx_pos>, verlängert worden */

	linn++;
	linx[linx_pos].pgmnr = pgmx_pos;
	linx[linx_pos].reldatnr = linx[linx_pos].datnr = -1;
	linx[linx_pos].selected = FALSE;

	redraw:
	cat_lines();
	if (selected_program)	/* Programm selektiert, dann... */
	{
		sync_sel();		/* Selektionsstatus korrigieren, selected_pgmline/datline setzen */
		linx_pos = (int) (selected_pgmline - linx);			/* ...nicht scrollen! */
	}

	setup_lbox_programs(linx_pos);
	if (!selected_program)
	{
		setup_lbox_datafiles(-1);
	}
	return 0;
}


/*********************************************************************
*
* Entfernt ein Programm aus den globalen Listen linx[] und pgmx[]
*
*********************************************************************/

int delete_pgm(int pgm)
{
	register int i,j,k;


	/* aussortieren aus linx[] */
	/* ----------------------- */

	for (i = 0; i < linn; i++)
	{
		if (linx[i].pgmnr == pgm)
		{
			for (j = i + 1; (linx[j].pgmnr == pgm) && (j < linn); j++)
				;
			for (k = j; k < linn; k++)
				linx[k + i - j] = linx[k];
			goto ok2;
		}
	}

	exit(-1);			/* ??? */

	ok2:

	if (pgmx[pgm].ntypes)
		linn -= pgmx[pgm].ntypes;
	else
		linn--;

	/* Dateitypen entfernen */
	/* -------------------- */

	for (i = pgmx[pgm].types; i >= 0;)
	{
		datx[i].name[0] = EOS;	/* als unbenutzt markieren */
		i = datx[i].next;
	}

	/* Programm entfernen */
	/* ------------------ */

	pgmx[pgm].name[0] = EOS;


	/* Redraw */
	/* ------ */

	cat_lines();
	setup_lbox_programs(-1);
	setup_lbox_datafiles(-1);

	return 0;
}


/*********************************************************************
*
* Das Icon für Applikation <mypgm> hat sich geändert.
*
*********************************************************************/

static void chg_icon(int win, int meinding, int iconnr)
{
	OBJECT *sob,*dob;
	int ding,n;
	struct icon *ic;
	register int i,j;


	if (!d_ica)
		return;
	ic = icnx + iconnr;
	sob = rscx[ic->rscfile].adr_icons + ic->objnr;
	n = lbox_get_first((win == PGMS) ? sbox1 : sbox2);
	for (i = 0; i < NLINES; i++)
	{
		if (win == PGMS)
			ding = (i + n < linn) ? linx[i + n].pgmnr : -1;
		else
		if (win == PDFLS)
			ding = (i + n < linn) ? linx[i + n].datnr : -1;
		else
	/*	if (win == DFLS)	*/
			ding = (i + n < selected_program->ntypes) ?
					sel_dfiles[i + n].datnr : -1;

		if (ding != meinding)
			continue;
		j = (win == PGMS) ? showindex_to_pgmicnobj(i) :
						showindex_to_daticnobj(i);
		dob = adr_ica_dialog + j;
		ob_dsel(dob, 0);
		dob->ob_spec = sob->ob_spec;
		dob->ob_type = sob->ob_type;
		dob->ob_width = sob->ob_width;
		dob->ob_height = sob->ob_height;
		subobj_wdraw(d_ica, j, 0, MAX_DEPTH);
	}
}


/*********************************************************************
*
* Alle selektierten Icons als <iconnr> setzen.
*
* Wird aufgerufen beim Doppelklick auf ein Icon im Fenster "Icons"
*
* Achtung: Wenn sowohl Datei-Icons als auch ein Programm-Icon
*          selektiert sind, müssen wir verhindern, daß das Programm
*          das gleiche Icon wie die Datei(en) bekommt. Deshalb
*          präferieren wir hier die Datei(en).
*
*********************************************************************/

void ica_dial_set_icon(int iconnr)
{
	register int i;
	int set = FALSE;

	/* Dateitypen */
	for (i = 0; i < datn; i++)
	{
		if (datx[i].sel)
		{
			datx[i].iconnr = iconnr;
			chg_icon((selected_program) ? DFLS : PDFLS, i, iconnr);
			set = TRUE;
		}
	}

	if (set)
	{
		return;		/* Datei zugewiesen, Programm ignorieren */
	}

	/* Programme */
	for (i = 0; i < pgmn; i++)
	{
		if (pgmx[i].sel)
		{
			pgmx[i].iconnr = iconnr;
			chg_icon(PGMS, i, iconnr);
		}
	}
}


/*********************************************************************
*
* Behandelt die Drag-Operation eines Icons auf ein Fenster.
*
*********************************************************************/

static void ica_set_icon(int iconnr, int objnr)
{
	int pgm,dat;

	pgm = pgmicnobj_to_showindex(objnr);
	if (pgm >= 0)
	{
		pgm += lbox_get_first(sbox1);
		pgm = linx[pgm].pgmnr;
		pgmx[pgm].iconnr = iconnr;
		chg_icon(PGMS, pgm, iconnr);
		return;
	}

	dat = daticnobj_to_showindex(objnr);
	if (dat >= 0)
	{
		dat += lbox_get_first(sbox2);
		if (selected_program)
		{
			dat = sel_dfiles[dat].datnr;
			datx[dat].iconnr = iconnr;
			chg_icon(DFLS, dat, iconnr);
		}
		else
		{
			dat = linx[dat].datnr;
			datx[dat].iconnr = iconnr;
			chg_icon(PDFLS, dat, iconnr);
		}
	}
}

static void ica_draw_subobj(int objnr)
{
	if (d_ica)
		subobj_wdraw(d_ica, objnr, 0, MAX_DEPTH);
}

int ica_get_target_obj(int x, int y, int whdl, OBJECT **tree,
			WORD *objnr, void (**set_icon)(int iconnr, int objnr),
			void (**drawfn)(int objnr))
{
	GRECT dummy;

	if (!d_ica)
		return FALSE;		/* Objekt ungültig */

	if (whdl != wdlg_get_handle(d_ica))
		return FALSE;

	wdlg_get_tree(d_ica, tree, &dummy);
	*objnr = objc_find(*tree, 0, 8, x, y);

	if ((pgmicnobj_to_showindex(*objnr) >= 0) ||
		 (daticnobj_to_showindex(*objnr) >= 0))
	{
		*set_icon = ica_set_icon;
		*drawfn = ica_draw_subobj;
		return TRUE;
	}

	return FALSE;
}


/*********************************************************************
*
* Deselektiert eine selektierte Datei.
*
*********************************************************************/

static void deselect_dfile(DIALOG *d)
{
	if (selected_datfile)
	{
		selected_datfile->sel = FALSE;
		selected_datfile = NULL;
		selected_datnr = -1;
		adr_ica_dialog[DEL_DAT].ob_state |= DISABLED;
		if (d)
			subobj_wdraw(d, DEL_DAT, DEL_DAT, 1);
	}
/*
	if (selected_datline)
	{
		selected_datline->sel = FALSE;
		selected_datline = NULL;
	}
	if (selected_spdfile)
	{
		selected_spdfile->selected = FALSE;
		selected_spdfile = NULL;
	}
*/
}


/*********************************************************************
*
* Deselektiert eine selektierte Applikation.
*
*********************************************************************/

static void deselect_program(DIALOG *d)
{
	enable_scroll(d, ctrl_objs1);
	disable_scroll(d, ctrl_objs2);
	if (selected_program)
	{
		selected_program->sel = FALSE;
		selected_pgmnr = -1;
		selected_program = NULL;
		adr_ica_dialog[DEL_PGM].ob_state |= DISABLED;
		adr_ica_dialog[NEU_DAT].ob_state |= DISABLED;
		subobj_wdraw(d, DEL_PGM, DEL_PGM, 1);
		subobj_wdraw(d, NEU_DAT, NEU_DAT, 1);
	}

	if (selected_datfile)
	{
		deselect_dfile(d);
		if (selected_datline)
		{
			selected_datline->selected = FALSE;
			selected_datline = NULL;
		}
		if (selected_curr_pgm_file_type)
		{
			selected_curr_pgm_file_type->selected = FALSE;
			selected_curr_pgm_file_type = NULL;
		}
	}
	if (d)
	{
		GRECT g;

		g = *((GRECT *) &adr_ica_dialog->ob_x);
/*
		g.g_x += xpos_textob;
		g.g_w = visible_pgmlen * gl_hwchar;
*/
		lbox_update(sbox1, &g);
	}
	setup_lbox_datafiles(-1);
}


/*********************************************************************
*
* Ein Programm ist angewählt worden. Das Aussehen des Dialogs
* muß entsprechend angepaßt werden.
*
*********************************************************************/

static void pgm_was_selected(DIALOG *d, int pgm, int do_sync)
{
	GRECT g;

	if (selected_datfile)
	{
		deselect_dfile(d);
		if (selected_datline)
		{
			selected_datline->selected = FALSE;
			selected_datline = NULL;
		}
		if (selected_curr_pgm_file_type)
		{
			selected_curr_pgm_file_type->selected = FALSE;
			selected_curr_pgm_file_type = NULL;
		}
	}

	selected_pgmnr = pgm;
	selected_program = pgmx+pgm;
	adr_ica_dialog[NEU_DAT].ob_state &= ~DISABLED;
	if (d)
		subobj_wdraw(d, NEU_DAT, NEU_DAT, 1);
	if (selected_program->name[0] != '<')
	{
		adr_ica_dialog[DEL_PGM].ob_state &= ~DISABLED;
		if (d)
			subobj_wdraw(d, DEL_PGM, DEL_PGM, 1);
	}

	selected_program->sel = TRUE;
	if (do_sync)
		sync_sel();
	disable_scroll(d, ctrl_objs1);

	if (d)
	{
		g = *((GRECT *) &adr_ica_dialog->ob_x);
/*
		g.g_x += xpos_textob;
		g.g_w = visible_pgmlen * gl_hwchar;
*/
	}

	lbox_update(sbox1, (d) ? &g : NULL);
	enable_scroll(d, ctrl_objs2);
	setup_spdfiles(0);
}


/*********************************************************************
*
* Eine Datei ist angewählt worden. Das Aussehen des Dialogs
* muß entsprechend angepaßt werden.
*
*********************************************************************/

static void dat_was_selected(DIALOG *d, int dat)
{
	if (selected_datfile)
		deselect_dfile(d);
	adr_ica_dialog[DEL_DAT].ob_state &= ~DISABLED;
	if (d)
		subobj_wdraw(d, DEL_DAT, DEL_DAT, 1);
	selected_datfile = datx + dat;
	selected_datnr = dat;
	selected_datfile->sel = TRUE;
}


/*********************************************************************
*
* ica_wdlg_cb: HNDL_INIT
*
*********************************************************************/

static int ica_wdlg_init(WORD nclicks, void *data, struct dialog_userdata *p_userdata)
{
	int retcode;


	if (d_ica)			/* Dialog ist schon geöffnet ! */
		return 0;		/* create verweigern */

	retcode = 1;			/* keine Aktion notwendig */

	if ((data) && (*((char **) data)))
	{
		p_userdata->cmd_pgm_path = ((char **) data)[0];
		p_userdata->cmd_dat_path = ((char **) data)[1];
		if (nclicks == 1)	/* Icon für Datei */
		{
			if (p_userdata->cmd_pgm_path[0] == '<')	/* nicht angem. */
				p_userdata->cmd_pgm_path = APP_NAME_FREE;
		}

		p_userdata->cmd_pgm = scroll_win_app(NULL, p_userdata->cmd_pgm_path);
		if (p_userdata->cmd_pgm >= 0)
		{
			pgm_was_selected(NULL, p_userdata->cmd_pgm, TRUE);
			if (p_userdata->cmd_dat_path)
				p_userdata->cmd_dat = scroll_win_dat(NULL, p_userdata->cmd_dat_path);
			else
				p_userdata->cmd_dat = -1;
			if (p_userdata->cmd_dat >= 0)
			{
				dat_was_selected(NULL, p_userdata->cmd_dat);
				sync_sel();
				lbox_update(sbox2, NULL);
			}
			sync_sel();
		}

		if (nclicks == 0)	/* Icon für Programm */
		{
			if (p_userdata->cmd_pgm == -1)	/* neues Programm */
				retcode = 2;		/* ... erst anmelden */
		}
		else
		if (nclicks == 1)	/* Icon für Datei */
		{
			if (p_userdata->cmd_dat == -1)	/* neue Datei */
				retcode = 2;		/* ... erst anmelden */
		}
		else
		if (nclicks == 2)			/* Anw. anmelden */
		{
			if (p_userdata->cmd_pgm != -2)
				retcode = 2;	/* Aktion notwendig ! */
		}
	}

	p_userdata->mode = retcode;
	return 1;
}


/*********************************************************************
*
* ica_wdlg_cb: HNDL_OPEN
*
*********************************************************************/

static int ica_wdlg_open(struct dialog_userdata *p_userdata)
{
	if (p_userdata->mode == 2)
	{
		p_userdata->mode = 1;
		if (p_userdata->cmd_dat_path)
		{
			def_txt = p_userdata->cmd_dat_path;
			if ((p_userdata->cmd_dat < 0) && (p_userdata->cmd_pgm >= 0))
			{
		 		d_typ = xy_wdlg_init(
		 				hdl_ftypes,
		 				adr_ftypes,
		 				IDENT_EDIT_FILE_TYPE,
		 				2,
		 				pgmx + p_userdata->cmd_pgm,
		 				STR_WINTITLE_MTY);
				if (!d_typ)
				{
					fehler:
					Rform_alert(1, ALRT_ERRWINDOPEN, NULL);
				}
			}
		}
		else
		{
			if (p_userdata->cmd_pgm >= 0)
			{
		 		d_anw = xy_wdlg_init(
		 				hdl_anwndg,
		 				adr_anwndg,
		 				IDENT_REGISTER_APP,
		 				0,
		 				pgmx + p_userdata->cmd_pgm,
		 				STR_WINTITLE_1AP);
				if (!d_anw)
					goto fehler;
			}
			else
			{
		 		d_anw = xy_wdlg_init(
		 				hdl_anwndg,
		 				adr_anwndg,
		 				IDENT_REGISTER_APP,
		 				1,
		 				p_userdata->cmd_pgm_path,
		 				STR_WINTITLE_1AP);
				if (!d_anw)
					goto fehler;
			}
		}
	}

	return 1;
}


/*********************************************************************
*
* Behandelt die Exit-Objekte des Icondialogs
* Das Exit-Objekt <objnr> wurde mit <clicks> Klicks angewählt.
*
* objnr = -1:	Initialisierung.
*			d->user_data und d->dialog_tree initialisieren!
*			<data> ist Zeiger auf Argumentzeiger,
*			d.h.		char *data[2]
*			<clicks> == 0:	Icon für Programm zuweisen
*					  1: Icon für Datei zuweisen
*					  2: Anwendung anmelden
*		-2:	Nachricht int data[8] wurde übergeben
* 		-3:	Fenster wurde durch Closebutton geschlossen.
*		-4:	Programm wurde beendet.
*		-5:	Initialisierung _NACH_ Öffnen des Fensters
*
* Rückgabe:	0	Dialog schließen
*			< 0	Fehlercode
*
*********************************************************************/


WORD cdecl ica_wdlg_cb(struct HNDL_OBJ_args args)
{
	DIALOG *d = args.dialog;
	short exitbutton = args.obj;
	short nclicks = args.clicks;
	void *data = args.data;
	int pgm, dat;
	OBJECT *tree;
	struct pgm_file *mypgm;
	struct dialog_userdata *p_userdata;


	/* 1. Fall: Dialog soll geöffnet werden */
	/* ------------------------------------ */

	tree = adr_ica_dialog;
	p_userdata = wdlg_get_udata(d);

	if (exitbutton == HNDL_INIT)
	{
		return(ica_wdlg_init(nclicks, data, p_userdata));
	}

	/* 1a. Fall: Dialog ist geöffnet worden */
	/* ------------------------------------ */

	if (exitbutton == HNDL_OPEN)
	{
		return(ica_wdlg_open(p_userdata));
	}

	/* 3. Fall: Dialog soll geschlossen werden */
	/* --------------------------------------- */

	if (exitbutton == HNDL_CLSD)
	{
		/* Wenn Dialog geschlossen werden soll... */
		close_dialog:
		d_ica = NULL;
		return 0;		/* ...dann schließen wir ihn auch */
	}

	if (exitbutton < 0)
		return 1;

	/* 4. Fall: Exitbutton wurde betätigt */
	/* ---------------------------------- */

		/* Doppelklick auf Programmnamen */
		/* ----------------------------- */

	pgm = pgmobj_to_showindex(exitbutton);
	if ((nclicks == 2) && (pgm >= 0))
	{
		pgm += lbox_get_first(sbox1);
		pgm = linx[pgm].pgmnr;
		if (pgm < 0 || pgmx[pgm].name[0] == '<')	/* freie Dateien */
		{
			Rform_alert(1, ALRT_IS_PSEUDO, NULL);
			return 1;
		}

 		d_anw = xy_wdlg_init(
 				hdl_anwndg,
 				adr_anwndg,
 				IDENT_REGISTER_APP,
 				0,
 				pgmx + pgm,
 				STR_WINTITLE_1AP);
		if (!d_anw)
			Rform_alert(1, ALRT_ERRWINDOPEN, NULL);
		return 1;
	}

		/* Doppelklick auf Dateinamen */
		/* -------------------------- */

	dat = datobj_to_showindex(exitbutton);
	if ((nclicks == 2) && (dat >= 0))
	{
		dat += lbox_get_first(sbox2);
		if (selected_program)
			dat = sel_dfiles[dat].datnr;
		else
			dat = linx[dat].datnr;

 		d_typ = xy_wdlg_init(
 				hdl_ftypes,
 				adr_ftypes,
 				IDENT_EDIT_FILE_TYPE,
 				1,
 				datx + dat,
 				STR_WINTITLE_MTY);
		if (!d_typ)
			Rform_alert(1, ALRT_ERRWINDOPEN, NULL);

		return 1;
	}


		/* Doppelklick auf Programm-Icon */
		/* ----------------------------- */

	if ((nclicks == 2) && (pgmicnobj_to_showindex(exitbutton) >= 0))
	{
		open_iconsel();
		return 1;
	}


		/* Doppelklick auf Datei-Icon */
		/* -------------------------- */

	if ((nclicks == 2) && (daticnobj_to_showindex(exitbutton) >= 0))
	{
		open_iconsel();
		return 1;
	}


	if (nclicks != 1)
		goto ende;


	/*
	 TODO:
	 Fehlfunktion bei:
	 	1. Dateityp auswählen, z.B. *.RSC
	 	2. Dann zugehöriges Programm auswählen, hier INTRFACE
	 	-> Klick wird ignoriert, zweiter Klick geht aber.
	*/


	/* Scrollbox #1 angewählt (Programme) */
	/* ---------------------------------- */

	if (exitbutton == PICON1)
		exitbutton = PRG1;
	if (exitbutton == PICON2)
		exitbutton = PRG2;
	if (exitbutton == PICON3)
		exitbutton = PRG3;
	if (exitbutton == PICON4)
		exitbutton = PRG4;
	if (exitbutton == PICON5)
		exitbutton = PRG5;

	if ((exitbutton == PN_UP) ||
		 (exitbutton == PN_DOWN) ||
		 (exitbutton == PN_BSL) ||
		 (exitbutton == PN_SLID) ||
		 (exitbutton == PRG1) ||
		 (exitbutton == PRG2) ||
		 (exitbutton == PRG3) ||
		 (exitbutton == PRG4) ||
		 (exitbutton == PRG5))
	{
		struct item_line *oldsel;

		handle_prg:
		oldsel = selected_pgmline;

		/*
		   Man beachte, daß, wenn links kein Programm ausgewählt wurde,
		   die Listboxen von Dateien und Programmen dieselbe
		   LBOX_ITEM-Tabelle verwenden, also auch dieselben <selected>-Zustände.
		*/
		lbox_do(sbox1, exitbutton);

		/* select-Status geändert ? */

		if (!selected_pgmline && oldsel)
		{
			deselect_program(d);
		}
		else
		if (selected_pgmline && !oldsel)
		{
			pgm_was_selected(d, selected_pgmline->pgmnr, FALSE);
		}
		return 1;
	}


	/* Scrollbox #2 angewählt (Dateitypen) */
	/* ----------------------------------- */

	if (exitbutton == DICON1)
		exitbutton = DAT1;
	if (exitbutton == DICON2)
		exitbutton = DAT2;
	if (exitbutton == DICON3)
		exitbutton = DAT3;
	if (exitbutton == DICON4)
		exitbutton = DAT4;
	if (exitbutton == DICON5)
		exitbutton = DAT5;

	if ((exitbutton == DN_UP) ||
		 (exitbutton == DN_DOWN) ||
		 (exitbutton == DN_BSL) ||
		 (exitbutton == DN_SLID) ||
		 (exitbutton == DAT1) ||
		 (exitbutton == DAT2) ||
		 (exitbutton == DAT3) ||
		 (exitbutton == DAT4) ||
		 (exitbutton == DAT5))
	{
		struct curr_pgm_file_type *oldsel = selected_curr_pgm_file_type;

		/* Man beachte, daß, solange links kein Programm ausgewählt ist,
		   die Listboxen von Dateien und Programmen dieselbe
		   LBOX_ITEM-Tabelle verwenden, also auch dieselben <selected>-Zustände. */
		lbox_do(sbox2, exitbutton);

		/* select-Status geändert ? */

		if (selected_program)
		{
			/*
			* In der linken Liste ist schon ein Programm selektiert.
			* Wir arbeiten ganz normal mit der rechten Liste.
			*/

			if (!selected_curr_pgm_file_type && oldsel)
			{
				deselect_dfile(d);
			}
			else
			if (selected_curr_pgm_file_type != oldsel)
			{
				dat_was_selected(d, selected_curr_pgm_file_type->datnr);
			}
		}
		else
		{
			/*
			* In der linken Liste ist kein Programm selektiert.
			* Linke und rechte Liste teilen sich die LBOX_ITEMs, wodurch auch links
			* eine Zeile als "selected" markiert wurde, aber nicht schwarz eingefärbt wurde.
			* Wenn man jetzt links auf das Programm klickt, ist der Status der Zeile
			* inkonsistent.
			* Deshalb löschen wir das "selected" und tun so, als ob das zugehörige
			* Programm in der linken Liste stattdessen angeklickt wurde.
			*/

			/* Der selektierte Index in linx[] */
			int sel_idx = lbox_get_slct_idx(sbox2);
			if (sel_idx >= 0)
			{
				/* Das zugehörige Programm */
				int sel_pgm = linx[sel_idx].pgmnr;
				/* selected-Status löschen, damit der nicht für die linke Liste gilt. */
				linx[sel_idx].selected = 0;
				/* Die erste Zeile mit demselben Programm suchen. Das ist die Zeile in der
				   linken Liste, in der das Programm steht */
				while ((sel_idx > 0) && (linx[sel_idx - 1].pgmnr == sel_pgm))
					sel_idx--;
				/* Linke Liste so scrollen, daß das selektierte Programm der oberste Eintrag ist */
				lbox_scroll_to(sbox1, sel_idx, NULL, NULL);
				/* Wir tun so, also ob der oberste Eintrag links gewählt wurde. */
				exitbutton = PRG1;
				goto handle_prg;
			}
			/*
			if (!selected_datline && oldsel)
			{
				deselect_dfile(d);
			}
			else
			if (selected_datline != oldsel)
			{
				dat_was_selected(d, selected_datline->datnr);
			}
			*/
		}
		return 1;
	}



	/* Neues Programm generieren */
	/* ------------------------- */

	if (exitbutton == NEU_PGM)
	{
 		d_anw = xy_wdlg_init(
 				hdl_anwndg,
 				adr_anwndg,
 				IDENT_REGISTER_APP,
 				0,
 				NULL,
 				STR_WINTITLE_1AP);
		if (!d_anw)
			Rform_alert(1, ALRT_ERRWINDOPEN, NULL);
		goto ende;
	}

		/* Programm löschen */
		/* ---------------- */

	if (exitbutton == DEL_PGM)
	{
		int sel;

		sel = selected_pgmnr;
		if (sel < 0)
			exit(-1);		/* ??? */
		deselect_program(d);
		if (selected_pgmline)
		{
			selected_pgmline->selected = FALSE;
			selected_pgmline = NULL;
		}
		delete_pgm(sel);
		goto ende;
	}

		/* Neue Datei generieren */
		/* --------------------- */

	if (exitbutton == NEU_DAT)
	{
		if (!selected_program)
			exit(-1);			/* interner Fehler */
		mypgm = selected_program;
		if ((mypgm->name[0] != '<') && (!mypgm->path[0]))
		{
			Rform_alert(1, ALRT_APPMUSTPATH, NULL);
			goto ende;
		}

 		d_typ = xy_wdlg_init(
 				hdl_ftypes,
 				adr_ftypes,
 				IDENT_EDIT_FILE_TYPE,
 				0,
 				mypgm,
 				STR_WINTITLE_MTY);
		if (!d_typ)
			Rform_alert(1, ALRT_ERRWINDOPEN, NULL);
		goto ende;
	}

		/* Dateityp löschen */
		/* ---------------- */

	if (exitbutton == DEL_DAT)
	{
		int sel;

		sel = selected_datnr;
		if (!selected_datfile)
			exit(-1);		/* ??? */
		deselect_dfile(d);
		if (selected_datline)
		{
			selected_datline->selected = FALSE;
			selected_datline = NULL;
		}
		if (selected_curr_pgm_file_type)
		{
			selected_curr_pgm_file_type->selected = FALSE;
			selected_curr_pgm_file_type = NULL;
		}
		delete_dat(sel);
		sync_sel();
		goto ende;
	}

		/* Applikation beendet */
		/* ------------------- */

	if (exitbutton == ICONS_OK)			/* OK */
	{
		save_dialog_xy(d);
		if (put_inf())
			goto ende;				/* Fehler bei INF */
		goto close_dialog;
	}

	if (exitbutton == ICONS_CN)			/* Abbruch */
	{
		save_dialog_xy(d);
		goto close_dialog;
	}

	return 1;

	ende:
	ob_dsel(tree, exitbutton);
	subobj_wdraw(d, exitbutton, exitbutton, 1);
	return 1;		/* weiter */
}
