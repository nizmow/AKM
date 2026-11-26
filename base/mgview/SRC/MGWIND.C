/*
*
* Dieses Modul enthält die Fenster-Routinen
* Für ein Textfenster
*
*/

#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include "GLOBALS.H"
#include "WINDOWS.H"
#include "MGWIND.H"



#define	MAXWIDTH	272

#define	MY_KIND	NAME+CLOSER+FULLER+MOVER+	\
				SIZER+UPARROW+DNARROW+VSLIDE+		\
				LFARROW+RTARROW+HSLIDE


/*****************************************************************
*
* Expandiert eine Zeile mit Tabulatoren. Die Zeilenlänge
* im Text ist <len>, ohne crlf.
* Die neue Zeilenlänge wird zurückgegeben.
* Ist die Zeile zu breit, wird abgebrochen.
* Ist <tabsize> = 0, werden Tabs wörtlich angezeigt (als Kuller)
* Ist <buf> = NULL, wird nix kopiert.
*
*****************************************************************/

static long expand_line( unsigned char *text, long len, int tabsize,
					unsigned char *buf, long bufsize )
{
	unsigned char *end = text+len;
	long newlen;
	int ntabs;


	newlen = 0L;
	while((text < end) && (bufsize))
		{
			/* Tab expandieren */
		if	((*text == '\t') && (tabsize))
			{
			ntabs = (int) (tabsize - (newlen % tabsize));
			if	(!ntabs)
				ntabs += tabsize;
			while(ntabs && bufsize)
				{
				if	(buf)
					{
					*buf++ = ' ';
					bufsize--;
					}
				newlen++;
				ntabs--;
				}
			}
			/* normal: ein Zeichen kopieren */
		else	{
			if	(buf)
				{
				if	(*text)
					*buf++ = *text;
				else	*buf++ = ' ';		/* Nullbyte! */
				bufsize--;
				}
			newlen++;
			}
		text++;
		}
	return(newlen);
}


/*****************************************************************
*
* Ermittelt die Anzahl der Zeilen und die maximale
* Spaltenbreite in einem Text.
* Dabei wird nur '\n' berücksichtigt, daß Zeilen mit '\n' oder
* mit '\r' abgeschlossen werden können damit man mit UNIX-
* und Mac- Textdateien keine Probleme hat.
* D.h. Zeilen können beendet werden durch:
*
*	$d$a
*	$a
*	$d
*	EOF
*
*****************************************************************/

static void get_line_column_count( unsigned char *text, long len,
				long *lc, long *cc, int tabsize )
{
	unsigned char *ende = text+len;
	unsigned char *eol,*eol2;
	long w;			/* Länge einer Zeile */


	*lc = *cc = 0;
	while(text < ende)
		{
		(*lc)++;		/* hier beginnt eine Zeile! */

		/* eol markiert das erste Zeichen hinter der Zeile */
		/* Das Zeilenende kann CR,LF oder CRLF sein. eol2  */
		/* markiert das erste Zeichen der nächsten Zeile   */
		/* ----------------------------------------------- */

		eol = memchr2(text, ende-text);
		if	(eol)
			{
			eol2 = eol+1;
			if ((*eol == '\r') && (eol2 < ende) && (*eol2 == '\n'))
				eol2++;
			}

		/* Letzte Zeile: */

		else	eol = eol2 = ende;

		w = expand_line( text, eol-text, tabsize,
						NULL, 1L );
		if	(w > MAXWIDTH)
			w = MAXWIDTH;

		if	(w > *cc)
			*cc = w;
		text = eol2;		/* aufs nächste Zeichen */
		}
}


/*****************************************************************
*
* Initialisiert das Feld w->user_lines[], das die Zeilenanfänge
* definiert.
*
*****************************************************************/

