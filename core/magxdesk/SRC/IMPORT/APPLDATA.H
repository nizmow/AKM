/*
*
* Diese Datei beschreibt den Aufbau der Datei APPLICAT.DAT
* für die Zuweisung von Icons und Eigenschaften an Programme
* und Dateitypen.
*
*/

/* Flags für APPLICAT */
#define PGMT_ISGEM		1		/* Benutzt GEM (PRG, APP) */
#define PGMT_TP		2		/* Nimmt Parameter (GTP, TTP) */
#define PGMT_SINGLE		4		/* single mode */
#define PGMT_WINPATH	8		/* Pfad = oberstes Fenster */
#define PGMT_NVASTART	16		/* Programm versteht kein VA_START */
#define PGMT_ACC		32		/* Accessory */
#define PGMT_NO_PROPFNT	64		/* kein prop. AES-Zeichensatz */
/* weitere Flags */
#define PGMT_BATCH		256		/* BTP oder BAT */
#define PGMT_NOEXE		-1		/* nicht ausführbar */


/* für Icondatei: */

struct ico_head {
	long magic;                   /* ist 'AnKr' */
	long version;                 /* ist z.Zt. 1L */
	long n_ap2ic;                 /* Länge der Tabelle ap2ic */
	long p_ap2ic;                 /* Zeiger auf Tabelle ap2ic */
	long n_da2ic;                 /* Länge der Tabelle da2ic */
	long p_da2ic;                 /* Zeiger auf Tabelle da2ic */
	long	n_pa2ic;				/* Länge der Tabelle pa2ic */
	long	p_pa2ic;				/* Zeiger Tabelle pa2ic */
	long	n_sp2ic;				/* Länge der Tabelle sp2ic */
	long	p_sp2ic;				/* Zeiger Tabelle sp2ic */
	long n_icn;                   /* Anzahl CICONBLKs */
	long p_icn;                   /* Zeiger auf Tabelle icn */
};


/* in Datei */
/* -------- */

struct ico_ap {
     long apname;             /* Zeiger auf Name ohne Extension */
     long path;               /* -1L oder Zeiger auf Pfad */
     short config;            /* 16-bit in der archivierten Datei */
     long memlimit;
     long icon_nr;            /* 0..n_icn-1 */
} __attribute__((__packed__, __aligned__(2)));

struct ico_dat {
     long daname;             /* Zeiger auf Dateityp */
     long ap;                 /* Index der App. oder -1L ("frei") */
     long icon_nr;            /* 0..n_icn-1 */
};

struct ico_path {
	long	pathname;			/* Pfad, z.B. A: oder c:\auto */
	long	icon_nr;
};

struct ico_spec {
	long	key;				/* 'APPS','DATS','BTCH','DEVC' */
						/* 'FLDR','DRVS','TRSH','PRNT' */
	long	icon_nr;
};

_Static_assert(sizeof(struct ico_head) == 48, "APPLICAT icon header ABI");
_Static_assert(sizeof(struct ico_ap) == 18, "APPLICAT program record ABI");
_Static_assert(offsetof(struct ico_ap, memlimit) == 10, "APPLICAT config offset");
_Static_assert(offsetof(struct ico_ap, icon_nr) == 14, "APPLICAT icon offset");
_Static_assert(sizeof(struct ico_dat) == 12, "APPLICAT data record ABI");
_Static_assert(sizeof(struct ico_path) == 8, "APPLICAT path record ABI");
_Static_assert(sizeof(struct ico_spec) == 8, "APPLICAT special record ABI");


/* im Speicher */
/* ----------- */

typedef struct __attribute__((__packed__, __aligned__(2))) {
	char *apname;
	char *path;
	short config;
	long memlimit;
	CICONBLK *icon;
} APPLICATION;

typedef struct __attribute__((__packed__, __aligned__(2))) {
	char *daname;
	APPLICATION *ap;
	CICONBLK *icon;
} DATAFILE;

typedef struct __attribute__((__packed__, __aligned__(2))) {
	char *path;
	CICONBLK *icon;
} PATHNAME;

typedef struct __attribute__((__packed__, __aligned__(2))) {
	long key;
	CICONBLK *icon;
} SPECIALOBJECT;

_Static_assert(sizeof(APPLICATION) == 18, "APPLICAT application overlay ABI");
_Static_assert(offsetof(APPLICATION, memlimit) == 10, "APPLICAT memory limit offset");
_Static_assert(offsetof(APPLICATION, icon) == 14, "APPLICAT icon pointer offset");
_Static_assert(sizeof(DATAFILE) == 12, "APPLICAT data overlay ABI");
_Static_assert(sizeof(PATHNAME) == 8, "APPLICAT path overlay ABI");
_Static_assert(sizeof(SPECIALOBJECT) == 8, "APPLICAT special overlay ABI");
