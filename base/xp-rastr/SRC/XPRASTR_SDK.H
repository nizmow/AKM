#ifndef XPRASTR_SDK_H
#define XPRASTR_SDK_H

#define __GEMLIB_OLDNAMES 1
#include <gem.h>
#include <gemx.h>
#include <mint/osbind.h>
#include <stdint.h>
#include <stddef.h>

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

#endif
