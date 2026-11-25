/* TSLICE CPX public-SDK compatibility definitions. */
#ifndef TSLICE_COMPAT_H
#define TSLICE_COMPAT_H

#include <gem.h>
#include <stdint.h>

#ifndef _WORD
#define _WORD short
#endif
#ifndef LONG
#define LONG long
#endif
#ifndef BOOLEAN
typedef short BOOLEAN;
#endif

#ifndef NULLFUNC
#define NULLFUNC ((void cdecl (*)(void)) 0)
#endif
#ifndef MFSAVE
#define MFSAVE 1
#endif
#ifndef MFRESTORE
#define MFRESTORE 0
#endif
#ifndef DESK
#define DESK 0
#endif
#ifndef MAX_DEPTH
#define MAX_DEPTH 8
#endif
#ifndef FLAT_HAND
#define FLAT_HAND 4
#endif
#ifndef VERTICAL
#define VERTICAL 0
#endif
#ifndef HORIZONTAL
#define HORIZONTAL 1
#endif
#ifndef NIL
#define NIL (-1)
#endif

typedef struct {
	short x;
	short y;
	short bstate;
	short kstate;
} EVNTDATA;

/* Only ctrl_timeslice is used; the kernel structure places it at offset 100. */
typedef struct {
	unsigned char reserved[100];
	long (*ctrl_timeslice)(long settings);
} AESVARS;

typedef struct {
	int32_t config_status;
	void *dosvars;
	AESVARS *aesvars;
} MAGX_COOKIE;

#endif
