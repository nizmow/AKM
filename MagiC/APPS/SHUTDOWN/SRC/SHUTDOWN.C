/*****************************************************************
*
* Erledigt für Mag!X 3.00 das Shutdown und den Auflösungs-
* wechsel.
*
* neue Version für das AES vom 28.6.95
*
* erweitert für MagiC 4 und den Falcon am 11.11.95.
* Konzept erneuert für sauberen Shutdown 13.11.95
* Neue PRJ-Datei 25.10.1999
*
*
* Aufbau von Shutdown.inf:
*
* zeile := TERMINATE <app-name>
*    oder	 IGNORE <app-name>
*
* <app-name> in Großbuchstaben, ohne Extension und ohne
* trailing blanks
*
* Shutdown-Reihenfolge:
*
*	1.	regulärer Shutdown über AES-Funktion. Entfernt
*		alle Programme, die AP_TERM verstehen
*	2.	behandelt alle Programme, die AP_TERM nicht
*		verstehen, bzw. gibt ihre Namen aus
*	3.	führt alle Programme im Ordner \gemsys\magic\stop
*		aus
*	4.	Lenkt die BIOS-Handles -1,-2,-3 auf NULL um (-4)
*		und löscht sämtliche Devices
*	5.	Option für Warmstart/Kaltstart bzw. Sprung in
*		den Macintosh Finder.
*
*****************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gem.h>
#include <mint/osbind.h>
#include <ctype.h>
#include "IMPORT/COUNTRY.H"
#include <mint/mintbind.h>
#include <sys/stat.h>

#define E_OK 0
#define RMODE_RD 0
#define EXE_LDEX 0
#define HDL_CON (-1)
#define HDL_AUX (-2)
#define HDL_PRN (-3)
#define HDL_NUL (-4)

typedef struct {
	unsigned short mode;
	long index;
	unsigned short dev, reserved1, nlink, uid, gid;
	long size, blksize, nblocks;
	short mtime, mdate, atime, adate, ctime, cdate, attr, reserved2;
	long reserved3[2];
} XATTR;

typedef struct {
	short x, y, bstate, kstate;
} EVNTDATA;

static long magicmac_xbios(short mode)
{
	register long result __asm__("d0");
	long magic = 0x416e4b72L;

	__asm__ volatile (
		"movw %2,%%sp@-\n\t"
		"movl %1,%%sp@-\n\t"
		"movw #39,%%sp@-\n\t"
		"trap #14\n\t"
		"lea %%sp@(8),%%sp"
		: "=r"(result)
		: "r"(magic), "r"(mode)
		: "d1", "d2", "a0", "a1", "a2", "cc", "memory");
	return result;
}


#define DEBUG 0

#define EOS '\0'
#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif
#ifndef NULL
#define NULL ((char *) 0)
#endif


/* definiere Default-Timeout-Zeit in Millisekunden */
#define	TIMEOUT	10000

/* appl_search liefert Typ: */
#define	AP_APPLICATION 2
#define	AP_ACCESSORY 4

/* maximale Anzahl Applikationen */
#define NAPPL 32

/* maximale Anzahl angemeldeter unkritischer Applikationen */
#define UNAPPL 32

#define LBUFLEN 80


typedef struct {			/* Ausnahme-APPs */
	char name[10];
	int	typ;				/* [T]erminate oder [I]gnore */
} APPLSPEC;

typedef struct {			/* laufende APPs */
	char name[16];
	int	typ;
	int	id;
} APPLINF;


short ap_id;
int	nappl;
int	unappl;
unsigned long timeout = TIMEOUT;
short language = COUNTRY_UK;

APPLINF applx[NAPPL];
APPLSPEC uncrit[UNAPPL];



/* Dxreaddir is provided by MiNTLib's <mint/mintbind.h>. */


/****************************************************************
*
* Führt alle Programme in <path> aus.
* Hier: Ordner \gemsys\magic\stop
*
****************************************************************/

