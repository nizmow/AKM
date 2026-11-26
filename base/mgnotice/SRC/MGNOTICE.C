/*******************************************************************
*
*             MGNOTICE.APP                             10.11.96
*             ============
*                                 letzte Änderung:
*
* geschrieben mit Pure C V1.1
* Projektdatei: MGNOTICE.PRJ
*
* Verwaltet Notizen als Fenster.
*
****************************************************************/

#define DEBUG 0

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "MGNOTICE_GEM.H"
#include "WINDOWS.H"
#include "GLOBALS.H"
#include "MGNOTICE.H"

short	gl_hhbox, gl_hwbox, gl_hhchar, gl_hwchar;
int	ap_id;
int	ncolours;
GRECT scrg;
short aes_handle;		/* Screen-Workstation des AES */

/* für die SharedLib */
SLB_EXEC	slbexec;
SLB_HANDLE	slb;

/* globale Einstellungen */

struct prefs prefs;
static char notice_path_buffer[] = "X:\\gemsys\\gemdesk\\notice\\";
char *notice_path = notice_path_buffer;
/*
char *notice_path = "X:\\pc\\source\\deskutil\\mgnotice.2\\notice\\";
*/

/* Dialoge */

OBJECT *adr_menu;
OBJECT *adr_about;
OBJECT *adr_options;
OBJECT *adr_colour;
void *d_options = NULL;

/* Fenster */
WINDOW *windows[NWINDOWS];


/****************************************************************
*
* Rechnet Koordinaten linear um. Der Wert <wert> wurde bei
* Bildschirmgröße <old> abgespeichert, jetzt ist die
* Bildschirmgröße <new>.
*
* Liegt das Fenster links aus dem Bildschirm heraus, wird
* nichts umgerechnet. Hier sollte aber sichergestellt werden,
* daß man an das Fenster noch herankommt, d.h.:
*	x+w > k > 0.
*
****************************************************************/

static void recalc(int *wert, int old, int new)
{
	unsigned long tmp;

	if	(*wert > 0)
		{
		tmp    = (unsigned long) *wert;
		tmp   *= new;
		tmp   /= old;
		*wert  = (int) tmp;
		}
}

static short recalc_short(short wert, int old, int new)
{
	unsigned long tmp;

	if	((wert > 0) && (old > 0) && (new > 0))
		{
		tmp   = (unsigned long) wert;
		tmp  *= new;
		tmp  /= old;
		return (short) tmp;
		}
	return wert;
}


/****************************************************************
*
* Lese INF-Datei
*
****************************************************************/

static char infpath[128];

void read_inf( void )
{
	char buf[512];
	long len;
	int hdl;
	char *s,*t;
	int oldwh[2];
	int font[3];


	prefs.fontID = 1;
	prefs.fontH = 10;
	prefs.fontprop = FALSE;
	strcpy(prefs.fontname, "System Font");
	prefs.colour = 0;
	prefs.prefs_win.g_x = prefs.prefs_win.g_y = -1;

	s = getenv("HOME");
	if	(s)
		{
		strcpy(infpath, s);
		t = infpath+strlen(s);
		if	(t[-1] != '\\')
			*t++ = '\\';
		}
	else	t = infpath;
	strcpy(t, "MGNOTICE.INF");

	if	((s) || (shel_find(infpath)))
		{
		hdl = (int) Fopen(infpath, RMODE_RD);
		if	(hdl < 0)
			return;
		len = Fread(hdl, 511L, buf);
		Fclose(hdl);
		if	(len < E_OK)
			return;
		buf[len] = EOS;

		/* erste Zeile überlesen */

		for	(s = buf; (*s) && (*s != '\n'); s++)
			;

		if	(*s == '\n')
			s++;

		/* andere Zeilen auswerten */

		while(*s)
			{
			if	(!strncmp(s, "SCREENSIZE ", 11))
				{
				s += 11;
				scan_values(&s, 2, oldwh);
				goto weiter;
				}

			if	(!strncmp(s, "WINDOW PREFS ", 12))
				{
				int winpos[4];

				s += 13;
				if	(scan_values(&s, 4, winpos) == 4)
					{
					prefs.prefs_win.g_x = (short) winpos[0];
					prefs.prefs_win.g_y = (short) winpos[1];
					prefs.prefs_win.g_w = (short) winpos[2];
					prefs.prefs_win.g_h = (short) winpos[3];
					}
				}

			if	(!strncmp(s, "FONT ", 5))
				{
				s += 5;
				scan_values(&s, 3, font);
				prefs.fontID = font[0];
				prefs.fontprop = font[1];
				prefs.fontH = font[2];
				}

			if	(!strncmp(s, "FONTNAME ", 9))
				{
				s += 9;
				t = prefs.fontname;
				while((*s) && (*s != '\r'))
					*t++ = *s++;
				*t = EOS;
				}

			if	(!strncmp(s, "COLOUR ", 7))
				{
				int colour;

				s += 7;
				if	(scan_values(&s, 1, &colour) == 1)
					prefs.colour = (short) colour;
				}

			weiter:

			while((*s) && (*s != '\n'))
				s++;

			if	(*s == '\n')
				s++;
			}


		if	(oldwh[0] != scrg.g_w)
			{
			prefs.prefs_win.g_x = recalc_short(prefs.prefs_win.g_x,
							  oldwh[0], scrg.g_w);
			prefs.prefs_win.g_w = recalc_short(prefs.prefs_win.g_w,
							  oldwh[0], scrg.g_w);
			}
		if	(oldwh[1] != scrg.g_h)
			{
			prefs.prefs_win.g_y = recalc_short(prefs.prefs_win.g_y,
							  oldwh[1], scrg.g_h);
			prefs.prefs_win.g_h = recalc_short(prefs.prefs_win.g_h,
							  oldwh[1], scrg.g_h);
			}
		prefs.prefs_win.g_x += scrg.g_x;
		prefs.prefs_win.g_y += scrg.g_y;
		}
}


