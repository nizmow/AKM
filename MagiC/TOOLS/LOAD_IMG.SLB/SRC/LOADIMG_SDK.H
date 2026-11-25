#ifndef LOADIMG_SDK_H
#define LOADIMG_SDK_H

#define __GEMLIB_OLDNAMES 1
#include <gem.h>
#include <gemx.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>
#include <mint/ostruct.h>
#include <mint/dcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef int16_t WORD;
typedef uint16_t UWORD;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef uint8_t UBYTE;
typedef int8_t BYTE;
#define cdecl __CDECL

#define E_OK 0
#define ERROR (-1)
#define ENSMEM (-39L)

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

#endif
