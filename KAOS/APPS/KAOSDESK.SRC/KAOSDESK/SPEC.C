/*********************************************************************
*
* Dieses Modul enthÑlt TOS- Spezifisches
*
*********************************************************************/

#define HICOL 0
#define hicol 7

/* AES von TOS 1.2: */

#define SH_12_WPCMD_ADDR  	((char **)  0xc79e)		/* Programmname  */
#define SH_12_WPTAIL_ADDR 	((char **)  0xc82a)		/* Kommandozeile */
#define SH_12_DOEX_ADDR   	((int   *)  0x98a0)		/* doex- Flag */
#define SH_12_WISGR_ADDR		((int   *)  0x96f4)		/* wisgr- Flag */
#define SH_12_FLAG1_ADDR		((int   *)  0xc83c)		/* Flag fÅr "Daten ungÅltig" */
#define SH_12_FLAG2_ADDR		((int   *)  0x96f2)		/* dito */
#define MO_12_COUNT_ADDR		((int   *)  0xc86a)		/* ZÑhler fÅr graf_mouse() */

/* AES von TOS 1.4: */

#define SH_14_WPCMD_ADDR  	((char **)  0x6d88)		/* Programmname  */
#define SH_14_WPTAIL_ADDR 	((char **)  0x6dc2)		/* Kommandozeile */
#define SH_14_DOEX_ADDR   	((int   *)  0x7006)		/* doex- Flag */
#define SH_14_WISGR_ADDR		((int   *)  0x6f0c)		/* wisgr- Flag */
#define MO_14_COUNT_ADDR		((int   *)  0x6e06)		/* ZÑhler fÅr graf_mouse() */

#define CONTERM  	((unsigned char *)	0x484)
#define HDV_BPB	((long *)			0x472)
#define HDV_RW      ((long *)			0x476)
#define HDV_MEDIACH ((long *)			0x47e)
#define _SYSBASE	((long *)			0x4f2)
#define P_COOKIES	((long *)			0x5a0)


typedef struct {
     char dir_name[11];       /* 0x00: Dateiname                           */
     char dir_attr;           /* 0x0b: Attribut                            */
     char dir_dummy[10];
     int  dir_time;           /* 0x16: Zeit  der letzten énderung          */
     int  dir_date;           /* 0x18: Datum der letzten énderung          */
     int  dir_stcl;           /* 0x1a: erster Cluster                      */
     long dir_flen;           /* 0x1c: DateilÑnge                          */
} DIRENTRY;


typedef struct ____bcb {
  struct ____bcb *b_link;     /* 0x00: Zeiger auf nÑchsten BCB             */
     int  b_bufdrv;           /* 0x04: Laufwersnummer, -1 fÅr ungÅltig     */
     int  b_buftyp;           /* 0x06: FAT=0, DIR=1, DATA=2                */
     int  b_bufrec;           /* 0x08: Sektornummer (GEMDOS- Code)         */
     int  b_dirty;            /* 0x0a: Pufferinhalt geÑndert               */
     void *b_dmd;             /* 0x0c: Zeiger auf DMD von b_bufdrv         */
     char *b_bufr;            /* 0x10: Zeiger auf Sektorpuffer             */
} BCB;


#include <tos.h>
#include "k.h"
#include <vdi.h>
#include <tosdefs.h>
#include <string.h>

extern int qbreak( void );
extern int disk_type(int drvnr);

/* Externals aus DKLICK */

extern void init_vt52( void );
extern void exit_vt52( void );
extern void str_conout(char *s, int len);

/* Externals aus KAOSDESK */

extern long    err_alert(long e);
extern int  	vdi_handle;

/* extern aus ALLG */

extern void Mgraf_mouse(int type);

#define Dskopen(d)	gemdos(201, d)		/* Cache ein */
#define Dskclose(d)	gemdos(202, d)		/* Cache aus */


/*********************************************************************
*
* Merkt Zeiger auf Betriebssystemheader und die Betriebssystemversion.
* Muû bei Initialisierung von KAOSDESK aufgerufen werden.
* TOS 1.4- Versionen, die nicht vom 6.4.89 sind, werden als TOS 1.5
* angesehen.
* Auûerdem wird ein Zeiger auf den aktuellen Applikationsnamen
* gemerkt, da die Funktion menu_register(-1, name) unter GEM 2.x nicht
* mehr unterstÅtzt wird.
*
*********************************************************************/

#if	MAGIXDESK
char *os_ver_s = "Mag!X v__.__  vom __.__.____";
#else
static unsigned long *p_cookies;
static SYSHDR *syshdr;
static char *apname;
static int sysver;
static int is_rom_aes;
static unsigned long kaos_date;
#endif
static AESVARS *aesvars;
#if	HICOL
static int hicol;			/* hîchster Farbwert (7=ST,15=STE/TT) */
#endif

