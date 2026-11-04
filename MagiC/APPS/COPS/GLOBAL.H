/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

# ifndef _global_h
# define _global_h

/*
# include <sys/types.h>
# include <mint/mintbind.h>
*/

# define str(x)		_stringify (x)
# define _stringify(x)	#x

#define min(a,b)	((a) < (b) ? (a) : (b))
#define max(a,b)	((a) > (b) ? (a) : (b))
#define between(x,a,b)	((x >= a) && (x <= b))
#define ABS(a)		((a) < 0 ? -(a) : (a))

#if 0
#define DEBUG(x) printf x, fflush(stdout)
#undef DEBUG
#include <mint/arch/nf_ops.h>
#define DEBUG(x) nf_debugprintf x
#if defined(__GNUC__)
#define DEBUG_FMT "%s"
#define DEBUG_ARGS __FUNCTION__
#else
#define DEBUG_FMT "%s:%d"
#define DEBUG_ARGS __FILE__, __LINE__
#endif
#else
#define DEBUG(x)
#endif

#ifndef BOOLEAN
#define BOOLEAN int
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

#ifndef __GNUC__
#define __builtin_unreachable()
#endif

/* gemlib and mintlib compatibility */

#define aes_global _GemParBlk.global
#define C_MagX 0x4D616758L     /* MagX */
typedef int32_t time_t;

#define S_FLUSHCACHE 22
#define Ssystem(a,b,c) gemdos(0x154, (int16_t) (a), (int32_t) (b), (int32_t) (c)) /* MiNT only */

struct tm
{
  int16_t tm_sec;			/* Seconds.	[0-60] (1 leap second) */
  int16_t tm_min;			/* Minutes.	[0-59] */
  int16_t tm_hour;			/* Hours.	[0-23] */
  int16_t tm_mday;			/* Day.		[1-31] */
  int16_t tm_mon;			/* Month.	[0-11] */
  int16_t tm_year;			/* Year	- 1900.  */
  int16_t tm_wday;			/* Day of week.	[0-6] */
  int16_t tm_yday;			/* Days in year.[0-365]	*/
  int16_t tm_isdst;			/* DST.		[-1/0/1]*/
};

/** parameters for the init callback function (7th parameter of mt_xfrm_popup() )
 */
struct POPUP_INIT_args
{
	OBJECT *tree;		/**< address of the object tree */
	int16_t scrollpos;	/**< scroll position */
	int16_t nlines;		/**< number of lines */
	void *param;		/**< user-defined parameter */
};

/** parameters of HNDL_OBJ callback functions */
struct HNDL_OBJ_args 
{
	DIALOG *dialog;	/**< Pointer to a dialog structure. One should not access the structure directly; the wdlg_xx functions should be used! */
	EVNT *events;	/**< If obj is an object number (>=0), then events points to the EVNT structure that was passed by wdlg_evnt; otherwise events is basically 0L and can not be used for addressing */
	int16_t obj;		/**< Object number (>=0) or function number (<0) */
	int16_t clicks;	/**< Number of mouse clicks (if obj is an object number) */
	void *data;		/**< If the parameter obj is a positive object number, then the variable user_data from wdlg_create() function will be passed here; otherwise the value depends on the corresponding function number */
};

/** 16b rectangle */
typedef struct			/* Rectangle with 16-Bit-coordinates */
{
	int16_t	x1;			/**< X coordinate of the top left corner */
	int16_t	y1;			/**< Y coordinate of the top left corner */
	int16_t	x2;			/**< X coordinate of the bottom right corner + 1 */
	int16_t	y2;			/**< Y coordinate of the bottom right corner + 1 */
} RECT16;

#define udef_vswr_mode vswr_mode
#define udef_vsf_color vsf_color
#define udef_vsl_color vsl_color
#define udef_vsl_type vsl_type
#define udef_vst_color vst_color
#define udef_vst_font vst_font
#define udef_vst_effects vst_effects
#define udef_vst_alignment vst_alignment
#define udef_vst_height vst_height
#define udef_v_gtext v_gtext
#define udef_v_pline v_pline
#define udef_vs_clip vs_clip
#define udef_vr_recfl vr_recfl
#define udef_vrt_cpyfm vrt_cpyfm

# endif /* _global_h */
