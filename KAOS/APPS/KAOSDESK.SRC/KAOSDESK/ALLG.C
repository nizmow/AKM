/*********************************************************************
*
* Dieses Modul enthÑlt allgemeine Routinen.
*
*********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <mgx_dos.h>	/* include before tos.h */
#include <tos.h>		/* include before mt_aes.h */
#include <tosdefs.h>
#include <mt_aes.h>
#include <vdi.h>
#include "k.h"

#define DATESEP 	'-'
#define TIMESEP	':'


#if	MAGIXDESK == 0

/****************************************************************
*
* Gibt zu einem DOS- Fehler den entsprechenden Text
*
****************************************************************/

const char *err_string(int e)
{
	static const char *errs[] =
	{
		"Allgemeiner Fehler",
		"Laufwerk nicht bereit",
		"Unbekanntes Kommando",
		"CRC-Fehler",
		"Kommando nicht mîglich",
		"Spur nicht gefunden",
		"Unbekanntes Medium",
		"Sektor nicht gefunden",
		"Kein Papier",
		"Schreibfehler",
		"Lesefehler",
		"Disk schreibgeschÅtzt",
		"unerlaubter Diskwechsel",
		"unbekanntes GerÑt",
		"Defekte Sektoren",

		"UngÅltiger DOS-Aufruf",
		"Datei nicht gefunden",
		"Pfad nicht gefunden",
		"Keine Handles mehr",
		"Zugriff unmîglich",
		"UngÅltiges Handle",
		"Zuwenig Speicher",
		"UngÅltiger Speicherblock",
		"UngÅltiges Laufwerk",
		"Nicht dasselbe Laufwerk",
		"Keine weiteren Dateien",
		"UngÅltiger Fseek-Bereich",
		"Interner Fehler",
		"Datei nicht ausfÅhrbar",
		"Mshrink-Fehler",

		"User break",
		"68000- Exception",

		"Pfad zu tief"
	};

	static const char errn[] =
	{
		ERROR,EDRVNR,EUNCMD,E_CRC,EBADRQ,E_SEEK,EMEDIA,
		ESECNF,EPAPER,EWRITF,EREADF,EWRPRO,E_CHNG,
		EUNDEV,EBADSF,
		EINVFN,EFILNF,EPTHNF,ENHNDL,
		EACCDN,EIHNDL,ENSMEM,EIMBA,EDRIVE,ENSAME,
		ENMFIL,ERANGE,EINTRN,EPLFMT,EGSBF,
		EBREAK,EXCPT,
		EPTHOV
	};

	register int i;

	for	(i = 0; i < sizeof(errn); i++)
	{
		if (e == (int) errn[i])
			return errs[i];
	}

	return NULL;
}


/****************************************************************
*
* Gibt zu einem DOS-Fehler die entsprechende Alertbox.
* Unter MAGIX wird form_xerr() verwendet, das automatisch
* die passende Sprache wÑhlt.
*
****************************************************************/

char *err_file;

long err_alert(long e)
{
	if (ismagix)
	{
		form_xerr(e, err_file);
	}
	else
	{
		const char *alrts;
		char s[90];

		if (e == E_OK || (int) (e) == (int) (EBREAK))
		{
			err_file = NULL;
			return e;
		}
		if (e == EDRIVE || e == ENSMEM || e == ENHNDL)
		{
			form_error((int) ((~e)-30));
		}
		else
		{
			alrts = err_string((int) e);
			if (!alrts)
			{
				alrts = "TOS-Fehler";
			}

			strcpy(s, "[3][");
			if (err_file == (char *) -1L)
			{
				strcpy(s+4, "Programm gab zurÅck :|");
			}
			else
			if (err_file)
			{
				if (err_file[0] != EOS && err_file[1] == ':')
				{
					s[4] = err_file[0];
					s[5] = ':';
					s[6] = EOS;
				}

				strcat(s, get_name(err_file));
				strcat(s, "|");
			}

			strcat(s, alrts);
			strcat(s, "| |(Fehler #");
			itoa((int) e, s+strlen(s), 10);
			strcat(s, ")|][Abbruch]");
			form_alert(1, s);
		}
	}

	err_file = NULL;
	return e;
}
#else

char *err_file;

long err_alert(long e)
{
	form_xerr(e, err_file);
	err_file = NULL;
	return e;
}
#endif


/*********************************************************************
*
* Ermittelt zu einem vollen Pfadnamen den Zeiger auf den
* reinen Dateinamen
*
*********************************************************************/

char *get_name(char *path)
{
	register char *n;

	n = strrchr(path, '\\');
	if (!n)
	{
		if ((*path) && (path[1] == ':'))
			path += 2;
		return path;
	}

	return n + 1;
}


/*********************************************************************
*
* Schaltet Maus ein/aus/Pfeil/Biene
*
*********************************************************************/

void Mgraf_mouse(int type)
{
	static int last = ARROW;

	if (type == 0x1000)
		type = last;
	graf_mouse(ABS(type), NULL);
	if (type >= 0 && type != 0x1000 && type != M_ON && type != M_OFF)
		last = type;
}


/*********************************************************************
*
* Wandelt DOS-Datum in eine Zeichenkette um.
*
*********************************************************************/

