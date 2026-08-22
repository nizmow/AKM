/*********************************************************************
*
* Beginn der Programmierung: 14.3.89
*
* Dieses Modul enthÑlt die Hauptsteuerung von MAGXDESK, also die
* event_multi - Schleife.
*
*********************************************************************/

#include <mgx_dos.h>
#include "k.h"
#include <vdi.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include "pattern.h"


/*
* global variables
*/

const char *pgm_ver = "5.10";	/* may not exceed four characters, due to .RSC file */

char *gmemptr = NULL;
WINDOW *windows[NUM_WINDOWS + 1];
ICON *icon;
MENUPROGRAM menuprograms[NUM_PROGRAMS];

int ap_id;
int *aes_global;
int work_out[57], work_in [12];	 /* VDI- Felder fÅr v_opnvwk() */
int isfirst;					/* MAGXDESK gebootet */
int is_primary_shell;			/* Haupt-Shell */

int aes_handle;
int vdi_handle;
int vdi_device;
int gl_hhbox, gl_hwbox, gl_hhchar, gl_hwchar;
GRECT desk_g,screen_g;				/* Maûe fÅr Hintergrund */
int char_w,char_h;
int text_column_dist;				/* FÅr Textausgabe */
int dirty_applicat_dat;

char *bg_tile_1;		/* Kachel 1 Plane (monochrom) */
char *bg_tile_4;			/* Kachel 4 Planes (16 Farben) */
char *bg_tile_8;			/* Kachel 8 Planes (256 Farben) */
char *bg_tile_m;			/* Kachel > 256 Farben */
MENUPROGRAM **bg_tile_path;

char *desk_col;
char *desk_patt;

char ext_bat[4];			/* Dateityp .BAT */
char ext_btp[4];			/* Dateityp .BTP */

GRECT windows_g[NUM_WINDOWS + 1];
int n_deskicons;

char dirty_drives[NUM_DRIVES];
DISKINFO dinfo[NUM_DRIVES];

/*
* local variables
*/

static WINDOW windows0;		/* fÅr den Desktop-Hintergrund */

static int text_attrib[10];			 /* Default- Textattribute */

/* Einstellungen */

DEFAULTS settings;

OBJECT *tree_menu;
OBJECT *tree_icons;
OBJECT *tree_settings;
OBJECT *tree_ttppar;
OBJECT *tree_about;
OBJECT *tree_datinf;

static MFDB folder_mfdb,parentfolder_mfdb;
static int folder_y_offs;
int folder_w;


/****************************************************************
*
* FÅhrt auf Tastencodes Aktionen durch.
*
* RÅckgabe 1, falls Code verarbeitet.
*
****************************************************************/

static int key_2_action(int key, char ascii, int kstate)
{
	WINDOW *w;

/*
/* SH-INS	*/
	if (key == 0x5230)
	{
		settings.resident = TRUE;
		return 1;
	}

/* SH-DEL	*/
	if (key == 0x537f)
	{
		settings.resident = FALSE;
		return 1;
	}
*/

	if ((kstate == K_LSHIFT+K_ALT) && (ascii == '<'))
	{
		int dst_apid;
		int msg[8];

		dst_apid = appl_find("START   ");
		if (dst_apid >= 0)
		{
			msg[0] = VA_START;
			msg[1] = ap_id;
			msg[2] = msg[3]=msg[4]=msg[5]=msg[6]=msg[7]=0;
			appl_write(dst_apid, 16, msg);
		}
		return 1;
	}

	if (kstate & (K_LSHIFT + K_RSHIFT + K_ALT))
		return 0;

/* SPACE	*/

	if (key == 0x3920)
	{
		w = whdl2window(top_whdl());
		if ((w) && (w->sel_maske[0]))
			return 0;	/* sonst -> Auswahl */
	}

	wind_update(BEG_UPDATE);
	switch (key)
	{

/* SPACE  */
		case 0x3920:
			dsel_all();
			break;

/* Ctrl-B */
		case 0x3002:
			start_path(menuprograms[INDEX_CMD].path, NULL, 0);
			break;

/* Ctrl-E */
		case 0x1205:
			start_path(menuprograms[INDEX_EDIT].path, NULL, 0);
			break;

/* Ctrl-W */
		case 0x1117:
		{
			WINDOW *tw;
			int i;

			tw = top_window();
			if ((!tw) || (!tw->handle))
				break;
			i = tw->wnr;
			do
			{
				i++;
				if (i > NUM_WINDOWS)
					i = 1;
				w = windows[i];
				if (w)
				{
					w->topped(w);
					break;
				}
			}
			while (i != tw->wnr);
		}
		break;

		default:
		 	wind_update(END_UPDATE);
			return 0;
	}

	wind_update(END_UPDATE);
	return 1;
}


/****************************************************************
*
* Initialisiert ein Popup-Objekt
*
****************************************************************/

void init_popup(OBJECT *dialog, int objnr,
			POPINFO *pop, int rscpop, int defobj)
{
	rsrc_gaddr(0, rscpop,  &(pop->tree));
	pop->obnum = defobj;
	dialog += objnr;
	dialog->ob_spec.free_string = (char *) pop;
	dialog->ob_type = G_POPUP;
}


/****************************************************************
*
* DurchlÑuft alle selektierten Objekte eines Fensters.
* Wenn die Funktion "tue" < 0 liefert, wird abgebrochen.
*
* tue == NULL:		Anzahl Objekte zÑhlen
*
* RÅckgabe: letzter Fehlercode von "tue" oder 0.
*
****************************************************************/

int walk_sel(WINDOW *w, int (*cb)(WINDOW *w, int obj, void *par), void *par)
{
	register int i,tail,ret;
	register OBJECT *tree = w->pobj;


	if ((i = (tree->ob_head)) <= 0)
		return 0;
	tail = (tree++->ob_tail);
	ret = 0;
	for	(; i <= tail; i++,tree++)
	{
		if (!(SELECTED&(tree->ob_state)) || (HIDETREE&(tree->ob_flags)))
			continue;
		if (cb)
		{
			ret = (*cb)(w, i, par);
			if (ret < 0)
				return ret;
		}
		else
			ret++;
	}

	return ret;
}


/****************************************************************
*
* Bestimmt die Begrenzung aller selektierten Unterobjekte
* in absoluten Bildschirmkoordinaten.
* (rechteckige HÅlle aller selektierten Unterobjekte).
*
****************************************************************/

