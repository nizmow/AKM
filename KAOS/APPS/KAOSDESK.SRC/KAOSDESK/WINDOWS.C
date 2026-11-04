/*********************************************************************
*
* Dieses Modul enthÑlt die Bearbeitung aller Fensteroperationen
*
*********************************************************************/

#include <string.h>
#include <stdlib.h>
/* #include <stdio.h> */
#include <vdi.h>
#include <mgx_dos.h>	/* include before tos.h */
#include <tos.h>
#include <tosdefs.h>
#include "k.h"

#define  WXOFFS	((settings.show_mode == M_ATEXT) ? (2*char_w - 1) : (3))
#define  WYOFFS	(1)
#define  WXDIST	((settings.show_mode == M_ATEXT) ? (2*char_w) : (3))
#define  WYDIST	(1)

static int showw, showh;		/* Objektgrîûen im Fenster */


/****************************************************************
*
* Gibt den Speicher fÅr Fenster <wnr> frei
*
****************************************************************/

void free_pool(int wnr)
{
	long size;
	long offs;
	long bottom;		/* untere Grenze des zu verschiebenden Speichers */
	long top;			/* obere Grenze des zu verschiebenden Speichers */
	register int i,j,num;
	register OBJECT *o;
	register ICONBLK *icn;


/*	Cconws("\x1b" "e" "\x1b" "HEintritt in free\r\n");
	for	(i = 1; i <= NUM_WINDOWS; i++)
		printf("Offset: %4ld Grîûe: %4ld\n", windows[i].alloc_offs, windows[i].alloc_size);
	printf("Frei: %3d\n", poolsize);
	Cnecin();
*/
	if (0L == (size = windows[wnr].alloc_size))
		return;		/* war kein Speicher belegt */
	offs = windows[wnr].alloc_offs;
	windows[wnr].alloc_size = 0L;

	bottom = offs + size;
	top = 0L;
	for	(i = 1; i <= NUM_WINDOWS; i++)
	{

		/* Wenn das Fenster nicht unsereres ist, keinen allozierten		*/
		/* Speicher hat oder dieser unter unserem liegt, mÅssen keine	*/
		/* Bereiche verschoben werden								*/
		/* ------------------------------------------------------------- */

		if (i == wnr ||
			 !windows[i].alloc_size ||
			 windows[i].alloc_offs < offs)
			continue;
		if (windows[i].alloc_offs+windows[i].alloc_size > top)
			top = windows[i].alloc_offs+windows[i].alloc_size;

		num = windows[i].realnum;
		for	(j = num,o = windows[i].pobj + 1; j > 0; j--,o++)
		{
			if ((o->ob_type == G_STRING) || (o->ob_type == G_ICON))
				o->ob_spec.free_string -= size;
		}
		for	(j = num, icn = windows[i].picb; j > 0; j--,icn++)
			icn->ib_ptext -= size;

		(char *) windows[i].pmydta	-= size;
		(char *) windows[i].pobj 	-= size;
		(char *) windows[i].picb 	-= size;
		(char *) windows[i].pstr 	-= size;
		windows[i].alloc_offs -= size;
	}

	if (top > bottom)
		memmove(poolptr+offs, poolptr+bottom, top-bottom);

	poolfree -= size;
	poolsize += windows[wnr].realnum;

/*	Cconws("Austritt aus free\r\n");
	for	(i = 1; i <= NUM_WINDOWS; i++)
		printf("Offset: %4ld Grîûe: %4ld\n", windows[i].alloc_offs, windows[i].alloc_size);
	printf("Frei: %3d\n", poolsize);
	Cnecin(); */
}


/****************************************************************
*
* Initialisiert die Zeiger fÅr ein neues Fenster.
* Die MYDTAs sind schon eingelesen, pmydta und realnum gesetzt.
*
****************************************************************/

void alloc_pool(int wnr)
{
	long size;
	int  num;
/*	register int i;


	Cconws("\x1b" "HEintritt in alloc \r\n");
	for	(i = 1; i <= NUM_WINDOWS; i++)
		printf("Offset: %4ld Grîûe: %4ld\n", windows[i].alloc_offs, windows[i].alloc_size);
	printf("Frei: %3d\n", poolsize);
	Cnecin();
*/
	num = windows[wnr].realnum;
	size = sizeof(OBJECT) + num *
			(sizeof(OBJECT)+sizeof(MYDTA)+sizeof(ICONBLK)+sizeof(MYFNAME));
	windows[wnr].alloc_offs = poolfree - poolptr;
	windows[wnr].alloc_size = size;
	windows[wnr].pobj = (OBJECT *)  (windows[wnr].pmydta + num);
	windows[wnr].picb = (ICONBLK *) (windows[wnr].pobj + num + 1);
	windows[wnr].pstr = (MYFNAME *) (windows[wnr].picb + num);
	poolfree += size;
	poolsize -= num;

/*	Cconws("Austritt aus alloc\r\n");
	for	(i = 1; i <= NUM_WINDOWS; i++)
		printf("Offset: %4ld Grîûe: %4ld\n", windows[i].alloc_offs, windows[i].alloc_size);
	printf("Frei: %3d\n", poolsize);
	Cnecin();*/
}


/****************************************************************
*
* Initialisiert ein Objekt, das ein Icon darstellt
*
****************************************************************/

void init_icnobj(OBJECT *o, ICONBLK *icn, int type, char *text)
{
	register OBJECT *srcobj;
	register int typ2;


	if (type >= PGMICN_OFFS)			/* externes Icon */
		typ2 = (type < DATICN_OFFS) ? I_PROGRA : I_DATEI;
	else
		typ2 = type;

	srcobj = tree_icons + typ2;
	*icn = * ((srcobj->ob_spec.iconblk));
	if (type >= PGMICN_OFFS && type < DATICN_OFFS) /* externes Programmicon */
	{
		type -= PGMICN_OFFS;	/* Offset lîschen */
		icn->ib_pdata = icf_pgmicn + type * 0x38;
	}
	else
	if (type >= DATICN_OFFS)		/* externes Dateiicon */
	{
		type -= DATICN_OFFS;	/* Offset lîschen */
		icn->ib_pdata = icf_daticn + type * 0x38;
	}

	if (text)
		icn->ib_ptext = text;

	o->ob_type   = G_ICON;
	o->ob_flags  = NORMAL;
	o->ob_state  = NORMAL;
	o->ob_width  = srcobj->ob_width;
	o->ob_height = srcobj->ob_height;
	o->ob_spec.iconblk = icn;
}


