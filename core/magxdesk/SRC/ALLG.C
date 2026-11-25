/*********************************************************************
*
* Dieses Modul enthält allgemeine Routinen.
*
*********************************************************************/

#include "MAGXDESK_GEM.H"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include "K.H"
#include "PATTERN.H"


#ifndef FL3DMASK
#define FL3DMASK 0x0600
#endif
/* #include <stdio.h> */

static char timesep = ':';	/* FRG => ':' */
static char kilosep;
static void *date2str;

static char *uint_to_dec(unsigned long value, char *buffer)
{
	char digits[10];
	int count = 0;

	do
	{
		digits[count++] = (char) ('0' + value % 10);
		value /= 10;
	}
	while (value);

	while (count)
		*buffer++ = digits[--count];
	*buffer = '\0';
	return buffer;
}

/****************************************************************
*
* Initialisierung.
* Ermittelt die Systemroutine zur Umwandlung des DOS-Datums
* in eine Zeichenkette.
*
****************************************************************/

void allg_init(void)
{
	date2str = (void *) (uintptr_t) trap_14_wlwl(39, 0x416e4b72L, 5, 0L);
	kilosep = tree_datinf[FI_SIZE].ob_spec.free_string[1];
}


/****************************************************************
*
* Bringt alle eigenen Fenster nach vorn, die von anderen
* überdeckt sind.
*
****************************************************************/

void top_all_my_windows(void)
{
	wind_set_int(0, 22367, ap_id);
	if (menu_bar(NULL, -1) != ap_id)
		menu_bar(tree_menu, TRUE);
}


/****************************************************************
*
* AES-Funktion für die erweiterte Rubberbox
*
****************************************************************/

void xgrf_rbox(GRECT *clip, GRECT *box)
{
	AESPB pb;
	short control[5] = { 132, 8, 0, 0, 0 };
	short intin[8];
	short intout[1];
	long addrin[1];
	long addrout[1];

	intin[0] = clip->g_x;
	intin[1] = clip->g_y;
	intin[2] = clip->g_w;
	intin[3] = clip->g_h;
	intin[4] = box->g_x;
	intin[5] = box->g_y;
	intin[6] = box->g_w;
	intin[7] = box->g_h;

	pb.control = control;
	pb.global = aes_global;
	pb.intin = intin;
	pb.intout = intout;
	pb.addrin = addrin;
	pb.addrout = addrout;
	aes(&pb);
}


/****************************************************************
*
* Gibt Fensterhandle des obersten Fensters zurück.
*
****************************************************************/

int top_whdl(void)
{
	short whdl;

	if (!wind_get_int(0, WF_TOP, &whdl))
		return -1;
	if (whdl < 0)
		return -1;
	return whdl;
}


/****************************************************************
*
* Bestimmt die Schnittmenge zwischen zwei Rechtecken
*
****************************************************************/




/****************************************************************
*
* Bestimmt das GRECT eines Objekts
*
****************************************************************/

void objc_grect(OBJECT *tree, int objn, GRECT *g)
{
	objc_offset(tree, objn, &(g->g_x), &(g->g_y));
	tree += objn;
	g->g_w = tree->ob_width;
	g->g_h = tree->ob_height;
/*
	if (tree->ob_type == G_FTEXT)
	{
		int thickness;

		thickness = tree->ob_spec.tedinfo->te_thickness;
		if (thickness < 0)
		{
			g->g_x += thickness;
			g->g_y += thickness;
			thickness <<= 1;
			g->g_w -= thickness;
			g->g_h -= thickness;
		}
	}
*/
}


/****************************************************************
*
* Bestimmt die Begrenzung eines Objekts
*
****************************************************************/

void objc_visgrect(OBJECT *tree, int objn, GRECT *g)
{
	OBJECT *o;
	int x, y;
	short nx, ny;

	o = tree + objn;
	objc_offset(tree, objn, &nx, &ny);
	if (((o->ob_type == G_BUTTON) || (o->ob_type == G_FTEXT)) &&
		 (o-> ob_flags & FL3DMASK))
	{
		x = o->ob_x;
		y = o->ob_y;
		form_center_grect(o, g);
		g->g_x += nx - o->ob_x;
		g->g_y += ny - o->ob_y;
		o->ob_x = x;
		o->ob_y = y;
	}
	else
	{
		g->g_x = nx;
		g->g_y = ny;
		g->g_w = o->ob_width;
		g->g_h = o->ob_height;
	}
}


