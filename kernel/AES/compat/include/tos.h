#ifndef AES_C_TOS_H
#define AES_C_TOS_H

#include <portab.h>

typedef struct
{
    char          d_reserved[21];
    unsigned char d_attrib;
    unsigned int  d_time;
    unsigned int  d_date;
    unsigned long d_length;
    char          d_fname[14];
} DTA;

#endif
