/*********************************************************************
*
* Beginn der Programmierung: 14.3.89
*
* Dieses Modul enthÑlt die Hauptsteuerung von KAOSDESK, also die
* event_multi - Schleife.
*
*********************************************************************/

#include <tos.h>
#include <vdi.h>
#include <tosdefs.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mgx_dos.h>
#include "k.h"


WINDOW fenster[ANZFENSTER+1];
ICON *icon;
APPLICATION *application;
char *poolptr;		/* Zeiger auf Pool					*/
char *poolfree;		/* Zeiger auf freien Teil des Pools	*/
int	poolsize;		/* Anzahl freier EintrÑge			*/

int n_icons, n_apps;

ICONFILE_HEADER iconfile_header;

int  *icf_data;		/* Zeiger auf Speicherblock	*/
int  *icf_pgmicn;	/* Daten fÅr Programmicons */
int  *icf_daticn;	/* Daten fÅr Dateiicons */
char *icf_pgms;		/* Programmnamen */
char *icf_fils;		/* Dateitypen */

char dirty_drives[ANZDRIVES];
DISKINFO dinfo[ANZDRIVES];

const char *pgm_ver = "2.42";

int message[16];
int work_out[57],work_in [12];	 /* VDI- Felder fÅr v_opnvwk() */
int text_attrib[10];			 /* Default- Textattribute */

int ap_id;
int *aes_global;
#if MAGIXDESK == 0
int isgem2;					/* GEM 2.2 */
int iskgem;					/* KAOS - GEM */
int ismagix;					/* Multitasking - GEM */
							/* MAGIXDesk lÑuft immer unter MAGIX! */
#endif
int isfirst;					/* KAOSDESK vom Desktop gestartet */

int	vdi_handle;
int	vdi_device;
int	gl_hhchar, gl_hwchar;
static int gl_hhbox, gl_hwbox;
GRECT desk_g;			/* Maûe fÅr Hintergrund */
int	char_w, char_h;

/* Einstellungen */

DEFAULTS status;
FMT_DEFAULTS fmtstat;

OBJECT *adr_hauptmen;
OBJECT *adr_laufwe;
OBJECT *adr_icons;
OBJECT *adr_einste;
OBJECT *adr_prgrms;
OBJECT *adr_ttppar;
OBJECT *adr_format;
OBJECT *adr_fmtopt;
OBJECT *adr_cpydsk;
OBJECT *adr_about;
OBJECT *adr_cpmvdl;
OBJECT *adr_datinf;
OBJECT *adr_ordinf;


/********** Objekte verstecken *********/

void objs_hide(OBJECT *tree, ...)
{
	va_list argpoint;
	int	objnr;

	va_start(argpoint, tree);
	for (;;)
	{
		objnr = va_arg(argpoint, int);
		if (objnr == 0)
			break;
		tree[objnr].ob_flags |= HIDETREE;
	}

	va_end(argpoint);
}

/********** Objekte hervorholen *********/

void objs_unhide(OBJECT *tree, ...)
{
	va_list argpoint;
	int	objnr;

	va_start(argpoint, tree);
	for (;;)
	{
		objnr = va_arg(argpoint, int);
		if (objnr == 0)
			break;
		tree[objnr].ob_flags &= ~HIDETREE;
	}

	va_end(argpoint);
}

/***************************************/


/********** Objekt selektiert? *********/

int selected(OBJECT *tree, int which)

{ return (tree[which].ob_state & SELECTED) ? 1 : 0; }

/******* Objekt deselektieren **********/

void ob_dsel(OBJECT *tree, int which)

{ tree[which].ob_state &= ~SELECTED; }

/******* Objekt selektieren/deselektieren **********/

void ob_sel_dsel(OBJECT *tree, int which, int sel)

{
	if (sel)
		tree[which].ob_state |=  SELECTED;
	else
		tree[which].ob_state &= ~SELECTED;
}

/***** Objekt selektieren **************/

void ob_sel(OBJECT *tree, int which)

{ tree[which].ob_state |= SELECTED; }


/****************************************************************
*
****************************************************************/

static void open_work(void)
{
	register int i;

	for (i = 0; i < 10; work_in[i++] = 1)
		;
	work_in[10]=2;                     /* Rasterkoordinaten */
	v_opnvwk(work_in, &vdi_handle, work_out);
	vswr_mode(vdi_handle,MD_REPLACE);      /* Replace- Modus */
	vsf_color(vdi_handle,WHITE);           /* FÅllfarbe weiû */
	vsf_interior(vdi_handle,SOLID);        /* komplett ausfÅllen */
	vqt_attributes(vdi_handle, text_attrib);
	Cursconf(0,0);
}


/****************************************************************
*
* gibt Nummer des "Fensters" zurÅck, in dem selektierte Objekte
* liegen.
* RÅckgabe : *rwnr = 1..ANZFENSTER		Fensternummer
*            		 0				Desktop
*		   		<0				kein Objekt selektiert
*		   *objn =  Nummer des ersten selektierten Objekts oder undef.
*
*            icsel = 0	kein Objekt selektiert
*			    = 1   genau ein Objekt selektiert
*                  = -1  mehrere Objekte selektiert
*
****************************************************************/

int icsel(int *rwnr, int *objn)
{
	OBJECT *tree;
	register int wnr,i;


	*rwnr = -2;
	for	(wnr = 0; wnr <= ANZFENSTER; wnr++)
	{
		if (wnr && !fenster[wnr].path[0])
			continue;				/* Fenster unbenutzt */
		tree = fenster[wnr].pobj;
		if ((i = (tree -> ob_head)) > 0)
		{
			for	(; i <= (tree -> ob_tail); i++)
			{
				if (!(tree[i].ob_flags & HIDETREE) && selected(tree, i))
				{
					if (*rwnr != -2)	/* noch einer ! */
						return -1;
					*rwnr = wnr;		/* erster gefundener */
					*objn = i;
				}
			}
		}
	}

	return (*rwnr != -2);						/* nix gefunden */
}


/****************************************************************
*
* Stellt fest, ob sich ein Koordinatenpunkt innerhalb eines
* GRECT befindet.
*
****************************************************************/

int in_grect(int x, int y, GRECT *g)
{
	return x >= (g->g_x) && x < (g->g_x+g->g_w) &&
		  y >= (g->g_y) && y < (g->g_y+g->g_h);
}


/****************************************************************
*
* Malt ein Unterobjekt. Wenn es sich um eine Zahl handelt, wird
* sie ausgegeben, sonst ggf. der String
*
****************************************************************/

void subobj_draw(OBJECT *tree, int obj, int n, char *s)
{
	GRECT g;
	char *z;
	void objc_grect(OBJECT *tree, int objn, GRECT *g);


	z = tree[obj].ob_spec.free_string;
	if (s)
	{
		strcpy(z, s);
	}
	else
	if (n >= 0)
	{
		ultoa(n, z, 10);
	}
	objc_grect(tree, obj, &g);
	objc_draw(tree, 0, MAX_DEPTH, &g);
}


/****************************************************************
*
* Rechnet window_handle <whdl> in Fensternummer um
*
****************************************************************/

int hdl_to_fnr(int whdl)
{
	register int i;

	for (i = 0; i <= ANZFENSTER; i++)
		if ((fenster[i].handle >= 0) && (fenster[i].handle == whdl))
			return i;

	return -2;
}


/****************************************************************
*
* Aktiviert/Desaktiviert MenÅeintrÑge je nach Kontext.
* Und zwar "zeige Info" und "formatiere"
*
****************************************************************/

