/*
*
* Enthält die Routinen für die Editorfenster
*
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "GLOBALS.H"
#include "MGEDIT.H"

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
* öffnet ein neues Fenster.
* Wenn <path> == NULL ist, wird ein leeres Dokument erzeugt.
*
****************************************************************/

void close_window( WINDOW *w )
{
	WINDOW **pw;

	pw = find_slot_window(w);
	if	(pw)
		{
		edit_close(&w->tree, EDITFELD);
		edit_delete( w->xedit );
		Mfree(w->buf);
		wind_close(w->handle);
		wind_delete(w->handle);
		Mfree(w);
		*pw = NULL;
		}
}


/****************************************************************
*
* öffnet ein neues Fenster.
* Wenn <path> == NULL ist, wird ein leeres Dokument erzeugt.
*
****************************************************************/

WINDOW *open_new_window( char *path )
{
	WINDOW *w = NULL;
	WINDOW **pw;
	int file = -1;
	int success = 0;
	long ret = 0;
	struct stat st;
	int wnr;


	/* Der Fensterpfad ist auf 127 Zeichen plus Terminator begrenzt. */
	/* ------------------------------------------------------------ */

	if	(path && (strlen(path) >= sizeof(w->path)))
		{
		form_xerr(-ENAMETOOLONG, path);
		return(NULL);
		}

	pw = new_window();
	if	(!pw)
		{
		ret = -ENOMEM;
		goto fehler;
		}

	wnr = (int) (pw-windows);
	w = (WINDOW *) Malloc(sizeof(WINDOW));
	if	(!w)
		{
		ret = -ENOMEM;
		goto fehler;
		}

	w->handle = -1;
	w->buf = NULL;

	w->out.g_x = -1 + wnr * 8;
	w->out.g_y = scrg.g_y + wnr * 8;
	w->out.g_w = 640;
	w->out.g_h = scrg.g_h-16;

	w->handle = wind_create_grect(EDITOR_W_KIND, &scrg);
	if	(w->handle < 0)
		{
		ret = -ENOMEM;
		goto fehler;
		}

	/* Allgemeine Einstellungen */
	/* ------------------------ */

	w->dirty = 0;
	w->save_active = 0;
	w->flags = 0;
	w->fontID = prefs.fontID;
	w->fontH = prefs.fontH;
	w->fontprop = prefs.fontprop;
	w->tcolour = prefs.tcolour;
	w->bcolour = prefs.bcolour;
	w->tabwidth = prefs.tabwidth;

	/* ggf. Dateilänge ermitteln */
	/* ------------------------- */

	if	(path)
		{
		strcpy(w->path, path);
		strcpy(w->title, path);
		ret = Fopen(path, FO_READ);
		if	(ret < 0)
			goto fehler;

		file = (int) ret;

		if	(fstat(file, &st) < 0)
			{
			ret = -errno;
			goto fehler;
			}
		}
	else	{
		w->path[0] = '\0';
		strcpy(w->title, Rgetstring(STR_NONAME, NULL));
		sprintf(w->title+strlen(w->title), "%d", wnr+1);
		st.st_size = 0;
		}

	w->bufsize = (long) st.st_size + prefs.bufsize;
	w->buf = (char *) Malloc(w->bufsize);
	if	(!w->buf)
		{
		ret = -ENOMEM;
		goto fehler;
		}

	if	(file >= 0)
		{
		ret = Fread(file, (long) st.st_size, w->buf);
		Fclose(file);
		file = -1;
		if	(ret != (long) st.st_size)
			{
			ret = -EIO;
			goto fehler;
			}
		}

	w->buf[(size_t) st.st_size] = '\0';

	w->xedit = edit_create();
	if	(!w->xedit)
		{
		ret = -ENOMEM;
		goto fehler;
		}
	w->tree.ob_next = w->tree.ob_head = w->tree.ob_tail = -1;
	w->tree.ob_flags = OF_LASTOB;
	w->tree.ob_state = 0;
	w->tree.ob_type = G_EDIT;
	w->tree.ob_spec.index = (long) w->xedit;
	edit_set_buf( &w->tree, EDITFELD,	w->buf, w->bufsize );
	edit_set_font( &w->tree, EDITFELD,
				w->fontID, w->fontH, 0, !w->fontprop);
	edit_set_colour( &w->tree, EDITFELD,
				w->tcolour, w->bcolour);
	edit_set_format( &w->tree, EDITFELD,
				w->tabwidth, 0);
	wind_set_str(w->handle, WF_NAME, w->title);
	success = 1;


	fehler:
	if	(!success)
		{
		if	(w)
			{
			if	(w->handle >= 0)
				wind_delete(w->handle);
			if	(w->buf)
				Mfree(w->buf);
			Mfree(w);
			}
		if	(file >= 0)
			Fclose(file);
		form_xerr(ret, path);
		}
	else	{
		*pw = w;
		wind_open_grect(w->handle, &(w->out));
		wind_get_grect(w->handle, WF_WORKXYWH, &(w->in));
		w->tree.ob_x = w->in.g_x;
		w->tree.ob_y = w->in.g_y;
		w->tree.ob_width = w->in.g_w;
		w->tree.ob_height = w->in.g_h;
		edit_open( &w->tree, EDITFELD );
		w->nlines = w->yscroll = w->xscroll = w->ncols = -1;
		window_set_slider(w);
		}
	return(w);
}