/****************************************************************
*
* Sucht in der Icondatei.
*
* RÅckgabe:	Nummer des Icons (inklusive Offset), bzw. I_PROGRA,
*			wenn Icondatei nicht geladen oder Icon nicht
*			gefunden.
*
****************************************************************/

int find_iconfile_pgm(char *fname)
{
	register int i;
	register char *s,*f;

	if (icf_data)
	{
		f = strchr(fname, '.');
		*f = EOS;
		for	(s = icf_pgms, i = 0;
			 i < iconfile_header.n_pgm_names;
			 s += 9,i++
			)
		{
			if (!strncmp(fname, s, 8))
			{
				*f = '.';
				return (int) (PGMICN_OFFS + ((unsigned char *) (s))[8]);
			}
		}
		*f = '.';
	}

	return I_PROGRA;
}


int find_iconfile_dat(char *fname)
{
	register int i;
	register char *s;

	if ((icf_data) && (NULL != (fname = strchr(fname, '.'))))
	{
		fname++;
		for	(s = icf_fils, i = 0;
			 i < iconfile_header.n_dat_names;
			 s += 4,i++)
		{
			if (!strncmp(fname, s, 3))
				return (int) (DATICN_OFFS + ((unsigned char *) (s))[3]);
		}
	}

	return I_DATEI;
}


/****************************************************************
*
* Merkt den Selectstatus in Bit 2 von MYDTA.flags.
*
****************************************************************/

static void save_select_status(int wnr)
{
	register int i,tail;
	register OBJECT *tree = windows[wnr].pobj;


	if ((i = (tree->ob_head)) <= 0)
		return;
	tail = (tree++->ob_tail);
	for	(; i <= tail; i++,tree++)
	{
		if (!(SELECTED&(tree->ob_state)) || (HIDETREE&(tree->ob_flags)))
			windows[wnr].pmydta[i - 1].flags &= ~2;
		else	windows[wnr].pmydta[i - 1].flags |=  2;
	}
}


/****************************************************************
*
* Gibt Fensternummer des obersten Fensters zurÅck.
*
****************************************************************/

int top_wnr(void)
{
	int whdl;

	if (!wind_get_int(0, WF_TOP, &whdl))
		return -1;
	if (whdl < 0)
		return -1;

	return hdl_to_fnr(whdl);
}


/****************************************************************
*
* Zeichnet das Fenster <wnr> neu.
*
****************************************************************/

void upd_wind(int wnr)
{
	int msg[8];

	/* Der Redraw wird nicht direkt ausgefÅhrt, sondern schickt */
	/* eine Nachricht an die Applikation, um Kollisionen mit    */
	/* von AES erzeugten Redraws zu vermeiden                   */
	/* -------------------------------------------------------- */

	msg[0] = WM_REDRAW;
	msg[1] = ap_id;
	msg[2] = 0;
	msg[3] = windows[wnr].handle;
	msg[4] = windows[wnr].out.g_x;
	msg[5] = windows[wnr].out.g_y;
	msg[6] = windows[wnr].out.g_w;
	msg[7] = windows[wnr].out.g_h;
	appl_write(ap_id, 16, msg);
}


/****************************************************************
*
* Sortiert die Dateien fÅr Fenster <wnr>.
* Auûerdem werden die "nicht zur Maske passenden" Dateien nach
* hinten sortiert und <windows[wnr].shownum> initialisiert.
*
****************************************************************/

static int cmp_mydtas(MYDTA *f1, MYDTA *f2)
{
	register int  r;
	register long l;
	register char *n1,*n2;


	if (((f1->flags)&1) != ((f2->flags)&1))		/* Match- Flag */
		return ((f2->flags)&1 - ((f1->flags)&1));

	if (settings.sort_mode == M_SNICHT)
		return f1->number - f2->number;

	if (f1->attrib & F_SUBDIR)
	{
		if (!(f2->attrib & F_SUBDIR))
			return -1;
		else
			goto name;
	}

	if (f2->attrib & F_SUBDIR)
		return 1;

	switch (settings.sort_mode)
	{
		case M_SNAME:
			name:
			return strcmp(f1->filename, f2->filename);

		case M_SDATUM:
			l = ((unsigned long) f2->date) - ((unsigned long) f1->date);
			if (l == 0)
				l = ((unsigned long) f2->time) - ((unsigned long) f1->time);
			goto groe;

		case M_SGROES:
			l = f2->filesize - f1->filesize;
			groe:
			if (l == 0)
				goto name;
			else
				return (l > 0) ? 1 : -1;

		case M_STYP:
			n1 = strchr(f1->filename, '.');
			n2 = strchr(f2->filename, '.');
			if (n1 == NULL && n2 == NULL)
				goto name;
			if (n1 == NULL)
				return -1;
			if (n2 == NULL)
				return 1;
			r = strcmp(n1,n2);
			if (r == 0)
				goto name;
			return r;
	}

	return 0;
}


static void sort_mydtas(int wnr)
{
	MYDTA *files = windows[wnr].pmydta;
	int count  = windows[wnr].realnum;
	register int i,match;
	register MYDTA *dtas;


	windows[wnr].shownum = 0;
	for	(dtas = files, i = 0; i < count; i++, dtas++)
	{
		match = ((dtas->attrib & F_SUBDIR) ||
			 (fname_match(dtas->filename, windows[wnr].fpattern)));
		if (match)
		{
			dtas->flags |=  1;
			windows[wnr].shownum++;
		}
		else
			dtas->flags &= ~1;
	}

	shelsort(windows[wnr].pmydta, (size_t) windows[wnr].realnum,
		    sizeof(MYDTA), cmp_mydtas);
}


/****************************************************************
*
* Setzt die Titelzeile des Fensters mit Nummer <wnr>.
*
****************************************************************/

static void set_title(int wnr)
{
	register char *s,*p,*m;


	s = windows[wnr].title;
	p = windows[wnr].path;
	m = windows[wnr].fpattern;
	*s++ = ' ';
	if (strlen(p) + strlen(m) > 124)
	{
#if LANGUAGE == LANG_GER
		strcpy(s, "Pfad zu tief");
#elif LANGUAGE == LANG_ENG
		strcpy(s, "Path to deep");
#endif
	}
	else
	{
		strcpy(s, p);
		strcat(s, m);
		if (p[0] == 'R')
#if LANGUAGE == LANG_GER
			strcat(s, "  (Rommodul)");
#elif LANGUAGE == LANG_ENG
			strcat(s, "  (ROM modul)");
#endif
	}

	s += strlen(s);
	*s++ = ' ';
	*s 	= EOS;
}


