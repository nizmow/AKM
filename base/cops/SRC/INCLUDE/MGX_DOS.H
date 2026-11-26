/*      MGX_DOS.H

     MagiC GEMDOS/(X)BIOS Definition Includes

	Andreas Kromke
	31.1.98

	- 20.8.99: SLB_EXEC und Slbclose korrigiert
*/

#ifndef NULL
#define NULL ((void *)0L)
#endif
#ifndef EOS
#define EOS  '\0'
#endif

#if !defined(__TOS)
#define __TOS

#include <stdint.h>


/* File Access Types */

#define RMODE_RD 0
#define RMODE_WR 1
#define RMODE_RW 2

/* Device Identifiers  (BIOS) */

#define PRT	0
#define AUX	1
#define CON	2
#define MIDI	3
#define IKBD	4
#define RAWCON	5		   /* no control- characters */

/* GEMDOS- Device- Handles */

#define HDL_CON -1
#define HDL_AUX -2
#define HDL_PRN -3
#define HDL_NUL -4		   /* KAOS extension */

/* GEMDOS- Standard- Handles */

#define STDIN	 0
#define STDOUT	 1
#define STDAUX	 2
#define STDPRN	 3
#define STDERR	 4
#define STDXTRA 5

/* File Attributes */

#define F_RDONLY 0x01
#define F_HIDDEN 0x02
#define F_SYSTEM 0x04
#define F_VOLUME 0x08
#define F_SUBDIR 0x10
#define F_ARCHIVE 0x20

/* BIOS level errors */

#define E_OK	  0L	/* OK, no error 		*/
#define ERROR	 -1L	/* basic, fundamental error	*/
#define EDRVNR	 -2L	/* drive not ready		*/
#define EUNCMD	 -3L	/* unknown command		*/
#define E_CRC	 -4L	/* CRC error			*/
#define EBADRQ	 -5L	/* bad request			*/
#define E_SEEK	 -6L	/* seek error			*/
#define EMEDIA	 -7L	/* unknown media		*/
#define ESECNF	 -8L	/* sector not found		*/
#define EPAPER	 -9L	/* no paper			*/
#define EWRITF	-10L	/* write fault			*/
#define EREADF	-11L	/* read fault			*/
#define EGENRL	-12L	/* general error		*/
#define EWRPRO	-13L	/* write protect		*/
#define E_CHNG	-14L	/* media change 		*/
#define EUNDEV	-15L	/* unknown device		*/
#define EBADSF	-16L	/* bad sectors on format	*/
#define EOTHER	-17L	/* insert other disk	*/

/* BDOS level errors */

#define EINVFN	-32L	/* invalid function number		 1 */
#define EFILNF	-33L	/* file not found				 2 */
#define EPTHNF	-34L	/* path not found				 3 */
#define ENHNDL	-35L	/* no handles left				 4 */
#define EACCDN	-36L	/* access denied				 5 */
#define EIHNDL	-37L	/* invalid handle				 6 */
#define ENSMEM	-39L	/* insufficient memory			 8 */
#define EIMBA	-40L	/* invalid memory block address 	 9 */
#define EDRIVE	-46L	/* invalid drive was specified	15 */
#define ENSAME -48L /* MV between two different drives 17 */
#define ENMFIL	-49L	/* no more files				18 */

/* our own inventions */

#define ERANGE	-64L	/* range error					33 */
#define EINTRN	-65L	/* internal error				34 */
#define EPLFMT	-66L	/* invalid program load format	35 */
#define EGSBF	-67L	/* setblock failure 			36 */

/* KAOS extensions */

#define EBREAK	-68L	/* user break (^C)				37 */
#define EXCPT	-69L	/* 68000- exception ("bombs")		38 */

/* MiNT extensions */

#define ELOCKED -58L
#define ENSLOCK -59L

/*  MagiC extensions */

#define EPTHOV -70L /* path overflow                           MAG!X    */
#define ELOOP  -80L /* too many symlinks in path               MiNT */

/* GEMDOS Pexec Modes */

#define EXE_LDEX    0                                       /* TOS */
#define EXE_LD      3                                       /* TOS */
#define EXE_EX      4                                       /* TOS */
#define EXE_BASE    5                                       /* TOS */
#define EXE_EXFR    6                                       /* TOS 1.4  */
#define EXE_XBASE   7                                       /* TOS 3.01 */


/* GEMDOS (MiNT) Fopen modes */

#define   OF_RDONLY       0
#define   OF_WRONLY       1
#define   OF_RDWR         2
#define   OF_APPEND       8
#define   OF_COMPAT       0
#define   OF_DENYRW       0x10
#define   OF_DENYW        0x20
#define   OF_DENYR        0x30
#define   OF_DENYNONE     0x40
#define   OF_CREAT        0x200
#define   OF_TRUNC        0x400
#define   OF_EXCL         0x800

/* GEMDOS Fseek Modes */

#define SEEK_SET    0                                       /* TOS */
#define SEEK_CUR    1                                       /* TOS */
#define SEEK_END    2                                       /* TOS */

/* Psemaphore */

#define PSEM_CRGET       0                                  /* MagiC 3.0 */
#define PSEM_DESTROY     1
#define PSEM_GET         2
#define PSEM_RELEASE     3