static void init_line_ptrs( unsigned char *text, long len,
					unsigned char **lines )
{
	unsigned char *ende = text+len;
	unsigned char *eol;


	while(text < ende)
		{
		*lines++ = text;		/* hier beginnt eine Zeile! */
		eol = memchr2(text, ende-text);

		/* Letzte Zeile: */

		if	(!eol)
			break;

		text = eol+1;
		if ((*eol == '\r') && (text < ende) && (*text == '\n'))
			text++;
		}
}


/*****************************************************************
*
* Ermittelt einen Zeiger auf die <n>-te Zeile des Textes.
* Gibt die Länge dieser Zeile (ohne CRLF bzw. LF) zurück.
*
*****************************************************************/

static unsigned char *get_line( WINDOW *w, long n, long *lw )
{
	unsigned char *line;
	unsigned char *ende;
	unsigned char *eol;


	if	(n >= w->vscroll.n)
		{
		*lw = 0;
		return(NULL);
		}

	ende = w->user_file + w->user_fsize;
	line = w->user_lines[n];
	eol = memchr2(line, ende-line);

	/* Letzte Zeile: */

	if	(!eol)
		eol = ende;

	*lw = eol - line;
	return(line);
}


/****************************************************************
*
* Ändert die Objektkoordinaten von w->out so, daß sie in
* bestimmte Raster einrasten.
* Hier:	Innen-GRECT muß ganzzahliges Vielfaches der Zeichengröße
*		sein.
*
****************************************************************/

static void _my_snap( WINDOW *w )
{
	/* Innenbereich berechnen */
	wind_calc_grect(WC_WORK, MY_KIND, &(w->out), &(w->in));
	/* x-Position auf 8er-Grenze */
	w->in.g_x &= ~7;
	/* Minimalgröße */
	if	(w->in.g_w < 8*text_attrib[8])
		w->in.g_w = 8*text_attrib[8];
	if	(w->in.g_h < 3*text_attrib[9])
		w->in.g_h = 3*text_attrib[9];
	/* Breite und Höhe */
	w->in.g_w -= w->in.g_w % w->hscroll.pixelsize;
	w->in.g_h -= w->in.g_h % w->vscroll.pixelsize;
	/* nochmal Außenbereich berechnen */
	wind_calc_grect(WC_BORDER, MY_KIND, &(w->in), &(w->out));
}


/****************************************************************
*
* Organisiert das Fenster neu, d.h. legt Position der Objekte fest.
* gibt TRUE zurück, falls sich fenster.cols oder fenster.shift
* geändert haben.
*
****************************************************************/

static int _my_arrange( WINDOW *w )
{
	long old_hshift,old_vshift;


	old_hshift 	= w->hscroll.shift;
	old_vshift 	= w->vscroll.shift;

	w->hscroll.nvis = w->in.g_w / (w->hscroll.pixelsize);
	w->vscroll.nvis = w->in.g_h / (w->vscroll.pixelsize);

	if	(w->vscroll.n >= 0)
		window_calc_slider(w, 0);
	if	(w->hscroll.n >= 0)
		window_calc_slider(w, 1);

	return((old_vshift != w->vscroll.shift) ||
		  (old_hshift != w->hscroll.shift));
}


/****************************************************************
*
* Routine zum Zeichnen eines Textfensters.
*
****************************************************************/