void get_syshdr( void )
{
	extern char inf_name[];
	extern int *aes_global;
#if	HICOL
	int col;
#endif

#if	MAGIXDESK
	aesvars = *((AESVARS **) (aes_global+11));
	if	(aesvars->magic2 != 'MAGX')
		Pterm(-1);
	inf_name[0] = *(aesvars->aes_bootdrv) + 'A';

	os_ver_s[ 7] = (((aesvars->version) >> 12)     ) + '0';
	os_ver_s[ 8] = (((aesvars->version) >> 8) & 0xf) +'0';
	os_ver_s[10] = (((aesvars->version) >> 4) & 0xf) +'0';
	os_ver_s[11] = (((aesvars->version)     ) & 0xf) +'0';

	if	(aesvars->release < 3)
		os_ver_s[12] = *("‡·‚" + (aesvars->release));

	os_ver_s[18] = (((aesvars->date) >> 28)      ) + '0';
	os_ver_s[19] = (((aesvars->date) >> 24) & 0xf) + '0';
	os_ver_s[21] = (((aesvars->date) >> 20) & 0xf) + '0';
	os_ver_s[22] = (((aesvars->date) >> 16) & 0xf) + '0';
	os_ver_s[24] = (((aesvars->date) >> 12) & 0xf) + '0';
	os_ver_s[25] = (((aesvars->date) >>  8) & 0xf) + '0';
	os_ver_s[26] = (((aesvars->date) >>  4) & 0xf) + '0';
	os_ver_s[27] = (((aesvars->date)      ) & 0xf) + '0';
#else
	long oldssp;
	unsigned long	kaos_vgl_date;		/* in der Form jjjjmmtt */
	unsigned long	*cookie;
	extern int ismagix;
	extern   BASPAG *_BasPag;


	/* Umschalten in den Supervisormodus */

	oldssp    = Super(0L);

	/* syshdr, sysver und aesvars holen */

	syshdr    = (SYSHDR *)  (* _SYSBASE);
	sysver    = syshdr->os_version;
	aesvars 	= (AESVARS *) syshdr->os_magic;
	ismagix	= iskgem && aesvars->magic2 == 'MAGX';

	/* p_cookies und kaos_date holen */

	p_cookies = cookie = (unsigned long *) (* P_COOKIES);
	kaos_date = 0L;
	while(p_cookies && *cookie)
		{
		if	(*cookie == 'KAOS')
			{
			cookie++;
			kaos_date = *cookie;
			break;
			}
		cookie += 2;
		}

	/* Wenn wir vom ROM-AES gestartet wurden: is_rom_aes */

	is_rom_aes = ((_BasPag->p_parent)->p_tbase == aesvars->aes_start);

	/* Das KAOS- Datum wird zum Vergleichen umsortiert */

	kaos_vgl_date =  kaos_date << 16L;				/* jjjj0000 */
	kaos_vgl_date |= kaos_date >> 24L;				/* jjjj00tt */
	kaos_vgl_date |= (kaos_date >> 8L) & 0xff00L;	/* jjjjmmtt */

	if	(!iskgem || (kaos_vgl_date < 0x19910112L))
		aesvars = NULL;
	if	(sysver == 0x0104 && syshdr->os_gendat != 0x04061989L)
		sysver = 0x0105;
	Super((char *) oldssp);

	if	(isgem2)
		{
		apname = *((char **) (aes_global+11));
		apname += 0xe94+12;
		if	(memcmp(apname, "KAOSDESK", 8L))
			apname = NULL;
		}
	else	apname = NULL;
#endif

#if	HICOL
	Vsync();
	col = Setcolor(15, 0x00f);				/* Farbe retten */
	col = Setcolor(15, col);					/* Farbe zurÅck */
	hicol = (col & 0xf);
#endif
}


/****************************************************************
*
* éndert die Auflîsung
*
****************************************************************/

int change_res(int dev, int txt)
{
	if	(!aesvars)
		return(0);
	aesvars->chgres(dev, txt);
	return(1);
}


#if	!MAGIXDESK
/****************************************************************
*
* Setzt die Farbpalette und den Blitterstatus
*
****************************************************************/

static int c_val(unsigned char c)
{
	if	(c >= '0' && c < '0'+16)
		return(c - '0');			/* nur "dezimal" */
	return(0);
}

void set_colors( void )
{
	unsigned char *buf,*bufp;
	int rgb[3];
	register int i,j;
	long oldssp;


	Blitmode(status.blitstate);
	if	(NULL == (buf = Malloc(8192L)))
		return;
	shel_get((char *) buf, 8192);
	bufp = buf;
	while(bufp < buf+8192L)
		{
		if	(*bufp == '#')
			{
			bufp++;
			if	(*bufp == 'c')
				{
				bufp++;
				for	(i = 0; i < 16; i++)
					{
					for	(j = 0; j < 3; j++)
						rgb[j] = (1000 * c_val(*bufp++)) / hicol;
					vs_color(vdi_handle, i, rgb);
					}
				evnt_dclick(c_val(*bufp++), RMODE_WR);

				oldssp = Super(0L);
				if	(*bufp++ != '0')
					*CONTERM |= 1;
				else *CONTERM &= ~1;
				if	(*bufp++ != '0')
					*CONTERM |= 4;
				else *CONTERM &= ~4;
				Super((char *) oldssp);
				i = 10 * c_val(*bufp++) + c_val(*bufp++);	/* Verzîgerung */
				j = 10 * c_val(*bufp++) + c_val(*bufp++);	/* Wiederholrate */
				Kbrate(i, j);
				}
			else if	(*bufp == 'b')
				{
				bufp++;
				j = 0;
				for	(i = 0; i < 6; i++)
					{
					j >>= 1;					/* weiterschieben  */
					if	(*bufp++ != '0')		/* Bit ggf. setzen */
						j |= 0x20;
					}
				Setprt(j);
				}
			}
		bufp = (unsigned char *) strchr((char *) bufp, '\n');
		if	(bufp == NULL)
			break;
		bufp++;
		}
	Mfree(buf);
}
#endif


/****************************************************************
*
* Schaltet den Cache an
*
****************************************************************/

long cache_on(char *path)
{
	long doserr;

	if	(status.use_cache)
		{
		Mgraf_mouse( - HOURGLASS );
		doserr = Dskopen(path[0] - 'A');
		Mgraf_mouse( 0x1000 );
		if	(doserr != EINVFN && doserr != EDRIVE)
			return(err_alert(doserr));
		}
	return(E_OK);
}


/****************************************************************
*
* Schaltet den Cache aus
*
****************************************************************/

long cache_off(char *path)
{
	long doserr;

	if	(status.use_cache)
		{
		Mgraf_mouse( - HOURGLASS );
		doserr = Dskclose(path[0] - 'A');
		Mgraf_mouse( 0x1000 );
		if	(doserr != EINVFN && doserr != EDRIVE && doserr < E_OK)
			return(err_alert(doserr));
		}
	return(E_OK);
}


/****************************************************************
*
* Meldet Laufwerk <drv> beim GEMDOS ab.
*
****************************************************************/

static long free_mediach( void )
{
	return(2L);
}

static long free_rw( void )
{
	return(E_CHNG);
}

static long free_bpb( void )
{
	return(0L);
}

void free_drive(int drv)
{
	DISKINFO dummy;
	long oldssp;
	long oldbpb,oldrw,oldmed;


	oldssp = Super(0L);
	oldbpb = *HDV_BPB;
	oldrw  = *HDV_RW;
	oldmed = *HDV_MEDIACH;
	*HDV_BPB     = (long) free_bpb;
	*HDV_RW      = (long) free_rw;
	*HDV_MEDIACH = (long) free_mediach;
	Dfree(&dummy, drv + 1);
	*HDV_BPB     = oldbpb;
	*HDV_RW      = oldrw;
	*HDV_MEDIACH = oldmed;
	Super((char *) oldssp);
}


/*********************************************************************
*
* Gibt Gesamtgrîûe des Speichers zurÅck.
*
*********************************************************************/

