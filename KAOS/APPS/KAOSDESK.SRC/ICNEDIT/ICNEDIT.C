/*******************************************************************
*
*		    ICNEDIT.PRG						05.08.89
*		    ===========
*						    letzte énderung:	10.04.94
*
* Icondateien ".ICD" und ".ICP" laden, anzeigen, editieren und
* wieder abspeichern
*
* geschrieben mit TUBO C V1.1, PureC 1.1
*
*******************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <mgx_dos.h>	/* include before tos.h */
#include <tos.h>		/* include before mt_aes.h */
#include <tosdefs.h>
#include <mt_aes.h>
#include <vdi.h>
#include "icnedit.h"

#define LANG_GER 1
#define LANG_FRA 2		/* not supported, yet */
#define LANG_ENG 3
#define LANG_SPA 4		/* not supported, yet */
#define LANG_ITA 5		/* not supported, yet */
#define LANG_SWE 6		/* not supported, yet */

#define LANGUAGE LANG_GER

#define SCREEN 0
#define TRUE  1
#define FALSE 0
#define MIN(a,b) ((a < b) ? a : b)
#define MAX(a,b) ((a > b) ? a : b)

#define XBEG_MM	4				/* Rand in Millimetern */
#define YBEG_MM	4				/* Rand in Millimetern */

#define ICON_W 32				/* Icon-Breite in Pixeln */
#define ICON_H 28				/* Icon-Hîhe in Pixeln */

int work_out[57], work_in[12];	/* VDI-Felder fÅr v_opnvwk() */

char pgm_data[0x70];			/* Programmicon */
char dat_data[0x70];			/* Dateiicon */

char pgm_fname[128];			/* Dateiname fÅr Programmicon */
char dat_fname[128];			/* Dateiname fÅr Dateiicon */

static int dirty_pgm;			/* Programm-Icon ist verÑndert worden */
static int dirty_dat;			/* Datei-Icon ist verÑndert worden */


int wpix;						/* Bildschirmpixel pro Icon-Pixel, ohne Linien */
int hpix;						/* Zwischen zwei Icon-Pixeln ist eine dÅnne Linie von einem Pixel */

int xbeg;						/* Rand in Pixeln */
int ybeg;						/* Rand in Pixeln einschl. MenÅleiste */


int ap_id;
int wind_handle = -1;
int vdi_handle, gl_hhbox, gl_hwbox, gl_hhchar, gl_hwchar;
static GRECT desk_g;			/* Maûe fÅr Hintergrund */
static GRECT wind_g;			/* Maûe fÅr Fenster */
static int scrx2;				/* rechte HÑlfte */
int clip_array[4];
GRECT clip_grect;

static OBJECT *addr_main_menu;
static OBJECT *addr_icon_pgm;
static OBJECT *addr_icon_dat;

static char path[128] = "\0", filename[20];

#if LANGUAGE == LANG_GER
const char *alrt_lose_changes = "[2][Ungesicherte énderungen verlieren?][JA| NEIN ]";
#elif LANGUAGE == LANG_ENG
const char *alrt_lose_changes = "[2][Lose changes?][YES|  NO  ]";
#endif


/********************************************************************
*
* Dateiauswahldialog ("file selector") fÅr alle TOS- Versionen.
* Ab TOS 1.4 (AES 1.40) und nicht >= GEM 2.0 wird die öberschrift
* vewendet.
*
* RÅckgabe 1 fÅr "OK" und 0 fÅr "Abbruch" oder im Fehlerfall
*
********************************************************************/

static int fsel(char *path, char *sel, const char *label)
{
	int ret;
	int button;

	if (_GemParBlk.global[0] < 0x0200 && _GemParBlk.global[0] >= 0x0140)
		ret = fsel_exinput(path, sel, &button, label);
	else
		ret = fsel_input(path, sel, &button);

	if (ret == 0)
	{
		button = 0;		/* Fehlerfall wie Abbruch */
	}

	return button;
}


/*********************************************************************
*
* Ermittelt zu einem vollen Pfadnamen den Zeiger auf den
* reinen Dateinamen
*
*********************************************************************/

static char *get_name(char *path)
{
	register char *n;

	n = strrchr(path, '\\');
	if (!n)
	{
		if ((*path) && (path[1] == ':'))
			path += 2;
		return path;
	}

	return n + 1;
}


/*********************************************************************
*
* Isoliert aus <path> den reinen Pfad ohne Dateinamen, komplettiert
* ihn mit Laufwerk, aktuellem Pfad und '\'. Schlieûlich wird der
* <typ> mit "*.<typ>" angehÑngt
*
* <typ> ist von der Form "EXT".
*
*********************************************************************/