/****************************************************************
*
* Schreibe Fensterposition auflösungsunabhängig in
* Zeichenkette.
*
****************************************************************/

static void print_winpos(char *s, GRECT *g, int n)
{
	int values[4];

	values[0] = g->g_x - scrg.g_x;
	values[1] = g->g_y - scrg.g_y;
	values[2] = g->g_w;
	values[3] = g->g_h;
	print_values(s, n, values);
}


/****************************************************************
*
* Schreibe INF-Datei
*
****************************************************************/

void save_options( void )
{
	int hdl;
	char buf[512];
	int scrwh[2];
	int font[3];



	if	(d_options)
		{
		prefs.prefs_win.g_x = adr_options->ob_x;
		prefs.prefs_win.g_y = adr_options->ob_y;
		}

	hdl = (int) Fcreate(infpath, 0);
	if	(hdl < 0)
		return;
	strcpy(buf, "[MGNOTICE Header V 1.0]\r\n"
					"SCREENSIZE ");
	scrwh[0] = scrg.g_w;
	scrwh[1] = scrg.g_h;
	print_values(buf+strlen(buf), 2, scrwh);

	strcat(buf, "\r\nWINDOW PREFS ");
	print_winpos(buf + strlen(buf), &prefs.prefs_win, 4);

	strcat(buf, "\r\nFONT ");
	font[0] = prefs.fontID;
	font[1] = prefs.fontprop;
	font[2] = prefs.fontH;
	print_values(buf + strlen(buf), 3, font);

	strcat(buf, "\r\nFONTNAME ");
	strcat(buf, prefs.fontname);

	strcat(buf, "\r\nCOLOUR ");
	{
		int colour = prefs.colour;

		print_values(buf + strlen(buf), 1, &colour);
	}

	Fwrite(hdl, (long) strlen(buf), buf);
	Fclose(hdl);
}


/****************************************************************
*
* liest alle Notizen ein.
*
****************************************************************/

