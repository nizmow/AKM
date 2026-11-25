/*
*
* Enthält die spezifischen Routinen für den Dialog
* "Voreinstellungen"
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GLOBALS.H"
#include "MGEDIT.H"

static struct prefs localprefs;

/*********************************************************************
*
* Schreibt eine Zahl in ein TEDINFO-Textfeld. Die tatsächliche
* Puffergröße steht in te_txtlen (inklusive abschließendem NUL).
*
*********************************************************************/

static void options_set_number(OBJECT *tree, WORD object, short value)
{
	TEDINFO *ted;

	ted = tree[object].ob_spec.tedinfo;
	if	(ted->te_txtlen > 0)
		{
		snprintf(ted->te_ptext, (size_t) ted->te_txtlen,
			"%d", (int) value);
		}
}


/*********************************************************************
*
* Initialisierung
*
*********************************************************************/

void options_dial_init_rsc( void )
{
}


/*********************************************************************
*
* Behandelt die Exit- Objekte des Dialogs
* Beim Initialisieren wird in <data> ggf. ein Zeiger auf einen
* Text übergeben.
*
*********************************************************************/

WORD	__CDECL hdl_options( struct HNDL_OBJ_args args )
{
	DIALOG *d;
	EVNT *events;
	WORD exitbutton;
	WORD clicks;
	void *data;
	OBJECT *tree;
	int obj;
	long id,pt;
	short mono;



	d = args.dialog;
	events = args.events;
	exitbutton = args.obj;
	clicks = args.clicks;
	data = args.data;
	(void) data;

	/* 1. Fall: Dialog soll geöffnet werden */
	/* ------------------------------------ */

	tree = adr_options;

	if	(exitbutton == HNDL_INIT)
		{
		if	(d_options)			/* Dialog ist schon geöffnet ! */
			return(0);

		localprefs = prefs;
		options_set_number(tree, OPTIONS_FONTSIZE, localprefs.fontH);
		tree[OPTIONS_FONTNAME].ob_spec.tedinfo->te_ptext =
			localprefs.fontname;
		tree[OPTIONS_TCOLOUR].ob_spec.tedinfo->te_ptext
				= adr_colour[localprefs.tcolour+1].ob_spec.tedinfo->te_ptext;
		tree[OPTIONS_TCOLOUR].ob_spec.tedinfo->te_color
				= adr_colour[localprefs.tcolour+1].ob_spec.tedinfo->te_color;
		tree[OPTIONS_BCOLOUR].ob_spec.tedinfo->te_ptext
				= adr_colour[localprefs.bcolour+1].ob_spec.tedinfo->te_ptext;
		tree[OPTIONS_BCOLOUR].ob_spec.tedinfo->te_color
				= adr_colour[localprefs.bcolour+1].ob_spec.tedinfo->te_color;
		options_set_number(tree, OPTIONS_TABWIDTH, localprefs.tabwidth);
		ob_dsel(tree, OPTIONS_OK);
		ob_dsel(tree, OPTIONS_CANCEL);

		return(1);
		}

	/* 2. Fall: Nachricht mit Code >= 1040 empfangen */
	/* --------------------------------------------- */

	if	(exitbutton == HNDL_MESG)	/* Wenn Nachricht empfangen... */
		{
		switch(events->msg[0])
			{
			/* Für diesen Dialog sind keine Sondernachrichten
			 * zu behandeln. */
			}
		return(1);		/* weiter */
		}

	/* 3. Fall: Dialog soll geschlossen werden */
	/* --------------------------------------- */

	if	(exitbutton == HNDL_CLSD)	/* Wenn Dialog geschlossen werden soll... */
		return(0);		/* ...dann schließen wir ihn auch */

	if	(exitbutton < 0)	/* unbekannte Unterfunktion */
		return(1);

	/* 4. Fall: Exitbutton wurde betätigt */
	/* ---------------------------------- */

	if	(clicks != 1)
		goto ende;

	if	(exitbutton == OPTIONS_TCOLOUR)
		{
		adr_colour->ob_x = tree->ob_x+tree[exitbutton].ob_x;
		adr_colour->ob_y = tree->ob_y+tree[exitbutton].ob_y;
		adr_colour->ob_y -= adr_colour[localprefs.tcolour+1].ob_y;
		obj = form_popup(adr_colour,0,0);
		if	(obj > 0)
			{
			tree[exitbutton].ob_spec.tedinfo->te_ptext
				= adr_colour[obj].ob_spec.tedinfo->te_ptext;
			tree[exitbutton].ob_spec.tedinfo->te_color
				= adr_colour[obj].ob_spec.tedinfo->te_color;
			localprefs.tcolour = obj-1;
			goto ende;
			}

		}

	if	(exitbutton == OPTIONS_BCOLOUR)
		{
		adr_colour->ob_x = tree->ob_x+tree[exitbutton].ob_x;
		adr_colour->ob_y = tree->ob_y+tree[exitbutton].ob_y;
		adr_colour->ob_y -= adr_colour[localprefs.bcolour+1].ob_y;
		obj = form_popup(adr_colour,0,0);
		if	(obj > 0)
			{
			tree[exitbutton].ob_spec.tedinfo->te_ptext
				= adr_colour[obj].ob_spec.tedinfo->te_ptext;
			tree[exitbutton].ob_spec.tedinfo->te_color
				= adr_colour[obj].ob_spec.tedinfo->te_color;
			localprefs.bcolour = obj-1;
			goto ende;
			}

		}

	if	((exitbutton == OPTIONS_FONTNAME) ||
		 (exitbutton == OPTIONS_FONTSIZE))
		{
		id = localprefs.fontID;
		pt = (((long) localprefs.fontH)<<16L);
		mono = !localprefs.fontprop;
		if	(dial_font( &id, &pt, &mono, localprefs.fontname ))
			{
			localprefs.fontID = (short) id;
			localprefs.fontH = (short) (pt >> 16L);
			localprefs.fontprop = !mono;
			options_set_number(tree, OPTIONS_FONTSIZE,
				localprefs.fontH);
			subobj_wdraw(d, OPTIONS_FONTNAME, OPTIONS_FONTNAME, 0);
			subobj_wdraw(d, OPTIONS_FONTSIZE, OPTIONS_FONTSIZE, 0);
			}
		}

	if	(exitbutton == OPTIONS_CANCEL)		/* Abbruch */
		{
		return(0);
		}

	localprefs.tabwidth = (short) atoi(
			tree[OPTIONS_TABWIDTH].ob_spec.tedinfo->te_ptext);

	if	(exitbutton == OPTIONS_SAVE)
		{
		prefs = localprefs;
		save_options();
		goto ende;
		}

	if	(exitbutton == OPTIONS_OK)			/* OK */
		{
		if	(memcmp( &prefs, &localprefs, sizeof(prefs)))
			{
			prefs = localprefs;
			prefs_were_changed();
			}
		return(0);
		}

	return(1);

	ende:
	ob_dsel(tree, exitbutton);
	subobj_wdraw(d, exitbutton, exitbutton, 0);
	return(1);		/* weiter */
}
