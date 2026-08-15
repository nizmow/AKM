/*********************************************************************
*
* Dieses Modul enthÑlt das Laden und Abspeichern der Defaults
*
*********************************************************************/

#define  PGM_I_VERSION	1

#include <stdlib.h>
#include <string.h>
#include <tosdefs.h>
#include <mgx_dos.h>	/* include before tos.h */
#include <tos.h>
#include "k.h"

#if	MAGIXDESK==0
static char *infdata;
#endif


/****************************************************************
*
* Extrahiert die Icondatei aus der Resourcedatei, falls vorhanden
*
****************************************************************/

void load_icons(void)
{
	register char *buf;


	/* Nachsehen, ob Icons im RSC versteckt sind */
	/* ----------------------------------------- */

	if (NULL != (buf = _iconfile))
	{
		memcpy(&iconfile_header, buf, sizeof(ICONFILE_HEADER));
		buf += sizeof(ICONFILE_HEADER);
		icf_data = (int *) buf;
		icf_pgmicn = (int *) buf;
		icf_daticn = (int *) (buf + (iconfile_header.n_pgm_icons) * 0x70);
		icf_pgms = buf + (0x70) * 
			(iconfile_header.n_pgm_icons + iconfile_header.n_dat_icons);
		icf_fils = icf_pgms + 9 * iconfile_header.n_pgm_names;
	}
	else
		icf_data = NULL;			/* keine Icons */
}


/****************************************************************
*
* Rechnet Koordinaten linear um. Der Wert <wert> wurde bei
* Bildschirmgrîûe <old> abgespeichert, jetzt ist die
* Bildschirmgrîûe <new>.
*
****************************************************************/

void recalc(int *wert, int old, int new)
{
	unsigned long tmp;

	tmp = (unsigned long) *wert;
	tmp *= new;
	tmp /= old;
	*wert = (int) tmp;
}


/****************************************************************
*
* LÑdt bzw. setzt alle Defaults.
* pass == 0: gmemptr initialisieren, d.h. n_icons, n_apps
*            bestimmen und Speicher fÅr icon und application
*		     allozieren
* pass == 1: Status  laden
*
****************************************************************/

static char *gmemptr = NULL;
#if	MAGIXDESK
char inf_name[] = "A:\\MAGX.INF";
#else
char inf_name[] = "A:\\KAOSDESK.INF";
#endif
static char *s;
static int scnlin(void)
{
	for (;;)
	{
		if (!*s)
			return 0;
		if (*s++ != '\r')
			continue;
		if (*s++ != '\n')
			return 0;
		return 1;
	}
}


static void scnstr(char *t)
{
	while (*s == ' ')
		s++;
	while (*s != ' ' && *s != '\0' && *s != '\r' && *s != '\n')
		*t++ = *s++;
	*t = '\0';
}


static void scnbstr(char *t)
{
	while (*s == ' ')
		s++;
	while (*s != '\0' && *s != '\r' && *s != '\n')
		*t++ = *s++;
	*t = '\0';
}


static int scndez(void)
{
	while (*s == ' ')
		s++;
	return (int) strtol(s, &s, 10);
}


static char scnflg(void)
{
	char c;

	while (*s == ' ')
		s++;
	if (*s)
	{
		c = *s++ - '0';
		return c;
	}

	return 0;
}


