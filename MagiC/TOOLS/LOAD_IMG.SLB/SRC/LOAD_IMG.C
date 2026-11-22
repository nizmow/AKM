/*
*
* "shared library" zum Laden einer IMG-Datei.
*
*
* (C) Behne&Behne 1996
*
* Implementation als SLB:
*  Andreas Kromke
*  31.1.98
*
*/

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#endif
#include "LOADIMG_SDK.H"
#include "IMGLOAD.H"
#include "LOAD_IMG.H"

#pragma warn -par

#define MIN(a,b) ((a < b) ? a : b)
#define MAX(a,b) ((a > b) ? a : b)
#define ABS(X) ((X>0) ? X : -X)

typedef struct {
	char intern[128];
	char cmd[128];
} PD;

/* MagiC replaces the SLB handle with pd, then jumps without moving
 * the remaining stack bytes. A single by-value record keeps each WORD
 * in its legacy two-byte slot under the default GCC ABI. */
struct __attribute__((packed, aligned(2))) slb_args {
	PD *pd;
	LONG fn;
	WORD nargs;
};

struct __attribute__((packed, aligned(2))) load_args {
	struct slb_args call;
	char *path;
	img_descriptor **pimg;
};

struct __attribute__((packed, aligned(2))) free_args {
	struct slb_args call;
	img_descriptor *img;
};

struct __attribute__((packed, aligned(2))) convert_args {
	struct slb_args call;
	img_descriptor *img;
	MFDB *fill;
	WORD min_index;
	WORD max_pen;
	WORD vdi_handle;
};

struct __attribute__((packed, aligned(2))) draw_args {
	struct slb_args call;
	WORD vdi_handle;
	MFDB *fill;
	WORD x;
	WORD y;
};

struct __attribute__((packed, aligned(2))) tile_args {
	struct slb_args call;
	WORD vdi_handle;
	MFDB *fill;
	GRECT *g;
	GRECT *clipg;
};

_Static_assert(sizeof(struct slb_args) == 10 &&
	offsetof(struct slb_args, fn) == 4 &&
	offsetof(struct slb_args, nargs) == 8, "SLB dispatch prefix");
_Static_assert(sizeof(struct load_args) == 18 &&
	offsetof(struct load_args, path) == 10 &&
	offsetof(struct load_args, pimg) == 14, "LOAD_IMG export 0");
_Static_assert(sizeof(struct free_args) == 14 &&
	offsetof(struct free_args, img) == 10, "LOAD_IMG export 1");
_Static_assert(sizeof(struct convert_args) == 24 &&
	offsetof(struct convert_args, img) == 10 &&
	offsetof(struct convert_args, fill) == 14 &&
	offsetof(struct convert_args, min_index) == 18 &&
	offsetof(struct convert_args, max_pen) == 20 &&
	offsetof(struct convert_args, vdi_handle) == 22, "LOAD_IMG export 2");
_Static_assert(sizeof(struct draw_args) == 20 &&
	offsetof(struct draw_args, vdi_handle) == 10 &&
	offsetof(struct draw_args, fill) == 12 &&
	offsetof(struct draw_args, x) == 16 &&
	offsetof(struct draw_args, y) == 18, "LOAD_IMG export 3");
_Static_assert(sizeof(struct tile_args) == 24 &&
	offsetof(struct tile_args, vdi_handle) == 10 &&
	offsetof(struct tile_args, fill) == 12 &&
	offsetof(struct tile_args, g) == 16 &&
	offsetof(struct tile_args, clipg) == 20, "LOAD_IMG export 4");

struct initparm conf;

#if DEBUG
WORD errno;
#endif

/*****************************************************************
*
* Ab MagiC 6 erhält man den eigenen PD und optional einen
* Parameter.
*
*****************************************************************/

extern WORD loadimg_call_raster(void *fn, LONG words, LONG len, WORD planes, void *src, void *des);