/****************************************************************
*
* Setzt die Infozeile des Fensters mit Nummer <wnr>.
*
****************************************************************/

static void set_info(int wnr)
{
	register char *s 			= windows[wnr].info;
	register unsigned long size 	= 0L;
		    unsigned long ssize	= 0L;
	register int num 			= windows[wnr].shownum;
		    int snum			= 0;
	register int i;
	MYDTA *file 				= windows[wnr].pmydta;
	MYDTA *sfile 				= NULL;
	OBJECT *o					= windows[wnr].pobj + 1;


	for	(i = num; i > 0; i--,file++,o++)
	{
		size += file->filesize;
		if ((o->ob_state) & SELECTED)
		{
			snum++;
			ssize += file->filesize;
			sfile = file;
		}
	}

	*s++ = ' ';
	if (snum)			/* selektierte Objekte */
	{
		size = ssize;
		num  = snum;
	}

	if (snum == 0 || snum > 1)
	{
		char *objs;


		if (snum == 0 && windows[wnr].too_much)
		{
#if LANGUAGE == LANG_GER
			strcpy(s, " mehr als ");
			objs = " Objekte";
#elif LANGUAGE == LANG_ENG
			strcpy(s, " more than ");
			objs = " objects";
#endif
		}
		else
		{
			ultoa(size, s, 10);
#if LANGUAGE == LANG_GER
			strcat(s, " Bytes in ");
			objs = (num == 1) ? " Objekt" : " Objekten";
#elif LANGUAGE == LANG_ENG
			strcat(s, " bytes in ");
			objs = (num == 1) ? " object" : " objects";
#endif
		}

		s += strlen(s);
		itoa(num, s, 10);
		if (snum)
		{
#if LANGUAGE == LANG_GER
			strcat(s, " selektierten");
#elif LANGUAGE == LANG_ENG
			strcat(s, " selected");
#endif
		}

		strcat(s, objs);
	}
	else
	{
		if (0 == ((sfile->attrib) & F_SUBDIR))
		{
			if (size < 100000L)
				*s++ = ' ';
			if (size < 10000)
				*s++ = ' ';
			if (size < 1000)
				*s++ = ' ';
			if (size < 100)
				*s++ = ' ';
			if (size < 10)
				*s++ = ' ';
			ultoa(size, s, 10);
#if LANGUAGE == LANG_GER
			strcat(s, " Bytes | ");
#elif LANGUAGE == LANG_ENG
			strcat(s, " bytes | ");
#endif
			s += strlen(s);
		}

		date_to_str(s, sfile->date);
		strcat(s, " | ");
		s += strlen(s);
		time_to_str(s, sfile->time);
		i = suffix_type(sfile->filename);
		if (i == '_T')
			i = (NULL != file_to_defapp(sfile->filename));
		else
			i = TRUE;
		if (i || ((sfile->attrib) & (F_RDONLY+F_HIDDEN+F_SYSTEM+F_ARCHIVE)))
			strcat(s, " | ");
		if ((sfile->attrib) & F_RDONLY)
			strcat(s, "R");
		if ((sfile->attrib) & F_SYSTEM)
			strcat(s, "S");
		if ((sfile->attrib) & F_HIDDEN)
			strcat(s, "H");
		if ((sfile->attrib) & F_ARCHIVE)
			strcat(s, "A");
		if (i)
			strcat(s, "E");		/* executable */
	}
	s += strlen(s) + 1;
	*s = EOS;
}


/****************************************************************
*
* Zeigt freien Speicher fÅr Disk in Fenster (wnr) an.
*
****************************************************************/

void show_free(int wnr)
{
	char alertstr[100];
	register char *s;
	int lwc;


	lwc = windows[wnr].path[0];
	s = alertstr;
#if LANGUAGE == LANG_GER
	strcpy(s, "[0][Freier Platz auf Laufwerk ");
#elif LANGUAGE == LANG_ENG
	strcpy(s, "[0][Free space at drive ");
#endif
	s += strlen(s);
	*s++ = lwc;
	strcpy(s, ":| |      ");
	s   += strlen(s);
	lwc -= 'A';
	print_ul(dinfo[lwc].b_free, dinfo[lwc].b_secsiz * dinfo[lwc].b_clsiz, s);
#if LANGUAGE == LANG_GER
	strcat(s, "Bytes|][  OK  ]");
#elif LANGUAGE == LANG_ENG
	strcat(s, "bytes|][  OK  ]");
#endif
	form_alert(1, alertstr);
}


/****************************************************************
*
* Zeigt den Disknamen <dname> auf allen Icons auf dem Desktop an,
* die die Disk <lw> darstellen.
*
****************************************************************/

void set_dname(int lw, char *dname)
{
	register int i;
	register char **p;
	char old_text[13];

	lw += 'A';
	for	(i = 0; i < n_icons; i++)
		if (icon[i].isdisk == lw)
		{
			p = &(icon[i].data.ib_ptext);
			strcpy(old_text, *p);

			/* 1. Fall: Ein Name wurde Åbergeben: einsetzen */
			if (*dname)
			{
				strcpy(icon[i].text, dname);
				*p = icon[i].text;
			}
			/* 2. Fall: Leerstring Åbergeben: Default einsetzen */
			else
			{
				*p = ((tree_icons + icon[i].icon_type)->ob_spec.iconblk)->ib_ptext;
			}

			if (strcmp(old_text, *p))
				draw_wind_obj(0, i + 1);
		}
}


/****************************************************************
*
* Liest das Verzeichnis fÅr Fenster <wnr> ein und gibt ggf. einen
*  DOS- Fehlercode zurÅck.
* Das Vezeichnis wird in die MYDTAs des Fensters eingelesen, dabei
*  werden in <icontyp> bereits die Icontypen festgelegt.
* Es werden alle Dateien eingelesen, ohne Maske zu berÅcksichtigen.
* Auûerdem wird die Baumstruktur des Fensterobjekts aufgebaut.
* Tritt irgendwo ein Fehler EPTHNF auf, wird das Verzeichnis auf
*  "Root" gesetzt und der Vorgang wiederholt. Trat ein anderer Fehler
*  auf oder war die Root schon eingeschaltet, wird der Fehlercode
*  zurÅckgegeben.
* Wenn <free_flag> != 0 ist, wird der freie Speicher berechnet.
*
****************************************************************/