void date_to_str(char *s, unsigned int date)
{
	int t,m;

	t = date & 31;
	date >>= 5;
	m = date & 15;
	date >>= 4;
	date += 80;
	date %=100;
	*s++ = t/10 + '0';
	*s++ = t%10 + '0';
	*s++ = DATESEP;
	*s++ = m/10 + '0';
	*s++ = m%10 + '0';
	*s++ = DATESEP;
	*s++ = date/10 + '0';
	*s++ = date%10 + '0';
	*s = '\0';
}


/*********************************************************************
*
* Wandelt DOS-Zeit in eine Zeichenkette um.
*
*********************************************************************/

void time_to_str(char *s, unsigned int time)
{
	int min,sec;

	sec = 2 * (time & 31);
	time >>= 5;
	min = time & 63;
	time >>= 6;
	*s++ = time/10 + '0';
	*s++ = time%10 + '0';
	*s++ = TIMESEP;
	*s++ = min/10 + '0';
	*s++ = min%10 + '0';
	*s++ = TIMESEP;
	*s++ = sec/10 + '0';
	*s++ = sec%10 + '0';
	*s = '\0';
}


/*********************************************************************
*
* Setzt einen Laufwerk-Buchstaben in eine Schablone ein.
*
*********************************************************************/

void drv_to_str(char *s, char c)
{
	while (*s)
	{
		if (s[1] == ':')		/* nÑchstes Zeichen ist ':' */
		{
			*s = c;
			break;
		}

		s++;
	}
}


/*********************************************************************
*
* Wandelt "unsigned long" dezimal in ASCII mit Tausenderkomma.
* Setzt Zeiger hinter den String;
*
*********************************************************************/

char *print_ul(unsigned long num, unsigned long factor, char *p)
{
	static const char *units = "\0kMGT";
	const char *unit = units;
	register char *s;
	register int len;
	char hilfs[20];

	while(factor > 1024)
	{
		factor >>= 10;
		unit++;
	}

	if (factor == 512)
	{
		factor = 1;
		num >>= 1;
		unit++;
	}

	if (num > 10000000L)
	{
		num >>= 10;
		unit++;
	}

	num *= factor;		/* hopefully not overflow */

	ultoa(num, hilfs, 10);
	s = hilfs;
	*p++ = *s++;
	len = (int) strlen(s);

	while (len > 0)
	{
		if (len % 3 == 0)
			*p++ = '.';
		*p++ = *s++;
		len--;
	}

	*p++ = ' ';
	*p++ = *unit;
	*p = '\0';
	return p;
}


/*********************************************************************
*
* Rechnet Dateinamen ins interne Format
*
*********************************************************************/

void fname_int(char *s, char *d)
{
	register char *p = d;

	while (*s && *s != '.')			/* Name */
		*p++ = *s++;
	if (*s)						/* Punkt */
		s++;
	if (*s)						/* Extension */
	{
		while (p < d+8)
			*p++ = ' ';
		while (*s)
			*p++ = *s++;
	}

	*p = '\0';
}


/*********************************************************************
*
* Rechnet Dateinamen vom internen Format zurÅck
*
*********************************************************************/

void fname_ext(char *s, char *d)
{
	register char *p = s;

	while (*p && p < s+8)
	{
		if (*p != ' ')
			*d++ = *p++;
		else
			p++;
	}

	if (*p)
		*d++ = '.';

	while (*p)
		*d++ = *p++;

	*d = '\0';
}


/*********************************************************************
*
* Berechnet den Dateityp, und zwar:
*
* Endung PRG,TOS,TTP,APP : Programm
* Endung BAT,BTP         : Batchdatei
* sonst                  : Textdatei
*
*********************************************************************/

int suffixtyp(char *s)
{
	s = strrchr(s, '.');
	if (s)
	{
		s++;
		if (!strcmp(s, "PRG") || !strcmp(s, "APP"))
			return 'GP';
		if (!strcmp(s, "TOS"))
			return '_P';
		if (!strcmp(s, "TTP"))
			return 'PP';
		if (!strcmp(s, "BAT"))
			return '_B';
		if (!strcmp(s, "BTP"))
			return 'PB';
	}

	return '_T';
}


/*********************************************************************
*
* gibt != 0 bei passenden Namen zurÅck.
*
*********************************************************************/

int fname_match(char *fname, char *fmask)
{
	if (fmask[0] == '*' && fmask[1] == '.' && fmask[2] == '*')
	{
		return 1;
	}

	while (*fmask && *fname)
	{
		if (*fmask == '?')
		{
			fmask++;
			if (*fname != '.')
				fname++;
			continue;
		}

		if (*fmask == '*')
		{
			fmask++;
			while (*fmask && *fmask != '.')
				fmask++;
			while (*fname && *fname != '.')
				fname++;
			continue;
		}

		if (*fmask != *fname)
			return 0;

		fmask++;
		fname++;
	}

	return *fmask == *fname;
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
		for (; i <= tail; i++,tree++)
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
		for (; i <= tail; i++,t++)
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

	for (; i <= end; i++)
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
* Bestimmt die Schnittmenge zwischen zwei Rechtecken
*
****************************************************************/

int rc_intersect(GRECT *p1, GRECT *p2)
{
	int tx, ty, tw, th;

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
	for (; i <= tail; i++,tree++)
	{
		if (!(SELECTED&(tree->ob_state)) || (HIDETREE&(tree->ob_flags)))
			continue;
		anz++;
		if (0 > (*tue)(wnr, i))
			break;
	}

	return anz;
}
