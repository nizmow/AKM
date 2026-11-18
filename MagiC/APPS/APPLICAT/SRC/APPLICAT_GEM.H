#ifndef APPLICAT_GEM_H
#define APPLICAT_GEM_H

/* Use GEMlib's published legacy constant names, not archived Pure C headers. */
#define __GEMLIB_OLDNAMES 1
#include <gem.h>
#include <gemx.h>
#include <mint/mintbind.h>
#include <mint/osbind.h>
#include <mint/sysbind.h>
#include <stdint.h>
#include <stddef.h>

typedef int16_t WORD;
typedef uint16_t UWORD;
typedef int32_t LONG;
typedef uint32_t ULONG;
#define cdecl __CDECL

/* Private mouse-event tuple used by the application, not an AES parameter block. */
typedef struct {
    short x, y, bstate, kstate;
} EVNTDATA;

#define PDOM_MINT 1

/* Negative GEMDOS results, not positive C errno values. */
#define E_OK 0
#define EFILNF (-33L)
#define EPTHNF (-34L)
#define EACCDN (-36L)
#define ENSMEM (-39L)
#define EIMBA (-40L)
#define EINVFN (-32L)
#define EREADF (-11L)
#define EWRITF (-10L)
#define EDRIVE (-15L)

#endif
