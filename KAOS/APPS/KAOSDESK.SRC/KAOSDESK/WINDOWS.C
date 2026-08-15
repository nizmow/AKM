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

#define  WXOFFS	((status.showtyp == M_ATEXT) ? (2*char_w - 1) : (3))
#define  WYOFFS	(1)
#define  WXDIST	((status.showtyp == M_ATEXT) ? (2*char_w) : (3))
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
	for	(i = 1; i <= ANZFENSTER; i++)
		printf("Offset: %4ld Grîûe: %4ld\n", fenster[i].alloc_offs, fenster[i].alloc_size);
	printf("Frei: %3d\n", poolsize);
	Cnecin();
*/
	if (0L == (size = fenster[wnr].alloc_size))
		return;		/* war kein Speicher belegt */
	offs = fenster[wnr].alloc_offs;
	fenster[wnr].alloc_size = 0L;

	bottom = offs + size;
	top = 0L;
	for	(i = 1; i <= ANZFENSTER; i++)
	{

		/* Wenn das Fenster nicht unsereres ist, keinen allozierten		*/
		/* Speicher hat oder dieser unter unserem liegt, mÅssen keine	*/
		/* Bereiche verschoben werden								*/
		/* ------------------------------------------------------------- */

		if (i == wnr ||
			 !fenster[i].alloc_size ||
			 fenster[i].alloc_offs < offs)
			continue;
		if (fenster[i].alloc_offs+fenster[i].alloc_size > top)
			top = fenster[i].alloc_offs+fenster[i].alloc_size;

		num = fenster[i].realnum;
		for	(j = num,o = fenster[i].pobj + 1; j > 0; j--,o++)
		{
			if (o -> ob_type == G_STRING || o -> ob_type == G_ICON)
				o -> ob_spec.free_string -= size;
		}
		for	(j = num, icn = fenster[i].picb; j > 0; j--,icn++)
			icn -> ib_ptext -= size;

		(char *) fenster[i].pmydta	-= size;
		(char *) fenster[i].pobj 	-= size;
		(char *) fenster[i].picb 	-= size;
		(char *) fenster[i].pstr 	-= size;
		fenster[i].alloc_offs -= size;
	}

	if (top > bottom)
		memmove(poolptr+offs, poolptr+bottom, top-bottom);

	poolfree -= size;
	poolsize += fenster[wnr].realnum;

/*	Cconws("Austritt aus free\r\n");
	for	(i = 1; i <= ANZFENSTER; i++)
		printf("Offset: %4ld Grîûe: %4ld\n", fenster[i].alloc_offs, fenster[i].alloc_size);
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
	for	(i = 1; i <= ANZFENSTER; i++)
		printf("Offset: %4ld Grîûe: %4ld\n", fenster[i].alloc_offs, fenster[i].alloc_size);
	printf("Frei: %3d\n", poolsize);
	Cnecin();
*/
	num = fenster[wnr].realnum;
	size = sizeof(OBJECT) + num *
			(sizeof(OBJECT)+sizeof(MYDTA)+sizeof(ICONBLK)+sizeof(MYFNAME));
	fenster[wnr].alloc_offs = poolfree - poolptr;
	fenster[wnr].alloc_size = size;
	fenster[wnr].pobj = (OBJECT *)  (fenster[wnr].pmydta + num);
	fenster[wnr].picb = (ICONBLK *) (fenster[wnr].pobj + num + 1);
	fenster[wnr].pstr = (MYFNAME *) (fenster[wnr].picb + num);
	poolfree += size;
	poolsize -= num;

/*	Cconws("Austritt aus alloc\r\n");
	for	(i = 1; i <= ANZFENSTER; i++)
		printf("Offset: %4ld Grîûe: %4ld\n", fenster[i].alloc_offs, fenster[i].alloc_size);
	printf("Frei: %3d\n", poolsize);
	Cnecin();*/
}