static long read_wind(int wnr, int free_flag)
{
	char path[128];
	register int count,drv;
	long errcode;
	DTA  dta;
	MYDTA *dests;


	Mgraf_mouse(HOURGLASS);
	Fsetdta(&dta);
	drv = windows[wnr].path[0] - 'A';
	dirty_drives[drv] = 0;		/* dirty- Flag lîschen */
	errcode = E_OK;

	/* Bei Fehler auf Root umschalten */
	/* ------------------------------ */

	do
	{
		if (errcode != E_OK)
		{
			windows[wnr].shift = 0;
			if (errcode != EPTHNF || windows[wnr].path[3] == EOS)
			{
				Mgraf_mouse(ARROW);
				return errcode;
			}
			else
			{
				windows[wnr].path[3] = EOS;
				free_flag = TRUE;
				set_title(wnr);
				wind_set_string(windows[wnr].handle, WF_NAME, windows[wnr].title);
			}
		}
	
		/* Pfad ÅberprÅfen, ggf. auf Root setzen */
		/* ------------------------------------- */

		Dsetdrv(drv);					/* wegen TOS- Fehler */
		errcode = Dsetpath(windows[wnr].path);
		if (errcode != E_OK)
			continue;
	
		/* Diskname einlesen (von der Root) */
		/* -------------------------------- */

		errcode = read_dname(drv, dta.d_fname);
		set_dname(drv, dta.d_fname);
		if (errcode != EFILNF && errcode != E_OK)
			continue;
	
		/* Freien Speicher ermitteln und daraus die Grîûe des horizontalen Scroll-Balkens */
		/* ------------------------------------------------------------------------------ */
	
		if (free_flag)
		{
			long slsize;

			if (dinfo[drv].b_clsiz == 0L)
			{
				/* noch nicht ermittelt oder ungÅltig => ermitteln */

				if (0L > (errcode = MDfree(&dinfo[drv], drv + 1)))
					continue;
			}

			if (dinfo[drv].b_free == dinfo[drv].b_total)
			{
				/* voll belegt */
				slsize = -1L;
			}
			else
			{
				unsigned long b_total = dinfo[drv].b_total;
				unsigned long b_free = dinfo[drv].b_free;
				
				/* 32-bit-öberlauf vermeiden */
				if (b_total > 1024 * 1024)
				{
					b_total >>= 10;
					b_free >>= 10;
				}
				slsize = ((b_total - b_free) * 1000L) / b_total;
			}

			wind_set_int(windows[wnr].handle, WF_HSLSIZE, (int) slsize);
		}

		/* Speicherverwaltung, alten Speicher freigeben und */
		/* gucken, wieviele Objekte noch reinpassen		  */

		free_pool(wnr);
		windows[wnr].pmydta = (MYDTA *) poolfree;	/* hier ist Platz */

		/* Verzeichnis einlesen */
		/* -------------------- */
	
		strcpy(path, windows[wnr].path);
		strcat(path, "*.*");
		dests = windows[wnr].pmydta;
		count = 0;
		errcode = Fsfirst(path, (settings.show_all) ? 0x37 : 0x10);
		while (errcode == E_OK && count < poolsize)
		{
			if (dta.d_fname[0] != '.' ||
			      (dta.d_fname[1] != EOS && dta.d_fname[1] != '.')
			     )
			{
				dests->number = count;
				dests->time = dta.d_time;
				dests->date = dta.d_date;
				dests->filesize = dta.d_length;
				dests->attrib = dta.d_attrib;
				strcpy(dests->filename,dta.d_fname);
				count++;
				dests++;
			}
			errcode = Fsnext();
		}

		if (errcode == E_OK)
			windows[wnr].too_much = TRUE;
		else
		{
			windows[wnr].too_much = FALSE;
			if (errcode == EFILNF || errcode == ENMFIL)
				errcode = E_OK;
		}
	}
	while (errcode != E_OK);

	windows[wnr].realnum = count;
	alloc_pool(wnr);

	/* Icontypen und Exec- Flag festlegen */
	/* ---------------------------------- */

	dests = windows[wnr].pmydta;
	for	(dests = windows[wnr].pmydta;
		 dests < windows[wnr].pmydta + count;
		 dests++)
	{
		int type;

		dests->flags = 0;					/* Datei */

		if ((dests->attrib) & F_SUBDIR)
		{
			dests->icon_type = I_ORDNER;
			continue;
		}

		type = suffix_type(dests->filename);

		/* Batchdateien bekommen das Icon I_BTCHDA */
		/* --------------------------------------- */

		if ((type & 0xff) == 'B')
			type = I_BTCHDA;

		/* Programmdateien bekommen, entweder das ihnen	*/
		/* zugeordnete Icon (Offset PGMICN_OFFS) oder	*/
		/* sonst I_PROGRA							*/
		/* ---------------------------------------------- */

		else
		if ((type & 0xff) == 'P')
		{
			dests->flags = 4;				/* Programm */
			type = find_iconfile_pgm(dests->filename);
		}

		/* Sonstige Dateien bekommen das ihnen zugewiesene	*/
		/* Icon (Offset DATICN_OFFS) oder sonst I_DATEI		*/
		/* --------------------------------------------------- */

		else
		{
			type = find_iconfile_dat(dests->filename);
		}

		dests->icon_type = type;
	}

	Mgraf_mouse(ARROW);
	return E_OK;
}


/****************************************************************
*
* Die Innenmaûe des Fensters werden berechnet.
*
****************************************************************/

static void calc_in(int wnr)
{
	register WINDOW *f;
	register int gescrollte_pixel;


	f = &(windows[wnr]);
	gescrollte_pixel = f->shift*(showh+WYDIST);

	wind_calc(WC_WORK, WINDOW_ATTRIBS, &f->out, &f->in);
	(f->pobj)->ob_x = f->in.g_x;
	(f->pobj)->ob_y = f->in.g_y - gescrollte_pixel;
	(f->pobj)->ob_width  = f->in.g_w;
	(f->pobj)->ob_height = f->in.g_h + gescrollte_pixel;
}


/****************************************************************
*
* Die Anzahl der Spalten des Fensters werden berechnet.
* Die Anzahl der sichtbaren Zeilen des Fensters werden berechnet.
*
****************************************************************/

static void calc_collin(int wnr)
{
	int x = windows[wnr].in.g_w - WXOFFS + WXDIST;
	int y = windows[wnr].in.g_h - WYOFFS + WYDIST;
	int c;


	if (settings.is_1col && settings.show_mode == M_ATEXT)
		c = 1;
	else
	{
		c = x / (showw + WXDIST);
		if (c < 1)
			c = 1;
	}

	windows[wnr].cols = c;
	windows[wnr].wlins = y / (showh + WYDIST);
	windows[wnr].lins = (windows[wnr].shownum + windows[wnr].cols - 1) /
					 windows[wnr].cols;
}