long memsize( void )
{
	long oldssp,phystop;

	oldssp = Super(0L);
	phystop = * ((long *) 0x42e);
	Super((char *) oldssp);
	return(phystop);
}


/*********************************************************************
*
* Eine Art "etv_critic" fÅr Xbios- Funktionen.
*
* Folgende Mîglichkeiten hat man:
*
* Abbruch:	RÅckgabe 0, doserr wird nicht verÑndert
* Wiederholen:	RÅckgabe 1, doserr wird nicht verÑndert
* Ignorieren:	RÅckgabe 0, doserr wird auf E_OK gesetzt
*
*********************************************************************/

int tryagain(long *doserr, int drv)
{
#if	MAGIXDESK==0
	extern char *err_string(int e);
#endif
	int ret;
	char *s;
	static char alerts[] =
#if		COUNTRY==FRG
		" auf X:][Abbruch|Nochmal|Weiter]";
#elif	COUNTRY==USA
		" at  X:][Cancel|Retry|OK]";
#endif
	char buf[200];


	alerts[5] = 'A' + drv;
#if MAGIXDESK==0
	s = err_string((int) *doserr);
	if	(s == NULL)
#if		COUNTRY==FRG
		s = "Diskettenfehler";
#elif	COUNTRY==USA
		s = "disk error";
#endif
#else
	s = (((DOSVARS *) Sconfig(SC_VARS, 0L))->err_to_str)((char) (*doserr));
#endif
	strcpy(buf, "[2][");
	strcat(buf, s);
	strcat(buf, alerts);
	ret = form_alert(2, buf);
	if	(ret == 1)		/* Abbruch */
		return(0);
	if	(ret == 2)		/* Wiederholen */
		return(1);
	*doserr = E_OK;		/* Ignorieren */
	return(0);
}

long M_Flopfmt( void *buf, void *filler, int devno, int spt, int trackno,
			 int sideno, int interlv, int virgin )
{
	long doserr;

	do	{
		doserr = Flopfmt(buf, filler, devno, spt, trackno, sideno,
					  interlv, 0x87654321L, virgin);
		}
	while(doserr != E_OK && tryagain(&doserr, devno));
	return(doserr);
}

long M_Flopwr(void *buf, void *filler, int devno, int sectno, int trackno,
		    int sideno, int count)
{
	long doserr;

	do	{
		doserr = Flopwr(buf, filler, devno, sectno, trackno, sideno, count);
		}
	while(doserr != E_OK && tryagain(&doserr, devno));
	return(doserr);
}

long M_Floprd (void *buf, void *filler, int devno, int sectno, int trackno,
		    int sideno, int count)
{
	long doserr;

	do	{
		doserr = Floprd(buf, filler, devno, sectno, trackno, sideno, count);
		}
	while(doserr != E_OK && tryagain(&doserr, devno));
	return(doserr);
}


/*********************************************************************
*
* Initialisiert ein Laufwerk
*
*********************************************************************/

static long init_disk(int dev)
{
	long doserr,buflen,fsizb,rsizb;
	long label(int lw, char *name);
	BPB	*bpb;
	char	*buf;


	bpb = Getbpb(dev);
	if	(!bpb)
		return(EMEDIA);
	fsizb = ((long) bpb->fsiz) * ((long) bpb->recsiz);
	rsizb = ((long) bpb->rdlen) * ((long) bpb->recsiz);
	buflen = fsizb+fsizb+rsizb;
	buf = Malloc(buflen);
	if	(!buf)
		return(ENSMEM);
	memset(buf, 0, buflen);

	buf[0] = buf[fsizb] = (dev > 1) ? 0xf8 : 0xf9;		/* Mediabyte */
	buf[1] = buf[2] = buf[fsizb+1] = buf[fsizb+2] = 0xff;
	if	(bpb->bflags & 1)							/* 16bit FAT */
		buf[3] = buf[fsizb+3] = 0xff;

	/* FATs und Root lîschen */
	/* --------------------- */

	free_drive(dev);
	doserr = Rwabs(1, buf, 2*bpb->fsiz + bpb->rdlen, 1, dev);

	Mfree(buf);
	return(doserr);
}


/*********************************************************************
*
* Formatiert eine Diskette
*
* dev	- Devicenummer (0 = A, 1 = B)
* sid	- Anzahl Seiten (1 oder 2)
* trk	- Anzahl Tracks (40..86)
* spt	- Anzahl Sektoren/Track (9,10)
* clu	- Anzahl Sektoren/Cluster (2,4,8)
* inter	- Interleave
* vtrk	- Spurversatz
* vsid	- Seitenversatz
* counter - Zeiger auf eine Funktion, die den gerade formatierten Track
*		   anzeigt
* is_logic- nur logisch formatieren, d.h. FAT und DIR lîschen
* dname	- Diskname oder Platz fÅr einen solchen
*
*********************************************************************/