void tree_sel_grect(OBJECT *tree, GRECT *g)
{
	register int i,rx,ry;
	register int mx,my,mrx,mry;	/* Maxima */
	int ox,oy,tail;


	ox = tree->ob_x;
	oy = tree->ob_y;
	mx = my = INT_MAX;		/* Ecke links oben */
	mrx = mry = -INT_MAX;	/* Ecke rechts unten (auûerhalb) */

	if ((i = (tree->ob_head)) > 0)
	{
		tail = (tree++->ob_tail);
		for	(; i <= tail; i++,tree++)
		{
			if ((SELECTED&(tree->ob_state)) && !(HIDETREE&(tree->ob_flags)))
			{
				rx = tree->ob_x;
				ry = tree->ob_y;
				if (rx < mx)
					mx = rx;
				if (ry < my)
					my = ry;
				rx += tree->ob_width;
				ry += tree->ob_height;
				if (rx > mrx)
					mrx = rx;
				if (ry > mry)
					mry = ry;
			}
		}
	}
	g->g_x = mx + ox;
	g->g_y = my + oy;
	g->g_w = mrx - mx;
	g->g_h = mry - my;
}


/*
/*----------------------------------------------------------------------------------------*/ 
/* Fontnamen und Grîûeninformationn zurÅckliefern														*/
/* Funktionsergebnis:	Font-ID																				*/
/*	handle:					VDI-Handle																			*/
/*	flags:					angeforderte Informationen														*/
/*	id:						Font-ID oder 0																		*/
/* index:					Index des Fonts (1 - Anzahl) oder 0											*/
/*	info:						Strukur fÅr die RÅckgabewerte													*/
/*----------------------------------------------------------------------------------------*/ 
typedef struct
{
   LONG  size;             /* LÑnge der Struktur, muû vor vqt_xfntinfo() gesetzt werden */
   WORD  format;           /* Fontformat, z.B. 4 fÅr TrueType */
   WORD  id;               /* Font-ID, z.B. 6059 */
   WORD  index;            /* Index */
   BYTE  font_name[50];    /* vollstÑndiger Fontname, z.B. "Century 725 Italic BT" */
   BYTE  family_name[50];  /* Name der Fontfamilie, z.B. "Century725 BT" */
   BYTE  style_name[50];   /* Name des Fontstils, z.B. "Italic" */
   BYTE  file_name1[200];  /* Name der 1. Fontdatei, z.B. "C:\FONTS\TT1059M_.TTF" */
   BYTE  file_name2[200];  /* Name der optionalen 2. Fontdatei */
   BYTE  file_name3[200];  /* Name der optionalen 3. Fontdatei */
   WORD  pt_cnt;           /* Anzahl der Punkthîhen fÅr vst_point(), z.B. 10 */
   WORD  pt_sizes[64];     /* verfÅgbare Punkthîhen,
                              z.B. { 8, 9, 10, 11, 12, 14, 18, 24, 36, 48 } */
} XFNT_INFO;

static WORD	vqt_xfntinfo(WORD handle, WORD flags, WORD id, WORD index, XFNT_INFO *info)
{
	VDIPB pb;
	WORD contrl[12];
	WORD intin[16];
	WORD ptsin[16];
	WORD intout[16];
	WORD ptsout[16];

	pb.contrl = contrl;
	pb.intin = intin;
	pb.ptsin = ptsin;
	pb.intout = intout;
	pb.ptsout = ptsout;

	info->size = (LONG) sizeof(XFNT_INFO);

	intin[0] = flags;
	intin[1] = id;
	intin[2] = index;
	*(XFNT_INFO **)&intin[3] = info;

	contrl[0] = 229;
	contrl[1] = 0;
	contrl[3] = 5;
	contrl[5] = 0;
	contrl[6] = handle;

	vdi(&pb);

	return intout[1];
}


/****************************************************************
*
* vqt_name fÅr NVDI 3
*
****************************************************************/

static int Mvqt_name(int handle, int index, unsigned int *flags)
{
	VDIPB pb;
	WORD contrl[12];
	WORD intin[4];
	WORD intout[40];

	pb.contrl = contrl;
	pb.intin = intin;
	pb.intout = intout;

	intin[0]  = index;

	contrl[0] = 130;
	contrl[1] = 0;
	contrl[3] = 2;
	contrl[5] = 1;
	contrl[6] = handle;

	vdi(&pb);

/*
Cconout(intout[1]);
Cconout(intout[2]);
Cconout(intout[3]);
Cconout(intout[4]);
Cconout(intout[5]);
Cconout(intout[6]);
Cconout(intout[7]);
Cconout(intout[8]);
*/

	*flags = intout[34];
	return (contrl[4] < 35);
}
*/


/****************************************************************
*
* Setzt die Zeichenhîhe
*
****************************************************************/

void set_char_dim(void)
{
	int dummy;
/*	unsigned int flags;
	XFNT_INFO info;	*/
	BITBLK *folder_image, *parentfolder_image;
	int folder_index,parentfolder_index;
	int out[8];


	if (vq_gdos())
		vst_font(vdi_handle, settings.fontID);
	vst_point(vdi_handle, settings.fontH, &dummy, &dummy,
			&char_w, &char_h);
	/* linksbÅndig, Zeichenzellenoberkante */
	vst_alignment(vdi_handle, 0, 5, &dummy, &dummy);

	/* Spaltenabstand fÅr Textmodus */
	vqt_extent(vdi_handle, "M", out);
	text_column_dist = out[2] - out[0];

	if (char_h >= 24)
	{
		folder_index = I_ORD24;
		parentfolder_index = I_PAR24;
	}
	else
	if (char_h >= 16)
	{
		folder_index = I_ORD16;
		parentfolder_index = I_PAR16;
	}
	else
	{
		folder_index = I_ORD08;
		parentfolder_index = I_PAR08;
	}

	folder_image = tree_icons[folder_index].ob_spec.bitblk;
	parentfolder_image = tree_icons[parentfolder_index].ob_spec.bitblk;

	folder_mfdb.fd_addr = folder_image->bi_pdata;
	parentfolder_mfdb.fd_addr = parentfolder_image->bi_pdata;
	folder_mfdb.fd_w = folder_image->bi_wb << 3;
	parentfolder_mfdb.fd_w = parentfolder_image->bi_wb << 3;
	folder_mfdb.fd_h = folder_image->bi_hl;
	parentfolder_mfdb.fd_h = parentfolder_image->bi_hl;
	folder_mfdb.fd_wdwidth = folder_image->bi_wb >> 1;
	parentfolder_mfdb.fd_wdwidth = parentfolder_image->bi_wb >> 1;
	folder_mfdb.fd_stand =
	parentfolder_mfdb.fd_stand = 1;
	folder_mfdb.fd_nplanes =
	parentfolder_mfdb.fd_nplanes = 1;
	/* vertikal zentrieren */
	folder_y_offs = (char_h - folder_mfdb.fd_h) >> 1;
	folder_w = folder_image->bi_hl; 	/* Breite wie Hîhe */
}


/****************************************************************
*
* Malt das Fenster <w> im Bereich <new_g> nach.
*
****************************************************************/

void redraw(const WINDOW *w, const GRECT *new_g)
{
	if (w->handle > 0 && !w->path[0])
		return;						/* Fenster geschlossen */
	objc_wdraw(w->pobj, 0, 1, new_g, w->handle);
}


