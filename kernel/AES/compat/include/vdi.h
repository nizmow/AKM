#ifndef AES_C_VDI_H
#define AES_C_VDI_H

#include <portab.h>

typedef long fix31;

typedef struct
{
    int *contrl;
    int *intin;
    int *ptsin;
    int *intout;
    int *ptsout;
} VDIPB;

void vdi( VDIPB *vdipb );

#define IP_HOLLOW   0
#define IP_1PATT    1
#define IP_2PATT    2
#define IP_3PATT    3
#define IP_4PATT    4
#define IP_5PATT    5
#define IP_6PATT    6
#define IP_SOLID    7

#define FIS_HOLLOW  0
#define FIS_SOLID   1
#define FIS_PATTERN 2
#define FIS_HATCH   3
#define FIS_USER    4

#define MD_REPLACE  1
#define MD_TRANS    2
#define MD_XOR      3
#define MD_ERASE    4

#define S_AND_D     1
#define S_OR_D      7

#define SOLID       1

#endif
