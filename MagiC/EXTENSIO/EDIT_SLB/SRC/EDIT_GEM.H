#ifndef EDIT_GEM_H
#define EDIT_GEM_H

#include <stdint.h>
#include <stddef.h>
typedef int16_t WORD;
typedef long LONG;
_Static_assert(sizeof(LONG) == 4, "GEM long outputs must be 32-bit");
#include <gem.h>
#include <gemx.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>

extern void *memchr2(const void *s, size_t len);

#endif
