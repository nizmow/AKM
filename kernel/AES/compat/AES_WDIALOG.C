/* Compile the original window-dialog core with the kernel ABI. */
#ifndef cdecl
#define cdecl __attribute__((cdecl))
#endif

#include <portab.h>

/* Pure C supplies these through GEMLIB; the kernel exports the lowercase form. */
extern LONG mfree( void *block );
#define Mfree( p ) mfree( (void *) (p) )

#include "../SRC/WDIALOG/WDIALOG.C"