long format(int dev, unsigned sid, unsigned trk, unsigned spt, unsigned clu,
		  int inter, int vtrk, int vsid,
		  void (*counter)(int n), int is_logic, char *dname)
{
	register ctrack,cside,i,j;
	int  interlv,versatz;
	unsigned char *buf;
	BPB *bpb;
	long doserr;
	int  secnos[100];		/* Sektornummern fÅr Versatz */
	extern long read_dname(int lw, char *name);


	if	((clu == 0) || (clu != 1 && clu & 1))
		return(EMEDIA);		/* ungerade Clustergrîûe */

	if	(is_logic)
		{
		if	(!dname[0])
			{
			doserr = read_dname(dev, dname);
			if	(doserr != EFILNF && doserr != E_OK)
				return(doserr);
			}
		doserr = init_disk(dev);
		goto set_dname;
		}
		
	if	(dev != 0 && dev != 1)
		return(EUNCMD);

	/* ggf. Standardwerte eintragen */
	/* ============================ */

	vtrk  %= spt;
	vsid  %= spt;
	inter %= spt;
	if	(!inter)
		inter = 1;

	/* Erst ab TOS 1.2 kann man die Sektornummern selbst angeben */
	/* --------------------------------------------------------- */

#if	MAGIXDESK
	interlv = -1;
#else
	interlv = (sysver >= 0x0102) ? -1 : inter;
#endif

	/* Speicher anfordern */
	/* ------------------ */

	buf = Malloc((long) (20480L));	/* 8k fÅr DD, 16k fÅr HD */
	if	(!buf)
		return(ENSMEM);

	/* Sektornummerliste initialisieren */
	/* -------------------------------- */

	for	(i = 0; i < spt; i++)
		secnos[i] = 0;				/* erst alle ungÅltig */

	i = 0;						/* Beginne mit Sektor 1 */
	for	(j = 1; j <= spt; j++)		/* diese Nummern werden vergeben */
		{
		while(secnos[i])			/* schon belegt */
			{
			i++;					/* daher einfach weiterschalten */
			i %= spt;
			}
		secnos[i] = j;				/* neue Sektornummer eintragen */
		i += inter;
		i %= spt;					/* nÑchster Sektor */
		}

	/* Disk formatieren */
	/* ---------------- */

	for	(ctrack = trk - 1; ctrack >= 0; ctrack--)
		{
		for	(cside = sid - 1; cside >= 0; cside--)
			{
			if	(qbreak())
				{
				doserr = EBREAK;
				goto err;
				}

			doserr = M_Flopfmt(buf, secnos, dev, spt, ctrack, cside,
						    interlv, (ctrack < 10) ? 0 : 0xe5e5);

			if	(doserr != E_OK)
				goto err;

			/* Seiten- bzw. Spurversatz */

			versatz = (cside) ? vsid : vtrk;
			for	(i = 0; i < spt; i++)
				{
				if	((secnos[i] += versatz) > spt)
					secnos[i] -= spt;
				}
			}
		counter(ctrack);
		}

	/* Bootsektor erstellen und schreiben */
	/* ---------------------------------- */

	i = 0;
	if	(spt == 9 && trk == 80 && clu == 2)
		{
		if	(sid == 2)				/* Standardtyp 3 */
			i = 3;
		if	(sid == 1)				/* Standardtyp 2 */
			i = 2;
		}
	Protobt(buf, 0x01000000L, (i) ? i : 3, FALSE);
	if	(!i)
		{
		unsigned int nsects,spf;
		unsigned char media;

		nsects = sid * trk * spt;		/* Anzahl Sektoren */
		media  = 0xf9;
		spf	  = (nsects/clu)/341 + 1;	/* Sektoren/FAT */

		buf[20] = nsects / 256;
		buf[19] = nsects % 256;
		buf[21] = media;
		buf[13] = clu;
		buf[23] = spf / 256;
		buf[22] = spf % 256;
		buf[25] = spt / 256;
		buf[24] = spt % 256;
		buf[27] = sid / 256;
		buf[26] = sid % 256;
		}
	buf[0] = 0xeb;
	buf[1] = 0x34;
	buf[2] = 0x90;
	buf[3] = 0x4e;        			/* Charlie lÑût grÅûen */
	Protobt(buf, -1L, -1, FALSE);		/* nicht ausfÅhrbar */
	doserr = M_Flopwr(buf, NULL, dev, 1, 0, 0, 1);
	if	(doserr)
		goto err;

	/* FATs initialisieren */
	/* ------------------- */

	bpb = Getbpb(dev);
	if	(bpb == NULL)
		{
		doserr = ERROR;
		goto err;
		}
	for	(i = 3; i < 512; i++)
		buf[i] = 0;
	buf[0] = 0xf9;
	buf[1] = buf[2] = 0xff;
	doserr = Rwabs(3, buf, 1, 1, dev);		/* FAT 1 */
	if	(doserr == E_OK)
		doserr = Rwabs(3, buf, 1, bpb->fsiz + 1, dev);
	err:

	/* Speicher freigeben, Diskwechsel simulieren */
	/* ------------------------------------------ */

	Mfree(buf);
	Rwabs(0, NULL, 2, 0, dev);

	/* Disknamen schreiben */
	/* ------------------- */

	set_dname:
	if	(!doserr && dname[0])
		doserr = label(dev, dname);

	return(doserr);
}


/*********************************************************************
*
* Untersucht den Bootsektor eines Mediums.
*
*********************************************************************/

long read_bootsec(int lw, long *ser, int *bps, int *spc,
						int *res,	int *nfats, int *ndirs, int *nsects,
						int *media, int *spf, int *spt, int *nsides,
						int *nhid, int *exec)
{
	long doserr;
	unsigned char *s;
	unsigned int  *sw;


	if	(NULL == (s = Malloc(512L)))
		return(ENSMEM);
	if	(lw == 0 || lw == 1)
		doserr = M_Floprd(s, NULL, lw, 1, 0, 0, 1);
	else doserr = Rwabs(0, s, 1, 0, lw);
	if	(doserr == E_OK)
		{
		*ser    = s[10] + 256 * s[ 9] + 65536L * s[8];
		*bps    = s[11] + 256 * s[12];
		*spc    = s[13];
		*res    = s[14] + 256 * s[15];
		*nfats  = s[16];
		*ndirs  = s[17] + 256 * s[18];
		*nsects = s[19] + 256 * s[20];
		*media  = s[21];
		*spf	   = s[22] + 256 * s[23];
		*spt	   = s[24] + 256 * s[25];
		*nsides = s[26] + 256 * s[27];
		*nhid   = s[28] + 256 * s[29];
		*exec   = 0;
		for	(sw = (unsigned int *) s; (unsigned char *) sw < s+512; sw++)
			*exec += *sw;
		*exec = (*exec == 0x1234);
		}
	Mfree(s);
	return(doserr);
}


/*********************************************************************
*
* Ersatz fÅr Dfree(), da dieses zu langsam ist.
*
*********************************************************************/