static void _my_draw(WINDOW *w, GRECT *g)
{
	unsigned char *line;
	int i;
	long l;
	long lw;
	int x,y;
	short pxy[4];
	unsigned char textbuf[MAXWIDTH+1];


	pxy[0] = g->g_x;
	pxy[1] = g->g_y;
	pxy[2] = g->g_x + g->g_w - 1;
	pxy[3] = g->g_y + g->g_h - 1;
	vs_clip(vdi_handle, 1, pxy);

	/* Für jede sichtbare Zeile */

	x = w->in.g_x - (int) (w->hscroll.shift*w->hscroll.pixelsize);
	y = w->in.g_y;
	l = w->vscroll.shift;

	for	(i = 0; i < w->vscroll.nvis; i++)
		{
		if	((y+w->vscroll.pixelsize > g->g_y) &&
			 (y < g->g_y+g->g_h))
			{
			line = get_line(w, l, &lw);

			/* TABs expandieren */

			if	(line)
				lw = expand_line( line, lw, w->user_tabsize,
								textbuf, MAXWIDTH );

			/* erst den Text ausgeben */

			if	(line)
				{
				textbuf[lw] = '\0';
				v_gtext(vdi_handle, x, y, (char *) textbuf);
				}

			lw *= w->hscroll.pixelsize;	/* ausgegebene Pixel */

			/* dann ggf. ein weißes Rechteck dahinter */

			if	(x+lw < w->in.g_x+w->in.g_w)
				{
				pxy[0] = x + (int) lw;
				pxy[1] = y;
				pxy[2] = w->in.g_x + w->in.g_w - 1;
				pxy[3] = y + w->vscroll.pixelsize - 1;
				vr_recfl(vdi_handle, pxy);
				}

			}

		/* nächste Zeile */

		y += w->vscroll.pixelsize;
		l++;
		}
}


/*****************************************************************
*
* Schließt ein Textfenster
*
*****************************************************************/



static void _my_close(WINDOW *w, int kstate)
{
	WINDOW **sl;

	if (kstate & K_CTRL)
		{
		v_clsvwk(vdi_handle);
		appl_exit();
		Pterm0();
		}
	wind_close(w->handle);
	wind_delete(w->handle);
	if (w->user_file)
		Mfree(w->user_file);
	if (w->user_lines)
		Mfree(w->user_lines);
	sl = find_slot_window(w);
	if (sl)
		{
		*sl = NULL;
		nwindows--;
		}
	Mfree(w);
}

/*****************************************************************
*
* Tastatureingabe für Textfenster
*
*****************************************************************/


static void _my_keyed( WINDOW *w, int kstate, int key )
{
	(void) kstate;
	int newtab;
	int ascii;

	/* Tabulator umschalten */

	ascii = key&0xff;
	if	((key < 0x1000) && (ascii >= '0') && (ascii <= '9'))
		{
		newtab = ascii-'0';
		if	(newtab != w->user_tabsize)
			{
			w->user_tabsize = newtab;
			get_line_column_count(w->user_file, w->user_fsize,
					&(w->vscroll.n), &(w->hscroll.n),
					w->user_tabsize);
			w->arrange(w);	/* Scrollbalken justieren! */
			update_window(w);
			}
		}
	else
	switch(key)
		{
/* Pos1  */
		case	0x4700:	w->arrowed(w, -1);
					break;
/* Ende (nur MF-2)     */
/* Ende (Macintosh)    */
		case 0x3700:
		case 0x4f00:
/* SH-Pos1 */
		case 0x4737:	w->arrowed(w, -2);
					break;
/* Cursor up */
		case 0x4800:	w->arrowed(w, WA_UPLINE);
					break;
/* Cursor down */
		case 0x5000:	w->arrowed(w, WA_DNLINE);
					break;
/* Bild hoch (Macintosh) */
		case 0x4900:
/* SH-Cursor up */
		case 0x4838:	w->arrowed(w, WA_UPPAGE);
					break;
/* Bild runter (Macintosh) */
		case 0x5100:
/* SH-Cursor dwn */
		case 0x5032:	w->arrowed(w, WA_DNPAGE);
					break;
/* Cursor links */
		case 0x4b00:	w->arrowed(w, WA_LFLINE);
					break;
/* Cursor rechts */
		case 0x4d00:	w->arrowed(w, WA_RTLINE);
					break;
/* SH-Cursor links */
		case 0x4b34:	w->arrowed(w, WA_LFPAGE);
					break;
/* SH-Cursor rechts */
		case 0x4d36:	w->arrowed(w, WA_RTPAGE);
					break;
/* ^U */
		case 0x1615:	w->close(w, 0);
					break;
		}
}



/*****************************************************************
*
* Initialisiert und öffnet ein Textfenster
*
*****************************************************************/

