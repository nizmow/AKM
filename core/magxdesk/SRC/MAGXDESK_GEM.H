#ifndef MAGXDESK_GEM_H
#define MAGXDESK_GEM_H

#define __GEMLIB_OLDNAMES 1
#include <gem.h>
#include <gemx.h>
#include <mint/basepage.h>
#include <mint/mintbind.h>
#include <mint/osbind.h>
#include <mint/dcntl.h>
#include <mint/errno.h>
#include <mint/trap14.h>
#include <mint/ostruct.h>
#include <stddef.h>
#include <stdint.h>

/* MiNTLib uses the default 32-bit int ABI; these names mark 16-bit GEM words. */
typedef short WORD;
typedef unsigned short UWORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef char BYTE;
typedef unsigned char UBYTE;
#define cdecl __CDECL

/* Application-local four-word mouse event, not an AES parameter block. */
typedef struct {
    short x, y, bstate, kstate;
} EVNTDATA;

typedef struct __attribute__((__packed__, __aligned__(2))) {
    WORD dst_apid;
    WORD unique_flg;
    void *attached_mem;
    WORD *msgbuf;
} XAESMSG;

_Static_assert(sizeof(XAESMSG) == 12, "MagiC extended AES message layout");

typedef _DTA DTA;
typedef _DISKINFO DISKINFO;
typedef _DOSTIME DOSTIME;

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

_Static_assert(sizeof(long) == 4, "MAGXDESK requires the Atari 32-bit long ABI");
_Static_assert(sizeof(WORD) == 2, "GEM word width");
_Static_assert(sizeof(DTA) == 44, "GEMDOS DTA layout");
_Static_assert(sizeof(XATTR) == 52, "MiNT XATTR layout");

typedef struct {
    char *xte_ptmplt;
    char *xte_pvalid;
    WORD xte_vislen;
    WORD xte_scroll;
} XTED;

typedef struct {
    OBJECT *tree;
    WORD obnum;
} POPINFO;

typedef struct {
    long key;
    long value;
} COOKIE;

typedef struct __attribute__((__packed__, __aligned__(2))) {
    long magic;
    void *membot;
    void *aes_start;
    long magic2;
    long date;
    void (*chgres)(short res, short txt);
    long (**shel_vector)(void);
    char *aes_bootdrv;
    short *vdi_device;
    void *reservd1;
    void *reservd2;
    void *reservd3;
    short version;
    short release;
} AESVARS;

_Static_assert(sizeof(AESVARS) == 52, "MagiC AESVARS layout");
_Static_assert(offsetof(AESVARS, version) == 48, "MagiC AESVARS version offset");

#ifndef DD_NAK
#define DD_NAK 1
#endif

#ifndef PDOM_MINT
#define PDOM_MINT 1
#endif
#ifndef E_OK
#define E_OK 0L
#endif
#ifndef ERROR
#define ERROR (-1L)
#endif
#ifndef EDRVNR
#define EDRVNR (-2L)
#endif
#ifndef EBADRQ
#define EBADRQ (-5L)
#endif
#ifndef EWRITF
#define EWRITF (-10L)
#endif
#ifndef EREADF
#define EREADF (-11L)
#endif
#ifndef EWRPRO
#define EWRPRO (-13L)
#endif
#ifndef EDRIVE
#define EDRIVE (-15L)
#endif
#ifndef EINVFN
#define EINVFN (-32L)
#endif
#ifndef EFILNF
#define EFILNF (-33L)
#endif
#ifndef EMEDIA
#define EMEDIA (-7L)
#endif
#ifndef EPTHNF
#define EPTHNF (-34L)
#endif
#ifndef ENHNDL
#define ENHNDL (-35L)
#endif
#ifndef EACCDN
#define EACCDN (-36L)
#endif
#ifndef EIHNDL
#define EIHNDL (-37L)
#endif
#ifndef ENSMEM
#define ENSMEM (-39L)
#endif
#ifndef EIMBA
#define EIMBA (-40L)
#endif
#ifndef ENSAME
#define ENSAME (-48L)
#endif
#ifndef ENMFIL
#define ENMFIL (-49L)
#endif
/* Trap results are negative; libc's ELOCKED/ERANGE are positive errno values. */
#define GEMDOS_ELOCKED (-58L)
#ifndef ENSLOCK
#define ENSLOCK (-59L)
#endif
#define GEMDOS_ERANGE (-64L)
#ifndef EINTRN
#define EINTRN (-65L)
#endif
#ifndef EPLFMT
#define EPLFMT (-66L)
#endif
#ifndef EGSBF
#define EGSBF (-67L)
#endif
#ifndef EBREAK
#define EBREAK (-68L)
#endif
#ifndef EXCPT
#define EXCPT (-69L)
#endif
#ifndef EPTHOV
#define EPTHOV (-70L)
#endif
#ifndef ELOOP
#define ELOOP (-80L)
#endif

#define RMODE_RD 0
#define RMODE_WR 1
#define RMODE_RW 2
#ifndef FO_READ
#define FO_READ 0
#define FO_WRITE 1
#define FO_RW 2
#endif
#define FXATTR_RESOLVE 0
#define FXATTR_NRESOLVE 1
#define EXE_EXFR 6

/* MiNT Dpathconf selectors not exported by this SDK's public headers. */
#define DP_NAMEMAX 3
#define DP_TRUNC 5
#define DP_DOSTRUNC 2
#define F_RDONLY 0x01
#define F_HIDDEN 0x02
#define F_SYSTEM 0x04
#define F_VOLUME 0x08
#define F_SUBDIR 0x10
#define F_ARCHIVE 0x20

#endif