static void modify_menu(void)
{
	static int last_sorttyp = -1;
	static int last_showtyp = -1;
	static int last_info = -1;
	static int last_loesch = -1;
	static int last_format = -1;
	static int last_1col = -1;
	static int last_isg = -1;
	static int last_isd = -1;
	static int last_isz = -1;
	static int last_mytop = -1;
	static int last_mytop0 = -1;
	static int last_anw = -1;
	static int last_points = -1;
	static int last_blittr = -1;
	int top;
	int icn,info,loesch,format,wnr,sicn,mytop,mytop0,anw,typ;


	top = top_wnr();
	icn = icsel(&wnr,&sicn);
	if (icn != 1)
	{
		info = format = anw = FALSE;
	}
	else
	{
		obj_typ(wnr, sicn, NULL, NULL, &typ);
		format = (wnr == 0 && icon[sicn-1].icontyp <= I_RAMDSK);
		info = TRUE;
		anw = ((typ & 0x5f) == 'P');
	}

	mytop = (0 < top);
	mytop0 = (0 <= top);

	if (mytop != last_mytop)
	{
		menu_ienable(adr_hauptmen, M_NEUORD, mytop);
		menu_ienable(adr_hauptmen, M_SCHLFN, mytop);
		menu_ienable(adr_hauptmen, M_MASKE, mytop);
		last_mytop = mytop;
	}

	loesch = (mytop && icn != 0 && wnr == top);
	if (loesch != last_loesch)
	{
		menu_ienable(adr_hauptmen, M_LOESCH, loesch);
		last_loesch = loesch;
	}

	if (mytop0 != last_mytop0)
	{
		menu_ienable(adr_hauptmen, M_SELALL, mytop0);
		last_mytop0 = mytop0;
	}

	if (anw != last_anw)
	{
		menu_ienable(adr_hauptmen, M_ANWNDG, anw);
		last_anw = anw;
	}

	if (info != last_info)
	{
		menu_ienable(adr_hauptmen, M_INFO,   info);
		menu_ienable(adr_hauptmen, M_OPEN,   info);
		last_info = info;
	}

	if (format != last_format)
	{
		menu_ienable(adr_hauptmen, M_FORMAT, format);
		last_format = format;
	}

	if (status.is_1col != last_1col)
	{
		menu_icheck(adr_hauptmen, M_SPALTN, status.is_1col);
		last_1col = status.is_1col;
	}

	if (status.is_groesse != last_isg)
	{
		menu_icheck(adr_hauptmen, M_ZGROES, status.is_groesse);
		last_isg = status.is_groesse;
	}

	if (status.is_datum != last_isd)
	{
		menu_icheck(adr_hauptmen, M_ZDATUM, status.is_datum);
		last_isd = status.is_datum;
	}

	if (status.is_zeit != last_isz)
	{
		menu_icheck(adr_hauptmen, M_ZZEIT,  status.is_zeit);
		last_isz = status.is_zeit;
	}

	if (status.blitstate != last_blittr)
	{
		menu_icheck(adr_hauptmen, M_BLITTR, status.blitstate);
		last_blittr = status.blitstate;
	}

	if (status.sorttyp != last_sorttyp)
	{
		char *t;

		switch(status.sorttyp)
		{
#if LANGUAGE == LANG_GER
			case M_SNAME:	t = "     NAMEN";	break;
			case M_SDATUM: t = "     DATUM";	break;
			case M_SGROES: t = "     GRôûE";	break;
			case M_STYP:	t = "       TYP";	break;
			case M_SNICHT: t = "UNSORTIERT";	break;
#elif LANGUAGE == LANG_ENG
			case M_SNAME:	t = "     NAMES";	break;
			case M_SDATUM: t = "      DATE";	break;
			case M_SGROES: t = "      SIZE";	break;
			case M_STYP:	t = "      TYPE";	break;
			case M_SNICHT: t = "  UNSORTED";	break;
#endif
		}
		subobj_draw(adr_hauptmen, MM_SMODE, -1, t);
		if (last_sorttyp > 0)
			menu_icheck(adr_hauptmen, last_sorttyp, FALSE);
		menu_icheck(adr_hauptmen, status.sorttyp, TRUE);
		last_sorttyp = status.sorttyp;
	}

	if (status.showtyp != last_showtyp)
	{
		if (last_showtyp > 0)
			menu_icheck(adr_hauptmen, last_showtyp, FALSE);
		menu_icheck(adr_hauptmen, status.showtyp, TRUE);
		last_showtyp = status.showtyp;
	}

	if (status.points != last_points)
	{
		if (last_points > 0)
			menu_icheck(adr_hauptmen, last_points, FALSE);
		menu_icheck(adr_hauptmen, status.points, TRUE);
		last_points = status.points;
	}
}


/****************************************************************
*
* Ermittelt die Zeichenhîhe
*
****************************************************************/

void get_char_dim(void)
{
	int h, dummy;

	switch(status.points)
	{
		case M_10_PKT: h = 10; break;
		case M_9_PKT:  h =  9; break;
		case M_8_PKT:  h =  8; break;
	}

	vst_point(vdi_handle, h, &dummy, &dummy, &char_w, &char_h);
	/* linksbÅndig, Zeichenzellenoberkante */
	vst_alignment(vdi_handle, 0, 5, &dummy, &dummy);
}


#if MAGIXDESK == 0
/****************************************************************
*
* Beendet KAOSDESK endgÅltig.
*
* code = 1: nach DESKTOP
* code = 2: nach DOS
*
****************************************************************/

void _ende(int code)
{
	close_all_wind();
	wind_update(FALSE);
	v_clsvwk(vdi_handle);
	rsrc_free();
	if (code != 1)
		shel_write(0, 0, 2, "", "");
	appl_exit();
	Pterm0();
}


/****************************************************************
*
****************************************************************/

void ende(void)
{
	int ret;


#if LANGUAGE == LANG_GER
	ret = form_alert(1, "[2][Verlasse KAOSDESK nach:][DESKTOP|DOS|Abbruch]");
#elif LANGUAGE == LANG_ENG
	ret = form_alert(1, "[2][Leave KAOSDESK for:][DESKTOP|DOS|Cancel]");
#endif
	if (ret == 3)
		return;

	/* Unter GEM 2 wird einfach das Desktop wieder als Default- */
	/* applikation angemeldet und KAOSDESK verlassen			*/
	/* -------------------------------------------------------- */

	if (isgem2)
	{
		char	path[128];
		char	*endp;


		strcpy(path, "DESKTOP.APP");
		if (!shel_find(path))
			return;				/* Erfolglos */
		endp = strrchr(path, '\\');
		if (endp)
			*(endp+1) = EOS;
		else
			path[0] = EOS;
		shel_wdef("DESKTOP.APP", path);
	}

	/* Unter KAOS- GEM (ab KAOS 1.4.1) kann man mit einem		*/
	/* Nullstring als Defaultshell das Atari- Desktop reakt.	*/
	/* Der Shellmananger Åbernimmt das fÅr andere Versionen	*/
	/* -------------------------------------------------------- */

	else
	{
		shel_wdef("", "");
	}

	_ende(ret);
}
#endif


/****************************************************************
*
* FÅhrt Reaktionen auf AnwÑhlen von MenÅeintrÑgen aus.
*
****************************************************************/

