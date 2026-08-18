/*******************************************************************
*
*		    ICNEDIT.PRG						05.08.89
*		    ===========
*						    letzte énderung:	10.04.94
*
* Icondateien ".ICD" und ".ICP" laden, anzeigen, editieren und
* wieder abspeichern
*
* geschrieben mit TUBO C V1.1, PureC 1.1
*
*******************************************************************/

#include <aes.h>
#include <vdi.h>
#include <tos.h>
#include <tosdefs.h>
#include <string.h>
/* #include <stdio.h> */
#include "icnedit.h"

#define USA	0
#define FRG	1

#define  SCREEN 0
#define  TRUE	 1
#define  FALSE  0
#define   NULL        ( ( void * ) 0L )
#define MIN(a,b) ((a < b) ? a : b)
#define MAX(a,b) ((a > b) ? a : b)

int message[16];
int work_out[57],work_in [12];	/* VDI- Felder fÅr v_opnvwk() */

char pgm_data[0x70];			/* Programmicon */
char dat_data[0x70];			/* Dateiicon */

char pgm_fname[128];			/* Dateiname fÅr Programmicon */
char dat_fname[128];			/* Dateiname fÅr Dateiicon */

#define XBEG_MM	4			/* Rand in Millimetern */
#define YBEG_MM	4			/* Rand in Millimetern */

int wpix;
int hpix;

int ybeg;						/* Rand in Pixeln einschl. MenÅleiste */
int xbeg;						/* Rand in Pixeln */


int ap_id;
int my_handle = -1;
int vdi_handle, gl_hhbox, gl_hwbox, gl_hhchar, gl_hwchar;
int scrx, scry, scrw, scrh;
int scrx2;				/* rechte HÑlfte */
int clip_array[4];
GRECT clip_grect;

OBJECT *adr_hauptmenu;
OBJECT *adr_picon;
OBJECT *adr_dicon;

extern char *get_name(char *path);		/* aus SDATEI */

void anfang	 (int err_tail);
void datei_menu (int entry);
void edit_menu  (int entry);
void icn_to_scr (int ispgm);
void mausknopf	 (int anzahl , int y_koor, int x_koor, int knopf, int kstate);
int rc_intersect(GRECT *p1, GRECT *p2);
void redraw	 (int whdl, GRECT *neu);