/****************************************************************
*
* Stellt fest, ob sich ein Koordinatenpunkt innerhalb eines
* GRECT befindet.
*
****************************************************************/

int in_grect(int x, int y, GRECT *g)
{
	return (x >= (g->g_x) && x < (g->g_x+g->g_w) &&
		  y >= (g->g_y) && y < (g->g_y+g->g_h));
}


/****************************************************************
*
* Malt ein Unterobjekt eines Fensterdialogs
*
****************************************************************/

void subobj_wdraw(DIALOG *d, int obj, int startob, int depth)
{
	GRECT g;
	OBJECT *tree;


	wdlg_get_tree(d, &tree, &g);
	objc_visgrect(tree, obj, &g);
	wdlg_redraw(d, &g, startob, depth);
}


/****************************************************************
*
* Malt ein Unterobjekt. Wenn es sich um eine Zahl handelt, wird
* sie ausgegeben, sonst ggf. der String
*
****************************************************************/

void subobj_draw(OBJECT *tree, int obj, int n, char *s)
{
	GRECT g;
	char *z;

	z = tree[obj].ob_spec.free_string;
	if (s)
		strcpy(z,s);
	else
	if (n >= 0)
		uint_to_dec((unsigned long) n, z);
	objc_grect(tree, obj, &g);
	objc_draw_grect(tree, 0, MAX_DEPTH, &g);
}


/****************************************************************
*
* Rechnet Mausposition in Objektnummer um.
* (ähnlich objc_find, jedoch nur eine Ebene).
* Rückgabe  0, falls kein Objekt unter dem Mauszeiger,
* Rückgabe -1, falls inaktiver Hintergrund.
*
****************************************************************/

int find_obj(OBJECT *tree, int x, int y)
{
	register int i,obx,oby,obw,obh;
	register ICONBLK *icn;
	int end;


	if ((i = (tree->ob_head)) < 0)
		return 0;
	if ((tree->ob_flags) & HIDETREE)
		return -1;			/* etwa inaktiver Hintergrund */
	end = tree->ob_tail;
	x -= tree->ob_x;			/* Position der Root abziehen */
	y -= tree->ob_y;
	for	(; i <= end; i++)
	{
		tree++;
		if ((tree->ob_flags) & HIDETREE)
			continue;
		/* 1. Fall: ICON */
		if ((tree->ob_type == G_ICON) || (tree->ob_type == G_CICON))
		{
			icn = tree->ob_spec.iconblk;
			/* 1. Fall: Icon selbst */
			obx = tree->ob_x + icn->ib_xicon;
			oby = tree->ob_y + icn->ib_yicon;
			obw = icn->ib_wicon;
			obh = icn->ib_hicon;
			if (x >= obx && x < obx+obw && y >= oby && y < oby+obh)
				return i;
			/* 2. Fall: Unterschrift */
			obx = tree->ob_x + icn->ib_xtext;
			oby = tree->ob_y + icn->ib_ytext;
			obw = icn->ib_wtext;
			obh = icn->ib_htext;
			if (x >= obx && x < obx+obw && y >= oby && y < oby+obh)
				return i;
		}
		/* 2. Fall: sonst (STRING bzw. G_USERDEF) */
		else
		{
			if (x >= tree->ob_x && x < tree->ob_x+tree->ob_width &&
				 y >= tree->ob_y && y < tree->ob_y+tree->ob_height)
				return i;
		}
	}

	return 0;
}


/********** Objekte verstecken *********/

void objs_hide(OBJECT *tree, ...)
{
	va_list argpoint;
	int objnr;

	va_start(argpoint, tree);
	do
	{
		objnr = va_arg(argpoint, int);
		if (objnr == 0)
			break;
		tree[objnr].ob_flags |= HIDETREE;
	}
	while (TRUE);
	va_end(argpoint);
}

/********** Objekte hervorholen *********/

void objs_unhide(OBJECT *tree, ...)
{
	va_list argpoint;
	int objnr;

	va_start(argpoint, tree);
	do
	{
		objnr = va_arg(argpoint, int);
		if (objnr == 0)
			break;
		tree[objnr].ob_flags &= ~HIDETREE;
	}
	while (TRUE);
	va_end(argpoint);
}

/********** Objekte EDITABLE machen *********/

