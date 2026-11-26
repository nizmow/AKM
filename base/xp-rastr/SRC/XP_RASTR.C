/*
*
* "shared library" zur Expandierung einer Bitmap.
* Wird für die Ausgabe von Farbicons oder Pixelbildern
* (IMG) benötigt.
* Die im Standard-Format (Planes hintereinander) vorliegende
* Bitmap wird auf die Bildschirm-Bit-Tiefe expandiert und
* hardwareabhängig umgewandelt.
*
*
* (C) Seven Behne 1996
*
* Implementation als SLB:
*  Andreas Kromke
*  22.10.97
*
*/

#include "XPRASTR_SDK.H"
#include "FARBICON.H"

#pragma warn -par

typedef struct {
	char intern[128];
	char cmd[128];
} PD;

/* The MagiC dispatcher tail-jumps with the classic packed stack layout. */
struct __attribute__((packed, aligned(2))) slb_args {
	PD *pd;
	LONG fn;
	WORD nargs;
};

struct __attribute__((packed, aligned(2))) get_raster_args {
	struct slb_args call;
	void **xp_r_ass;
	void **xp_r_cdecl;
};

struct __attribute__((packed, aligned(2))) set_colours_args {
	struct slb_args call;
	UWORD *colour_values;
};

_Static_assert(sizeof(struct slb_args) == 10 &&
	offsetof(struct slb_args, fn) == 4 &&
	offsetof(struct slb_args, nargs) == 8, "SLB dispatch prefix");
_Static_assert(sizeof(struct get_raster_args) == 18 &&
	offsetof(struct get_raster_args, xp_r_ass) == 10 &&
	offsetof(struct get_raster_args, xp_r_cdecl) == 14, "XP_RASTR export 0");
_Static_assert(sizeof(struct set_colours_args) == 14 &&
	offsetof(struct set_colours_args, colour_values) == 10, "XP_RASTR export 1");

struct initparm {
	VDIPB *vdipb;
	WORD nplanes;
};

void *xp_tab;		/* Farbtabelle Index => Farbe */
void (*xp_ptr)( void );
static UBYTE color_remap[16] = { 0,2,3,6,4,7,5,8,9,10,11,14,12,15,13,255 };
static WORD nplanes;
VDIPB *saved_vdipb;

static void xp_colmp( VDIPB *vdipb, WORD planes,
				WORD *work_out, UWORD *colour_values );
static ULONG intensity_to_value( WORD intensity, WORD bits, WORD *bit_no );


/*****************************************************************
*
* Ab MagiC 6 erhält man den eigenen PD und optional einen
* Parameter.
*
* Rückgabe:		-1L	Wandlung nicht notwendig
*				-2L	unbekanntes Format
*				0L	OK
*
*****************************************************************/

extern LONG cdecl slb_init( PD *mypd, struct initparm *ip )
{
	WORD	work_out[272];
	WORD planes = ip->nplanes;
	LONG	len;

	saved_vdipb = ip->vdipb;


	if	( planes <= 1 )	/* nur eine Ebene? */
		return(ERROR);		/* Wandlung nicht nötig */

	xp_vq_scrninfo( ip->vdipb, work_out );

	switch( work_out[0] )		/* Pixelorganisation */
	{
		case	0:				/* Interleaved Planes? */
		{
			switch( planes )
			{
				case	4:	xp_ptr = xp_ip_4;	break;
				case	8:	xp_ptr = xp_ip_8;	break;
				default:	return(-2L);	/* unbekanntes Format */
			}
			break;
		}
		case	2:				/* Packed Pixels? */
		{
			switch( planes )
			{
				case	4:	xp_ptr = xp_pp_4;	break;
				case	8:	xp_ptr = xp_pp_8;	break;
				case	16:	xp_ptr = xp_pp_16;	break;
				case	24:	xp_ptr = xp_pp_24;	break;
				case	32:	xp_ptr = xp_pp_32;	break;
				default:	return(-2L);	/* unbekanntes Format */
			}
			break;
		}
		default:	return(-2L);	/* unbekanntes Format */
	}

	if	( planes > 8 )		/* Direct Colour? */
		{
		len = ( planes + 15 ) / 8;	/* Bytes pro Farbe */
		len *= 256;				/* Platz für 256 Farben lassen */
		xp_tab = (void *)Malloc( len );		/* Speicher f�r Expandiertabelle */
		}
	/*
	else
		xp_tab = 0L;		ist so initialisiert.
	*/

	xp_colmp( ip->vdipb, planes, work_out, (void *) 0 );

	return(E_OK);
}