/****************************************************************
*
* gibt Nummer des "Fensters" zurÅck, in dem selektierte Objekte
* liegen.
*
* RÅckgabe : *rw = WINDOW *
*		   	  = NULL			kein Objekt selektiert
*		   *objn =  Nummer des 1. selekt. Objekts oder undef.
*
*            icsel = 0	kein Objekt selektiert
*			    = 1   genau ein Objekt selektiert
*                  = -1  mehrere Objekte selektiert
*
****************************************************************/

int icsel(WINDOW **rw, int *objn)
{
	OBJECT *tree;
	register WINDOW **pw;
	register WINDOW *w;
	register int wnr,i;


	*rw = NULL;
	for	(wnr = 0,pw = windows; wnr <= NUM_WINDOWS; wnr++,pw++)
	{
		w = *pw;
		if (!w)
			continue;				/* Fenster unbenutzt */
		if (!w->pobj)
			continue;				/* auch unbenutzt */
		tree = w->pobj;
		if ((i = (tree->ob_head)) > 0)
		{
			for	(; i <= (tree->ob_tail); i++)
			{
				if (!(tree[i].ob_flags & HIDETREE) && selected(tree, i))
				{
					if (*rw)	/* noch einer ! */
						return -1;
					*rw = w;		/* erster gefundener */
					*objn = i;
				}
			}
		}
	}
	return (*rw != NULL);		/* nix gefunden */
}


/****************************************************************
*
* Ermittelt das Icon zu Laufwerk <drv> ('A'..'Z')
*
****************************************************************/

int drv_to_icn(int drv)
{
	register int i;

	for	(i = 0; i < n_deskicons; i++)
		if (icon[i].isdisk == drv)
			return i;
	return -1;
}


/****************************************************************
*
* Rechnet Fensterhandle in WINDOW um.
*
****************************************************************/

WINDOW *whdl2window(int whdl)
{
	register int i;
	register WINDOW **pw,*w;


	for	(i = 0,pw = windows; i <= NUM_WINDOWS; i++,pw++)
	{
		w = *pw;
		if ((w) && ((w)->handle == whdl))
			return w;
	}
	return NULL;
}


/****************************************************************
*
* Gibt an, ob ein Objekt ein Ziel fÅr ein Verschieben von Icons
* sein kann.
* GÅltiges Ziel sind Ordner (O_), Laufwerke (?D), Programme bzw.
* angemeldete Dateien (?P), Batchprogramme (?B),
* Papierkorb und Drucker.
*
****************************************************************/

int is_dest(WINDOW *w, int objnr)
{
	int obtype,config;
	MYDTA *dummy;

	get_obj_type(w, objnr, NULL, &dummy, &obtype, &config, NULL);
	return ((obtype == '_X') || (obtype == 'BX') ||
	        (obtype == 'DO') || (obtype == '_O') ||
	        (obtype == 'P_') || (obtype == 'D_'));
}


/****************************************************************
*
* Zeichnet die benutzerdefinierten Objekte (Texte verschiedener
* Grîûe). Textgrîûe und Ausrichtung auf Zellenoberkante ist schon
* eingestellt.
* Der Clippingbereich wird nur dann neu gesetzt, wenn sich dieser
* geÑndert hat, dadurch werden einige vs_clip()'s gespart.
*
****************************************************************/

int clip_rect[4];

int cdecl draw_userdef(PARMBLK *p)
{
	char *s,*t,*fname;
	register int wnr;
	register WINDOW **pw,*w;
	register MYDTA *m;
	int isalias,isdir,isparentdir;
	int dummy;
	int pxy[4];
	char buf[256];
	static int last_cursive = FALSE;
	int pos,offs;


	for	(wnr = 0,pw = windows; wnr <= NUM_WINDOWS; wnr++,pw++)
	{
		w = *pw;
		if ((w) && (w->pobj == p->pb_tree))
			goto found;
	}
	return 0;
	found:
	pxy[0] = p->pb_xc;
	pxy[1] = p->pb_yc;
	pxy[2] = pxy[0] + p->pb_wc - 1;
	pxy[3] = pxy[1] + p->pb_hc - 1;
	m = (w->pmydta)[p->pb_obj - 1];
	if (pxy[0] != clip_rect[0] || pxy[1] != clip_rect[1] ||
		 pxy[2] != clip_rect[2] || pxy[3] != clip_rect[3])
	{
		clip_rect[0] = pxy[0];
		clip_rect[1] = pxy[1];
		clip_rect[2] = pxy[2];
		clip_rect[3] = pxy[3];
		vs_clip(vdi_handle, TRUE, pxy);
	}
	fname = m->filename;
	isdir = ((m->attrib) & F_SUBDIR);
	isparentdir = (isdir) && (!strcmp(fname, ".."));
	isalias = (int) m->is_alias;
	if (isalias != last_cursive)
	{
		vst_effects(vdi_handle, isalias << 2);	/* kursiv! */
		last_cursive = isalias;
	}

	pos = p->pb_x;

	/* FÅr den Fall, daû nicht der Systemfont verwendet wird,	*/
	/* muû fÅr Ordner ein Icon ausgegeben werden				*/
	/* -------------------------------------------------------- */

	offs = 0;
	if ((settings.fontID != 1) && (isdir))
	{	/* G_IMAGE fÅr Ordner ausgeben */
		int col[2] = {BLACK,WHITE};
		MFDB mfdb_scr;
		MFDB *mfdb_ic;
		int pxy[8];


		mfdb_ic = (isparentdir) ? &parentfolder_mfdb : &folder_mfdb;

		pxy[0] = pxy[1] = 0;
		pxy[2] = mfdb_ic->fd_w - 1;
		pxy[3] = mfdb_ic->fd_h - 1;
		pxy[4] = pos;
		pxy[5] = p->pb_y + folder_y_offs;
		pxy[6] = pos + pxy[2];
		pxy[7] = p->pb_y + pxy[3];

		mfdb_scr.fd_addr = NULL;

		vrt_cpyfm(vdi_handle, MD_TRANS, pxy,
				mfdb_ic, &mfdb_scr, col);
		if (!settings.font_is_prop)
		{
			pos += 3 * char_w;
			offs = 3;
		}
	}

	/* 1. Fall: proportionaler Font			*/
	/* Die Ausgabe erfolgt einzeln in Spalten	*/
	/* ----------------------------------------- */

	if (settings.font_is_prop)
	{
		pos += folder_w + 4;

		if (w->dos_mode && settings.show_8p3 && (!isparentdir))
		{
			t = strchr(fname, '.');
			if (t)
				*t = EOS;
			v_gtext(vdi_handle, pos, p->pb_y, fname);
			pos += 8 + w->xtab_namelen;
			if (t)
			{
				*t++ = '.';
				v_gtext(vdi_handle, pos, p->pb_y, t);
			}
			pos += text_column_dist + w->xtab_typelen;
		}
		else
		{
			v_gtext(vdi_handle, pos, p->pb_y, fname);
			pos += 8 + w->xtab_namelen;
		}

		if (isparentdir)
			goto end_ret;		/* keine weitere Ausgabe */

		if (settings.is_groesse)
		{
			pos += w->xtab_sizelen;
			if (!isdir)
			{
				if (m->filesize > 9999999L)
				{
					ultoa((m->filesize) >> 10L, buf, 10);
					strcat(buf, "k");
				}
				else
					ultoa(m->filesize, buf, 10);
				/* Ausrichtung rechts */
				vst_alignment(vdi_handle, 2, 5, &dummy, &dummy);
				v_gtext(vdi_handle, pos, p->pb_y, buf);
				vst_alignment(vdi_handle, 0, 5, &dummy, &dummy);
			}
			pos += text_column_dist;
		}

		if (settings.is_datum)
		{
			date_to_str(buf, m->date);
			v_gtext(vdi_handle, pos, p->pb_y, buf);
			pos += text_column_dist + w->xtab_datelen;
		}

		if (settings.is_zeit)
		{
			time_to_str(buf, m->time);
			buf[5] = EOS;
			v_gtext(vdi_handle, pos, p->pb_y, buf);
		}
	}

	/* 2. Fall: nichtproportionaler Font		*/
	/* Die Ausgabe erfolgt in einem Rutsch		*/
	/* ----------------------------------------- */

	else
	{
		s = t = buf;

		*s++ = ' ';

		if ((m->icon_type == ITYP_ORDNER) && (settings.fontID == 1))
			*s++ = '\7';		/* Zeichen fÅr Ordner */
		else
		if (m->icon_type == ITYP_PROGRA)
			*s++ = '.';
		else
			*s++ = ' ';

		*s++ = ' ';
		if (w->dos_mode && settings.show_8p3 && (!isparentdir))
		{
			if (*fname == '.' && fname[1] == EOS)
				*s++ = *fname++;
			else
			while (*fname != '.' && *fname != EOS)
				*s++ = *fname++;
			if (*fname == '.')
				fname++;
			t += 12;
			while (s < t)
				*s++ = ' ';
			t += 3;
		}
		else
		{
			strcpy(s, fname);
			t += 3+w->max_fname;
		}

		if (isparentdir)
			goto gtext;		/* keine weitere Ausgabe */

		while (*fname)
			*s++ = *fname++;
		while (s < t)
			*s++ = ' ';

		if (settings.is_groesse)
		{
			char gr[10],*g;

			if (isdir)
				gr[0] = EOS;
			else
			{
				if (m->filesize > 9999999L)
				{
					ultoa((m->filesize) >> 10L, gr, 10);
					strcat(gr, "k");
				}
				else
					ultoa(m->filesize, gr, 10);
			}
			for	(wnr = 8 - (int) strlen(gr); wnr > 0; wnr--)
				*s++ = ' ';
			g = gr;
			while (*g)
				*s++ = *g++;
		}
		if (settings.is_datum)
		{
			*s++ = ' ';
			*s++ = ' ';
			date_to_str(s, m->date);
			s += 8;
		}
		if (settings.is_zeit)
		{
			*s++ = ' ';
			*s++ = ' ';
			time_to_str(s, m->time);
			s += 5;
		}
		*s = EOS;
	    gtext:
		v_gtext(vdi_handle, pos, p->pb_y, buf+offs);
	}

    end_ret:
	return p->pb_currstate;
}

