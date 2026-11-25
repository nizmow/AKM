#ifndef MGCOPY_GEM_H
#define MGCOPY_GEM_H

/* MagiC DOS/GEM types and constants for the MGCOPY port. */
#include "GEMUT_MT_SDK.H"
#include "GEMUT_MT.H"
#include <mint/ostruct.h>

#define RMODE_RD 0
#define RMODE_WR 1

/* MagiC/MiNT Fopen extension bits for reserving a staging file. */
#ifndef O_WRONLY
#define O_WRONLY 1
#endif
#ifndef O_CREAT
#define O_CREAT 0x200
#endif
#ifndef O_EXCL
#define O_EXCL 0x800
#endif

#ifndef FA_READONLY
#define FA_READONLY FA_RDONLY
#endif
#ifndef F_RDONLY
#define F_RDONLY FA_RDONLY
#endif
#ifndef F_HIDDEN
#define F_HIDDEN FA_HIDDEN
#endif
#ifndef F_SYSTEM
#define F_SYSTEM FA_SYSTEM
#endif
#ifndef F_ARCHIVE
#define F_ARCHIVE 0x20
#endif
#ifndef S_IFMT
#define S_IFMT 0170000
#endif
#ifndef S_IFDIR
#define S_IFDIR 0040000
#endif
#ifndef S_IFREG
#define S_IFREG 0100000
#endif
#ifndef S_IFLNK
#define S_IFLNK 0160000
#endif

#define DP_NAMEMAX 3
#define DP_TRUNC 5
#define DP_DOSTRUNC 2
#define DOPEN_NORMAL 0
#define FUTIME 0x4603
#define DLOCKMODE_LOCK 1
#define DLOCKMODE_UNLOCK 0

#define EWRPRO (-13L)
#define ENSAME (-48L)
#define ERANGE (-64L)
#define EINTRN (-65L)
#define EPLFMT (-66L)
#define EGSBF (-67L)
#define EBREAK (-68L)
#define EBADRQ (-5L)
#define EDRVNR (-2L)
#define EPTHOV (-70L)
#define ENMFIL (-49L)
#define EXCPT (-69L)

/* Legacy names and layouts used by MGCOPY. */
typedef struct {
	char d_reserved[21];
	unsigned char d_attrib;
	unsigned short d_time;
	unsigned short d_date;
	unsigned long d_length;
	char d_fname[14];
} DTA;

typedef _DISKINFO DISKINFO;
typedef _DOSTIME DOSTIME;

struct mutimbuf {
	unsigned short actime;
	unsigned short acdate;
	unsigned short modtime;
	unsigned short moddate;
};

typedef struct {
	char *xte_ptmplt;
	char *xte_pvalid;
	WORD xte_vislen;
	WORD xte_scroll;
} XTED;

typedef struct {
	unsigned short mode;
	long index;
	unsigned short dev;
	unsigned short reserved1;
	unsigned short nlink;
	unsigned short uid;
	unsigned short gid;
	long size;
	long blksize;
	long nblocks;
	short mtime;
	short mdate;
	short atime;
	short adate;
	short ctime;
	short cdate;
	short attr;
	short reserved2;
	long reserved3[2];
} XATTR;

_Static_assert(sizeof(DTA) == 44, "legacy DTA layout");
_Static_assert(sizeof(XATTR) == 52, "MiNT XATTR layout");

#endif