/* Dlock modes */

#define DLOCKMODE_LOCK   1
#define DLOCKMODE_UNLOCK 0
#define DLOCKMODE_GETPID 2

/* Dopendir modes */

#define DOPEN_COMPAT     1
#define DOPEN_NORMAL     0

/* Fxattr modes */

#define FXATTR_RESOLVE	0
#define FXATTR_NRESOLVE	1

/* Pdomain modes */

#define PDOM_TOS         0
#define PDOM_MINT        1

/* Modi und Codes fÅr Dpathconf() (-> MiNT) */

#define   DP_MAXREQ      -1
#define   DP_IOPEN       0
#define   DP_MAXLINKS    1
#define   DP_PATHMAX     2
#define   DP_NAMEMAX     3
#define   DP_ATOMIC      4
#define   DP_TRUNC       5
#define    DP_NOTRUNC    0
#define    DP_AUTOTRUNC  1
#define    DP_DOSTRUNC   2
#define   DP_CASE        6
#define    DP_CASESENS   0
#define    DP_CASECONV   1
#define    DP_CASEINSENS 2
#define DP_MODEATTR		7
#define	DP_ATTRBITS	0x000000ffL
#define	DP_MODEBITS	0x000fff00L
#define	DP_FILETYPS	0xfff00000L
#define	DP_FT_DIR		0x00100000L
#define	DP_FT_CHR		0x00200000L
#define	DP_FT_BLK		0x00400000L
#define	DP_FT_REG		0x00800000L
#define	DP_FT_LNK		0x01000000L
#define	DP_FT_SOCK	0x02000000L
#define	DP_FT_FIFO	0x04000000L
#define	DP_FT_MEM		0x08000000L
#define DP_XATTRFIELDS	8
#define	DP_INDEX		0x0001
#define	DP_DEV		0x0002
#define	DP_RDEV		0x0004
#define	DP_NLINK		0x0008
#define	DP_UID		0x0010
#define	DP_GID		0x0020
#define	DP_BLKSIZE	0x0040
#define	DP_SIZE		0x0080
#define	DP_NBLOCKS	0x0100
#define	DP_ATIME		0x0200
#define	DP_CTIME		0x0400
#define	DP_MTIME		0x0800

/* additional Dcntl/Fcntl Modes */

#define CDROMEJECT			(('C'<<8)|0x09)
#define CDROMPREVENTREMOVAL	(('C'<<8)|0x0e)
#define CDROMALLOWREMOVAL	(('C'<<8)|0x0f)
#define KER_DRVSTAT			0x0104	/* Kernel: Drive-Status (ab 9.9.95) */
#define KER_XFSNAME			0x0105	/* Kernel: XFS-Name (ab 15.6.96) */
#define VFAT_CNFDFLN		0x5600	/* VFAT-XFS: ab 2.1.96 */
#define VFAT_CNFLN			0x5601	/* VFAT-XFS: ab 2.1.96 */
#define PLOADINFO			0x500c

/* D/Fcntl(PLOADINFO,...) */

struct ploadinfo
{
	 int16_t fnamelen;			/* LÑnge von <fname> inkl. EOS */
	 char *cmdlin;			/* Zeiger auf 128 Bytes Platz */
	 char *fname;			/* Zeiger auf <fnamelen> Bytes Platz */
};

/* D/Fcntl(FUTIME,...) */

struct mutimbuf
{
	 uint16_t actime;          /* Zugriffszeit */
	 uint16_t acdate;
	 uint16_t modtime;         /* letzte énderung */
	 uint16_t moddate;
};

typedef struct          /* used by Cconrs */
{
	uint8_t maxlen;
	uint8_t actuallen;
	char buffer[255];
} LINE;


typedef struct          /* used by Fsetdta, Fgetdta */
{
	char            d_reserved[21];
	uint8_t   d_attrib;
	uint16_t    d_time;
	uint16_t    d_date;
	uint32_t   d_length;
	char            d_fname[14];
} DTA;


typedef struct          /* used by Dfree */
{
    uint32_t   b_free;
    uint32_t   b_total;
    uint32_t   b_secsiz;
    uint32_t   b_clsiz;
} DISKINFO;


typedef struct baspag   /* used by Pexec */
{
	void    *p_lowtpa;
	void    *p_hitpa;
	void    *p_tbase;
	int32_t p_tlen;
	void    *p_dbase;
	int32_t p_dlen;
	void    *p_bbase;
	int32_t p_blen;
	DTA     *p_dta;
	struct baspag *p_parent;
	int32_t p_resrvd0;
	char    *p_env;
	char    p_stdfh[6];
	char    p_resrvd1;
	char    p_curdrv;
	int32_t p_resrvd2[18];
	char    p_cmdlin[128];
} BASPAG;


typedef struct          /* used by Getbpb */
{
	int16_t recsiz;
	int16_t clsiz;
	int16_t clsizb;
	int16_t rdlen;
	int16_t fsiz;
	int16_t fatrec;
	int16_t datrec;
	int16_t numcl;
	int16_t bflags;
} BPB;