void load_status(int pass)
{
	register int i,d,dx,dy,icw,ich;
	char dummy_s[128];
	int	i_version;
#if	MAGIXDESK
	char *inf;
	unsigned int len;
#endif
	int	old_w,old_h;
	int	ln_wind,ln_icns,ln_pgms,ln_apps;
	register long drvs;
	register int code;


	Mgraf_mouse(HOURGLASS);
	if (!pass)
	{
		if (gmemptr)
			Mfree(gmemptr);
		gmemptr = NULL;
#if	DEBUG
		icon = application = fenster[0].pobj = NULL;
#endif
	}

	if (pass)
	{

		/* zunÑchst sicherheitshalber Defaults setzen */
		/* ------------------------------------------ */

		(((adr_ttppar+TTPPAR_1)->ob_spec.tedinfo)->te_ptext)[0] = EOS;
		(((adr_ttppar+TTPPAR_2)->ob_spec.tedinfo)->te_ptext)[0] = EOS;

		status.clock		= TRUE;
		status.blitstate	= TRUE;

		status.sorttyp 	= M_SNAME;
		status.showtyp 	= M_ABILDR;
		status.points		= M_10_PKT;
		status.is_1col		= FALSE;
		status.is_groesse	= TRUE;
		status.is_datum	= TRUE;
		status.is_zeit		= TRUE;

		status.cnfm_del	= TRUE;
		status.cnfm_copy	= TRUE;
		status.mode_ovwr	= CONFIRM;
		status.check_free	= TRUE;
		status.use_fldl	= MAGIXDESK;
		status.resident	= TRUE;
		status.show_all	= FALSE;
		status.use_cache	= FALSE;
		status.dnam_init	= TRUE;

		fmtstat.tmpdrv		= 0;
		fmtstat.sides		= 2;
		fmtstat.tracks		= 80;
		fmtstat.sectors	= 9;
		fmtstat.interlv	= 1;
		fmtstat.trkincr	= 2;
		fmtstat.sidincr	= 2;
		fmtstat.clustsize	= 2;

		for	(ln_wind = 1; ln_wind <= ANZFENSTER; ln_wind++)
		{
			fenster[ln_wind].shift = 0;
			fenster[ln_wind].out.g_x = 140 + 20 * ln_wind;
			fenster[ln_wind].out.g_y = desk_g.g_y + 10 * ln_wind;
			fenster[ln_wind].out.g_w = 300;
			fenster[ln_wind].out.g_h = 10 * gl_hhchar;
			fenster[ln_wind].handle = -1;
			fenster[ln_wind].path[0] = EOS;
			fenster[ln_wind].maske[0] = EOS;
		}

		drvs = Drvmap();
		ln_icns = d = 0;
		while (drvs)
		{
			if (drvs & 1)			/* Laufwerk existiert */
			{
				icon[ln_icns].isdisk = 'A' + d;
				if (ln_icns < 2)
					icon[ln_icns].icontyp = I_FLPDSK;
				else
				if (ln_icns < 6)
					icon[ln_icns].icontyp = I_HRDDSK;
				else
					icon[ln_icns].icontyp = I_RAMDSK;
				ln_icns++;
			}
			drvs >>= 1;
			d++;
		}

		for	(d = ln_icns; ln_icns < n_icons; ln_icns++)
		{
			icon[ln_icns].isdisk = 0;
			if (ln_icns == d)
				icon[ln_icns].icontyp = I_DRUCKR;
			else
			if (ln_icns == d+1)
				icon[ln_icns].icontyp = I_PAPIER;
			else
				icon[ln_icns].icontyp = 0;
		}
	}	/* END IF (pass) */
	else
	{
		ln_wind = 1;
		ln_icns = 40;
		ln_apps = 30;
	}

	/* Jetzt die Inf- Datei aus dem AES- Puffer holen */
	/* ---------------------------------------------- */

#if	MAGIXDESK
	len = shel_get(NULL, 0);	/* LÑnge des INF- Puffers ermitteln */
	inf = Malloc((long) (len+1));
	if (inf == NULL)
	{
		err_alert(ENSMEM);
		return;
	}
	inf[len] = EOS;
	shel_get(inf, len);
	s = inf+128;			/* Beginn der Desktop- Daten */
#else
	s = infdata;
#endif

	ln_wind = 1;					/* erstes Fenster */
	ln_pgms = ln_apps = 0;
	if ((s == NULL) || (*s == EOS))	/* keine Daten */
		goto install;				/* ... dann Defaults verwenden */

	if (strncmp(s, "#_DSK", 5))
	{
		err:
#if		COUNTRY==FRG
		form_alert(1, "[3][Fehler auf INF- Datei!][Abbruch]");
#elif	COUNTRY==USA
		form_alert(1, "[3][Error at INF file!][Abort]");
#endif
		goto install;
	}

	s += 5;					/* #_DSK Åberspringen */
	scnstr(dummy_s);			/* "MAGXDESK */
	scnstr(dummy_s);			/* Vxx.xx */
	i_version = scndez();
	if (i_version != PGM_I_VERSION)
		goto err;				/* falsche INF- Version */
	if (!scnlin())			/* Zeile #_DSK Åberspringen */
		goto err;

	/* Wir beginnen mit dem Einlesen */
	/* ----------------------------- */

	old_w = desk_g.g_w;
	old_h = desk_g.g_h;
	ln_icns = 0;		/* erstes Icon */

	/* Die groûe Schleife fÅrs Einlesen */
	/* -------------------------------- */

	for (; *s; scnlin())
	{
		if (strncmp(s, "#_D", 3))
			continue;

		s += 3;

		if (!*s || *s == '\r')
			break;
		code = (*s++) << 8;			/* Hibyte holen */
		if (!*s || *s == '\r')
			break;
		code |= *s++;				/* Lobyte holen */

		switch(code)
		{
			case 'SW':
				if (!pass)
					break;
				old_w = scndez();

				old_h = scndez();

				deflt_topwnr = scndez();

				status.sorttyp = scndez() + M_SNAME;
 
				status.showtyp = (scnflg()) ? M_ATEXT : M_ABILDR;

				status.points = (char) scndez();

				if (status.points == 8)
					status.points = M_8_PKT;
				else	if (status.points == 9)
					status.points = M_9_PKT;
				else
					status.points = M_10_PKT;

				status.is_1col		= scnflg();
				status.is_groesse	= scnflg();
				status.is_datum	= scnflg();
				status.is_zeit	= scnflg();

				status.blitstate	= scnflg();
				status.cnfm_del	= scnflg();
				status.cnfm_copy	= scnflg();
				status.mode_ovwr	= scnflg();
				status.check_free	= scnflg();
				status.use_fldl	= scnflg();
				status.resident	= scnflg();
				status.show_all	= scnflg();
				status.clock		= scnflg();
				status.use_cache	= scnflg();
				status.dnam_init	= scnflg();
				status.resvd1		= scnflg();
				status.resvd2		= scnflg();
				break;

			case 'FM':
				if (!pass)
					break;
				fmtstat.tmpdrv		= scnflg() + '0';

				fmtstat.sides		= scndez();
				fmtstat.tracks		= scndez();
				fmtstat.sectors	= scndez();
				fmtstat.interlv	= scndez();
				fmtstat.trkincr	= scndez();
				fmtstat.sidincr	= scndez();
				fmtstat.clustsize	= scndez();
				break;

			case 'WN':
				if (!pass || ln_wind > ANZFENSTER)
					break;
				fenster[ln_wind].out.g_x = scndez();
				fenster[ln_wind].out.g_y = scndez();
				fenster[ln_wind].out.g_w = scndez();
				fenster[ln_wind].out.g_h = scndez();
				if (old_w != desk_g.g_w)
				{
					recalc(&(fenster[ln_wind].out.g_x), old_w, desk_g.g_w);
					recalc(&(fenster[ln_wind].out.g_w), old_w, desk_g.g_w);
				}
				if (old_h != desk_g.g_h)
				{
					recalc(&(fenster[ln_wind].out.g_y), old_h, desk_g.g_h);
					recalc(&(fenster[ln_wind].out.g_h), old_h, desk_g.g_h);
				}
				fenster[ln_wind].out.g_x += desk_g.g_x;
				fenster[ln_wind].out.g_y += desk_g.g_y;
				fenster[ln_wind].shift = scndez();
			{
				char buf[200];
				char *name;

				scnstr(buf);
				name = get_name(buf);
				strcpy(fenster[ln_wind].maske, name);
				*name = '\0';
				strcpy(fenster[ln_wind].path, buf);
			}
				ln_wind++;
				break;

			case 'IC':
				if (pass)
				{
					icon[ln_icns].icontyp = scndez();
					icon[ln_icns].isdisk = scnflg() + '0';
					if (icon[ln_icns].isdisk == '@')
						icon[ln_icns].isdisk = 0;
					(fenster[0].pobj+ln_icns+1)->ob_x = scndez();
					(fenster[0].pobj+ln_icns+1)->ob_y = scndez();
					if (old_w != desk_g.g_w)
						recalc(&((fenster[0].pobj+ln_icns+1)->ob_x), old_w, desk_g.g_w);
					if (old_h != desk_g.g_h)
						recalc(&((fenster[0].pobj+ln_icns+1)->ob_y), old_h, desk_g.g_h);
					(fenster[0].pobj+ln_icns+1)->ob_x += desk_g.g_x;
					(fenster[0].pobj+ln_icns+1)->ob_y += desk_g.g_y;
					if (icon[ln_icns].icontyp >= I_ORDNER)
						scnstr(icon[ln_icns].text);
				}

				ln_icns++;
				break;

			case 'PG':
				if (!pass || ln_pgms >= ANZPROGRAMS)
					break;
				scnbstr(((adr_prgrms+PG_1+ln_pgms)->ob_spec.tedinfo)->te_ptext);
				ln_pgms++;
				break;

			case 'AP':
				if (pass)
				{
					while (*s == ' ')
						s++;
					if (s[0] && s[1])
						memcpy(&(application[ln_apps].typ), s, 2L);
					s += 2;
					scnstr(application[ln_apps].path);
					scnstr(application[ln_apps].ext1);
					scnstr(application[ln_apps].ext2);
				}
				ln_apps++;
				break;

			case 'PP':
				if (!pass)
					break;
			{
				char	buf[200];
				char *ziel1,*ziel2;

				buf[0] = EOS;
				if (*s)
					s++;
				ziel1 = buf;
				while ((*s) && (*s != '\r') && (*s != '\n'))
					*ziel1++ = *s++;
				*ziel1 = EOS;
				ziel1 = ((adr_ttppar+TTPPAR_1)->ob_spec.tedinfo)->te_ptext;
				ziel2 = ((adr_ttppar+TTPPAR_2)->ob_spec.tedinfo)->te_ptext;
				if (buf[0])
				{
					strncpy(ziel1, buf, TTPLEN);
					if (strlen(buf) >= TTPLEN)
					{
						ziel1[TTPLEN] = EOS;
						strcpy(ziel2, buf + TTPLEN);
					}
				}
			}
				break;
		} /* END CASE */
	} /* END FOR */

	install:
#if	MAGIXDESK
	Mfree(inf);
#endif

	if (!pass)		/* wollte nur n_icons,n_apps bestimmen */
	{
		register OBJECT *o;


		n_icons = ln_icns + PLUSICONS;
		n_apps  = ln_apps + PLUSAPPS;
		gmemptr = Malloc(sizeof(ICON) * n_icons +
					  sizeof(OBJECT) * (n_icons+1) +
					  sizeof(APPLICATION) * n_apps);
		if (!gmemptr)
		{
			err_alert(ENSMEM);
			return;
		}
		fenster[0].shownum = fenster[0].realnum = n_icons;
		fenster[0].pobj = (OBJECT *) gmemptr;
		icon = (ICON *) (fenster[0].pobj + n_icons + 1);
		application = (APPLICATION *) (icon + n_icons);

		/* Desktop- Hintergrund zusammenfummeln */
		/* ------------------------------------ */

		o = fenster[0].pobj;
		o->ob_next = -1;
		o->ob_head = 1;
		o->ob_tail = n_icons;
		o->ob_type = G_BOX;
		(o->ob_spec).index = adr_icons[(aes_global[10] > 1) ? I_BG_COL : I_BG_BW].ob_spec.index;
/*		(o->ob_spec).index = (aes_global[10] > 1) ? 0x1173L : 0x1143L; */
		o->ob_flags = o->ob_state = 0;
		icw = (adr_icons+I_FLPDSK)->ob_width + 1;
		ich = (adr_icons+I_FLPDSK)->ob_height + 1;

		dx = desk_g.g_x;
		dy = desk_g.g_y;

		for	(i = 0; i < n_icons; i++)
		{
			o++;
			o->ob_next = i+2;
			o->ob_head = o->ob_tail = -1;
			o->ob_x = dx;
			o->ob_y = dy;
			dx += icw;
			if (dx + icw > desk_g.g_w)
			{
				dx = desk_g.g_x;
				dy += ich;
				if (dy + ich > desk_g.g_h)
					dy = desk_g.g_y;
			}
			o->ob_flags = o->ob_state = 0;
		}
		o->ob_next = 0;
		o->ob_flags = LASTOB;
		return;
	}

	/* unbenutzte Fenster/Icons/Programme/Applikationen */
	/* ------------------------------------------------ */

	for	(; ln_wind <= ANZFENSTER; ln_wind++)
		fenster[ln_wind].path[0] = EOS;
	for	(; ln_icns < n_icons; ln_icns++)
		icon[ln_icns].icontyp = 0;
	for	(; ln_pgms < ANZPROGRAMS; ln_pgms++)
		*((adr_prgrms+PG_1+ln_pgms)->ob_spec.tedinfo)->te_ptext = EOS;
	for	(; ln_apps < n_apps; ln_apps++)
		application[ln_apps].path[0] = EOS;

	/* allgemeine Installation der Icons */
	/* --------------------------------- */

	for	(i = 0; i < n_icons; i++)
	{
		if (icon[i].icontyp != 0)
		{
			int typ;
			char *txt,*name;


			txt = icon[i].text;
			typ = icon[i].icontyp;
			if (typ >= I_ORDNER)
			{
				name = get_name(txt);
				if (typ == I_DATEI || typ >= DATICN_OFFS)
					icon[i].icontyp = find_iconfile_dat(name);
				else
				if (typ == I_PROGRA || typ >= PGMICN_OFFS)
					icon[i].icontyp = find_iconfile_pgm(name);
			}
			else
				name = NULL;
			init_icnobj(fenster[0].pobj + i+1, &icon[i].data, icon[i].icontyp, name);
			if (icon[i].isdisk)
				icon[i].data.ib_char = icon[i].isdisk + 0x1000;
		}
		else
			(fenster[0].pobj + i+1)->ob_flags = HIDETREE;
	}

	Mgraf_mouse(ARROW);
	get_char_dim();
	Blitmode(status.blitstate & 1);
}


