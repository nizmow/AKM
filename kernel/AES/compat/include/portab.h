#ifndef AES_C_PORTAB_H
#define AES_C_PORTAB_H

typedef void VOID;
typedef char BYTE;
typedef int WORD;
typedef long LONG;
typedef unsigned char UBYTE;
typedef unsigned int UWORD;
typedef unsigned long ULONG;

#ifndef TRUE
typedef enum { FALSE = 0, TRUE = 1 } BOOLEAN;
#endif

#ifndef cdecl
#define cdecl __attribute__((cdecl))
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifndef EOS
#define EOS '\0'
#endif

#define EXTERN extern

#endif