static void makepath(char *path, const char *type)
{
	char *t;
	int  drv;
	char temp[128];


	*get_name(path) = '\0';						/* Pfad isolieren */
	strcpy(temp, path);
	t = temp;
	if (!t[0] || t[1] != ':')					/* kein Laufwerk */
		drv = Dgetdrv();
	else
	{
		drv = toupper(*t) - 'A';
		t += 2;
	}
	*path++ = drv + 'A';
	*path++ = ':';
	if (*t != '\\')							/* kein absoluter Pfad */
	{
		*path = '\0';							/* sicherheitshalber */
		Dgetpath(path, drv + 1);
		path += strlen(path);
		if (path[-1] != '\\')					/* kein letzer '\' */
			*path++ = '\\';
	}

	strcpy(path, t);
	path += strlen(path);
	if (path[-1] != '\\')						/* kein letzer '\' */
		*path++ = '\\';
	*path++ = '*';
	*path++ = '.';
	strcpy(path, type);
}


/*********************************************************************
*
* selected an path anhÑngen
*
*********************************************************************/

static char *makenam(char *path, const char *selected)
{
	char *ret;

	ret = path;
	path += strlen(path);
	while(0 != (*--path != '\\'));
	while(0 != (*++path =  *selected++));
	return ret;
}


/*********************************************************************
*
* Gibt eine Zeichenkette Åber form_alert() aus und bindet dabei den
* Dateinamen bei %F ein.
*
*********************************************************************/

static int alert(int defbutton, const char *s)
{
	char temp[200];
	register char *t,*s2;


	t = temp;
	while(*s)
	{
		if (*s == '%')
		{
			s++;
			if (*s == 'F')
			{
				s++;
				s2 = filename;
				while(*s2)
					*t++ = *s2++;
				continue;
			}
		}
		*t++ = *s++;
	}

	*t = '\0';
	return form_alert(defbutton, temp);
}


/********************************************************************
*
* PrÅft, ob in filename[] die korrekte <extension> vorliegt.
* Wenn bisher "*" die Extension war, wird sie = <extension> gesetzt.
* Die Extension ist von der Form "EXT".
* Die Routine gibt TRUE zurÅck, falls alles in Ordnung ist.
*
********************************************************************/

static int is_filename_valid(const char *file_type)
{
	register char *curr_file_type;

	if ((curr_file_type = strrchr(filename, '.')) == NULL)
	{
		err:
		#if LANGUAGE == LANG_GER
		alert(1,"[3][Falscher Dateityp|von %F!][ABBRUCH]");
		#elif LANGUAGE == LANG_ENG
		alert(1,"[3][Invalid file type:| %F!][ Abort ]");
		#endif
		return FALSE;
	}

	if (*(++curr_file_type) == '*')
		strcpy(curr_file_type, file_type);
	else
	{
		if (strcmp(curr_file_type, file_type))
			goto err;
	}

	return TRUE;
}


/********************************************************************
*
* ôffnet (liest) eine Datei ein.
* FÅr das Lesen wird die Funktion <lesen()> benutzt, die bei Fehlern
* einen Wert ungleich Null zurÅckgibt.
* Der Dateityp ist von der Form "EXT".
* Die Routine gibt FALSE zurÅck, falls korrekt gelesen wurde.
*
********************************************************************/

static int fsel_open_file(int (*read_cb)(int handle), const char *file_type)
{
	#if LANGUAGE == LANG_GER
	const char *title = "Datei lesen...";
	const char *alrt_new = "[2][Neue Datei %F|anlegen?][OK|ABBRUCH]";
	const char *alrt_read_err = "[3][Datei- Lesefehler|auf %F!][ABBRUCH]";
	const char *alrt_open_err = "[3][Datei %F|lÑût sich nicht îffnen.][ABBRUCH]";
	#elif LANGUAGE == LANG_ENG
	const char title = "Read file...";
	const char *alrt_new = "[2][Create new file %F?][OK| Abort ]";
	const char *alrt_read_err = "[3][Read error on file %F!][ Abort ]";
	const char *alrt_open_err = "[3][Cannot open file %F.][ Abort ]";
	#endif


	for (;;)
	{
		makepath(path, file_type);
		if (!fsel(path, filename, title))
		{
			return TRUE;	/* error or Cancel */
		}

		if (is_filename_valid(file_type))
		{
			int	handle;

			handle = (int) Fopen(makenam(path, filename), RMODE_RD);
			if (handle == (int) EFILNF)
			{
				if (1 == alert(1, alrt_new))
					return (int) E_OK;
			}
			else
			if (handle > 0)
			{
				if (!(*read_cb) (handle))
				{
					return FALSE;		  /* alles ok */
				}
				else
				{
					alert(1, alrt_read_err);	/* Lesefehler und Abbruch */
				}
				Fclose(handle);
				return TRUE;	/* Fehler */
			}
			else
			{
				alert(1, alrt_open_err);	/* Fehler und nochmal versuchen */
			}
		}
	}
}


/********************************************************************
*
* Speichert eine Datei unter <name>, wenn <name> keine Wildcards
* enthÑlt und den korrekten Dateityp hat.
* FÅr das Schreiben wird die Funktion <schreiben()> benutzt, die bei
* Fehlern einen Wert ungleich Null zurÅckgibt.
* <typ> ist von der Form "EXT".
* Die Routine gibt FALSE zurÅck, falls korrekt geschrieben wurde.
*
********************************************************************/