void objs_editable(OBJECT *tree, ...)
{
	va_list argpoint;
	int objnr;

	va_start(argpoint, tree);
	for	(;;)
	{
		objnr = va_arg(argpoint, int);
		if (objnr == 0)
			break;
		tree[objnr].ob_flags |= EDITABLE;
	}
	va_end(argpoint);
}

/********** Objekte nicht-EDITABLE machen *********/

void objs_uneditable(OBJECT *tree, ...)
{
	va_list argpoint;
	int objnr;

	va_start(argpoint, tree);
	for	(;;)
	{
		objnr = va_arg(argpoint, int);
		if (objnr == 0)
			break;
		tree[objnr].ob_flags &= ~EDITABLE;
	}
	va_end(argpoint);
}

/********** Objekte DISABLED machen *********/

void objs_disable(OBJECT *tree, ...)
{
	register OBJECT *t;
	va_list argpoint;
	int objnr;
	unsigned short color;

	va_start(argpoint, tree);
	for	(;;)
	{
		objnr = va_arg(argpoint, int);
		if (objnr == 0)
			break;
		t = tree + objnr;
		/* mind. 16 Farben: G_TEXT Dunkelgrau auf Hellgrau */
		if ((t->ob_type == G_FTEXT) && (aes_global[10] >= 4))
		{
			color = t->ob_spec.tedinfo->te_color;
			color &= (unsigned short) ~((0x0f << 8) | (1 << 7) | 0x0f);
			color |= (unsigned short) ((LBLACK << 8) | LWHITE);
			t->ob_spec.tedinfo->te_color = color;
		}
		else
			t->ob_state |= DISABLED;
	}
	va_end(argpoint);
}


/********** Objekt selektiert? *********/

int selected(OBJECT *tree, int which)

{ return (tree[which].ob_state & SELECTED) ? 1 : 0; }

/******* Objekt deselektieren **********/

void ob_dsel(OBJECT *tree, int which)

{ tree[which].ob_state &= ~SELECTED; }

/******* Objekt selektieren/deselektieren **********/

void ob_sel_dsel(OBJECT *tree, int which, int sel)

{
	if (sel)
		tree[which].ob_state |=  SELECTED;
	else
		tree[which].ob_state &= ~SELECTED;
}

/***** Objekt selektieren **************/

void ob_sel(OBJECT *tree, int which)

{ tree[which].ob_state |= SELECTED; }

/***************************************/


int do_dialog(OBJECT *dialog)
{
	GRECT cg;
	short exitbutton;
	short dummy;
	void *flyinf;
	void **p_flyinf;


	flyinf = NULL;
/*	p_flyinf = (status.use_fldl) ? &flyinf : NULL;	*/
	p_flyinf = &flyinf;
	form_center_grect(dialog, &cg);
	form_xdial_grect(FMD_START, NULL, &cg, p_flyinf);
	objc_draw_grect(dialog, ROOT, MAX_DEPTH, &cg);
	exitbutton = 0x7f & form_xdo(dialog, 0, &dummy, NULL, flyinf);
	form_xdial_grect(FMD_FINISH, NULL, &cg, p_flyinf);
	ob_dsel(dialog, exitbutton);

	return exitbutton;
}

int do_exdialog(OBJECT *dialog,
			 int (*check)(OBJECT *dialog, int exitbutton),
			 int *was_redraw)
{
	GRECT cg;
	short exitbutton;
	short dummy;
	void *flyinf;
	void **p_flyinf;


	flyinf = NULL;
/*	p_flyinf = (status.use_fldl) ? &flyinf : NULL;	*/
	p_flyinf = &flyinf;
	form_center_grect(dialog, &cg);
	form_xdial_grect(FMD_START, NULL, &cg, p_flyinf);
	objc_draw_grect(dialog, ROOT, MAX_DEPTH, &cg);
	for	(;;)
	{
		exitbutton = 0x7f & form_xdo(dialog, 0, &dummy, NULL, flyinf);
		ob_dsel(dialog, exitbutton);
		if ((*check)(dialog, exitbutton))
			break;
		objc_draw_grect(dialog, exitbutton, 1, &cg);
	}

	form_xdial_grect(FMD_FINISH, NULL, &cg, p_flyinf);
	if (was_redraw != NULL)
		*was_redraw = (flyinf == NULL);	/* Rückgabe: Bildschirm zerstört */

	return exitbutton;
}


/****************************************************************
*
* Gibt zu einem DOS- Fehler den entsprechenden Text
*
****************************************************************/

char *err_file;