/****************************************************************
*
* Initialisiert ein Objekt, das ein Icon darstellt
*
****************************************************************/

void init_icnobj(OBJECT *o, ICONBLK *icn, int typ, char *text)
{
	register OBJECT *srcobj;
	register int typ2;


	if (typ >= PGMICN_OFFS)			/* externes Icon */
		typ2 = (typ < DATICN_OFFS) ? I_PROGRA : I_DATEI;
	else
		typ2 = typ;

	srcobj = adr_icons + typ2;
	*icn = * ((srcobj -> ob_spec.iconblk));
	if (typ >= PGMICN_OFFS && typ < DATICN_OFFS) /* externes Programmicon */
	{
		typ -= PGMICN_OFFS;	/* Offset lîschen */
		icn -> ib_pdata = icf_pgmicn + typ * 0x38;
	}
	else
	if (typ >= DATICN_OFFS)		/* externes Dateiicon */
	{
		typ -= DATICN_OFFS;	/* Offset lîschen */
		icn -> ib_pdata = icf_daticn + typ * 0x38;
	}

	if (text)
		icn->ib_ptext = text;

	o->ob_type   = G_ICON;
	o->ob_flags  = NORMAL;
	o->ob_state  = NORMAL;
	o->ob_width  = srcobj -> ob_width;
	o->ob_height = srcobj -> ob_height;
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
	register OBJECT *tree = fenster[wnr].pobj;


	if ((i = (tree -> ob_head)) <= 0)
		return;
	tail = (tree++ -> ob_tail);
	for	(; i <= tail; i++,tree++)
	{
		if (!(SELECTED&(tree->ob_state)) || (HIDETREE&(tree->ob_flags)))
			fenster[wnr].pmydta[i - 1].flags &= ~2;
		else	fenster[wnr].pmydta[i - 1].flags |=  2;
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
	msg[3] = fenster[wnr].handle;
	msg[4] = fenster[wnr].out.g_x;
	msg[5] = fenster[wnr].out.g_y;
	msg[6] = fenster[wnr].out.g_w;
	msg[7] = fenster[wnr].out.g_h;
	appl_write(ap_id, 16, msg);
}


/****************************************************************
*
* Sortiert die Dateien fÅr Fenster <wnr>.
* Auûerdem werden die "nicht zur Maske passenden" Dateien nach
* hinten sortiert und <fenster[wnr].shownum> initialisiert.
*
****************************************************************/

static int cmp_mydtas(MYDTA *f1, MYDTA *f2)
{
	register int  r;
	register long l;
	register char *n1,*n2;


	if (((f1->flags)&1) != ((f2->flags)&1))		/* Match- Flag */
		return ((f2->flags)&1 - ((f1->flags)&1));

	if (status.sorttyp == M_SNICHT)
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

	switch(status.sorttyp)
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
	MYDTA *files = fenster[wnr].pmydta;
	int anzahl  = fenster[wnr].realnum;
	register int i,match;
	register MYDTA *dtas;


	fenster[wnr].shownum = 0;
	for	(dtas = files, i = 0; i < anzahl; i++, dtas++)
	{
		match = ((dtas->attrib & F_SUBDIR) ||
			 (fname_match(dtas->filename, fenster[wnr].maske)));
		if (match)
		{
			dtas->flags |=  1;
			fenster[wnr].shownum++;
		}
		else
			dtas->flags &= ~1;
	}

	shelsort(fenster[wnr].pmydta, (size_t) fenster[wnr].realnum,
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


	s = fenster[wnr].title;
	p = fenster[wnr].path;
	m = fenster[wnr].maske;
	*s++ = ' ';
	if (strlen(p) + strlen(m) > 124)
	{
#if		COUNTRY==FRG
		strcpy(s, "Pfad zu tief");
#elif	COUNTRY==USA
		strcpy(s, "Path to deep");
#endif
	}
	else
	{
		strcpy(s, p);
		strcat(s, m);
		if (p[0] == 'R')
#if		COUNTRY==FRG
			strcat(s, "  (Rommodul)");
#elif	COUNTRY==USA
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
	register char *s 			= fenster[wnr].info;
	register unsigned long size 	= 0L;
		    unsigned long ssize	= 0L;
	register int num 			= fenster[wnr].shownum;
		    int snum			= 0;
	register int i;
	MYDTA *file 				= fenster[wnr].pmydta;
	MYDTA *sfile 				= NULL;
	OBJECT *o					= fenster[wnr].pobj + 1;


	for	(i = num; i > 0; i--,file++,o++)
	{
		size += (file -> filesize);
		if ((o -> ob_state) & SELECTED)
		{
			snum++;
			ssize += (file -> filesize);
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


		if (snum == 0 && fenster[wnr].too_much)
		{
#if		COUNTRY==FRG
			strcpy(s, " mehr als ");
			objs = " Objekte";
#elif	COUNTRY==USA
			strcpy(s, " more than ");
			objs = " objects";
#endif
		}
		else
		{
			ultoa(size, s, 10);
#if		COUNTRY==FRG
			strcat(s, " Bytes in ");
			objs = (num == 1) ? " Objekt" : " Objekten";
#elif	COUNTRY==USA
			strcat(s, " bytes in ");
			objs = (num == 1) ? " object" : " objects";
#endif
		}

		s += strlen(s);
		itoa(num, s, 10);
		if (snum)
		{
#if		COUNTRY==FRG
			strcat(s, " selektierten");
#elif	COUNTRY==USA
			strcat(s, " selected");
#endif
		}

		strcat(s, objs);
	}
	else
	{
		if (0 == ((sfile -> attrib) & F_SUBDIR))
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
#if		COUNTRY==FRG
			strcat(s, " Bytes | ");
#elif	COUNTRY==USA
			strcat(s, " bytes | ");
#endif
			s += strlen(s);
		}

		date_to_str(s, sfile -> date);
		strcat(s, " | ");
		s += strlen(s);
		time_to_str(s, sfile -> time);
		i = suffixtyp(sfile->filename);
		if (i == '_T')
			i = (NULL != file_to_defapp(sfile->filename));
		else
			i = TRUE;
		if (i || ((sfile -> attrib) & (F_RDONLY+F_HIDDEN+F_SYSTEM+F_ARCHIVE)))
			strcat(s, " | ");
		if ((sfile -> attrib) & F_RDONLY)
			strcat(s, "R");
		if ((sfile -> attrib) & F_SYSTEM)
			strcat(s, "S");
		if ((sfile -> attrib) & F_HIDDEN)
			strcat(s, "H");
		if ((sfile -> attrib) & F_ARCHIVE)
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
	long bytes;


	lwc  = fenster[wnr].path[0];
	s    = alertstr;
#if		COUNTRY==FRG
	strcpy(s, "[0][Freier Platz auf Laufwerk ");
#elif	COUNTRY==USA
	strcpy(s, "[0][Free space at drive ");
#endif
	s += strlen(s);
	*s++ = lwc;
	strcpy(s, ":| |      ");
	s   += strlen(s);
	lwc -= 'A';
	bytes = dinfo[lwc].b_free * dinfo[lwc].b_secsiz * dinfo[lwc].b_clsiz;
	print_ul(bytes, s);
#if		COUNTRY==FRG
	strcat(s, " Bytes|][  OK  ]");
#elif	COUNTRY==USA
	strcat(s, " bytes|][  OK  ]");
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
				*p = ((adr_icons + icon[i].icontyp) -> ob_spec.iconblk) -> ib_ptext;
			}

			if (strcmp(old_text, *p))
				obj_malen(0, i+1);
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
	register int anzahl,drv;
	long errcode;
	DTA  dta;
	MYDTA *ziele;


	Mgraf_mouse(HOURGLASS);
	Fsetdta(&dta);
	drv = fenster[wnr].path[0] - 'A';
	dirty_drives[drv] = 0;		/* dirty- Flag lîschen */
	errcode = E_OK;

	/* Bei Fehler auf Root umschalten */
	/* ------------------------------ */

	do
	{
		if (errcode != E_OK)
		{
			fenster[wnr].shift = 0;
			if (errcode != EPTHNF || fenster[wnr].path[3] == EOS)
			{
				Mgraf_mouse(ARROW);
				return errcode;
			}
			else
			{
				fenster[wnr].path[3] = EOS;
				free_flag = TRUE;
				set_title(wnr);
				wind_set_string(fenster[wnr].handle, WF_NAME, fenster[wnr].title);
			}
		}
	
		/* Pfad ÅberprÅfen, ggf. auf Root setzen */
		/* ------------------------------------- */

		Dsetdrv(drv);					/* wegen TOS- Fehler */
		errcode = Dsetpath(fenster[wnr].path);
		if (errcode != E_OK)
			continue;
	
		/* Diskname einlesen (von der Root) */
		/* -------------------------------- */

		errcode = read_dname(drv, dta.d_fname);
		set_dname(drv, dta.d_fname);
		if (errcode != EFILNF && errcode != E_OK)
			continue;
	
		/* Freien Speicher ermitteln */
		/* ------------------------- */
	
		if (free_flag)
		{
			long slsize;

			if (dinfo[drv].b_clsiz == 0L)	/* ungÅltig */
				if (0L > (errcode = MDfree(&dinfo[drv], drv + 1)))
					continue;
			if (dinfo[drv].b_free == dinfo[drv].b_total)
				slsize = -1L;
			else
				slsize = ((dinfo[drv].b_total-dinfo[drv].b_free) * 1000L) / dinfo[drv].b_total;

			wind_set_int(fenster[wnr].handle, WF_HSLSIZE, (int) slsize);
		}

		/* Speicherverwaltung, alten Speicher freigeben und */
		/* gucken, wieviele Objekte noch reinpassen		  */

		free_pool(wnr);
		fenster[wnr].pmydta = (MYDTA *) poolfree;	/* hier ist Platz */

		/* Verzeichnis einlesen */
		/* -------------------- */
	
		strcpy(path, fenster[wnr].path);
		strcat(path, "*.*");
		ziele = fenster[wnr].pmydta;
		anzahl = 0;
		errcode = Fsfirst(path, (status.show_all) ? 0x37 : 0x10);
		while (errcode == E_OK && anzahl < poolsize)
		{
			if (dta.d_fname[0] != '.' ||
			      (dta.d_fname[1] != EOS && dta.d_fname[1] != '.')
			     )
			{
				ziele->number = anzahl;
				ziele->time = dta.d_time;
				ziele->date = dta.d_date;
				ziele->filesize = dta.d_length;
				ziele->attrib = dta.d_attrib;
				strcpy(ziele->filename,dta.d_fname);
				anzahl++;
				ziele++;
			}
			errcode = Fsnext();
		}

		if (errcode == E_OK)
			fenster[wnr].too_much = TRUE;
		else
		{
			fenster[wnr].too_much = FALSE;
			if (errcode == EFILNF || errcode == ENMFIL)
				errcode = E_OK;
		}
	}
	while (errcode != E_OK);

	fenster[wnr].realnum = anzahl;
	alloc_pool(wnr);

	/* Icontypen und Exec- Flag festlegen */
	/* ---------------------------------- */

	ziele = fenster[wnr].pmydta;
	for	(ziele = fenster[wnr].pmydta;
		 ziele < fenster[wnr].pmydta + anzahl;
		 ziele++)
	{
		int typ;

		ziele -> flags = 0;					/* Datei */

		if ((ziele -> attrib) & F_SUBDIR)
		{
			ziele -> icontyp = I_ORDNER;
			continue;
		}

		typ = suffixtyp(ziele -> filename);

		/* Batchdateien bekommen das Icon I_BTCHDA */
		/* --------------------------------------- */

		if ((typ & 0xff) == 'B')
			typ = I_BTCHDA;

		/* Programmdateien bekommen, entweder das ihnen	*/
		/* zugeordnete Icon (Offset PGMICN_OFFS) oder	*/
		/* sonst I_PROGRA							*/
		/* ---------------------------------------------- */

		else
		if ((typ & 0xff) == 'P')
		{
			ziele -> flags = 4;				/* Programm */
			typ = find_iconfile_pgm(ziele -> filename);
		}

		/* Sonstige Dateien bekommen das ihnen zugewiesene	*/
		/* Icon (Offset DATICN_OFFS) oder sonst I_DATEI		*/
		/* --------------------------------------------------- */

		else
		{
			typ = find_iconfile_dat(ziele -> filename);
		}

		ziele -> icontyp = typ;
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


	f = &(fenster[wnr]);
	gescrollte_pixel = f->shift*(showh+WYDIST);

	wind_calc(WC_WORK, FENSTERTYP, &f->out, &f->in);
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
	int x = fenster[wnr].in.g_w - WXOFFS + WXDIST;
	int y = fenster[wnr].in.g_h - WYOFFS + WYDIST;
	int c;


	if (status.is_1col && status.showtyp == M_ATEXT)
		c = 1;
	else
	{
		c = x / (showw + WXDIST);
		if (c < 1)
			c = 1;
	}

	fenster[wnr].cols = c;
	fenster[wnr].wlins = y / (showh + WYDIST);
	fenster[wnr].lins = (fenster[wnr].shownum + fenster[wnr].cols - 1) /
					 fenster[wnr].cols;
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
	int anzahl 	= fenster[wnr].shownum;
	OBJECT *o 	= fenster[wnr].pobj;
	MYFNAME *f 	= fenster[wnr].pstr;
	MYDTA *d 		= fenster[wnr].pmydta;
	ICONBLK *c 	= fenster[wnr].picb;
	int 	typ;
	register int i;
	register char *s,*t,*fname;


	if (status.showtyp == M_ATEXT)
	{
		showh = char_h;
		showw = char_w * (16 + 8*status.is_groesse +
							 10*status.is_datum +
							 7*status.is_zeit
						);
	}
	else
	{
		showh = (adr_icons + I_DATEI) -> ob_height;
		showw = (adr_icons + I_DATEI) -> ob_width;
	}

	/* Objekt der weiûen Hintergrundbox */

	o -> ob_next = -1;
	o -> ob_type = G_BOX;
	o -> ob_state = NORMAL;
	o -> ob_spec.index = (long) (WHITE);

	if (anzahl > 0)
	{
		o -> ob_flags = NONE;
		o -> ob_head = 1;
		o -> ob_tail = anzahl;
	}
	else
	{
		o -> ob_flags = LASTOB;
		o -> ob_head = o -> ob_tail = -1;
	}

	/* Objekte der Dateien */

	for	(i = 1; i <= anzahl; i++,c++,d++,f++)
	{
		o++;
		o -> ob_next = (i < anzahl) ? (i+1) : (0);
		o -> ob_head = o -> ob_tail = -1;
		o -> ob_flags = (i < anzahl) ? NONE : LASTOB;

		typ = d -> icontyp;
		if (status.showtyp == M_ABILDR)
		{
			strcpy((char *) f, d -> filename);
			init_icnobj(o, c, typ, (char *) f);
			o -> ob_state = ((!dsel_flag) && (d -> flags & 2))
					 	? SELECTED+WHITEBAK : NORMAL+WHITEBAK;
		}
		else
		{
			s = t = (char *) f;
			fname = d -> filename;
			*s++ = ' ';
			if (typ == I_ORDNER)
				*s++ = '\7';
			else
			if (typ == I_PROGRA || (typ >= PGMICN_OFFS && typ < DATICN_OFFS))
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

			if (status.is_groesse)
			{
				char gr[10],*g;
				int  j;
	
				if ((d -> attrib) & F_SUBDIR)
					gr[0] = EOS;
				else
					ultoa(d -> filesize, gr, 10);

				for	(j = 8 - (int) strlen(gr); j > 0; j--)
					*s++ = ' ';

				g = gr;
				while (*g)
					*s++ = *g++;
			}
			if (status.is_datum)
			{
				*s++ = ' ';
				*s++ = ' ';
				date_to_str(s, d -> date);
				s += 8;
			}
			if (status.is_zeit)
			{
				*s++ = ' ';
				*s++ = ' ';
				time_to_str(s, d -> time);
				s += 5;
			}
			*s = EOS;
	
			o -> ob_type = G_USERDEF;
			o -> ob_spec.userblk = &userblk;
			o -> ob_state = ((!dsel_flag) && (d -> flags & 2))
					 	? SELECTED : NORMAL;
		}
		o -> ob_width = showw;
		o -> ob_height = showh;
	}
	set_info(wnr);
}


/****************************************************************
*
* Organisiert das Fenster neu, d.h. legt Position der Objekte fest.
* gibt TRUE zurÅck, falls sich fenster.cols oder fenster.shift
* geÑndert haben.
*
****************************************************************/

static int re_arrange(int wnr)
{
	register int i,j;
	int x,y,cols;
	int oldcols 	= fenster[wnr].cols;
	int oldshift 	= fenster[wnr].shift;
	int anzahl 	= fenster[wnr].shownum;
	OBJECT *o 	= fenster[wnr].pobj;
	long vsize,vpos;


	calc_in(wnr);			/* Innenmaûe berechnen */
	calc_collin(wnr);		/* Zeilen/Spalten berechnen */
	cols = fenster[wnr].cols;

	fenster[wnr].maxshift = fenster[wnr].lins - fenster[wnr].wlins;
	if (fenster[wnr].maxshift < 0)
		fenster[wnr].maxshift = 0;

	if (fenster[wnr].shift < 0)
		fenster[wnr].shift = 0;
	if (fenster[wnr].shift > fenster[wnr].maxshift)
		fenster[wnr].shift = fenster[wnr].maxshift;

	if (fenster[wnr].lins > 0)
		vsize = (1000L * fenster[wnr].wlins) / fenster[wnr].lins;
	else
		vsize = 1000L;

	if (vsize > 1000L)
		vsize = 1000L;

	wind_set_int(fenster[wnr].handle, WF_VSLSIZE, (int) vsize);

	if (fenster[wnr].maxshift > 0)
		vpos = (1000L * fenster[wnr].shift) / fenster[wnr].maxshift;
	else
		vpos = 1L;

	if (vpos > 1000L)
		vpos = 1000L;

	if (vpos < 1)
		vpos = 1;

	wind_set_int(fenster[wnr].handle, WF_VSLIDE, (int) vpos);

	/* Position und Hîhe ggf. korrigieren (von calc_in schon gesetzt) */
	o -> ob_y 	= fenster[wnr].in.g_y - fenster[wnr].shift*(showh+WYDIST);
	o -> ob_height = fenster[wnr].in.g_h + fenster[wnr].shift*(showh+WYDIST);
	for	(i = 0, y = WYOFFS, x = WXOFFS, j = anzahl; j > 0; i++,j--)
	{
		o++;
		if (i >= cols)
		{
			i = 0;
			x = WXOFFS;
			y += showh + WYDIST;
		}
		o -> ob_x = x;
		o -> ob_y = y;
		x += showw + WXDIST;
	}
	if (anzahl < oldcols)
		oldcols = anzahl;
	if (anzahl < cols)
		cols = anzahl;
	return (oldcols != cols || oldshift != fenster[wnr].shift);
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
		fenster[wnr].handle = wind_create(FENSTERTYP, &desk_g);
	}
	else
	{
		fenster[wnr].handle = -1;
	}

	if (fenster[wnr].handle < 0)
	{
		ret = ENOWND;
		goto err;
	}

	Dsetdrv(fenster[wnr].path[0] - 'A');
	for	(i = 0; i < n_icons; i++)
	{
		if (icon[i].isdisk == fenster[wnr].path[0])
			break;
	}
	/* Verzeichnis einlesen */
	if (!fast)
		fenster[wnr].shift = 0;
	dinfo[fenster[wnr].path[0] - 'A'].b_clsiz = 0L;	  /* ungÅltig machen */
	ret =  read_wind(wnr, TRUE);
	if (ret)
	{		/* Fehler */
		wind_delete(fenster[wnr].handle);
		fenster[wnr].handle = -1;
		err:
		fenster[wnr].path[0] = EOS;	/* als unbenutzt markieren */
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
		graf_growbox((GRECT *) &(fenster[0].pobj+i+1)->ob_x, (GRECT *) &fenster[wnr].out);
	}

	wind_set_string(fenster[wnr].handle, WF_NAME, fenster[wnr].title);
	wind_set_string(fenster[wnr].handle, WF_INFO, fenster[wnr].info);
	wind_open(fenster[wnr].handle, &fenster[wnr].out);
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
		Dsetdrv(fenster[wnr].path[0] - 'A');
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


	fenster[wnr].shift = 0;
	set_title(wnr);
	wind_set_string(fenster[wnr].handle, WF_NAME, fenster[wnr].title);
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
		char	path[128];

		if (fenster[n].path[3] == EOS)	/* schon Root */
			return;
		strcpy(path, fenster[n].path);	/* Pfad erst kopieren */
		path[strlen(path) - 1] = EOS;		/* letzten '\' lîschen */
		lastb = strrchr(path, '\\');		/* letzten '\' ermitteln */
		*(lastb + 1) = EOS;				/* dahinter absÑgen */
		opn_newwnd(path);
		return;
	}

	free_pool(n);

	/* 1. Fall : Fenster war nicht die Root: zurÅckgehen */
	/* ------------------------------------------------- */

	if (fenster[n].path[3] != EOS)
	{
		fenster[n].path[strlen(fenster[n].path) - 1] = EOS;
		lastb = strrchr(fenster[n].path, '\\');
		*(lastb + 1) = EOS;
		upd_path(n, FALSE);
		return;
	}

	/* 2. Fall : Fenster war die Root: schlieûen		   */
	/* ------------------------------------------------- */

	for	(i = 0; i < n_icons; i++)
	{
		if (icon[i].isdisk == fenster[n].path[0])
			break;
	}
	wind_close(whdl);
	wind_delete(whdl);
	fenster[n].handle = -1;
	if (i < n_icons)
		graf_shrinkbox((GRECT *) &(fenster[0].pobj+i+1)->ob_x, &fenster[n].out);
	fenster[n].path[0] = EOS; /* als unbenutzt markieren */
}


/****************************************************************
*
* Das Fenster mit Handle <wnr> wird sofort geschlossen.
*
****************************************************************/

void close_immed(int wnr)
{
	if (fenster[wnr].path[0])
	{
		fenster[wnr].path[3] = EOS;		/* zurÅck auf Root */
		Dsetpath(fenster[wnr].path);		/* Standardpfad auf Root */
		closed(fenster[wnr].handle, 0);	/* Fenster schlieûen */
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
	f = &(fenster[wnr]);			/* f = Fenster */
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
	register int diff;		/* um soviele Zeilen Scrollen */
	register int ycopy;		/* soviele y-Pixel werden kopiert */
	MFDB src_mfdb,dest_mfdb;
	int  pxy[8];
	GRECT g;
	register int whdl;


	if (newshift < 0)
		newshift = 0;
	else
	if (newshift > fenster[wnr].maxshift)
		newshift = fenster[wnr].maxshift;

	if (0 == (diff = newshift - fenster[wnr].shift))
		return;

	fenster[wnr].shift = newshift;
	re_arrange(wnr);

	/* wenn >= 1 Seite gescrollt, wird das Fenster neu aufgebaut */

	if (abs(diff) > fenster[wnr].wlins)
	{
		upd_wind(wnr);
		return;
	}

	/* Von Zeilen in Pixel umrechnen */
	/* diff > 0: Balken nach unten, Inhalt nach oben */

	diff *= (showh + WYDIST);

	/* Fenster Åber Rechteckliste scrollen */

	whdl = fenster[wnr].handle;
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

			objc_draw(fenster[wnr].pobj, 0, 1, &g);
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
	int	newshift;


	if (wnr <= 0)
		return;
	newshift = fenster[wnr].shift;
	switch(arrow)
	{
		case -1:			newshift  = 0;
						break;
		case -2:			newshift  = fenster[wnr].maxshift;
						break;
		case WA_UPPAGE:	newshift -= fenster[wnr].wlins;
						break;
		case WA_DNPAGE:	newshift += fenster[wnr].wlins;
						break;
		case WA_UPLINE:	newshift--;
						break;
		case WA_DNLINE:	newshift++;
						break;
		default:			show_free(wnr);
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
	int	wnr;
	long newshift;


	if ((wnr = hdl_to_fnr(whdl)) <= 0)
		return;
	newshift = (newpos * (fenster[wnr].maxshift + 1L))/1000L;
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
		fenster[n].out = *g;
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
		fenster[n].out = *g;
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
	for	(i = 1; i <= ANZFENSTER; i++)
		if (fenster[i].path[0] == (lw+'A'))
		{
			if (read_wind(i, free_flag))
			{
				close_immed(i);
				break;
			}
			sort_mydtas(i);
			calc_obj_look(i, TRUE);
			wind_set_string(fenster[i].handle, WF_INFO, fenster[i].info);
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

	for	(i = 1; i <= ANZFENSTER; i++)
		if (fenster[i].path[0])
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

	if (status.showtyp == M_ABILDR)
		return;
	for	(i = 1; i <= ANZFENSTER; i++)
		if (fenster[i].path[0])
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

	for	(i = 1; i <= ANZFENSTER; i++)
		if (fenster[i].path[0])
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
	if (fenster[wnr].path[0])
	{
		int ow,ob;

		if (icsel(&ow, &ob) && ow != wnr)
			dsel_all();
		set_title(wnr);
		save_select_status(wnr);
		sort_mydtas(wnr);
		calc_obj_look(wnr, FALSE);
		re_arrange(wnr);
		wind_set_string(fenster[wnr].handle, WF_NAME, fenster[wnr].title);
		wind_set_string(fenster[wnr].handle, WF_INFO, fenster[wnr].info);
		upd_wind(wnr);
	}
}


/****************************************************************
*
* Die Anzahl/Art der selektierten Dateien fÅr Fenster <wnr> hat
* sich geÑndert. Merkt nur die Flags.
*
****************************************************************/

char dirty[ANZFENSTER+1];

void dirty_info(int wnr)
{
	if (wnr > 0 && fenster[wnr].path[0])
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
	

	for	(wnr = 1; wnr <= ANZFENSTER; wnr++)
	{
		if (dirty[wnr] && fenster[wnr].path[0])
		{
			set_info(wnr);
			wind_set_string(fenster[wnr].handle, WF_INFO, fenster[wnr].info);
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


	if (status.showtyp == M_ABILDR)
		return;
	for	(i = 1; i <= ANZFENSTER; i++)
		if (fenster[i].path[0] && fenster[i].shownum > 1)
		{
			if (status.is_1col)
			{
				if (fenster[i].cols > 1)
					goto doit;
				else
					continue;
			}
			else
			{
				oldcol = fenster[i].cols;
				calc_collin(i);
				if (oldcol == fenster[i].cols)
					continue;
			}

			doit:
			re_arrange(i);
			upd_wind(i);
		}
}