void main(int argc, char *argv[])
{
	int  err_tail;		/* Fehler in Kommandozeile */
	int  ev_mwich, ev_mbreturn, ev_mkreturn, ev_mmokstate,
		ev_mmobutton, ev_mmoy, ev_mmox;


	err_tail = FALSE;
	for	(argc--; argc > 0; argc--)
		{
		char *typ,*ziel;
		int  ispgm;

		strupr(argv[argc]);
		typ = strrchr(argv[argc], '.');
		if	(typ == NULL)
			goto err;
		typ++;
		if	(!strcmp(typ, "ICP"))
			ispgm = TRUE;
		else if	(!strcmp(typ, "ICD"))
				ispgm = FALSE;
			else {
				err: err_tail = TRUE;
				continue;
				}
		ziel = (ispgm) ? pgm_fname : dat_fname;
		if	(*ziel)
			goto err;		/* schon besetzt */
		strcpy(ziel, argv[argc]);
		}
	anfang(err_tail);
	graf_mouse(ARROW, NULL);
	for  (;;)
		{
		ev_mwich = evnt_multi(MU_KEYBD+MU_BUTTON+MU_MESAG,
						  0x0101,			/* nur Einfachklicks	*/
						  3,				/* l. und r. Knopf, NOT	*/
						  0,				/* l. und r. losgelassen	*/
						  0,0,0,0,0,		/* kein 1. Rechteck		*/
						  0,0,0,0,0,		/* kein 2. Rechteck		*/
						  message,
						  0,0,			/* kein Timer			*/
						  &ev_mmox,
						  &ev_mmoy,
						  &ev_mmobutton,
						  &ev_mmokstate,	/* Tastaturstatus		*/
						  &ev_mkreturn,	/* Tastaturcode		*/
						  &ev_mbreturn		/* Dummy (1 Klick)		*/
						 );

		/* Abfrage auf Nachricht im Nachrichtenpuffer */
		/* ------------------------------------------ */

		wind_update(BEG_UPDATE);

		if	(ev_mwich & MU_MESAG)
			{
			if	(message[0] == MN_SELECTED)
				{
			     switch(message[3]) {
#if		COUNTRY==FRG
			          case        3:  form_alert(1,"[1]["
			          			 "ICNEDIT fÅr MAGXDESK"
			          			"|\xbd Andreas Kromke 1989,94"
			          			"|Version 2.01"
			          			"][  OK  ]");break;
#elif	COUNTRY==USA
			          case        3:  form_alert(1,"[1]["
			          			 "ICNEDIT for MAGXDESK"
			          			"|\xbd Andreas Kromke 1989,94"
			          			"|Version 2.01"
			          			"][  OK  ]");break;
#endif
			          case MM_DATEI:  datei_menu(message[4]);break;
			          case MM_EDIT:    edit_menu(message[4]);break;
			          }
			     menu_tnormal(adr_hauptmenu,message[3],1);
			     }
			if	(message[0] == WM_REDRAW)
				redraw(message[3], (GRECT *) (message+4));
			}

		/* Abfrage auf BetÑtigung eines Mausknopfs */
		/* --------------------------------------- */

		if	(ev_mwich & MU_BUTTON)
			mausknopf(ev_mbreturn,ev_mmoy,ev_mmox,ev_mmobutton,ev_mmokstate);

		/* Abfrage auf BetÑtigung einer Taste */
		/* ---------------------------------- */

		if	(ev_mwich & MU_KEYBD)
			{
			switch(ev_mkreturn)
				{
				case 0x1900: datei_menu(M_LDPGM);break;
				case 0x2000: datei_menu(M_LDDAT);break;
				case 0x1910: datei_menu(M_SPPGM);break;
				case 0x2004: datei_menu(M_SPDAT);break;
				case 0x1F13: datei_menu(M_ENDE );break;
				case 0x1011: datei_menu(M_ABBRU);break;
				}
			}

		wind_update(END_UPDATE);

		} /* END FOREVER */
}


/****************************************************************
*
* Bestimmt die Schnittmenge zwischen zwei Rechtecken
*
****************************************************************/

int rc_intersect(GRECT *p1, GRECT *p2)
{
	int	tx, ty, tw, th;

	tw = MIN(p2->g_x + p2->g_w, p1->g_x + p1->g_w);
	th = MIN(p2->g_y + p2->g_h, p1->g_y + p1->g_h);
	tx = MAX(p2->g_x, p1->g_x);
	ty = MAX(p2->g_y, p1->g_y);
	p2->g_x = tx;
	p2->g_y = ty;
	p2->g_w = tw - tx;
	p2->g_h = th - ty;
	return( (tw > tx) && (th > ty) );
}


/****************************************************************
*
* Malt das Fenster mit Handle <whdl> im Bereich
* neu_x,neu_y,neu_b,neu_h  nach.
*
****************************************************************/

void redraw(int whdl, GRECT *neu)
{
	GRECT w;
	void zeichne(int whdl, int x, int y, int w, int h);


	if	(whdl != my_handle)
		return;						/* ungÅltiges Handle */
	graf_mouse(M_OFF, NULL);
	wind_get(whdl,WF_FIRSTXYWH,&(w.g_x),&(w.g_y),&(w.g_w),&(w.g_h));
	do	{
		if	(rc_intersect(neu,&w))
			zeichne(whdl, w.g_x, w.g_y, w.g_w, w.g_h);
		wind_get(whdl,WF_NEXTXYWH,&(w.g_x),&(w.g_y),&(w.g_w),&(w.g_h));
		}
	while(w.g_w > 0);					/* bis Rechteckliste vollstÑndig */
	graf_mouse(M_ON, NULL);
	clip_grect.g_x = scrx;
	clip_grect.g_y = scry;
	clip_grect.g_w = scrw;
	clip_grect.g_h = scrh;
	clip_array[0] = scrx;
	clip_array[1] = scry;
	clip_array[2] = clip_array[0] + scrw - 1;
	clip_array[3] = clip_array[1] + scrh - 1;
	vs_clip(vdi_handle, FALSE, clip_array);
}


