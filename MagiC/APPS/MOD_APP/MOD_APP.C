/*******************************************************************
*
*             MOD_APP.TTP                            28.04.98
*             ===========
*                                 letzte Änderung:   2026-10-03
*
* geschrieben mit Pure C V1.1
* Projektdatei: PCX_TTP.PRJ
*
* Programm zur Modifikation von APPLICAT.INF. Das Programm sollte
* auch unter TOS laufen können.
*
* Parameter:
*
*	-Xia pgmname rscname iconnr path|- code
*
*		"Insert Application":
*		X ist das MagiC-Laufwerk
*		Anwendung anmelden, z.B.
*		pgmname = mupfel.prg rscname=MAGICICN.RSC path=c:\mupfel.prg
*		iconnr=132 code=1(GEM)
*
*	-Xid pgmname dat rscname iconnr
*
*		"Insert Data File Type":
*		X ist das MagiC-Laufwerk
*		Dateityp für Applikation anmelden, z.B.
*		pgmname = mupfel.prg dat=*.BAT rscname=MAGICICN.RSC iconnr=132
*
*	-Xis key[4] name icondatei iconnr
*
*		"Insert Special":
*		X ist das MagiC-Laufwerk
*		Icon für "special object" anmelden. z.B.
*		key=PARD name=MAGICICN.RSC iconnr=132
*
* Rückgabewert:
*
*	0				OK
*	< 0				Systemfehler
*	1				Syntaxfehler beim Aufruf
*	2				Formatfehler in APPLICAT.INF
*	3				Kennung schon vergeben
*	4				Applikation nicht eingetragen (-id)
*	5				Pfad für Applikation fehlt (-id)
*
*
* noch nicht eingebaut:
*
*	-ad	appath dat	; angemeldete Datei modifizieren
*
*	-io	opath		; Icon für Ordner/Disk anmelden
*
*
* ----------------------------------------------------------------
*
* Textdatei für APPLICAT:
* Konfigurationsdatei applicat.inf:
*
*	magic			; Zeile mit "applicat.inf" und Versionsnummer
*	[Programmname]
*	.... (Programmdaten):
*		Icon mit RSC-Dateiname, Icon-Name und Index (oder Leerzeile)
*		kompletter Pfad oder Leerzeile
*		Konfigurationsangaben im Klartext ("TTP", "Single")
*		Dateitypen, jeweils einer pro Zeile
*	[Programmname]
*	.... (Programmdaten)
*	[Dateityp]
*	.... (Dateidaten)
*		Icon mit RSC-Dateiname, Icon-Name und Index (oder Leerzeile)
*
****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <osbind.h>
#include <mint/basepage.h>

_Static_assert(sizeof(int) == 4 && sizeof(long) == 4,
			   "Build MOD_APP with the default MiNTLib integer ABI");

enum {
	MODE_SPECIAL = ('i' << 8) | 's',
	MODE_APPLICATION = ('i' << 8) | 'a',
	MODE_FILETYPE = ('i' << 8) | 'd'
};

#define DEBUG 0

#if DEBUG
char infname[] = "D:\\DEV_ROOT\\APPS\\MOD_APP\\APPLICAT.INF";
char bakname[] = "D:\\DEV_ROOT\\APPS\\MOD_APP\\APPLICAT.BAK";
char datname[] = "D:\\DEV_ROOT\\APPS\\MOD_APP\\APPLICAT.DAT";
#else
char infname[] = "C:\\GEMSYS\\GEMDESK\\APPLICAT.INF";
char bakname[] = "C:\\GEMSYS\\GEMDESK\\APPLICAT.BAK";
char datname[] = "C:\\GEMSYS\\GEMDESK\\APPLICAT.DAT";
#endif

char *buf = NULL;
char *new_line = NULL;
char *ins_pos;
char drv;

/*********************************************************************
*
* Ermittelt zu einem Programmnamen den zugehörigen
* Sektionsnamen
*
*********************************************************************/

char *name_to_section(char *s, char *name)
{
	register int i;

	*s++ = '[';
	for	(i = 0; i < 8; i++)
	{
		if (*name == '.' || *name == '\0')
			break;
		*s++ = toupper(*name++);
	}

	*s++ = ']';
	return s;
}