/****************************************************************
*
* Die die Dateien darstellenden Objekte werden festgelegt.
* Aus den DTAs werden alle <shownum> sichtbaren Dateien als
*  Objekte zusammengestellt.
* Die Icontypen (falls nicht als Text dargestellt wird), sind
*  bereits in den MYDTAs festgelegt.
* Ist dsel_flg FALSE, werden die "ob_flags" aus den MYDTAs geholt
*  (Bit 2), um den Selectstatus zu erhalten, der vorher mittels
*  "save_select_status" gesichert wurde.
* Eingabe: (global) showtyp,is_groesse,
*				is_datum,is_zeit
* Ausgabe: (global) showw,showh
*
****************************************************************/

static void calc_obj_look(int wnr, int dsel_flag)
{
	int count  = windows[wnr].shownum;
	OBJECT *o  = windows[wnr].pobj;
	MYFNAME *f = windows[wnr].pstr;
	MYDTA *d   = windows[wnr].pmydta;
	ICONBLK *c = windows[wnr].picb;
	int type;
	register int i;
	register char *s,*t,*fname;


	if (settings.show_mode == M_ATEXT)
	{
		showh = char_h;
		showw = char_w * (16 + 8*settings.is_size +
							 10*settings.is_date +
							 7*settings.is_time
						);
	}
	else
	{
		showh = tree_icons[I_DATEI].ob_height;
		showw = tree_icons[I_DATEI].ob_width;
	}

	/* Objekt der weiûen Hintergrundbox */

	o->ob_next = -1;
	o->ob_type = G_BOX;
	o->ob_state = NORMAL;
	o->ob_spec.index = (long) (WHITE);

	if (count > 0)
	{
		o->ob_flags = NONE;
		o->ob_head = 1;
		o->ob_tail = count;
	}
	else
	{
		o->ob_flags = LASTOB;
		o->ob_head = o->ob_tail = -1;
	}

	/* Objekte der Dateien */

	for	(i = 1; i <= count; i++,c++,d++,f++)
	{
		o++;
		o->ob_next = (i < count) ? (i+1) : (0);
		o->ob_head = o->ob_tail = -1;
		o->ob_flags = (i < count) ? NONE : LASTOB;

		type = d->icon_type;
		if (settings.show_mode == M_ABILDR)
		{
			strcpy((char *) f, d->filename);
			init_icnobj(o, c, type, (char *) f);
			o->ob_state = ((!dsel_flag) && (d->flags & 2))
					 	? SELECTED+WHITEBAK : NORMAL+WHITEBAK;
		}
		else
		{
			s = t = (char *) f;
			fname = d->filename;
			*s++ = ' ';
			if (type == I_ORDNER)
				*s++ = '\7';
			else
			if (type == I_PROGRA || (type >= PGMICN_OFFS && type < DATICN_OFFS))
				*s++ = '.';
			else
				*s++ = ' ';
			*s++ = ' ';

			if (*fname == '.' && fname[1] == '.')
			{
				*s++ = *fname++;
				*s++ = *fname++;
			}
			else
			{
				if (*fname == '.' && fname[1] == EOS)
					*s++ = *fname++;
					else
					while (*fname != '.' && *fname != EOS)
						*s++ = *fname++;
			}

			if (*fname == '.')
				fname++;

			t += 12;
			while (s < t)
				*s++ = ' ';

			while (*fname)
				*s++ = *fname++;

			t += 3;

			while (s < t)
				*s++ = ' ';

			if (settings.is_size)
			{
				char gr[10],*g;
				int  j;
	
				if ((d->attrib) & F_SUBDIR)
					gr[0] = EOS;
				else
					ultoa(d->filesize, gr, 10);

				for	(j = 8 - (int) strlen(gr); j > 0; j--)
					*s++ = ' ';

				g = gr;
				while (*g)
					*s++ = *g++;
			}
			if (settings.is_date)
			{
				*s++ = ' ';
				*s++ = ' ';
				date_to_str(s, d->date);
				s += 8;
			}
			if (settings.is_time)
			{
				*s++ = ' ';
				*s++ = ' ';
				time_to_str(s, d->time);
				s += 5;
			}
			*s = EOS;
	
			o->ob_type = G_USERDEF;
			o->ob_spec.userblk = &userblk;
			o->ob_state = ((!dsel_flag) && (d->flags & 2))
					 	? SELECTED : NORMAL;
		}

		o->ob_width = showw;
		o->ob_height = showh;
	}
	set_info(wnr);
}


/****************************************************************
*
* Organisiert das Fenster neu, d.h. legt Position der Objekte fest.
* gibt TRUE zurÅck, falls sich windows.cols oder windows.shift
* geÑndert haben.
*
****************************************************************/

static int re_arrange(int wnr)
{
	register int i,j;
	int x,y,cols;
	int oldcols = windows[wnr].cols;
	int oldshift = windows[wnr].shift;
	int count = windows[wnr].shownum;
	OBJECT *o = windows[wnr].pobj;
	long vsize,vpos;


	calc_in(wnr);			/* Innenmaûe berechnen */
	calc_collin(wnr);		/* Zeilen/Spalten berechnen */
	cols = windows[wnr].cols;

	windows[wnr].maxshift = windows[wnr].lins - windows[wnr].wlins;
	if (windows[wnr].maxshift < 0)
		windows[wnr].maxshift = 0;

	if (windows[wnr].shift < 0)
		windows[wnr].shift = 0;
	if (windows[wnr].shift > windows[wnr].maxshift)
		windows[wnr].shift = windows[wnr].maxshift;

	if (windows[wnr].lins > 0)
		vsize = (1000L * windows[wnr].wlins) / windows[wnr].lins;
	else
		vsize = 1000L;

	if (vsize > 1000L)
		vsize = 1000L;

	wind_set_int(windows[wnr].handle, WF_VSLSIZE, (int) vsize);

	if (windows[wnr].maxshift > 0)
		vpos = (1000L * windows[wnr].shift) / windows[wnr].maxshift;
	else
		vpos = 1L;

	if (vpos > 1000L)
		vpos = 1000L;

	if (vpos < 1)
		vpos = 1;

	wind_set_int(windows[wnr].handle, WF_VSLIDE, (int) vpos);

	/* Position und Hîhe ggf. korrigieren (von calc_in schon gesetzt) */
	o->ob_y = windows[wnr].in.g_y - windows[wnr].shift*(showh+WYDIST);
	o->ob_height = windows[wnr].in.g_h + windows[wnr].shift*(showh+WYDIST);
	for	(i = 0, y = WYOFFS, x = WXOFFS, j = count; j > 0; i++,j--)
	{
		o++;
		if (i >= cols)
		{
			i = 0;
			x = WXOFFS;
			y += showh + WYDIST;
		}
		o->ob_x = x;
		o->ob_y = y;
		x += showw + WXDIST;
	}
	if (count < oldcols)
		oldcols = count;
	if (count < cols)
		cols = count;
	return (oldcols != cols || oldshift != windows[wnr].shift);
}