/****************************************************************
*
* Speichert alle Defaults.
* Die TMP- Datei wird IMMER aufs Bootlaufwerk geschrieben.
*
****************************************************************/

static void prtdez(int i)
{
	*s++ = ' ';
	itoa(i, s, 10);
	while (*s++)
		;
	s--;
}
static void prtflg(int i)
{
	*s++ = (i) ? '1' : '0';
}
static void prtstr(const char *str)
{
	while ((*s++ = *str++) != '\0')
		;
	s--;
}

static void status_to_ascii(char *inf)
{
	register int i;


	s = inf;

	/* 1. Zeile: "#_DSK MAGXDESK Vx.xx" */
	/* -------------------------------- */

#if	MAGIXDESK
	prtstr("#_DSK MAGXDESK V");
#else
	prtstr("#_DSK KAOSDESK V");
#endif
	prtstr(pgm_ver);
	prtdez(PGM_I_VERSION);
	prtstr("\r\n#_DSW");

	/* 2. Zeile: "#_DSW w h t a bb cccc ddddddddddddd"	*/
	/* w	 Bildschirmbreite							*/
	/* h  Bildschirmhîhe							*/
	/* t	 Sortiertyp 0..4							*/
	/* a  '0'/'1' = Anzeigen als Bilder/als Text 		*/
	/* b  '08'/'09'/'10' = Punktgrîûe fÅr Text			*/
	/* c  1col/isgr/isdat/iszeit						*/
	/* d  status.blitstate							*/
	/*	 status.cnfm_del							*/
	/*	 status.cnfm_copy							*/
	/*	 status.mode_ovwr							*/
	/*	 status.check_free							*/
	/*	 status.use_fldl							*/
	/*	 status.resident							*/
	/*	 status.show_all							*/
	/*	 status.clock								*/
	/*	 status.use_cache							*/
	/*	 status.dnam_init							*/
	/*	 status.resvd1								*/
	/*	 status.resvd2								*/
	/* --------------------------------------------------- */

	prtdez(desk_g.g_w);
	prtdez(desk_g.g_h);
	prtdez(deflt_topwnr);
	prtdez(status.sorttyp - M_SNAME);
	*s++ = ' ';
	prtflg(status.showtyp == M_ATEXT);
	if (status.points == M_8_PKT)
		i = 8;
	else
		i = (status.points == M_9_PKT) ? 9 : 10;
	prtdez(i);
	*s++ = ' ';
	prtflg(status.is_1col);
	prtflg(status.is_groesse);
	prtflg(status.is_datum);
	prtflg(status.is_zeit);
	*s++ = ' ';
	prtflg(status.blitstate);
	prtflg(status.cnfm_del);
	prtflg(status.cnfm_copy);
	*s++ = status.mode_ovwr + '0';
	prtflg(status.check_free);
	prtflg(status.use_fldl);
	prtflg(status.resident);
	prtflg(status.show_all);
	prtflg(status.clock);
	prtflg(status.use_cache);
	prtflg(status.dnam_init);
	prtflg(status.resvd1);
	prtflg(status.resvd2);
	prtstr("\r\n#_DFM");

	/* 3. Zeile: "#_DFM a b c d e f g h"				*/
	/* a	 TemporÑrlaufwerk							*/
	/* b	 Seiten									*/
	/* c	 Spuren									*/
	/* d	 Sektoren									*/
	/* e	 Interleave								*/
	/* f	 Spurversatz								*/
	/* g	 Seitenversatz								*/
	/* h	 Clustergrîûe								*/
	/* --------------------------------------------------- */

	*s++ = ' ';
	*s++ = (fmtstat.tmpdrv) ? fmtstat.tmpdrv : '@';
	prtdez(fmtstat.sides);
	prtdez(fmtstat.tracks);
	prtdez(fmtstat.sectors);
	prtdez(fmtstat.interlv);
	prtdez(fmtstat.trkincr);
	prtdez(fmtstat.sidincr);
	prtdez(fmtstat.clustsize);
	prtstr("\r\n");

	/* 3. Zeile und folgende: "#_DWN x y w h shift path\mask" */
	/* ------------------------------------------------------ */

	for	(i = 1; i <= ANZFENSTER; i++)
	{
		prtstr("#_DWN");
		prtdez(fenster[i].out.g_x - desk_g.g_x);
		prtdez(fenster[i].out.g_y - desk_g.g_y);
		prtdez(fenster[i].out.g_w);
		prtdez(fenster[i].out.g_h);
		if (fenster[i].path[0])
		{
			prtdez(fenster[i].shift);
			*s++ = ' ';
			prtstr(fenster[i].path );
			prtstr(fenster[i].maske);
		}
		else
			prtdez(0);
		*s++ = '\r';
		*s++ = '\n';
	}

	/* "#_DIC typ c x y path" */
	/* ---------------------- */

	for	(i = 0; i < n_icons; i++)
	{
		if (0 == icon[i].icontyp)
			continue;
		prtstr("#_DIC");
		prtdez(icon[i].icontyp);
		*s++ = ' ';
		*s++ = (icon[i].isdisk) ? (icon[i].isdisk) : '@';
		prtdez((fenster[0].pobj+i+1)->ob_x - desk_g.g_x);
		prtdez((fenster[0].pobj+i+1)->ob_y - desk_g.g_y);
		if (icon[i].icontyp >= I_ORDNER)
		{
			*s++ = ' ';
			prtstr(icon[i].text);
		}
		*s++ = '\r';
		*s++ = '\n';
	}

	/* "#_DAP path typ ext1 ext2" */
	/* -------------------------- */

	for	(i = 0; i < n_apps; i++)
	{
		if (!application[i].path[0])
			continue;
		prtstr("#_DAP ");
		*s++ = ((char *) (&(application[i].typ)))[0];
		*s++ = ((char *) (&(application[i].typ)))[1];
		*s++ = ' ';
		prtstr(application[i].path);
		*s++ = ' ';
		prtstr(application[i].ext1);
		*s++ = ' ';
		prtstr(application[i].ext2);
		*s++ = '\r';
		*s++ = '\n';
	}

	/* "#_DPG path */
	/* ----------- */

	for	(i = 0; i < ANZPROGRAMS; i++)
	{
		prtstr("#_DPG ");
		prtstr(((adr_prgrms+PG_1+i)->ob_spec.tedinfo)->te_ptext);
		*s++ = '\r';
		*s++ = '\n';
	}

	/* "#_DPP line" */
	/* ------------ */

	prtstr("#_DPP ");
	prtstr(((adr_ttppar+TTPPAR_1)->ob_spec.tedinfo)->te_ptext);
	prtstr(((adr_ttppar+TTPPAR_2)->ob_spec.tedinfo)->te_ptext);
	*s++ = '\r';
	*s++ = '\n';

	*s = '\0';
}


