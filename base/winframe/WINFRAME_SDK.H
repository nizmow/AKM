#ifndef WINFRAME_SDK_H
#define WINFRAME_SDK_H

#define __GEMLIB_OLDNAMES 1
#include <gem.h>
#include <gemx.h>
#include <stdint.h>
#include <stddef.h>

typedef int16_t WORD;
typedef uint16_t UWORD;
typedef int32_t LONG;
typedef uint32_t ULONG;
#define cdecl __CDECL

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define E_OK 0
#define ERROR (-1)
#define EFILNF (-33L)

#endif
