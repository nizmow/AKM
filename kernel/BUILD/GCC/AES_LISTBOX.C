/* Compile the original listbox implementation with the kernel ABI. */
#ifndef cdecl
#define cdecl __attribute__((cdecl))
#endif

#include <portab.h>

extern LONG mfree( void *block );
#define Mfree( p ) mfree( (void *) (p) )

#include "../../AES/SRC/WDIALOG/LISTBOX.C"