/*********************************************************************
*
* Erzeugt die Zeile
*	RSCNAME.RSC NNNN "-"
*
*********************************************************************/

char *cr_rscinfo(char *s, char *rscname, int iconnr)
{
	strcpy(s, rscname);
	s += strlen(s);
	*s++ = ' ';
	sprintf(s, "%d", iconnr);
	strcat(s, " \"-\"\r\n");
	s += strlen(s);
	return s;
}


/*******************************************************************
*
* APPLICAT.INF mit eingefügten Daten schreiben
*
****************************************************************/

static long ainf_save(void)
{
	long ret,len;
	int f;
	long magic;


	if (!new_line)
		return 0;			/* nix zu tun */

	Fdelete(bakname);
	ret = Frename(0, infname, bakname);
	if (ret)
		return ret;

	ret = Fcreate(infname, 0);
	if (ret < 0)
		return ret;

	f = (int) ret;

	/* Anfang schreiben */

	len = ins_pos - buf;
	if (len)
	{
		ret = Fwrite(f, len, buf);
		if ((ret > 0) && (ret < len))
			ret = -1L;
		if (ret < 0)
		{
			 err:
			Fclose(f);
			Fdelete(infname);
			Frename(0, bakname, infname);
			return ret;
		}
	}

	/* neue Zeile(n) schreiben */

	len = strlen(new_line);
	if (len)
	{
		ret = Fwrite(f, len, new_line);
		if ((ret > 0) && (ret < len))
			ret = -1L;
		if (ret < 0)
			goto err;
	}

	/* Rest schreiben */

	len = strlen(ins_pos);
	if (len)
	{
		ret = Fwrite(f, len, ins_pos);
		if ((ret > 0) && (ret < len))
			ret = -1L;
		if (ret < 0)
			goto err;
	}

	/* DAT modifizieren */

	ret = Fopen(datname, O_RDWR);
	if (ret >= 0)
	{
		f = (int) ret;
		ret = Fread(f, sizeof(magic), &magic);
		if ((ret == sizeof(magic)) && (magic == 0x416e4b72L)) /* AnKr */
		{
			ret = Fseek(0L, f, SEEK_SET);		/* zurück */
			if (!ret)
			{
				magic = 0x426e4b72L; /* BnKr: cache invalidated */
				Fwrite(f, sizeof(magic), &magic);
			}
		}
		Fclose(f);
	}
	return 0;
}


/*******************************************************************
*
* APPLICAT.INF öffnen
*
* Rückgabe: 0 (OK), <0 (Systemfehler) oder 2 (Formatfehler)
*
****************************************************************/

static long ainf_open(void)
{
	long ret,len;
	int f;


	ret = Fopen(infname, O_RDONLY);
	if (ret < 0)
		return ret;

	f = (int) ret;
	len = Fseek(0L, f, SEEK_END);		/* Dateilänge bestimmen */
	if (len > 0)
	{
		ret = Fseek(0L, f, SEEK_SET);
		if (ret)
			len = -1L;
	}
	if (!len)
	{
		Fclose(f);
		return 2;				/* Formatfehler */
	}
	if (len < 0)
	{
		Fclose(f);
		return len;
	}

	buf = (char *) Malloc(len+1);	/* GEMDOS returns the address as LONG */
	if (!buf)
	{
		Fclose(f);
		return -ENOMEM;
	}
	buf[len] = '\0';				/* Ende-Zeichen */
	ret = Fread(f, len, buf);
	Fclose(f);

	if ((ret > 0) && (ret < len))
		ret = -1L;
	if (ret < 0)
	{
		Mfree(buf);
		buf = NULL;
		return ret;
	}
	if (strncmp(buf, "[APPLICAT Header V ", 19))
		return 2;			/* Formatfehler */
	return 0;
}


/*******************************************************************
*
* neue, einzufügende Zeile zusammenstellen.
*
****************************************************************/