static void read_all_notices( char *path )
{
	long errcode;
	char p[128];
	char *pname;
	WINDOW *w;
	DTA dta;
	int handle;
	unsigned char *data;
	char *s;
	int id;			/* eindeutige Nummer 0..NWINDOWS-1 */
	int xywh[4];
	int oldwh[2];
	int font[3];		/* fontID, propFlag und Größe */
	int colour;
	int n;


	Fsetdta(&dta);
	strcpy(p, path);
	pname = p + strlen(p);
	strcpy(pname, "*.NOT");
	errcode = Fsfirst(p, 0);
	if	(errcode == EPTHNF)
		{
		s = path+strlen(path)-1;
		*s = EOS;
		Dcreate(path);
		*s = '\\';
		errcode = Fsfirst(p, 0);
		}

	xywh[0] = 100;
	xywh[1] = 100;
	xywh[2] = 80;
	xywh[3] = 32;
	font[0] = prefs.fontID;
	font[1] = prefs.fontprop;
	font[2] = prefs.fontH;
	colour = prefs.colour;

	oldwh[0] = scrg.g_w;
	oldwh[1] = scrg.g_h;
	while(!errcode)
		{
		if	(!errcode)
			{
			strcpy(pname, dta.dta_name);
			id = atoi(pname);
			handle = (int) Fopen(p, RMODE_RD);
			if	(handle > 0)
				{
				data = (unsigned char *) Malloc(dta.dta_size+1);
				if	(data)
					{
					data[dta.dta_size] = '\0';
					Fread(handle, dta.dta_size, data);

					/* Steuer-Infos auswerten */
					/* ---------------------- */

					s = (char *) data;
					while(*s)
						{
						if	(!strncmp(s, "BEGIN", 5))
							{
							s += 5;
							while((*s) && (*s != '\n'))
								s++;
							if	(*s == '\n')
								s++;
							break;		/* Anfang der Notiz */
							}

						if	(!strncmp(s, "SCREENSIZE ", 11))
							{
							s += 11;
							scan_values(&s, 2, oldwh);
							goto nextline;
							}

						if	(!strncmp(s, "WINPOS ", 7))
							{
							s += 7;
							n = scan_values(&s, 4, xywh);
							if	(n == 2)
								{
								xywh[2] = 80;	/* Breite */
								xywh[3] = 32;	/* Höhe */
								}
							goto nextline;
							}

						if	(!strncmp(s, "FONT ", 5))
							{
							s += 5;
							scan_values(&s, 3, font);
							goto nextline;
							}

						if	(!strncmp(s, "COLOUR ", 7))
							{
							s += 7;
							scan_values(&s, 1, &colour);
							goto nextline;
							}

					nextline:
						while((*s) && (*s != '\n'))
							s++;
						if	(*s == '\n')
							s++;
						}
					if	(oldwh[0] != scrg.g_w)
						{
						recalc(&(xywh[0]), oldwh[0], scrg.g_w);
						}
					if	(oldwh[1] != scrg.g_h)
						{
						recalc(&(xywh[1]), oldwh[1], scrg.g_h);
						}
					xywh[0] += scrg.g_x;
					xywh[1] += scrg.g_y;

					if	(xywh[0] > scrg.g_w - 8)
						xywh[0] = scrg.g_w - 8;
					if	(xywh[1] > scrg.g_h - 8)
						xywh[1] = scrg.g_h - 8;

					GRECT g;

					memcpy(data, s, strlen(s)+1);
					Mshrink(data, strlen((char *) data)+1);
					g.g_x = (short) xywh[0];
					g.g_y = (short) xywh[1];
					g.g_w = (short) xywh[2];
					g.g_h = (short) xywh[3];
					w = NULL;
					errcode = open_notice_wind(data,
								(short) id,
								&g,
								(short) font[0],
								(short) font[1],
								(short) font[2],
								(short) colour,
								&w);
					if	(errcode)
						form_xerr(errcode, NULL);
					if	(w)
						w->position_dirty = FALSE;
					}
				Fclose(handle);
				}
			}
		errcode = Fsnext();
		}
}


/****************************************************************
*
* Schreibt eine Notiz.
*
****************************************************************/