/****************************************************************
*
* Zeichnet den Ausschnitt x,y,b,h des Fensters mit Handle <whdl>
* neu.
*
****************************************************************/

void zeichne(int whdl, int x, int y, int w, int h)
{
	if	(whdl != my_handle)
		return;
	clip_grect.g_x = x;
	clip_grect.g_y = y;
	clip_grect.g_w = w;
	clip_grect.g_h = h;
	clip_array[0] = x;
	clip_array[1] = y;
	clip_array[2] = clip_array[0] + w - 1;
	clip_array[3] = clip_array[1] + h - 1;
	vs_clip(vdi_handle, TRUE, clip_array);
	vsf_color(vdi_handle, WHITE);
	v_bar(vdi_handle, clip_array);
	icn_to_scr(1);
	icn_to_scr(0);
}


/****************************************************************
*
* Zeichnet das Fenster <wnr> neu.
*
****************************************************************/

void upd_wind(int whdl, int ispgm)
{
		  int    msg[8];
	extern int ap_id;

	/* Der Redraw wird nicht direkt ausgefÅhrt, sondern schickt */
	/* eine Nachricht an die Applikation, um Kollisionen mit    */
	/* von AES erzeugten Redraws zu vermeiden                   */
	/* -------------------------------------------------------- */

	msg[0] = WM_REDRAW;
	msg[1] = ap_id;
	msg[2] = 0;
	msg[3] = whdl;
	msg[4] = (ispgm) ? scrx : scrx2;
	msg[5] = scry;
	msg[6] = scrw / 2;
	msg[7] = scrh;
	appl_write(ap_id, 16, msg);
}


/*********************************************************************
*
* Dieses Modul enthÑlt die Initialisierung des AES/VDI, die Initiali-
* sierung von ICNEDIT sowie allgemeine Routinen.
*
*********************************************************************/

void open_work(void)
{
	register int i;


	for  (i = 0; i < 10; work_in[i++] = 1)
		;
	work_in[10]=2;                     /* Rasterkoordinaten */
	v_opnvwk(work_in, &vdi_handle, work_out);
	vswr_mode(vdi_handle,MD_REPLACE);      /* Replace- Modus */
	vsf_interior(vdi_handle,SOLID);        /* komplett ausfÅllen */
	/* RÑnder von mm in Pixel umrechnen */
	xbeg = (int) ((XBEG_MM * 1000L) / work_out[3]);
	ybeg = (int) ((YBEG_MM * 1000L) / work_out[4]);
}

/***************************************/

void close_work(void)
{
	wind_update(END_UPDATE);
	if	(my_handle >= 0)
		{
		wind_close(my_handle);
		wind_delete(my_handle);
		}
	v_clsvwk(vdi_handle);
	rsrc_free();
	appl_exit();
	Pterm0();
}


/****************************************************************
*
* Initialisierung von ICNEDIT (AES,RSC,VDI,Fenster,Icons,MenÅ).
*
****************************************************************/