long MDfree( DISKINFO *ds, int driveno)
{
	char	*buf;
	BPB *bpb;
	long doserr;
	register unsigned int numcl;
	unsigned int freecl = 0;


	if	(--driveno < 0)
		return(EDRIVE);
	if	('HD' != disk_type(driveno) && 'FD' != disk_type(driveno))
		return(Dfree(ds, driveno+1));
	bpb = Getbpb(driveno);
	if	(!bpb)
		return(EMEDIA);
	buf = Malloc(((long) bpb->fsiz) * ((long) bpb->recsiz));
	if	(!buf)
		return(ENSMEM);

	/* Zuerst wird FAT 1 eingelesen */
	/* ---------------------------- */

	doserr = Rwabs(0, buf, bpb->fsiz, 1, driveno);

	/* Bei Lesefehlern aufgrund von "kleineren" Diskettenfehlern */
	/* wird dann versucht, FAT 2 zu lesen					 */
	/* --------------------------------------------------------- */

	if	(doserr == E_CRC || doserr == ESECNF)
		doserr = Rwabs(0, buf, bpb->fsiz, 1+bpb->fsiz, driveno);

	if	(doserr)
		goto err;
	numcl = bpb->numcl;

	/* 1. Fall: 16 Bit FAT */
	/* ------------------- */

	if	((bpb->bflags) & 1)
		{
		register int *s = ((int *) buf) + 2;


		while(numcl--)
			if	(!(*s++))
				freecl++;
		}

	/* 2. Fall: 12 Bit FAT */
	/* ------------------- */

	else {
		register char *s = buf+3;
		register char c;


		numcl >>= 1;
		while(numcl--)
			{
			c = s[1];
			if	(*s == 0 && (c & 0x0f) == 0)
				freecl++;
			s += 2;
			if	(*s == 0 && (c & 0xf0) == 0)
				freecl++;
			s++;
			}
		if	((bpb -> numcl) & 1)
			{
			c = s[1];
			if	(*s == 0 && (c & 0x0f) == 0)
				freecl++;
			}
		}
	
	ds->b_free   = freecl;
	ds->b_total  = bpb->numcl;
	ds->b_secsiz = bpb->recsiz;
	ds->b_clsiz  = bpb->clsiz;
	err:
	Mfree(buf);
	return(doserr);
}


/*********************************************************************
*
* Disk umbenennen.
*
*********************************************************************/

#define _bufl	 ( (BCB **) 0x4b2L)	/* sehr geschickt ! */
#define FAT	 0
#define DIR	 1
#define DATA	 2				/* BCB- Puffertypen */
#define DELETED '\xe5'			/* gelîschte Datei */

long label(int lw, char *name)
{
	static 	char pfad[] = "A:\\*.*";
               long retcode;
#if	MAGIXDESK == 0
     		DTA  dtapuffer;
               char tmpdatei[50];
	register	BCB	*buffer;
			BCB	*bufl[2];		/* Hier DOS- Sektorpufferlisten */
			long oldssp;
		DIRENTRY	*dirs;
	register	int  liste,i;
	extern void fname_int(char *s, char *d);
#endif
	extern char    *tmpnam( char *s );


	pfad[0] = lw + 'A';

	/* ZunÑchst wird der bisherige Diskettenname geholt    */
	/* Dann muû der in der DOS- Pufferliste liegen		*/
	/* --------------------------------------------------- */

#if	MAGIXDESK == 0
	if	(sysver < 0x0104)
		{
		Fsetdta(&dtapuffer);
		retcode =  Fsfirst(pfad, F_VOLUME);
		if	(retcode != EFILNF && retcode != E_OK)
			return(retcode);
		}
	else retcode = EFILNF;
     if   (retcode == EFILNF)
#endif

	/* Falls der Name nicht existiert, wird er erzeugt	*/
	/* Dies ist unter TOS 1.4 immer mîglich				*/
	/* Ist der neue Name leer, wird ein gelîschter Eintrag	*/
	/* erzeugt.									*/
	/* --------------------------------------------------- */

     	{
     	char	diskname[40];

		strcpy(diskname,     pfad);
		strcpy(diskname + 3, name);
		if	(!*name)
			{
			diskname[3] = '\xe5';
			diskname[4] = EOS;
			}
		retcode = Fcreate(diskname, F_VOLUME);
		if	(retcode >= E_OK)
			retcode = Fclose((int) retcode);
		return(retcode);
		}

#if	MAGIXDESK == 0

     /* Sonst wird der Umweg Åber die Pufferliste gegangen	 */
     /* ---------------------------------------------------- */

	oldssp = Super(NULL);
	bufl[0] = _bufl[0];
	bufl[1] = _bufl[1];
	Super((char *) oldssp);

	/* Beide Listen durchsuchen */
	/* ------------------------ */

	for	(liste = 0; liste < 2; liste++)
		{

		/* Jeden BCB durchsuchen */
		/* --------------------- */

		for	(buffer = bufl[liste]; buffer != NULL; buffer = buffer->b_link)
			{
			if	(buffer->b_bufdrv != lw || buffer->b_buftyp != DIR)
				continue;
			dirs = (DIRENTRY *) (buffer->b_bufr);

			/* Jeden Eintrag durchsuchen */
			/* ------------------------- */

			for	(i = 0; i < 16; dirs++,i++)
				{
				if	(dirs->dir_attr & F_VOLUME)
					{
					if	(*name)
						{
						int j;

						fname_int(name, dirs->dir_name);
						for	(j = (int) strlen(dirs->dir_name); j < 11; j++)
							dirs->dir_name[j] = ' ';
						}
					else dirs->dir_name[0] = DELETED;
					dirs->dir_attr = F_VOLUME;
					buffer->b_dirty = TRUE;
					tmpdatei[0] = pfad[0];
					tmpdatei[1] = ':';
			     	tmpnam(tmpdatei+2);
			     	retcode = Fcreate(tmpdatei,0);
			     	if	(retcode >= E_OK)
			     		retcode = Fclose((int) retcode);
			     	Fdelete(tmpdatei);
					return(retcode);
					}
				}
			}
		}

	/* Nichts gefunden */
	/* --------------- */

	return(EINTRN);
#endif
}


/*********************************************************************
*
* Kopiert eine Diskette
*
* slw	- Quell- Laufwerk
* dlw	- Ziel - Laufwerk
* fmt	- Flag fÅr "Formatieren"
*
*********************************************************************/

#if		COUNTRY==FRG
char chg_msg[] = "[2][Quelldisk in X: einlegen!][OK|Abbruch]";
#elif	COUNTRY==USA
char chg_msg[] = "[2][Insert source disk into X:!][OK|Cancel]";
#endif

