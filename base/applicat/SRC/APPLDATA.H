/*
*
* Diese Datei beschreibt den Aufbau der Datei APPLICAT.DAT
* für die Zuweisung von Icons und Eigenschaften an Programme
* und Dateitypen.
*
*/

#include <stdint.h>

/* Flags für APPLICAT */
#define PGMT_ISGEM		1		/* Benutzt GEM (PRG, APP) */
#define PGMT_TP			2		/* Nimmt Parameter (GTP, TTP) */
#define PGMT_SINGLE		4		/* single mode */
#define PGMT_WINPATH	8		/* Pfad = oberstes Fenster */
#define PGMT_NVASTART	16		/* Programm versteht kein VA_START */
#define PGMT_ACC		32		/* Accessory */
#define PGMT_NO_PROPFNT	64		/* kein prop. AES-Zeichensatz */
/* weitere Flags */
#define PGMT_BATCH		256		/* BTP oder BAT */
#define PGMT_NOEXE		-1		/* nicht ausführbar */


/* für Icondatei: */

struct ico_head
{
	long magic;                   /* ist 'AnKr' */
	long version;                 /* ist z.Zt. 1L */
	long n_ap2ic;                 /* Länge der Tabelle ap2ic */
	long p_ap2ic;                 /* Zeiger auf Tabelle ap2ic */
	long n_da2ic;                 /* Länge der Tabelle da2ic */
	long p_da2ic;                 /* Zeiger auf Tabelle da2ic */
	long n_pa2ic;				/* Länge der Tabelle pa2ic */
	long p_pa2ic;				/* Zeiger Tabelle pa2ic */
	long n_sp2ic;				/* Länge der Tabelle sp2ic */
	long p_sp2ic;				/* Zeiger Tabelle sp2ic */
	long n_icn;                   /* Anzahl CICONBLKs */
	long p_icn;                   /* Zeiger auf Tabelle icn */
};


/* in Datei */
/* -------- */

struct ico_ap
{
	long apname;             /* Zeiger auf Name ohne Extension */
	long path;               /* -1L oder Zeiger auf Pfad */
	int16_t config;             /* -1 or Konfigurationsbits */
	long memlimit;
	long icon_nr;            /* 0..n_icn-1 */
};

struct ico_dat
{
	long daname;             /* Zeiger auf Dateityp */
	long ap;                 /* Index der App. oder -1L ("frei") */
	long icon_nr;            /* 0..n_icn-1 */
};

struct ico_path
{
	long pathname;			/* Pfad, z.B. A: oder c:\auto */
	long icon_nr;
};

struct ico_spec
{
	long key;				/* 'APPS','DATS','BTCH','DEVC' */
						/* 'FLDR','DRVS','TRSH','PRNT' */
	long icon_nr;
};

_Static_assert(sizeof(struct ico_head) == 48, "APPLICAT.DAT header ABI");
_Static_assert(sizeof(struct ico_ap) == 18, "APPLICAT.DAT application record ABI");
_Static_assert(offsetof(struct ico_ap, memlimit) == 10, "APPLICAT.DAT config is 16-bit");
_Static_assert(sizeof(struct ico_dat) == 12, "APPLICAT.DAT file-type record ABI");
_Static_assert(sizeof(struct ico_path) == 8, "APPLICAT.DAT path record ABI");
_Static_assert(sizeof(struct ico_spec) == 8, "APPLICAT.DAT special-object record ABI");


/* im Speicher */
/* ----------- */

typedef struct
{
	char *apname;
	char *path;
	int16_t config;
	long memlimit;
	CICONBLK *icon;
} APPLICATION;

typedef struct
{
	char *daname;
	APPLICATION *ap;
	CICONBLK *icon;
} DATAFILE;

typedef struct
{
	char *path;
	CICONBLK *icon;
} PATHNAME;

typedef struct
{
	long key;
	CICONBLK *icon;
} SPECIALOBJECT;