void anfang(int err_tail)
{
	/* Applikation beim AES anmelden */
	/* ----------------------------- */
	if   ((ap_id = appl_init()) < 0)
		Pterm(1);

	/* VDI - Bildschirmhandle holen */
	/* ---------------------------- */
	vdi_handle = graf_handle(&gl_hwchar, &gl_hhchar, &gl_hwbox, &gl_hhbox);
	open_work();

	/* Mauskontrolle abschalten, MenÅ installieren */
	/* ------------------------------------------- */

	wind_update(BEG_UPDATE);

	/* Resourcedatei laden */
	/* ------------------- */
	if	(!rsrc_load("icnedit.rsc"))
		{
#if		COUNTRY==FRG
		form_alert(1, "[3][Kann \"ICNEDIT.RSC\"|nicht finden][ABBRUCH]");
#elif	COUNTRY==USA
		form_alert(1, "[3][Cannot find \"ICNEDIT.RSC\"][ Abort ]");
#endif
		close_work();
		}

     /* Cursor abschalten */
     /* ----------------- */
	Cursconf(0,0);

	/* Bildschirm- Arbeitsbereich berechnen */
	/* ------------------------------------ */

	wind_get(SCREEN, 4, &scrx, &scry, &scrw, &scrh);
	scrx2 = scrx + (scrw >> 1);

	my_handle = wind_create(0, -1, scry, scrw+2, scrh+2);
	if	(my_handle < 0)
		close_work();

	/* maximal mîgliche Punktgrîûen berechnen */
	/* -------------------------------------- */

	wpix = ((scrw >> 1) - xbeg - 32 * 3) / 32 + 2;
	hpix = (scrh - ybeg - 2 * gl_hhchar - 28 * 3) / 28 + 2;

	/* Bildschirmgeometrie fÅr die drei Atari-	*/
	/* Auflîsungen berÅcksichtigen			*/
	/* ----------------------------------------- */

	if	((scrw == 640 && scrh+scry == 400) ||
		 (scrw == 320 && scrh+scry == 200))
		{
		if	(wpix < hpix)
			hpix = wpix;
		wpix = hpix;
		}
	if	((scrw == 640 && scrh+scry == 200))
		{
		if	(wpix < hpix+hpix)
			hpix = (wpix >> 1);
		wpix = hpix+hpix;
		}
	
	if	(wpix < 1 || hpix < 1)
		{
#if		COUNTRY==FRG
		form_alert(1, "[3][Auflîsung zu gering!][ABBRUCH]");
#elif	COUNTRY==USA
		form_alert(1, "[3][Screen resolution too low!][ Abort ]");
#endif
		close_work();
		}

	ybeg += scry;
	rsrc_gaddr(0, HAUPTMEN, &adr_hauptmenu);
	rsrc_gaddr(0, T_ICONS, &adr_dicon);
	adr_picon = adr_dicon + I_PROGRA;
	adr_dicon += I_DATEI;

	adr_picon -> ob_x = 1;
	adr_dicon -> ob_x = scrx2 + 1;
	adr_dicon -> ob_y = adr_picon -> ob_y = scrh+scry-37;
	adr_dicon -> ob_height = adr_picon -> ob_height = 36;
	adr_picon -> ob_spec.iconblk->ib_pdata = (int *) pgm_data;
	adr_dicon -> ob_spec.iconblk->ib_pdata = (int *) dat_data;

	menu_bar(adr_hauptmenu, TRUE);
	wind_open(my_handle, -1, scry-1, scrw+2, scrh+2);

	if	(pgm_fname[0])
		{
		int	hdl;

		hdl = (int) Fopen(pgm_fname, RMODE_RD);
		if	((hdl < 0) || (0x70L != Fread(hdl, 0x70L, pgm_data)))
			err_tail = TRUE;
		if	(hdl >= 0)
			Fclose(hdl);
		}

	if	(dat_fname[0])
		{
		int	hdl;

		hdl = (int) Fopen(dat_fname, RMODE_RD);
		if	((hdl < 0) || (0x70L != Fread(hdl, 0x70L, dat_data)))
			err_tail = TRUE;
		if	(hdl >= 0)
			Fclose(hdl);
		}

	if	(err_tail)
#if		COUNTRY==FRG
		form_alert(1, "[3][Dateiname ungÅltig|oder nicht gefunden!][ABBRUCH]");
#elif	COUNTRY==USA
		form_alert(1, "[3][Filename invalid|or file does not exist!][ Abort ]");
#endif

	wind_update(END_UPDATE);
}


/****************************************************************
*
* Das MenÅ "Datei" ist angewÑhlt werden.
*
****************************************************************/

int pgm_lesen(int handle)
{
	return(Fread(handle, 0x70L, pgm_data) != 0x70L);
}