static void exec_pgm_path(char *path)
{
	long dir;
	long ret,err_xr;
	XATTR xa;
	char fname[80];
	char p[256];
	char *s;


	dir = Dopendir(path, 0);		/* Modus: lange Namen */
	if (dir < E_OK)
		return;				/* Fehler */
	do
	{

		/* Name einlesen */
		/* ------------- */

		ret = Dxreaddir(70, dir, fname, &xa, &err_xr);

		if (ret || err_xr)
			continue;
		if ((xa.mode & S_IFMT) == S_IFLNK)
		{
			strcpy(p, path);
			strcat(p, fname+4);
			err_xr = Fxattr(0, p, &xa);	/* folge SymLink */
			if (err_xr)
				continue;
		}

		if ((xa.mode & S_IFMT) != S_IFREG)
			continue;
		s = strrchr(fname+4, '.');
		if (!s)
			continue;

		if (!stricmp(s, ".PRG"))
		{
			strcpy(p, path);
			strcat(p, fname+4);
			Pexec(EXE_LDEX, p, "\0", NULL);
		}
	}
	while (!ret && !err_xr);

	Dclosedir(dir);
}


/****************************************************************
*
* Lenkt alle BIOS-Device-Handles auf u:\dev\null um (Handle -4)
* und löscht alle Devices
*
****************************************************************/

static void shutdown_devices(char *path)
{
	long dir;
	long ret,err_xr;
	XATTR xa;
	char fname[80];
	char p[256];


	Fforce(HDL_CON, HDL_NUL);
	Fforce(HDL_AUX, HDL_NUL);
	Fforce(HDL_PRN, HDL_NUL);
	dir = Dopendir(path, 0);		/* Modus: lange Namen */
	if (dir < E_OK)
		return;				/* Fehler */
	do
	{

		/* Name einlesen */
		/* ------------- */

		ret = Dxreaddir(70, dir, fname, &xa, &err_xr);

		if (ret || err_xr)
			continue;
		strcpy(p, path);
		strcat(p, fname+4);
		if ((xa.mode & S_IFMT) == S_IFLNK)
		{
			err_xr = Fxattr(0, p, &xa);	/* folge SymLink */
			if (err_xr)
				continue;
		}
		if ((xa.mode & S_IFMT) != S_IFCHR)	/* Devices! */
			continue;

		Fdelete(p);
/*
		Cconws("FDELETE ");
		Cconws(p);
		printf(" => %ld\n", );
*/
	}
	while (!ret && !err_xr);
	Dclosedir(dir);
}


/****************************************************************
*
* Entfernt rechtsbündige Leerstellen aus <s>
*
****************************************************************/

static void trim(char *s)
{
	register char *t;

	t = s + strlen(s) - 1;
	while ((t >= s) && isspace(*t))
	{
		*t = EOS;
		t--;
	}
}


/****************************************************************
*
* Liest eine Zeile
*
****************************************************************/

static int readline(int hdl, char *line)
{
	register char *s;
	long ret;

	again:
	s = line;
	*s = '\0';
	do{
		ret = Fread(hdl, 1L, s);
		if (ret < E_OK)
		{
			/* error("Lesefehler"); */
			return 0;
		}
		if (ret != 1L)
		{
			/* error("Dateiende"); */
			*s = EOS;
			trim(line);
			return (s == line) ? 0 : 1;
		}
		if (*s == '\r')
			continue;		/* CR überlesen */
		if (*s != '\n')
			s++;
		if ((s - line) > (LBUFLEN - 2))
		{
			/* error("Zeilenüberlauf"); */
			return 0;
		}
	}
	while (*s != '\n');

	*s = EOS;
	trim(line);
	if (line[0] == ';')
		goto again;		/* Kommentar */

	return 1;
}


/****************************************************************
*
* Liest die uncrit[] - Tabelle aus SHUTDOWN.INF
*
****************************************************************/