typedef struct
{
	uint16_t  time;
	uint16_t  date;
} DOSTIME;


typedef struct          /* used by Iorec */
{
	void    *ibuf;
	int16_t ibufsiz;
	int16_t ibufhd;
	int16_t ibuftl;
	int16_t ibuflow;
	int16_t ibufhi;
} IOREC;


typedef struct          /* used by Kbdvbase */
{
	void    (*kb_midivec)();
	void    (*kb_vkbderr)();
	void    (*kb_vmiderr)();
	void    (*kb_statvec)();
	void    (*kb_mousevec)();
	void    (*kb_clockvec)();
	void    (*kb_joyvec)();
	void    (*kb_midisys)();
	void    (*kb_kbdsys)();
} KBDVBASE;


typedef struct          /* used by Pexec */
{
	uint8_t   length;
	char            command_tail[128];
} COMMAND;


typedef struct			/* Header for executable files */
{
	int16_t branch;		/* always == 0x601a */
	int32_t tlen;		/* length of TEXT segment */
	int32_t dlen;		/* length of DATA segment */
	int32_t blen;		/* length of BSS segment */
	int32_t slen;		/* length of symbol table */
	int32_t res1;		/* unused, must be zero */
	int32_t flags;		/* different flags */
	int16_t reloflag;		/* if not zero, neither relocate nor clear BSS */
} PH;

#define PH_MAGIC	0x601a					/* value of PH.branch */
#define PHFLAG_DONT_CLEAR_HEAP	0x00000001	/* PH.flags */
#define PHFLAG_LOAD_TO_FASTRAM	0x00000002	/* PH.flags */
#define PHFLAG_MALLOC_FROM_FASTRAM	0x00000004	/* PH.flags */
#define PHFLAG_MINIMAL_RAM		0x00000008	/* PH.flags (MagiC 5.20) */
#define PHFLAG_MEMPROT			0x000000f0	/* PH.flags (MiNT) */
#define PHFLAG_SHARED_TEXT		0x00000800	/* PH.flags (MiNT) */
#define PHFLAG_TPA_SIZE			0xf0000000	/* PH.flags */


typedef struct          /* used by Initmouse */
{
	char    topmode;
	char    buttons;
	char    x_scale;
	char    y_scale;
	int16_t x_max;
	int16_t y_max;
	int16_t x_start;
	int16_t y_start;
} MOUSE;


typedef struct          /* used by Prtblk() */
{
	void    *pb_scrptr;
	int16_t pb_offset;
	int16_t pb_width;
	int16_t pb_height;
	int16_t pb_left;
	int16_t pb_right;
	int16_t pb_screz;
	int16_t pb_prrez;
	void    *pb_colptr;
	int16_t pb_prtype;
	int16_t pb_prport;
	void    *pb_mask;
} PBDEF;


typedef struct          /* used by Keytbl() */
{
	char *unshift;
	char *shift;
	char *capslock;
} KEYTAB;


typedef struct __md
{
	struct __md *m_link;
	void        *m_start;
	int32_t     m_length;
	BASPAG      *m_own;
} MD;


typedef struct          /* used by Getmpb */
{
	MD *mp_mfl;
	MD *mp_mal;
	MD *mp_rover;
} MPB;

typedef struct
{
	int16_t (*Bconstat)();
	int32_t (*Bconin)();
	int16_t (*Bcostat)();
	void    (*Bconout)();
	int32_t (*Rsconf)();
	IOREC *iorec;
} MAPTAB;

typedef struct
{
	MAPTAB *maptab;
	int16_t    maptabsize;
} BCONMAP;

/* system variable _sysbase (0x4F2L) points to next structure         */
typedef struct _syshdr
{
    unsigned       os_entry;   /* $00 BRA to reset handler             */
    unsigned       os_version; /* $02 TOS version number               */
    void           *os_start;  /* $04 -> reset handler                 */
    struct _syshdr *os_base;   /* $08 -> baseof OS                     */
    void           *os_membot; /* $0c -> end BIOS/GEMDOS/VDI ram usage */
    void           *os_rsv1;   /* $10 << unused,reserved >>            */
    int32_t        *os_magic;  /* $14 -> GEM memoryusage parm. block   */
    int32_t        os_gendat;  /* $18 Date of system build($MMDDYYYY)  */
    int16_t        os_conf;    /* $1c OS configuration bits            */
    int16_t        os_gendatg; /* $1e DOS-format date of systembuild   */
/*
    The next three fields are only available in TOS versions 1.2 and
    greater
*/
    void           *_root;     /* $20 -> base of OS pool               */
    int32_t        *kbshift;   /* $24 -> keyboard shift state variable */
    BASPAG         **_run;     /* $28 -> GEMDOS PID of current process */
    void           *p_rsv2;    /* $2c << unused, reserved >>           */
} SYSHDR;


/* Cookie structure */

typedef struct
{
	int32_t key;
	int32_t value;
} COOKIE;

/* Sconfig(2) -> */