static void do_menu(int title, int entry)
{
	int wnr, obj;


	switch(entry)
	{
     	case M_ABOUT:
     		dial_about();
     		break;

     	case M_OPEN:
     		if (1 == icsel(&wnr, &obj))
     			dclick(wnr, obj, K_CTRL);
			break;

		case M_INFO:
			dial_info();
			break;

		case M_LOESCH:
			if (top_wnr() > 0)
				dl_icns(top_wnr());
			break;

		case M_NEUORD:
			dial_neuord();
			break;

		case M_SCHLFN:
			close_immed(top_wnr());
			break;

		case M_SELALL:
			sel_all(top_wnr());
			break;

		case M_DCOPY:
			dial_cpydsk();
			break;

		case M_FORMAT:
			dial_format();
			break;

#if MAGIXDESK == 0
		case M_ENDE:
			ende();break;
#endif
		case M_ABILDR:
		case M_ATEXT:
			if (status.showtyp != entry)
			{
				status.showtyp = entry;
				upd_show();
			}
			break;

		case M_10_PKT:
		case M_9_PKT:
		case M_8_PKT:
			if (status.points != entry)
			{
				status.points = entry;
				get_char_dim();
				if (status.showtyp == M_ATEXT)
					upd_show();
			}
			break;

		case M_SPALTN:
			status.is_1col = !status.is_1col;
			upd_col();
			break;

		case M_ZGROES:
			status.is_groesse = !status.is_groesse;
			upd_is();
			break;

		case M_ZDATUM:
			status.is_datum = !status.is_datum;
			upd_is();
			break;

		case M_ZZEIT:
			status.is_zeit = !status.is_zeit;
			upd_is();
			break;

		case M_SNAME:
		case M_SDATUM:
		case M_SGROES:
		case M_STYP:
		case M_SNICHT:
			if (status.sorttyp != entry)
			{
				status.sorttyp = entry;
				upd_sort();
			}
			break;

		case M_MASKE:
			dial_maske();
			break;

		case M_LAUFWE:
			dial_laufwe();
			break;

		case M_ANWNDG:
			dial_anwndg();
			break;

		case M_EINSTE:
			dial_einste();
			break;

		case M_PRGRMS:
			dial_prgrms();
			break;

		case M_ARBSIC:
			dial_arbsic();
			break;

		case M_BLITTR:
			status.blitstate = !status.blitstate;
			Blitmode(status.blitstate);
			break;
	}

	menu_tnormal(adr_hauptmen, title, 1);
}


/****************************************************************
*
* Laden und Initialisieren der Ressource- Datei.
*
****************************************************************/

static void _rsrc_load(char *fname)
{
	if (!rsrc_load(fname))
	{
#if LANGUAGE == LANG_GER
		form_alert(1, "[3][Kann \".RSC\"- Datei|nicht finden][Abbruch]");
#elif LANGUAGE == LANG_ENG
		form_alert(1, "[3][Cannot find \".RSC\" file][Cancel]");
#endif
		appl_exit();
		Pterm((int) EFILNF);
	}
}


/****************************************************************
*
****************************************************************/

static void get_rsc(void)
{
	register int i,lowres;
#if MAGIXDESK
	register OBJECT *image;
	int isfirst;
#endif


	lowres = (desk_g.g_w/gl_hwchar < 60) ||
				 (K_CTRL+K_ALT == (K_CTRL+K_ALT & Kbshift(-1)));
#if MAGIXDESK
	isfirst = (*((void **) (aes_global+7)) == NULL);
	if (isfirst)								/* erster Start */
	{
		if (lowres)
		{
			_rsrc_load("magxdesl.rsc");
			rsrc_gaddr(0, HAUPTMEN, &adr_hauptmen);
		}
		else
		{
			adr_hauptmen = NULL;
		}

		_rsrc_load("magxdesk.rsc");
		if (!adr_hauptmen)
			rsrc_gaddr(0, HAUPTMEN, &adr_hauptmen);
	}
#else
	_rsrc_load((lowres) ? "kdeskl.rsc" : "kaosdesk.rsc");
	rsrc_gaddr(0, HAUPTMEN, &adr_hauptmen);
#endif
	load_icons();
	rsrc_gaddr(0, T_ICONS,  &adr_icons);
	rsrc_gaddr(0, T_ICNCHG, &adr_laufwe);
	rsrc_gaddr(0, T_EINSTE, &adr_einste);
	rsrc_gaddr(0, T_PRGRMS, &adr_prgrms);
	rsrc_gaddr(0, T_TTPPAR, &adr_ttppar);
	rsrc_gaddr(0, T_FORMAT, &adr_format);
	rsrc_gaddr(0, T_FMTOPT, &adr_fmtopt);
	rsrc_gaddr(0, T_CPYDSK, &adr_cpydsk);
	rsrc_gaddr(0, T_ABOUT,  &adr_about);
	rsrc_gaddr(0, T_CPMVDL, &adr_cpmvdl);
	rsrc_gaddr(0, T_DATINF, &adr_datinf);
	rsrc_gaddr(0, T_ORDINF, &adr_ordinf);
	ob_dsel(adr_hauptmen, MM_DATEI);
	menu_ienable(adr_hauptmen, M_BLITTR, Blitmode(-1) & 2);
	/* Diskname fÅrs Formatieren auf "" setzen 	*/
	adr_format[FORMAT_T].ob_spec.tedinfo->te_ptext[0] = EOS;

	/* Icongrîûen fÅr Auflîsungen korrigieren */
	for	(i = I_FLPDSK; i <= I_BTCHDA; i++)
	{
		(adr_icons+i) -> ob_width  = 72;
		(adr_icons+i) -> ob_height = 36;
	}

	/* Icons im Dialog "Laufwerk anmelden" installieren */
	for	(i = I_FLPDSK; i <= I_PAPIER; i++)
	{
		adr_laufwe[ICNCHG_F + i - 1].ob_type   = G_ICON;
		adr_laufwe[ICNCHG_F + i - 1].ob_state  = NORMAL;
		adr_laufwe[ICNCHG_F + i - 1].ob_width  = adr_icons[i].ob_width;
		adr_laufwe[ICNCHG_F + i - 1].ob_height = adr_icons[i].ob_height;
		adr_laufwe[ICNCHG_F + i - 1].ob_spec   = adr_icons[i].ob_spec;
	}

#if MAGIXDESK
	if (!isfirst)
		return;
	/* Gruppenrahmen anpassen */
	i = gl_hhchar >> 1;

	adr_einste[EINST_S1].ob_y		+= i;
	adr_einste[EINST_S1].ob_height	+= i;
	adr_einste[EINST_S2].ob_y		+= i;
	adr_einste[EINST_S2].ob_height	+= i;
	adr_einste[EINST_R1].ob_y -= i;
	adr_einste[EINST_R2].ob_y -= i;
	adr_cpydsk[CPYDS_R1].ob_y -= i;
	adr_format[FORMT_R1].ob_y -= i;
	adr_cpmvdl[CPMVD_R1].ob_y -= i;
	adr_datinf[FI_R1   ].ob_y -= i;
	adr_ordinf[OI_R1   ].ob_y -= i;

	image = adr_about + ABOU_IMG;
	image->ob_height = image->ob_spec.bitblk->bi_hl;
	while ((image->ob_y + image->ob_height > (adr_about + ABOU_OS) -> ob_y) && (image->ob_y > 0))
	{
		image->ob_y--;
	}

	strcpy(adr_about[ABOU_VER].ob_spec.tedinfo->te_ptext + 11, pgm_ver);
	adr_about[ABOU_OS].ob_spec.free_string = os_ver_s;
	i = adr_about->ob_width;
	i -= (int) strlen(os_ver_s) * gl_hwchar;
	i /= 2;
	if (i < 0)
		i = 0;

	adr_about[ABOU_OS].ob_x = i;
	adr_hauptmen[M_ENDE].ob_state |= DISABLED;

	/* Uhrzeit in der MenÅleiste an den rechten Rand */
	if (lowres)
	{
		objs_hide(adr_hauptmen, MM_TIMES, 0);
	}
	else
	{
		adr_hauptmen[MM_TIMES].ob_x = desk_g.g_w - 5 * gl_hwchar;
	}

#else
	strcpy(adr_about[ABOU_VER].ob_spec.free_string + 21, pgm_ver);

	/* Programmparameter auf "" setzen 	*/
	adr_ttppar[TTPPAR_1].ob_spec.tedinfo->te_ptext[0] = EOS;
	adr_ttppar[TTPPAR_2].ob_spec.tedinfo->te_ptext[0] = EOS;

	/* Dialog "Einstellungen" fÅr KAOS/TOS bzw. MAGIX anpassen */
	if (!ismagix)
	{
		adr_einste[EINST_TX].ob_flags = HIDETREE;
		adr_einste[EINST_BD].ob_state |= DISABLED;
		adr_einste[EINS_BDS].ob_state |= DISABLED;
	}
	/* Uhrzeit in der MenÅleiste an den rechten Rand */
	if (isgem2)
		objs_hide(adr_hauptmen, MM_TIMES, MM_SMODE, 0);
	else
		adr_hauptmen[MM_TIMES].ob_x = desk_g.g_w - 5 * gl_hwchar;
#endif
}


