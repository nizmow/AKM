#ifndef GEMUTILS_SDK_H
#define GEMUTILS_SDK_H

/*
 * Public-SDK bridge for the GEMUTILS/WINDOWS sources.
 *
 * These helpers were written for Pure C, whose int is 16-bit.  Keep the
 * 16-bit AES/VDI/GEMDOS ABI here and use the supplied MiNTLib/GEMlib headers
 * for every call; do not copy the old IMPORT trees.
 */

#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>

typedef int16_t WORD;

#include <gem.h>
#include <gemx.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>

#ifndef EOS
#define EOS '\0'
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef E_OK
#define E_OK 0
#endif
#ifndef EFILNF
#define EFILNF (-33L)
#endif
#ifndef ERROR
#define ERROR (-1)
#endif
#ifndef EBREAK
#define EBREAK (-128)
#endif
#ifndef RMODE_RD
#define RMODE_RD 0
#endif
#ifndef NORMAL
#define NORMAL OS_NORMAL
#endif
#ifndef SELECTED
#define SELECTED OS_SELECTED
#endif
#ifndef DISABLED
#define DISABLED OS_DISABLED
#endif
#ifndef HIDETREE
#define HIDETREE OF_HIDETREE
#endif
#ifndef EDITABLE
#define EDITABLE OF_EDITABLE
#endif
#ifndef LASTOB
#define LASTOB OF_LASTOB
#endif
#ifndef WHITE
#define WHITE 0
#endif

/*
 * MiNT file attributes returned by Dxreaddir()/Fxattr().
 * The kernel record is 52 bytes with 16-bit status/timestamp fields; native
 * GCC ints here would shift every field after mode.
 */
typedef struct
{
	uint16_t mode;
	int32_t index;
	uint16_t dev;
	uint16_t reserved1;
	uint16_t nlink;
	uint16_t uid;
	uint16_t gid;
	int32_t size;
	int32_t blksize;
	int32_t nblocks;
	uint16_t mtime;
	uint16_t mdate;
	uint16_t atime;
	uint16_t adate;
	uint16_t ctime;
	uint16_t cdate;
	uint16_t attr;
	uint16_t reserved2;
	int32_t reserved3[2];
} XATTR;

_Static_assert(sizeof(XATTR) == 52, "MiNT XATTR record must stay 52 bytes");

#endif
