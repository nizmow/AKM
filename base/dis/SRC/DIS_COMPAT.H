/* Public-SDK replacements for the Pure C-era DIS build.
 *
 * The DIS sources were written for 16-bit int and the Borland TOS
 * declarations. MiNTLib uses 32-bit int, so file-format records and
 * machine words must stay explicitly 16/32-bit.
 */
#ifndef DIS_COMPAT_H
#define DIS_COMPAT_H

#include <stdint.h>
#include <mint/osbind.h>
#include <mint/ostruct.h>

#define RMODE_RD 0
#define HDL_CON (-1)

/* Fsetdta/Fsfirst use MiNTLib's _DTA field names. */
typedef _DTA DTA;
#define d_length dta_size
#define d_fname dta_name

/* GEMDOS program header: 28 bytes, 16-bit branch/flag fields. */
typedef struct __attribute__((packed)) {
	int16_t ph_branch;
	int32_t ph_tlen;
	int32_t ph_dlen;
	int32_t ph_blen;
	int32_t ph_slen;
	int32_t ph_res1;
	int32_t ph_res2;
	int16_t ph_flag;
} PH;

/* 14-byte GEMDOS object-file symbol record. */
struct symbol {
	char symname[8];
	int16_t symtype;
	int32_t symvalue;
} __attribute__((packed));

_Static_assert(sizeof(PH) == 28, "GEMDOS program header must be 28 bytes");
_Static_assert(sizeof(struct symbol) == 14, "GEMDOS symbol must be 14 bytes");

#endif
