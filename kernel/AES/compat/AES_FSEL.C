/* Compile the original MagiC file selector with the kernel ABI. */
#ifndef cdecl
#define cdecl __attribute__((cdecl))
#endif

#include "../SRC/FSEL/FSEL.C"