long save_notice( WINDOW *w, char *path )
{
	long errcode;
	char p[128];
	char buf[256];
	int handle;
	char *s;
	int font[3];		/* fontID und Größe */


	strcpy(p, path);
	sprintf(p + strlen(p), "%d", w->id_code);
	strcat(p, ".NOT");
	errcode = Fcreate(p, 0);
	if	(errcode < E_OK)
		{
	  err1:
		form_xerr(errcode, p);
		return(errcode);
		}

	handle = (int) errcode;

	/* Steuer-Infos schreiben */
	/* ---------------------- */

	strcpy(buf, "SCREENSIZE ");
	{
		int scrwh[2];

		scrwh[0] = scrg.g_w;
		scrwh[1] = scrg.g_h;
		print_values(buf+strlen(buf), 2, scrwh);
	}
	s = buf+strlen(buf);
	*s++ = '\r';
	*s++ = '\n';

	strcpy(s, "WINPOS ");
	print_winpos(buf + strlen(buf), &w->out, 4);
	s = buf+strlen(buf);
	*s++ = '\r';
	*s++ = '\n';

	strcpy(s, "FONT ");
	s += strlen(s);
	font[0] = w->fontID;
	font[1] = w->fontprop;
	font[2] = w->fontH;
	print_values(s, 3, font);
	s += strlen(s);
	*s++ = '\r';
	*s++ = '\n';

	strcpy(s, "COLOUR ");
	s += strlen(s);
	{
		int colour = w->bcolour;

		print_values(s, 1, &colour);
	}
	s += strlen(s);
	*s++ = '\r';
	*s++ = '\n';

	strcpy(s, "BEGIN\r\n");
	errcode = Fwrite(handle, (long) strlen(buf), buf);
	if	(errcode != (long) strlen(buf))
		{
		err2:
		if	(errcode >= E_OK)
			errcode = EWRITF;
		Fclose(handle);
		goto err1;
		}

	errcode = Fwrite(handle, (long) strlen(w->buf), w->buf);
	if	(errcode != (long) strlen(w->buf))
		goto err2;

	w->position_dirty = FALSE;		/* ist gesichert */
	return(Fclose(handle));
}


/****************************************************************
*
* Schreibt alle geänderten Notizen.
*
****************************************************************/

void save_all_notices( char *path )
{
	register int i;
	register WINDOW **wp;

	for	(i = 0, wp = windows; i < NWINDOWS; i++,wp++)
		if	((*wp) && ((*wp)->position_dirty))
			save_notice(*wp, path);
}


/****************************************************************
*
* Ändert die Farbe für die entsprechende Notiz
*
****************************************************************/

static void change_colour( WINDOW *w )
{
	EVNTDATA ev;
	int obj;
	int tcolour;


	graf_mkstate(&ev.x, &ev.y, &ev.bstate, &ev.kstate);
	obj = form_popup(adr_colour,ev.x,ev.y);
	if	(obj > 0)
		{
		w->bcolour = (short) (obj-1);
		w->tcolour = TED_TEXTCOLOR(adr_colour[obj].ob_spec.tedinfo);
		if	(!(w->flags & WFLAG_ICONIFIED))
			{
			tcolour = (w->bcolour < ncolours) ? w->tcolour : WHITE;
			edit_set_colour( &w->tree, EDITFELD,
				(short) tcolour, w->bcolour);
			update_window(w);
			}
		save_notice( w, notice_path );
		}
}


/****************************************************************
*
* Wählt einen Zeichensatz aus.
*
****************************************************************/

#define	FONT_FLAGS	( FNTS_BTMP + FNTS_OUTL + FNTS_MONO + FNTS_PROP )
#define	BUTTON_FLAGS ( FNTS_SNAME + FNTS_SSTYLE + FNTS_SSIZE )

int dial_font( long *id, long *pt, short *mono, char *name )
{
	short work_out[57], work_in[11];	/* VDI-Felder für v_opnvwk() */
	short	handle;
	register int i;
	FNT_DIALOG *fnt_dialog;
	int button;
	short check_boxes;
	long ratio;
	short dummy;


	for( i = 0; i < 10 ; i++ )											/* work_in initialisieren */
		work_in[i] = 1;
	work_in[10] = 2;		/* Rasterkoordinaten benutzen */
	handle = aes_handle;
	v_opnvwk( work_in, &handle, work_out );

	ratio = (1L<<16L);
	fnt_dialog = fnts_create( handle, 0, FONT_FLAGS, FNTS_3D,
				  "Was Shumway Your favourite Gordon?", 0L );
	if	(!fnt_dialog )
		return(0);
	button = fnts_do( fnt_dialog, BUTTON_FLAGS, *id, *pt, ratio,
			&check_boxes, id, pt, &ratio );
	if	(button == FNTS_OK)
		{
/*
		char s[100];
		Cconws("\x1b" "Hid=");
		ltoa(id, s, 16);
		Cconws(s);
		Cconws("        pt=");
		ltoa(pt, s, 16);
		Cconws(s);
		Cconws("        ratio=");
		ltoa(ratio, s, 16);
		Cconws(s);
		Cconws("        ");
		Cnecin();
*/
		if	(!fnts_get_info(fnt_dialog, *id, mono, &dummy ))
			*mono = FALSE;
		if	(name)
			{
			fnts_get_name(fnt_dialog, *id, name, NULL, NULL);
			}
		}
	fnts_delete( fnt_dialog, handle );
	v_clsvwk(handle);
	return(button == FNTS_OK);
}


