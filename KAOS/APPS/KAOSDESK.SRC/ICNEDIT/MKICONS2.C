/*******************************************************************
*
*		    MKICONS.PRG						6.8.89
*		    ===========
*						    letzte énderung:	10.04.94
*
* Textdatei "NAMES.TXT" compilieren und Icondatei an
* "KAOSDESK.PRG" anbinden. Die Datei wird hinter den Programm-
* vorspann geschrieben, und zwar mit dem Header:
*	0x4e71		nop
*	0x41f9		lea	icn_len,a0
*	len	(long)
*	0x4efb		jmp	0(pc,a0.l)
*	0x8800
*	'suec'
*
* alternativ Datei MAGXICNS.RSC/KDSKICNS.RSC verwenden oder .ICP bzw.
* .ICD aus dem aktuellen Verzeichnis
*
* geschrieben mit PureC
*
*******************************************************************/

#include <tos.h>
#include <aes.h>
#include <magix.h>
#include <tosdefs.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#define MAXPGMIC	130
#define MAXDATIC	106
#define MAXPGMS	1000
#define MAXFILS	1000
#define MAXDSTS	5

#define TRUE	1
#define FALSE	0
#define EOS	'\0'
#define P_COOKIES	((long *)			0x5a0)

#define USA	0
#define FRG	1

int	ismagix;
char *icnrsc_fname;
int	file;
OBJECT *adr_box;
void seek(long pos);
void read (long count, void *buf);
void write(long count, void *buf);
void fehler( char *s );
void ende(int retcode);


/* Es wird eine Textdatei "NAMES.TXT" folgenden Aufbaus "compiliert": */
/* FROM <Pfad fÅr Icondateien>								*/
/* TO   <rsc_datei1>										*/
/* TO   <rsc_datei2>										*/
/* ...													*/
/* <vollst. Pfad der 1. Icondatei>:							*/
/* <beliebig viele Dateitypen (.XXX) oder Programme (XXXXXXXX)>		*/
/* <vollst. Pfad der 2. Icondatei>:							*/
/* ... (usw.) ...											*/

/* Leerzeilen sind erlaubt.									*/
/* vor jeder Zeile dÅrfen Tabs und Leerstellen stehen.				*/
/* zwischen Dateiname und ':' dÅrfen Tabs und Leerstellen stehen.	*/
/* In jeder Zeile darf ein durch ';' abgetrennter Kommentar stehen.	*/
/* hinter der Zeile dÅrfen Tabs und Leerstellen stehen.			*/
/* Etwa:													*/

/* FILENAME.ICP  :		; Icon fÅr Textverarbeitung				*/
/*		WORDPLUS		; das neue Wordplus von GST				*/
/*		TEMPUS		; der berÅhmte Editor					*/
/*		SIGNUM2		; Signum V2.00 von Application Systems		*/
/*														*/
/* TEXTVER.ICD:		; Icon fÅr Textdateien					*/
/*   .													*/
/*	.													*/
/*	.													*/

/* Erstellt die RSC-/Icondatei fÅr KAOSDESK. Aufbau:	*/

/*  char [...]					Resourcedatei selbst		*/
/*  int						'IC' (magic)				*/
/*  unsigned int				Anzahl Programm- Icons		*/
/*  unsigned int				Anzahl Datei- Icons			*/
/*  unsigned int				Anzahl Programmnamen		*/
/*  unsigned int				Anzahl Dateitypen			*/
/*  unsigned char [..][0x70]		Icondaten fÅr Programm- Icons	*/
/*  unsigned char [..][0x70]		Icondaten fÅr Datei- Icons	*/
/*  unsigned char [..][9]		Programmnamen.				*/
/*  unsigned char [..][4]		Dateitypen.				*/

/*   Bei den Namen gibt jeweils das letzte Byte die	*/
/*    Icon- Nummer an. Programmnamen ohne Extensions!	*/
/*	Die Icon- Nummern beziehen sich jeweils auf die	*/
/*    Programm bzw. auf die Dateiicons				*/

struct iconfile_header {
 unsigned		n_pgm_icons;
 unsigned		n_dat_icons;
 unsigned		n_pgm_names;
 unsigned		n_dat_names;
} header;


char	cline[256];
char sourcepath[256];
char	dstprgs[MAXDSTS][256];		/* Ziel- Programmdateien  */
char picons[MAXPGMIC][0x70];		/* Programmicon- Daten */
char dicons[MAXDATIC][0x70];		/* Dateiicon-	  Daten */
unsigned char pgms[MAXPGMS][9];
unsigned char fils[MAXFILS][4];


