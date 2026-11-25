/* Compile the original font selector, including VDI_BIND.C and OBJ_TOOL.C. */
#ifndef cdecl
#define cdecl __attribute__((cdecl))
#endif

#include <portab.h>

extern LONG mfree( void *block );
#define Mfree( p ) mfree( (void *) (p) )


#include "../SRC/WDIALOG/FNT_MENU.C"