/****************************************************************
*
* Ändert den Zeichensatz für die entsprechende Notiz.
* w == NULL: Ändert den Default-Font
*
****************************************************************/

#define	FONT_FLAGS	( FNTS_BTMP + FNTS_OUTL + FNTS_MONO + FNTS_PROP )
#define	BUTTON_FLAGS ( FNTS_SNAME + FNTS_SSTYLE + FNTS_SSIZE )

static void change_font( WINDOW *w )
{
	long id,pt;
	short mono;
	int ret;


	id = w->fontID;
	pt = (((long) w->fontH)<<16L);
	ret = dial_font( &id, &pt, &mono, NULL );
	if	(ret)
		{
		w->fontID = (short) id;
		w->fontH = (short) (pt >> 16L);
		w->fontprop = !mono;
		calc_size_notice_wind( w );
		edit_set_font( &w->tree, EDITFELD,
				w->fontID, w->fontH, FALSE, !w->fontprop);
		if	(!(w->flags & WFLAG_ICONIFIED))
			{
			w->arrange(w);
			w->moved(w, &(w->out));
			update_window(w);
			}
		save_notice( w, notice_path );
		}
}


/****************************************************************
*
* Erstellt eine neue Notiz.
*
****************************************************************/

void create_notice( int x, int y )
{
	WINDOW *newwin;
	long errcode;
	GRECT g;

	g.g_x = x;
	g.g_y = y;
	g.g_w = 80;
	g.g_h = 32;

	errcode = open_notice_wind( (unsigned char *) "\0", -1,
					&g,
					prefs.fontID,
					prefs.fontprop,
					prefs.fontH,
					prefs.colour,
					&newwin );
	if	(errcode)
		{
		form_xerr(errcode, NULL);
		}
	else	{
		select_window(newwin);
		save_notice(newwin, notice_path);
		}
}


/****************************************************************
*
* Öffnet den "Voreinstllungen" Dialog
*
****************************************************************/

void open_options( void )
{
	int whdl;


	if	(d_options)	/* ist schon geöffnet */
		{
		}
	else	{
		d_options = wdlg_create(
			hdl_options,
			adr_options,
			NULL,
			0,
			NULL,
			0);
		if	(d_options)
			{
			whdl =
				wdlg_open(
					d_options,
					Rgetstring(
						STR_OPTIONTITLE,
						NULL),
					NAME+
					CLOSER+
					MOVER/*+
					SMALLER*/,
					prefs.prefs_win.g_x,
					prefs.prefs_win.g_y,
					0,
					NULL );
			if	(whdl <= 0)
				{
				wdlg_delete(d_options);
				d_options = NULL;
				Rform_alert(1,
						ALRT_ERROPENWIND,
						NULL);
				}
			}
		}
}


/****************************************************************
*
* Ändert Status DISABLED von Menüeinträgen
*
****************************************************************/

void menu_change( int enable )
{
/*	menu_ienable(adr_menu, M_OPEN, enable);	*/
	menu_ienable(adr_menu, M_DELETE, enable);
	menu_ienable(adr_menu, M_FONT, enable);
	menu_ienable(adr_menu, M_COLOUR, enable);
}


/****************************************************************
*
* Bearbeitet die Menübefehle
*
****************************************************************/

static int do_menu( int title, int entry)
{
	int state;
	int ret = 0;


	/* Sonderfall für Tastaturbedienung: */
	/* --------------------------------- */

	state = adr_menu[title].ob_state;
	if	(state & DISABLED)
		return(0);	/* Eintrag ungültig */
	if	(!(state & SELECTED))
		menu_tnormal(adr_menu, title, 0);

	switch( entry )
		{
		case M_ABOUT:
			do_dialog(adr_about);
			break;
		case M_NEW:
			create_notice(100, 100);
			break;
/*
		case M_OPEN:
			if	(selected_window)
				{
				create_notice(selected_window);
				}
			break;
*/
		case M_DELETE:
			if	(selected_window)
				{
				char p[128];

				strcpy(p, notice_path);
				sprintf(p + strlen(p), "%d",
						selected_window->id_code);
				strcat(p, ".NOT");
				Fdelete(p);

				edit_close(&selected_window->tree, EDITFELD);
				edit_delete( selected_window->xedit );

				Mfree(selected_window->buf);
				wind_close(selected_window->handle);
				wind_delete(selected_window->handle);
				Mfree(selected_window);
				*(find_slot_window(selected_window)) = NULL;
				selected_window = NULL;
				}
			break;
		case M_QUIT:
			ret = 1;
			break;

		case M_FONT:
			if	(selected_window)
				change_font(selected_window);
			break;
		case M_COLOUR:
		/*	w = whdl2window(top_whdl());	*/
			if	(selected_window)
				change_colour(selected_window);
			break;
		case M_PREFS:
			open_options();
			break;
		}
     menu_tnormal(adr_menu, title, 1);
	return(ret);
}