/****************************************************************
*
* Setzt Quell- bzw. Zielpfad fÅr alle Operationen
*
****************************************************************/

void set_path(char *dst, char *src)
{
	int l;

	_appl_yield();
	while(isspace(*src))
		src++;
	_appl_yield();
	strcpy(dst, src);
	l = (int) strlen(dst);
	if	(l > 0)
		{
		if	(dst[l-1] != '\\' && dst[l-1] != ':')
			{
			dst[l] = '\\';
			dst[l+1] = EOS;
			}
		}
}


/****************************************************************
*
* rechtsbÅndige Leerstellen in einer Zeichenkette lîschen
*
****************************************************************/

void clr_rightsp(char *s)
{
	int l;

	_appl_yield();
	while((l = (int) strlen(s) - 1) >= 0 && isspace(s[l]))
		s[l] = EOS;
	_appl_yield();
}


/****************************************************************
*
* Icondatei <fname> îffnen, nach <adr> einlesen und schlieûen.
* Bei Fehler Programm abbrechen.
* Bei vorhandener Ressourcedatei werden die Icondaten aus dieser
* geholt.
*
****************************************************************/

void lies_icondatei(char *fname, char *adr)
{
	int hdl;
	char	path[256];
	register OBJECT *o;
	ICONBLK *icn;



	if	(adr_box)
		{
		o = adr_box;
		do	{
			if	(o->ob_type != G_ICON)
				continue;
			icn = o->ob_spec.iconblk;
			if	(!stricmp(icn->ib_ptext, fname))
				{
				memcpy(adr, icn->ib_pdata, 0x70L);
				return;
				}
			}
		while(!((o++->ob_flags) & LASTOB));
		fehler("Icon \"");
		fehler(fname);
#if		COUNTRY==FRG
		fehler("\" ist nicht in|");
		fehler(icnrsc_fname);
		fehler(" enthalten!");
#elif	COUNTRY==USA
		fehler("\" not stored in|");
		fehler(icnrsc_fname);
		fehler(" !");
#endif
		fehler(NULL);
		ende((int) EFILNF);
		}


	if	(!strchr(fname, '\\') && !strchr(fname, ':'))
		strcpy(path, sourcepath);
	else path[0] = EOS;
	strcat(path, fname);
	hdl = (int) Fopen(path, RMODE_RD);
	if	(hdl < 0)
		{
#if		COUNTRY==FRG
		fehler("Datei \"");
		fehler(fname);
		if	(hdl == EFILNF || hdl == EPTHNF || hdl == EDRIVE)
			fehler("\" wurde|nicht gefunden!");
		else fehler("\" konnte|nicht geîffnet werden!");
#elif	COUNTRY==USA
		fehler("File \"");
		fehler(fname);
		if	(hdl == EFILNF || hdl == EPTHNF || hdl == EDRIVE)
			fehler("\" not found!");
		else fehler("\" could not|be opened!");
#endif
		fehler(NULL);
		ende(hdl);
		}

	if	(0x70 != Fread(hdl, 0x70L, adr))
		{
#if		COUNTRY==FRG
		fehler("Lesefehler auf Datei \"");
#elif	COUNTRY==USA
		fehler("Read error on file \"");
#endif
		fehler(fname);
		fehler("\" !");
		fehler(NULL);
		ende(-1);
		}
	Fclose(hdl);
}


/****************************************************************
*
* Eine Zeile der Datei <hdl> nach <adr> einlesen (durch LF
* oder EOF abgeschlossen).
* RÅckgabe <> 0, falls Dateiende erreicht.
* Die Zeile wird ohne abschlieûendes CR,LF eingelesen und durch
* EOS abgeschlossen.
*
****************************************************************/

int getline(int hdl, char *adr)
{
	long doserr;
	char *line;
	int  eof;


	_appl_yield();
	eof  = FALSE;
	line = adr;
	do	{
		_appl_yield();
		doserr = Fread(hdl, 1L, adr);
		_appl_yield();
		if	(doserr < 0)
			ende((int) doserr);
		if	(doserr == 0L || *adr == '\x1a')
			{
			eof = TRUE;
			break;		/* EOF */
			}
		if	(*adr == '\r')
			continue;
		if	(*adr == '\n')
			break;
		adr++;
		}
	while(1);
	*adr = 0;
	strcpy(cline, line);	/* akt. Zeile merken fÅr ggf. Fehler */
	return(eof);
}


/****************************************************************
*
* Icondatei schreiben
*
****************************************************************/