long err_alert(long e)
{
	form_xerr(e, err_file);
	err_file = NULL;
	return e;
}


/*********************************************************************
*
* Holt eine Zeichenkette aus der RSC-Datei.
*
*********************************************************************/

char *Rgetstring(WORD string_id)
{
	char *alert;

	rsrc_gaddr(R_STRING, string_id, &alert);
	return alert;
}


/*********************************************************************
*
* Führt einen Alert aus der RSC-Datei durch.
*
*********************************************************************/

WORD Rform_alert(WORD defbutton, WORD alert_id)
{
	return form_alert(defbutton, Rgetstring(alert_id));
}


/*********************************************************************
*
* Führt einen Alert aus der RSC-Datei durch.
* Der Alertstring enthält einen Dateinamen.
*
*********************************************************************/

WORD Rxform_alert(WORD defbutton, WORD alert_id,
				char drv, char *path)
{
	register char *alert;
	char buf[256];
	register char *t;


	alert = Rgetstring(alert_id);
	t = buf;
	while (*alert)
	{
		if (*alert == '%')
		{
			alert++;
			if (*alert == 'c')
			{
				*t++ = drv;
				alert++;
				continue;
			}
			if (*alert == 's')
			{
				while (*path)
					*t++ = *path++;
				alert++;
				continue;
			}
		}
		*t++ = *alert++;
	}
	*t = EOS;

	return form_alert(defbutton, buf);
}

#if 0
/*********************************************************************
*
* Dxreaddir()
*
* Beim Fxattr werden Symlinks nicht verfolgt.
* <xr> enthält nach dem Aufruf den Fehlercode von Fxattr.
*
*********************************************************************/

long Dxreaddir(int len, long dirhandle,
			char *buf, XATTR *xattr, long *xr)
{
	return gemdos(0x142, len, dirhandle, buf, xattr, xr);
}


/*********************************************************************
*
* Dreadlabel()
* Dwritelabel()
*
*********************************************************************/

long Dreadlabel(char *path, char *buf, int len)
{
	return gemdos(0x152, path, buf, len);
}
long Dwritelabel(char *path, char *name)
{
	return gemdos(0x153, path, name);
}
#endif

/*********************************************************************
*
* Ermittelt zu einem vollen Pfadnamen den Zeiger auf den
* reinen Dateinamen
*
*********************************************************************/

char *get_name(char *path)
{
	register char *n;

	n = strrchr(path, '\\');
	if (!n)
	{
		if ((*path) && (path[1] == ':'))
			path += 2;
		return path;
	}

	return (n + 1);
}


/*********************************************************************
*
* Ermittelt zu einem vollen Pfadnamen den zugehörigen
* Applikationsnamen (8 Zeichen, rechts Leerstellen).
*
*********************************************************************/

void get_app_name(char *path, char apname[9])
{
	char *name,*apnamp;
	register int i;

	name = get_name(path);
	apnamp = apname;
	for	(i = 0; i < 8; i++)
	{
		if (*name == '.' || *name == EOS)
			*apnamp++ = ' ';
		else
			*apnamp++ = mgx_toupper((unsigned char) *name++);
	}
	*apnamp = EOS;
}


/*********************************************************************
*
* Kürzt einen Pfadnamen sinnvoll ab.
* Der Pfad muß mit X:\ beginnen.
* Er wird, wenn er zu lang ist, auf
*
*	X:\...\lastdirs
*
* gekürzt.
*
*********************************************************************/

void abbrev_path(char *dst, char *src, int len )
{
	register char *t,*u;
	int l;

	if ((l = (int) strlen(src)) < len)
	{
		strcpy(dst, src);
		return;
	}
	
	u = t = src + l - len + 6;
	while ((*t) && (*t != '\\'))
		t++;
	*dst++ = *src++;	/* "X:\" */
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = '.';
	*dst++ = '.';
	*dst++ = '.';
	if (!(*t) || !(t[1]))
		strcpy(dst, u);
	else
		strcpy(dst, t);
}


/*********************************************************************
*
* Schaltet Maus ein/aus/Pfeil/Biene
*
*********************************************************************/

void Mgraf_mouse(int type)
{
	static int last = ARROW;

	if (type == 0x1000)
		type = last;
	graf_mouse(ABS(type), NULL);
	if (type >= 0 && type != 0x1000 && type != M_ON && type != M_OFF)
		last = type;
}


/*********************************************************************
*
* Wandelt DOS- Datum in eine Zeichenkette um.
*
*********************************************************************/