USERBLK userblk = {draw_userdef, 0L};


/****************************************************************
*
* Setzt den Clipping-Bereich auf die Bildschirmgrîûe.
*
****************************************************************/

void set_deflt_clip(void)
{
	clip_rect[0] = desk_g.g_x;
	clip_rect[1] = desk_g.g_y;
	clip_rect[2] = desk_g.g_x + desk_g.g_w-1;
	clip_rect[3] = desk_g.g_y + desk_g.g_h-1;
	vs_clip	(vdi_handle, TRUE, clip_rect);
}


/*********************************************************************
*
* Allgemeine Routinen
*
*********************************************************************/


void open_work(void)
{
	register int i;


	for (i = 0; i < 10; work_in[i++] = 1)
		;
	work_in[10] = 2;                     /* Rasterkoordinaten */
	v_opnvwk(work_in, &vdi_handle, work_out);
	vswr_mode(vdi_handle,MD_REPLACE);      /* Replace- Modus */
	vsf_color(vdi_handle,WHITE);           /* FÅllfarbe weiû */
	vsf_interior(vdi_handle,SOLID);        /* komplett ausfÅllen */
	vqt_attributes(vdi_handle, text_attrib);
	Cursconf(0,0);
	if (vq_gdos())
		vst_load_fonts(vdi_handle, 0);
	/* set_char_dim(); */
}


/****************************************************************
*
* MenÅzeile abschalten
* Speicher fÅr Fenster freigeben
* Desktop- Hintergrund abmelden
* Fenster schlieûen und lîschen
*
****************************************************************/

int deflt_topwnr = -1;

void close_all_wind(void)
{
	register int i;
	register WINDOW **pw,*w;
		    /* int dummy; */


	/* Default- Zeichengrîûe wieder einstellen */
/*
	vst_height(vdi_handle, text_attrib[7], &char_w, &char_h, &dummy, &dummy);
*/
	w = top_window();
	deflt_topwnr = (w) ? w->wnr : -1;
	menu_bar(tree_menu, FALSE);
	wind_set_ptr_int(SCREEN, WF_NEWDESK, NULL, 0);	/* Desktop- Hintergrund */
	for	(i = 1, pw = windows + 1; i <= NUM_WINDOWS; i++,pw++)
	{
		w = *pw;
		if (w)         /* wenn Fenster existiert... */
		{
			if (w->flags & WFLAG_ICONIFIED)
			{
				wind_get_grect(w->handle, WF_UNICONIFY, &w->out);
			}
			wind_close(w->handle);  /* ...dann schlieûen */
			wind_delete(w->handle); /* ...und immer lîschen!  */
			windows_g[i] = w->out;
			Mfree(w);
			*pw = NULL;
		}
	}
}


/****************************************************************
*
* Macht einen Shutdown
*
* dev == -1:		Shutdown
* sonst:			Auflîsungswechsel mit dev/txt
*
* Wird beim MenÅpunkt "Ende" mit (-1,0) aufgerufen.
* Wird beim MenÅpunkt "Alt-Ende" mit (-2,0) aufgerufen.
* Wird beim MenÅpunkt "Auflîsung wechseln" mit (0,0) aufgerufen.
*
****************************************************************/