static int save(char *name, int (*write_cb)(int handle), const char *file_type)
{
	int handle;

	#if LANGUAGE == LANG_GER
	#define overwrite_err_s "[2][Datei %F|Åberschreiben?][Ja|Nein]"
	#define write_err_s "[3][Datei- Schreibfehler|auf %F!][ABBRUCH]"
	#elif LANGUAGE == LANG_ENG
	#define overwrite_err_s	"[2][Overwrite file %F?][Yes|No]"
	#define write_err_s	"[3][Write error on file %F!][ Abort ]"
	#endif

	strcpy(filename, get_name(name));
	if ((!is_filename_valid(file_type)) || strchr(name, '*') || strchr(name, '?'))
	{
		return TRUE;	  /* Dateityp stimmt nicht oder Wildcards */
	}

	if (Fsfirst(name, 0xff) == 0L)
	{
		/* Datei existiert schon */
		if (alert(2, overwrite_err_s) == 2)
		{
			return TRUE;
		}

		#if 0
		if (Fdelete(name) < 0L)
		{
			err: alert(1, write_err_s);
			return TRUE;
		}
		#endif
	}

	if ((handle = (int) Fcreate(name, 0)) > 0)
	{
		int ret = (*write_cb)(handle);		/* write data */
		Fclose(handle);
		if (!ret)
		{
			return FALSE;		/* success */
		}

		Fdelete(name);
	}

	alert(1, write_err_s);
	return TRUE;	/* failure */
}


/********************************************************************
*
* Speichert eine Datei, fragt vorher nach dem Namen.
* FÅr das Schreiben wird die Funktion <write_cb()> benutzt, die bei
* Fehlern einen Wert ungleich Null zurÅckgibt.
* Der Dateityp ist von der Form "EXT".
* Die Routine gibt FALSE zurÅck, falls korrekt geschrieben wurde.
*
********************************************************************/

int saveas(int (*write_cb)(int handle), const char *file_type)
{
	#if LANGUAGE == LANG_GER
	const char *title = "Datei speichern...";
	#elif LANGUAGE == LANG_ENG
	const char *title = "Save file...";
	#endif


	strcpy(filename, get_name(path));

	for (;;)
	{
		makepath(path, file_type);
		if (!fsel(path, filename, title))
		{
			return TRUE;	/* Cancel or error */
		}
		if (!save(makenam(path, filename), write_cb, file_type))
		{
			return FALSE;	/* success */
		}
	}
}


/********************************************************************
*
* Speichert eine Datei, benutzt den zuletzt verwendeten Namen.
* FÅr das Schreiben wird die Funktion <write_cb()> benutzt, die bei
* Fehlern einen Wert ungleich Null zurÅckgibt.
* Der Dateityp ist von der Form "EXT".
*
********************************************************************/

static int close_file_dialogue(int (*write_cb)(int handle), const char *file_type)
{
	return save(path, write_cb, file_type);
}


/*********************************************************************
*
* Initialisierung des AES/VDI
*
*********************************************************************/

static void open_work(void)
{
	register int i;


	for  (i = 0; i < 10; work_in[i++] = 1)
		;
	work_in[10] = 2;                     /* Rasterkoordinaten */
	v_opnvwk(work_in, &vdi_handle, work_out);
	vswr_mode(vdi_handle, MD_REPLACE);      /* Replace- Modus */
	vsf_interior(vdi_handle, SOLID);        /* komplett ausfÅllen */

	#if 0
	printf(" work_out[3]=%d,  work_out[4]=%d\n", work_out[3], work_out[4]);
	Bconin(2);
	#endif

	/* RÑnder von mm in Pixel umrechnen */
	/* work_out[3] ist Pixelbreite in Mikrometern */
	/* work_out[4] ist Pixelhîhe in Mikrometern */

	xbeg = (int) ((XBEG_MM * 1000L) / work_out[3]);
	ybeg = (int) ((YBEG_MM * 1000L) / work_out[4]);
}


/****************************************************************
*
*
****************************************************************/

static void close_work(void)
{
	wind_update(END_UPDATE);
	if (wind_handle >= 0)
	{
		wind_close(wind_handle);
		wind_delete(wind_handle);
	}
	v_clsvwk(vdi_handle);
	rsrc_free();
	appl_exit();
	Pterm0();
}


/****************************************************************
*
* Initialisierung von ICNEDIT (AES,RSC,VDI,Fenster,Icons,MenÅ).
*
****************************************************************/