typedef struct
{
	char      *in_dos;                 /* Adresse der DOS- Semaphore */
	int16_t   *dos_time;               /* Adresse der DOS- Zeit      */
	int16_t   *dos_date;               /* Adresse des DOS- Datums    */
	int32_t   res1;                    /*                            */
	int32_t   res2;                    /*                            */
	int32_t   res3;                    /* ist 0L                     */
	void      *act_pd;                 /* Laufendes Programm         */
	int32_t   res4;                    /*                            */
	int16_t   res5;                    /*                            */
	void      *res6;                   /*                            */
	void      *res7;                   /* interne DOS- Speicherliste */
	void      (*resv_intmem)();        /* DOS- Speicher erweitern    */
	int32_t   (*etv_critic)();         /* etv_critic des GEMDOS      */
	char *    ((*err_to_str)(char e)); /* Umrechnung Code->Klartext  */
	int32_t   res8;                    /*                            */
	int32_t   res9;                    /*                            */
	int32_t   res10;                   /*                            */
} DOSVARS;

/* os_magic -> */

typedef struct
{
	 int32_t magic;                   /* muû $87654321 sein              */
	 void *membot;                 /* Ende der AES- Variablen         */
	 void *aes_start;              /* Startadresse                    */
	 int32_t magic2;                  /* ist 'MAGX'                      */
	 int32_t date;                    /* Erstelldatum ttmmjjjj           */
	 void (*chgres)(int16_t res, int16_t txt);  /* Auflîsung Ñndern           */
	 int32_t (**shel_vector)(void);   /* residentes Desktop              */
	 char *aes_bootdrv;            /* von hieraus wurde gebootet      */
	 int16_t  *vdi_device;             /* vom AES benutzter VDI-Treiber   */
	 void *reservd1;
	 void *reservd2;
	 void *reservd3;
	 int16_t  version;                 /* z.B. $0201 ist V2.1             */
	 int16_t  release;                 /* 0=alpha..3=release              */
} AESVARS;

/* Cookie MagX --> */

typedef struct
{
	int32_t config_status;
	DOSVARS *dosvars;
	AESVARS *aesvars;
	void *res1;
	void *hddrv_functions;
	int32_t status_bits;             /* MagiC 3 ab 24.5.95         */
} MAGX_COOKIE;

/* Bits for <status_bits> in MAGX_COOKIE (read only!) */

#define MGXSTB_TSKMAN_ACTIVE  1    /* MagiC task manager is currently active */

/* attributes for Fcreate/Fsfirst/Fsnext: */

#define FA_READONLY     0x01
#define FA_HIDDEN       0x02
#define FA_SYSTEM       0x04
#define FA_VOLUME       0x08
#define FA_SUBDIR       0x10
#define FA_ARCHIVE      0x20

/* Fopen modes: */

#define FO_READ         0
#define FO_WRITE        1
#define FO_RW           2

extern BASPAG *_BasPag;
extern int32_t _PgmSize;

int32_t gemdos(void, ...);
int32_t bios(void, ...);
int32_t xbios(void, ...);

/****** Tos *************************************************************/

void    Pterm0(void);
int32_t Cconin(void);
void    Cconout(int16_t c);
int16_t Cauxin(void);
void    Cauxout(int16_t c);
int16_t Cprnout(int16_t c);
int32_t Crawio(int16_t w);
int32_t Crawcin(void);
int32_t Cnecin(void);
int16_t Cconws(const char *buf);
int32_t Cconrs(LINE *buf);
int16_t Cconis(void);
int32_t Dsetdrv(int16_t drv);
int16_t Cconos(void);
int16_t Cprnos(void);
int16_t Cauxis(void);
int16_t Cauxos(void);
int16_t Dgetdrv(void);
void    Fsetdta(DTA *buf);
int32_t Super(void *stack);
uint16_t  Tgetdate(void);
uint16_t Tsetdate(uint16_t date);
uint16_t  Tgettime(void);
uint16_t  Tsettime(uint16_t time);
DTA     *Fgetdta(void);
int16_t Sversion(void);
void    Ptermres(int32_t keepcnt, int16_t retcode);
int16_t Dfree(DISKINFO *buf, int16_t driveno);
int16_t Dcreate(const char *path);
int16_t Ddelete(const char *path);
int16_t Dsetpath(const char *path);
int32_t Fcreate(const char *filename, int16_t attr);
int32_t Fopen(const char *filename, int16_t mode);
int16_t Fclose(int16_t handle);
int32_t Fread(int16_t handle, int32_t count, void *buf);
int32_t Fwrite(int16_t handle, int32_t count, void *buf);
int32_t Fdelete(const char *filename);
int32_t Fseek(int32_t offset, int16_t handle, int16_t seekmode);
int16_t Fattrib(const char *filename, int16_t wflag, int16_t attrib);
int32_t Fdup(int16_t handle);
int16_t Fforce(int16_t stch, int16_t nonstdh);
int16_t Dgetpath(char *path, int16_t driveno);
void    *Malloc(int32_t number);
int16_t Mfree(void *block);
int16_t Mshrink(int16_t zero, void *block, int32_t newsiz);
int32_t Pexec(int16_t mode, char *ptr1, void *ptr2, void *ptr3);
void    Pterm(int16_t retcode);
int16_t Fsfirst(const char *filename, int16_t attr);
int16_t Fsnext(void);
int16_t Frename(int16_t zero, const char *oldname, const char *newname);
int16_t Fdatime(DOSTIME *timeptr, int16_t handle, int16_t wflag);

