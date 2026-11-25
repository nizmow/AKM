/*
 * Color/bitmap definitions required by the MagiC VDI kernel headers.
 * The original Colors2B.h is not present in this archive; values and layout
 * follow the public VDI color/bitmap structures used by MAGICVDI.2.
 */
#ifndef __COLORS2B__
#define __COLORS2B__

#include "Types2B.h"
#include <stddef.h>

#define REFS_ARE_PTRS	1

typedef struct
{
	int16	red;
	int16	green;
	int16	blue;
} RGB1000;

/* Pixel format flags */
#define PX_1COMP		0x01000000L
#define PX_2COMP		0x02000000L
#define PX_3COMP		0x03000000L
#define PX_4COMP		0x04000000L

#define PX_REVERSED	0x00800000L
#define PX_xFIRST		0x00400000L
#define PX_kFIRST		0x00200000L
#define PX_aFIRST		0x00100000L

#define PX_PACKED		0x00020000L
#define PX_PLANES		0x00010000L
#define PX_IPLANES		0x00000000L

#define PX_USES1		0x00000100L
#define PX_USES2		0x00000200L
#define PX_USES3		0x00000300L
#define PX_USES4		0x00000400L
#define PX_USES8		0x00000800L
#define PX_USES15		0x00000f00L
#define PX_USES16		0x00001000L
#define PX_USES24		0x00001800L
#define PX_USES30		0x00001e00L
#define PX_USES32		0x00002000L
#define PX_USES48		0x00003000L

#define PX_1BIT		0x00000001L
#define PX_2BIT		0x00000002L
#define PX_3BIT		0x00000003L
#define PX_4BIT		0x00000004L
#define PX_8BIT		0x00000008L
#define PX_16BIT		0x00000010L
#define PX_24BIT		0x00000018L
#define PX_32BIT		0x00000020L
#define PX_48BIT		0x00000030L
#define PX_64BIT		0x00000040L

#define PX_CMPNTS		0x0f000000L
#define PX_FLAGS		0x00f00000L
#define PX_PACKING		0x00030000L
#define PX_USED		0x0000ff00L
#define PX_BITS		0x000000ffL

#define PX_ATARI1		( PX_PACKED + PX_1COMP + PX_USES1 + PX_1BIT )
#define PX_ATARI2		( PX_IPLANES + PX_1COMP + PX_USES2 + PX_2BIT )
#define PX_ATARI4		( PX_IPLANES + PX_1COMP + PX_USES4 + PX_4BIT )
#define PX_ATARI8		( PX_IPLANES + PX_1COMP + PX_USES8 + PX_8BIT )
#define PX_FALCON15		( PX_PACKED + PX_3COMP + PX_USES16 + PX_16BIT )

#define PX_MAC1		( PX_PACKED + PX_1COMP + PX_USES1 + PX_1BIT )
#define PX_MAC4		( PX_PACKED + PX_1COMP + PX_USES4 + PX_4BIT )
#define PX_MAC8		( PX_PACKED + PX_1COMP + PX_USES8 + PX_8BIT )
#define PX_MAC15		( PX_xFIRST + PX_PACKED + PX_3COMP + PX_USES15 + PX_16BIT )
#define PX_MAC32		( PX_xFIRST + PX_PACKED + PX_3COMP + PX_USES24 + PX_32BIT )

#define PX_PREF1		( PX_PACKED + PX_1COMP + PX_USES1 + PX_1BIT )
#define PX_PREF2		( PX_PACKED + PX_1COMP + PX_USES2 + PX_2BIT )
#define PX_PREF4		( PX_PACKED + PX_1COMP + PX_USES4 + PX_4BIT )
#define PX_PREF8		( PX_PACKED + PX_1COMP + PX_USES8 + PX_8BIT )
#define PX_PREF15		( PX_xFIRST + PX_PACKED + PX_3COMP + PX_USES15 + PX_16BIT )
#define PX_PREF32		( PX_xFIRST + PX_PACKED + PX_3COMP + PX_USES24 + PX_32BIT )

#define PX_PREFn( bits )	( (bits) <= 1 ? PX_PREF1 : \
							  (bits) <= 2 ? PX_PREF2 : \
							  (bits) <= 4 ? PX_PREF4 : PX_PREF8 )

enum
{
	CSPACE_UNKNOWN = 0x0000,
	CSPACE_RGB		= 0x0001,
	CSPACE_ARGB		= 0x0002,
	CSPACE_CMYK		= 0x0004,
	CSPACE_HSL		= 0x0008,
	CSPACE_YCbCr	= 0x0100,
	CSPACE_YCCK		= 0x0200,
	CSPACE_GRAY		= 0x0400,
	CSPACE_AGRAY	= 0x0800
};

typedef struct
{
	uint16	reserved;
	uint16	red;
	uint16	green;
	uint16	blue;
} COLOR_RGB;

typedef struct
{
	uint16	cyan;
	uint16	magenta;
	uint16	yellow;
	uint16	black;
} COLOR_CMYK;

typedef union
{
	COLOR_RGB	rgb;
	COLOR_CMYK	cmyk;
} COLOR_ENTRY;

/* GCC -mshort gives multi-character constants only 16 bits. */
#define COLOR_TAB_MAGIC	0x63746162L

typedef struct
{
	int32	magic;
	int32	length;
	int32	format;
	int32	reserved;

	int32	map_id;
	int32	color_space;
	int32	flags;
	int32	no_colors;

	int32	reserved1;
	int32	reserved2;
	int32	reserved3;
	int32	reserved4;

	COLOR_ENTRY	colors[VARIABLE_ARRAY_SIZE];
} COLOR_TAB;

#define sizeof_COLOR_TAB	offsetof( COLOR_TAB, colors )

typedef struct
{
	int32	magic;
	int32	length;
	int32	format;
	int32	reserved;

	int32	map_id;
	int32	color_space;
	int32	flags;
	int32	no_colors;

	int32	reserved1;
	int32	reserved2;
	int32	reserved3;
	int32	reserved4;

	COLOR_ENTRY	colors[256];
} COLOR_TAB256;

typedef void INVERSE_CTAB;

typedef COLOR_TAB		*CTAB_PTR;
typedef COLOR_TAB		*CTAB_REF;
typedef INVERSE_CTAB	*ITAB_REF;

#define CBITMAP_MAGIC	0x6362746dL

typedef struct _gcbitmap
{
	int32		magic;
	int32		length;
	int32		format;
	int32		reserved;

	uint8		*addr;
	int32		width;
	int32		bits;
	uint32	px_format;

	int32		xmin;
	int32		ymin;
	int32		xmax;
	int32		ymax;

	CTAB_REF	ctab;
	ITAB_REF	itab;
	int32		color_space;
	int32		reserved1;
} GCBITMAP;

#endif