extern LONG cdecl slb_init( PD *mypd, struct initparm *parm )
{
	conf = *parm;
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

extern LONG cdecl slb_open( PD *pd, LONG parm )
{
	return(E_OK);
}


/*****************************************************************
*
*****************************************************************/

extern void cdecl slb_close( PD *pd )
{
}


/*****************************************************************
*
* Funktion #0: IMG laden.
*
*****************************************************************/

LONG cdecl slb_fn0(struct load_args args)
{
	char *path = args.path;
	img_descriptor *img = (img_descriptor *)Malloc(sizeof(img_descriptor));

#if DEBUG
	printf("Lade %s\n", path);
	Cconin();
#endif

	*args.pimg = img;
	if	(!img)
		return(ENSMEM);
	return(load_IMG( (BYTE *)path, &img->w, &img->h,
		&img->line_width, &img->nplanes,
		&img->palette, &img->pal_entries,
		&img->buf ));
}


/*****************************************************************
*
* Funktion #1: IMG freigeben.
*
*****************************************************************/

LONG cdecl slb_fn1(struct free_args args)
{
	img_descriptor *img = args.img;

	if	(img->buf)
		Mfree(img->buf);
	if	(img->palette)
		Mfree(img->palette);
	Mfree(img);
	return(E_OK);
}


/*****************************************************************
*
* Funktion #2: IMG in Bildschirmdarstellung wandeln.
* <max_pen> ist i.a. work_out[13] - 1
*
*****************************************************************/

LONG cdecl slb_fn2(struct convert_args args)
{
	img_descriptor *img = args.img;
	MFDB *fill = args.fill;
	WORD min_index = args.min_index;
	WORD max_pen = args.max_pen;
	WORD vdi_handle = args.vdi_handle;
	WORD	index;
	WORD	pen;
	static UBYTE color_map[16] = { 0, 255, 1, 2, 4, 6, 3, 5, 7 , 8, 9, 10, 12, 14, 11, 13 };
	LONG	plane_len;
	void *exp_img;


#if DEBUG
	printf("Einstellungen:\n"
			"xp_raster = %08lx\n"
			"xp_ret = %08lx\n",
			conf.xp_raster,
			conf.xp_ret);

	printf("Wandle Bild:\n"
			"desc = %08lx\n"
			"fill = %08lx\n"
			"min_index = %d max_pen = %d vdi_handle = %d\n",
			img,
			fill,
			min_index,
			max_pen,
			vdi_handle);

	printf("Bild:\n"
		"w = %d\n"
		"h = %d\n"
		"planes = %d\n",
		img->w,img->h,img->nplanes);
	Cconin();
#endif

/*	min_index = 16;	*/

	fill->fd_w = img->w;
	fill->fd_h = img->h;
	fill->fd_wdwidth = img->line_width / 2;
	fill->fd_stand = 0;
	fill->fd_r1 = 0;
	fill->fd_r2 = 0;
	fill->fd_r3 = 0;

	if	( (img->nplanes > 1) && (conf.xp_ret >= 0) )	/* farbiges IMG? */
		{
		exp_img = (void *)Malloc((LONG) img->line_width * img->h * conf.nplanes );	/* Speicher f�r expandiertes IMG anfordern */

		if	(! exp_img )
			return(ENSMEM);

		plane_len = (LONG) img->line_width * img->h;		/* Länge einer Ebene */

		if ( loadimg_call_raster( (void *)conf.xp_raster, plane_len / 2,
						plane_len,
						img->nplanes, 
						img->buf,
						exp_img ))	/* IMG expandierbar? */
			{
			fill->fd_addr = exp_img;
			fill->fd_nplanes = conf.nplanes;
			}
		else	{
			Mfree(exp_img);
#if DEBUG
	printf("Fehler beim Wandeln?\n");
#endif
			return(ERROR);
			}
		}
	else						/* monochromes IMG */
		{
		exp_img = 0L;

		fill->fd_addr = img->buf;
		fill->fd_nplanes = img->nplanes;
		}

	if	( img->palette )				/* Palette vorhanden? */
		{

		if (( img->pal_entries - 1 ) > max_pen )	/* mehr Paletteneinträge als Farbstifte??? */
			img->pal_entries = max_pen + 1;
	
		for ( pen = 0; pen < img->pal_entries; pen++ )
			{
			if ( pen < 16 )		/* Systemfarbe? */
				index = color_map[pen] & max_pen;	/* dann Index anpassen */
			else if ( pen == 255 )
				index = 15;
			else
				index = pen;
			
			if ( pen >= min_index )	/* darf die Farbe verändert werden? */
				vs_color( vdi_handle, pen, img->palette + (index * 3) );
		}
	}
	return(E_OK);
}


/*****************************************************************
*
* Funktion #3: zeichnen
*
*****************************************************************/

LONG cdecl slb_fn3(struct draw_args args)
{
	WORD vdi_handle = args.vdi_handle;
	MFDB *fill = args.fill;
	WORD x = args.x;
	WORD y = args.y;
	WORD	r[8];				/* wird bei vro_cpyfm() benutzt */
	MFDB	screen;				/* MFDB für Bildschirm */

	screen.fd_addr = 0L;		/* Bildschirm */

	r[0] = 0;
	r[1] = 0;
	r[2] = fill->fd_w - 1;
	r[3] = fill->fd_h - 1;
	r[5] = y;
	r[7] = y + fill->fd_h - 1;
	r[4] = x;
	r[6] = x + fill->fd_w - 1;
	if	( fill->fd_nplanes == 1 )									/* monochromes IMG? */
		{
		WORD	colors[2] = { 1, 0 };

		vrt_cpyfm( vdi_handle, 1, r, fill, &screen, colors );
		}
	else	{
		vro_cpyfm( vdi_handle, 3, r, fill, &screen );
		}

	return( E_OK );
}


/****************************************************************
*
* Bestimmt die Schnittmenge zwischen zwei Rechtecken
*
****************************************************************/

static int loadimg_rc_intersect(GRECT *p1, GRECT *p2)
{
	int	tx, ty, tw, th;

	tw = MIN(p2->g_x + p2->g_w, p1->g_x + p1->g_w);
	th = MIN(p2->g_y + p2->g_h, p1->g_y + p1->g_h);
	tx = MAX(p2->g_x, p1->g_x);
	ty = MAX(p2->g_y, p1->g_y);
	p2->g_x = tx;
	p2->g_y = ty;
	p2->g_w = tw - tx;
	p2->g_h = th - ty;
	return( (tw > tx) && (th > ty) );
}


/*****************************************************************
*
* Funktion #4: Kacheln zeichnen
*
*****************************************************************/

LONG cdecl slb_fn4(struct tile_args args)
{
	WORD vdi_handle = args.vdi_handle;
	MFDB *fill = args.fill;
	GRECT *g = args.g;
	GRECT *clipg = args.clipg;
	WORD	x;
	WORD	y;
	WORD clip_rect[4];			/* Clipping-Rechteck */
	WORD	obj[4];				/* Größe des zu zeichnenden Bildbereichs */
	WORD	r[8];				/* wird be vro_cpyfm() benutzt */
	MFDB	screen;				/* MFDB für Bildschirm */


	screen.fd_addr = 0L;		/* Bildschirm */

	*(GRECT *) obj = *g;
	obj[2] += obj[0] - 1;
	obj[3] += obj[1] - 1;		/* GRECT->RECT */

	*(GRECT *) clip_rect = *g;
	/* Schneide Objekt-GRECT mit Clipping-GRECT => clip_rect*/
	loadimg_rc_intersect( clipg, (GRECT *) clip_rect );	/* Clipping-Rechteck */
	/* clip_rect GRECT -> RECT */
	clip_rect[2] += clip_rect[0] - 1;
	clip_rect[3] += clip_rect[1] - 1;
	vs_clip( vdi_handle, 1, clip_rect );	/* Zeichenoperationen auf gegebenen Bereich beschränken */


	obj[0] -= obj[0] % fill->fd_w;	/* x-Startkoordinate an Musterbreite ausrichten */
	obj[1] -= obj[1] % fill->fd_h;	/* y-Startkoordinate an Musterhöhe ausrichten */

	r[0] = 0;
	r[1] = 0;
	r[2] = fill->fd_w - 1;
	r[3] = fill->fd_h - 1;

	for ( y = obj[1]; y <= clip_rect[3]; y += fill->fd_h )
	{
		r[5] = y;
		r[7] = y + fill->fd_h - 1;

		for ( x = obj[0]; x <= clip_rect[2]; x += fill->fd_w )	
		{
			r[4] = x;
			r[6] = x + fill->fd_w - 1;

			if ( fill->fd_nplanes == 1 )									/* monochromes IMG? */
			{
				WORD	colors[2] = { 1, 0 };

				vrt_cpyfm( vdi_handle, 1, r, fill, &screen, colors );
			}
			else
				vro_cpyfm( vdi_handle, 3, r, fill, &screen );
		}
	}
	
	return( E_OK );
}