void program_init(int err_tail)
{
	/* Applikation beim AES anmelden */
	/* ----------------------------- */
	if ((ap_id = appl_init()) < 0)
		Pterm(1);

	/* VDI - Bildschirmhandle holen */
	/* ---------------------------- */
	vdi_handle = graf_handle(&gl_hwchar, &gl_hhchar, &gl_hwbox, &gl_hhbox);
	open_work();

	/* Mauskontrolle abschalten, MenÅ installieren */
	/* ------------------------------------------- */

	wind_update(BEG_UPDATE);

	/* Resourcedatei laden */
	/* ------------------- */
	if (!rsrc_load("icnedit.rsc"))
	{
		#if LANGUAGE == LANG_GER
		form_alert(1, "[3][Kann \"ICNEDIT.RSC\"|nicht finden][ABBRUCH]");
		#elif LANGUAGE == LANG_ENG
		form_alert(1, "[3][Cannot find \"ICNEDIT.RSC\"][ Abort ]");
		#endif
		close_work();
	}

	/* Cursor abschalten */
	/* ----------------- */
	Cursconf(0, 0);

	/* Bildschirm- Arbeitsbereich berechnen */
	/* ------------------------------------ */

	wind_get_grect(SCREEN, WF_WORKXYWH, &desk_g);
	scrx2 = desk_g.g_x + (desk_g.g_w >> 1);

	wind_g.g_x = -1;		/* Innenbereich des Fensters: ganzer Bildschirm, RÑnder auûerhalb */
	wind_g.g_y = desk_g.g_y;
	wind_g.g_w = desk_g.g_w + 2;
	wind_g.g_h = desk_g.g_h + 2;

	wind_handle = wind_create(0, &wind_g);
	if (wind_handle < 0)
		close_work();

	/* maximal mîgliche Punktgrîûen berechnen */
	/* -------------------------------------- */

	wpix = ((desk_g.g_w >> 1) - xbeg - ICON_W * 3) / ICON_W + 2;
	hpix = (desk_g.g_h - ybeg - 2 * gl_hhchar - ICON_H * 3) / ICON_H + 2;

#if 1
	if ((work_out[4] == 2 * work_out[3]) || (work_out[3] == 169 && work_out[4] == 372))
	{
		/* Die Pixel sind doppelt so hoch wie breit: "ST medium" */
		/* Unter MagiC sind die Maûe 278x556, unter TOS 169x372 */

		if (wpix < hpix + hpix)
			hpix = (wpix >> 1);
		wpix = hpix + hpix;
	}
	else
	{
		/* quadratische Pixel (Normalfall) */

		if (wpix < hpix)
			hpix = wpix;
		wpix = hpix;
	}

#else

	/* Bildschirmgeometrie fÅr die drei Atari-	*/
	/* Auflîsungen berÅcksichtigen			*/
	/* ----------------------------------------- */

	if ((desk_g.g_w == 640 && desk_g.g_h + desk_g.g_y == 400) ||
	    (desk_g.g_w == 320 && desk_g.g_h + desk_g.g_y == 200))
	{
		if (wpix < hpix)
			hpix = wpix;
		wpix = hpix;
	}

	if ((desk_g.g_w == 640 && desk_g.g_h + desk_g.g_y == 200))
	{
		if (wpix < hpix + hpix)
			hpix = (wpix >> 1);
		wpix = hpix + hpix;
	}
#endif
	
	if (wpix < 1 || hpix < 1)
	{
		#if LANGUAGE == LANG_GER
		form_alert(1, "[3][Auflîsung zu gering!][ABBRUCH]");
		#elif LANGUAGE == LANG_ENG
		form_alert(1, "[3][Screen resolution too low!][Abort]");
		#endif
		close_work();
	}

	ybeg += desk_g.g_y;
	rsrc_gaddr(0, HAUPTMEN, &addr_main_menu);
	rsrc_gaddr(0, T_ICONS, &addr_icon_dat);
	addr_icon_pgm = addr_icon_dat + I_PROGRA;
	addr_icon_dat += I_DATEI;

	/* Die beiden Vorschau-Icons positionieren */

	addr_icon_pgm->ob_x = 1;
	addr_icon_dat->ob_x = scrx2 + 1;
	#if 1
	/* Vorschau-Icons unter dem Zeichenbereich */
	addr_icon_dat->ob_y = addr_icon_pgm->ob_y = ybeg + (hpix + 1) * ICON_H + hpix;
	#else
	/* alte Version: Vorschau-Icons am unteren Bildschirmrand */
	addr_icon_dat->ob_y = addr_icon_pgm->ob_y = desk_g.g_h + desk_g.g_y - 37;
	#endif
	addr_icon_dat->ob_height = addr_icon_pgm->ob_height = 36;
	addr_icon_pgm->ob_spec.iconblk->ib_pdata = (int *) pgm_data;
	addr_icon_dat->ob_spec.iconblk->ib_pdata = (int *) dat_data;

	/* MenÅ und Vollbildfenster */

	menu_bar(addr_main_menu, TRUE);
	wind_open(wind_handle, &wind_g);

	if (pgm_fname[0])
	{
		int	hdl;

		hdl = (int) Fopen(pgm_fname, RMODE_RD);
		if ((hdl < 0) || (0x70L != Fread(hdl, 0x70L, pgm_data)))
			err_tail = TRUE;
		if (hdl >= 0)
			Fclose(hdl);
	}

	if (dat_fname[0])
	{
		int	hdl;

		hdl = (int) Fopen(dat_fname, RMODE_RD);
		if ((hdl < 0) || (0x70L != Fread(hdl, 0x70L, dat_data)))
			err_tail = TRUE;
		if (hdl >= 0)
			Fclose(hdl);
	}

	if (err_tail)
	{
		#if LANGUAGE == LANG_GER
		form_alert(1, "[3][Dateiname ungÅltig|oder nicht gefunden!][ABBRUCH]");
		#elif LANGUAGE == LANG_ENG
		form_alert(1, "[3][Filename invalid|or file does not exist!][ Abort ]");
		#endif
	}

	wind_update(END_UPDATE);
}