/****************************************************************
*
* Ein Fenster <wnr> wird geîffnet. Pfad und Maske sind festgelegt.
* Wenn  <fast> auf TRUE ist, ist auch der Shiftstatus festgelegt,
*  dann wird auch keine Growbox gezeichnet.
*
****************************************************************/

long opn_wnd(int wnr, int fast)
{
	register int i;
	register long ret;


	/* Das Fenster wird hier erst erstellt */
	/* ----------------------------------- */

	if (poolptr)
	{
		windows[wnr].handle = wind_create(WINDOW_ATTRIBS, &desk_g);
	}
	else
	{
		windows[wnr].handle = -1;
	}

	if (windows[wnr].handle < 0)
	{
		ret = ENOWND;
		goto err;
	}

	Dsetdrv(windows[wnr].path[0] - 'A');
	for	(i = 0; i < n_icons; i++)
	{
		if (icon[i].isdisk == windows[wnr].path[0])
			break;
	}
	/* Verzeichnis einlesen */
	if (!fast)
		windows[wnr].shift = 0;
	dinfo[windows[wnr].path[0] - 'A'].b_clsiz = 0L;	  /* ungÅltig machen */
	ret =  read_wind(wnr, TRUE);
	if (ret)
	{		/* Fehler */
		wind_delete(windows[wnr].handle);
		windows[wnr].handle = -1;
		err:
		windows[wnr].path[0] = EOS;	/* als unbenutzt markieren */
		return ret;
	}

	sort_mydtas(wnr);
	calc_obj_look(wnr, TRUE);
	re_arrange(wnr);
		 
	/* Titel ist " <pfad><maske> "	*/
	/* ------------------------------- */

	set_title(wnr);

	if (!fast)
	{
		graf_growbox((GRECT *) &(windows[0].pobj+i+1)->ob_x, (GRECT *) &windows[wnr].out);
	}

	wind_set_string(windows[wnr].handle, WF_NAME, windows[wnr].title);
	wind_set_string(windows[wnr].handle, WF_INFO, windows[wnr].info);
	wind_open(windows[wnr].handle, &windows[wnr].out);
	return E_OK;
}


/****************************************************************
*
* Das Fenster mit Handle <whdl> wird nach oben gebracht.
*
****************************************************************/

void topped(int whdl)
{
	int wnr;

	if ((wnr = hdl_to_fnr(whdl)) > 0)
	{
		wind_set_int(whdl, WF_TOP, 0);
		Dsetdrv(windows[wnr].path[0] - 'A');
	}
}


/****************************************************************
*
* Ein Pfad hat sich geÑndert.
* Zeichne Fenster <wnr> neu.
* Wenn <free_flag> TRUE ist, hat sich auch das Laufwerk geÑndert.
*
****************************************************************/

void upd_path(int wnr, int free_flag)
{
	void dirty_info(int wnr);


	windows[wnr].shift = 0;
	set_title(wnr);
	wind_set_string(windows[wnr].handle, WF_NAME, windows[wnr].title);
	if (read_wind(wnr, free_flag))
	{
		close_immed(wnr);
		return;
	}
	sort_mydtas(wnr);
	calc_obj_look(wnr, TRUE);
	dirty_info(wnr);
	re_arrange(wnr);
	upd_wind(wnr);
}


/****************************************************************
*
* Im Fenster mit Handle <whdl> ist auf die Schlieûbox
* geklickt worden.
*
****************************************************************/

void closed(int whdl, int kstate)
{
	register int n,i;
	char *lastb;


	if ((n = hdl_to_fnr(whdl)) <= 0)
		return;

	/* Wenn die ALT- Taste gedrÅckt war, wird ein neues Fenster */
	/* erîffnet, das den Parent des Fensters enthÑlt			*/
	/* -------------------------------------------------------- */

	if (kstate & K_ALT)
	{
		char path[128];

		if (windows[n].path[3] == EOS)	/* schon Root */
			return;
		strcpy(path, windows[n].path);	/* Pfad erst kopieren */
		path[strlen(path) - 1] = EOS;		/* letzten '\' lîschen */
		lastb = strrchr(path, '\\');		/* letzten '\' ermitteln */
		*(lastb + 1) = EOS;				/* dahinter absÑgen */
		opn_newwnd(path);
		return;
	}

	free_pool(n);

	/* 1. Fall : Fenster war nicht die Root: zurÅckgehen */
	/* ------------------------------------------------- */

	if (windows[n].path[3] != EOS)
	{
		windows[n].path[strlen(windows[n].path) - 1] = EOS;
		lastb = strrchr(windows[n].path, '\\');
		*(lastb + 1) = EOS;
		upd_path(n, FALSE);
		return;
	}

	/* 2. Fall : Fenster war die Root: schlieûen		   */
	/* ------------------------------------------------- */

	for	(i = 0; i < n_icons; i++)
	{
		if (icon[i].isdisk == windows[n].path[0])
			break;
	}
	wind_close(whdl);
	wind_delete(whdl);
	windows[n].handle = -1;
	if (i < n_icons)
	{
		graf_shrinkbox((GRECT *) &(windows[0].pobj+i+1)->ob_x, &windows[n].out);
	}

	windows[n].path[0] = EOS; /* als unbenutzt markieren */
}


/****************************************************************
*
* Das Fenster mit Handle <wnr> wird sofort geschlossen.
*
****************************************************************/

void close_immed(int wnr)
{
	if (windows[wnr].path[0])
	{
		windows[wnr].path[3] = EOS;		/* zurÅck auf Root */
		Dsetpath(windows[wnr].path);		/* Standardpfad auf Root */
		closed(windows[wnr].handle, 0);	/* Fenster schlieûen */
	}
}


