#ifndef MGNOTICE_GEM_H
#define MGNOTICE_GEM_H

#include "GEMUT_MT_SDK.H"
#include <mint/ostruct.h>
#include <mint/slb.h>
#include "GEMUT_MT.H"

/* MiNTLib names the Fsfirst/Fsnext record _DTA. */
typedef _DTA DTA;

#define RMODE_RD S_READ

/* TEDINFO.te_color bits 4..7 carry the text colour. */
#define TED_TEXTCOLOR(ted) ((short) (((ted)->te_color >> 4) & 0x0f))

#endif