/****** MagiC 030 Gemdos Extension ****************************************/

/* new GEMDOS- Calls */

#define SCONFIG		0x33							/* KAOS 1.2 */
#define Sconfig(a,b)    gemdos(0x33, (int16_t) (a), (int32_t) (b)) /* KAOS 1.2 */
/* extern int32_t Sconfig(int16_t fn, ...); */
#define Fshrink(a)      Fwrite(a, 0L, (void *) -1L)         /* KAOS 1.2 */
#define Mgrow(a,b)      Mshrink(a,b)                        /* KAOS 1.2 */
#define Mblavail(a)     Mshrink(a,-1L)                      /* KAOS 1.2 */
#define Ssync()         gemdos(0x150)					/* MagiC 4.01 */

/* Sconfig - Functions */

#define SC_GETCONF   0                                      /* KAOS 1.2 */
#define SC_SETCONF   1                                      /* KAOS 1.2 */
#define SC_DOSVARS   2                                      /* KAOS 1.4 */
#define SC_MOWNER	 3		/* reserviert */			/* MagiC 3.00 */
#define SC_WBACK	 4								/* MagiC 4.01 */
#define  SCWB_GET	 0		/* (fÅr WBDAEMON) */		/* MagiC 4.01 */
#define  SCWB_SET    1								/* MagiC 4.01 */
#define  SCWB_RESET  2								/* MagiC 4.01 */
#define SC_INTMAVAIL 5        /* reserviert */			/* MagiC 4.01 */
#define SC_INTGARBC  6		/* reserviert */			/* MagiC 4.01 */

/* Sconfig Bits */

#define SCB_PTHCK   0x001                                   /* KAOS 1.2 */
#define SCB_DSKCH   0x002                                   /* KAOS 1.2 */
#define SCB_BREAK   0x004                                   /* KAOS 1.2 */
#define SCB_NCTLC   0x008                                   /* KAOS 1.2 */
#define SCB_NFAST   0x010                                   /* KAOS 1.2 */
#define SCB_CMPTB   0x020                                   /* KAOS 1.4 */
#define SCB_NSMRT   0x040                                   /* KAOS 1.4 */
#define SCB_NGRSH   0x080                                   /* KAOS 1.4 */
#define SCB_NHALT   0x100                                   /* KAOS 1.4 */
#define SCB_RESVD   0x200                                   /* KAOS 1.4 */
#define SCB_PULLM   0x400                                   /* KAOS 1.4 */
#define SCB_FLPAR   0x800                                   /* MagiC 3.0 */

/* Memory Control Block */

typedef struct
{
	int32_t mcb_magic;                    /* 'ANDR' oder 'KROM' (letzter)    */
	int32_t mcb_len;                      /* NettolÑnge                      */
	int32_t mcb_owner;                    /* PD *                            */
	int32_t mcb_prev;                     /* vorh. Block oder NULL           */
	char mcb_data[0];
} MCB;

/****** TOS 030 Gemdos Extension ****************************************/

/* Modes for Mxalloc() */
#define MX_STRAM		0x00
#define MX_TTRAM		0x01
#define MX_PREFSTRAM    0x02
#define MX_PREFTTRAM    0x03
/* Protection bits */
#define MX_MPROT        0x08			/* change protection of already allocated block */
#define MX_HEADER		0x00
#define MX_PRIVATE		0x10
#define MX_GLOBAL		0x20
#define MX_SUPERVISOR	0x30
#define MX_READABLE		0x40
void    *Mxalloc(int32_t number, int16_t mode);

int32_t Maddalt(void *start, int32_t size);

/****** Network Gemdos Extension ****************************************/

int32_t Flock(int16_t handle, int16_t mode, int32_t start, int32_t length);

/****** Bios ************************************************************/

void    Getmpb(MPB *ptr);
int16_t Bconstat(int16_t dev);
int32_t Bconin(int16_t dev);
void    Bconout(int16_t dev, int16_t c);
int32_t Rwabs(int16_t rwflag, void *buf, int16_t cnt, int16_t recnr, int16_t dev);
void    (*Setexc(int16_t number, void (*exchdlr)())) ();
int32_t Tickcal(void);
BPB     *Getbpb(int16_t dev);
int32_t Bcostat(int16_t dev);
int32_t Mediach(int16_t dev);
int32_t Drvmap(void);

/* Mode bitmask for Kbshift() */
#define K_RSHIFT		0x0001
#define K_LSHIFT		0x0002
#define K_CTRL			0x0004
#define K_ALT			0x0008
#define K_CAPSLOCK		0x0010
#define K_CLRHOME		0x0020
#define K_INSERT		0x0040
int32_t Kbshift(uint16_t mode);


/****** XBios ***********************************************************/

void    Initmouse(int16_t type, MOUSE *par, void (*mousevec)());
void    *Ssbrk(int16_t count);
void    *Physbase(void);
void    *Logbase(void);
int16_t Getrez(void);
void    Setscreen(void *laddr, void *paddr, int16_t rez);
void    Setpalette(void *pallptr);
int16_t Setcolor(int16_t colornum, int16_t color);
int16_t Floprd(void *buf, void *filler, int16_t devno, int16_t sectno,
               int16_t trackno, int16_t sideno, int16_t count);