void date_to_str(char *s, unsigned int date)
{
	/* Kernel Pure C ABI: A0/D0 in; D0-D2/A0-A1 destroyed.
	 * Declaring D2 clobbered makes GCC preserve its callee-saved value. */
	register char *buffer __asm__("a0") = s;
	register unsigned long value __asm__("d0") = (unsigned short) date;
	register void *routine __asm__("a1") = date2str;

	__asm__ volatile ("jsr (%2)"
		: "+a" (buffer), "+d" (value), "+a" (routine)
		:
		: "d1", "d2", "cc", "memory");
}


/*********************************************************************
*
* Wandelt DOS- Zeit in eine Zeichenkette um.
*
*********************************************************************/

void time_to_str(char *s, unsigned int time)
{
	int min,sec;

	sec = 2 * (time & 31);
	time >>= 5;
	min = time & 63;
	time >>= 6;
	*s++ = time/10 + '0';
	*s++ = time%10 + '0';
	*s++ = timesep;
	*s++ = min/10 + '0';
	*s++ = min%10 + '0';
	*s++ = timesep;
	*s++ = sec/10 + '0';
	*s++ = sec%10 + '0';
	*s = '\0';
}


/*********************************************************************
*
* Setzt einen Laufwerk-Buchstaben in eine Schablone ein.
*
*********************************************************************/

void drv_to_str(char *s, char c)
{
	while (*s)
	{
		if (s[1] == ':')		/* nächstes Zeichen ist ':' */
		{
			*s = c;
			break;
		}
		s++;
	}
}


/*********************************************************************
*
* Wandelt 32-bit unsigned dezimal in Text, mit Tausender-Trenner.
* Setzt Zeiger hinter die Zeichenkette.
*
*********************************************************************/

char *print_ul(unsigned long z, char *p)
{
	register char *s;
	register int len;
	char hilfs[20];


	uint_to_dec(z, hilfs);
	s = hilfs;
	*p++ = *s++;
	len = (int) strlen(s);
	while (len > 0)
	{
		if (len % 3 == 0)
			*p++ = kilosep;
		*p++ = *s++;
		len--;
	}

	*p = '\0';
	return p;
}


/*********************************************************************
*
* Wandelt 64-bit unsigned dezimal in Text mit Tausender-Trenner.
* Fügt ggf. ein "k" dahinter, d.h. geht bis 4096 GB.
* '#' bei Überlauf.
*
*********************************************************************/

char *print_ull(unsigned long z[2], char *p)
{
	char c = '\0';
	ULONG l = z[1];	/* l = untere 32 Bit */

/*
	printf("%08lx %08lx\n", z[0], z[1]);
*/
	if (z[0])		/* Zahl > 32 Bit */
	{
		if (z[0] >= 1024L)
		{
			c = '#';		/* Überlauf */
			goto err;
		}

		l >>= 10L;			/* durch 1024 teilen */
		l |= (z[0] << 22L);		/* obere 10 Bit ODERn */
		c = 'k';
	}

	p = print_ul(l, p);

	err:
	if (c)
	{
		*p++ = c;
		*p++ = '\0';
	}

	return p;
}


#define STR_END(p) \
    while (*p) (p)++;


/*********************************************************************
*
* Wandelt "64-bit unsigned" dezimal in Text, und zwar für große
* Festplatten-Kapazitäten.
*
* z >= 1 G	=> n,m G
* 
* Setzt Zeiger hinter die Zeichenkette
*
*********************************************************************/