#pragma warn -par
void shutdown(int dev, int txt)
{
	char shutdown_prg[128];


	if (is_primary_shell)
	{
		strcpy(shutdown_prg, desk_path);
		if (dev >= 0)
			strcat(shutdown_prg, "chgres.prg");
/*
		{
#if			COUNTRY==FRG
			if (1 != form_alert(1, "[2][Auflîsungswechsel ?][ OK | Abbruch ]"))
				return;
#elif		COUNTRY==USA
			if (1 != form_alert(1, "[2][Change resolution ?][ OK | Cancel ]"))
				return;
#endif
		}
		itoa(dev, tail+1, 10);
		strcat(tail+1, " ");
		itoa(txt, tail+1+strlen(tail+1), 10);
		tail[0] = (char) strlen(tail+1);
*/
		else
			strcat(shutdown_prg, "shutdown.prg");

		if (settings.save_on_exit)
			save_status(TRUE);		/* VOR close_all_wind() */

		close_all_wind();
		if (vq_gdos())
			vst_unload_fonts(vdi_handle, 0);
		v_clsvwk(vdi_handle);
		/* Pfade an den Parent weiterreichen: */
		Dsetdrv(desk_path[0]-'A');
		Dsetpath(desk_path);
		shel_write(TRUE, TRUE, SHW_SINGLE, shutdown_prg, "");
		/* und Programm aufrufen */
		shel_write(TRUE, TRUE, SHW_CHAIN, shutdown_prg,
			(dev == -2) ? "\1""1" : "");
	}
	else
	if (settings.save_on_exit)
		save_status(TRUE);

	rsrc_free();
	appl_exit();
	Pterm0();
}
#pragma warn +par


/****************************************************************
*
* Ein Tastencode fÅr Fenster 0 muû verarbeitet werden.
* Er wird einfach ignoriert.
*
****************************************************************/

#pragma warn -par
static void wind0_keyed(WINDOW *w, int kstate, int key)
{
}
static void wind0_fulled(WINDOW *w)
{
}
static void wind0_closed(WINDOW *w, int kstate)
{
}
#pragma warn +par


/****************************************************************
*
* Initialisierung aller Fenster, ggf. îffnen
*
****************************************************************/

void open_all_wind(void)
{
	register int i,j,set;
	long ret;


	Mgraf_mouse(HOURGLASS);
	for	(i = 0; i < NUM_DRIVES; i++)
	{
		dinfo[i].b_clsiz = 0L;		/* ungÅltig */
	}

	/* MenÅzeile auf den Bildschirm */
	/* ---------------------------- */

	menu_bar(tree_menu, TRUE);

	/* Icons installieren und malen */
	/* ---------------------------- */

	windows0.wnr = windows0.handle = 0;
	windows0.key = wind0_keyed;
	windows0.fulled = wind0_fulled;
	windows0.closed = wind0_closed;
	*((GRECT *) &(windows0.pobj->ob_x)) = screen_g;
	wind_set_ptr_int(SCREEN, WF_NEWDESK, windows0.pobj, 0);
	form_dial(FMD_FINISH, NULL, &screen_g);

	/* Fenster îffnen */
	/* -------------- */

	set_char_dim();		/* Zeichengrîûe einstellen */

/*
	for	(i = 1; i <= NUM_WINDOWS; i++)
	{
		windows[i].handle = -1;			/* unbenutzt			*/
		windows[i].memblk = NULL; 		/* kein allozierter Speicher */
	}
*/
	for	(set = FALSE,i = 1; i <= NUM_WINDOWS; i++)
	{
		j = i;
		if (i == deflt_topwnr)
		{
			j = NUM_WINDOWS;
			set = TRUE;
		}
		if (set && i == NUM_WINDOWS)
			j = deflt_topwnr;
		if (windows[j])
		{
			ret = opn_wnd(windows[j], TRUE);
			if (ret == ENOWND)
				ret = ENSMEM;
			err_alert(ret);
		}
	}
	set_deflt_clip();
	Mgraf_mouse(ARROW);
}


/****************************************************************
*
* Laden und Initialisieren der Ressource-Datei.
*
****************************************************************/

static void _rsrc_load(char *fname)
{
	if (!rsrc_load(fname))
	{
		form_xerr(EFILNF, fname);
		appl_exit();
		Pterm((int) EFILNF);
	}
}

void get_rsc(void)
{
	register int i,lowres;
	register OBJECT *image;
	int      isfirst;


	lowres = (desk_g.g_w/gl_hwchar < 60) ||
				 (K_CTRL+K_ALT == (K_CTRL+K_ALT & Kbshift(-1)));
	isfirst = (*((void **) (aes_global+7)) == NULL);
	if (isfirst)								/* erster Start */
	{

		desk_path[0] = Dgetdrv() + 'A';
		desk_path[1] = ':';
		Dgetpath(desk_path+2, 0);
		strcat(desk_path, "\\");

		if (lowres)
		{
			_rsrc_load("magxdesl.rsc");
			rsrc_gaddr(0, HAUPTMEN, &tree_menu);
		}
		else
		{
			tree_menu = NULL;
		}

		_rsrc_load("magxdesk.rsc");
		if (!tree_menu)
		{
			rsrc_gaddr(0, HAUPTMEN, &tree_menu);
		}

		rsrc_gaddr(0, T_ICONS, &tree_icons);
		rsrc_gaddr(0, T_EINST, &tree_settings);
		rsrc_gaddr(0, T_TTPPAR, &tree_ttppar);
		rsrc_gaddr(0, T_ABOUT,  &tree_about);
		rsrc_gaddr(0, T_DATINF, &tree_datinf);
		load_app_icons();
	}

	/* Icongrîûen fÅr Auflîsungen korrigieren */
/*
	for	(i = I_FLPDSK; i <= I_BTCHDA; i++)
	{
		tree_icons[i].ob_width  = 72;
		tree_icons[i].ob_height = 36;
	}
*/

	if (!isfirst)
		return;

	/* Gruppenrahmen anpassen */
	image = tree_about + ABOU_IMG;
	image->ob_height = image->ob_spec.bitblk->bi_hl;
	while ((image->ob_y + image->ob_height > tree_about[ABOU_OS].ob_y) &&
		   (image->ob_y > 0))
		image->ob_y--;

	strcpy(tree_about[ABOU_VER].ob_spec.tedinfo->te_ptext + 11, pgm_ver);
	tree_about[ABOU_OS].ob_spec.free_string = os_ver_s;
	i = tree_about[0].ob_width;
	i -= (int) strlen(os_ver_s) * gl_hwchar;
	i /= 2;
	if (i < 0)
		i = 0;
	tree_about[ABOU_OS].ob_x = i;

	allg_init();
	menu_init();
	dialogs_init();
	windows_init();
}


/****************************************************************
*
* Initialisierung von MAGIXDESK (AES,RSC,VDI,Fenster,Icons,MenÅ).
* MAGIXDESK wird immer im eigenen Verzeichnis gestartet.
*
****************************************************************/