/****************************************************************
*
* Das Fenster mit Nummer <wnr> wird auf Maximalgrîûe gebracht.
*
****************************************************************/

void fulled(int wnr)
{
	register int whdl;
	GRECT prev, full;
	register WINDOW *f;
	register GRECT *out;


	if (wnr <= 0)
		return;

	f = &(windows[wnr]);			/* f = Fenster */
	if ((whdl = f->handle) <=0 )
		return;

	out = &(f->out);				/* out = Auûenmaûe */
	wind_get_grect(whdl, WF_PREVXYWH, &prev);
	wind_get_grect(whdl, WF_FULLXYWH, &full);
 
	/* 1. Fall: Das Fenster hat bereits Maximalgrîûe	  */
	/*		  Also muû das Fenster verkleinert werden */
	/* ------------------------------------------------ */

	if ((out->g_x == full.g_x) && (out->g_y == full.g_y) &&
		 (out->g_w == full.g_w) && (out->g_h == full.g_h))
	{
		graf_shrinkbox(&prev, &full);
		*out = prev;
	}

	/* 2. Fall: Das Fenster hat nicht Maximalgrîûe	  */
	/*		  Also Grîûe auf Maximum 			  */
	/* ------------------------------------------------ */

	else
	{
		graf_growbox(out, &full);
		*out = full;
	}

	wind_set_grect(whdl, WF_CURRXYWH, out);
	if (re_arrange(wnr))
		upd_wind(wnr);
}


/****************************************************************
*
* Der vertikale Scrollbalken von <wnr> ist auf die Position <newshift>
* zu bringen und der Redraw zu bewerkstelligen.
*
****************************************************************/

static void vshift(int wnr, int newshift)
{
	register int diff;		/* um soviele Zeilen scrollen */
	register int ycopy;		/* soviele y-Pixel werden kopiert */
	MFDB src_mfdb,dest_mfdb;
	int pxy[8];
	GRECT g;
	register int whdl;


	if (newshift < 0)
		newshift = 0;
	else
	if (newshift > windows[wnr].maxshift)
		newshift = windows[wnr].maxshift;

	if (0 == (diff = newshift - windows[wnr].shift))
		return;

	windows[wnr].shift = newshift;
	re_arrange(wnr);

	/* wenn mindestens eine Seite gescrollt, wird das Fenster neu aufgebaut */

	if (abs(diff) > windows[wnr].wlins)
	{
		upd_wind(wnr);
		return;
	}

	/* Von Zeilen in Pixel umrechnen */
	/* diff > 0: Balken nach unten, Inhalt nach oben */

	diff *= (showh + WYDIST);

	/* Fenster Åber Rechteckliste scrollen */

	whdl = windows[wnr].handle;
	Mgraf_mouse(M_OFF);
	wind_get_grect(whdl, WF_FIRSTXYWH, &g);
	do
	{
		if (rc_intersect(&desk_g, &g))
		{
			ycopy = g.g_h - abs(diff);		/* soviele Pixel blitten */
			if (ycopy > 0)
			{
				pxy[0] = pxy[4] = g.g_x;
				pxy[2] = pxy[6] = g.g_x + g.g_w - 1;
				if (diff > 0)
				{
					pxy[5] = g.g_y;
					pxy[1] = g.g_y + diff;
				}
				else
				{
					pxy[1] = g.g_y;
					pxy[5] = g.g_y - diff;
				}
				pxy[3] = pxy[1] + ycopy - 1;
				pxy[7] = pxy[5] + ycopy - 1;
				src_mfdb.fd_addr = dest_mfdb.fd_addr = NULL;
				set_deflt_clip();
				vro_cpyfm(vdi_handle, S_ONLY, pxy, &src_mfdb, &dest_mfdb);
				/* alles neu zeichnen, was nicht vom Zielraster Åberdeckt wurde */
				g.g_h -= ycopy;
				if (diff > 0)
					g.g_y += ycopy;
			}

			objc_draw(windows[wnr].pobj, 0, 1, &g);
		}
		wind_get_grect(whdl, WF_NEXTXYWH, &g);
	}
	while (g.g_w > 0);					/* bis Rechteckliste vollstÑndig */

	Mgraf_mouse(M_ON);
}


/****************************************************************
*
* Im Fenster mit Handle <whdl> ist einer der Scrollpfeile
* angeklickt worden.
* zusÑtzlich: Code -1 => Scrollbalken ganz nach oben
*		    Code -2 => Scrollbalken ganz nach unten
*
****************************************************************/

void arrowed(int wnr, int arrow)
{
	int newshift;


	if (wnr <= 0)
		return;
	newshift = windows[wnr].shift;
	switch (arrow)
	{
		case -1:
			newshift  = 0;
			break;

		case -2:
			newshift  = windows[wnr].maxshift;
			break;

		case WA_UPPAGE:
			newshift -= windows[wnr].wlins;
			break;

		case WA_DNPAGE:
			newshift += windows[wnr].wlins;
			break;

		case WA_UPLINE:
			newshift--;
			break;

		case WA_DNLINE:
			newshift++;
			break;

		default:
			show_free(wnr);
			return;
	}

	vshift(wnr, newshift);
}


/****************************************************************
*
* Im Fenster mit Handle <whdl> ist der horizontale Scrollbalken
* bewegt worden.
*
****************************************************************/

void hslid(int whdl)
{
	int n;

	if (0 >= (n = hdl_to_fnr(whdl)))
		return;
	show_free(n);
}


/****************************************************************
*
* Im Fenster mit Handle <whdl> ist der vertikale Scrollbalken
* bewegt worden.
*
****************************************************************/

void vslid(int whdl, int newpos)
{
	int wnr;
	long newshift;


	if ((wnr = hdl_to_fnr(whdl)) <= 0)
		return;
	newshift = (newpos * (windows[wnr].maxshift + 1L))/1000L;
	vshift(wnr, (int) newshift);
}


/****************************************************************
*
* Das Fenster mit Handle <whdl> ist in seiner Grîûe
* verÑndert worden.
*
****************************************************************/

void sized(int whdl, GRECT *g)
{
	register int n;


	if ((n = hdl_to_fnr(whdl)) <= 0)
		return;
	if (wind_set_grect(whdl, WF_CURRXYWH, g))
	{
		windows[n].out = *g;
		if (re_arrange(n))
			upd_wind(n);
	}
}


/****************************************************************
*
* Das Fenster mit Handle <whdl> ist verschoben worden.
*
****************************************************************/

