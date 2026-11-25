#ifndef VT52_GEM_H
#define VT52_GEM_H

/* Public MiNTLib/GEMlib headers for the VT52 port.  The original sources
 * include Pure C's TOS.H, VDI.H, MT_AES.H and MGX_DOS.H; keep the application
 * on public declarations while preserving its 16-bit GEM fields. */
#include <gem.h>
#include <gemx.h>
#include <mint/ostruct.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>
#include <mint/sysvars.h>
#include <mint/trap14.h>
#include <mint/errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed long int32;
typedef unsigned long uint32;

typedef unsigned char BYTE;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned long ULONG;
typedef unsigned char BOOLEAN;
typedef unsigned char boolean;

typedef _DTA DTA;

/* The legacy DTA field names used by VT52; the offsets are the public _DTA
 * layout, so keep aliases rather than defining a second disk structure. */
#define d_length dta_size
#define d_fname  dta_name

/* Mouse status returned by the AES. */
typedef struct
{
	WORD x;
	WORD y;
	WORD bstate;
	WORD kstate;
} EVNTDATA;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef SELECTED
#define SELECTED OS_SELECTED
#endif
#ifndef FA_SUBDIR
#define FA_SUBDIR FA_DIR
#endif
#ifndef E_OK
#define E_OK 0L
#endif
#ifndef ERROR
#define ERROR (-1L)
#endif
#ifndef EACCDN
#define EACCDN (-36L)
#endif

/* MagiC's 'AnKr' XBIOS service.  The public MiNTLib bindings expose the
 * XBIOS trap directly, not Pure C's varargs xbios() helper. */
static inline long vt_ankr_xbios(short code, void *arg)
{
	return trap_14_wlwl(39, 0x416e4b72L, code, (long) arg);
}

#endif