void load_defaults(void)
{
	char path[128] = "shutdown.inf";
	int hdl;
	register char *s;
	unsigned long t;
	char *endptr;


#if DEBUG
		Cconws("Suche INF-Datei\r\n");
#endif

	if (!shel_find(path))
		return;

	hdl = (int) Fopen(path, RMODE_RD);
	if (hdl <= 0)
		return;

	unappl = 0;
	while (readline(hdl, path) && unappl < UNAPPL)
	{
		s = path;
		while (*s == ' ')	/* führende ' ' entfernen */
			s++;
		if (!*s)
			continue;		/* Leerzeile entfernen */
		if (*s == '#')
			continue;		/* Kommentarzeilen */

		if (!strnicmp(s, "TIMEOUT ", 8))
		{
			uncrit[unappl].typ = 'T';
			s += 8;
			while (*s == ' ')	/* führende ' ' entfernen */
				s++;
			if (!*s)
				continue;		/* Leerzeile entfernen */
			t = strtoul(s, &endptr, 10);
			if (s == endptr)
				goto err;
			timeout = t;
		}
		else
		if (!strnicmp(s, "TERMINATE ", 10))
		{
			uncrit[unappl].typ = 'T';
			s += 10;
		}
		else
		if (!strnicmp(s, "IGNORE ", 7))
		{
			uncrit[unappl].typ = 'I';
			s += 7;
		}
		else
		{
			char s[60];
			char t[256];

			err:
			strncpy(s, path, 59);
			s[59] = EOS;
			
			strcpy(t, "[3][SHUTDOWN:|");
			if (language == COUNTRY_DE)
			{
				strcat(t, "Fehler in SHUTDOWN.INF. Zeile| |");
				strcat(t, s);
				strcat(t, "][Weiter]");
			}
			else
			if (language == COUNTRY_FR)
			{
				strcat(t, "Erreur dans SHUTDOWN.INF. Ligne| |");
				strcat(t, s);
				strcat(t, "][Suite]");
			}
			else
			{
				strcat(t, "Error in SHUTDOWN.INF. Line| |");
				strcat(t, s);
				strcat(t, "][Continue]");
			}
			
			form_alert(1, t);
			continue;
		}
		
		while (*s == ' ')	/* führende ' ' entfernen */
			s++;
		if (!*s)
			continue;		/* Leerzeile entfernen */

		strncpy(uncrit[unappl].name, s, 9);
		unappl++;
	}

	Fclose(hdl);
#if DEBUG
	Cconws("INF-Datei verarbeitet\r\n");
#endif
}


/****************************************************************
*
* Ermittelt alle laufenden APPs, die nicht in der
* uncrit[] - Tabelle als "Ignore" gekennzeichnet sind.
*
****************************************************************/

void get_all_apps(void)
{
	int i, j;
	char apname[16];
	short aptyp, apid;


	/* sfirst */
	nappl = 0;
	i = appl_search(0, apname, &aptyp, &apid);

	while (i && nappl < NAPPL)
	{
		trim(apname);
		if ((apid != ap_id) && (aptyp == AP_APPLICATION))
		{
			for	(j = 0; j < unappl; j++)
			{
				if (uncrit[j].typ != 'I')
					continue;
				if (!stricmp(uncrit[j].name, apname))
					goto weiter;
			}

			strcpy(applx[nappl].name, apname);
			applx[nappl].typ = aptyp;
			applx[nappl].id = apid;
			nappl++;
			weiter:
			;
		}

		/* snext */
		i = appl_search(1, apname, &aptyp, &apid);
	}
}


/******************************************************************
*
* Entfernt eine Applikation
*
* Das Problem ist, daß das Terminieren über Fdelete() der
* Prozeßdatei zunächst nur eine Nachricht an den SCRENMGR
* verschickt, d.h. das eigentliche Löschen der Applikation
* kann noch etwas dauern. Daher wird gewartet, bis die
* Applikation über appl_find() nicht mehr sichtbar ist.
*
******************************************************************/

void remove_app(short apid, char *name)
{
	int pid;
	char path[256];
	long val;


	val = 0xfffe0000L | apid;
	pid = appl_find((char *) val);	/* apid -> pid */
	if (pid >= 0)
	{
		sprintf(path, "u:\\proc\\*.%03d", pid);
		Fdelete(path);

		/* maximal 100 Mal je 10ms warten, d.h. */
		/* Timeout von 1s	*/

		for	(val = 0;
			(val < 100) && (appl_find(name) >= 0);
			val++)
			evnt_timer(10);	/* 10ms */
	}
}


/******************************************************************
*
* type = 'T':
*
* Entfernt gleich zu Anfang alle "unwilligen" Applikationen, d.h.
* diejenigen, die angeblich AP_TERM explizit verstehen, aber
* die Nachricht ignorieren (XCONTROL).
*
* type = 'I':
*
* Entfernt alle unkritischen Applikationen, damit der Auflösungs-
* wechsel beim zweiten Anlauf endlich ausgeführt werden kann.
* 
******************************************************************/