static void pgm_init(void)
{
	register int i;
	char srcmd[128],srtail[128];
	SHELTAIL *sht;


	Pdomain(PDOM_MINT);

	for	(i = 0; i < NUM_DRIVES; i++)
	{
		dirty_drives[i] = FALSE;
	}

	/* Applikation beim AES anmelden und Version feststellen */
	/* ----------------------------------------------------- */

	if ((ap_id = appl_init()) < 0)
		Pterm(-1);

	aes_handle = vdi_handle =
	graf_xhandle(&gl_hwchar, &gl_hhchar, &gl_hwbox, &gl_hhbox,
						 &vdi_device);
	aes_global = _GemParBlk.global;
	wind_get_grect(SCREEN, WF_WORKXYWH, &desk_g);
	screen_g.g_x = screen_g.g_y = 0;
	screen_g.g_w = desk_g.g_x+desk_g.g_w;
	screen_g.g_h = desk_g.g_y+desk_g.g_h;

	/* TOS- Version und GEM- Variablen fÅr spÑter merken */
	/* ------------------------------------------------- */

	get_syshdr();		/* bestimmt aesvars, inf_name[0] (drv) */
	shel_read(srcmd, srtail);

	is_primary_shell = FALSE;
	isfirst = FALSE;

	if (!srcmd[0])	/* Name muû leer sein */
	{
		sht = (SHELTAIL *) srtail;
		if (sht->dummy == 0 && sht->magic == 'SHEL')
			is_primary_shell = TRUE;
		isfirst = sht->isfirst;
	}

	get_rsc();

	/* VDI - Workstationhandle holen */
	/* ----------------------------- */

	open_work();

	wind_update(BEG_UPDATE);

	/* Koordinaten des Desktop- Fensters initialisieren */
	/* ------------------------------------------------ */

	windows[0] = &windows0;		/* alle anderen Zeiger sind NULL */
	windows0.handle = SCREEN;
	windows0.in = windows0.out = desk_g;
	windows0.pmydta	= NULL;

	/* numplanes-abhÑngige Variablen */
	/* ----------------------------- */

	bg_tile_1 = bg_tile_4 = bg_tile_8 = bg_tile_m = NULL;
	if (aes_global[10] < 4)	/* 2,4 oder 8 Farben */
	{
		bg_tile_path = (MENUPROGRAM **) (&bg_tile_1);
		desk_col = &(settings.desk_col_1);
		desk_patt = &(settings.desk_patt_1);
	}
	else
	{
		desk_col = &(settings.desk_col_4);
		desk_patt = &(settings.desk_patt_4);
		if (aes_global[10] < 8)	/* 16 Farben */
			bg_tile_path = (MENUPROGRAM **) (&bg_tile_4);
		else
		if (aes_global[10] == 8)	/* 256 Farben */
			bg_tile_path = (MENUPROGRAM **) (&bg_tile_8);
		else
			bg_tile_path = (MENUPROGRAM **) (&bg_tile_m);
	}
	

	/* INF-Datei laden oder Defaults setzen */
	/* ------------------------------------ */

	load_status(0);
	load_status(1);

	open_all_wind();

	/* Wenn keine Laufwerke da sind, "Laufwerke finden" */
	/* ------------------------------------------------ */

	for	(i = 0; i < n_deskicons; i++)
	{
		if (icon[i].isdisk)
			goto disk_found;
	}

	dial_register_drv();		/* Laufwerke anmelden */
	disk_found:

	/* ggf. Disknamen initialisieren */
	/* ----------------------------- */

	if (settings.dnam_init)
	{
		for	(i = 0; i < n_deskicons; i++)
		{
			register int j;
			int  drv;
			static char *pth = "X:\\";
			char name[65];
	
			if (!icon[i].isdisk)
				nexticn:
				continue;
			drv = icon[i].isdisk;
			for	(j = 1; j <= NUM_WINDOWS; j++)
			{
				if ((windows[j]) && (windows[j]->path[0] == drv))
					goto nexticn;
			}
			if (drv < 'C')		/* nicht fÅr A: und B: */
				continue;
			name[0] = EOS;
			pth[0] = drv;
			Dreadlabel(pth, name, 65);
			set_dname(drv-'A', name);
		}
	}

	wind_update(END_UPDATE);

	/* Wenn APPLICAT.INF verÑndert wurde, muû	*/
	/* APPLICAT.DAT neu aufgebaut werden		*/
	/* ----------------------------------------- */

	if (dirty_applicat_dat)
	{
		start_util_program("APPLICAT.APP", FALSE, FALSE,
						"-c", NULL, NULL);
		dirty_applicat_dat = FALSE;
	}
}


/****************************************************************
*
* Tastenbehandlung
*
****************************************************************/

static void handle_key(EVNT *w_ev)
{
	register int i, scancode;
	int title, entry;
	int ascii;
	WINDOW *w;


	scancode = (w_ev->key >> 8) & 0xff;

	if (w_ev->kstate & K_ALT)
		ascii = alt_keycode_2_ascii(w_ev->key);
	else
		ascii = w_ev->key & 0xff;

	if (w_ev->kstate & (K_RSHIFT + K_LSHIFT))
	{
		w_ev->kstate |= K_LSHIFT;
	}
	w_ev->kstate &= (K_LSHIFT + K_CTRL + K_ALT);

	/* 1. Fall: Alt-Shift-[A..Z] (Fenster îffnen) */
	/* ----------------------------------------- */

	if (w_ev->kstate == (K_ALT + K_LSHIFT))
	{
		int drv = toupper(ascii);
		if ((drv >= 'A') && (drv <= 'Z'))
		{
			if (0 > (i = drv_to_icn(drv)))
				return;	/* Taste verarbeitet */

			/* gefunden: Doppelklick simulieren */

			wind_update(BEG_UPDATE);
			dsel_all();
			ob_sel(windows[0]->pobj, i + 1);	/* ??? */
			redraw_window_object(windows[0], i + 1);
			dclick(windows[0], i + 1, 0);
			wind_update(END_UPDATE);
			return;	/* Taste verarbeitet */
		}
	}

	/* 2. Fall: Alt-1 .. Alt-9 (Auflîsungswechsel) */
	/* ------------------------------------------- */
/*
	if ((w_ev->kstate & K_ALT) &&
		 !(w_ev->kstate & (K_LSHIFT+K_RSHIFT+K_CTRL)) &&
		 (unsigned int) keycode >= 0x7800 &&
		 (unsigned int) keycode <= 0x8100)
	{
		i = scancode - 0x77;
		if (i != vdi_device);
			shutdown(i, 0);
		return;	/* Taste verarbeitet */
	}
