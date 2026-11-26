/*
*
* Enthält die allgemeinen Routinen für die
* Fensterverwaltung
*
*/

#include <stdlib.h>
#include <stddef.h>
#include "MGNOTICE_GEM.H"
#include "WINDOWS.H"

#ifndef WM_ICONIFY
#define WM_ICONIFY       34                                 /* AES 4.1     */
#define WM_UNICONIFY     35                                 /* AES 4.1     */
#define WM_ALLICONIFY    36                                 /* AES 4.1     */
#endif

extern WINDOW *windows[NWINDOWS];
extern GRECT scrg;


/****************************************************************
*
* Rechnet Fensterhandle in WINDOW um.
*
****************************************************************/

WINDOW *whdl2window(int whdl)
{
	register int i;
	register WINDOW **w;


	for	(i = 0,w = windows; i < NWINDOWS; i++,w++)
		{
		if	(!*w)
			continue;		/* leerer Slot */
		if	((*w)->handle == whdl)
			return(*w);
		}
	return(NULL);
}


/****************************************************************
*
* ermittelt unbenutzten WINDOW-Slot.
*
****************************************************************/

WINDOW **new_window( void )
{
	register int i;
	register WINDOW **w;


	for	(i = 0,w = windows; i < NWINDOWS; i++,w++)
		{
		if	(!*w)
			return(w);		/* leerer Slot */
		}
	return(NULL);
}


/****************************************************************
*
* ermittelt einen WINDOW-Slot
*
****************************************************************/

WINDOW **find_slot_window( WINDOW *myw )
{
	register int i;
	register WINDOW **w;


	for	(i = 0,w = windows; i < NWINDOWS; i++,w++)
		{
		if	(*w == myw)
			return(w);
		}
	return(NULL);
}


/****************************************************************
*
* Zeichnet das Fenster <wnr> neu.
*
****************************************************************/

int update_window(WINDOW *w)
{
	WMESAG mesag;

	/* Der Redraw wird nicht direkt ausgeführt, sondern schickt */
	/* eine Nachricht an die Applikation, um Kollisionen mit    */
	/* von AES erzeugten Redraws zu vermeiden                   */
	/* -------------------------------------------------------- */

	mesag.msg.code		= WM_REDRAW;
	mesag.msg.dest_apid	= ap_id;
	mesag.msg.is_zero	= 0;
	mesag.msg.whdl		= w->handle;
	mesag.msg.g		= w->out;
	return(appl_write(ap_id, 16, mesag.message));
}


/****************************************************************
*
* Das Fenster mit Nummer <wnr> wird auf Maximalgröße gebracht.
*
****************************************************************/

static void fulled(WINDOW *w)
{
	GRECT   prev, full;
	register GRECT *out;
	GRECT *newg;


	if	(w->flags & WFLAG_ICONIFIED)
		return;
	out = &(w->out);				/* out = Außenmaße */
	wind_get(w->handle, WF_PREVXYWH, &prev.g_x, &prev.g_y, &prev.g_w, &prev.g_h);
	wind_get(w->handle, WF_FULLXYWH, &full.g_x, &full.g_y, &full.g_w, &full.g_h);
 
	/* 1. Fall: Das Fenster hat bereits Maximalgröße	  */
	/*		  Also muß das Fenster verkleinert werden */
	/* ------------------------------------------------ */

	if	((out->g_x == full.g_x) && (out->g_y == full.g_y) &&
		 (out->g_w == full.g_w) && (out->g_h == full.g_h)) {
		graf_shrinkbox_grect(&prev, &full);
		newg = &prev;
		}

	/* 2. Fall: Das Fenster hat nicht Maximalgröße	  */
	/*		  Also Größe auf Maximum 			  */
	/* ------------------------------------------------ */

	else {
		graf_growbox_grect(out, &full);
		newg = &full;
		}

	w->sized(w, newg);
}


static void _message_obj_window(WINDOW *w, short kstate, short message[8])
{
	switch(message[0])
		{
		case WM_REDRAW:
			objc_wdraw(&w->tree,0,0,(GRECT *) (message+4),
					w->handle);
			break;
		case WM_TOPPED:
			wind_set(w->handle, WF_TOP, 0, 0, 0, 0);
			break;
		case WM_CLOSED:
			w->close(w, kstate);
			break;
		case WM_ALLICONIFY:
			w->alliconified(w, (GRECT *) (message+4), TRUE);
			break;
		case WM_ICONIFY:
			w->iconified(w, (GRECT *) (message+4));
			break;
		case WM_UNICONIFY:
			w->uniconified(w, (GRECT *) (message+4), FALSE);
			break;
		case WM_FULLED:
			w->fulled(w);
			break;
/*
		case WM_ARROWED:
			w->arrowed(w, message[4]);
			break;
		case WM_HSLID:
			w->hslid(w, message[4]);
			break;
		case WM_VSLID:
			w->vslid(w, message[4]);
			break;
*/
		case WM_SIZED:
			w->sized(w, (GRECT *) (message+4));
			break;
		case WM_MOVED:
			w->moved(w, (GRECT *) (message+4));
			break;
		}
}


#pragma warn -par
static void iconified(WINDOW *w, GRECT *g)
{
}
static void uniconified(WINDOW *w, GRECT *g, int unhide)
{
}
static void alliconified(WINDOW *w, GRECT *g, int hide)
{
}
static void closed(WINDOW *w, int kstate)
{
}
static void buttoned( WINDOW *w, int nclicks, EVNTDATA *ev )
{
}
static void keyed( WINDOW *w, short kstate, short key )
{
}
static void draw (WINDOW *w, GRECT *g)
{
}
static void snap (WINDOW *w)
{
}
static int arrange (WINDOW *w)
{
	return(0);
}
#pragma warn +par

static void opened(WINDOW *w)
{
	wind_open_grect(w->handle, &(w->out));
	wind_get_grect(w->handle, WF_WORKXYWH, &(w->in));
	w->arrange(w);
}
static void sized(WINDOW *w, GRECT *g)
{
	if	(w->flags & WFLAG_ICONIFIED)
		return;
	w->out = *g;
	wind_set_grect(w->handle, WF_CURRXYWH, &(w->out));
	wind_get_grect(w->handle, WF_WORKXYWH, &(w->in));
	if	(w->arrange(w))
		update_window(w);
}

void	init_window( WINDOW *w)
{
	w->close = closed;
	w->arrange = arrange;
	w->open = opened;
	w->message = _message_obj_window;
	w->moved = NULL;
	w->fulled = fulled;
	w->sized = sized;

	w->iconified = iconified;
	w->uniconified = uniconified;
	w->alliconified = alliconified;
	w->key = keyed;
	w->button = buttoned;
	w->flags = 0;
}