/****************************************************************
*
* liest das Programm-Icon aus der Datei
*
****************************************************************/

static int read_pgm_icon(int handle)
{
	return (Fread(handle, 0x70L, pgm_data) != 0x70L);
}


/****************************************************************
*
* liest das Datei-Icon aus der Datei
*
****************************************************************/

static int read_dat_icon(int handle)
{
	return (Fread(handle, 0x70L, dat_data) != 0x70L);
}


/****************************************************************
*
* schreibt das Programm-Icon in die Datei
*
****************************************************************/

static int write_pgm_icon(int handle)
{
	return (Fwrite(handle, 0x70L, pgm_data) != 0x70L);
}


/****************************************************************
*
* schreibt das Datei-Icon in die Datei
*
****************************************************************/

static int write_dat_icon(int handle)
{
	return (Fwrite(handle, 0x70L, dat_data) != 0x70L);
}


/****************************************************************
*
* Zeichnet das Fenster <whdl> neu.
*
****************************************************************/

static void upd_wind(int whdl, int ispgm)
{
	int msg[8];
	extern int ap_id;

	/* Der Redraw wird nicht direkt ausgefÅhrt, sondern schickt */
	/* eine Nachricht an die Applikation, um Kollisionen mit    */
	/* von AES erzeugten Redraws zu vermeiden                   */
	/* -------------------------------------------------------- */

	msg[0] = WM_REDRAW;
	msg[1] = ap_id;
	msg[2] = 0;
	msg[3] = whdl;
	msg[4] = (ispgm) ? desk_g.g_x : scrx2;
	msg[5] = desk_g.g_y;
	msg[6] = desk_g.g_w / 2;
	msg[7] = desk_g.g_h;
	appl_write(ap_id, 16, msg);
}


/****************************************************************
*
* Bestimmt die Schnittmenge zwischen zwei Rechtecken
*
****************************************************************/

static int rc_intersect(const GRECT *p1, const GRECT *p2)
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
	return (tw > tx) && (th > ty);
}


/****************************************************************
*
* Zeichne den Namen eines Icons
*
****************************************************************/

static void name_to_scr(int ispgm)
{
	OBJECT *icn;
	const char *name;
	GRECT g;


	icn  = (ispgm) ? addr_icon_pgm : addr_icon_dat;
	name = get_name((ispgm) ? pgm_fname : dat_fname);
	strcpy(icn->ob_spec.iconblk->ib_ptext, name);

	g.g_x = icn->ob_spec.iconblk->ib_xtext + icn->ob_x;
	g.g_y = icn->ob_spec.iconblk->ib_ytext + icn->ob_y;
	g.g_w = icn->ob_spec.iconblk->ib_wtext;
	g.g_h = icn->ob_spec.iconblk->ib_htext;
	if (rc_intersect(&clip_grect, &g))
	{
		objc_draw(icn, 0, 0, &g);
	}
}


/****************************************************************
*
* stellt fest, ob der Iconpunkt (x,y) zum "Rahmen" gehîrt
* und daher auf *status stehen muû
*
****************************************************************/

static int is_fixed_icon_frame(int ispgm, int x, int y, int *status)
{
	*status = FALSE;		/* weiûer Rahmen */
	if (y == 0 || y == 27)
		return TRUE;

	if (ispgm)
	{
		if (x == 0 || x == 31)
			return TRUE;
		if ((x == 29) && (y == 2))
			return TRUE;
	}
	else
	{
		if (x <= 3 || x >= ICON_H)
			return TRUE;
		if (x-y >= 22)
			return TRUE;
		if ((x >= 23) && (y <= 5) && (x != y+21))
			return TRUE;
	}

	*status = TRUE;		/* schwarzer Rahmen */
	if (ispgm)
	{
		if (x == 1 || x == 30 || y == 26 || y <= 3)
			return TRUE;
	}
	else
	{
		if (x == 4 || x == 27 || y == 26 || y == 1)
			return TRUE;
		if ((x == 22) && (y > 0) && (y <= 6))
			return TRUE;
		if ((y == 6) && (x >= 22) && (x <= 27))
			return TRUE;
		if ((y <= 5) && (y > 1) && (x == y+21))
			return TRUE;
	}

	return FALSE;			
}


/****************************************************************
*
* stellt fest, ob der Iconpunkt (x,y) gesetzt ist
*
****************************************************************/

int is_pixel(int ispgm, int x, int y)
{
	char *data;

	data = (ispgm) ? pgm_data : dat_data;
	data += y * 4 + x / 8;
	x %= 8;
	return (0 != (*data & (1 << (7 - x))));
}