int dat_lesen(int handle)
{
	return(Fread(handle, 0x70L, dat_data) != 0x70L);
}

int pgm_schreiben(int handle)
{
	return(Fwrite(handle, 0x70L, pgm_data) != 0x70L);
}

int dat_schreiben(int handle)
{
	return(Fwrite(handle, 0x70L, dat_data) != 0x70L);
}


void datei_menu(int entry)
{
	extern int  oeffnen (int (*lesen)(),	char *dateityp);
	extern int  saveas	(int (*schreibe)(), char *dateityp);
	extern int  schliess(int (*schreibe)(), char *dateityp);
	void name_to_scr(int ispgm);
	extern char *get_fname( void );


	switch (entry) {

	  case M_LDPGM:	if	(!oeffnen(pgm_lesen, "ICP"))
	  					{
	  					strcpy(pgm_fname, get_fname());
	  					upd_wind(my_handle, 1);
	  					}
					break;

	  case M_LDDAT:	if	(!oeffnen(dat_lesen, "ICD"))
	  					{
	  					strcpy(dat_fname, get_fname());
	  					upd_wind(my_handle, 0);
	  					}
	  				break;

	  case M_SPPGM:	if	(*pgm_fname)
	  					strcpy(get_fname(), pgm_fname);
	  				if	(!saveas(pgm_schreiben, "ICP"))
	  					{
	  					strcpy(pgm_fname, get_fname());
	  					name_to_scr(TRUE);
	  					}
					break;

	  case M_SPDAT:	if	(*dat_fname)
	  					strcpy(get_fname(), dat_fname);
	  				if	(!saveas(dat_schreiben, "ICD"))
	  					{
	  					strcpy(dat_fname, get_fname());
	  					name_to_scr(FALSE);
	  					}
					break;

#if		COUNTRY==FRG
	  case M_ENDE:		if	(1 != form_alert(1,"[2][Icons sichern und Programm|beenden ?][OK|Abbruch]"))
#elif	COUNTRY==USA
	  case M_ENDE:		if	(1 != form_alert(1,"[2][Save icons and exit program ?][OK| Abort ]"))
#endif
	  					break;
	  				if	(*pgm_fname)
	  					{
	  					strcpy(get_fname(), pgm_fname);
	  					if	(schliess(pgm_schreiben, "ICP"))
	  						return;
						}
					if	(*dat_fname)
	  					{
	  					strcpy(get_fname(), dat_fname);
	  					if	(schliess(dat_schreiben, "ICD"))
	  						return;
						}
					close_work();

#if		COUNTRY==FRG
	  case M_ABBRU:	if	(form_alert(2, "[2][Programm abbrechen ?][JA| NEIN ]") == 1)
#elif	COUNTRY==USA
	  case M_ABBRU:	if	(form_alert(2, "[2][Abort program ?][YES|  NO  ]") == 1)
#endif
						close_work();
					break;
	}
}


/****************************************************************
*
* Das MenÅ "Editieren" ist angewÑhlt werden.
*
****************************************************************/

void edit_menu(int entry)
{
	char *data;

	data = NULL;
	switch(entry)
		{
		case M_CLRPGM: data = pgm_data;break;
		case M_CLRDAT: data = dat_data;break;
		}
	if	(data)
		{
		memset(data, 0, 0x70L);
		icn_to_scr(data == pgm_data);
		}
}


/****************************************************************
*
* stellt fest, ob der Iconpunkt (x,y) zum "Rahmen" gehîrt
* und daher auf *status stehen muû.
*
****************************************************************/