int16_t Flopwr(void *buf, void *filler, int16_t devno, int16_t sectno,
               int16_t trackno, int16_t sideno, int16_t count);
int16_t Flopfmt(void *buf, void *filler, int16_t devno, int16_t spt, int16_t trackno,
                int16_t sideno, int16_t interlv, int32_t magic, int16_t virgin);
void    Midiws(int16_t cnt, void *ptr);
void    Mfpint(int16_t erno, void (*vector)());
IOREC   *Iorec(int16_t dev);
int32_t Rsconf(int16_t baud, int16_t ctr, int16_t ucr, int16_t rsr, int16_t tsr, int16_t scr);
KEYTAB  *Keytbl(void *unshift, void *shift, void *capslock);
int32_t Random(void);
void    Protobt(void *buf, int32_t serialno, int16_t disktype, int16_t execflag);
int16_t Flopver(void *buf, void *filler, int16_t devno, int16_t sectno,
                int16_t trackno, int16_t sideno, int16_t count);
void    Scrdmp(void);
int16_t Cursconf(int16_t func, int16_t rate);
void    Settime(uint32_t time);
uint32_t  Gettime(void);
void    Bioskeys(void);
void    Ikbdws(int16_t count, void *ptr);
void    Jdisint(int16_t number);
void    Jenabint(int16_t number);
char    Giaccess(char data, int16_t regno);
void    Offgibit(int16_t bitno);
void    Ongibit(int16_t bitno);
void    Xbtimer(int16_t timer, int16_t control, int16_t data, void (*vector)());
void    *Dosound(void *buf);
int16_t Setprt(int16_t config);
KBDVBASE *Kbdvbase(void);
int16_t Kbrate(int16_t initial, int16_t repeat);
void    Prtblk(PBDEF *par);
void    Vsync(void);
int32_t Supexec(int32_t (*func)());
void    Puntaes(void);
int16_t Floprate(int16_t devno, int16_t newrate);
int16_t Blitmode(int16_t mode);

/* TOS030 XBios */
int16_t DMAread(int32_t sector, int16_t count, void *buffer, int16_t devno);
int16_t DMAwrite(int32_t sector, int16_t count, void *buffer, int16_t devno);
int16_t NVMaccess(int16_t opcode, int16_t start, int16_t count, void *buffer);
int32_t Bconmap(int16_t devno);
int16_t Esetshift(int16_t shftMode);
int16_t Egetshift(void);
int16_t EsetBank(int16_t bankNum);
int16_t EsetColor(int16_t colorNum, int16_t color);
void    EsetPalette(int16_t colorNum, int16_t count, int16_t *palettePtr);
void    EgetPalette(int16_t colorNum, int16_t count, int16_t *palettePtr);
int16_t EsetGray(int16_t swtch);
int16_t EsetSmear(int16_t swtch);

/* MinT Gemdos */

/* Fcntl opcodes */

#define	F_DUPFD		0x0000
#define	F_GETFD		0x0001
#define	F_SETFD		0x0002
 #define	FD_CLOEXEC	1

#define   FSTAT          0x4600
#define   FIONREAD       0x4601
#define   FIONWRITE      0x4602
#define   FUTIME         0x4603
#define   FTRUNCATE      0x4604
#define   SHMGETBLK      0x4d00
#define   SHMSETBLK      0x4d01
#define   PBASEADDR      0x5002

/* signal handling */

#define NSIG       31       /* number of signals recognized   */

#define SIGNULL     0       /* not really a signal             */
#define SIGHUP      1       /* hangup signal                   */
#define SIGINT      2       /* sent by ^C                      */
#define SIGQUIT     3       /* quit signal                     */
#define SIGILL      4       /* illegal instruction             */
#define SIGTRAP     5       /* trace trap                      */
#define SIGABRT     6       /* abort signal                    */
#define SIGPRIV     7       /* privilege violation             */
#define SIGFPE      8       /* divide by zero                  */
#define SIGKILL     9       /* cannot be ignored               */
#define SIGBUS     10       /* bus error                       */
#define SIGSEGV    11       /* illegal memory reference        */
#define SIGSYS     12       /* bad argument to a system call   */
#define SIGPIPE    13       /* broken pipe                     */
#define SIGALRM    14       /* alarm clock                     */
#define SIGTERM    15       /* software termination signal     */
#define SIGURG     16       /* urgent condition on I/O channel */
#define SIGSTOP    17       /* stop signal not from terminal   */
#define SIGTSTP    18       /* stop signal from terminal       */
#define SIGCONT    19       /* continue stopped process        */
#define SIGCHLD    20       /* child stopped or exited         */
#define SIGTTIN    21       /* read by background process      */
#define SIGTTOU    22       /* write by background process     */
#define SIGIO      23       /* I/O possible on a descriptor    */
#define SIGXCPU    24       /* CPU time exhausted              */
#define SIGXFSZ    25       /* file size limited exceeded      */
#define SIGVTALRM  26       /* virtual timer alarm             */
#define SIGPROF    27       /* profiling timer expired         */
#define SIGWINCH   28       /* window size changed             */
#define SIGUSR1    29       /* user signal 1                   */
#define SIGUSR2    30       /* user signal 2                   */