/****************************************************************
*
* setzt Farbe des Iconpunktes (x,y) sowohl auf dem Bildschirm
* als auch in den Daten
*
****************************************************************/

void set_pixel(int ispgm, int x, int y, int status, int print)
{
	char *data;
	char *mask;
	int colour,iix,iiy,my_xbeg;
	int pxy[4];
	const OBJECT *icn;
	GRECT g;
	int ix,iy;


	icn = (ispgm) ? addr_icon_pgm : addr_icon_dat;
	data = (char *) icn->ob_spec.iconblk->ib_pdata;	/* Icondaten */
	mask = (char *) icn->ob_spec.iconblk->ib_pmask;	/* Iconmaske */

	my_xbeg = (ispgm) ? xbeg : xbeg + scrx2;
	iiy  = y * 4 + x / 8;
	data += iiy;
	mask += iiy;
	iix = 1 << (7 - x%8);
	if (status)		/* Punkt setzen */
		*data |= iix;
	else
		*data &= ~iix;	/* Punkt lîschen */

	colour = (status) ? BLACK : WHITE;
	vsf_color(vdi_handle, colour);

	if (print)
	{
		pxy[0] = g.g_x = my_xbeg + x * (wpix + 1) + 2;
		pxy[1] = g.g_y = ybeg + y * (hpix + 1) + 2;
		pxy[2] = pxy[0] + wpix - 3;
		pxy[3] = pxy[1] + hpix - 3;
		g.g_w = pxy[2] - pxy[0] + 1;
		g.g_h = pxy[3] - pxy[1] + 1;
		if (rc_intersect(&clip_grect, &g))
			v_bar(vdi_handle, pxy);
	}

	if (*mask & iix)
	{
		ix = icn->ob_spec.iconblk->ib_xicon + icn->ob_x + x;
		iy = icn->ob_spec.iconblk->ib_yicon + icn->ob_y + y;

		if ((ix >= clip_array[0]) && (ix <= clip_array[2]) &&
		    (iy >= clip_array[1]) && (iy <= clip_array[3]))
		{
			pxy[0] = ix;
			pxy[1] = iy;
			pxy[2] = pxy[0];
			pxy[3] = pxy[1];
			v_bar(vdi_handle, pxy);
		}
	}
}


/****************************************************************
*
* Das linke oder rechte Icon zeichnen
*
****************************************************************/

static void icn_to_scr(int ispgm)
{
	register int i,j;
	int my_xbeg;
	int pxy[4];


	graf_mouse(M_OFF, NULL);
	vsf_color(vdi_handle,WHITE);           		/* FÅllfarbe weiû */
	my_xbeg = (ispgm) ? xbeg : xbeg + scrx2;	/* linkes oder rechtes Icon */

	/* weiûe FlÑche */
	pxy[0] = my_xbeg;
	pxy[1] = ybeg;
	pxy[2] = my_xbeg + (wpix + 1) * ICON_W;
	pxy[3] = ybeg + (hpix + 1) * ICON_H;
	v_bar(vdi_handle, pxy);

	/* waagerechte Linien */
	pxy[0] = my_xbeg;
	pxy[2] = my_xbeg + (wpix + 1) * ICON_W;
	for	(i = 0; i <= ICON_H; i++)
	{
		pxy[1] = pxy[3] = ybeg + i * (hpix + 1);
		v_pline(vdi_handle, 2, pxy);
	}

	/* senkrechte Linien */
	pxy[1] = ybeg;
	pxy[3] = ybeg + (hpix + 1) * ICON_H;
	for	(i = 0; i <= ICON_W; i++)
	{
		pxy[0] = pxy[2] = my_xbeg + i * (wpix + 1);
		v_pline(vdi_handle, 2, pxy);
	}

	/* Pixel zeichnen */
	for	(i = 0; i < ICON_H; i++)			/* y */
		for	(j = 0; j < ICON_W; j++)		/* x */
		{
			int status;

			if (is_fixed_icon_frame(ispgm, j, i, &status))
				set_pixel(ispgm, j, i, status, status);
			else
			{
				if (is_pixel(ispgm, j, i))
					set_pixel(ispgm, j, i, TRUE, TRUE);
				else
					set_pixel(ispgm, j, i, FALSE, FALSE);
			}
		}

	/* Dateinamen malen */
	name_to_scr(ispgm);
	graf_mouse(M_ON, NULL);
}


/****************************************************************
*
* Das MenÅ "Datei" ist angewÑhlt werden.
*
****************************************************************/