void remove_apps(char type)
{
	register int i, apid;
	char name[9];


	for	(i = 0; i < unappl; i++)
	{
		if (uncrit[i].typ != type)
			continue;
		strcpy(name, uncrit[i].name);

		while (strlen(name) < 8)
			strcat(name, " ");

		apid = appl_find(name);
		if (apid >= 0)
		{
			/* Programm läuft */
			remove_app(apid, name);
		}
	}
}


/****************************************************************
*
* Terminiert alle ACCs
*
****************************************************************/

void remove_accs(void)
{
	int i;
	char apname[16];
	short aptyp, apid;


	/* sfirst */
	nappl = 0;
	i = appl_search(0, apname, &aptyp, &apid);

	while (i && nappl < NAPPL)
	{
		trim(apname);
		if ((apid != ap_id) && (aptyp == AP_ACCESSORY))
		{
			remove_app(apid, apname);
			nappl++;
		}
		/* snext */
		i = appl_search(1, apname, &aptyp, &apid);
	}
}


/******************************************************************
*
* Hauptfunktion.
*
* Erwartet ein bis vier Parameter:
*
* arg1 = [-w|-c] Warm- bzw. Kaltstart ohne Rückfrage
* arg2 = Gerätenummer (dev), Default: -1
*		  wenn -1: Shutdown, kein Auflösungswechsel
*
* arg3 = Texthöhe. Default: 0 (d.h. nicht setzen)
*
* arg4 = Falcon-Auflösungsmodus.
*
******************************************************************/