/****************************************************************
*
* Bearbeitet die Tastaturbefehle
*
****************************************************************/

int do_key( EVNT *w_ev, WINDOW *w )
{
	WORD menu, entry;


	menu = -1;
	switch( w_ev->key )
		{
		case 0x310e:	/* ^N */
			menu = MT_FILE;
			entry = M_NEW;
			break;
		case 0x2004:	/* ^D */
			menu = MT_FILE;
			entry = M_DELETE;
			break;

		case 0x1011:
			return(1);/* ^Q */

		case 0x151a:	/* ^Z */
			menu = MT_OPTIONS;
			entry = M_FONT;
			break;

		case 0x2e00:	/* Alt-C */
			menu = MT_OPTIONS;
			entry = M_COLOUR;
			break;

		case 0x1200:	/* Alt-E */
			menu = MT_OPTIONS;
			entry = M_PREFS;
			break;

		case	0x1117:
			if	(w)	/* ^W */
				{
				int i,wnr;

				wnr = (int) (find_slot_window(w) - windows);
				i = wnr;
				do	{
					i++;
					if	(i >= NWINDOWS)
						i = 0;
					if	(windows[i])
						{
						wind_set(windows[i]->handle,
								WF_TOP, 0, 0, 0, 0);
						break;
						}
					}
				while(i != wnr);
				w_ev->mwhich &= ~MU_KEYBD;	/* bearbeitet */
				}
			}

	if	(menu >= 0)
		{
		do_menu(menu, entry);
		w_ev->key &= ~MU_KEYBD;
		}
	return(0);
}


/*************************************************/
/**************** HAUPTPROGRAMM ******************/
/*************************************************/