#if	MAGIXDESK
void save_status(int is_inf)
{
	char *inf,*t;
	int handle = 0;
	long doserr;


	Mgraf_mouse(HOURGLASS);
	inf = (char *) Malloc(65536L);
	if (inf == NULL)
	{
		doserr = ENSMEM;
		goto ende;
	}

	if (is_inf)
	{
		doserr = Fopen(inf_name, RMODE_RW);
		handle = (int) doserr;
		if (doserr == EFILNF)
		{
			doserr = Fcreate(inf_name, 0);
			handle = (int) doserr;
			if (doserr > E_OK)
			{
				char *os_ver_s2 = "#_MAG MAG!X v__.__\r\n#_CTR\r\n";

				strncpy(os_ver_s2+13, os_ver_s+7, 5);
				Fwrite((int) doserr, strlen(os_ver_s2), os_ver_s2);
				Fseek(0L, handle, SEEK_SET);
			}
		}
		if (doserr < E_OK)
			goto ende;
		doserr = Fread(handle, 32768L, inf);
		if (doserr < E_OK)
			goto ende;
		s = inf;
		while (strncmp(s, "#_CTR", 5))
		{
			if (!scnlin())
			{
				err2:
				doserr = ERROR;
				goto ende;			/* unerwartetes Dateiende */
			}
		}
		if (!scnlin())
			goto err2;
		t = s;					/* Beginn der neuen Daten */
		shel_get(s, 128);
		deflt_topwnr = top_wnr();
		status_to_ascii(t + 128);
		Fseek(t - inf, handle, SEEK_SET);	/* Dateizeiger auf Kontrollfeld */
		doserr = Fwrite(handle, 128 + strlen(t+128), t);
		if (doserr != 128 + strlen(t+128))
		{
			if (doserr >= E_OK)
				doserr = EWRITF;
			goto ende;
		}
		Fshrink(handle);
	}
	else
	{
		unsigned int len;

		doserr = E_OK;
		len = shel_get(NULL, 0);
		shel_get(inf, len);
		inf[len] = EOS;
		s = inf;
		status_to_ascii(s + 128);
		if (!shel_put(inf, 128 + ((unsigned int) strlen(inf+128) + 1)))
#if	COUNTRY == FRG
			form_alert(1, "[3][öberlauf des AES- Puffers!][Abbruch]");
#elif COUNTRY == USA
			form_alert(1, "[3][Overflow in AES- buffer!][Cancel]");
#endif
	}

	ende:

	if (handle > 0)
		Fclose(handle);
	if (inf)
		Mfree(inf);
	if (!is_inf && gmemptr)
	{
		Mfree(gmemptr);
		gmemptr = NULL;
#if	DEBUG
		icon = application = fenster[0].pobj = NULL;
#endif
	}
	if (doserr < E_OK)
		err_alert(doserr);
	Mgraf_mouse(ARROW);
}
#else
void save_status(int is_inf)
{
	char *inf;
	int handle = 0;
	long doserr;


	Mgraf_mouse(HOURGLASS);
	inf = (char *) Malloc(65536L);
	if (inf == NULL)
	{
		doserr = ENSMEM;
		goto ende;
	}

	strcpy(inf_name + 12, (is_inf) ? "INF" : "TMP");
	doserr = Fcreate(inf_name, 0);
	if (doserr < E_OK)
		goto ende;
	handle = (int) doserr;
	s = inf;
	deflt_topwnr = top_wnr();
	status_to_ascii(inf);
	doserr = Fwrite(handle, strlen(inf), inf);
	if (doserr != strlen(inf))
	{
		if (doserr >= E_OK)
			doserr = EWRITF;
		goto ende;
	}

	ende:

	if (handle > 0)
		Fclose(handle);
	if (inf)
		Mfree(inf);
	if (!is_inf && gmemptr)
	{
		Mfree(gmemptr);
		gmemptr = NULL;
#if	DEBUG
		icon = application = fenster[0].pobj = NULL;
#endif
	}
	if (doserr < E_OK)
		err_alert(doserr);
	Mgraf_mouse(ARROW);
}
#endif