void modify_prg(char *path)
{
	PH hdr;
	char *rest_buf;
	long flen,old,new,rest_len;
	long buf[3];
	long pbuf[3] = {0L,'ICNS','suec'};



	_appl_yield();
	file = (int) Fopen(path, RMODE_RW);
	_appl_yield();
	if	(file < 0)
		{
#if		COUNTRY==FRG
		fehler("Kann Datei|");
		fehler(path);
		fehler("|nicht îffnen!");
#elif	COUNTRY==USA
		fehler("Cannot open file|");
		fehler(path);
		fehler(" !");
#endif
		fehler(NULL);
		ende(file);
		}
	flen = Fseek(0L, file, SEEK_END);
	seek(0L);
	read(sizeof(PH), &hdr);
	if	(hdr.ph_branch != 0x601a)
		ende((int) EPLFMT);
	seek(hdr.ph_tlen+hdr.ph_dlen+sizeof(PH)-sizeof(pbuf));
	read(sizeof(buf), buf);
	if	(buf[1] == pbuf[1] && buf[2] == pbuf[2])
		{
		old = buf[0];		/* LÑnge der integrierten Icons abziehen */
		}
	else	{
		old = 0L;
		}
	rest_len = flen - sizeof(PH) - hdr.ph_tlen - hdr.ph_dlen;
	rest_buf = Malloc(rest_len);
	if	(!rest_buf)
		ende((int) ENSMEM);
	read(rest_len, rest_buf);		/* Dateirest einlesen */

	new = sizeof(struct iconfile_header) +
		 	(header.n_pgm_icons) * 0x70 +
			(header.n_dat_icons) * 0x70 +
			header.n_pgm_names * 9 +
			header.n_dat_names * 4 +
			sizeof(pbuf);
	if	(new & 1)
		new++;
	pbuf[0] = new;

	seek(sizeof(PH) + hdr.ph_tlen + hdr.ph_dlen - old);
	write(sizeof(struct iconfile_header), &header);
	write((header.n_pgm_icons) * 0x70, picons);
	write((header.n_dat_icons) * 0x70, dicons);
	write((header.n_pgm_names) * 9, pgms);
	write((header.n_dat_names) * 4, fils);
	if	((Fseek(0L, file, SEEK_CUR) & 1L))
		write(1L, "");
	write(sizeof(pbuf), pbuf);
	write(rest_len, rest_buf);		/* Dateirest schreiben */
	Fshrink(file);

	hdr.ph_dlen += new - old;
	seek(offsetof(PH, ph_dlen));
	write(sizeof(long), &hdr.ph_dlen);

	Fclose(file);
	_appl_yield();
}



void get_cookies( void )
{
	MAGX_COOKIE *magx_cookie;
	long oldssp;
	unsigned long	*cookie;


	ismagix = FALSE;
	oldssp    = Super(0L);
	cookie = (unsigned long *) (* P_COOKIES);
	Super((char *) oldssp);

	if	(!cookie)
		return;
	while(*cookie)
		{
		if	(*cookie == 'MagX')
			{
			cookie++;
			magx_cookie = (MAGX_COOKIE *) (*cookie);
			if	(magx_cookie->aesvars)
				ismagix = TRUE;
			break;
			}
		cookie += 2;
		}
}


/****************************************************************
*
* HAUPTPROGRAMM
*
****************************************************************/