static long is_get_new_line(char *key, char *name, char *rscname,
					int iconnr, char **line)
{
	static char l[] = "ABCD 012345678901234567890123456789 "
				"01234567.RSC 99999 \"0123456789abcdefg\"\r\n";

	if (strlen(key) != 4)
		return 1;
	if (strlen(rscname) > 12)
		return 1;

	memcpy(l, key, 4);

	if (strlen(name) > 30)
		return 1;

	strcpy(l + 5, name);
	strcat(l, " ");
	cr_rscinfo(l + strlen(l), rscname, iconnr);
	*line = l;

	return 0;
}


/*******************************************************************
*
* Position für einzufügende Zeile ermitteln.
*
****************************************************************/

static long is_get_insert_position(char *key, char **line)
{
	static char findstr[] = "\r\nABCD ";
	char *beg,*end;
	char *s;
	static const char *spec_part = "\r\n[<Special>]\r\n";
	static const char *spec_part_compat = "\r\n[<Spezial>]\r\n";


	*line = NULL;

	/* Beginn des Abschnitts suchen */
	beg = strstr(buf, spec_part);
	if (!beg)
		beg = strstr(buf, spec_part_compat);

	if (!beg)
	{
#if DEBUG
		printf("Abschnitt [<Spezial>] nicht gefunden\n");
#endif
		return 2;				/* Formatfehler */
	}
	beg += strlen(spec_part) - 2;		/* auf vorh. Zeilenende */

	/* Ende des Abschnitts suchen */

	end = strstr(beg, "\r\n[");
	if (!end)
		return 2;				/* Formatfehler */
	end += 2;
	*end = '\0';					/* EOS statt [ */

	/* nachsehen, ob der Schlüssel schon vorkommt */

	memcpy(findstr + 2, key, 4);
	s = strstr(beg, findstr);
	*end = '[';					/* wieder [ */
	if (s)
		return 3;				/* Kennung schon da */

	/* Einfügepunkt ist vor nächstem Abschnitt */

	*line = end;

	return 0;
}


/*******************************************************************
*
* Schalter 'is': Insert Special Object
*
*******************************************************************/

long do_mode_is(char *key, char *name, char *rscname, int iconnr)
{
	long ret;

	/* neue Daten bestimmen */

	ret = is_get_new_line(key, name, rscname, iconnr, &new_line);
	if (ret)
	{
#if DEBUG
		printf("Fehler %ld beim Bestimmen der Daten\n", ret);
#endif
		return (int) ret;
	}

	/* Einfügeposition festlegen */

	ret = is_get_insert_position(key, &ins_pos);
	if (ret)
	{
#if DEBUG
		printf("Fehler %ld beim Bestimmen der Einf\201geposition\n", ret);
#endif
		return (int) ret;
	}

	return 0;
}


/*******************************************************************
*
* Schalter 'ia': Insert Application
*
*******************************************************************/

long do_mode_ia(char *pgmname, char *rscname, int iconnr, char *path, int code)
{
	static char ins_data[] =
			"[12345678]\r\n"
			"12345678.rsc 99999 \"-\"\r\n"
			"1234567890123456789012345678901234567890"
			"1234567890123456789012345678901234567890"
			"1234567890123456789012345678901234567890"
			"12345678\r\n"
			"99999\r\n";
	char *s,*t;
	int cmp;
	long namelen;


	if (strlen(rscname) > 12)
		return 1;
	if (strlen(path) > 128)
		return 1;
	s = name_to_section(ins_data, pgmname);
	namelen = s - ins_data;
	*s++ = '\r';
	*s++ = '\n';

	t = buf;
	for (;;)
	{
		t = strstr(t, "\r\n[");	/* nächste Applikation suchen */

		if (!t)
			return 2;		/* Formatfehler */

		if (t[3] == '<')		/* erste Spezial-Section */
			cmp = 1;
		else
		{
			cmp = strncmp(t + 2, ins_data, namelen);
			if (!cmp)
				return 3;	/* Kennung schon vergeben */
		}

		if (cmp > 0)
		{
			break;
		}

		t += 3;
	}
	
	ins_pos = t + 2;		/* hier einfügen! */
	s = cr_rscinfo(s, rscname, iconnr);
	if ((*path != '-') || (path[1]))
	{
		t = strchr(path, ' ');		/* Leerzeichen ? */
		if (t)
			*s++ = '\'';			/* in Hochkomma einschließen */

		strcpy(s, path);
		if (t)
			strcat(s, "'");		/* in Hochkomma einschließen */
	}

	strcat(s, "\r\n");
	s += strlen(s);
	sprintf(s, "%d", code);
	strcat(s, "\r\n");
	new_line = ins_data;

	return 0;
}