static void menu_file(int entry)
{
	switch (entry)
	{
		case M_LDPGM:
			if (dirty_pgm && (form_alert(2, alrt_lose_changes) == 2))
				break;

			if (!fsel_open_file(read_pgm_icon, "ICP"))
 			{
				strcpy(pgm_fname, path);
				upd_wind(wind_handle, 1);
				dirty_pgm = 0;
			}
			break;

		case M_LDDAT:
			if (dirty_dat && (form_alert(2, alrt_lose_changes) == 2))
				break;

			if (!fsel_open_file(read_dat_icon, "ICD"))
			{
				strcpy(dat_fname, path);
				upd_wind(wind_handle, 0);
				dirty_dat = 0;
			}
			break;

		case M_SPPGM:
			if (*pgm_fname)
				strcpy(path, pgm_fname);

 			if (!saveas(write_pgm_icon, "ICP"))
			{
				strcpy(pgm_fname, path);
				name_to_scr(TRUE);
				dirty_pgm = 0;
			}
			break;

		case M_SPDAT:
			if (*dat_fname)
				strcpy(path, dat_fname);

 			if (!saveas(write_dat_icon, "ICD"))
			{
				strcpy(dat_fname, path);
				name_to_scr(FALSE);
				dirty_dat = 0;
			}
			break;

		case M_ENDE:
			#if LANGUAGE == LANG_GER
			if (1 != form_alert(1,"[2][Icons sichern und Programm|beenden ?][OK|Abbruch]"))
			#elif LANGUAGE == LANG_ENG
			if (1 != form_alert(1,"[2][Save icons and exit program ?][OK| Abort ]"))
			#endif
				break;

			if (*pgm_fname)
			{
				strcpy(path, pgm_fname);
				if (close_file_dialogue(write_pgm_icon, "ICP"))
				{
					return;
				}
			}

			if (*dat_fname)
			{
				strcpy(path, dat_fname);
				if (close_file_dialogue(write_dat_icon, "ICD"))
				{
					return;
				}
			}

			close_work();
			break;

		case M_ABBRU:
			if (dirty_pgm || dirty_dat)
			{
				if (form_alert(2, alrt_lose_changes) == 2)
					break;
			}

			close_work();
			break;
	}
}


/****************************************************************
*
* Das MenÅ "Editieren" ist angewÑhlt werden.
*
****************************************************************/

static void menu_edit(int entry)
{
	char *data;

	data = NULL;
	switch(entry)
	{
		case M_CLRPGM:
			if (pgm_fname[0])
			{
				dirty_pgm = 1;		/* Icon wurde verÑndert */
			} 
			data = pgm_data;
			break;

		case M_CLRDAT:
			if (dat_fname[0])
			{
				dirty_dat = 1;		/* Icon wurde verÑndert */
			}
			data = dat_data;
			break;
	}

	if (data)
	{
		/* lîsche das jeweilige Icon und male es neu */

		memset(data, 0, 0x70L);
		icn_to_scr(data == pgm_data);
	}
}


/****************************************************************
*
* Zeichnet den Ausschnitt g des Fensters mit Handle <whdl> neu.
*
****************************************************************/

static void _redraw(int whdl, const GRECT *g)
{
	if (whdl != wind_handle)
		return;

	clip_grect = *g;
	clip_array[0] = g->g_x;
	clip_array[1] = g->g_y;
	clip_array[2] = clip_array[0] + g->g_w - 1;
	clip_array[3] = clip_array[1] + g->g_h - 1;
	vs_clip(vdi_handle, TRUE, clip_array);
	vsf_color(vdi_handle, WHITE);
	v_bar(vdi_handle, clip_array);
	icn_to_scr(1);
	icn_to_scr(0);
}


/****************************************************************
*
* Malt das Fenster mit Handle <whdl> im Bereich g_new nach.
*
****************************************************************/

static void redraw(int whdl, const GRECT *g_new)
{
	GRECT g;


	if (whdl != wind_handle)
		return;						/* ungÅltiges Handle */

	graf_mouse(M_OFF, NULL);
	wind_get_grect(whdl, WF_FIRSTXYWH, &g);
	do
	{
		if (rc_intersect(g_new, &g))
		{
			_redraw(whdl, &g);
		}
		wind_get_grect(whdl, WF_NEXTXYWH, &g);
	}
	while(g.g_w > 0);					/* bis Rechteckliste vollstÑndig */

	graf_mouse(M_ON, NULL);

	clip_grect = desk_g;

	clip_array[0] = desk_g.g_x;
	clip_array[1] = desk_g.g_y;
	clip_array[2] = clip_array[0] + desk_g.g_w - 1;
	clip_array[3] = clip_array[1] + desk_g.g_h - 1;
	vs_clip(vdi_handle, FALSE, clip_array);
}


/****************************************************************
*
* Rechnet Bildschirmkoordinaten in Iconkoordinaten um.
* RÅckgabe 1: Programmicon
*	       0: Dateiicon
*         -1: kein Iconpunkt
*
****************************************************************/

static int getxy(int *x, int *y)
{
	int ispgm;
	int nx,ny;


	if (*y <= ybeg || *x <= xbeg)
		return -1;		/* zu hoch oder zu weit links */

	*y -= ybeg;
	ispgm = (*x < scrx2);

	if (ispgm)
		*x -= xbeg;
	else
		*x -= xbeg+scrx2;

	nx = *x % (wpix+1);
	ny = *y % (hpix+1);

	if (!nx || !ny)
		return -1;		/* Gitter erwischt */

	*x /= (wpix+1);
	*y /= (hpix+1);

	if (*x >= ICON_W || *y >= ICON_H)
		return -1;
	else
		return ispgm;
}


