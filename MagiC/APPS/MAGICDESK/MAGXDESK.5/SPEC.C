/*********************************************************************
*
* Dieses Modul enthält TOS- Spezifisches
*
*********************************************************************/


#include "MAGXDESK_GEM.H"
#include "K.H"
#include <stdlib.h>

#define _HZ_200     ((long *)          0x4ba)


/*********************************************************************
*
* Merkt Zeiger auf Betriebssystemheader und die Betriebssystemversion.
* Muß bei Initialisierung von KAOSDESK aufgerufen werden.
* TOS 1.4- Versionen, die nicht vom 6.4.89 sind, werden als TOS 1.5
* angesehen.
* Außerdem wird ein Zeiger auf den aktuellen Applikationsnamen
* gemerkt, da die Funktion menu_register(-1, name) unter GEM 2.x nicht
* mehr unterstützt wird.
*
*********************************************************************/

char os_ver_s[] = "MagiC v__.__  vom __.__.____";
static AESVARS *aesvars;

void get_syshdr(void)
{
	aesvars = *((AESVARS **) (aes_global+11));
	if (aesvars->magic2 != 0x4d414758L)
		Pterm(-1);
	inf_name[0] = *(aesvars->aes_bootdrv) + 'A';

	os_ver_s[ 7] = (((aesvars->version) >> 12)     ) + '0';
	os_ver_s[ 8] = (((aesvars->version) >> 8) & 0xf) +'0';
	os_ver_s[10] = (((aesvars->version) >> 4) & 0xf) +'0';
	os_ver_s[11] = (((aesvars->version)     ) & 0xf) +'0';

	if (aesvars->release < 3)
	{
		os_ver_s[12] = *("\340\341\342" + (aesvars->release));
	}

	os_ver_s[18] = (((aesvars->date) >> 28)      ) + '0';
	os_ver_s[19] = (((aesvars->date) >> 24) & 0xf) + '0';
	os_ver_s[21] = (((aesvars->date) >> 20) & 0xf) + '0';
	os_ver_s[22] = (((aesvars->date) >> 16) & 0xf) + '0';
	os_ver_s[24] = (((aesvars->date) >> 12) & 0xf) + '0';
	os_ver_s[25] = (((aesvars->date) >>  8) & 0xf) + '0';
	os_ver_s[26] = (((aesvars->date) >>  4) & 0xf) + '0';
	os_ver_s[27] = (((aesvars->date)      ) & 0xf) + '0';

	ismagic622 = (aesvars->version >= 0x0622);
}


/*********************************************************************
*
* Rechnet einen Alt-Tastencode in ASCII um.
*
*********************************************************************/

int alt_keycode_2_ascii(int keycode)
{
	register long result __asm__("d0");
	short key = (short) keycode;

	/* Puntaes subfunction 1 consumes a WORD, not a LONG keycode. */
	__asm__ volatile (
		"move.w %1,-(%%sp)\n\t"
		"move.w #1,-(%%sp)\n\t"
		"move.l #0x416e4b72,-(%%sp)\n\t"
		"move.w #39,-(%%sp)\n\t"
		"trap #14\n\t"
		"lea 10(%%sp),%%sp"
		: "=r" (result)
		: "r" (key)
		: "d1", "d2", "a0", "a1", "a2", "cc", "memory");
	return (int) result;
}


/*********************************************************************
*
* Ermittelt einen Cookie
*
*********************************************************************/

COOKIE *getcookie(long key)
{
	return (COOKIE *) (uintptr_t) trap_14_wlwl(39, 0x416e4b72L, 4, key);
}


/*********************************************************************
*
* Untersucht den Bootsektor eines Mediums.
*
*********************************************************************/

long read_bootsec
(
	int lw,
	long *ser, short *bps, short *spc,
	short *res, short *nfats, short *ndirs, short *nsects,
	short *media, short *spf, short *spt, short *nsides,
	short *nhid, short *exec
)
{
	long doserr;
	unsigned char *s;
	unsigned short *sw;


	if (NULL == (s = malloc(512)))
		return ENSMEM;
	doserr = Rwabs(0, s, 1, 0, lw);
/*
	if (lw == 0 || lw == 1)
		doserr = M_Floprd(s, NULL, lw, 1, 0, 0, 1);
	else
		doserr = Rwabs(0, s, 1, 0, lw);
*/
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
		*spf    = s[22] + 256 * s[23];
		*spt    = s[24] + 256 * s[25];
		*nsides = s[26] + 256 * s[27];
		*nhid   = s[28] + 256 * s[29];
		*exec   = 0;
		for	(sw = (unsigned short *) s; (unsigned char *) sw < s+512; sw++)
		{
			*exec += *sw;
		}
		*exec = (*exec == 0x1234);
	}

	free(s);
	return doserr;
}


/****************************************************************
*
* Hilfsfunktion für getHz200
*
****************************************************************/

static long _getHz200(void)
{
	return *_HZ_200;
}


/****************************************************************
*
* Systemzeit für Intervall-Berechnungen
*
****************************************************************/

long getHz200(void)
{
	return Supexec(_getHz200);
}


/****************************************************************
*
* Doppelklick-Zeit in Millisekunden,
* muß mit 5 multipliziert werden für 5-ms-Einheiten für den 200-Hz-Timer
*
****************************************************************/

unsigned get_dclick_interval_ms(void)
{
	/* Tabelle aus dem MagiC-Kernel */
	static const unsigned dclick_tab[] =
	{
		450,	/* in ms */
		330,
		275,
		220,
		165
	};

	unsigned v = (unsigned) evnt_dclick(0, 0);
	if (v > 4)
	{
		v = 4;
	}
	
	return dclick_tab[v];
}


/*********************************************************************
*
* MAGXDESK zum Programmstart verlassen.
*
*********************************************************************/

static void *rsc_block;

static long restart(void)
{
	/* _base is the public MiNTLib current basepage pointer. */
	*(aesvars->shel_vector) = NULL;
	*((void **) (aes_global+7)) = rsc_block;
	rsc_block = NULL;
	return Pexec(EXE_EXFR, "", _base, NULL);
}

void res_exec(void)
{
	rsc_block = *((void **) (aes_global + 7));
	*((void **) (aes_global + 7)) = NULL;
	appl_exit();
	*(aesvars->shel_vector) = restart;
	Ptermres(-1L, 0);		/* allen Speicher behalten */
}
