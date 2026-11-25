/*
 * GCC/Atari variant of the 2B universal types header used by the VDI kernel
 * sources.  The original Pure-C header is not part of this archive; this
 * variant keeps the same scalar/pointer layout and implements the byte-swap
 * helpers with GCC builtins instead of Pure-C inline opcodes.
 */
#ifndef __2B_UNIVERSAL_TYPES__
#define __2B_UNIVERSAL_TYPES__

#include "target.h"

#if !TARGET_TOS
#error VDI kernel build must select the Atari target
#endif

#if !defined( __GNUC__ )
#error VDI kernel build expects GCC
#endif

#define INTEL			0
#define PPC				0
#define MC68K			1

/* Structure alignment: the Atari 68k ABI uses natural 16/32-bit alignment. */
#define PRAGMA_PACKED_ALIGN
#define PRAGMA_NATIVE_ALIGN
#define PRAGMA_RESET_ALIGN

#define VARIABLE_ARRAY_SIZE	1

typedef signed char		int8;
typedef unsigned char	uint8;

typedef signed short	int16;
typedef unsigned short	uint16;

typedef signed long		int32;
typedef unsigned long	uint32;

typedef signed long long	int64;
typedef unsigned long long	uint64;

typedef uint8	boolean;

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif

typedef int32	fixed32;
typedef int64	fixed64;
typedef int32	fixed2p30;
#define fixed	fixed32

#define int_to_fixed( i )			(((fixed) ( i )) << 16 )
#define double_to_fixed( f )		((fixed) (( f ) * ( 1L << 16 )))

#define fixed_to_int16( x )		((int16) (( x ) >> 16 ))
#define fixed_to_int32( x )		((int32) (( x ) >> 16 ))
#define fixed_to_double( x )		(((double) ( x )) / ( 1L << 16 ))

#define int_to_fixed64( i )		(((fixed64) ( i )) << 32 )
#define double_to_fixed64( f )	((fixed64) (( f ) * ( 1LL << 32 )))

#define fixed64_to_int16( x )	((int16) (( x ) >> 32 ))
#define fixed64_to_int32( x )	((int32) (( x ) >> 32 ))
#define fixed64_to_int64( x )	((int64) (( x ) >> 32 ))
#define fixed64_to_double( x )	(((double) ( x )) / ( 1LL << 32 ))

#define int_to_fixed2p30( i )		(((fixed2p30) ( i )) << 30 )
#define double_to_fixed2p30( f )	((fixed2p30) (( f ) * ( 1L << 30 )))

#define fixed2p30_to_int16( x )	((int16) (( x ) >> 30 ))
#define fixed2p30_to_int32( x )	((int32) (( x ) >> 30 ))
#define fixed2p30_to_double( x )	(((double) ( x )) / ( 1L << 30 ))

/* 68k is big endian. */
#define HOST_2_BENDIAN_16( a )	( a )
#define HOST_2_BENDIAN_32( a )	( a )
#define LENDIAN_2_HOST_16( a )	( REVERSEu16( a ))
#define LENDIAN_2_HOST_32( a )	( REVERSEu32( a ))
#define BENDIAN_2_HOST_16( a )	( a )
#define BENDIAN_2_HOST_32( a )	( a )

#define CHAR_CONST16( a )	( a )
#define CHAR_CONST32( a )	( a )

#define REVERSE16( a )		((int16) __builtin_bswap16( (uint16) ( a )))
#define REVERSEu16( a )		__builtin_bswap16( (uint16) ( a ))
#define REVERSE32( a )		((int32) __builtin_bswap32( (uint32) ( a )))
#define REVERSEu32( a )		__builtin_bswap32( (uint32) ( a ))
#define REVERSEptr( a )		((void *) REVERSEu32( (uint32) ( a )))

#define XBIT8( a, b )	((( a & b ) | (( a | b ) & ( a + b ))) >> 7 )
#define XBIT16( a, b )	((( a & b ) | (( a | b ) & ( a + b ))) >> 15 )
#define XBIT32( a, b )	((( a & b ) | (( a | b ) & ( a + b ))) >> 31 )

#define LROTATE8( a, shifts )	((uint8) (((uint8) ( a ) << ( shifts )) | ((uint8) ( a ) >> ( 8 - ( shifts )))))
#define LROTATE16( a, shifts )	((uint16) (((uint16) ( a ) << ( shifts )) | ((uint16) ( a ) >> ( 16 - ( shifts )))))
#define LROTATE32( a, shifts )	((uint32) (((uint32) ( a ) << ( shifts )) | ((uint32) ( a ) >> ( 32 - ( shifts )))))

#define RROTATE8( a, shifts )	((uint8) (((uint8) ( a ) >> ( shifts )) | ((uint8) ( a ) << ( 8 - ( shifts )))))
#define RROTATE16( a, shifts )	((uint16) (((uint16) ( a ) >> ( shifts )) | ((uint16) ( a ) << ( 16 - ( shifts )))))
#define RROTATE32( a, shifts )	((uint32) (((uint32) ( a ) >> ( shifts )) | ((uint32) ( a ) << ( 32 - ( shifts )))))

#define FASTCALL

#endif