void main()
{
	char line[128];
	char *name;
	int rn;			/* Anzahl Ressourcedateien */
	int pn,dn;		/* Anzahl Programmnamen, Dateinamen */
	int pi,di;		/* Anzahl Programmicons, Dateiicons */
	int curr_no;		/* Aktuelle Iconnummer */
	int curr_ispgm;	/* Flag fÅr "Programmicon" */
	int rel_cnt;		/* Anzahl EintrÑge fÅr aktuelle Iconnummer */
	register int eof;
	char *s;



	adr_box = NULL;
	if	(0 > appl_init())
		ende(-1);
	get_cookies();

	icnrsc_fname = (ismagix) ? "MAGXICNS.RSC" : "KDSKICNS.RSC";

	if	(rsrc_load(icnrsc_fname))
		rsrc_gaddr(0, 0,  &adr_box);

	rn = pn = dn = pi = di = rel_cnt = curr_ispgm = 0;
	curr_no = -1;
	file = (int) Fopen("NAMES.TXT", RMODE_RD);
	_appl_yield();
	if	(file == EFILNF || file == EPTHNF || file == EDRIVE)
		{
#if		COUNTRY==FRG
		fehler("Datei \"NAMES.TXT\" wurde|nicht gefunden!");
#elif	COUNTRY==USA
		fehler("File \"NAMES.TXT\" not found!");
#endif
		fehler(NULL);
		ende(file);
		}
	if	(file < 0)
		{
#if		COUNTRY==FRG
		Cconws("Datei \"NAMES.TXT\" konnte nicht|geîffnet werden!");
#elif	COUNTRY==USA
		Cconws("Cannot open file \"NAMES.TXT\" !");
#endif
		fehler(NULL);
		ende(file);
		}


	do	{
		_appl_yield();
		eof = getline(file, line);
		if	(*line == EOS)
			continue;

		/* fÅhrende Leerstellen und Tabs Åberspringen */
		/* ------------------------------------------ */

		for	(name = line; isspace(*name); name++)
			;

		/* Kommentare lîschen */
		/* ------------------ */

		if	(NULL != (s = strchr(name, ';')))
			*s = EOS;

		/* rechtsbÅndige Leerstellen lîschen */
		/* --------------------------------- */

		clr_rightsp(name);

		if	(*name == EOS)
			continue;

		/* 0. Fall: Es wurde ein Quell- bzw. Zielpfad angegeben  */
		/* ----------------------------------------------------- */

		if	(!strnicmp(name, "FROM ", 5))
			{
			set_path(sourcepath, name+5);
			continue;
			}
		if	(!strnicmp(name, "TO ", 3))
			{
			char *src;

			if	(rn >= MAXDSTS)
				{
#if		COUNTRY==FRG
				fehler("Zuviele Zieldateien (max. 5)");
#elif	COUNTRY==USA
				fehler("Too many target files (max. 5)");
#endif
				goto err;
				}
			src = name+3;
			while(isspace(*src))
				src++;
			strcpy((char *) dstprgs[rn++], src);
			continue;
			}

		/* 1. Fall: Es wurde ein neuer Icon- Dateipfad angegeben */
		/* ----------------------------------------------------- */

		if	(line[strlen(name) - 1] == ':')
			{

			/* Wenn das vorherige Icon keine EintrÑge hatte, dieses lîschen */
			/* ------------------------------------------------------------ */

			if	(rel_cnt == 0 && curr_no >= 0)
				{
				if	(curr_ispgm)
					pi--;
				else di--;
				}

			/* Dateinamen extrahieren */
			/* ---------------------- */

			line[strlen(name) - 1] = EOS;
			clr_rightsp(name);
			s = strrchr(name, '.');
			if	(s == NULL)
				{
				err_ftyp:
#if		COUNTRY==FRG
				fehler("Icondateiname ");
				fehler(name);
				fehler(" hat falschen|Dateityp (nicht .ICP oder .ICD)!");
#elif	COUNTRY==USA
				fehler("Icon file name ");
				fehler(name);
				fehler(" has invalid|Type (neither .ICP nor .ICD)!");
#endif
				err:
				fehler(NULL);
				Fclose(file);
				ende(-1);
				}
			if	(!strcmpi(s, ".ICP"))
				{
				curr_no = pi++;
				curr_ispgm = TRUE;
				if	(curr_no >= MAXPGMIC)
					{
#if		COUNTRY==FRG
					fehler("Zuviele Programmicons (max. 130)");
#elif	COUNTRY==USA
					fehler("Too many program icons (max. 130)");
#endif
					goto err;
					}
				lies_icondatei(name, picons[curr_no]);
				}
			else {
				if	(!stricmp(s, ".ICD"))
					{
					curr_no = di++;
					curr_ispgm = FALSE;
					if	(curr_no >= MAXDATIC)
						{
#if		COUNTRY==FRG
						fehler("Zuviele Dateiicons (max. 106)");
#elif	COUNTRY==USA
						fehler("Too many file icons (max. 106)");
#endif
						goto err;
						}
					lies_icondatei(name, dicons[curr_no]);
					}
				else goto err_ftyp;
				}
			rel_cnt = 0;		/* noch keine EintrÑge */
			}
		else {
			if	(curr_no < 0)	/* kein vorgestellter Dateiname */
				{
#if		COUNTRY==FRG
				fehler("kein Icondateiname vorangestellt");
#elif	COUNTRY==USA
				fehler("no preceding icon file name");
#endif
				goto err;
				}

		/* 2. Fall: Es wurde ein Programmname angegeben		  */
		/* ----------------------------------------------------- */

			if	(*name != '.')
				{
				if	(!curr_ispgm)	/* erwarte aber Dateityp */
					{
#if		COUNTRY==FRG
					fehler("Erwarte Dateityp (.XXX)");
#elif	COUNTRY==USA
					fehler("Expecting file type (.XXX)");
#endif
					goto err;
					}
				if	(strlen(name) > 8)
					{
#if		COUNTRY==FRG
					fehler("Programmname zu lang|(mehr als 8 Zeichen)!");
#elif	COUNTRY==USA
					fehler("program name too long|(> 8 characters)!");
#endif
					goto err;
					}
				strncpy((char *) (pgms[pn]), name, 8);
				pgms[pn][8] = curr_no;
				pn++;
				}

		/* 3. Fall: Es wurde ein Dateityp angegeben			  */
		/* ----------------------------------------------------- */

			else {
				name++;
				if	(curr_ispgm)	/* erwarte aber Programmnamen */
					{
#if		COUNTRY==FRG
					fehler("Erwarte Programmnamen (XXXXXXXX)!");
#elif	COUNTRY==USA
					fehler("Expecting program name (XXXXXXXX)!");
#endif
					goto err;
					}
				if	(strlen(name) > 3)
					{
#if		COUNTRY==FRG
					fehler("Dateityp zu lang (mehr als 3 Zeichen)!");
#elif	COUNTRY==USA
					fehler("File type too long (> 3 characters)!");
#endif
					goto err;
					}
				strncpy((char *) (fils[dn]), name, 3);
				fils[dn][3] = curr_no;
				dn++;
				}
			rel_cnt++;		/* EintrÑge pro Icon */
			}
		}
	while(eof == 0 && pn < MAXPGMS  && dn < MAXFILS &&
				   pi < MAXPGMIC && di < MAXDATIC);
	Fclose(file);
	_appl_yield();

	header.n_pgm_names = pn;
	header.n_dat_names = dn;
	header.n_pgm_icons = pi;
	header.n_dat_icons = di;


	/* Zieldatei îffnen und speichern */

	while(rn)
		modify_prg((char *) dstprgs[--rn]);

	ende(0);
}