#define SIG_DFL    0        /* default signal handler          */
#define SIG_IGN    1        /* ignore the signal               */


/* file types */
#define S_IFMT  0170000     /* mask to select file type */
#define S_IFCHR 0020000     /* BIOS special file */
#define S_IFDIR 0040000     /* directory file */
#define S_IFREG 0100000     /* regular file */
#define S_IFIFO 0120000     /* FIFO */
#define S_IMEM  0140000     /* memory region or process */
#define S_IFLNK 0160000     /* symbolic link */

/* special bits: setuid, setgid, sticky bit */
#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000

/* file access modes for user, group, and other*/
#define S_IRUSR         0400
#define S_IWUSR         0200
#define S_IXUSR         0100
#define S_IRGRP         0040
#define S_IWGRP         0020
#define S_IXGRP         0010
#define S_IROTH         0004
#define S_IWOTH         0002
#define S_IXOTH         0001
#define DEFAULT_DIRMODE (0777)
#define DEFAULT_MODE    (0666)


typedef struct
{
	int32_t  msg1;
	int32_t  msg2;
	int16_t  pid;
} MSG;

typedef struct
{
	uint16_t mode;
	int32_t  index;
	uint16_t dev;
	uint16_t reserved1;
	uint16_t nlink;
	uint16_t uid;
	uint16_t gid;
	int32_t  size;
	int32_t  blksize;
	int32_t  nblocks;
	int16_t  mtime;
	int16_t  mdate;
	int16_t  atime;
	int16_t  adate;
	int16_t  ctime;
	int16_t  cdate;
	int16_t  attr;
	int16_t  reserved2;
	int32_t  reserved3[2];
} XATTR;


#define SA_NOCLDSTOP    1

typedef struct
{
	void    cdecl (*sa_handler)(int32_t sig);
	int32_t sa_mask;
	int16_t sa_flags;
} SIGACTION;


int16_t Syield(void);
int16_t Fpipe(int16_t *usrh);
int16_t Fcntl(int16_t f, int32_t arg, int16_t cmd);
int32_t Finstat(int16_t f);
int32_t Foutstat(int16_t f);
int32_t Fgetchar(int16_t f, int16_t mode);
int32_t Fputchar(int16_t f, int32_t c, int16_t mode);
int32_t Pwait(void);
int16_t Pnice(int16_t delta);
int16_t Pgetpid(void);
int16_t Pgetppid(void);
int16_t Pgetpgrp(void);
int16_t Psetpgrp(int16_t pid, int16_t newgrp);
int16_t Pgetuid(void);
int16_t Psetuid(int16_t id);
int16_t Pkill(int16_t pid, int16_t sig);
void    *Psignal(int16_t sig, void *handler);
int16_t Pvfork(void);
int16_t Pgetgid(void);
int16_t Psetgid(int16_t id);
int32_t Psigblock(int32_t mask);
int32_t Psigsetmask(int32_t mask);
int32_t Pusrval(int32_t arg);
int16_t Pdomain(int16_t newdom);
int32_t Psigreturn(void);
int16_t Pfork(void);
int32_t Pwait3(int16_t flag, int32_t *rusage);
int16_t Fselect(uint16_t timeout, int32_t *rfds, int32_t *wfds, int32_t *xfds);
void    Prusage(int32_t *r);
int32_t Psetlimit(int16_t lim, int32_t value);
int32_t Talarm(int32_t secs);
int32_t	Tmalarm(int32_t millisecs);
void    Pause(void);
int32_t Sysconf(int16_t n);
int32_t Psigpending(void);
int32_t Dpathconf(char *name, int16_t n);
int32_t Pmsg(int16_t mode, int32_t mbox, MSG *msg);
int32_t Fmidipipe(int16_t pid, int16_t in, int16_t out);
int16_t Prenice(int16_t pid, int16_t delta);
int32_t Dopendir(char *name, int16_t flag);
int32_t Dreaddir(int16_t buflen, int32_t dir, char *buf);
int32_t	Dxreaddir(int16_t len, int32_t dirhandle, char *buf, XATTR *xattr, int32_t *xr);
int32_t Drewinddir(int32_t dir);
int32_t Dclosedir(int32_t dir);
int32_t Fxattr(int16_t flag, const char *name, XATTR *buf);
int32_t Flink(char *oldname, char *newname);
int32_t Fsymlink(char *oldname, char *newname);
int32_t Freadlink(int16_t size, char *buf, char *name);
int32_t Dcntl(int16_t cmd, char *name, int32_t arg);
int32_t Fchown(char *name, int16_t uid, int16_t gid);
int32_t Fchmod(char *name, int16_t mode);
int32_t Pumask(uint16_t mode);
int32_t Psemaphore(int16_t mode, int32_t id, int32_t timeout);
int32_t Dlock(int16_t mode, int16_t drive);
void    Psigpause(uint32_t mask);
int32_t Psigaction(int16_t sig, SIGACTION *act, SIGACTION *oact);
int16_t Pgeteuid(void);
int16_t Pgetegid(void);
int32_t Pwaitpid(int16_t pid, int16_t flag, int32_t *rusage);
int32_t	Dreadlabel(char *path, char *buf, int16_t len);
int32_t	Dwritelabel(char *path, char *name);