long diskcopy(int slw, int dlw, int tmp, int sid, int trk, int spt,
		    int fmt, void (*feedb)(char *action, int n))
{
	int isquell;
	char tmpdatei[128];
	int  gelesen;
	int handle;
	unsigned long bufsiz_char;	/* Puffergrîûe in Zeichen */
	unsigned long n_bytes;		/* gefÅllte Bytes */
	unsigned int  bufsiz_trk;	/* Puffergrîûe in Spuren */
	unsigned int  c_tracks;		/* gefÅllte Spuren */
	register unsigned ltrack,ctrack,cside,i;
	int  secnos[100];			/* Sektornummern fÅr Versatz */
	int  interlv;
	register unsigned char *cbuf;
	unsigned char *buf, *fbuf;
	long doserr;



	/* Wenn die Laufwerke nicht A: oder B: sind oder wenn Quell- oder */
	/* Ziellaufwerk mit dem temporÑren Laufwerk identisch sind, wird	 */
	/* eine Fehlermeldung geliefert							 */
	/* -------------------------------------------------------------- */

	if	((slw != 0 && slw != 1) || (dlw != 0 && dlw != 1))
		return(EUNCMD);
	if	(tmp == slw || tmp == dlw)
		return(EDRIVE);
	isquell = TRUE;		/* Quelldisk ist eingelegt */

#if	MAGIXDESK
	interlv = -1;
#else
	interlv = (sysver >= 0x0102) ? -1 : 2;
#endif

	/* Wenn ein temporÑres Laufwerk angegeben wurde, Pfad erstellen	*/
	/* -------------------------------------------------------------	*/

	if	(tmp >= 0)
		{
		tmpdatei[0] = tmp + 'A';
		tmpdatei[1] = ':';
     	tmpnam(tmpdatei+2);
     	handle = -1;
		}

	/* Wenn nicht formatiert werden soll, wird das Format der		*/
	/* Zieldisk geprÅft, ob es mit dem der Quelldisk Åbereinstimmt.	*/
	/* ggf. wird trotzdem formatiert							*/
	/* ------------------------------------------------------------- */

	if	(!fmt)
		{
		long ser;
		int bps,spc,res,nfats,ndirs,nsects,media,spf,dspt,dsid,nhid,exec;
		int dtrk;

		if	(slw == dlw)
			{
#if		COUNTRY==FRG
			strncpy(chg_msg+4, " Ziel", 5);
#elif	COUNTRY==USA
			strncpy(chg_msg+11, " dest.", 6);
#endif
			if	(2 == form_alert(1, chg_msg))
				return(EBREAK);
			isquell = FALSE;
			}

		doserr = read_bootsec(dlw, &ser,&bps,&spc,&res,&nfats,&ndirs,&nsects,
						   &media,&spf,&dspt,&dsid,&nhid,&exec);
		if	(doserr != E_OK)
			return(doserr);
		if	(dsid <= 0 || dspt <= 0 || nsects <= 0)
			return(EMEDIA);
		dtrk = nsects/(dsid*dspt);
		if	(dsid != sid || trk != dtrk || dspt != spt)
			{
#if		COUNTRY==FRG
			if	(1 != form_alert(1, "[2][Formatieren ?][ OK | Abbruch]"))
#elif	COUNTRY==USA
			if	(1 != form_alert(1, "[2][Format ?][ OK | Cancel]"))
#endif
				return(E_OK);
			fmt = TRUE;
			}
		}

	/* Speicher anfordern, ggf. Tmpdatei erstellen */
	/* ------------------------------------------- */

	if	(fmt)
		{
		fbuf = Malloc(20480L);	/* Formatierpuffer */
		if	(fbuf == NULL)
			return(ENSMEM);
		}
	else fbuf = NULL;

	bufsiz_char = (unsigned long) Malloc(-1L);
	if	(bufsiz_char < 20000L)
		return(ENSMEM);
	bufsiz_char /= (sid * spt * 512);
	bufsiz_char *= (sid * spt * 512);	/* In Spuren umrechnen */
	buf = Malloc(bufsiz_char);
	if	(!buf)
		return(ENSMEM);
	bufsiz_trk = (unsigned int) (bufsiz_char/(sid * spt * 512L));

	if	(bufsiz_trk < trk && tmp >= 0)	/* Speicher reicht nicht fÅr ganze Disk */
		{
		doserr = Fcreate(tmpdatei, 0);
		if	(doserr < E_OK)
			goto err;
		handle = (int) doserr;
		}

	/* Sektornummerliste initialisieren (fÅrs Formatieren) */
	/* --------------------------------------------------- */

	for	(i = 0; i < spt; i++)
		secnos[i] = i+1;

	gelesen = FALSE;
	nochmal:
	for	(ltrack = 0; ltrack < trk; ltrack += bufsiz_trk)
		{

		/* Lesen */
		/* ----- */

		c_tracks = MIN(trk, ltrack + bufsiz_trk);	/* bis hierher lesen */
		n_bytes  = (c_tracks-ltrack)*spt*sid*512L;	/* soviele Bytes lesen */

		if	(!gelesen)
			{
			if	(slw == dlw && !isquell)
				{
#if		COUNTRY==FRG
				strncpy(chg_msg+4, "Quell", 5);
#elif	COUNTRY==USA
				strncpy(chg_msg+11, "source", 6);
#endif
				if	(2 == form_alert(1, chg_msg))
					goto abbruch;
				isquell = TRUE;
				}
			for	(ctrack = ltrack, cbuf = buf;
			      ctrack < c_tracks;
			      ctrack++, cbuf += spt*sid*512)
				{
#if		COUNTRY==FRG
				feedb("Lese", ctrack);
#elif	COUNTRY==USA
				feedb("reading", ctrack);
#endif
				for	(cside = 0; cside < sid; cside++)
					{
				if	(qbreak())
					{
					abbruch:
					doserr = EBREAK;
					goto err;
					}
					doserr = M_Floprd(cbuf + 512L*spt*cside, NULL, slw, 1, ctrack, cside, spt);
					if	(doserr != E_OK)
						goto err;
					}	
				}
			if	(tmp >= 0 && bufsiz_trk < trk)
				{
				doserr = Fwrite(handle, n_bytes, buf);
				if	(doserr != n_bytes && doserr > E_OK)
					doserr = ENSMEM;
				if	(doserr < E_OK)
					goto err;
				if	(ctrack < trk)
					continue;			/* Weiterlesen */
				gelesen = TRUE;
				doserr = Fseek(0L, handle, 0);
				if	(doserr < E_OK)
					goto err;
				goto nochmal;
				}
			}
		else {
			doserr = Fread(handle, n_bytes, buf);
			if	(doserr != n_bytes)
				goto err;
			}

		/* Schreiben und ggf. Formatieren */
		/* ------------------------------ */

		if	(slw == dlw && isquell)
			{
#if		COUNTRY==FRG
			strncpy(chg_msg+4, " Ziel", 5);
#elif	COUNTRY==USA
			strncpy(chg_msg+11, " dest.", 6);
#endif
			if	(2 == form_alert(1, chg_msg))
				goto abbruch;
			isquell = FALSE;
			}
		for	(ctrack = ltrack, cbuf = buf;
			 ctrack < c_tracks;
			 ctrack++, cbuf += spt*sid*512)
			{
			if	(fmt)
				{
#if		COUNTRY==FRG
				feedb("Formatiere", ctrack);
#elif	COUNTRY==USA
				feedb("formatting", ctrack);
#endif
				for	(cside = 0; cside < sid; cside++)
					{
					if	(qbreak())
						goto abbruch;
					doserr = M_Flopfmt(fbuf, secnos, dlw, spt, ctrack, cside, interlv,
								   (ctrack < 10) ? 0 : 0xe5e5);
					if	(doserr != E_OK)
						goto err;
					for	(i = 0; i < spt; i++)
						{
						secnos[i] -= 2;
						if	(secnos[i] < 1)
							secnos[i] += spt;
						}
					}
				}
#if		COUNTRY==FRG
			feedb("Schreibe", ctrack);
#elif	COUNTRY==USA
			feedb("writing", ctrack);
#endif
			for	(cside = 0; cside < sid; cside++)
				{
				if	(ctrack == 0 && cside == 0)
					Protobt(cbuf, 0x01000000L, -1, -1);	/* Eigene Seriennummer */
				if	(qbreak())
					goto abbruch;
				doserr = M_Flopwr(cbuf + 512L*spt*cside, NULL, dlw, 1, ctrack, cside, spt);
				if	(doserr != E_OK)
					goto err;
				}	
			}

		}

	err:
	if	(buf)
		Mfree(buf);
	if	(fbuf)
		Mfree(fbuf);
	if	(tmp >= 0 && (handle >= 0))
		{
		Fclose(handle);
		Fdelete(tmpdatei);
		}
	Rwabs(0, NULL, 2, 0, dlw);
	return(doserr);
}


