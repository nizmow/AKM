/* Reconstructed FILEDIV interface for the Atari/GCC build.
   The original project header is absent from the source archive; prototypes
   below match the definitions and uses in FILEDIV.C/DRIVERS.C. */
#ifndef __FILEDIV_H__
#define __FILEDIV_H__

#include <PORTAB.H>
#include <Types2B.h>

void	*load_file( BYTE *name, int32 *length );
int32	read_file( BYTE *name, void *dest, int32 offset, int32 len );

#endif