*/

	/* 3. Fall: Ctrl-Fn (MenÅprogramm ausfÅhren) */
	/* ----------------------------------------- */

	if ((w_ev->kstate & K_CTRL) &&
			w_ev->key >= 0x3b00 &&
			w_ev->key <= 0x4600)
	{
		wind_update(BEG_UPDATE);
		start_path(menuprograms[scancode - 0x3b + INDEX_USER].path,
					NULL, w_ev->kstate & K_ALT);
		wind_update(END_UPDATE);
		return;	/* Taste verarbeitet */
	}

	/* 4. Fall: MenÅeintrag Åber Tastatur angewÑhlt */
	/* -------------------------------------------- */

	if (key_2_menu(w_ev->key, w_ev->kstate, &title, &entry))
	{
		if ((title >= 0) && !((tree_menu[entry].ob_state) & DISABLED))
		{
			wind_update(BEG_UPDATE);
			menu_tnormal(tree_menu, title, FALSE);
			do_menu(title, entry, 0);
			wind_update(END_UPDATE);
		}
		return;	/* Taste verarbeitet */
	}

	/* 5. Fall: Aktion durchfÅhren */
	/* --------------------------- */

	if (key_2_action(w_ev->key, ascii, w_ev->kstate))
		return;	/* Taste verarbeitet */


	/* letzer Fall: Tastencode an Fenster durchreichen */
	/* ----------------------------------------------- */

	w = top_window();
	if (w)
	{
		wind_update(BEG_UPDATE);
		w->key(w, w_ev->kstate, w_ev->key);
		wind_update(END_UPDATE);
	}

}


/****************************************************************
*
* "message"-Behandlung
*
****************************************************************/

static void handle_message(EVNT *w_ev)
{
	if (w_ev->msg[0] == MN_SELECTED)
	{
		do_menu(w_ev->msg[3], w_ev->msg[4], w_ev->kstate);
	}
	else
	{
		switch (w_ev->msg[0])
		{
			case 'AK':
				switch (w_ev->msg[3])
				{
					case 0:
						re_read_icons(*(char **)(w_ev->msg + 6));
						break;

					case 1:
				 		settings.mode_ovwr = w_ev->msg[4];
						break;
				}
				break;

			case AV_PATH_UPDATE:
				w_ev->msg[3] = (**(char **)(w_ev->msg + 3)) - 'A';

			case SH_WDRAW:
		     	if ((w_ev->msg[3] >= 0) && (w_ev->msg[3] < NUM_DRIVES))
		     	{
		     		int i;
					WINDOW **pw;
					WINDOW *w;

					dirty_drives[w_ev->msg[3]] = TRUE;
					for	(i = 1,pw = windows+1;
						i <= NUM_WINDOWS;
						i++,pw++)
					{
						w = *pw;
						if ((w) && (w->path[0] -'A' == w_ev->msg[3]))
							dirty_drives[w->real_drive] = TRUE;
					}
				}
				else
				{
		     		int i;

					for	(i = 0; i < NUM_DRIVES; i++)
						dirty_drives[i] = TRUE;
				}
				break;

			case AV_PROTOKOLL:
			{
				int client = w_ev->msg[1];
				w_ev->msg[0] = VA_PROTOSTATUS;
				w_ev->msg[1] = ap_id;
				w_ev->msg[2] =
				w_ev->msg[4] = w_ev->msg[5] = 0;
				w_ev->msg[3] =
					1 +		/* AV_SENDKEY */
					16 +		/* AV_OPENWIND */
					32 +		/* AV_STARTPROG */
					512 +	/* AV_PATH_UPDATE,
							   AV_WHAT_IZIT,
							   AV_DRAG_ON_WINDOW */
					1024 +	/* AV_EXIT */
					2048 +	/* AV_XOPENWIND */
					8192;	/* AV_STARTED */
				(*(char **)(w_ev->msg + 6)) = "MAGXDESK";
				appl_write(client, 16, w_ev->msg);
				break;
			}/*
			case AV_EXIT:
				break;
*/
			case AV_SENDKEY:
				w_ev->kstate = w_ev->msg[3];
				w_ev->key = w_ev->msg[4];
				w_ev->mwhich &= ~MU_MESAG;	/* bearbeitet */
				return;

			case AV_WHAT_IZIT:
			{				int client = w_ev->msg[1];
				WINDOW *w = whdl2window(wind_find(w_ev->msg[3], w_ev->msg[4]));
				if (w)
				{
					if (w->handle)
					{
						w_ev->msg[4] = VA_OB_WINDOW;
					}
					else
					{
						/* hier erweitern! */
						w_ev->msg[4] = VA_OB_UNKNOWN;
					}

					w_ev->msg[0] = VA_THAT_IZIT;
					w_ev->msg[1] = ap_id;
					w_ev->msg[2] =
					w_ev->msg[7] = 0;
					w_ev->msg[3] = ap_id;					(*(char **)(w_ev->msg + 5)) = NULL;
					appl_write(client, 16, w_ev->msg);
				}				break;
			}
			case AV_DRAG_ON_WINDOW:
			{				int client = w_ev->msg[1];
				w_ev->msg[0] = VA_DRAG_COMPLETE;
				w_ev->msg[1] = ap_id;
				w_ev->msg[2] =
				w_ev->msg[3] =
				w_ev->msg[4] =
				w_ev->msg[5] =
				w_ev->msg[6] =
				w_ev->msg[7] = 0;
				appl_write(client, 16, w_ev->msg);
				break;
			}
			case AV_STARTED:
				Mfree((*(char **)(w_ev->msg + 3)) - 1);
				break;

			case AV_STARTPROG:
				{
				int i;
				int client = w_ev->msg[1];
				i = start_path(*(char **)(w_ev->msg + 3),
						*(char **)(w_ev->msg + 5),
						-1);
				w_ev->msg[0] = VA_PROGSTART;
				w_ev->msg[1] = ap_id;
				w_ev->msg[2] = w_ev->msg[4] =
				w_ev->msg[5] = w_ev->msg[6] = 0;
				w_ev->msg[3] = i;
				appl_write(client, 16, w_ev->msg);
				if (i == -1)
					res_exec();
				break;
				}

			case AV_OPENWIND:
				w_ev->msg[7] = 0;
			case AV_XOPENWIND:
			{
				int client;
				if (w_ev->msg[7] & 2)
					open_window(*(char **)(w_ev->msg + 3),	/* path */
							  "*",				/* mask */
							  *(char **)(w_ev->msg + 5),	/* selmask */
							  ((w_ev->msg[7] & 1) == 0));	/* new */
				else
					open_window(*(char **)(w_ev->msg + 3),
							  *(char **)(w_ev->msg + 5),
							  NULL,
							  ((w_ev->msg[7] & 1) == 0));

				client = w_ev->msg[1];
				w_ev->msg[0] += (VA_WINDOPEN-AV_OPENWIND);
				w_ev->msg[1] = ap_id;
				w_ev->msg[2] = w_ev->msg[4] =
				w_ev->msg[5] = w_ev->msg[6] = 0;
				w_ev->msg[3] = 1;	/* Erfolg */
				appl_write(client, 16, w_ev->msg);
				break;
			}

			case AP_DRAGDROP:
			{
				char *s;
				int h;

				s = "U:\\PIPE\\DRAGDROP.AA";
				s[17] = ((char *) (w_ev->msg + 7))[0];
				s[18] = ((char *) (w_ev->msg + 7))[1];
				h = (int) Fopen(s, RMODE_WR);
				if (h >= 0)
				{
					s[17] = DD_NAK;
					Fwrite(h, 1L, &s[17]);
					Fclose(h);
				}
				break;
			}

			case AP_TERM:
				wind_update(END_UPDATE);
				shutdown(-1, 0);
				wind_update(BEG_UPDATE);
				break;

			default:
			{
				int i;
				WINDOW **pw;
				WINDOW *w = whdl2window(w_ev->msg[3]);

				if (w)
					switch (w_ev->msg[0])
					{
						case WM_REDRAW:
							redraw(w, (GRECT *) (w_ev->msg + 4));
							break;

						case WM_TOPPED:
					 		w->topped(w);
					 		break;

						case WM_CLOSED:
					 	   	w->closed(w, w_ev->kstate);
					 	   	break;

						case WM_ALLICONIFY:
							for	(i = 1,pw = windows+1; i <= NUM_WINDOWS; i++,pw++)
							{
								WINDOW *w2 = *pw;
								if ((w2) && (w2->handle > 0) && (w2 != w))
									w2->iconified(w2, NULL, ICONIFIED_MODE_HIDE);
							}
							w->iconified(w, (GRECT *) (w_ev->msg + 4), ICONIFIED_MODE_ALL);
							break;

						case WM_ICONIFY:
					 	    w->iconified(w, (GRECT *) (w_ev->msg + 4), ICONIFIED_MODE_NORMAL);
					 	    break;

						case WM_UNICONIFY:
							for	(i = 1,pw = windows + 1; i <= NUM_WINDOWS; i++,pw++)
							{
								WINDOW *w2 = *pw;
								if ((w2) && (w2->path[0]) && (w2->handle < 0) && (w2 != w) && (w2->flags & WFLAG_ALLICONIFIED))
									w2->uniconified(w2, NULL, TRUE);
							}
					 		if (w->flags & WFLAG_ALLICONIFIED)
						 		w->topped(w);
					 		w->uniconified(w, (GRECT *) (w_ev->msg + 4), FALSE);
					 		break;

						case WM_FULLED:
							w->fulled(w);
							break;

						case WM_ARROWED:
							w->arrowed(w, w_ev->msg[4]);
							break;

						case WM_HSLID:
							w->hslid(w, w_ev->msg[4]);
							break;

						case WM_VSLID:
							w->vslid(w, w_ev->msg[4]);
							break;

						case WM_SIZED:
							w->sized(w, (GRECT *) (w_ev->msg + 4));
							break;

						case WM_MOVED:
							w->moved(w, (GRECT *) (w_ev->msg + 4));
							break;

						case WM_NEWTOP:
						case WM_UNTOPPED:
							w->message(w, w_ev->kstate, w_ev->msg);
							break;
					}
				break;
			}
		}
	}
}