/****************************************************************
*
* Mausknopfbedienung
*
****************************************************************/

static void handle_button(int nclicks, const EVNTDATA *ev)
{
	int ispgm;
	int clear;
	int state_prev, state_new;
	int x, y;


	if (nclicks != 1)
		return;

	clear = (ev->bstate & 2);
	x = ev->x;
	y = ev->y;

	ispgm = getxy(&x, &y);
	if (ispgm < 0)
	{
		return;		/* Kein gÅltiger Icon-Pixel */
	}

	/* Den Rahmen darf man nicht Ñndern */
	if (is_fixed_icon_frame(ispgm, x, y, &state_prev))
		return;

	state_prev = is_pixel(ispgm, x, y);
	state_new = !clear;
	if (state_new != state_prev)
	{
		graf_mouse(M_OFF, NULL);
		set_pixel(ispgm, x, y, state_new, TRUE);
		graf_mouse(M_ON, NULL);
		if (ispgm)
		{
			dirty_pgm = 1;
		}
		else
		{
			dirty_dat = 1;
		}
	}
}


/*********************************************************************
*
* Hauptfunktion
*
*********************************************************************/

void main(int argc, char *argv[])
{
	int err_tail;		/* Fehler in Kommandozeile */
	EVNT w_ev;
	int message[16];


	err_tail = FALSE;
	for	(argc--; argc > 0; argc--)
	{
		const char *type;
		char *dst;
		int ispgm;

		strupr(argv[argc]);
		type = strrchr(argv[argc], '.');
		if (type == NULL)
			goto err;
		type++;
		if (!strcmp(type, "ICP"))
			ispgm = TRUE;
		else
		if (!strcmp(type, "ICD"))
			ispgm = FALSE;
		else
		{
			err: err_tail = TRUE;
			continue;
		}

		dst = (ispgm) ? pgm_fname : dat_fname;
		if (dst[0])
			goto err;		/* schon besetzt */

		strcpy(dst, argv[argc]);
	}

	program_init(err_tail);
	graf_mouse(ARROW, NULL);
	for (;;)
	{
		w_ev.mwhich = evnt_multi(MU_KEYBD+MU_BUTTON+MU_MESAG,
						  0x0101,		/* nur Einfachklicks	*/
						  3,			/* l. und r. Knopf, NOT	*/
						  0,			/* l. und r. losgelassen	*/
						  0,NULL,		/* kein 1. Rechteck			*/
						  0,NULL,		/* kein 2. Rechteck			*/
						  message,
						  0L,			/* kein Timer */
						  (EVNTDATA *) &(w_ev.mx),
						  &w_ev.key,
						  &w_ev.mclicks
						 );

		/* Abfrage auf Nachricht im Nachrichtenpuffer */
		/* ------------------------------------------ */

		wind_update(BEG_UPDATE);

		if (w_ev.mwhich & MU_MESAG)
		{
			if (message[0] == MN_SELECTED)
			{
				switch(message[3])
				{
					case MM_ABOUT:
						#if LANGUAGE == LANG_GER
			 			form_alert(1,
			 					"[1]["
								"ICNEDIT fÅr KAOSDESK"
								"|\xbd Andreas Kromke 1989/1994/2026"
								"|Version 2.02"
								"][  OK  ]");
						#elif LANGUAGE LANG_ENG
						form_alert(1,
								"[1]["
								"ICNEDIT for KAOSDESK"
					 			"|\xbd Andreas Kromke 1989/1994/2026"
								"|Version 2.02"
								"][  OK  ]");
						#endif
								break;

					case MM_DATEI:
						menu_file(message[4]);
						break;

					case MM_EDIT:
						menu_edit(message[4]);
						break;
				}

				menu_tnormal(addr_main_menu, message[3], 1);
			}
			else
			if (message[0] == WM_REDRAW)
			{
				redraw(message[3], (GRECT *) (message + 4));
			}
			else
			if (message[0] == WM_TOPPED)
			{
				if (message[3] == wind_handle)
				{
					wind_set_int(wind_handle, WF_TOP, 0);
				}
			}
		}

		/* Abfrage auf BetÑtigung eines Mausknopfs */
		/* --------------------------------------- */

		if (w_ev.mwhich & MU_BUTTON)
		{
			handle_button(w_ev.mclicks, (EVNTDATA *) &(w_ev.mx));
		}

		/* Abfrage auf BetÑtigung einer Taste */
		/* ---------------------------------- */

		if (w_ev.mwhich & MU_KEYBD)
		{
			switch(w_ev.key)
			{
				case 0x1900: menu_file(M_LDPGM);break;
				case 0x2000: menu_file(M_LDDAT);break;
				case 0x1910: menu_file(M_SPPGM);break;
				case 0x2004: menu_file(M_SPDAT);break;
				case 0x1F13: menu_file(M_ENDE);break;
				case 0x1011: menu_file(M_ABBRU);break;
			}
		}

		wind_update(END_UPDATE);

	} /* END FOREVER */
}