int israhmen(int ispgm, int x, int y, int *status)
{
	*status = FALSE;		/* weiûer Rahmen */
	if	(y == 0 || y == 27)
		return(TRUE);

	if	(ispgm)
		{
		if	(x == 0 || x == 31)
			return(TRUE);
		if	((x == 29) && (y == 2))
			return(TRUE);
		}
	else	{
		if	(x <= 3 || x >= 28)
			return(TRUE);
		if	(x-y >= 22)
			return(TRUE);
		if	((x >= 23) && (y <= 5) && (x != y+21))
			return(TRUE);
		}

	*status = TRUE;		/* schwarzer Rahmen */
	if	(ispgm)
		{
		if	(x == 1 || x == 30 || y == 26 || y <= 3)
			return(TRUE);
		}
	else {
		if	(x == 4 || x == 27 || y == 26 || y == 1)
			return(TRUE);
		if	((x == 22) && (y > 0) && (y <= 6))
			return(TRUE);
		if	((y == 6) && (x >= 22) && (x <= 27))
			return(TRUE);
		if	((y <= 5) && (y > 1) && (x == y+21))
			return(TRUE);
		}
	return(FALSE);			
}


/****************************************************************
*
* stellt fest, ob der Iconpunkt (x,y) gesetzt ist.
*
****************************************************************/

int  is_pixel(int ispgm, int x, int y)
{
	char *data;

	data = (ispgm) ? pgm_data : dat_data;
	data += y * 4 + x / 8;
	x %= 8;
	return(0 != (*data & (1 << (7 - x))));
}


/****************************************************************
*
* setzt Farbe des Iconpunktes (x,y) sowohl auf dem Bildschirm
* als auch in den Daten.
*
****************************************************************/

void set_pixel(int ispgm, int x, int y, int status, int print)
{
	char *data;
	char *mask;
	int  color,iix,iiy,my_xbeg;
	int  pxy[4];
	OBJECT *icn;
	GRECT g;
	int ix,iy;


	icn   = (ispgm) ? adr_picon : adr_dicon;
	data = (char *) icn -> ob_spec.iconblk -> ib_pdata;	/* Icondaten */
	mask = (char *) icn -> ob_spec.iconblk -> ib_pmask;	/* Iconmaske */
	my_xbeg = (ispgm) ? xbeg : xbeg+scrx2;
	iiy  = y * 4 + x / 8;
	data += iiy;
	mask += iiy;
	iix = 1 << (7 - x%8);
	if	(status)		/* Punkt setzen */
		*data |= iix;
	else	*data &= ~iix;	/* Punkt lîschen */
	color = (status) ? BLACK : WHITE;
	vsf_color(vdi_handle,color);
	if	(print)
		{
		pxy[0] = g.g_x = my_xbeg + x * (wpix+1) + 2;
		pxy[1] = g.g_y = ybeg + y * (hpix+1) + 2;
		pxy[2] = pxy[0] + wpix - 3;
		pxy[3] = pxy[1] + hpix - 3;
		g.g_w = pxy[2] - pxy[0] + 1;
		g.g_h = pxy[3] - pxy[1] + 1;
		if	(rc_intersect(&clip_grect, &g))
			v_bar(vdi_handle, pxy);
		}

	if	(*mask & iix)
		{
		ix = icn->ob_spec.iconblk->ib_xicon + icn->ob_x + x;
		iy = icn->ob_spec.iconblk->ib_yicon + icn->ob_y + y;

		if	(ix >= clip_array[0] && ix <= clip_array[2] &&
			 iy >= clip_array[1] && iy <= clip_array[3])
			{
			pxy[0] = ix;
			pxy[1] = iy;
			pxy[2] = pxy[0];
			pxy[3] = pxy[1];
			v_bar(vdi_handle, pxy);
			}
		}
}


/****************************************************************
*
* Rechnet Bildschirmkoordinaten in Iconkoordinaten um.
* RÅckgabe 1: Programmicon
*		 0: Dateiicon
*		-1: kein Iconpunkt
*
****************************************************************/

int getxy(int *x, int *y)
{
	int ispgm;
	int nx,ny;


	if	(*y <= ybeg || *x <= xbeg)
		return(-1);		/* zu hoch oder zu weit links */
	*y -= ybeg;
	ispgm = (*x < scrx2);
	if	(ispgm)
		*x -= xbeg;
	else *x -= xbeg+scrx2;
	nx = *x % (wpix+1);
	ny = *y % (hpix+1);
	if	(!nx || !ny)
		return(-1);		/* Gitter erwischt */
	*x /= (wpix+1);
	*y /= (hpix+1);
	if	(*x >= 32 || *y >= 28)
		return(-1);
	else return(ispgm);
}


