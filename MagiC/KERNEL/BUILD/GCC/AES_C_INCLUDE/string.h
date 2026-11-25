#ifndef AES_C_STRING_H
#define AES_C_STRING_H

#include <stddef.h>
#include <portab.h>

LONG  strlen( const char *s );
WORD  strcmp( char *s1, char *s2 );
WORD  stricmp( const char *s1, const char *s2 );
void  strcpy( char *dst, char *src );
void  strcat( char *dst, char *src );
char *strchr( char *s, char c );
char *strrchr( char *s, char c );
void  strncpy( char *dst, char *src, UWORD maxlen );
void  memcpy( void *dst, void *src, UWORD len );
void  memmove( void *dst, void *src, ULONG len );

#endif