#if	MAGIXDESK == 0
/*********************************************************************
*
* Setzt die Standardpfade fÅr den Parentprozeû von KAOSDESK, also fÅr
* das AES (hoffentlich).
* Wird vor dem Starten eines Programms aufgerufen, bei dem KAOSDESK
* nicht resident bleibt.
*
*********************************************************************/

void set_parent_paths( void )
{
	extern   BASPAG *_BasPag;
	register char *p,*s;
	register int  i;
	

	s = (char *) _BasPag;					/* Basepage von KAOSDESK */
	p = (char *) (* ((long *) (s + 0x24)));		/* Basepage von AES		*/
	p [0x37] = s[0x37];						/* Laufwerk kopieren	*/
	s += 0x40;							/* auf Pfadhandles		*/
	p += 0x40;
	for	(i = 0; i < 16; i++)				/* Pfadhandles tauschen	*/
		{
		char tmp;
	
		tmp = *s;
		*s++ = *p;
		*p++ = tmp;
		}
}


/*********************************************************************
*
* Setzt den Namen der aktuellen Applikation mit Hilfe von
* undokumentierten Systemvariablen.
* <path> ist NULL oder muû in Groûbuchstaben vorliegen.
*
*********************************************************************/

static void set_apname(char *path)
{
	extern char *get_name(char *path);
	register char *name;
	register int i;


	if	(!path)
		path = "KAOSDESK";
	else path = get_name(path);
	if	(NULL == (name = apname))
		menu_register(-1, path);			/* special feature */
	else	{
		for	(i = 0; i < 8; i++)
			{
			if	(*path == '.' || *path == EOS)
				*name++ = ' ';
			else *name++ = *path++;
			}
		}
}


/*********************************************************************
*
* Schaltet noch einmal sicherheitshalber den Textcursor aus und
* definitiv die Maus ein.
* Das Mauseinschalten geschieht einmal Åber VDI, andererseits aber
* auch fÅr AES. Bei KGEM ist dies bereits mit wind_new() geschehen,
* fÅr TOS 1.2 und TOS 1.4 wird auf undokumentierte Variablen
* zurÅckgegriffen
*
*********************************************************************/

static void show_mouse_immed( void )
{
	Cursconf(0,0);
	v_show_c(vdi_handle, 0);
	if	(!isgem2 && !iskgem && is_rom_aes)
		{
		if	(sysver == 0x0102)
			*MO_12_COUNT_ADDR = 0;
		if	(sysver == 0x0104)
			*MO_14_COUNT_ADDR = 0;
		}
}


/*********************************************************************
*
* Initialisiert den Windowmanager
*
*********************************************************************/

static void Mwind_new( void )
{
	int hdl;

	if	(isgem2 || sysver < 0x0104)
		{
		do	{
			if	(!wind_get(0, WF_TOP, &hdl) || hdl <= 0)
				break;
			wind_close(hdl);
			wind_delete(hdl);
			}
		while(TRUE);
		}
	else wind_new();
}


/*********************************************************************
*
* Schreibt die Parameter fÅr ein zu startendes Programm
*
*********************************************************************/

static void Mshel_write(int doex, int isgr, char *program, char *param )
{
	if	(aesvars)
		{
		shel_write(0, TRUE, 0, program, param);
		return;
		}

	if	(is_rom_aes && (sysver == 0x0102))
		{
		*SH_12_FLAG1_ADDR = *SH_12_FLAG2_ADDR = TRUE;
		strcpy(*SH_12_WPCMD_ADDR,  program);
		memcpy(*SH_12_WPTAIL_ADDR, param, 128);
		}
	else if	(is_rom_aes && (sysver == 0x0104))
		{
		*SH_14_DOEX_ADDR = FALSE;
		strcpy(*SH_14_WPCMD_ADDR,  program);
		memcpy(*SH_14_WPTAIL_ADDR, param, 128);
		}
	else {
		register int i;

		for	(i = (int) param[0] + 2; i < 128; i++)
			param[i] = (char) i;		/* "magics" */
		shel_write(doex, isgr, TRUE, program, param);
		}
}


/*********************************************************************
*
* PrÅft nach, ob ein Programm einen shel_write() gemacht hat.
* Wenn ja, gib TRUE zurÅck und setze die RÅckgabeparameter
*
*********************************************************************/