/*****************************************************************
*
*****************************************************************/

extern void cdecl slb_exit( void )
{
}


/*****************************************************************
*
*****************************************************************/

extern LONG cdecl slb_open( PD *pd )
{
	return(E_OK);
}


/*****************************************************************
*
*****************************************************************/

extern void cdecl slb_close( PD *pd )
{
}


/************************************************************************
*
* Berechnet die Farbtabelle für direct colour.
* Beim ersten Aufruf ist <colour_values> Null, daher wird der VDI-
* Farbwert über vq_color berechnet. Beim Ändern der Palette werden
* 3*256 UWORDs mit den Promillewerten für r,g,b übergeben.
*
************************************************************************/

static void xp_colmp( VDIPB *vdipb, WORD planes,
				WORD *work_out, UWORD *colour_values )
{
	short *vintout;
	short local_colour[4];
	WORD	_work_out[272];
	register WORD i;
	WORD	index;
	register UWORD *wtab = (UWORD *) xp_tab;
	register ULONG *ltab = (ULONG *) xp_tab;
	register UWORD *cv;
	ULONG value;



	if	(!xp_tab)							/* Indirect Colour? */
		return;							/* ja, Ende */

	if	(!vdipb)
		vdipb = saved_vdipb;
	if	(vdipb)
		vintout = vdipb->intout;

	if	(!work_out)
		{
		work_out = _work_out;
		xp_vq_scrninfo( vdipb, work_out );
		}


	for	( i = 0; i < 256; i++ )
		{

		/* Pixelwert in VDI-Farbindex umrechnen */
		/* ------------------------------------ */

		if ( i < 16 )
			index = color_remap[i];
		else if ( i < 255 )
			index = i;
		else
			index = 1;

		/* Farbwert berechnen */
		/* ------------------ */

		if	(colour_values)
			{
			cv = colour_values+3*index;
			local_colour[1] = *cv++;
			local_colour[2] = *cv++;
			local_colour[3] = *cv;
			vintout = local_colour;
			}
		else	{
			xp_vq_color( vdipb, index, 0 );
			vintout = vdipb->intout;
			}

		value = intensity_to_value( vintout[1], work_out[8], &work_out[16] );/* Bitmuster für Rot-Intensität */
		value |= intensity_to_value( vintout[2], work_out[9], &work_out[32] );/* Bitmuster für Grün-Intensität */
		value |= intensity_to_value( vintout[3], work_out[10], &work_out[48] );/* Bitmuster für Blau-Intensität */

		/* Farbwert ablegen */
		/* ---------------- */

		if	(planes == 16)
			*wtab++ = (UWORD) value;
		else	*ltab++ = value;
		}
}


/*----------------------------------------------------------------------------------------*/ 
/* Farbintensität in Promille in Pixelwert wandeln								*/
/* Funktionsresultat:	Pixelwert für die übergebene Farbkomponente					*/
/*	intensity:		Intensität in Promille									*/
/*	bits:			Anzahl der Bits für diese Intensität						*/
/*	bit_no:			Zuordnung von Bitnummer zum Wert innerhalb des Pixels (work_out)	*/
/*----------------------------------------------------------------------------------------*/ 
static ULONG intensity_to_value( WORD intensity, WORD bits, WORD *bit_no )
{
	ULONG	value;
	ULONG	ret_val;
	WORD	bit;

	value = ( 1L << bits ) - 1;

	value = xp_wmul((UWORD) value, intensity );
	value = xp_ldiv( value, 1000 );

	ret_val = 0;

	for ( bit = 0; bit < bits; bit++ )
	{
		if ( value & ( 1L << bit ))				/* Bit gesetzt? */
			ret_val |= ( 1L << ( *bit_no ));

		bit_no++;
	}

	return( ret_val );
}


/*****************************************************************
*
* Funktion #0: Gibt Funktionszeiger für Assembler- und
* 'cdecl'-Zugriff zurück.
*
*****************************************************************/

LONG cdecl slb_fn0(struct get_raster_args args)
{
	if	(args.call.nargs >= 2)
		*args.xp_r_ass = &xp_raster;
	if	(args.call.nargs >= 4)
		*args.xp_r_cdecl = &xp_rasterC;
	return(E_OK);
}


/*****************************************************************
*
* Funktion #1: neue Farbtabelle übergeben.
*
*****************************************************************/

LONG cdecl slb_fn1(struct set_colours_args args)
{
	xp_colmp( (void *) 0L, nplanes, (void *) 0L, args.colour_values );
	return(E_OK);
}
