/*********************************************************************
*
* Dieses Modul enthÑlt allgemeine Routinen.
*
*********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <mgx_dos.h>	/* include before tos.h */
#include <tos.h>
#include <tosdefs.h>
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
