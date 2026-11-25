/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

# ifndef _global_h
# define _global_h

#include <stdint.h>
#include <gem.h>
#include <gemx.h>
#include <mint/mintbind.h>
#include <mint/osbind.h>
#include <mint/cookie.h>
#include <mint/ssystem.h>
#include <fcntl.h>

#ifndef cdecl
#define cdecl __CDECL
#endif

/* MiNTLib sets this in its startup code: non-zero for an application,
 * zero for an accessory. */
extern int16_t _app;

/* COPS converts TOS dates itself instead of using the libc time package. */
typedef int32_t time_t;

struct tm
{
	int16_t tm_sec;			/* Seconds.	[0-60] (1 leap second) */
	int16_t tm_min;			/* Minutes.	[0-59] */
	int16_t tm_hour;		/* Hours.	[0-23] */
	int16_t tm_mday;		/* Day.		[1-31] */
	int16_t tm_mon;			/* Month.	[0-11] */
	int16_t tm_year;		/* Year	- 1900.  */
	int16_t tm_wday;		/* Day of week.	[0-6] */
	int16_t tm_yday;		/* Days in year.[0-365]	*/
	int16_t tm_isdst;		/* DST.		[-1/0/1]*/
};

/* MiNTLib declares Dxreaddir() but leaves the record to the caller. */
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

/* Keytbl() returns _KEYTAB; COPS only uses the three translation tables. */
typedef struct
{
	char *unshift;
	char *shift;
	char *caps;
} KEYTAB;

/* GEM mouse state in COPS' historical 16-bit record. */
typedef struct
{
	int16_t x;
	int16_t y;
	int16_t bstate;
	int16_t kstate;
} EVNTDATA;

#define graf_mkstate_evntdata(ev) \
	graf_mkstate(&(ev)->x, &(ev)->y, &(ev)->bstate, &(ev)->kstate)
#define evnt_button_evntdata(clicks, mask, state, ev) \
	evnt_button((clicks), (mask), (state), &(ev)->x, &(ev)->y, \
		    &(ev)->bstate, &(ev)->kstate)

/* TOS system header, reached through the 0x4f2 system variable. */
typedef struct _syshdr
{
	uint16_t os_entry;
	uint16_t os_version;
	void *os_start;
	struct _syshdr *os_base;
	void *os_membot;
	void *os_rsv1;
	int32_t *os_magic;
	int32_t os_gendat;
	int16_t os_conf;
	int16_t os_gendatg;
	void *_root;
	int32_t *kbshift;
	void **_run;
	void *p_rsv2;
} SYSHDR;

/* MagiC AES cookie payload. Only the version field is read here. */
typedef struct
{
	int32_t magic;
	void *membot;
	void *aes_start;
	int32_t magic2;
	int32_t date;
	void (*chgres)(int16_t res, int16_t txt);
	int32_t (**shel_vector)(void);
	char *aes_bootdrv;
	int16_t *vdi_device;
	void *reservd1;
	void *reservd2;
	void *reservd3;
	int16_t version;
	int16_t release;
} AESVARS;

typedef struct
{
	int32_t config_status;
	void *dosvars;
	AESVARS *aesvars;
	void *res1;
	void *hddrv_functions;
	int32_t status_bits;
} MAGX_COOKIE;

# define str(x)		_stringify (x)
# define _stringify(x)	#x

#ifndef min
#define min(a,b)	((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)	((a) > (b) ? (a) : (b))
#endif
#define between(x,a,b)	((x >= a) && (x <= b))
#define ABS(a)		((a) < 0 ? -(a) : (a))

#if 0
#define DEBUG(x) printf x, fflush(stdout)
#undef DEBUG
#include <mint/arch/nf_ops.h>
#define DEBUG(x) nf_debugprintf x
#if defined(__GNUC__)
#define DEBUG_FMT "%s"
#define DEBUG_ARGS __FUNCTION__
#else
#define DEBUG_FMT "%s:%d"
#define DEBUG_ARGS __FILE__, __LINE__
#endif
#else
#define DEBUG(x)
#endif

#ifndef BOOLEAN
#define BOOLEAN int
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

#ifndef __GNUC__
#define __builtin_unreachable()
#endif



# endif /* _global_h */