/*******************************************************************
*
* Schalter 'id': Insert Data File Type
*
*******************************************************************/

long do_mode_id(char *pgmname, char *ftyp, char *rscname, int iconnr)
{
	static char section[] = "[12345678]";
	static const char *app_free = "[<free>]";
	static const char *app_free_compat = "[<frei>]";
	static char ins_data[] =
			"12345678901234567890123456789012 12345678.rsc 99999 \"-\"\r\n";
	char *s,*t,*u;
	int cmp_nam,cmp_typ;
	long namelen,typlen;
	int is_free_app;


	if (strlen(rscname) > 12)
		return 1;
	if (strlen(ftyp) > 32)
		return 1;

	s = name_to_section(section, pgmname);
	*s = '\0';
	namelen = s - section;

	strcpy(ins_data, ftyp);
	strupr(ins_data);
	strcat(ins_data, " ");
	typlen = strlen(ins_data);
	s = ins_data + typlen;

	t = buf;
	for (;;)
	{
		t = strstr(t, "\r\n[");	/* nächste Applikation suchen */
		if (!t)
			return 2;		/* Formatfehler */
		t += 2;				/* überpringe \r\n */

		is_free_app = !strncmp(t, app_free, 8) || !strncmp(t, app_free_compat, 8);

		if ((t[1] == '<') && !is_free_app)
			break;			/* <frei> ist letzte APP */

#if DEBUG
	Cconws("Bearbeite APP ");
	Fwrite(1, strchr(t, '\r')-t, t);
	Cconws("\r\n");
#endif
		cmp_nam = strncmp(t, section, namelen);
		u = strstr(t, "\r\n");
		if ((!u) || (u[2] == '['))
			return 2;		/* Formatfehler */
		u += 2;				/* u: Icondaten für Programm */
		u = strstr(u, "\r\n");
		if ((!u) || (u[2] == '['))
			return 2;		/* Formatfehler */
		u += 2;				/* u: Pfad für Programm */
		u = strstr(u, "\r\n");
		if ((!u) || (u[2] == '['))
			return 2;		/* Formatfehler */
		u += 2;				/* u: Code für Programm */
		u = strstr(u, "\r\n");
		if (!u)
			return 2;		/* Formatfehler */
		u += 2;				/* u: Zeile mit Datentypen */

		while (*u != '[')
		{
#if DEBUG
	Cconws(" Bearbeite Typ ");
	Fwrite(1, strchr(u, '\r')-u, u);
	Cconws("\r\n");
#endif
			cmp_typ = strncmp(u, ins_data, typlen);
			if (!cmp_typ)
				return 3;	/* Kennung schon vergeben */

			if (cmp_typ > 0)
			{
				if (!cmp_nam)	/* unsere APP */
					ins_pos = u;	/* hier gehören wir hin */
				break;		/* wir liegen hier nicht */
			}

			u = strstr(u, "\r\n");
			if (!u)
				return 2;	/* Formatfehler */
			u += 2;
		}

		if ((!cmp_nam) && (!ins_pos))
			ins_pos = u;			/* anhängen! */
		t = u - 2;				/* nächste Section suchen */
	/*	Fwrite(1, 8L, t+2);	*/
	}

	if (!ins_pos)
		return 4;			/* Applikation fehlt */

	s = cr_rscinfo(s, rscname, iconnr);
	new_line = ins_data;

	return 0;
}


/*
 * PCMXSTRT accepts single/double-quoted arguments anywhere in a TOS tail.
 * MiNTLib only enables its quote parser when the entire tail starts with '.
 * Keep this application-local until another port needs the same convention.
 */