/****************************************************************
*
* LÑdt alle Defaults neu von Laufwerk <drv>
*
****************************************************************/
#if	MAGIXDESK
void reload_status(int drv)
{
	int	handle;
	char *inf;
	char olddrv;
	long doserr;

#if		COUNTRY==FRG
	static char errstr[] = "[2][Keine INF- Datei auf X:|Defaults verwenden ?][OK|Abbruch]";
	#define _DRV 25
#elif	COUNTRY==USA
	static char errstr[] = "[2][INF file not found at X:|Use defaults ?][OK|Cancel]";
	#define _DRV 24
#endif


	Mgraf_mouse(HOURGLASS);
	olddrv = inf_name[0];
	if (drv >= 0)
		inf_name[0] = drv + 'A';
	inf = (char *) Malloc(65536L);
	if (inf == NULL)
	{
		doserr = ENSMEM;
		goto err;
	}

	doserr = Fopen(inf_name, RMODE_RD);
	if (doserr == EFILNF)
	{
		errstr[_DRV] = inf_name[0];
		if (1 != form_alert(2, errstr))
		{
			inf_name[0] = olddrv;
			Mgraf_mouse(ARROW);
			Mfree(inf);
			return;
		}
		goto getit;
	}
	handle = (int) doserr;
	if (doserr < E_OK)
	{
		err:

		if (handle > 0)
			Fclose(handle);
		Mgraf_mouse(ARROW);
		err_alert(doserr);
		if (inf)
			Mfree(inf);
		return;
	}
	doserr = Fread(handle, 65535L, inf);
	Fclose(handle);
	if (doserr < E_OK)
		goto err;
	inf[doserr] = EOS;
	s = inf;
	while (strncmp(s, "#_CTR", 5))
	{
		if (!scnlin())
		{
			err2:
			doserr = ERROR;
			goto err;			/* unerwartetes Dateiende */
		}
	}

	if (!scnlin())
		goto err2;

	shel_put(s, (unsigned int) strlen(s) + 1);
	getit:
	Mfree(inf);

	close_all_wind();
	load_status(0);
	load_status(1);
	open_all_wind();
	Mgraf_mouse(ARROW);
}