int main( void )
{
	static WINDOW *last_selected_window = (WINDOW *) -1;
	WINDOW *w;
	EVNT w_ev;
	int i;
	long err;



	/* SharedLib laden */
	/* --------------- */

	Pdomain(PDOM_MINT);
	err = Slbopen("EDITOBJC.SLB", NULL, 9L, &slb, &slbexec);
	if	(err < 0)
		Pterm((short) err);

	/* Initialisierung */
	/* --------------- */

	notice_path[0] = Dgetdrv()+'A';
	if   ((ap_id = appl_init()) < 0)
		Pterm(-1);
	i = aes_global[10];
	ncolours = (i > 8) ? 32767 : (1 << i);
	wind_get_grect(SCREEN, WF_WORKXYWH, &scrg);

	if	(!rsrc_load("mgnotice.rsc"))
		{
		form_xerr(EFILNF, "mgnotice.rsc");
		appl_exit();
		Pterm((int) EFILNF);
		}

	aes_handle = graf_handle(&gl_hwchar, &gl_hhchar, &gl_hwbox, &gl_hhbox);
	vdi_handle = aes_handle;
	open_work();

	vst_load_fonts(vdi_handle, 0);

	rsrc_gaddr(0, T_MENU, &adr_menu);
	rsrc_gaddr(0, T_ABOUT, &adr_about);
	rsrc_gaddr(0, T_OPTIONS, &adr_options);
	rsrc_gaddr(0, T_COLOUR, &adr_colour);

	/* Voreinstellungen */
	/* ---------------- */

	read_inf();

	options_dial_init_rsc();

	menu_bar(adr_menu, TRUE);

	/* Abgespeicherte Notizen öffnen */
	/* ----------------------------- */

	read_all_notices(notice_path);

	for	(;;)
		{

		/* Menüeintträge ENABLE/DISABLE */
		/* ---------------------------- */

		if	(selected_window != last_selected_window)
			{
			if	(selected_window && !last_selected_window)
				{
				menu_change(TRUE);
				}
			else
			if	(!selected_window && last_selected_window)
				{
				menu_change(FALSE);
				}
			last_selected_window = selected_window;
			}

		w_ev.mwhich = mt_evnt_multi(MU_KEYBD+MU_BUTTON+MU_MESAG,
			  2,			/* Doppelklicks erkennen 	*/
			  1,			/* nur linke Maustaste		*/
			  1,			/* linke Maustaste gedrückt	*/
			  0, 0, 0, 0, 0,	/* kein 1. Rechteck			*/
			  0, 0, 0, 0, 0,	/* kein 2. Rechteck			*/
			  w_ev.msg,
			  0L,	/* ms */
			  &w_ev.mx, &w_ev.my, &w_ev.mbutton, &w_ev.kstate,
			  &w_ev.key, &w_ev.mclicks,
			  aes_global);

		/* Fensterdialoge */
		/* -------------- */

		if	(d_options && !wdlg_evnt(d_options, &w_ev))
			{
			wdlg_close(d_options, NULL, NULL);
			wdlg_delete(d_options);
			d_options = NULL;
			/* Fensterposition merken */
			prefs.prefs_win.g_x = adr_options->ob_x;
			prefs.prefs_win.g_y = adr_options->ob_y;
			}

		/* Tastaturbedienung für Menüs */
		/* --------------------------- */

		w = selected_window;
		if	(w_ev.mwhich & MU_KEYBD)
			{
			if	(do_key( &w_ev, w ))
				break;
			}

		/* Taste an Fenster */
		/* ---------------- */

		if	(w_ev.mwhich & MU_KEYBD)
			{
			if	(w)
				w->key(w, w_ev.kstate, w_ev.key);
			w_ev.mwhich &= ~MU_KEYBD;	/* IMMER bearbeitet */
			}

		/* Mausknopf an Fenster */
		/* -------------------- */

		if	(w_ev.mwhich & MU_BUTTON)
			{
			w = whdl2window(wind_find(w_ev.mx, w_ev.my));

			if	(w)
				{
				w->button(w, w_ev.mclicks,
						(EVNTDATA *) &w_ev.mx);
				}
			w_ev.mwhich &= ~MU_BUTTON;	/* bearbeitet */
			}

		if	(w_ev.mwhich & MU_MESAG)
			{
			if	((w_ev.msg[0] == WM_TOPPED) ||
			 	 (w_ev.msg[0] == WM_ONTOP))
				{
				wind_set(0, WF_TOPALL, ap_id, 0,0,0);
				/*
				top_all_my_windows();
				*/
				}
			else
			if	(w_ev.msg[0] == WM_UNTOPPED)
				{
				i = menu_bar(NULL, -1);
				if	(i != ap_id)
					{
					wind_set(0, WF_BOTTOMALL, ap_id, 0,0,0);
					/*
					bottom_all_my_windows();
					*/
					}
				}
			}

		/* Nachricht */
		/* --------- */

		if	(w_ev.mwhich & MU_MESAG)
			{
			if	(w_ev.msg[0] == MN_SELECTED)
				{
				if	(do_menu(w_ev.msg[3], w_ev.msg[4]))
					goto ende;
				}
				
			else
			if	(w_ev.msg[0] == AP_TERM)
				break;
			else

			/* Fenster-Nachricht empfangen */
			/* --------------------------- */
	
			if	(((w_ev.msg[0] >= 20) &&
						(w_ev.msg[0] < 40)) ||		/* WM_XX */
				 		(w_ev.msg[0] >= 1040))
				{
				w = whdl2window(w_ev.msg[3]);
				if	(w)
					{
					w->message(w, w_ev.kstate, w_ev.msg);
					w_ev.mwhich &= ~MU_MESAG;	/* bearbeitet */
					}
				}
			}
		}

ende:
	save_all_notices(notice_path);

	vst_unload_fonts(vdi_handle, 0);

	v_clsvwk(vdi_handle);
	rsrc_free();
	if	(slb)
		Slbclose( slb );
	appl_exit();
	return(0);
}


/****************************************************************
*
* Malt ein Unterobjekt eines Fensters
*
****************************************************************/

void subobj_wdraw(void *d, int obj, int startob, int depth)
{
	OBJECT *tree;
	GRECT g;


	wdlg_get_tree( d, &tree, &g );
	objc_grect(tree, obj, &g);
	wdlg_redraw( d, &g, startob, depth );
}