/****************************************************************
*
* Initialisierung von MAGIXDESK (AES,RSC,VDI,Fenster,Icons,MenÅ).
* MAGIXDESK wird immer im eigenen Verzeichnis gestartet.
*
****************************************************************/

#if MAGIXDESK
void anfang(void)
{
	register int i;
	char srcmd[128],srtail[128];
	SHELTAIL *sht;


	for	(i = 0; i < 16; i++)
		dirty_drives[i] = FALSE;

	/* Applikation beim AES anmelden und Version feststellen */
	/* ----------------------------------------------------- */

	if ((ap_id = appl_init()) < 0)
		Pterm(-1);

	vdi_handle = graf_xhandle(&gl_hwchar, &gl_hhchar, &gl_hwbox, &gl_hhbox,
						 &vdi_device);
	aes_global = _GemParBlk.global;

#if MAGIXDESK == 0
	if (!(_GemParBlk.intin[0] == 'KA') && (_GemParBlk.intin[1] == 'OS'))
		Pterm(-1);
#endif

	wind_get_grect(SCREEN, WF_WORKXYWH, &desk_g);

	/* TOS- Version und GEM- Variablen fÅr spÑter merken */
	/* ------------------------------------------------- */

	get_syshdr();		/* bestimmt aesvars und hicol */
	shel_read(srcmd, srtail);
#if DEBUG == 0
	if (srcmd[0])	/* Name muû leer sein */
		Pterm(-1);
	sht = (SHELTAIL *) srtail;
	if (sht->dummy != 0 || sht->magic != 'SHEL')
		Pterm(-1);
	isfirst = sht->isfirst;
#endif
	get_rsc();

	/* VDI - Workstationhandle holen */
	/* ----------------------------- */

	open_work();

	wind_update(BEG_UPDATE);

	/* Koordinaten des Desktop- Fensters initialisieren */
	/* ------------------------------------------------ */

	fenster[0].handle = SCREEN;
	fenster[0].out = desk_g;
	fenster[0].in = fenster[0].out;
	fenster[0].pmydta	= NULL;
	fenster[0].picb 	= NULL;
	fenster[0].pstr	= NULL;

	/* INF- Datei laden oder Defaults setzen */
	/* ------------------------------------- */

	load_status(0);
	load_status(1);

	open_all_wind();

	if (status.dnam_init)
	{
		for	(i = 0; i < n_icons; i++)
		{
			register int j;
			int  drv;
			char name[20];
	
			if (icon[i].icontyp != I_HRDDSK &&
				 icon[i].icontyp != I_RAMDSK)
				nexticn:
				continue;
			drv = icon[i].isdisk;
			for	(j = 1; j <= ANZFENSTER; j++)
			{
				if (fenster[j].path[0] == drv)
					goto nexticn;
			}
			drv -= 'A';
			read_dname(drv, name);
			set_dname(drv, name);
		}
	}

	wind_update(END_UPDATE);
}

#else

void anfang(void)
{
	register int i;
	char srcmd[128],srtail[128];



	for	(i = 0; i < 16; i++)
		dirty_drives[i] = FALSE;

	/* Applikation beim AES anmelden und Version feststellen */
	/* ----------------------------------------------------- */

	if ((ap_id = appl_init()) < 0)
		Pterm(1);

	vdi_handle = graf_xhandle(&gl_hwchar, &gl_hhchar, &gl_hwbox, &gl_hhbox,
						 &vdi_device);
	aes_global = _GemParBlk.global;
	iskgem = (_GemParBlk.intin[0] == 'KA') && (_GemParBlk.intin[1] == 'OS');
	isgem2 = (aes_global[0] >= 0x0200 && aes_global[0] < 0x0300) && (!iskgem);
	if (!iskgem)
		vdi_device = Getrez() + 2;
	wind_get_grect(SCREEN, WF_WORKXYWH, &desk_g);

	/* TOS- Version und GEM- Variablen fÅr spÑter merken */
	/* ------------------------------------------------- */

	get_syshdr();	/* bestimmt ismagix */

	/* Erkennen, ob vom Desktop gestartet, und Pfad setzen */
	/* --------------------------------------------------- */

	shel_read(srcmd, srtail);
	if (strcmp(get_name(srcmd), "KAOSDESK.PRG"))
		srcmd[0] = EOS;		/* ungÅltig */
	if (srcmd[0] && srcmd[1] == ':')
	{
		char *sl;

		Dsetdrv(srcmd[0] - 'A');
		if (NULL != (sl = strrchr(srcmd, '\\')))
		{
			*sl = EOS;
			Dsetpath(srcmd);
			*sl = '\\';
		}
	}

	/* Laufwerk fÅr "KAOSDESK.INF" feststellen */
	/* --------------------------------------- */

	inf_name[0] += (char) Dgetdrv();

	/* Ressourcedatei laden */
	/* -------------------- */

	get_rsc();

	/* Feststellen, ob KAOSDESK vom Desktop gestartet wurde	 */
	/* oder ob es nach Beendigung eines Prgramms nachgeladen	 */
	/* wurde: unter GEM 2 existiert die Nachfrage per shel_rdef, */
	/* unter KAOS und SHELMAN wird die Kommandozeile gefragt	 */
	/* --------------------------------------------------------- */

	if (isgem2)
	{
		char *namep;
		char path[128],cmd[128];

		shel_rdef(cmd, path);
		isfirst = strcmp(cmd, "KAOSDESK.PRG");

		namep = strrchr(srcmd, '\\');
		if (namep)
		{
			strcpy(cmd, namep+1);
			*namep = EOS;
		}
		else
		{
			strcpy(cmd, srcmd);
			srcmd[0] = EOS;
		}
		shel_wdef(cmd, srcmd);
	}
	else
	{
		SHELTAIL *sht;
		long err;


		sht = (SHELTAIL *) srtail;
		if (sht->dummy == 0 && sht->magic == 'SHEL')
		{
			isfirst = sht->isfirst;
			err = (sht->wasgr || ismagix) ? 0L : sht->lasterr;
		}
		else
		{
			isfirst = TRUE;
			shel_wdef(srcmd, "");
			err = 0L;
		}

		if (!isfirst)
		{
			if (err)
			{
				if (0 == (err & 0xffff0000L))
					err_file = (char *) -1L;
				err_alert(err);	/* letzter Fehler */
			}
		}
	}

	/* VDI - Workstationhandle holen */
	/* ----------------------------- */

	open_work();

	wind_update(BEG_UPDATE);

	/* Koordinaten des Desktop- Fensters initialisieren */
	/* ------------------------------------------------ */

	fenster[0].handle = SCREEN;
	fenster[0].out = desk_g;
	fenster[0].in = fenster[0].out;
	fenster[0].pmydta	= NULL;
	fenster[0].picb 	= NULL;
	fenster[0].pstr	= NULL;
	fenster[0].shownum  = fenster[0].realnum = n_icons;


	/* INF- Datei laden oder Defaults setzen */
	/* ------------------------------------- */

	reload_status(isfirst, -1);

	if (status.dnam_init)
	{
		for	(i = 0; i < n_icons; i++)
		{
			register int j;
			int  drv;
			char name[20];

			if (icon[i].icontyp != I_HRDDSK && icon[i].icontyp != I_RAMDSK)
			{
				nexticn:
				continue;
			}

			drv = icon[i].isdisk;

			for	(j = 1; j <= ANZFENSTER; j++)
			{
				if (fenster[j].path[0] == drv)
					goto nexticn;
			}

			drv -= 'A';
			read_dname(drv, name);
			set_dname(drv, name);
		}
	}

	wind_update(END_UPDATE);
}
#endif