/****************************************************************
*
* Hauptfunktion
*
****************************************************************/

void main()
{
	EVNT w_ev;
	int button_waitclicks;
	int mbuttons;
	int waitfor, mbmask;
	long last_clicktime = 0;
	unsigned dclick_interval_hz200;		/* in 5-ms-Einheiten */


	pgm_init();
	Mgraf_mouse(ARROW);

	for (;;)
	{
		upd_drives();
		upd_infos();
		modify_menu();

		/* Das eingestellte Doppelklick-Intervall kînnte geÑndert worden sein. */
		dclick_interval_hz200 = get_dclick_interval_ms() / 5;

		/* Wenn ein Laufwerk mit Tastatur selektiert wurde, wird 200 ms	*/
		/* lang auf den zweiten Klick gewartet							*/
		/* ------------------------------------------------------------ */

		waitfor = MU_KEYBD + MU_BUTTON + MU_MESAG + MU_TIMER;

		if (settings.rtbutt_dclick)
		{
			button_waitclicks = 0x101;	/* NOT und Doppelklicks manuell */
			mbuttons = 0;		/* keine Maustaste gedrÅckt */
			mbmask = 3;		/* beide Maustasten */
		}
		else
		{
			button_waitclicks = 1; /* Doppelklicks manuell erkennen, nicht vom System */
			mbuttons = 1;		/* linke Maustaste gedrÅckt	*/
			mbmask = 1;		/* nur linke Maustaste */
		}
		
		w_ev.mwhich = evnt_multi(waitfor,
						  button_waitclicks,
						  mbmask,			
						  mbuttons,
						  0,NULL,		/* kein 1. Rechteck */
						  0,NULL,		/* kein 2. Rechteck */
						  w_ev.msg,
						  AUTOUPDATE_MS,	/* ms */
						  (EVNTDATA *) &(w_ev.mx),
						  &w_ev.key,
						  &w_ev.mclicks
						  );

		/* Fensterdialoge */
		/* -------------- */

		do_info_file_dialogs(&w_ev);

		/* Timer-Ereignis */
		/* -------------- */

		if (w_ev.mwhich & MU_TIMER)
		{
			/* alternatively, we could omit the TIMER when auto-reload is deactivated. */
			if (!settings.auto_reload_off)
			{
				dirwindow_autoupdate();
			}
		}

		/* Abfrage auf BetÑtigung einer Taste */
		/* ---------------------------------- */

		if (w_ev.mwhich & MU_KEYBD)
		{
			handle_key(&w_ev);
		}

		/* Abfrage auf Nachricht im Nachrichtenpuffer */
		/* ------------------------------------------ */

		if (w_ev.mwhich & MU_MESAG)
		{
			wind_update(BEG_UPDATE);
			handle_message(&w_ev);
			wind_update(END_UPDATE);
		}

		/* Abfrage auf BetÑtigung eines Mausknopfs */
		/* --------------------------------------- */

		if (w_ev.mwhich & MU_BUTTON)
		{
			/* linken Doppelklick feststellen */

			if (w_ev.mbutton & 1)
			{
				long clicktime = getHz200();
				if (clicktime - last_clicktime < dclick_interval_hz200)
				{
					w_ev.mclicks = 2;
				}
				last_clicktime = clicktime;
			}

			/* Doppelklick mit rechter Maustaste simulieren */

			if (settings.rtbutt_dclick && (w_ev.mbutton == 2))
			{
				w_ev.mclicks = 2;
				w_ev.mbutton = 1;		/* Doppelklick simulieren */
			}

			handle_mouse_button(w_ev.mclicks, (const EVNTDATA *) &(w_ev.mx));

			/* poll mouse and keyboard state */
			graf_mkstate((EVNTDATA *) &(w_ev.mx));
			if (w_ev.mbutton & 1)
			{
				/* left button still pressed, wait for release */
				evnt_button(1, 1, 0, (EVNTDATA *) &(w_ev.mx));
			}
		}

	} /* END FOREVER */
}
