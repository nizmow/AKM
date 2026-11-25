#ifndef AES_C_MGX_DOS_H
#define AES_C_MGX_DOS_H

#include <portab.h>
#include <tosdefs.h>

#define F_RDONLY 0x01
#define F_HIDDEN 0x02
#define F_SYSTEM 0x04
#define F_VOLUME 0x08
#define F_SUBDIR 0x10
#define F_ARCHIVE 0x20

#define DOPEN_COMPAT 1
#define DOPEN_NORMAL 0

#define DP_MAXREQ  -1
#define DP_PATHMAX  2
#define DP_NAMEMAX  3
#define DP_TRUNC    5
#define DP_NOTRUNC  0
#define DP_AUTOTRUNC 1
#define DP_DOSTRUNC 2

#define S_IFMT  0170000
#define S_IFCHR 0020000
#define S_IFDIR 0040000
#define S_IFREG 0100000
#define S_IFIFO 0120000
#define S_IFLNK 0160000

typedef struct
{
    unsigned int mode;
    long         index;
    unsigned int dev;
    unsigned int reserved1;
    unsigned int nlink;
    unsigned int uid;
    unsigned int gid;
    long         size;
    long         blksize;
    long         nblocks;
    int          mtime;
    int          mdate;
    int          atime;
    int          adate;
    int          ctime;
    int          cdate;
    int          attr;
    int          reserved2;
    long         reserved3[2];
} XATTR;

#endif