static int tos_arguments(char *command, char **args)
{
	char *read = command, *write = command;
	int count = 1;

	while (*read)
	{
		char quote;
		while (*read && (unsigned char) *read <= ' ')
			read++;
		if (!*read)
			break;
		args[count++] = write;
		quote = (*read == '\'' || *read == '"') ? *read++ : '\0';
		while (*read)
		{
			if (quote)
			{
				if (*read == quote)
				{
					read++;
					if (quote == '\'' && *read == '\'')
					{
						*write++ = *read++;
						continue;
					}
					break;
				}
			}
			else if ((unsigned char) *read <= ' ')
				break;
			*write++ = *read++;
		}
		while (*read && (unsigned char) *read <= ' ')
			read++;
		*write++ = '\0';
	}
	args[count] = NULL;
	return count;
}

/*******************************************************************
*
* Hauptprogramm
*
*******************************************************************/

int main(int argc, char *argv[])
{
	char *s;
	long ret;
	int mode;
	char command[128];
	char *args[65];		/* at most 63 one-byte arguments in a 126-byte tail */
	unsigned int length = (unsigned char) _base->p_cmdlin[0];

	/* MiNTLib supplies a nonempty argv[0] for ARGV; retain that parsed vector. */
	if (length < 127 && argv[0][0] == '\0')
	{
		memcpy(command, _base->p_cmdlin + 1, length);
		command[length] = '\0';
		args[0] = argv[0];
		argc = tos_arguments(command, args);
		argv = args;
	}


	/* Modus ermitteln: mode */
	/* --------------------- */

	if (argc < 2)
	{
		global_syntax:
		Cconws("Syntax: mod_app -[Xis|Xia|Xid] ...\r\n");
		return 1;
	}

	s = argv[1];
	if (*s++ != '-')
		goto global_syntax;
	if (!(*s))
		goto global_syntax;

	drv = toupper(*s++);		/* Laufwerk (A,B,C,...) */
	if (!(*s))
		goto global_syntax;
	mode = *s << 8;
	s++;
	mode |= *s;
	s++;
	if (*s)
		goto global_syntax;

	if ((mode != MODE_SPECIAL) && (mode != MODE_APPLICATION) && (mode != MODE_FILETYPE))
		goto global_syntax;

	/* Pfade anpassen */
	/* -------------- */

	infname[0] = bakname[0] = datname[0] = drv;

	/* APPLICAT.INF öffnen und einlesen */
	/* -------------------------------- */

	ret = ainf_open();
	if (ret)
	{
#if DEBUG
		printf("Fehler %ld beim \231ffnen\n", ret);
#endif
		return (int) ret;
	}

	/* Aktionen je nach Modus */
	/* ---------------------- */

	switch (mode)
	{
		case MODE_SPECIAL:
			if (argc != 6)
			{
				Cconws("Syntax: mod_app -Xis key name rscname iconnr\r\n");
				return 1;
			}
			ret = do_mode_is(argv[2],argv[3],argv[4],atoi(argv[5]));
			break;

		case MODE_APPLICATION:
			if (argc != 7)
			{
				Cconws("Syntax: mod_app -Xia pgmname rscname iconnr path|- code\r\n");
				return 1;
			}
			ret = do_mode_ia(argv[2],argv[3],atoi(argv[4]), argv[5],atoi(argv[6]));
			break;

		case MODE_FILETYPE:
			if (argc != 6)
			{
				Cconws("Syntax: mod_app -Xid pgmname dtype rscname iconnr\r\n");
				return 1;
			}
			ret = do_mode_id(argv[2],argv[3], argv[4],atoi(argv[5]));
			break;
	}

	/* Fehlerbehandlung */
	/* ---------------- */

#if DEBUG
	if (ret)
		printf("Fehler %ld beim Bearbeiten\n", ret);
#endif
	if (ret)
		return (int) ret;

	/* Datei rausschreiben */

	ret = ainf_save();

#if DEBUG
	if (ret)
		printf("Fehler %ld beim Schreiben\n", ret);
#endif
	return (int) ret;
}