/****************************************************************
*
* Malt das Fenster mit Nummer <wnr> im Bereich
* neu_x,neu_y,neu_b,neu_h  nach.
*
****************************************************************/

void redraw(int wnr, GRECT *neu)
{
	GRECT g;
	register int whdl;

	if (wnr < 0)
		return;						/* ungÅltiges Handle */
	if (wnr > 0 && !fenster[wnr].path[0])
		return;						/* Fenster geschlossen */

	whdl = fenster[wnr].handle;
	Mgraf_mouse(M_OFF);
	wind_get_grect(whdl, WF_FIRSTXYWH, &g);

	do
	{
		if (rc_intersect(neu, &g))
		{
			objc_draw(fenster[wnr].pobj, 0, 1, &g);
		}
		wind_get_grect(whdl, WF_NEXTXYWH, &g);
	}
	while (g.g_w > 0);		/* bis Rechteckliste vollstÑndig */

	Mgraf_mouse(M_ON);
}


/****************************************************************
*
* Tastenbehandlung
*
****************************************************************/

static int last_wasopen = -1;
static int last_drvopen = -1;

static void handle_key(EVNT *w_ev)
{
	int drv;
	register int i, scancode;
	int title, entry;


	title = -1;
	drv = w_ev->key & 0xff;
	scancode = (w_ev->key >> 8) & 0xff;

	/* ^SH-A..P */
	if ((w_ev->kstate & K_CTRL) && (w_ev->kstate & (K_RSHIFT + K_LSHIFT)))
	{
		#if MAGIXDESK
		if (drv >= 1 && drv <= 16)
		{
			reload_status(drv - 1);
		}
		#else
		if (drv >= 1 && drv <= 16)
		{
			reload_status(TRUE, drv - 1);
		}
		#endif
		w_ev->key = drv = 0;
	}

	/* Alt-n */
	if ((w_ev->kstate & K_ALT) && (unsigned int) w_ev->key >= 0x7800 &&
	    (unsigned int) w_ev->key <= 0x8100)
	{
		i = scancode - 0x77;
		if (i != vdi_device);
			change_res(i, 0);
		w_ev->key = 0;
	}

	/* ^Fn */
	if ((w_ev->kstate & K_CTRL) && (w_ev->key >= 0x3b00) &&
		(w_ev->key <= 0x4600))
	{
		i = scancode - 0x3b;
		starten("", '#\0'+i+4, 0, 0, w_ev->kstate & K_ALT);
		w_ev->key = 0;
	}

	/* A..P */
	for	(i = 0; i < n_icons; i++)
		if (icon[i].isdisk == toupper(drv))
			break;
	if ((i < n_icons) && drv)
	{
		if (drv <= 'Z')
		{
			dsel_all();
			ob_sel(fenster[0].pobj, i+1);
			obj_malen(0, i+1);
		}

		if (last_wasopen == TRUE && last_drvopen == drv)
		{
			last_wasopen = FALSE;
			if (drv <= 'Z')
			{
				dclick(0, i + 1, 0);
			}
			else
			{
				int wnr;

				wnr = top_wnr();
				if (wnr > 0 && fenster[wnr].path[0])
				{
					fenster[wnr].path[0] = toupper(drv);
					fenster[wnr].path[3] = EOS;
					upd_path(wnr, TRUE);
				}
			}
		}
		else
		{
			last_wasopen = TRUE;
			last_drvopen = drv;
		}
	}
	else
	{
		last_wasopen = FALSE;
	}

	switch (w_ev->key)
	{
/* Retrn */
		case 0x1c0d:
/* Enter */
		case 0x720d:
/* ^O    */
		case 0x180f:
			title = MM_DATEI;
			entry = M_OPEN;
			break;

/* Tab   */
		case 0x0f09:
/* ^I    */
		case 0x1709:
			title = MM_DATEI;
			entry = M_INFO;
			break;

/* ^L    */
		case 0x260c:
			title = MM_DATEI;
			entry = M_LOESCH;
			break;

/* ^N    */
		case 0x310e:
			title = MM_DATEI;
			entry = M_NEUORD;
			break;

/* ^U    */
		case 0x1615:
			title = MM_DATEI;
			entry = M_SCHLFN;
			break;

/* ^A    */
		case 0x1e01:
			title = MM_DATEI;
			entry = M_SELALL;
			break;

/* ^K    */
		case 0x250b:
			title = MM_DATEI;
			entry = M_DCOPY;
			break;

#if MAGIXDESK==0
/* ^Q    */
		case 0x1011:
			title = MM_DATEI;
			entry = M_ENDE;
			break;
#endif

/* Alt-B */
		case 0x3000:
			title = MM_ANZEI;
			entry = M_ABILDR;
			break;

/* Alt-T */
		case 0x1400:
			title = MM_ANZEI;
			entry = M_ATEXT;
			break;

/* Alt-H */
		case 0x2300:
			title = MM_ANZEI;
			entry = M_SPALTN;
			break;

/* Alt-G */
		case 0x2200:
			title = MM_ANZEI;
			entry = M_ZGROES;
			break;

/* Alt-D */
		case 0x2000:
			title = MM_ANZEI;
			entry = M_ZDATUM;
			break;

/* Alt-Z */
		case 0x1500:
			title = MM_ANZEI;
			entry = M_ZZEIT;
			break;

/* F1 */
		case 0x3B00:
			title = MM_SORT;
			entry = M_SNAME;
			break;

/* F2 */
		case 0x3C00:
			title = MM_SORT;
			entry = M_SDATUM;
			break;

/* F3 */
		case 0x3D00:
			title = MM_SORT;
			entry = M_SGROES;
			break;

/* F4 */
		case 0x3E00:
			title = MM_SORT;
			entry = M_STYP;
			break;

/* F5 */
		case 0x3F00:
			title = MM_SORT;
			entry = M_SNICHT;
			break;

/* Alt-M */
		case 0x3200:
			title = MM_SORT;
			entry = M_MASKE;
			break;

/* Alt-L */
		case 0x2600:
			title = MM_OPTIO;
			entry = M_LAUFWE;
			break;

/* Alt-A */
		case 0x1E00:
			title = MM_OPTIO;
			entry = M_ANWNDG;
			break;

/* Alt-E */
		case 0x1200:
			title = MM_OPTIO;
			entry = M_EINSTE;
			break;

/* Alt-P */
		case 0x1900:
			title = MM_OPTIO;
			entry = M_PRGRMS;
			break;

/* Alt-S */
		case 0x1F00:
			title = MM_OPTIO;
			entry = M_ARBSIC;
			break;

/* SPACE	*/
		case 0x3920:
			dsel_all();
			break;

/* SH-INS	*/
		case 0x5230:
			status.resident = TRUE;
			break;

/* SH-DEL	*/
		case 0x537f:
			status.resident = FALSE;
			break;

/* Einfg */
		case 0x5200:
			fulled(top_wnr());
			break;

/* Pos1  */
		case 0x4700:
			arrowed(top_wnr(), -1);
			break;

/* SH-Pos1 */
		case 0x4737:
			arrowed(top_wnr(), -2);
			break;

/* Cursor up */
		case 0x4800:
			arrowed(top_wnr(), WA_UPLINE);
			break;

/* Cursor down */
		case 0x5000:
			arrowed(top_wnr(), WA_DNLINE);
			break;

/* SH-Cursor up */
		case 0x4838:
			arrowed(top_wnr(), WA_UPPAGE);
			break;

/* SH-Cursor dwn */
		case 0x5032:
			arrowed(top_wnr(), WA_DNPAGE);
			break;

/* Ctrl-B */
		case 0x3002:
			starten("", '#\0', 0, 0, w_ev->kstate & K_ALT);
			break;

/* Ctrl-E */
		case 0x1205:
			starten("", '#\3', 0, 0, w_ev->kstate & K_ALT);
			break;

/* Ctrl-W */
		case 0x1117:
		{
			int w,i;

			w = top_wnr();
			if (w <= 0)
				break;
			i = w;
			do
			{
				i++;
				if (i > ANZFENSTER)
					i = 1;
				if (fenster[i].path[0])
				{
					topped(fenster[i].handle);
					break;
				}
			}
			while (i != w);
		}
			break;

/* Esc */
		case 0x011b:
		{
			int n;

			wind_get_int(0, WF_TOP, &n);
			n = hdl_to_fnr(n);
			if (n < 0)
				break;
			n = fenster[n].path[0];
			if (n < 'A' || n > 'P')
				break;
			dirty_drives[n - 'A'] = TRUE;
		}
			break;

/* BS */
		case 0x0e08:
		{
			int wnr;
			wnr = top_wnr();
			if (wnr > 0 && fenster[wnr].path[3])
				closed(fenster[wnr].handle, w_ev->kstate);
		}
			break;

/* Undo */
		case 0x6100:
		{
			int wnr;
			wnr = top_wnr();
			if (wnr > 0 && fenster[wnr].path[3])
			{
				fenster[wnr].path[3] = '\0';
				upd_path(wnr, FALSE);
			}
		}

	}

	if ((title >= 0) && !(((adr_hauptmen + entry) -> ob_state) & DISABLED))
	{
		menu_tnormal(adr_hauptmen, title, FALSE);
		do_menu(title, entry);
	}
}