/****************************************************************
*
* Die Icons werden auf den Bildschirm gebracht
*
****************************************************************/

void name_to_scr(int ispgm)
{
	OBJECT *icn;
	char *name;
	GRECT ig;


	icn  = (ispgm) ? adr_picon : adr_dicon;
	name = get_name((ispgm) ? pgm_fname : dat_fname);
	strcpy(icn -> ob_spec.iconblk->ib_ptext, name);

	ig.g_x	= icn->ob_spec.iconblk->ib_xtext + icn->ob_x;
	ig.g_y	= icn->ob_spec.iconblk->ib_ytext + icn->ob_y;
	ig.g_w	= icn->ob_spec.iconblk->ib_wtext;
	ig.g_h	= icn->ob_spec.iconblk->ib_htext;
	if	(rc_intersect(&clip_grect, &ig))
		objc_draw(icn,	0, 0, ig.g_x, ig.g_y, ig.g_w, ig.g_h);
}


void icn_to_scr(int ispgm)
{
	register int i,j;
	int my_xbeg;
	int pxy[4];


	graf_mouse(M_OFF, NULL);
	vsf_color(vdi_handle,WHITE);           /* FÅllfarbe weiû */
	my_xbeg = (ispgm) ? xbeg : xbeg+scrx2;
	/* weiûe FlÑche */
	pxy[0] = my_xbeg;
	pxy[1] = ybeg;
	pxy[2] = my_xbeg+(wpix+1)*32;
	pxy[3] = ybeg+(hpix+1)*28;
	v_bar(vdi_handle, pxy);
	/* waagerechte Linien */
	pxy[0] = my_xbeg;
	pxy[2] = my_xbeg+(wpix+1)*32;
	for	(i = 0; i <= 28; i++)
		{
		pxy[1] = pxy[3] = ybeg + i*(hpix+1);
		v_pline(vdi_handle, 2, pxy);
		}
	/* senkrechte Linien */
	pxy[1] = ybeg;
	pxy[3] = ybeg+(hpix+1)*28;
	for	(i = 0; i <= 32; i++)
		{
		pxy[0] = pxy[2] = my_xbeg + i*(wpix+1);
		v_pline(vdi_handle, 2, pxy);
		}
	/* Pixel zeichnen */
	for	(i = 0; i < 28; i++)			/* y */
		for	(j = 0; j < 32; j++)		/* x */
			{
			int status;

			if	(israhmen(ispgm, j, i, &status))
				set_pixel(ispgm, j, i, status, status);
			else	{
				if	(is_pixel(ispgm, j, i))
					set_pixel(ispgm, j, i, TRUE, TRUE);
				else set_pixel(ispgm, j, i, FALSE, FALSE);
				}
			}
	/* Dateinamen schreiben */
	name_to_scr(ispgm);
	graf_mouse(M_ON, NULL);
}


/****************************************************************
*
* Mausknopfbedienung
*
****************************************************************/

#pragma warn -par

void mausknopf	 (int anzahl , int y_koor, int x_koor, int knopf, int kstate)
{
	int  ispgm;
	int  clear;
	int  altstatus,neustatus;


	if	(anzahl != 1)
		return;
	clear = (knopf & 2);
	ispgm = getxy(&x_koor, &y_koor);
	if	(ispgm < 0)
		return;
	/* Den Rahmen darf man nicht Ñndern */
	if	(israhmen(ispgm, x_koor, y_koor, &altstatus))
		return;
	altstatus = is_pixel(ispgm, x_koor, y_koor);
	neustatus = !clear;
	if	(altstatus == neustatus)
		return;
	graf_mouse(M_OFF, NULL);
	set_pixel(ispgm, x_koor, y_koor, neustatus, TRUE);
	graf_mouse(M_ON, NULL);
}

#pragma warn +par