static int Mshel_read(char *program, char *param, int *isgr )
{
	char	cmd[128],tail[128];
	int	doex;


	doex = FALSE;

	if	(aesvars)
		{
		*isgr = *(aesvars->shelw_isgr);
		doex = *(aesvars->shelw_doex);
		}

	else if	(is_rom_aes && (sysver == 0x0102))
		{
		if	(!*SH_12_FLAG1_ADDR && !*SH_12_FLAG2_ADDR)
			{
			*isgr = *SH_12_WISGR_ADDR;
			doex = *SH_12_DOEX_ADDR;
			}
		}

	else if	(is_rom_aes && (sysver == 0x0104))
		{
		*isgr = *SH_14_WISGR_ADDR;
		doex = *SH_14_DOEX_ADDR;
		}

	else {
		shel_read(cmd, tail);
		if	(memcmp(tail, param, 128L))
			{
			strcpy(program, cmd);
			strncpy(param, tail, 128);
			return(TRUE);
			}
		}
	if	(doex)
		shel_read(program, param);
	return(doex);
}


/*********************************************************************
*
* Startet ein Programm, wobei KAOSDESK resident bleibt
*
* doex	- Parameter fÅr "shel_write"
* isgr	- GEM- Applikation
* program	- Zeichenkette mit vollst. Pfad des Programms
* param	- Kommandozeile
* envptr	- Zeiger aufs Environment oder NULL
* wait	- Flag fÅr "Nach TOS- Programm anhalten".
*
*********************************************************************/

long	res_exec(int doex, int isgr, char *program,
		    char *param, char *envptr)
{
	long doserr;
	int	wait;
	int  graphmode = TRUE;		/* Bildschirm ist im Grafikmodus */
	extern long setpath(char *program, int stdpath, int isover);




	if	(iskgem)
		wait = !(Sconfig('AK', 0L) & SCB_NHALT);
	else wait = TRUE;

	/* Die Schleife wird solange ausgefÅhrt, bis ein Programm keinen	*/
	/* neuen shel_write()- Aufruf macht.						*/
	/* ------------------------------------------------------------- */

	do	{

		/* Wenn KAOSDESK resident bleibt, muû KAOSDESK den Bildschirm	*/
		/* selbst in Grafik/Textmodus umschalten.					*/
		/* FÅr Grafikapplikationen wird die Titelzeile ausgegeben		*/
		/* ------------------------------------------------------------- */

		if	(!isgr)			/* neuer Modus: Text */
			{
			init_vt52();
			if	(graphmode)	/* alter Modus war: Grafik */
				{
				v_hide_c(vdi_handle);
				v_enter_cur(vdi_handle);
				}
			}
		else {				/* neuer Modus: Grafik */
			int w;
			extern int scry,scrw,char_w;

			w = scrw - (int) strlen(program) * char_w;
			if	(w >= 0)
				{
				int pxy[4];

				pxy[0] = pxy[1] = 0;
				pxy[2] = scrw;
				pxy[3] = scry - 2;
				vs_clip(vdi_handle, FALSE, pxy);
				v_bar  (vdi_handle, pxy);
				v_gtext(vdi_handle, w >> 1, 1, program);
				}
			exit_vt52();
			if	(!graphmode)	/* alter Modus war: Text */
				{
				v_exit_cur(vdi_handle);
				v_show_c(vdi_handle, 1);
				}
			}
		graphmode = isgr;
	
		/* Die Parameter werden sowohl fÅr DOS als auch fÅr AES	*/
		/* gesetzt, damit das aufgerufene Programm einen shel_read	*/
		/* machen kann.									*/
		/* -------------------------------------------------------- */

		Mshel_write(doex, isgr, program, param);

		/* Bei GEM- Programmen wird vor Programmstart ein wind_new	*/
		/* gemacht, um die Fenster zu lîschen und ab KAOS 1.4 die	*/
		/* Maus noch einmal definitiv einzuschalten				*/
		/* -------------------------------------------------------- */

		if	(isgr)
			{
			set_apname(program);
			appl_exit();
			appl_init();
			Mwind_new();
			Mgraf_mouse(HOURGLASS);
			appl_exit();
			}

		doserr = Pexec(0, program, (COMMAND *) param, envptr);

		if	(isgr)
			{
			appl_init();
			set_apname(NULL);
			Mwind_new();
			}
		else	if	(wait)
			{
			extern unsigned char waitkey( void );

#if		COUNTRY==FRG
			str_conout("\x1b" "e" "\r\nTaste drÅcken : ", 20);
#elif	COUNTRY==USA
			str_conout("\x1b" "e" "\r\npress any key : ", 20);
#endif
			waitkey();
			}

		/* Wenn kein Fehler aufgetreten ist, wird geprÅft, ob das	*/
		/* aufgerufene Programm einen shel_write() gemacht hat		*/
		/* -------------------------------------------------------- */

		doex = Mshel_read(program, param, &isgr);
		if	(doex)
			doserr = setpath(program, FALSE, FALSE); /* Pgmpfad, Desk resident */

		}
	while(doex && doserr == E_OK);



	if	(isgem2 || (sysver != 0x0102))
		{

		/* Unter TOS 1.4 kann man einen shel_write mit  		   */
		/* doex = 0 machen, um das Laden des Desktop zu erzwingen */

		shel_write((sysver != 0x0104), 1, 1, "", "\0");
		}

	if	(!graphmode)
		{
		v_exit_cur(vdi_handle);
		/* exit_vt52(); wird von open_all_windows() erledigt */
		}

	/* FÅr einige Programme ist es noch einmal nîtig, den Mauszeiger */
	/* definitiv einzuschalten.								*/
	/* FÅr VDI geht das ganz einfach, da man die Anzahl der		*/
	/* v_hide_c()- Aufrufe ignorieren kann. FÅr AES geht das 		*/
	/* blîderweise nur durch Verwendung einer undokumentierten		*/
	/* Systemvariablen (Anzahl der graf_mouse(M_OFF)- Aufrufe.		*/
	/* ------------------------------------------------------------- */

	show_mouse_immed();

	return(doserr);
}
#endif

#if	MAGIXDESK
static void *rsc_block;

long restart( void )
{
	extern   BASPAG *_BasPag;

	*(aesvars->shel_vector) = NULL;
	*((void **) (aes_global+7)) = rsc_block;
	rsc_block = NULL;
	return(Pexec(EXE_EXFR, "", _BasPag, NULL));
}
void res_exec( void )
{
	rsc_block = *((void **) (aes_global+7));
	*((void **) (aes_global+7)) = NULL;
	appl_exit();
	*(aesvars->shel_vector) = restart;
	Ptermres(-1L, 0);		/* allen Speicher behalten */
}
#endif