/****************************************************************
*
* Zeichnet das Fenster <w> neu.
*
****************************************************************/

int update_window(WINDOW *w, GRECT *g)
{
	WMESAG mesag;

	/* Der Redraw wird nicht direkt ausgeführt, sondern schickt */
	/* eine Nachricht an die Applikation, um Kollisionen mit    */
	/* von AES erzeugten Redraws zu vermeiden                   */
	/* -------------------------------------------------------- */

	mesag.msg.code		= WM_REDRAW;
	mesag.msg.dest_apid	= gl_apid;
	mesag.msg.is_zero	= 0;
	mesag.msg.whdl		= w->handle;
	mesag.msg.g		= (g) ? *g : w->out;
	return(appl_write(gl_apid, 16, mesag.message));
}


/****************************************************************
*
* Fenster <w> in Größe und/oder Position ändern
*
****************************************************************/

static void change_g(WINDOW *w, GRECT *g, int resized)
{
	short oldrh,newrh;
	GRECT rg;

	if	(w->flags & WFLAG_ICONIFIED)
		return;
	w->out = *g;
	wind_set_grect(w->handle, WF_CURRXYWH, &(w->out));
	wind_get_grect(w->handle, WF_WORKXYWH, &(w->in));
	w->tree.ob_x = w->in.g_x;
	w->tree.ob_y = w->in.g_y;
	w->tree.ob_width = w->in.g_w;
	w->tree.ob_height = w->in.g_h;
	if	(resized)
		{
		edit_resized(&w->tree, EDITFELD, &oldrh, &newrh);

		/* weißen Rand nachbessern */
		/* ----------------------- */

		if	(newrh != oldrh)
			{
			if	(newrh < oldrh)
				oldrh = newrh;
			rg.g_x = w->tree.ob_x;
			rg.g_y = w->tree.ob_y + oldrh;
			rg.g_w = w->tree.ob_width;
			rg.g_h = w->tree.ob_height - oldrh;
			if	((rg.g_h > 0) && (rg.g_w > 0))
				{
				objc_wdraw(&w->tree,0,0, &rg,	w->handle);
				/*
				update_window(w, &rg);
				*/
				}
			}
		window_set_slider(w);
		}
}


/****************************************************************
*
* Das Fenster <w> wird auf Maximalgröße gebracht.
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
	wind_get_grect(w->handle, WF_PREVXYWH, &prev);
	wind_get_grect(w->handle, WF_FULLXYWH, &full);
 
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

	change_g(w, newg, 1);
}


/****************************************************************
*
* Fensternachricht verarbeiten
*
****************************************************************/