void open_textwind( char *path )
{
	WINDOW **new_slot;
	WINDOW *w;
	struct stat st;
	long err;
	int handle;
	GRECT full;

	if (strlen(path) > 127)
		return;
	new_slot = new_window();
	if (!new_slot)
		{
		form_alert(1,"[3][Kein Fenster mehr.][Abbruch]");
		return;
		}
	w = (WINDOW *) Malloc(sizeof(WINDOW));
	if (!w)
		{
		form_xerr(-ENOMEM, NULL);
		return;
		}
	w->user_file = NULL;
	w->user_lines = NULL;
	wind_calc_grect(WC_WORK, MY_KIND, &scrg, &full);
	full.g_x &= ~7;
	full.g_w -= full.g_w % text_attrib[8];
	full.g_h -= full.g_h % text_attrib[9];
	wind_calc_grect(WC_BORDER, MY_KIND, &full, &full);
	w->handle = wind_create_grect(MY_KIND, &full);
	if (w->handle <= 0)
		{
		Mfree(w);
		form_alert(1,"[3][Kein Fenster mehr.][Abbruch]");
		return;
		}
	err = Fopen(path, 0);
	if (err < 0)
		goto fail_window;
	handle = (int) err;
	if (fstat(handle, &st) < 0)
		{
		err = -errno;
		goto fail_file;
		}
	w->user_fsize = st.st_size;
	w->user_file = (unsigned char *) Malloc(w->user_fsize + 1);
	if (!w->user_file)
		{
		err = -ENOMEM;
		goto fail_file;
		}
	err = Fread(handle, w->user_fsize, w->user_file);
	if (err != w->user_fsize)
		{
		if (err >= 0)
			err = -EIO;
		goto fail_buffer;
		}
	w->user_file[w->user_fsize] = 0;
	Fclose(handle);
	init_window(w);
	w->user_tabsize = 8;
	get_line_column_count(w->user_file, w->user_fsize,
			&(w->vscroll.n), &(w->hscroll.n), w->user_tabsize);
	w->user_lines = NULL;
	if (w->vscroll.n)
		{
		w->user_lines = (unsigned char **) Malloc(w->vscroll.n * sizeof(*w->user_lines));
		if (!w->user_lines)
			{
			err = -ENOMEM;
			goto fail_buffer_closed;
			}
		init_line_ptrs(w->user_file, w->user_fsize, w->user_lines);
		}
	strcpy(w->title, path);
	w->hscroll.pixelsize = text_attrib[8];
	w->vscroll.pixelsize = text_attrib[9];
	w->snap = _my_snap;
	w->draw = _my_draw;
	w->arrange = _my_arrange;
	w->close = _my_close;
	w->key = _my_keyed;
	w->in.g_x = scrg.g_x;
	w->in.g_y = scrg.g_y;
	err = (long) w->hscroll.n * text_attrib[8];
	if (err > scrg.g_w)
		err = scrg.g_w;
	w->in.g_w = (int) err;
	err = (long) w->vscroll.n * text_attrib[9];
	if (err > scrg.g_h)
		err = scrg.g_h;
	w->in.g_h = (int) err;
	wind_calc_grect(WC_BORDER, MY_KIND, &w->in, &w->out);
	if (w->out.g_y < scrg.g_y)
		w->out.g_y = scrg.g_y;
	if (w->out.g_w > scrg.g_w)
		w->out.g_w = scrg.g_w;
	if (w->out.g_h > scrg.g_h)
		w->out.g_h = scrg.g_h;
	w->out.g_x += (int) (new_slot-windows)*text_attrib[8];
	w->out.g_y += (int) (new_slot-windows)*text_attrib[9];
	*new_slot = w;
	nwindows++;
	w->open(w);
	wind_set_str(w->handle, WF_NAME, w->title);
	return;

fail_buffer_closed:
	Mfree(w->user_file);
	goto fail_window;
fail_buffer:
	Mfree(w->user_file);
fail_file:
	Fclose(handle);
fail_window:
	wind_delete(w->handle);
	Mfree(w);
	form_xerr(err, path);
}