/* Falcon */

#define VERTFLAG          0x0100
#define STMODES           0x0080
#define OVERSCAN          0x0040
#define PAL               0x0020
#define VGA_FALCON        0x0010
#define TV                0x0000

#define COL80             0x08
#define COL40             0x00

#define BPS16             4
#define BPS8              3
#define BPS4              2
#define BPS2              1
#define BPS1              0

#define NUMCOLS           7


typedef struct
{
	int16_t blocktype;
	int32_t blocksize;
	void    *blockaddr;
} DSPBLOCK;


int32_t locksnd(void);
int32_t unlocksnd(void);
int32_t soundcmd(int16_t mode, int16_t data);
int32_t setbuffer(int16_t reg, void *begaddr, void *endaddr);
int32_t setmode(int16_t mode);
int32_t settracks(int16_t playtracks, int16_t rectracks);
int32_t setmontracks(int16_t montrack);
int32_t setinterrupt(int16_t src_inter, int16_t cause);
int32_t buffoper(int16_t mode);
int32_t dsptristate(int16_t dspxmit, int16_t dsprec);
int32_t gpio(int16_t mode, int16_t data);
int32_t devconnect(int16_t src, int16_t dst, int16_t srcclk, int16_t prescale,
                    int16_t protocol);
int32_t sndstatus(int16_t reset);
int32_t buffptr(int32_t *ptr);

int16_t Dsp_LoadProg(char *file, int16_t ability, char *buffer);
int32_t Dsp_LodToBinary(char *file, char *codeptr);
void    Dsp_DoBlock(char *data_in, int32_t size_in, char *data_out,
                     int32_t size_out);
void    Dsp_BlkHandShake(char *data_in, int32_t size_in, char *data_out,
                          int32_t size_out);
void    Dsp_BlkUnpacked(int32_t *data_in, int32_t size_in, int32_t *data_out,
                         int32_t size_out);
void    Dsp_InStream(char *data_in, int32_t block_size, int32_t num_blocks,
                      int32_t *blocks_done);
void    Dsp_OutStream(char *data_out, int32_t block_size, int32_t num_blocks,
                       int32_t *blocks_done);
void    Dsp_IOStream(char *data_in, char *data_out, int32_t block_insize,
                      int32_t block_outsize, int32_t num_blocks,
                      int32_t *blocks_done);
void    Dsp_RemoveInterrupts(int16_t mask);
int16_t Dsp_GetWordSize(void);
int16_t Dsp_Lock(void);
void    Dsp_Unlock(void);
void    Dsp_Available(int32_t *xavailable, int32_t *yavailable);
int16_t Dsp_Reserve(int32_t xreserve, int32_t yreserve);
void    Dsp_ExecProg(char *codeptr, int32_t codesize, int16_t ability);
void    Dsp_ExecBoot(char *codeptr, int32_t codesize, int16_t ability);
void    Dsp_TriggerHC(int16_t vector);
int16_t Dsp_RequestUniqueAbility(void);
int16_t Dsp_GetProgAbility(void);
void    Dsp_FlushSubroutines(void);
int16_t Dsp_LoadSubroutine(char *codeptr, int32_t codesize, int16_t ability);
int16_t Dsp_InqSubrAbility(int16_t ability);
int16_t Dsp_RunSubroutine(int16_t handle);
int16_t Dsp_Hf0(int16_t flag);
int16_t Dsp_Hf1(int16_t flag);
int16_t Dsp_Hf2(void);
int16_t Dsp_Hf3(void);
void    Dsp_BlkWords(void *data_in, int32_t size_in, void *data_out,
                      int32_t size_out);
void    Dsp_BlkBytes(void *data_in, int32_t size_in, void *data_out,
                      int32_t size_out);
char    Dsp_HStat(void);
void    Dsp_SetVectors(void (*receiver)(), int32_t (*transmitter)());
void    Dsp_MultBlocks(int32_t numsend, int32_t numreceive,
                        DSPBLOCK *sendblocks, DSPBLOCK *receiveblocks);
int16_t Vsetmode(int16_t modecode);
int16_t mon_type(void);
int32_t VgetSize(int16_t mode);
void    VsetSync(int16_t flag);
void    VsetRGB(int16_t index, int16_t count, int32_t *array);
void    VgetRGB(int16_t index, int16_t count, int32_t *array);
void    VsetMask(int16_t ormask, int16_t andmask, int16_t overlay);

typedef void *SHARED_LIB;
typedef int32_t cdecl (*SLB_EXEC)(SHARED_LIB sl, int32_t fn, int16_t nargs, ...);

extern int32_t Slbopen(char *name, char *path, int32_t min_ver,
				SHARED_LIB *sl, SLB_EXEC *fn, int32_t param);
extern int32_t Slbclose(SHARED_LIB sl);

#endif

/************************************************************************/