void moved(int whdl, GRECT *g)
{
	register int n;


	if ((n = hdl_to_fnr(whdl)) <= 0)
		return;
	g->g_x &= ~7;
	if (wind_set_grect(whdl, WF_CURRXYWH, g))
	{
		windows[n].out = *g;
		calc_in(n);
	}
}


/****************************************************************
*
* Die folgende "Message" tritt nicht auf und hat keine Bedeutung.
*
****************************************************************/

#pragma warn -par

void newtop(int whdl)
{
}

void untopped(int whdl)
{
}

#pragma warn +par


/****************************************************************
*
* Ein Disketteninhalt hat sich geÑndert.
* Zeichnet alle Fenster, die das Laufwerk <lw> darstellen, neu.
*
****************************************************************/

static void upd_disk(int lw, int free_flag)
{
	register int i;

	dirty_drives[lw] = 0;		/* dirty- Flag lîschen */

	if (free_flag)
		dinfo[lw].b_clsiz = 0L;	  /* ungÅltig machen */

	for	(i = 1; i <= NUM_WINDOWS; i++)
		if (windows[i].path[0] == (lw+'A'))
		{
			if (read_wind(i, free_flag))
			{
				close_immed(i);
				break;
			}
			sort_mydtas(i);
			calc_obj_look(i, TRUE);
			wind_set_string(windows[i].handle, WF_INFO, windows[i].info);
			re_arrange(i);
			upd_wind(i);
		}
}


/****************************************************************
*
* Mehrere Disketteninhalte haben sich geÑndert.
* Zeichnet alle Fenster, die in der globalen Liste <drives>
* enthalten sind, neu.
*
****************************************************************/

void upd_drives(void)
{
	register int i;

	for	(i = 0; i < 16; i++)
		if (dirty_drives[i])
			upd_disk(i, dirty_drives[i] == TRUE);
}


/****************************************************************
*
* Die Darstellungsart (Bilder/Text) hat sich geÑndert.
* Zeichnet alle Fenster neu.
*
****************************************************************/

void upd_show(void)
{
	register int i;

	for	(i = 1; i <= NUM_WINDOWS; i++)
		if (windows[i].path[0])
		{
			save_select_status(i);
			calc_obj_look(i, FALSE);
			re_arrange(i);
			upd_wind(i);
		}
}


/****************************************************************
*
* Die Textdarstellungsart (zeige auch ...) hat sich geÑndert.
* Zeichnet alle Fenster neu, wenn Textdarstellung eingestellt.
*
****************************************************************/

void upd_is(void)
{
	register int i;

	if (settings.show_mode == M_ABILDR)
		return;
	for	(i = 1; i <= NUM_WINDOWS; i++)
		if (windows[i].path[0])
		{
			save_select_status(i);
			calc_obj_look(i, FALSE);
			re_arrange(i);
			upd_wind(i);
		}
}


/****************************************************************
*
* Der Sortiermodus hat sich geÑndert.
* Zeichnet alle Fenster neu.
*
****************************************************************/

void upd_sort(void)
{
	register int i;

	for	(i = 1; i <= NUM_WINDOWS; i++)
		if (windows[i].path[0])
		{
			save_select_status(i);
			sort_mydtas(i);
			calc_obj_look(i, FALSE);
			upd_wind(i);
		}
}


/****************************************************************
*
* Die Maske fÅr Fenster <wnr> hat sich geÑndert.
* Zeichnet das Fenster neu.
*
****************************************************************/

void upd_mask(int wnr)
{
	if (windows[wnr].path[0])
	{
		int ow,ob;

		if (icsel(&ow, &ob) && ow != wnr)
			dsel_all();
		set_title(wnr);
		save_select_status(wnr);
		sort_mydtas(wnr);
		calc_obj_look(wnr, FALSE);
		re_arrange(wnr);
		wind_set_string(windows[wnr].handle, WF_NAME, windows[wnr].title);
		wind_set_string(windows[wnr].handle, WF_INFO, windows[wnr].info);
		upd_wind(wnr);
	}
}


/****************************************************************
*
* Die Anzahl/Art der selektierten Dateien fÅr Fenster <wnr> hat
* sich geÑndert. Merkt nur die Flags.
*
****************************************************************/

char dirty[NUM_WINDOWS+1];

void dirty_info(int wnr)
{
	if (wnr > 0 && windows[wnr].path[0])
		dirty[wnr] = TRUE;
}


/****************************************************************
*
* Die Anzahl/Art der selektierten Dateien fÅr Fenster <wnr> hat
* sich geÑndert. Zeichnet die neuen Infozeilen neu.
*
****************************************************************/

void upd_infos(void)
{
	register int wnr;
	

	for	(wnr = 1; wnr <= NUM_WINDOWS; wnr++)
	{
		if (dirty[wnr] && windows[wnr].path[0])
		{
			set_info(wnr);
			wind_set_string(windows[wnr].handle, WF_INFO, windows[wnr].info);
			dirty[wnr] = FALSE;
		}
	}
}


/****************************************************************
*
* Der Spaltenmodus hat sich geÑndert.
* Zeichnet alle nîtigen Fenster neu, wenn "zeige als Text"
*
****************************************************************/

void upd_col(void)
{
	register int i,oldcol;


	if (settings.show_mode == M_ABILDR)
		return;

	for	(i = 1; i <= NUM_WINDOWS; i++)
		if (windows[i].path[0] && windows[i].shownum > 1)
		{
			if (settings.is_1col)
			{
				if (windows[i].cols > 1)
					goto doit;
				else
					continue;
			}
			else
			{
				oldcol = windows[i].cols;
				calc_collin(i);
				if (oldcol == windows[i].cols)
					continue;
			}

			doit:
			re_arrange(i);
			upd_wind(i);
		}
}


/****************************************************************
*
* DurchlÑuft alle selektierten Objekte eines Baums.
* Wenn die Funktion "cb" < 0 liefert, wird abgebrochen.
* Gibt Anzahl dieser Objekte zurÅck.
*
****************************************************************/

int walk_wind_sel_objs(int wnr, int (*cb)(int wnr, int obj))
{
	register int i,tail,count;
	register OBJECT *tree = windows[wnr].pobj;


	if ((i = (tree->ob_head)) <= 0)
		return 0;

	count = 0;
	tail = (tree++->ob_tail);
	for (; i <= tail; i++,tree++)
	{
		if (!(SELECTED & (tree->ob_state)) || (HIDETREE & (tree->ob_flags)))
			continue;
		count++;
		if (0 > (*cb)(wnr, i))
			break;
	}

	return count;
}