void seek(long pos)
{
	long errcode;

	_appl_yield();
	errcode = Fseek(pos, file, SEEK_SET);
	_appl_yield();
	if	(errcode >= 0L && errcode != pos)
		errcode = -1L;
	if	(errcode < 0L)
		{
#if		COUNTRY==FRG
		fehler("Fehler bei Seek");
#elif	COUNTRY==USA
		fehler("seek error");
#endif
		fehler(NULL);
		ende((int) errcode);
		}
}

void read(long count, void *buf)
{
	long errcode;

	_appl_yield();
	errcode = Fread(file, count, buf);
	_appl_yield();
	if	(errcode < 0L)
		{
		err:
#if		COUNTRY==FRG
		fehler("Lesefehler");
#elif	COUNTRY==USA
		fehler("read error");
#endif
		fehler(NULL);
		ende((int) errcode);
		}
	if	(errcode != count)
		{
		errcode = -1L;
		goto err;
		}
}

void write(long count, void *buf)
{
	long errcode;

	_appl_yield();
	errcode = Fwrite(file, count, buf);
	_appl_yield();
	if	(errcode < 0L)
		{
#if		COUNTRY==FRG
		fehler("Schreibfehler");
#elif	COUNTRY==USA
		fehler("write error");
#endif
		err:
		fehler(NULL);
		ende((int) errcode);
		}
	if	(errcode != count)
		{
		errcode = -1L;
#if		COUNTRY==FRG
		fehler("Diskette/Platte voll");
#elif	COUNTRY==USA
		fehler("Disk full");
#endif
		goto err;
		}
}

void fehler( char *s )
{
	static char alrts[500] = "[1][MKICONS:| |";

	if	(s)
		strcat(alrts, s);
	else	{
		strcat(alrts, "][");
		if	(cline[0])
#if		COUNTRY==FRG
			strcat(alrts, "Zeigen|");
#elif	COUNTRY==USA
			strcat(alrts, "Show|");
#endif
		strcat(alrts, "ABBRUCH]");
		if	((1 == form_alert(1, alrts)) && cline[0])
			{
#if		COUNTRY==FRG
			strcpy(alrts+12, " Fehler in Zeile:| |");
#elif	COUNTRY==USA
			strcpy(alrts+12, " error in line:| |");
#endif
			strncat(alrts, cline, 40);
			if	(strlen(cline) > 40)
				{
				strcat(alrts, "|");
				strncat(alrts, cline+40, 40);
				}
			if	(strlen(cline) > 80)
				{
				strcat(alrts, "|");
				strncat(alrts, cline+80, 40);
				}
#if		COUNTRY==FRG
			strcat(alrts, "][ABBRUCH]");
#elif	COUNTRY==USA
			strcat(alrts, "][ Abort ]");
#endif
			form_alert(1, alrts);
			}
		}
}

void ende(int retcode)
{
	appl_exit();
	Pterm(retcode);
}