char *print_big_bytes(unsigned long z[2], char *p)
{
	/*
	 * z[1] ist ausreichend für 4 Gbyte, bei größeren
	 * Zahlen wird z[0] benötigt
	 */

	register long zh = z[0];
	register long zl = z[1];

	char suffix = '\0';

	if (zh >= 0x40000000l)
	{
		/* overflow more than 62 bits */
		*p++ = '#';
	}
	else
	if (zh >= 0x40000l)
	{
		/* one petabyte or more, 2^50  */
		/* get terabytes, i.e. 2^40 bits */
		zl = zh >> 8;
		uint_to_dec((unsigned long) (zl >> 10), p);
		zl &= 0x3ff;     /* remove P */
		suffix = 'P';
	}
	else
	if (zh >= 0x100l)
	{
		/* one terabyte or more, 2^40  */
		/* get gigabytes, i.e. 2^30 bits */
		zl >>= 30;
		uint_to_dec((unsigned long) (zh >> 8), p);
		zh &= 0xff;     /* remove T */
		zl |= (zh << 2);
		suffix = 'T';
	}
	else
	if ((zh > 0) || zl >= 0x40000000l)
	{
		/* one gigabyte or more, 2^30  */
		/* get megabytes, i.e. 2^20 bits */
		zl >>= 20;
		zl |= (zh << 12);
		uint_to_dec((unsigned long) (zl >> 10), p);
		zl &= 0x3ff;     /* remove G */
		suffix = 'G';
	}
	else
	if (zl >= 0x00100000l)
	{
		/* one megabyte or more, 2^20  */
		/* get kilobytes, i.e. 2^10 bits */
		zl >>= 10;
		uint_to_dec((unsigned long) (zl >> 10), p);
		zl &= 0x3ff;     /* remove M */
		suffix = 'M';
	}
	else
	if (zl >= 0x00000400l)
	{
		/* one kilobyte or more, 2^10  */
		/* get bytes, i.e. 2^0 bits */
		uint_to_dec((unsigned long) (zl >> 10), p);
		zl &= 0x3ff;     /* remove k */
		suffix = 'k';
	}
	else
	{
		uint_to_dec((unsigned long) zl, p);
		suffix = 'B';
	}
	
	STR_END(p);
	if (zl >= 102)
	{
		*p++ = ',';
		uint_to_dec((unsigned long) ((zl * 10) >> 10), p);
		STR_END(p);
	}
	*p++ = suffix;
	*p = '\0';
	
	return p;
}


/*********************************************************************
*
* Rechnet Dateinamen ins interne Format und zurück
*
*********************************************************************/

void fname_int(char *s, char *d)
{
	register char *p = d;

	while (*s && *s != '.')			/* Name */
		*p++ = *s++;
	if (*s)						/* Punkt */
		s++;
	if (*s)						/* Extension */
	{
		while (p < d+8)
			*p++ = ' ';
		while (*s)
			*p++ = *s++;
	}
	*p = '\0';
}

void fname_ext(char *s, char *d)
{
	register char *p = s;

	while (*p && p < s+8)
	{
		if (*p != ' ')
			*d++ = *p++;
		else
			p++;
	}

	if (*p)
		*d++ = '.';
	while (*p)
		*d++ = *p++;
	*d = '\0';
}


/*********************************************************************
*
* Berechnet den Dateityp, und zwar:
*
* Endung PRG,TOS,TTP,APP	: Programm
* Endung BAT,BTP		: Batchdatei
* sonst				: Textdatei
*
* vorbereitet für lange Dateinamen: Die Extension darf groß oder
* klein geschrieben werden.
*
*********************************************************************/

int suffixtyp(char *s)
{
	char ext[4];

	s = strrchr(s, '.');
	if (s)
	{
		s++;
		ext[0] = (*s++ & 0x5f);
		ext[1] = (*s++ & 0x5f);
		ext[2] = (*s++ & 0x5f);
		ext[3] = '\0';
		if (!strcmp(ext, "PRG") || !strcmp(ext, "APP"))
			return PGMT_ISGEM;
		if (!strcmp(ext, "TOS"))
			return 0;
		if (!strcmp(ext, "TTP"))
			return PGMT_TP;
		if (!strcmp(ext, ext_bat))
			return PGMT_BATCH;
		if (!strcmp(ext, ext_btp))
			return PGMT_BATCH + PGMT_TP;
		if ((!strcmp(ext, "ACC")) || (!strcmp(ext, "ACX")))
			return PGMT_ACC+PGMT_ISGEM;
	}
	return PGMT_NOEXE;
}


/*********************************************************************
*
* gibt != 0 bei passenden Namen zurück.
* Müßte für lange Dateinamen geändert werden.
*
*********************************************************************/
/*
int fname_match(char *fname, char *fmask)
{
	if (fmask[0] == '*' && fmask[1] == '.' && fmask[2] == '*')
		return 1;

	while (*fmask && *fname)
	{
		if (*fmask == '?')
		{
			fmask++;
			if (*fname != '.')
				fname++;
			continue;
		}
		if (*fmask == '*')
		{
			fmask++;
			while (*fmask && *fmask != '.')
				fmask++;
			while (*fname && *fname != '.')
				fname++;
			continue;
		}

		if (*fmask != *fname)
			return 0;
		fmask++;
		fname++;
	}

	return (*fmask == *fname);
}
*/