#else
void reload_status(int is_inf, int drv)
{
	int	handle;
	char	*inf;
	char olddrv;
	long doserr;

#if		COUNTRY==FRG
	static char errstr[] = "[2][Kein KAOSDESK.INF auf X:|Defaults verwenden ?][OK|Abbruch]";
	#define _DRV 26
#elif	COUNTRY==USA
	static char errstr[] = "[2][KAOSDESK.INF not found at X:|Use defaults ?][OK|Cancel]";
	#define _DRV 30
#endif

	strcpy(inf_name + 12, (is_inf) ? "INF" : "TMP");
	Mgraf_mouse(HOURGLASS);
	olddrv = inf_name[0];
	if (drv >= 0)
		inf_name[0] = drv + 'A';
	doserr = (long) Malloc(-1L);
	if (doserr < 65536L)
		goto memerr;
	inf = Malloc(doserr);				/* grîûten Block holen */
	if (!inf)
		goto memerr;
	Mshrink(0, inf, 32768L);				/* obere HÑlfte freigeben */
	infdata = (char *) Malloc(32768L);		/* Block holen */
	Mfree(inf);						/* untere HÑlfte freigeben */
	if (infdata == NULL)
	{
		memerr:
		doserr = ENSMEM;
		goto err;
	}

	doserr = Fopen(inf_name, RMODE_RD);
	if (doserr == EFILNF)
	{
		Mfree(infdata);
		infdata = NULL;
		errstr[_DRV] = inf_name[0];
		if ((gmemptr) && (1 != form_alert(2, errstr)))
		{
			inf_name[0] = olddrv;
			Mgraf_mouse(ARROW);
			return;
		}
		goto getit;
	}
	handle = (int) doserr;
	if (doserr < E_OK)
	{
		err:

		if (handle > 0)
			Fclose(handle);
		Mgraf_mouse(ARROW);
		err_alert(doserr);
		if (infdata)
			Mfree(infdata);
		return;
	}
	doserr = Fread(handle, 32767L, infdata);
	Fclose(handle);
	if (doserr < E_OK)
		goto err;
	infdata[doserr] = EOS;
	s = infdata;
	getit:

	close_all_wind();
	load_status(0);
	load_status(1);
	if (infdata)
		Mfree(infdata);
	infdata = NULL;
	open_all_wind();
	Mgraf_mouse(ARROW);
}
#endif
