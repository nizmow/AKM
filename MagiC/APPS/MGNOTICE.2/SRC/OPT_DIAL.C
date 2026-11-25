/*
*
* Enthält die spezifischen Routinen für den Dialog
* "Voreinstellungen"
*
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "MGNOTICE_GEM.H"
#include "WINDOWS.H"
#include "GLOBALS.H"
#include "MGNOTICE.H"

static struct prefs localprefs;

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

#pragma warn -par
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
		sprintf(tree[OPTIONS_FONTSIZE].ob_spec.tedinfo->te_ptext,
			"%d", localprefs.fontH);
		tree[OPTIONS_FONTNAME].ob_spec.tedinfo->te_ptext =
			localprefs.fontname;
		tree[OPTIONS_COLOUR].ob_spec.tedinfo->te_ptext
				= adr_colour[localprefs.colour+1].ob_spec.tedinfo->te_ptext;
		tree[OPTIONS_COLOUR].ob_spec.tedinfo->te_color
				= adr_colour[localprefs.colour+1].ob_spec.tedinfo->te_color;
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
/*
			 case WM_ALLICONIFY:
	
			 case WM_ICONIFY:
			 	wind_update(BEG_UPDATE);
			 	wdlg_set_iconify(d, (GRECT *) (events->msg+4),
	 							" MGCOPY ",
	 							adr_beg_iconified, 1);
			 	is_iconified = TRUE;
			 	wind_update(END_UPDATE);
			 	break;
	
			 case WM_UNICONIFY:
			 	wind_update(BEG_UPDATE);
			 	wdlg_set_uniconify(d, (GRECT *) (events->msg+4),
		 							Rgetstring(STR_MAINTITLE, global),
		 							adr_beg);
			 	is_iconified = FALSE;
			 	wind_update(END_UPDATE);
				break;
*/	
			}
		return(1);		/* weiter */
		}

	/* 3. Fall: Dialog soll geschlossen werden */
	/* --------------------------------------- */

	if	(exitbutton == HNDL_CLSD)	/* Wenn Dialog geschlossen werden soll... */
		{
		close_dialog:
		return(0);		/* ...dann schließen wir ihn auch */
		}

	if	(exitbutton < 0)	/* unbekannte Unterfunktion */
		return(1);

	/* 4. Fall: Exitbutton wurde betätigt */
	/* ---------------------------------- */

	if	(clicks != 1)
		goto ende;

	if	(exitbutton == OPTIONS_COLOUR)
		{
		adr_colour->ob_x = tree->ob_x+tree[exitbutton].ob_x;
		adr_colour->ob_y = tree->ob_y+tree[exitbutton].ob_y;
		adr_colour->ob_y -= adr_colour[localprefs.colour+1].ob_y;
		obj = form_popup(adr_colour,0,0);
		if	(obj > 0)
			{
			tree[exitbutton].ob_spec.tedinfo->te_ptext
				= adr_colour[obj].ob_spec.tedinfo->te_ptext;
			tree[exitbutton].ob_spec.tedinfo->te_color
				= adr_colour[obj].ob_spec.tedinfo->te_color;
			localprefs.colour = (short) (obj-1);
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
			sprintf(tree[OPTIONS_FONTSIZE].ob_spec.tedinfo->te_ptext,
				"%d", localprefs.fontH);
			subobj_wdraw(d, OPTIONS_FONTNAME, OPTIONS_FONTNAME, 0);
			subobj_wdraw(d, OPTIONS_FONTSIZE, OPTIONS_FONTSIZE, 0);
			}
		}

	if	(exitbutton == OPTIONS_SAVE)
		{
		prefs = localprefs;
		save_options();
		goto ende;
		}

	if	(exitbutton == OPTIONS_CANCEL)		/* Abbruch */
		{
		goto close_dialog;
		}

	if	(exitbutton == OPTIONS_OK)			/* OK */
		{
		prefs = localprefs;
		return(0);
		}

	return(1);

	ende:
	ob_dsel(tree, exitbutton);
	subobj_wdraw(d, exitbutton, exitbutton, 0);
	return(1);		/* weiter */
}
#pragma warn +par