void window_message(WINDOW *w, int kstate, short message[8])
{
	EVNT w_ev;
	int key = 0;
	long line;
	long ldummy;
	short dummy;
	short yvis,xvis;
	GRECT g;
	long h_offs;
	long err;


	(void) kstate;

	switch(message[0])
		{
		case WM_REDRAW:
			g.g_x = message[4];
			g.g_y = message[5];
			g.g_w = message[6];
			g.g_h = message[7];
			objc_wdraw(&w->tree,0,0, &g,
					w->handle);
			break;
		case WM_TOPPED:
			wind_set_int(w->handle, WF_TOP, 0);
			break;
		case WM_CLOSED:
			if	(w->save_active)
				Bconout(2,7);
			else	close_file(w);
			break;
		case WM_ALLICONIFY:
			break;
		case WM_ICONIFY:
			break;
		case WM_UNICONIFY:
			break;
		case WM_FULLED:
			fulled(w);
			break;
		case WM_ARROWED:

			switch(message[4])
				{
				case WA_LFLINE:
					h_offs = -8;
					goto hscroll;
				case WA_RTLINE:
					h_offs = 8;
					goto hscroll;
				case WA_LFPAGE:
					h_offs = -w->xvis;
					goto hscroll;
				case WA_RTPAGE:
					h_offs = w->xvis;
					hscroll:
					h_offs += w->xscroll;
					if	(h_offs >= w->ncols)
						h_offs = w->ncols - 1;
					if	(h_offs < 0)
						h_offs = 0;
					if	(w->xscroll != h_offs)
						{
						if	(edit_scroll( &w->tree, EDITFELD, w->handle, -1L, (short) h_offs ))
							window_set_slider(w);
						}
					break;

				case WA_UPLINE:
					key = 0x4800;
					w_ev.kstate = K_CTRL;
					break;
				case WA_DNLINE:
					key = 0x5000;
					w_ev.kstate = K_CTRL;
					break;
				case WA_UPPAGE:
					key = 0x4900;
					w_ev.kstate = K_LSHIFT;
					break;
				case WA_DNPAGE:
					key = 0x5100;
					w_ev.kstate = K_RSHIFT;
					break;
				default: key = 0;
					break;
				}

			if	(key)
				{
				w_ev.mwhich = MU_KEYBD;
				w_ev.key = key;
				edit_evnt( &w->tree, EDITFELD, w->handle, &w_ev, &err );
				window_set_slider(w);
				}
			break;

		case WM_HSLID:
			edit_get_scrollinfo(&w->tree, EDITFELD,
				&ldummy, &ldummy, &dummy, &dummy,
				&dummy, &dummy, &xvis);

			line = (message[4] * (w->ncols - xvis + 1L))/1000L;
			if	(edit_scroll( &w->tree, EDITFELD, w->handle, -1L, (short) line ))
				window_set_slider(w);
			break;

		case WM_VSLID:

			edit_get_scrollinfo(&w->tree, EDITFELD,
				&ldummy, &ldummy, &yvis, &dummy,
				&dummy, &dummy, &dummy);

			line = (message[4] * (w->nlines - yvis + 1L))/1000L;
			if	(edit_scroll( &w->tree, EDITFELD, w->handle, line, (short) -1 ))
				window_set_slider(w);
			break;
		case WM_SIZED:
			g.g_x = message[4];
			g.g_y = message[5];
			g.g_w = message[6];
			g.g_h = message[7];
			change_g(w, &g, 1);
			break;
		case WM_MOVED:
			g.g_x = message[4];
			g.g_y = message[5];
			g.g_w = message[6];
			g.g_h = message[7];
			change_g(w, &g, 0);
			break;
		case WM_SHADED:
			w->flags |= WFLAG_SHADED;
			break;
		case WM_UNSHADED:
			w->flags &= ~WFLAG_SHADED;
			break;
		}
}


/****************************************************************
*
* Fenster-Slider ggf. ändern
*
****************************************************************/

void window_set_slider(WINDOW *w)
{
	long nlines,yscroll;
	short ncols,xscroll;
	short yvis,yval,xvis;
	long size;


	edit_get_scrollinfo(&w->tree, EDITFELD,
				&nlines, &yscroll, &yvis, &yval,
				&ncols, &xscroll, &xvis);

	if	(ncols < 0)
		ncols = 256*8;			/* Textbreite erraten */

	if	((w->nlines != nlines) || (w->yvis != yvis))
		{
		w->nlines = nlines;
		w->yvis = yvis;
		if	(nlines)
			{
			size = yval;
			size *= 1000;
			size /= nlines;
			}
		else	size = 1000;
		wind_set_int(w->handle, WF_VSLSIZE, (short) size);
		}
	if	(w->yscroll != yscroll)
		{
		w->yscroll = yscroll;
		if	(nlines)
			{
			nlines -= yval;
			yscroll *= 1000;
			yscroll /= nlines;
			}
		else	yscroll = 1;
		wind_set_int(w->handle, WF_VSLIDE, (short) yscroll);
		}
	if	((w->ncols != ncols) || (w->xvis != xvis))
		{
		w->ncols = ncols;
		w->xvis = xvis;
		if	(ncols)
			{
			size = xvis;
			size *= 1000;
			size /= ncols;
			}
		else	size = 1000;
		wind_set_int(w->handle, WF_HSLSIZE, (short) size);
		}
	if	(w->xscroll != xscroll)
		{
		w->xscroll = xscroll;
		if	(ncols)
			{
			ncols -= xvis;
			size = xscroll;
			size *= 1000;
			size /= ncols;
			}
		else	size = 1;
		wind_set_int(w->handle, WF_HSLIDE, (short) size);
		}
}