/****************************************************************
*
* Hauptfunktion
*
****************************************************************/

void main()
{
	EVNT w_ev;
	int ev_mwhich;
	int	waitfor;
	int ltime;


	anfang();
	Mgraf_mouse(ARROW);
	for (;;)
	{
		upd_drives();
		upd_infos();
		modify_menu();

		/* Wenn ein Laufwerk mit Tastatur selektiert wurde, wird 200ms */
		/* lang auf den zweiten Klick gewartet.                        */
		/* ----------------------------------------------------------- */

		if (last_wasopen == -1 && !status.clock)
		{
			last_wasopen = FALSE;
		}

		if (last_wasopen)
		{
			ltime = 200;
			waitfor = MU_KEYBD+MU_BUTTON+MU_MESAG+MU_TIMER;
		}
		else
		{
			if (status.clock)
			{
				ltime = 10000;
				waitfor = MU_KEYBD+MU_BUTTON+MU_MESAG+MU_TIMER;
			}
			else
			{
				waitfor = MU_KEYBD+MU_BUTTON+MU_MESAG;
			}
		}

		ev_mwhich = evnt_multi(waitfor,
						  2,			/* Doppelklicks erkennen 	*/
						  1,			/* nur linke Maustaste		*/
						  1,			/* linke Maustaste gedrÅckt	*/
						  0,NULL,		/* kein 1. Rechteck			*/
						  0,NULL,		/* kein 2. Rechteck			*/
						  message,
						  ltime,		/* ms */
						  (EVNTDATA *) &(w_ev.mx),
						  &w_ev.key,
						  &w_ev.mclicks
						  );

		wind_update(BEG_UPDATE);

		/* Abfrage auf Timer- Ereignis	   */
		/* ---------------------------------- */

		if (last_wasopen == -1 || (ev_mwhich & MU_TIMER))
		{
			char times[10];
			static int lasttime = 0;
			int time;


			last_wasopen = FALSE;
#if MAGIXDESK
			if (status.clock)
#else
			if (status.clock && !isgem2)
#endif
			{
				time = Tgettime();
				time &= 0xffe0;		/* Stunden und Minuten */
				if (time != lasttime)
				{
					lasttime = time;
					time_to_str(times, time);
					times[5] = EOS;	/* Stunden und Minuten */
					subobj_draw(adr_hauptmen, MM_TIMES, -1, times);
				}
			}
		}

		/* Abfrage auf BetÑtigung einer Taste */
		/* ---------------------------------- */

		if (ev_mwhich & MU_KEYBD)
		{
			handle_key(&w_ev);
		}
		else
		{
			last_wasopen = FALSE;
		}

		/* Abfrage auf Nachricht im Nachrichtenpuffer */
		/* ------------------------------------------ */

		if (ev_mwhich & MU_MESAG)
		{
			if (message[0] == MN_SELECTED)
			{
				do_menu(message[3], message[4]);
			}
			else
			{
				switch(message[0])
				{
				     case WM_REDRAW:   redraw(hdl_to_fnr(message[3]),
				     				     (GRECT *) (message+4));
				                       break;
				     case WM_TOPPED:   topped(message[3]);break;
				     case WM_CLOSED:   closed(message[3], w_ev.kstate);break;
				     case WM_FULLED:   fulled(hdl_to_fnr(message[3]));break;
				     case WM_ARROWED: arrowed(hdl_to_fnr(message[3]), message[4]);break;
				     case WM_HSLID:	    hslid(message[3]);break;
				     case WM_VSLID:     vslid(message[3], message[4]);break;
				     case WM_SIZED:	    sized(message[3], (GRECT *) (message+4));break;
				     case WM_MOVED:     moved(message[3], (GRECT *) (message+4));break;
				     case WM_NEWTOP:   newtop(message[3]);break;
				     case WM_UNTOPPED: untopped(message[3]);break;
				     case SH_WDRAW:
				     			if ((message[3] >= 0) &&
				     				 (message[3] < ANZDRIVES))
									dirty_drives[message[3]] = TRUE;
								else
								{
									register int i;

									for	(i = 0; i < ANZDRIVES; i++)
										dirty_drives[i] = TRUE;
								}
								break;
				     }
			}
		}

		/* Abfrage auf BetÑtigung eines Mausknopfs */
		/* --------------------------------------- */

		if (ev_mwhich & MU_BUTTON)
		{
			mausknopf(w_ev.mclicks, (EVNTDATA *) &(w_ev.mx));

			/* poll mouse and keyboard state */
			graf_mkstate((EVNTDATA *) &(w_ev.mx));
			if (w_ev.mbutton & 1)
			{
				/* left button still pressed, wait for release */
				evnt_button(1, 1, 0, (EVNTDATA *) &(w_ev.mx));
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
	return (tw > tx) && (th > ty);
}


/****************************************************************
*
* DurchlÑuft alle selektierten Objekte eines Baums.
* Wenn die Funktion "tue" < 0 liefert, wird abgebrochen.
* gibt Anzahl dieser Objekte zurÅck.
*
****************************************************************/

int walk_sel(int wnr, int (*tue)(int wnr, int obj))
{
	register int i,tail,anz;
	register OBJECT *tree = fenster[wnr].pobj;


	if ((i = (tree -> ob_head)) <= 0)
		return 0;

	anz = 0;
	tail = (tree++ -> ob_tail);
	for	(; i <= tail; i++,tree++)
	{
		if (!(SELECTED&(tree->ob_state)) || (HIDETREE&(tree->ob_flags)))
			continue;
		anz++;
		if (0 > (*tue)(wnr, i))
			break;
	}

	return anz;
}


/****************************************************************
*
* Bestimmt die Begrenzung eines Objekts
*
****************************************************************/

void objc_grect(OBJECT *tree, int objn, GRECT *g)
{
	objc_offset(tree, objn, &(g -> g_x), &(g -> g_y));
	g -> g_w = (tree + objn) -> ob_width;
	g -> g_h = (tree + objn) -> ob_height;
}


/****************************************************************
*
* Bestimmt die Begrenzung aller selektierten Unterobjekte
* in absoluten Bildschirmkoordinaten.
* (rechteckige HÅlle aller selektierten Unterobjekte).
*
****************************************************************/

void tree_sel_grect(OBJECT *tree, GRECT *g)
{
	register int i,rx,ry;
	register int mx,my,mrx,mry;	/* Maxima */
	int ox,oy,tail;


	ox = tree->ob_x;
	oy = tree->ob_y;
	mx = my = INT_MAX;		/* Ecke links oben */
	mrx = mry = -INT_MAX;	/* Ecke rechts unten (auûerhalb) */

	if ((i = (tree -> ob_head)) > 0)
	{
		tail = (tree++ -> ob_tail);
		for	(; i <= tail; i++,tree++)
		{
			if ((SELECTED&(tree->ob_state)) && !(HIDETREE&(tree->ob_flags)))
			{
				rx = tree->ob_x;
				ry = tree->ob_y;
				if (rx < mx)
					mx = rx;
				if (ry < my)
					my = ry;
				rx += tree->ob_width;
				ry += tree->ob_height;
				if (rx > mrx)
					mrx = rx;
				if (ry > mry)
					mry = ry;
			}
		}
	}
	g -> g_x = mx + ox;
	g -> g_y = my + oy;
	g -> g_w = mrx - mx;
	g -> g_h = mry - my;
}


/****************************************************************
*
* éndert den Selektionsstatus aller Unterobjekte von <tree>, die
* sich im Rechteck <*g> befinden.
* Bestimmt eine mîglichst kleine rechteckige Begrenzung aller
* geÑnderten Objekte in absoluten Bildschirmkoordinaten.
* (rechteckige HÅlle aller geÑnderten selektierten Unterobjekte).
*
****************************************************************/

void grect_toggle(OBJECT *tree, GRECT *g)
{
	register int i,rx,ry;
	register int mx,my,mrx,mry;	/* Maxima */
	int tx,ty;				/* Koordinaten des Wurzelobjekts */
	int tail;
	GRECT  ob;
	register OBJECT *t = tree;
	register ICONBLK *icn;


	tx = tree -> ob_x;
	ty = tree -> ob_y;
	mx = my = INT_MAX;		/* Ecke links oben */
	mrx = mry = -INT_MAX;	/* Ecke rechts unten (auûerhalb) */

	if ((i = (t -> ob_head)) > 0)
	{
		tail = (t++ -> ob_tail);
		for	(; i <= tail; i++,t++)
		{
			if (HIDETREE & (t->ob_flags))
				continue;
			if (t -> ob_type == G_ICON)
			{
				int x = tx + t -> ob_x;
				int y = ty + t -> ob_y;

				icn = t -> ob_spec.iconblk;
				/* 1. Fall: Icon selbst */
				ob.g_x = x + icn->ib_xicon;
				ob.g_y = y + icn->ib_yicon;
				ob.g_w = icn->ib_wicon;
				ob.g_h = icn->ib_hicon;
				if (rc_intersect(g, &ob))
					goto toggle;
				/* 2. Fall: Unterschrift */
				ob.g_x = x + icn->ib_xtext;
				ob.g_y = y + icn->ib_ytext;
				ob.g_w = icn->ib_wtext;
				ob.g_h = icn->ib_htext;
			}
			/* 2. Fall: sonst (STRING bzw. G_USERDEF) */
			else
			{
				ob = *((GRECT *) (&(t->ob_x)));
				ob.g_x += tx;
				ob.g_y += ty;
			}

			if (!rc_intersect(g, &ob))
				continue;

			toggle:
			t->ob_state ^= SELECTED;
			rx = t->ob_x;
			ry = t->ob_y;
			if (rx < mx)
				mx = rx;
			if (ry < my)
				my = ry;
			rx += t->ob_width;
			ry += t->ob_height;
			if (rx > mrx)
				mrx = rx;
			if (ry > mry)
				mry = ry;
		}
	}
	g -> g_x = mx + tree->ob_x;
	g -> g_y = my + tree->ob_y;
	g -> g_w = mrx - mx;
	g -> g_h = mry - my;
}


/****************************************************************
*
* Rechnet Mausposition in Objektnummer um.
* (Ñhnlich objc_find, jedoch nur eine Ebene).
* RÅckgabe  0, falls kein Objekt unter dem Mauszeiger,
* RÅckgabe -1, falls inaktiver Hintergrund.
*
****************************************************************/

int find_obj(OBJECT *tree, int x, int y)
{
	register int i,obx,oby,obw,obh;
	register ICONBLK *icn;
	int end;


	if ((i = (tree -> ob_head)) < 0)
		return 0;

	if ((tree -> ob_flags) & HIDETREE)
		return -1;			/* etwa inaktiver Hintergrund */

	end = tree -> ob_tail;
	x -= tree -> ob_x;			/* Position der Root abziehen */
	y -= tree -> ob_y;

	for	(; i <= end; i++)
	{
		tree++;
		if ((tree->ob_flags) & HIDETREE)
			continue;
		/* 1. Fall: ICON */
		if (tree -> ob_type == G_ICON)
		{
			icn = tree->ob_spec.iconblk;
			/* 1. Fall: Icon selbst */
			obx = tree->ob_x + icn->ib_xicon;
			oby = tree->ob_y + icn->ib_yicon;
			obw = icn->ib_wicon;
			obh = icn->ib_hicon;
			if (x >= obx && x < obx+obw && y >= oby && y < oby+obh)
				return i;

			/* 2. Fall: Unterschrift */
			obx = tree->ob_x + icn->ib_xtext;
			oby = tree->ob_y + icn->ib_ytext;
			obw = icn->ib_wtext;
			obh = icn->ib_htext;
			if (x >= obx && x < obx+obw && y >= oby && y < oby+obh)
				return i;
		}
		/* 2. Fall: sonst (STRING bzw. G_USERDEF) */
		else
		{
			if (x >= tree->ob_x && x < tree->ob_x+tree->ob_width &&
				 y >= tree->ob_y && y < tree->ob_y+tree->ob_height)
				return i;
		}
	}

	return 0;
}


/****************************************************************
*
* Gibt an, ob ein Objekt ein Ziel fÅr ein Verschieben von Icons
* sein kann.
* GÅltiges Ziel sind Ordner (O_), Laufwerke (?D), Programme bzw.
* angemeldete Dateien (?P), Batchprogramme (?B),
* Papierkorb und Drucker.
*
****************************************************************/

int is_dest(int wnr, int objnr)
{
	int obtyp;
	char obtyp2;
	MYDTA *dummy;

	obj_typ(wnr, objnr, NULL, &dummy, &obtyp);
	obtyp2 = toupper((char) obtyp);
	return (obtyp2 == 'P') || (obtyp2 == 'D') || (obtyp2 == 'B') ||
		  (obtyp == '_O') || (obtyp == 'P_') || (obtyp == 'D_');
}


/****************************************************************
*
* Zeichnet die benutzerdefinierten Objekte (Texte verschiedener
* Grîûe). Textgrîûe und Ausrichtung auf Zellenoberkante ist schon
* eingestellt.
* Der Clippingbereich wird nur dann neu gesetzt, wenn sich dieser
* geÑndert hat, dadurch werden einige vs_clip()'s gespart.
*
****************************************************************/

static int clip_rect[4];

int cdecl draw_userdef(PARMBLK *p)
{
	char *t;
	register int wnr;
	int pxy[4];


	for	(wnr = 0; wnr <= ANZFENSTER; wnr++)
		if (fenster[wnr].pobj == p->pb_tree)
			goto found;

	return 0;

	found:
	pxy[0] = p->pb_xc;
	pxy[1] = p->pb_yc;
	pxy[2] = pxy[0] + p->pb_wc - 1;
	pxy[3] = pxy[1] + p->pb_hc - 1;
	t = (char *) (fenster[wnr].pstr+(p->pb_obj - 1));
	if (pxy[0] != clip_rect[0] || pxy[1] != clip_rect[1] ||
		 pxy[2] != clip_rect[2] || pxy[3] != clip_rect[3])
	{
		clip_rect[0] = pxy[0];
		clip_rect[1] = pxy[1];
		clip_rect[2] = pxy[2];
		clip_rect[3] = pxy[3];
		vs_clip(vdi_handle, TRUE, pxy);
	}

	v_gtext(vdi_handle, p->pb_x, p->pb_y, t);
	return p->pb_currstate;
}


USERBLK userblk = {draw_userdef, 0L};


/****************************************************************
*
* Setzt den Clippingbereich auf die Bildschirmgrîûe.
*
****************************************************************/

void set_deflt_clip(void)
{
	clip_rect[0] = desk_g.g_x;
	clip_rect[1] = desk_g.g_y;
	clip_rect[2] = desk_g.g_x + desk_g.g_w - 1;
	clip_rect[3] = desk_g.g_y + desk_g.g_h - 1;
	vs_clip(vdi_handle, TRUE, clip_rect);
}


/****************************************************************
*
* MenÅzeile abschalten
* Speicher fÅr Fenster freigeben
* Desktop- Hintergrund abmelden
* Fenster schlieûen und lîschen
*
****************************************************************/

int deflt_topwnr = -1;

void close_all_wind(void)
{
	register int i;
    int dummy;


	/* Default- Zeichengrîûe wieder einstellen */
	vst_height(vdi_handle, text_attrib[7], &char_w, &char_h, &dummy, &dummy);
	deflt_topwnr = top_wnr();
	menu_bar(adr_hauptmen, FALSE);

	if (poolptr)
	{
		Mfree(poolptr);
		poolptr = NULL;				/* sicher ist sicher */
	}

	wind_set_ptr_int(SCREEN, WF_NEWDESK, NULL, 0);	/* Desktop-Hintergrund */

#if MAGIXDESK == 0
	if (!isgem2 && !ismagix)
		wind_set_int(1, WF_RESVD, 0);			/* Redraw ausschalten */
#endif
	for (i = 1; i <= ANZFENSTER; i++)
		if (fenster[i].handle > 0)         /* wenn Fenster existiert... */
		{
			if (fenster[i].path[0] != EOS)		 /* ...und geîffnet   */
			{
				wind_close(fenster[i].handle);  /* ...dann schlieûen */
			}

			wind_delete(fenster[i].handle); /* ...und immer lîschen!  */
			fenster[i].handle = -1;
		}
#if MAGIXDESK == 0
	if (!isgem2 && !ismagix)
	{
		wind_set_int(0, WF_RESVD, 0);	/* Redraw ein */
	}
#endif
}


/****************************************************************
*
* Initialisierung aller Fenster, ggf. îffnen
*
****************************************************************/

void open_all_wind(void)
{
	register int i,j,set;
	long ret;

#if MAGIXDESK == 0
	set_colors();
#endif
	Mgraf_mouse(HOURGLASS);
	for	(i = 0; i < ANZDRIVES; i++)
	{
		dinfo[i].b_clsiz = 0L;		/* ungÅltig */
	}

	/* MenÅzeile auf den Bildschirm */
	/* ---------------------------- */

	menu_bar(adr_hauptmen, TRUE);

	/* Icons installieren und malen */
	/* ---------------------------- */

	fenster[0].pobj->ob_x = fenster[0].pobj->ob_y = 0;
	fenster[0].pobj->ob_height = desk_g.g_y + desk_g.g_h;
	fenster[0].pobj->ob_width  = desk_g.g_x + desk_g.g_w;
	wind_set_ptr_int(SCREEN, WF_NEWDESK, fenster[0].pobj, 0);
	exit_vt52();

	/* Fenster îffnen */
	/* -------------- */

	get_char_dim();		/* Zeichengrîûe einstellen */

	/* Speicher fÅr ANZDATEIEN Datei- EintrÑge holen */

	poolsize = ANZDATEIEN;	/* alle EintrÑge frei */

	ret = ANZDATEIEN * (sizeof(OBJECT) +
					sizeof(MYDTA)  +
					sizeof(ICONBLK) +
					sizeof(MYFNAME)
				    ) + ANZFENSTER * sizeof(OBJECT);

	poolptr = Malloc(ret);
	poolfree = poolptr;			/* frei ab Anfang */

	for	(i = 1; i <= ANZFENSTER; i++)
	{
		fenster[i].handle = -1;			/* unbenutzt */
		fenster[i].alloc_offs = -1L; 	/* kein allozierter Speicher */
		fenster[i].alloc_size = 0L; 	/* kein allozierter Speicher */
	}

	for	(set = FALSE,i = 1; i <= ANZFENSTER; i++)
	{
		j = i;
		if (i == deflt_topwnr)
		{
			j = ANZFENSTER;
			set = TRUE;
		}
		if (set && i == ANZFENSTER)
			j = deflt_topwnr;
		if (fenster[j].path[0])
		{
			ret = opn_wnd(j, TRUE);
			if (ret == ENOWND)
				ret = ENSMEM;
			err_alert(ret);
		}
	}

	set_deflt_clip();
	Mgraf_mouse(ARROW);
}