int main(int argc, char *argv[])
{
	EVNTDATA evd;
	short ev, dummy;
	short buf[8];
	char apname[16];
	char s[256];
	register int i;
	int dev,xdv,txt,isfalcon;
	int doex, isgr, isover;
	int msgtyp;
	int iteration;
	char c;
	enum {ask, warmboot = 2, coldboot = 3} bootmode = ask;


	ap_id = appl_init();

	/* get AES language, sub-function 3 */
	(void) appl_getinfo(3, &language, NULL, NULL, NULL);
	/* reduce to supported languages: EN,DE,FR */
	if ((language == COUNTRY_US) || (language > COUNTRY_UK))
	{
		language = COUNTRY_UK;
	}

	/* -w oder -c */

	if ((argc >= 2) && (argv[1][0] == '-'))
	{
		c = toupper(argv[1][1]);
		if (c == 'C')
			bootmode = coldboot;
		else
		if (c == 'W')
			bootmode = warmboot;
		argv++;
		argc--;
	}

	if (argc >= 2)
		dev = atoi(argv[1]);
	else
		dev = -1;

	if (argc >= 3)
		txt = atoi(argv[2]);
	else
		txt = 0;

	if (argc >= 4)			/* Falcon-Modus */
	{
		xdv = atoi(argv[3]);
		isfalcon = TRUE;
	}
	else
	{
		xdv = 0;
		isfalcon = FALSE;
	}

	load_defaults();
	if (dev < 0)
	{
		doex = SHW_SHUTDOWN;
		isgr = TRUE;
		isover = 0;
	}
	else
	{
		doex = SHW_RESCHNG;
		isgr = (isfalcon) ? xdv : dev;
		isover = (txt << 8) + isfalcon;
		if ((isfalcon) && (dev != 5))
			isover += (dev << 1);
	}

	msgtyp = (dev < 0) ? SHUT_COMPLETED : RESCH_COMPLETED;


	/* Der erste Aufruf kann nur gutgehen, wenn nur	*/
	/* "intelligente" Programme im System sind.		*/
	/* Daher ist i.a. ein zweiter Anlauf nötig.		*/
	/* ---------------------------------------------- */

	for	(iteration = 0; iteration < 2; iteration++)
	{

#if DEBUG
		Cconws("initiiere SHUTDOWN beim AES\r\n");
		Cconws(" doex = ");
		itoa(msgtyp, s, 10);
		Cconws(s);
		Cconws("\r\n");
		Cconws(" isgr = ");
		itoa(isgr, s, 10);
		Cconws(s);
		Cconws("\r\n");
		Cconws(" isover = ");
		itoa(isover, s, 10);
		Cconws(s);
		Cconws("\r\n");
#endif

		if (!shel_write(doex, isgr, isover, NULL, NULL))
		{
			const char *t;

			if (language == COUNTRY_DE)
			{
				t = "[3][SHUTDOWN:| |"
						"Proze\236 ist gesperrt.]"
						"[Abbruch]";
			}
			else
			if (language == COUNTRY_FR)
			{
				t = "[3][SHUTDOWN:| |"
						"Processus bloqu\202.]"
						"[Abandon]";
			}
			else
			{
				t = "[3][SHUTDOWN:| |"
						"Process ist currently locked.]"
						"[Cancel]";
			}

			form_alert(1, t);
			appl_exit();
			return 1;
		}
	
#if DEBUG
		Cconws("entferne kritische Programme\r\n");
#endif

		remove_apps('T');		/* "schlechte" APPs entfernen */

		/* Wir warten auf SHUT/RESCH_COMPLETED oder	*/
		/* auf einen Timeout.					*/
		/* ----------------------------------------- */

#if DEBUG
		Cconws("warte auf SHUT/RESCH_COMPLETED\r\n");
#endif

		do{
			ev = evnt_multi((timeout) ? MU_MESAG+MU_TIMER : MU_MESAG,
							0, 0, 0,
							0, 0, 0, 0, 0,
							0, 0, 0, 0, 0,
							buf, timeout,
							&evd.x, &evd.y, &evd.bstate, &evd.kstate,
							&dummy, &dummy);
	
			/* Timeout.							*/
			/* ----------------------------------------- */

			if (ev & MU_TIMER)
			{
				const char *t;

				if (language == COUNTRY_DE)
				{
					t = "[3][SHUTDOWN:| |"
							"Zeit\201berschreitung.]"
							"[Abbruch]";
				}
				else
				if (language == COUNTRY_FR)
				{
					t = "[3][SHUTDOWN:| |"
							 "D\202bordement temps.]"
							 "[Abandon]";
				}
				else
				{
					t = "[3][SHUTDOWN:| |"
							"Timeout.]"
							"[Cancel]";
				}

				form_alert(1, t);

				shel_write(SHW_SHUTDOWN, FALSE, 0, NULL, NULL);
				appl_exit();
				return 3;
			}
		}
		while ((!(ev & MU_MESAG)) || (buf[0] != msgtyp));

#if DEBUG
		Cconws("SHUT_COMPLETED eingetroffen => ");
		Cconws((buf[3]) ? "OK\r\n" : "Fehler\r\n");
#endif

		/* SHUT/RESCH_COMPLETED eingetroffen	*/
		/* ------------------------------------ */

		if (!buf[3] && (buf[4] >= 0))
		{
			char *template;

			/* Eine Applikation hat den Prozeß per AP_TFAIL   */
			/* verweigert.								*/
			/* Namen der App zur ap_id <buf[4]> ermitteln	*/
			/* ---------------------------------------------- */

			apname[0] = '?';
			apname[1] = '\0';
			apname[2] = (int) buf[4];
			if (!appl_find(apname))
			{
				strcpy(apname, "<Unbekannt>");
			}

			if (language == COUNTRY_DE)
			{
				template = "[3][SHUTDOWN:| |"
					"Shutdown- Proze\236 nicht m\224glich.|"
					"%s lieferte Fehlercode %d.]"
					"[Abbruch]";
			}
			else
			if (language == COUNTRY_FR)
			{
				template = "[3][SHUTDOWN:| |"
					"Processus Shutdown impossible. |"
					"%s retourne erreur %d.]"
					"[Abandon]";
			}
			else
			{
				template = "[3][SHUTDOWN:| |"
					"Shutdown process failed.|"
					"%s has sent error code %d.]"
					"[Cancel]";
			}

			sprintf(s, template, apname, buf[5]);

			form_alert(1, s);
			appl_exit();
			return 2;
		}

		/*
		Cconws("Shutdown- Prozeß ist OK.\r\n");
		Cconws("Alle Programme, die AP_TERM verstehen, sind weg.\r\n");
		Cconws("Suche nach weiteren Programmen...\r\n");
		*/
	

		/* Shutdown war so weit erfolgreich.			*/
		/* d.h. buf[3] = 0 und buf[4] = -1:			*/
		/*		"dumme" Programme noch aktiv			*/
		/*	   buf[3] = 1:							*/
		/*		"Shutdown erfolgreich beendet"		*/
		/* ---------------------------------------------- */

		if (buf[3])
		{
			nappl = 0;
		}
		else
		{
			/* Test auf "alte" Programme					*/
			/* ---------------------------------------------- */
	
			i = 0;
			do
			{
				get_all_apps();
				if (nappl)
					evnt_timer(100);	/* 0,1s warten */
				i++;
			}
			while (nappl && i < 50);		/* ges.: 5s warten */
	
			for	(ev = 0; ev < nappl; ev++)
			{
				const char *template;

				if (language == COUNTRY_DE)
				{
					template = "[3][SHUTDOWN:| |"
						"%s ist noch aktiv.|"
						"Bitte manuell beenden!]"
						"[Abbruch]";
				}
				else
				if (language == COUNTRY_FR)
				{
					template = "[3][SHUTDOWN:| |"
						"%s encore actif.|"
						"Quittez manuellement !]"
						"[Abandon]";
				}
				else
				{
					template = "[3][SHUTDOWN:| |"
						"%s is still running.|"
						"Please terminate manually!]"
						"[Cancel]";
				}

				sprintf(s, template, applx[ev].name);
				form_alert(1, s);
			}
		}

		/* Wenn noch Programme laufen, die nicht ignoriert	*/
		/* werden dürfen, wird der Shutdown-Prozeß hier		*/
		/* abgebrochen.								*/
		/* --------------------------------------------------- */

		if (nappl)
		{
#if DEBUG
			Cconws("Es laufen noch Programme.\r\n");
#endif

			/* unnötig, buf[3] ist sowieso 0, d.h. kein Shutdown:
			shel_write(SHW_SHUTDOWN, FALSE, 0, NULL, NULL);
			*/
			break;
		}

		/* Wenn nur noch Programme laufen, die ignoriert       */
		/* werden dürfen, hat das AES aber den Shutdown-Prozeß */
		/* schon abgebrochen. D.h. wir müssen diese Programme  */
		/* entfernen und starten einen zweiten Versuch.        */
		/* --------------------------------------------------- */

		if (!buf[3])
		{
#if DEBUG
			Cconws("Es laufen nur unwichtige Programme.\r\n");
#endif
			remove_apps('I');
			continue;
		}

#if DEBUG
		Cconws("SHUTDOWN wird ausgef\201hrt.\r\n");
#endif

		/* buf[3] war 1, d.h. das System ist im Shutdown-		*/
		/* Modus, wir können zuschlagen					*/
		/* --------------------------------------------------- */

		remove_accs();

/* beim Auflösungswechsel nicht ausführen
		exec_pgm_path("\\GEMSYS\\MAGIC\\STOP\\");
		shutdown_devices("U:\\DEV\\");
*/

		/* Shutdown */
		/* -------- */

		if (dev < 0)
		{
#if DEBUG
			Bconin(2);
#endif
			exec_pgm_path("\\GEMSYS\\MAGIC\\STOP\\");
#if DEBUG
			Bconin(2);
#endif
			shutdown_devices("U:\\DEV\\");
#if DEBUG
			Bconin(2);
#endif
			magicmac_xbios(0);	/* Beendet MagicMac */

			if (bootmode == ask)
			{
				const char *t;

				if (language == COUNTRY_DE)
				{
					t = "[3][SHUTDOWN:| |"
							"Shutdown war erfolgreich.|"
							"Rechner jetzt abschalten!]"
							"[Neustart|Kaltstart]";
				}
				else
				if (language == COUNTRY_FR)
				{
					t = "[3][SHUTDOWN:| |"
							"Shutdown r\202ussi.|"
							"Eteignez l'ordinateur!]"
							"[Red\202marrer|Reset \205 froid]";
				}
				else
				{
					t = "[3][SHUTDOWN:| |"
							"Shutdown was successful.|"
							"Shut off computer now!]"
							"[Restart|Cold Boot]";
				}

				dev = form_alert(1, t);

/*				shel_write(SHW_SHUTDOWN, FALSE, 0, NULL, NULL);	*/

				if (dev == 1)
					bootmode = warmboot;
				else
					bootmode = coldboot;
			}

			magicmac_xbios(bootmode);	/* warm/cold boot */
		}

		/* Auflösungswechsel: ausführen */
		/* ---------------------------- */

		break;
	}

	/* Alles OK. Wir terminieren und machen Aufl.wechsel/Neustart */
	/* ---------------------------------------------------------- */

	appl_exit();
	return 0;
}
