/*****************************************************************************
 *                           B a l l e r b u r g          Modul 1            *
 * Dies ist der Hauptteil von Ballerburg. Die Routinen dieses Teils dienen   *
 * im groûen und ganzen der Steuerung des Programmes, dem Aufruf der Objekt- *
 * bÑume, sowie der AusfÅhrung elementarer Grafikoperationen.                *
 *                                       29.4.87      Eckhard Kruse          *
 *                                                    Reichenbergweg 7       *
 *                                                    D-3302 Weddel          *
 * öberarbeitet: 22.10.89                                                    *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <mt_aes.h>
#include <gemut_mt.h>
#include <vdi.h>
#include <osbind.h>     /* operating system Makros */
#include "baller.h"     /* Namen der Objekte der RSC-Datei */
#include "musik.h"

#define Min(a,b)  ((a)<(b)?(a):(b))
#define Max(a,b)  ((a)>(b)?(a):(b))

#define COLOUR_WHITE 0
#define COLOUR_BLACK 1

#define N_SPIELER  2
#define N_SPIELERNAMEN 6			/* im Dialog "Spielernamen fÅr die Tabelle" */
#define N_FOERDERTUERME 5			/* maximale Anzahl FîrdertÅrme pro Spieler */
#define N_KANONEN 10				/* maximale Anzahl Kanonen pro Spieler */
#define BURGEN_DATEN_MAXLEN 5120	/* maximaler Anzahl Elemente fÅr alle Burgen */
#define N_BURGEN 20					/* maximale Anzahl Burgen */
#define P57       57.296
#define G         0.02
#define bing()    Bconout(2, '\x07');
#define fn()      f = 1 - 2 * (n & 1);

int ap_id;
char *screenptr;		/* von Logbase() */
char *screenbuffer;		/* TODO: auflîsungsunabhÑngig machen */
GRECT desk_g;			/* nutzbarer Bildschirm unterhalb der MenÅleiste */
GRECT screen_g;			/* Gesamtgrîûe des Bildschirms */

int *burgen_daten;				/* hier sind alle Burgen drin, per Malloc() geholt */
int *burgen[N_BURGEN];			/* soviele Burgen sind in BALLER.DAT erlaubt */
int burgen_anzahl;				/* soviele sind drin */


/* Daten fÅr die beiden Spieler */

int spieler_burg[N_SPIELER];	/* Jede Spieler hat seine Burg, Index auf burgen[] */
int spieler_geld[N_SPIELER];
int spieler_pulver[N_SPIELER];
int spieler_kugeln[N_SPIELER];
int spieler_volk[N_SPIELER];
int spieler_steuer[N_SPIELER];
int spieler_koenig[N_SPIELER];			/* TODO: Eine Zahl, die eine Art Status fÅr den Kînig angibt? */
struct { int x,y; } spieler_foerdertuerme[N_SPIELER][N_FOERDERTUERME];	/* bis zu fÅnf FîrdertÅrme pro Spieler */
struct { int x,y,w,p; } spieler_kanonen[N_SPIELER][N_KANONEN];


int tabelle_gewonnen[N_SPIELERNAMEN][10];
char tabelle_spielernamen[N_SPIELERNAMEN][8] =
{
	"Tîlpel","Dummel","Brubbel","Wusel","Brîsel","Toffel"
};

char wind;		/* TODO: Wertebereich sicherstellen */

int *bh;
int zx,zy;  /* Koordinaten des Zieles */

static const GRECT scr640x400 = {0, 0, 640, 400 };
static EVNTDATA ev;
static GRECT fgrect;		/* fx,fy,fw,fh */

OBJECT *rsctree_menu;
OBJECT *rsctree_info;
OBJECT *rsctree_names;
OBJECT *rsctree_options;
OBJECT *rsctree_computer;
OBJECT *rsctree_players;
OBJECT *rsctree_shootparams;
OBJECT *rsctree_castles;
OBJECT *rsctree_move;
OBJECT *rsctree_help1;
OBJECT *rsctree_help2;
OBJECT *rsctree_help3;
OBJECT *rsctree_winner;
OBJECT *rsctree_status;

double vvx,vvy;
int m_buf[8], xy[100];
int ws,wc;
int *bg, zug;
int n;	/* Gerade aktiver Spieler, 0 oder 1. TODO: Bereich sicherstellen und sinnvoll umbenennen */
int oldn;
int p[6];
int max_rund,
      ftx,fty,ftw,fth,
      f_h;
char *l_nam;
char *r_nam;

char f;
/* 0 = Spieler/Spieler (default), 1 = Spieler/Computer, 2 = Computer/Spieler, 3 = Computer/Computer */
char spiel_modus;
char end, txt[4], an_erl, mxin, au_kap;
char cw[2] = {2, 2};
char cx[2] = {1, 1};
char cn[7][8]=
{
	"Tîlpel","Dummel","Brubbel",
	"Wusel","Brîsel","Toffel","RÅpel"
};
char nsp1[22] = "Hugo";
char nsp2[22] = "Emil";

int bx[2], by[2], wx[2], wy[2];		/* irgendwelche Koordinaten */


/*********************** Elementare  Grafikbefehle ***************************/


/* ôffnen der Workstation... */
static int gem_init(void)
{
	WORD dummy;
	int i;

	ap_id = appl_init();
	if (ap_id < 0)
	{
		return -1;
	}
	vdi_handle = graf_handle(&dummy, &dummy, &dummy, &dummy);
	for (i = 0; i < 10; i++)
	{
		work_in[i] = 1;
	}
	work_in[10] = 2;	/* RC-Koordinaten */
	v_opnvwk(work_in, &vdi_handle, work_out);
	if (vdi_handle == 0)
	{
		appl_exit();
		return -2;
	}

	wind_get_grect(SCREEN, WF_WORKXYWH, &desk_g);
	screen_g.g_x = screen_g.g_y = 0;
	screen_g.g_w = desk_g.g_x+desk_g.g_w;
	screen_g.g_h = desk_g.g_y+desk_g.g_h;

	return 0;
}


static void gbox(const GRECT *g, int colour)
{
	int pxy[4];
	pxy[0] = g->g_x;
	pxy[1] = g->g_y;
	pxy[2] = g->g_x + g->g_w - 1;
	pxy[3] = g->g_x + g->g_h - 1;
	vsf_color(vdi_handle, colour);
	v_bar(vdi_handle, pxy);
}


static void color(int a)
{
   vst_color(vdi_handle, a);
   vsl_color(vdi_handle, a);
   vsf_color(vdi_handle, a);
}


/* Zeichnet eine Linie */
static void line(int x1, int y1, int x2, int y2)
{
   xy[0] = x1;
   xy[1] = y1;
   xy[2] = x2;
   xy[3] = y2;
   v_pline(vdi_handle, 2, xy);
}


static void box(int x, int y, int x2, int y2, int c)
{
   color(c);
   xy[0] = x;
   xy[1] = y;
   xy[2] = x2;
   xy[3] = y2;
   v_bar(vdi_handle, xy);
}


static void clr(int x, int y, int w, int h)
{
    box(x, y, x + w - 1, y + h - 1, 0);
}


static void save_screen(void)
{
	graf_mouse(M_OFF, NULL);
	memcpy(screenbuffer, screenptr, 32000);
	graf_mouse(M_ON, NULL);
}


static void restore_screen(void)
{
	graf_mouse(M_OFF, NULL);
	memcpy(screenptr, screenbuffer, 32000);
	graf_mouse(M_ON, NULL);
}


/* Lîscht den gesamten Bildschirm auûer der MenÅleiste */
static void cls(void)
{
	graf_mouse(M_OFF, NULL);		/* graf_mouse() ist schachtelbar */
	gbox(&desk_g, COLOUR_WHITE);	/* Bildschirm unterhalb der MenÅleiste lîschen */
	graf_mouse(M_ON, NULL);
}


/********************** BALLER.TAB laden/speichern ***************************/
static int t_load(void)
{
   if ((f_h = (int) Fopen("BALLER.TAB", 0)) < 0)
   {
       return 1;
   }
   Fread(f_h, 1, &an_erl);
   Fread(f_h, 1, &au_kap);
   Fread(f_h, 1, &mxin);
   Fread(f_h, 3, rsctree_options[MAX_XX].ob_spec.tedinfo->te_ptext);
   Fread(f_h, 2, &max_rund);
   Fread(f_h, sizeof(tabelle_spielernamen), tabelle_spielernamen);
   Fread(f_h, sizeof(tabelle_gewonnen), tabelle_gewonnen);
   Fclose(f_h);
   return 0;
}

static int t_save(void)
{
   if ((f_h = (int) Fcreate("BALLER.TAB", 0)) < 0)
   {
      return 1;
   }
   Fwrite(f_h, 1, &an_erl);
   Fwrite(f_h, 1, &au_kap);
   Fwrite(f_h, 1, &mxin);
   Fwrite(f_h, 3, rsctree_options[MAX_XX].ob_spec.tedinfo->te_ptext);
   Fwrite(f_h, 2, &max_rund);
   Fwrite(f_h, sizeof(tabelle_spielernamen), tabelle_spielernamen);
   Fwrite(f_h, sizeof(tabelle_gewonnen), tabelle_gewonnen);
   Fclose(f_h);
   return 0;
}


/* liest ein char von der Datei */
static char zeichen(void)
{
   char a;
   Fread(f_h, 1, &a);
   return a;
}


/* liest eine Dezimalzahl von der Datei, Kommentare werden Åberlesen */
long rdzahl(void)
{
   char a, sign = 1,rem = 0;    /* wird durch * umgeschaltet, und zeigt damit an, */
                           /* ob man sich in einem Kommentar befindet */
   long val = 0;

   do
      if ((a = zeichen()) == '*')
      {
         rem = !rem;
      }
   while ((a != '-') && (a < '0') || (a > '9') || rem);

   if (a == '-' )
   {
      sign = -1;
      a = zeichen();
   }

   while((a >= '0') && (a <= '9'))
   {
      val *= 10;
      val += a - '0';
      a = zeichen();
   }
   return sign * val;
}


/************************* BALLER.DAT laden **********************************/
/* Die Datei ist textbasiert und kann von Hand editiert werden. */
static int burgen_laden(void)
{
	int *a,j;
	int *a_lim;

	a = burgen_daten;
	a_lim = burgen_daten + BURGEN_DATEN_MAXLEN;
	if ((f_h = (int) Fopen("BALLER.DAT", 0)) < 0)
	{
		return 1;
	}

	burgen_anzahl = 0;
	while ((j = (int) rdzahl()) != -999)
	{
		if ((a >= a_lim) || (burgen_anzahl >= N_BURGEN))
		{
			return 2;
		}

		/* neue Burg anmelden */
		burgen[burgen_anzahl++] = a;

		/* ersten integer-Wert fÅr die Burg lesen */
		*a++ = j;

		/* 40 weitere integer-Werte fÅr die Burg einlesen */
		for (j = 0; j < 40; j++)
		{
			if (a >= a_lim)
			{
				return 2;
			}
			*a++ = (int) rdzahl();
		}

		/* solange weitere Werte lesen, bis -1 oder öberlauf */
		while ((*a++ = (int) rdzahl()) != -1)
		{
			if (a >= a_lim)
			{
				return 2;
			}
		}
	}

	Fclose(f_h);
	return 0;
}


/***************************** Zeichenroutine ********************************/
void draw(int x, int y, int *a)
{
	int i;
	int fil = 1;

	graf_mouse(M_OFF, NULL);
	vsf_interior(vdi_handle, 2);
	vsf_style(vdi_handle, 9);

	while (*a != -1)
	{
		if (*a == -2)
		{
			vsf_interior(vdi_handle, *++a);
			vsf_style(vdi_handle, *++a);
		}
		else
		if (*a == -4)
		{
			fil = !fil;
		}
		else
		{
			i = 0;
			while (*a > -1)
			{
				xy[i++] = x + *a++ * f;
				xy[i++] = y - *a++;
			}

			xy[i++] = xy[0];
			xy[i++] = xy[1];
			if (fil)
			{
				v_fillarea(vdi_handle, i/2, xy);
			}
			else
			{
				v_pline(vdi_handle, i/2 - 1, xy);
			}
		}
		a++;
	}

	graf_mouse(M_ON, NULL);
}


/* Kanone k setzen */
static void init_kanone(int k, int xr)
{
	int x,y;
	static const int kanon[] =
	{
		-2,1,1, 1,0,3,3,5,0,3,3,3,11,2,11,2,9,4,9,4,11,3,11,3,8,0,5,
		3,8,6,5,3,8,3,3,-9, 11,0,13,2,15,0,16,0,17,1,17,2,15,4,19,8,16,11,8,3,-9,
		-1
	};

	draw(x = xr + bg[1 + k * 2] * f, y = by[n & 1] - bg[2 + k * 2], kanon);
	if (~n & 2)
	{
		spieler_kanonen[n][k].x = x - 20 * n;
		spieler_kanonen[n][k].y = y;
		spieler_kanonen[n][k].w = 45;		/* Winkel */
		spieler_kanonen[n][k].p = 12;		/* Pulvermenge */
	}
}


/* w: Geld (0), Pulver (1) oder Kugeln (2) zeichnen */
static void drw_gpk(char w)
{
	int i,z, x,y, xr,yr, *a,xp,yp, m=n&1;
	static const int sack[] =
	{
		-4, 1,1,1,4,2,5,2,0,3,0,3,8,2,8,4,8,3,8,3,6,4,6,4,0,5,0,
		5,6,6,5,6,1,-9, -1
	};
	static const int fass[] =
	{
		-4, 2,0,6,0,8,3,8,5,6,8,2,8,0,5,0,3,2,0,3,0,3,3,2,4,3,5,3,8,
		5,8,5,5,6,4,5,3,5,1,-9, -1
	};
	static const int kuge[] =
	{
		-2,1,1, 1,0,0,1,0,3,1,4,3,4,4,3,4,1,3,0,-9,-1
	};

	switch( w )
	{
		case 0:
			a = sack;
			xp = 7;
			yp = 10;
			i = (spieler_geld[m] + 149) / 150;
			break;
		case 1:
			a = fass;
			xp = yp = 9;
			i = (spieler_pulver[m] + 29) / 30;
			break;
		case 2:
			a = kuge;
			xp = yp = 6;
			i = spieler_kugeln[m];
	}

	bg = burgen[spieler_burg[m]];
	w *= 2;
	xr = (n & 2 ? bx[m] : 639 * m) + f * bg[25 + w];
	yr = by[m] - bg[26 + w];

	graf_mouse(M_OFF, NULL);
	clr(xr - bg[31 + w] * m - !n, yr - bg[32 + w], bg[31 + w] + 1, bg[32 + w] + 1);
	color(1);

	for (y = z = 0; i > 0 && y < bg[32 + w]; y += yp)
		for (x = 0; i > 0 && x < bg[31 + w]; x += xp, z++,i--)
			draw(xr + f * x, yr - y, a);

	graf_mouse(M_ON, NULL);
	if (i > 0)    /* Maximalbetrag Åberschritten ? */
		switch (w)
		{
			case 0:
				spieler_geld[m] = z * 150;
				break;
			case 2:
				spieler_pulver[m] = z * 30;
				break;
			case 4:
				spieler_kugeln[m] = z;
		}
}


/* Geld, Pulver und Kugeln zeichnen */
static void drw_all(void)
{
   drw_gpk(0);
   drw_gpk(1);
   drw_gpk(2);
}


/***************************** Burg zeichnen *********************************/
/* Der Berg ist hier schon gezeichnet worden */
static void burg(int nn)
{
	/***************** Grafikdaten fÅr Thron, Kanone... **************************/
	static const int thron[] =
	{
		-2,2,2, 8,3,5,12,8,12,12,16,12,17,14,15,
		15,15,17,17,17,16,21,11,24,11,21,3,-9, -4, 3,0,5,2,24,2,26,0,-9,
		19,4,18,9,-9, 10,4,11,9,-9, 14,4,15,4,-9, 8,10,11,11,-9, 21,10,18,11,-9,
		14,17,15,17,-9, 13,19,13,19,-9, 16,19,16,19,-9, 12,22,12,21,15,21,14,22,
		17,21,17,22,-9, 6,13,6,17,5,16,5,17,7,17,7,16,-9, -1
	};
	int i,xr;

	oldn = n;		/* TODO: Das Chaos entwirren */
	n = nn;
	fn();
	xr = n & 2 ? (bx[n &= 1]) : (639 * n);
	bg = burgen[spieler_burg[n]];		/* Das ist die gerade aktive Burg, linke oder rechte. */
	spieler_geld[n] = bg[37];			/* Die Burg legt das Anfangskapital fest. */
	spieler_pulver[n] = bg[38];
	spieler_kugeln[n] = bg[39];
	spieler_volk[n] = bg[40];

	/* das GemÑuer und die beiden dreieckigen Turmspitzen */
	color(COLOUR_BLACK);
	vswr_mode(vdi_handle, MD_ERASE);	/* reverse transparent */
	draw(xr, by[n], &bg[45]);

	/* Stanze ein weiûes Loch in die Burg, wo spÑter der Kînig sitzt */
	vswr_mode(vdi_handle, MD_REPLACE);
	clr(xr + f * bg[21] - n * 30, by[n] - bg[22] - 25, 30, 25);

	/* Der Kînig auf dem Thron */
	color(COLOUR_BLACK);
	draw(xr + f * bg[21], by[n] - bg[22], thron);

	/* Malt die normalerweise zwei Kanonen */
	n = nn;
	for (i = 0; (i < N_KANONEN) && (bg[i * 2 + 1] > -1); i++)
	{
		init_kanone(i, xr);
	}

	/* Malt Geld, Pulver und Kugeln */
	drw_all();
	n = oldn;
}


/**************************** Neues Bild zeichnen ****************************/
static void bild(void)
{
	int y,x1,x2,v1,v2;

	graf_mouse(M_OFF, NULL);
	cls();

	by[0] = (int) (Random() % 80 + 300 & ~3);
	by[1] = (int) (Random() % 80 + 300 & ~3);

	y = 400;
	x1 = 0;
	x2 = 2556;
	color(COLOUR_BLACK);
	v1 = v2 = 2;

	/* Den Berg zeichnen */

	vsl_type(vdi_handle, USERLINE);

	while ((x1 < x2) && (--y > 20))
	{
		vsl_udsty(vdi_handle, ~((257 << (Random() & 7)) * (y & 1)));
		line(x1 / 4, y, x2 / 4, y);
		if (y == by[0])
		{
			x1 = *burgen[spieler_burg[0]] * 4;
		}
		if (y == by[1])
		{
			x2 = (639 - *burgen[spieler_burg[1]]) << 2;
		}
		if (x1)
		{
			v1 = (int) (v1 + Random() % 5 - 2);
			v1 = Max(0, v1);
			v1 = Min(7, v1);
			x1 += v1;
		}
		if (x2 < 2556)
		{
			v2 = (int) (v2 + Random() % 5 - 2);
			v2 = Max(0, v2);
			v2 = Min(7, v2);
			x2 -= v2;
		}
	}

	/* Burgen und Text zeichnen */

	vsl_type(vdi_handle, SOLID);
	memset(spieler_kanonen, -1, sizeof(spieler_kanonen));
	burg(0);	/* linke Burg */
	burg(1);	/* rechte Burg */
	v_gtext(vdi_handle, 276, 395, " Runde     ");
	graf_mouse(M_ON, NULL);
}


/******************** Initialisierung vor neuem Spiel ************************/
static void neues_spiel(void)
{
	static int pr[6] = { 200,500,400,150,50,50 };   /* Preise zu Beginn */
	int i;

	wind = Random() % 60 - 30;
	spieler_steuer[0] = spieler_steuer[1] = 20;		/* 20% Steuersatz*/
	spieler_koenig[0] = spieler_koenig[1] = 0;

	for (i = 0; i < 6; i++)
	{
		p[i] = (int) (pr[i] * (95 + Random() % 11) / 100);
	}

	bild();

	for (i = 0; i < N_SPIELER; i++)
	{
		bg = burgen[spieler_burg[i]];
		wx[i] = i ? (639 - bg[23]) : bg[23];
		wy[i] = by[i] - bg[24];
		for (f = 0; f < N_FOERDERTUERME;)
		{
			spieler_foerdertuerme[i][f++].x = -1;	/* noch kein Fîrderturm */
		}
	}

	zug = n = end = 0;
	f = 1;
}


void z_txt(int a)
{
   txt[0]=a/100+48; txt[1]=a%100/10+48; txt[2]=a%10+48;
   if(a<100) { txt[0]=32; if(a<10) txt[1]=32; }
   for(a=0;a<3;a++) txt[a]=txt[a]==48? 79: txt[a]==49? 108: txt[a];
}


/******************* Darstellung der beiden Windfahnen in den Burgen ***********************/
static void fahne(void)
{
	char m = -1;

	while (++m < 2)		/* TODO: Soll wohl heiûen m=0 und m = 1 */
		if (wx[m] > -1)
		{
			clr(wx[m] - 10, wy[m] - 15, 20, 15);
			color(COLOUR_BLACK);
			line(wx[m], wy[m], wx[m], wy[m] - 15);		/* Das mÅûte der Flaggenstock sein. */

			if (m == n)
			{
				/* Nur die "aktive" Burg bekommt auch die Fahne. */
				line(wx[m], wy[m] - 15, wx[m] + ws, wy[m] - 13 + wc);
				line(wx[m], wy[m] - 11, wx[m] + ws, wy[m] - 13 + wc);
			}
		}
}


/** Anzeige des Spielers, der am Zug ist, sowie Darstellung der Windfahnen ***/
static void werdran(char c)
{
	int *a,i,x,y,w,h,c1,s1,c2,s2;
	char *ad;
	double wk,wl;

	/* zeichne die Nummer der Runde unten in die Mitte */

	z_txt(zug / 2 + 1);
	v_gtext(vdi_handle, 332, 395, txt);

	a = &rsctree_move[0].ob_x;		/* Der rechteckige Dialog mit den Daten des nÑchsten Zugs */
	if (c)
	{
		a[0] = 5 + (629 - a[2]) * n;
		a[1] = 25;
		rsctree_move[DNAM].ob_spec.free_string = n ? r_nam : l_nam;
		ad = rsctree_move[DWIN].ob_spec.free_string;
		ad[0] = ad[5] = 4 + 28 * !wind - (wind > 0);
		i = (wind < 0) ? -wind : wind;
		ad[2] = 48 + i/10;
		ad[3] = 48 + i%10;
		if (wx[n] < 0)
		{
			ad[0] = ad[5] = (char) 32;
			ad[2] = ad[3] = '?';
		}
		objc_offset(rsctree_move, DOK, &ftx, &fty);
		ftw = rsctree_move[DOK].ob_width;
		fth = rsctree_move[DOK].ob_height;
		objc_offset(rsctree_move, DWBX, &x, &y);
		w = rsctree_move[DWBX].ob_width;
		h = rsctree_move[DWBX].ob_height;

		objc_draw(rsctree_move, 0, 4, &desk_g);		/* male links oder rechts oben den Dialog fÅr den Zug */

		c = (wind > 0) ? 1 : -1;
		wk = c * wind/15.0;
		wl = wk*.82;
		if (wk > 1.57)
		{
			wk = 1.57;
		}
		if (wl > 1.57)
		{
			wl=1.57;
		}
		s1 = c * 20 * sin(wk);
		c1 = 20 * cos(wk);
		s2 = c * 20 * sin(wl);
		c2 = 20 * cos(wl);
		ws = s1 / 2.0;
		ws += !ws;
		wc = c1 / 2.0;

		graf_mouse(M_OFF, NULL);
		if (wx[n] > -1)
		{
			color(1);
			x += w/2;
			line(x,    y + h, x    , y + 5);		/* male den senkrechten Flaggenstock in den Dialog */
			line(x + 1,y + h, x + 1, y + 5);
			xy[0] = xy[2] = x + 1;
			xy[1] = y + 5;
			xy[3] = y + 11;
			if (wk < .2)
			{
				xy[0] = x - 1;
				xy[1] = xy[3] = y + 5;
				xy[2] = x + 2;
			}
			xy[ 4] = xy[2] + s1;
			xy[ 5] = xy[3] + c1;
			xy[ 8] = xy[0] + s1;
			xy[9]  = xy[1] + c1;
			xy[10] = xy[0];
			xy[11] = xy[1];
			xy[ 6] = ((xy[4] + xy[8]) >> 1) + s2;
			xy[ 7] = ((xy[5] + xy[9]) >> 1) + c2;
			v_pline(vdi_handle, 6, xy);			/* hÑnge die Fahne an den Flaggenstock im Dialog */
		}

		fahne();	/* Male die Fahne in der aktiven Burg und nur den Flaggenstock in der inaktiven */
		graf_mouse(M_ON, NULL);
	}
	else
	{
		graf_mouse(M_OFF, NULL);
		clr(a[0], a[1], a[2], a[3]);
		graf_mouse(M_ON, NULL);
	}
}


/************** Darstellung und Verwaltung eines Objektbaumes ****************/
static int obj_do(OBJECT *tree)
{
#if 1
	return do_dialog(tree);		/* aus GEMUT_MT fÅr MagiC, rettet Hintergrund */
#else
	int ret;
	int fl = 0;
	static GRECT g1 = {20, 20, 30, 20};

	if (tree == rsctree_names)
	{
		fl = NSP1;
		g1.g_x = 80;
	}
	if (tree == rsctree_options)
	{
		fl = MAX_XX;
		g1.g_x = 80;
	}
	if (tree == rsctree_computer)
	{
		g1.g_x = 150;
	}
	if (tree == rsctree_players)
	{
		fl = EI_N1;
		g1.g_x = 260;
	}

	/* Man beachte, daû wir hier FMD_START und FMD_FINISH weglassen, weil wir
	   derzeit keine WM_REDRAW-Nachricht auslîsen mîchte, die wir nicht behandeln. */

	form_center(tree, &fgrect);
	save_screen();
	form_dial(FMD_GROW, &g1, &fgrect);
	objc_draw(tree, 0, 5, &desk_g);
	ret = form_do(tree, fl);
	tree[ret].ob_state = 0;		/* SELECTED lîschen */
	restore_screen();
	form_dial(FMD_SHRINK, &g1, &fgrect);
	return ret;
#endif
}


/********************* Neues Spiel: Auswahl der Burgen ***********************/
static int bur_obj(void)
{
	static const GRECT g1 = {70, 20, 30, 20};

	int i,ob0,ob1,oy0,oy1, x,y,w,h, ol[8];

	oy0=by[0]; oy1=by[1]; ob0=spieler_burg[0]; ob1=spieler_burg[1];

	rsctree_castles[BSP1].ob_spec.free_string = l_nam;
	rsctree_castles[BSP2].ob_spec.free_string = r_nam;

	form_center(rsctree_castles, &fgrect);

	w = rsctree_castles[BK1].ob_width;
	h = rsctree_castles[BK1].ob_height;
	objc_offset(rsctree_castles, BK1, &x, &y );
	bx[0] = x;
	by[0] = y + h;
	objc_offset(rsctree_castles, BK2, &x, &y);
	bx[1] = x + w;
	by[1] = y+h;

	/* Wir lassen FMD_START und FMD_FINISH weg, weil wir kein WM_REDRAW verarbeiten. */
	save_screen();
	form_dial(FMD_GROW, &g1, &fgrect);
	objc_draw(rsctree_castles, 0, 5, &desk_g);

	/* alte Werte sichern */
	ol[0] = spieler_geld[0];
	ol[1] = spieler_geld[1];
	ol[2] = spieler_pulver[0];
	ol[3] = spieler_pulver[1];
	ol[4] = spieler_kugeln[0];
	ol[5] = spieler_kugeln[1];
	ol[6] = spieler_volk[0];
	ol[7] = spieler_volk[1];

	/* TODO: sind Werte ungleich Null oder Eins erlaubt? */
	burg(2);
	burg(3);

	do
	{
		spieler_geld[0] = spieler_geld[1] = spieler_pulver[0] = spieler_pulver[1] = spieler_kugeln[0] = spieler_kugeln[1] = 9999;
		i = form_do(rsctree_castles, 0);
		rsctree_castles[i].ob_state = 0;

		if ((i == BL1) || (i == BR1))
		{
			spieler_burg[0] = (spieler_burg[0] + 1 - 2 * (i == BL1) + burgen_anzahl) % burgen_anzahl;
			clr(bx[0], y, w, h);
			burg(2);
		}

		if ((i == BL2) || (i == BR2))
		{
			spieler_burg[1] = (spieler_burg[1] + 1 - 2 * (i == BL2) + burgen_anzahl) % burgen_anzahl;
			clr(bx[1] - w, y, w, h);
			burg(3);
		}
	} while ((i != BOK) && (i != BAB));

	spieler_geld[0] = ol[0];
	spieler_geld[1] = ol[1];
	spieler_pulver[0] = ol[2];
	spieler_pulver[1] = ol[3];
	spieler_kugeln[0] = ol[4];
	spieler_kugeln[1] = ol[5];
	spieler_volk[0] = ol[6];
	spieler_volk[1] = ol[7];

	restore_screen();
	form_dial(FMD_SHRINK, &g1, &fgrect);

	fn();
	by[0] = oy0;
	by[1] = oy1;

	if (i == BAB)
	{
		spieler_burg[0] = ob0;
		spieler_burg[1] = ob1;
	}

	return i == BOK;
}


/**************************** Tabelle ****************************************/
static void tabelle(void)
{
	int i,j;
	graf_mouse(M_OFF, NULL);
	vsf_interior(vdi_handle,0);
	box(53, 56, 587, 343, 1);
	box(55, 58, 585, 341, 1);
	box(56, 59, 584, 340, 1);

	line(144, 59, 144, 340);

	for (i = 152; i < 584; i += 72)
	{
		line(i, 59, i, 340);
	}

	line(56, 84, 584, 84);

	for (i = 92; i < 240; i += 24)
	{
		line(56, i, 584, i);
	}

	for (i = 244; i < 340; i += 24)
		line(56, i, 584, i);

	for (i = 0; i < N_SPIELERNAMEN; i++)
		v_gtext(vdi_handle, 160 + i * 72, 78, tabelle_spielernamen[i]);

	for (i = 0; i < N_SPIELERNAMEN; i++)
		v_gtext(vdi_handle, 80, 110 + i * 24, tabelle_spielernamen[i]);

	vsf_interior(vdi_handle, 2);
	vsf_style(vdi_handle, 2);

	for (i = 0; i < N_SPIELERNAMEN; i++)
	{
	   for (j = 0; j < 10; j++)
	   {
	      z_txt(tabelle_gewonnen[i][j]);
	      if ((j == 9) && (!tabelle_gewonnen[i][6]))
	      {
	      	txt[0] = 32;
	      	txt[1] = '-';
	      	txt[2] = 0;
	      }
	      v_gtext(vdi_handle, 176 + i * 72, 110 + j * 24 + 8 * (j > 5), txt);
	      if (i == j)
	      {
	      	/* BUG: The colour parameter was missing */
	         box(152 + i * 72, 92 + j * 24, 224 + i * 72, 116 + j * 24, 1);
	      }
	   }
	}

	v_gtext(vdi_handle, 64, 262, "\344  Spiele");
	v_gtext(vdi_handle, 64, 286, "\344gewonnen");
	v_gtext(vdi_handle, 64, 310, "\344verloren");
	v_gtext(vdi_handle, 64, 334, "Siege in%");
	line(56,59,144,84);
	vst_height(vdi_handle, 4, &i, &i, &i, &i);
	v_gtext(vdi_handle, 60, 81, "VERLOREN");
	v_gtext(vdi_handle, 92, 66, "GEWONNEN");
	vst_height(vdi_handle, 13, &i, &i, &i, &i);
	graf_mouse(M_ON, NULL);
}


static void handle_menu_options(void)
{
	rsctree_options[A_ERL].ob_state  = an_erl;
	rsctree_options[A_VER].ob_state  = !an_erl;
	rsctree_options[MAX_1].ob_state  = !mxin;
	rsctree_options[MAX_2].ob_state  = (mxin == 1);
	rsctree_options[MAX_XX].ob_state = (mxin == 2);
	rsctree_options[MAX_NE].ob_state = (mxin == 3);
	rsctree_options[A_KAP].ob_state  = au_kap;

	if (obj_do(rsctree_options) == OP_OK)
	{
		char *h;
		au_kap = rsctree_options[A_KAP].ob_state;
		an_erl = rsctree_options[A_ERL].ob_state;
		mxin   =     rsctree_options[MAX_2].ob_state +
		         2 * rsctree_options[MAX_XX].ob_state +		/* TODO: Why this? */
		         3 * rsctree_options[MAX_NE].ob_state;
		h = rsctree_options[MAX_XX].ob_spec.tedinfo->te_ptext;
		max_rund = h[0] - '0';
		if (h[1])
		{
			max_rund = max_rund * 10 + h[1] - '0';
			if (h[2])
		    	max_rund = max_rund * 10 + h[2] - '0';
		}
		if (!mxin)
			max_rund = 20;
		if (mxin == 1)
			max_rund = 50;
		if (mxin == 3)
			max_rund = 32767;
	}
}


static void handle_menu_choose_computer(void)
{
	obj_do(rsctree_computer);
	for (cw[0] = 0; cw[0] <7 ; cw[0]++)
		if (rsctree_computer[CN1 + cw[0]].ob_state)
			break;

	for (cw[1]=0; cw[1]<7; cw[1]++ )
		if (rsctree_computer[CN2 + cw[1]].ob_state)
			break;

	for (cx[0] = 0; cx[0] < 4; cx[0]++)
		if (rsctree_computer[CTS1 + cx[0]].ob_state)
			break;
	for (cx[1] = 0; cx[1] < 4; cx[1]++)
		if (rsctree_computer[CTS2 + cx[1]].ob_state)
			break;
}


/* */
static void handle_menu_help(void)
{
	static const GRECT g1 = {190, 20, 30, 20};
	int a = 0, b;
	OBJECT *c;

	/* Wir lassen hier FMD_START und FMD_FINISH weg, weil wir keine WM_REDRAW-Nachricht wollen */
	form_center(rsctree_help1, &fgrect);
	form_center(rsctree_help2, &fgrect);
	form_center(rsctree_help3, &fgrect);
	save_screen();
	form_dial(FMD_GROW, &g1, &fgrect);
	do
	{
		c = a ? ((a < 2) ? rsctree_help2 : rsctree_help3) : rsctree_help1;
		objc_draw(c, 0, 5, &desk_g);
		b = form_do(c, 0);
		*(int *)(c + 24 * b + 10) = 0;
		if( a==1 && b==R21 || a==2 && b==R31 ) { b=-1; a=0; }
		if( a==0 && b==R12 || a==2 && b==R32 ) { b=-1; a=1; }
		if( a==0 && b==R13 || a==1 && b==R23 ) { b=-1; a=2; }
	} while(b < 0);

	restore_screen();
	form_dial(FMD_FINISH, &g1, &fgrect);
}


static void handle_menu_player_names(void)
{
	memcpy(rsctree_players[EI_N1].ob_spec.tedinfo->te_ptext, tabelle_spielernamen[0], 8);
	memcpy(rsctree_players[EI_N2].ob_spec.tedinfo->te_ptext, tabelle_spielernamen[1], 8);
	memcpy(rsctree_players[EI_N3].ob_spec.tedinfo->te_ptext, tabelle_spielernamen[2], 8);
	memcpy(rsctree_players[EI_N4].ob_spec.tedinfo->te_ptext, tabelle_spielernamen[3], 8);
	memcpy(rsctree_players[EI_N5].ob_spec.tedinfo->te_ptext, tabelle_spielernamen[4], 8);
	memcpy(rsctree_players[EI_N6].ob_spec.tedinfo->te_ptext, tabelle_spielernamen[5], 8);

	rsctree_players[ER_LOE].ob_state = 0;
	rsctree_players[ER_BLE].ob_state = SELECTED;

	if (obj_do(rsctree_players) == ER_OK)
	{
		int i,j;
		memcpy(tabelle_spielernamen[0], rsctree_players[EI_N1].ob_spec.tedinfo->te_ptext, 8);
		memcpy(tabelle_spielernamen[1], rsctree_players[EI_N2].ob_spec.tedinfo->te_ptext, 8);
		memcpy(tabelle_spielernamen[2], rsctree_players[EI_N3].ob_spec.tedinfo->te_ptext, 8);
		memcpy(tabelle_spielernamen[3], rsctree_players[EI_N4].ob_spec.tedinfo->te_ptext, 8);
		memcpy(tabelle_spielernamen[4], rsctree_players[EI_N5].ob_spec.tedinfo->te_ptext, 8);
		memcpy(tabelle_spielernamen[5], rsctree_players[EI_N6].ob_spec.tedinfo->te_ptext, 8);
		if (rsctree_players[ER_LOE].ob_state)
		{
			for (i = 0; i < N_SPIELERNAMEN; i++)
			{
				for (j = 0; j < 10; j++)
				{
					tabelle_gewonnen[i][j] = 0;
				}
			}
		}
	}
}


static int handle_menu(void)
{
	static const GRECT g2 = {260, 20, 30, 20};
	static const GRECT g3 = {48, 52, 548, 308};

	wind_update(BEG_MCTRL);
	if (m_buf[4] == MN_INFO)
	{
		obj_do(rsctree_info);
	}
	else
	if (m_buf[4] == SPI_NEU)     /* Neues Spiel */
	{
		if (bur_obj())
		{
			neues_spiel();
			werdran(1);
		}
	}
	else
	if (m_buf[4] == SPI_NAM)     /* Namen eingeben */
	{
		memcpy(rsctree_names[NSP1].ob_spec.tedinfo->te_ptext, nsp1, 20);
		memcpy(rsctree_names[NSP2].ob_spec.tedinfo->te_ptext, nsp2, 20);
		
		if (obj_do(rsctree_names) == NOK)
		{
			memcpy(nsp1, rsctree_names[NSP1].ob_spec.tedinfo->te_ptext, 20);
			memcpy(nsp2, rsctree_names[NSP2].ob_spec.tedinfo->te_ptext, 20);
			if (!end)
			{
				werdran(1);
			}
		}
	}
	else
	if (m_buf[4] == SPI_OPT) /* Optionen */
	{
		handle_menu_options();
	}
	else
	if (m_buf[4] == SPI_ENDE)
	{
		wind_update(END_MCTRL);
		return 1;
	}
	else
	if ((m_buf[4] >= MOD1) && (m_buf[4] <= MOD4))
	{
		/* Spielmodus (wer gegen wen) */
		menu_icheck(rsctree_menu, MOD1 + spiel_modus, 0);
		spiel_modus = m_buf[4] - MOD1;
		menu_icheck(rsctree_menu, m_buf[4], 1);
	}
	else
	if (m_buf[4] == MOD5)
	{
		/* Computer auswÑhlen */
		handle_menu_choose_computer();
	}
	else if (m_buf[4] == REG)     /* Spielregeln */
	{
		handle_menu_help();
	}
	else
	if (m_buf[4] == ERG1)
	{
		handle_menu_player_names();
	}
	else if( m_buf[4]==ERG2 )
	{
		if (t_load())
		{
			form_alert(1,"[1][Kann BALLER.TAB nicht laden!][Abbruch]" );
		}
	}
	else
	if (m_buf[4] == ERG3)
	{
		t_save();
	}
	else
	if (m_buf[4] == ERG4)
	{
		save_screen();
		form_dial(1, &g2, &g3);
		tabelle();
		ev.bstate = 0;
		while(!ev.bstate)
		{
			graf_mkstate(&ev);
		}
		restore_screen();
		form_dial(2, &g2, &g3);
	}
	if ((m_buf[4] >= MOD1) && (m_buf[4] <= MOD5))
	{
		if (spiel_modus < 2)
		{
			/* links ist ein Spieler */
			l_nam = nsp1;
		}
		else
		{
			/* links ist ein Computer */
			l_nam = cn[cw[0]];
		}

		if (spiel_modus & 1)
		{
			/* rechts ist ein Computer */
			r_nam = cn[cw[1]];
		}
		else
		{
			/* rechts ist ein Spieler */
			r_nam = nsp2;
		}

		if (!end)
		{
			werdran(1);
		}
	}

	menu_tnormal(rsctree_menu, m_buf[3], 1);
	wind_update(END_MCTRL);
	return 0;
}


/* Verwaltung von Ereignissen: Messages, Maus oder Timer */
static int event(void)
{
	int wh;
	int keycode = 0;
	int dummy;
	static const GRECT dummy_g = {0, 0, 0, 0};

	wh = evnt_multi(
					MU_TIMER + MU_MESAG + MU_KEYBD + MU_BUTTON,		/* evtypes */
					1,1,1,	/* nclicks, bmask, bstate */
					0, &dummy_g,
					0, &dummy_g,
					m_buf,	/* message buffer */
					300,		/* milliseconds */
					&ev,		/* x, y, bstate, kstate */
					&keycode,
					&dummy	/* nbclicks */
					);
	if (wh & MU_KEYBD)
	{
		if ((keycode & 0xff) == 17)		/* Ctrl-Q */
			return 1;
	}

	if ((wh & MU_MESAG) && (m_buf[0] == MN_SELECTED))
	{
		return handle_menu();
	}
	
	if (wh & MU_BUTTON)
	{
		/*printf("button\n");*/
	}
	else
	{
		ev.bstate = 0;
	}

	return 0;
}


/* Test, ob Koord. innerhalb eines Rechteckes */
static int drin(int xk, int yk, int w, int h, int r, int x, int y)
{
	if (n)
	{
		xk = 639 - xk - w;
	}
	return (x > xk - r) && (x < xk + w + r) && (y < by[n] - yk + r) && (y > by[n] - yk - h - r);
}


/* fÅnfstellige Zahl, rechtsbÅndig, ohne fÅhrende Nullen */
static void zahl(int nr, int wert )
{
	short i,a,b;
	char *adr;

	adr = rsctree_status[nr].ob_spec.free_string + 11;
	for (b = i = 0, a = 10000; i < 5; i++, a /= 10 )
	{
		*adr++ = 48 + wert / a - 16 * ((wert < a) && (i < 4) && (!b));
		b |= wert / a;
		wert %= a;
	}
}


/* Ermittelt, ob Punkt gesetzt ist */
int loc(int x, int y)
{
	int pel;
	int index;

	/* pel ist der Inhalt des Bildspeichers,
	   bei TrueColour die unteren 16 Bit */
	/* index ist der Farbindex, bei HighColour -1 und bei
	   TrueColour die oberen 16 Bit */
	v_get_pixel(vdi_handle, x, y, &pel, &index);

	return pel;		/* TODO: an alle Farbmodi anpassen */
}


/* Anbauen */
static void anbau(void)
{
	short s;
	char a[3];

	color(1);
	vsf_interior(vdi_handle, 2);
	vsf_style(vdi_handle, 9);
	v_gtext(vdi_handle, 280, 375, " Anbauen: " );
	v_gtext(vdi_handle, 220, 395, " Verbleibende Steine: 20 ");
	s = 20;
	graf_mouse(6,0);

	do
	{
		graf_mkstate(&ev);
		if ((ev.bstate) && (n ? (ev.x > 624 - bg[0]) : (ev.x < bg[0] + 15) && (ev.y > 155)))
		{
			graf_mouse(M_OFF, NULL);
			if (!(loc(ev.x, ev.y)      || loc(ev.x + 1, ev.y + 1) || loc(ev.x - 1, ev.y - 1)) &&
			  (loc(ev.x + 3, ev.y - 1) || loc(ev.x + 3, ev.y + 1) || loc(ev.x - 3, ev.y - 1) || loc(ev.x - 3, ev.y + 1)||
			   loc(ev.x + 1, ev.y + 2) || loc(ev.x - 1, ev.y + 2) || loc(ev.x + 1, ev.y - 2) || loc(ev.x - 1, ev.y - 2) ))
			{
				vswr_mode(vdi_handle, MD_ERASE);
				xy[0] = ev.x - 2;
				xy[1] = ev.y - 1;
				xy[2] = ev.x + 2;
				xy[3] = ev.y + 1;
				v_bar(vdi_handle, xy);
				vswr_mode(vdi_handle, MD_REPLACE);
				s--;
				a[0] = 48 + s / 10;
				a[1] = 48 + s % 10;
				a[2] = 0;
				v_gtext(vdi_handle, 396, 395, a);
			}
			graf_mouse(M_ON, NULL);
		}
	} while((s > 0) && (ev.bstate < 2));

	/* TODO: auflîsungsunabhÑngig machen */
	for (s = 360; s < 400; s++)
	{
		/* in 640x400x1: 80 Bytes pro Zeile */
		/* Bildschirmzeilen 360..399, Spalten 208..319 aus dem Puffer in den Bildschirm kopieren */
		memmove(screenptr + s * 80 + 26, screenbuffer + s * 80 + 26, 28);
	}

	graf_mouse(0, 0);
}


/********************* Von Markt aufzurufende Routinen ***********************/
static void baue_foerderturm(void)
{
	int x,y,yy,i,t;
	static const int turm[] =
	{
		-4, 23,30,27,0,3,10,24,20,6,30,23,30,5,20,26,10,2,0,6,30,-9,
		8,0,8,31,9,34,11,36,14,37,15,37,18,36,20,34,21,31,21,29,20,26,18,24,15,23,
		14,23,11,24,9,26,8,29,-9, 14,23,15,37,-9, 10,25,19,35,-9, 10,35,19,25,-9,
		21,30,21,0,-9, -1
	};

	graf_mouse(M_OFF, NULL);

	for (t = 0; t < N_FOERDERTUERME; t++)
		if (spieler_foerdertuerme[n][t].x == -1)
			break;

	x = 639 * n + f * (bg[0] + 20 + 30 * t);
	y = 380;
	do
	{
		while (loc(x, y) && loc(x + 29 * f, y--))
			;

		yy = y;
		for (i = 0; i < 40; i++,y--)
			if (loc(x, y) && loc(x + 29 * f,y))
				break;
	} while (i < 40);

	y = yy;
	clr(x - 29 * n, y - 70, 30, 70);
	color(COLOUR_BLACK);
	draw(spieler_foerdertuerme[n][t].x = x, spieler_foerdertuerme[n][t].y = y, turm);
	graf_mouse(M_ON, NULL);
}


static void markt(void)
{
	int a,k,ko,t;

	GRECT g1 = { 0, 0, 30, 20 };

	g1.g_x = ev.x;
	g1.g_y = ev.y;
	form_center(rsctree_status, &fgrect);

	/* Wir lassen FMD_START und FMD_FINISH weg, weil wir kein WM_REDRAW verarbeiten. */
	save_screen();
	form_dial(FMD_GROW, &g1, &fgrect);

	for (a = 0; a < 6;a++)
		zahl(SM1+a, p[a]);

	do
	{
		for (t = k = 0; k < N_FOERDERTUERME;)
			t += (spieler_foerdertuerme[n][k++].x > -1);

		for (ko = k = 0; k < N_KANONEN;)
			ko += (spieler_kanonen[n][k++].x > -1);

		for (k = 0; k < N_KANONEN; k++)
			if (bg[1 + k * 2] > -1 && spieler_kanonen[n][k].x == -1)
			{
			    graf_mouse(M_OFF, NULL);
			    for (a = 1; a < 10; a++)
			    	if (loc(639 * n + (bg[1 + k * 2] + 5 + a) * f, by[n] - bg[2 + k * 2] - a))
			    		break;
			    graf_mouse(M_ON, NULL);
			    if (a > 9)
			    	break;
			}
		zahl(ST_GELD, spieler_geld[n]);
		zahl(ST_FOERD, t);
		zahl(ST_GESCH, ko);
		zahl(ST_FAHNE, wx[n] > -1);
		zahl(ST_PULVR, spieler_pulver[n]);
		zahl(ST_KUGEL, spieler_kugeln[n]);
		zahl(ST_VOLK, spieler_volk[n]);
		zahl(ST_STEUR, spieler_steuer[n]);

		for (a = 0; a < 6; a++)
		{
			rsctree_status[SM1 + a].ob_state = 8 * (spieler_geld[n] < p[a] || !an_erl && !a ||
			                                   (a == 1) && (bg[0] + t * 30 > 265 || t > 4) ||
			                                   (a == 2) && (k > 9) || (a == 3) && (wx[n] > -1));
		}

		objc_draw(rsctree_status, 0, 5, &desk_g);

		a = form_do(rsctree_status, 0);
		if (a >= 0)
			rsctree_status[a].ob_state = 0;
		if (a == SHK)
			spieler_steuer[n] -= 2 * (spieler_steuer[n] > 0);
		else
		if (a == SHG)
			spieler_steuer[n] += 2 * (spieler_steuer[n] < 100);
		else
		if ((a != FERTIG) && (a - SM1 >-1) && (a - SM1 < 6))
		{
			spieler_geld[n] -= p[a - SM1];
			if (a < SM5)
			{
				restore_screen();
				drw_all();
				if (a == SM1)
					anbau();
				else
				if (a == SM2)
					baue_foerderturm();	/* Fîrderturm bauen */
				else
				if (a == SM3)
					init_kanone(k, 639 * n);
				else
				if (a == SM4)
				{
					wx[n] = 639 * n + f * bg[23];
					wy[n] = by[n] - bg[24];
					werdran(1);
				}

				save_screen();
			}
			else
			{
				spieler_pulver[n] += 30 * (a == SM5);
				spieler_kugeln[n] += 2 * (a == SM6);
				drw_gpk(a - SM4);
				drw_gpk(0);}
			}
	} while (a != FERTIG);

	restore_screen();
	drw_all();
	form_dial(FMD_SHRINK, &g1, &fgrect);
}


/********************* Routinen zur Zielerkennung: ***************************/
/* Die Routinen berechnen die Zielkoordinaten fÅr den Kînig, Kanonen, Geld...*/
static void z_kn(void)
{
   zx=!n*639-f*(bh[21]+15);
   zy=by[!n]-bh[22] -5 ;
}

static void z_ka(void)
{
	int i;

	for (i = 0; i < N_KANONEN; i++)
		if (spieler_kanonen[!n][i].x > -1)
			break;
	if (i < N_KANONEN)
	{
		do
			i = ((int) Random() % N_KANONEN);
		while (spieler_kanonen[!n][i].x == -1);

		zx = spieler_kanonen[!n][i].x + 10;
		zy = spieler_kanonen[!n][i].y;
	}
}


static void z_ft(void)
{
	int i;

	for (i = 0; i < N_FOERDERTUERME; i++)
		if (spieler_foerdertuerme[!n][i].x >- 1)
			break;
	if (i < N_FOERDERTUERME)
	{
		zx = spieler_foerdertuerme[!n][i].x - 15 * f;
		zy = spieler_foerdertuerme[!n][i].y - 10;
	}
}


static void z_ge(void)
{
	if (spieler_geld[!n] > 100)
	{
		zx = !n * 639 - f * (bh[25] + bh[31] / 2);
		zy = by[!n] - bh[26];
	}
}


static void z_pk(void)
{
	short i;

	if (spieler_kugeln[!n] || spieler_pulver[!n] > 19)
	{
		i = Random() & 2;
		zx = !n * 639 - f * (bh[27 + i] + bh[33 + i] / 2);
		zy = by[!n] - bh[28 + i];
	}
}


/* FÅhrt einen Zug des Computers durch */
static int comp(void)
{
	char wd;
	int i,t;
	register double x2=0,x,y,vx2,vx,vy=0,wi;

	spieler_steuer[n] = 16;

	for (i = 0; i < N_KANONEN; i++)
		if (spieler_kanonen[n][i].x >- 1)
			break;

	if (spieler_pulver[n] < 20 && spieler_geld[n] >= p[4])
	{
		spieler_pulver[n] += 30;
		spieler_geld[n] -= p[4];
	}  /* Pulv. kaufen*/

	if (!spieler_kugeln[n] && spieler_geld[n] >= p[5])
	{
		spieler_kugeln[n] += 2;
		spieler_geld[n] -= p[5];
	}  /* Kug.  kaufen*/

	if (i > 9 && spieler_geld[n] >= p[2])
	{
		init_kanone(i = 0, 639 * n);
		spieler_geld[n] -= p[2];
	} /* Ka. kauf.*/

	if ((spieler_foerdertuerme[n][0].x < 0 || spieler_foerdertuerme[n][1].x < 0 || spieler_foerdertuerme[n][2].x < 0) &&
	    spieler_geld[n] >= p[1] && cw[n] > 2)
	{
		spieler_geld[n] -= p[1];
		baue_foerderturm();
	} /* Fîrderturm kaufen */

	drw_all();

	if ((i > 9) || spieler_pulver[n] < 20 || !spieler_kugeln[n])
	{
		return -1;
	}

	    /* Jetzt kommen die Berechnungen fÅr den Schuss: */
	bh = burgen[spieler_burg[!n]];

	do
		i = (int) (Random() % N_KANONEN);
	while (spieler_kanonen[n][i].x == -1);

	zx = 639 * !n - f * (int)(Random() % bh[0]);
	zy = by[!n];
	t = (int) (Random() % 100);
	if( cw[n] ) z_kn();
	switch( cw[n] )  /* Die verschiedenen Strategien */
	{
	  case 1: if( t<30 ) z_ge(); if( t>60 ) z_pk(); break;
	  case 2: z_ka(); if( t<90 ) z_ge(); break;
	  case 3: if( t<50 )z_ka(); else if( t<70 )z_ge(); else if( t<90 )z_pk();
	          if( !(Random()%3) ) z_ft();
	          break;
	  case 4: z_ka(); break;
	  case 5: z_ka(); if( t<90 ) z_ft();
	}

	y = spieler_kanonen[n][i].y - 10 - zy;
	t = 49 - cx[n] * 16;   /* Berechnen der Distanz */
	x = spieler_kanonen[n][i].x+9+8*f-zx -t/2+Random()%t; if( x<0 ) x=-x;

	wd = n ? -wind : wind;

	x2 = -1;

	for (vx = .5; vx < 7 && (x2 < x || vy / 3.5 > vx); )  /* Zeitberechnung bei */
	{                              /* verschiedenen X-Geschwindigkeiten */
		vx += .4;
		t = x2 = 0;
		vx2 = vx;
		while (vx2 > 0 && x2 < x)
		{
			x2 += vx2;
			vx2 += (wd / 2 - vx2) / 5000;
			t++;
		}

		vy = y / t +.5 * G * t;
	}

	if (n)
		vx = -vx;
	vvx = vx;
	vvy = vy;  /* Werte fÅr Schuss Åbergeben */

	if (vx < 0)
		vx =- vx;

	wi = atan2(vy, vx);
	spieler_kanonen[n][i].w = wi * P57;
	spieler_kanonen[n][i].p = (int)(4.0 * vx / cos(wi) - 1.6);

	return i;
}


/*************************** Audienz beim Kînig ******************************/

static void koenig(void)
{
	static char
		kna[] = "[0][Der Kînig meint:             |'",
		kne[]="'|][DemÅtig  zur Kenntnis genommen]",
		kn0[]="Naja...| Nun gut...| Weiter so...",
		kn1[]="Ich bin zufrieden| mit Ihren Leistungen!",
		kn2[]="Hervorragend,| Weiter so!",
		kn3[]="Vielleicht sollten Sie mal| die Steuern senken...",
		kn4[]="Wenn Sie so weiter machen| werde ich Sie entlassen!",
		kn5[]="Vielleicht mal 'nen| Fîrderturm kaufen...",
		kn6[]="Sie sollten sich| gefÑlligst mehr MÅhe| geben!",
		kn7[]="Sie brauchen nicht| jede Runde zu kommen.",
		kn8[]="Wissen Sie eigentlich,| daû Sie mich bereits| xxmal besucht haben?",
		kn9[]="Und Sie sind sich sicher,| daû Sie auch ohne eine| Windfahne zurecht kommen?",
		kn10[]="Schîn, Sie zu sehen...",
		kn11[]="Was soll ich denn in| so einer frÅhen Phase| schon sagen?",
		kn12[]="Sie sollten mehr Geld| verdienen, FîrdertÅrme bauen| und den Gegner besiegen.",
		kn13[]="Ich habe Ihnen nichts| neues zu sagen.",
		kn14[]="Find' ich nett, daû| Sie mich mal besuchen!";
	char a[300],*s[20],k,t,j,i;
	static char *ltz[2];

	for (j = k = 0; j < N_KANONEN; j++)
		k += spieler_kanonen[n][j].x > -1;

	for (j = t = 0; j < N_FOERDERTUERME; j++)
		t += spieler_foerdertuerme[n][j].x > -1;

	if (!(Random()%20) || (spieler_koenig[n] & 16) || ((spieler_koenig[n] & 15) > 9))
	{
		form_alert(1,"[0][Der Kînig hat keine Lust,   |dich zu sprechen.][Schade]");
	}
	else
	{
		kn8[46] = (spieler_koenig[n] > 2559) ? (48 + spieler_koenig[n] / 2560) : 32;
		kn8[47] = 48 + spieler_koenig[n] % 2560 / 256;

		j = Random();
		i = 2;
		s[0] = (j & 1) ? kn0 : kn10;
		s[1] = (j & 2) ? kn12 : kn6;
		if (j & 4)
			s[i++] = kn14;

		if ((spieler_geld[n] > p[1]) && (t < 3))
			s[i++] = kn5;

		if ((spieler_steuer[n] > 40) && (spieler_volk[n] < bg[40]) || (spieler_steuer[n] > 70))
			s[i++] = kn3;

		if  (wx[n] < 0)
			s[i++] = kn9;

		if (spieler_geld[n] > bg[37] && spieler_volk[n] > bg[40] && k > 1)
			s[1] = t < 2 ? kn1 : kn2;

		if (t > 2)
			s[i++] = kn2;

		if (k < 1 && spieler_geld[n] < p[2])
			s[i++] = kn4;

		if ((spieler_koenig[n] & 15) > 4)
			s[i++] = s[i++] = s[i++] = kn7;

		if ((spieler_koenig[n] & 15) > 6 || !(Random() & 7) && (spieler_koenig[n] > 2048))
			s[i++] = s[i++] = kn8;

		if (zug < 4)
			s[1] =s[2] = s[(i = 4) - 1] = kn11;

		do
			if (s[j = Random()%i] == ltz[n])
				s[i++] = s[i++] = kn13;
		while (s[j] == ltz[n]);

		strcpy(a, kna );
		strcat(a, ltz[n] = s[j]);
		strcat(a, kne);
		form_alert(1, a);
	}

	spieler_koenig[n] |= 16;
	spieler_koenig[n] += 256;
	if (~spieler_koenig[n] & 15)
		spieler_koenig[n]++;
}


/************* Kanonenobjektbaum, Wahl von Winkel und Pulver *****************/
static int sch_obj(int k)
{
#if 1
	void *flyinf;		/* MagiC-spezifisch */
	void **p_flyinf;
	int dummy;
#endif

	/* Daten fÅr das MÑnnchen */
	static const int fig[] =
	{
		0,0,15,20,30,20,20,15,10,0,10,-30,18,-18,20,-5,24,-6,
		20,-25,10,-40,0,-45, -10,-40,-20,-25,-24,-6,-20,-5,-18,-18,-10,-30,-10,0,
		-20,15,-30,20,-15,20, -1,-1
	};
	GRECT g1;
	GRECT g2 = {0, 0, 42, 18};
	int i, wi,pv, xw,yw,xp,yp,xk,yk;
	double s,c;
	char *aw, *ap, fl = 1;

 /*printf("sch_obj\n"); */

	aw = rsctree_shootparams[WINK].ob_spec.free_string;
	ap = rsctree_shootparams[PULV].ob_spec.free_string;
	ap[2] = '\0';

#if 1
	flyinf = NULL;
	p_flyinf = &flyinf;
	form_center(rsctree_shootparams, &fgrect);
	form_xdial(FMD_START, NULL, &fgrect, p_flyinf);
#else
	/* Wir lassen FMD_START und FMD_FINISH weg, weil wir kein WM_REDRAW verarbeiten. */
	save_screen();
	form_center(rsctree_shootparams, &fgrect);
#endif

	objc_offset(rsctree_shootparams, WINK, &xw, &yw);
	objc_offset(rsctree_shootparams, PULV, &xp, &yp);
	objc_offset(rsctree_shootparams, KAST, &xk, &yk);
	xk += 105 + f * 36;
	yk += 102;

	g1.g_x = spieler_kanonen[n][k].x + 10 * f;
	g1.g_y = spieler_kanonen[n][k].y - 10;
	g1.g_w = 20;
	g1.g_h = 14;
	form_dial(FMD_GROW, &g1, &fgrect);
	wi = spieler_kanonen[n][k].w;
	pv = spieler_kanonen[n][k].p;

	vsf_interior(vdi_handle, 1);

	do
	{
		if( pv>spieler_pulver[n] ) pv=spieler_pulver[n];
		*aw=48+wi/100;
		*(aw+1)=48+wi%100/10;
		*(aw+2)=48+wi%10;
		if(wi<100) { *aw=*(aw+1); *(aw+1)=*(aw+2); *(aw+2)=0; }
		*ap=48+pv/10; *(ap+1)=48+pv%10;
		if (pv < 10)
		{
			*ap = *(ap + 1);
			*(ap + 1) = 0;
		}

		if( fl )
		{
			objc_draw(rsctree_shootparams, 0, 4, &desk_g);
			graf_mouse(M_OFF, NULL);
			color(1);
			v_circle(vdi_handle, xk - 88 * f, yk - 60, 15);
			i = 0;
			while( fig[i]!=-1 )
				{ xy[i]=xk-88*f+fig[i]; i++; xy[i]=yk-5+fig[i]; i++; }
			xy[i++]=xy[0]; xy[i++]=xy[1];
			v_fillarea(vdi_handle,i/2-1,xy );
			graf_mouse(M_ON, NULL);
		}
		g2.g_x = xw;
		g2.g_y = yw;
		objc_draw(rsctree_shootparams, 0, 4, &g2);
		g2.g_x = xp;
		g2.g_y = yp;
		objc_draw(rsctree_shootparams, 0, 4, &g2);

		if ((i != PL2) && (i != PR2) && (i != PL1) && (i != PR1) || fl)
		{
			graf_mouse(M_OFF, NULL);
			clr(xk - 55, yk - 76, 110, 90);
			color( 1 );
			v_circle(vdi_handle,xk,yk,15 );

			s=sin(wi/P57); c=cos(wi/P57);
			fl=-f; if( wi>90 ) { fl=-fl; c=-c; }
			xy[0]=xk+fl*(c*14+s*14);  xy[1]=yk+s*14-c*14;
			xy[2]=xk+fl*(c*14+s*40);  xy[3]=yk+s*14-c*40;
			xy[4]=xk-fl*(c*55-s*40);  xy[5]=yk-s*55-c*40;
			xy[6]=xk-fl*(c*55-s*14);  xy[7]=yk-s*55-c*14;
			xy[8]=xy[0]; xy[9]=xy[1];
			v_fillarea(vdi_handle,4,xy );
			graf_mouse(M_ON, NULL);
			fl = 0;
		}

#if 1
		i = 0x7f & form_xdo(rsctree_shootparams, 0, &dummy, NULL, flyinf);
#else
		i = form_do(rsctree_shootparams, 0);
#endif
		if (i >= 0)
		{
			rsctree_shootparams[i].ob_state = 0;
		}
		wi -= 10 * (i == WL2) - 10 * (i == WR2) + (i == WL1) - (i == WR1);
		if (wi < 0)
		{
			wi=0;
		}
		if (wi > 180)
		{
			wi = 180;
		}
		pv -= 3 * (i == PL2) - 3 * (i == PR2) + (i == PL1) - (i == PR1);
		if (pv < 5)
		{
			pv = 5;
		}
		if (pv > 20)
		{
			pv = 20;
		}
	} while ((i != SOK) && (i != SAB));

#if 1
	form_xdial(FMD_FINISH, NULL, &fgrect, p_flyinf);
#else
	restore_screen();
	form_dial(FMD_SHRINK, &g1, &fgrect);
#endif

	spieler_kanonen[n][k].w = wi;
	spieler_kanonen[n][k].p = pv;
	return i == SOK;
}


/* Tonausgabe fÅr den Kanonenschuû */
static void baller(char r)
{
	static char s_bal[] =
	{ 0,0,1,15,6,31,9,0,11,0,12,50,13,0,7,192,8,16,255,0 };

	s_bal[5] = r;
	Dosound(s_bal);
}


/****************************** Explosion ************************************/
static void expls(int x, int y, int w, int h, int d)
{
	int i,j;

	vswr_mode(vdi_handle, 3);

	for (i = 0; i < 32;)
	{
		xy[i++] = x;
		xy[i++] = y;
	}

	while (d-- > 0)
	{
		v_pline(vdi_handle, 2, xy);
		memmove(xy, xy + 4, 120);
		for (j = 28; j < 32;)
		{
	  		xy[j++] = x - w + w * (int) ((Random() & 511) / 256);
	  		xy[j++] = y - h + h * (int) ((Random() & 511) / 256);
		}
		v_pline(vdi_handle, 2, xy + 28);
		baller((d & 31) ^ 31);		/* Tonausgabe */
	}

	for (i = 0; i < 32; i += 4)
	{
		v_pline(vdi_handle, 2, xy + i);
	}
	vswr_mode(vdi_handle, 1);
}


/* Kugel zeichnen aus fÅnf Linien, solange innerhalb des Spielbereichs */
static int kugel(int x, int y)
{
	/* TODO: parametrisieren */
	if ((x < 3) || (x > 637) || (y < 23))
	{
		return 0;
	}

	line(x - 2, y - 2, x + 1, y - 2);
	line(x - 3, y - 1, x + 2, y - 1);
	line(x - 3, y    , x + 2, y);
	line(x - 3, y + 1, x + 2, y + 1);
	line(x - 2, y + 2, x + 1, y + 2);
	return 1;
}


/********************************* Ein Schuss ********************************/
static void schuss(/* Nr. der Kanone */ int k)
{
	double x,y,ox,oy,vx,vy;
	short v,c,a, j;

	graf_mouse(M_OFF, NULL);
	spieler_pulver[n] -= spieler_kanonen[n][k].p;
	spieler_kugeln[n]--;
	drw_gpk(1);
	drw_gpk(2);

	x = spieler_kanonen[n][k].x + 9 + 8 * f;
	y = spieler_kanonen[n][k].y - 10;
	vx = (.4 + 0.25 * spieler_kanonen[n][k].p) * cos(spieler_kanonen[n][k].w / P57) * f;
	vy = (.4 + 0.25 * spieler_kanonen[n][k].p) * sin(spieler_kanonen[n][k].w / P57);

	if (spiel_modus & (2 - n))
	{
		/* Der jeweilige Spieler (n = 0 oder 1) ist ein Computer */
		vx = vvx;
		vy = vvy;
	}

	color(COLOUR_BLACK);
	vswr_mode(vdi_handle, MD_XOR);

	baller(0);	/* Tonausgabe */

	kugel((int) x, (int) y);	/* Kugel zeichnen */

	v = 1;	/* damit die Kugel erstmal losfliegt. Sollte obsolet sein. */
	c = 2;	/* mindestens zwei SchleifendurchlÑufe  */

	while((x > 3) && (x < 637) && (y < 396) && (v || c))  /* Flugschleife */
	{
		ox = x;
		oy = y;
		x += vx;
		y -= vy;
		vy -= G;
		vx += (wind / 2 - vx) / 5000;
		kugel((int) ox, (int) oy);		/* vorherige Kugel lîschen (durch XOR-Schreibmodus) */

		if (kugel((int) x, (int) y))	/* neue Kugel zeichnen, wenn mîglich */
		{
			/* Kugel ist innerhalb des Spielbereichs gezeichnet worden. */
			/* v = 1, wenn die drei getesteten Pixel alle gesetzt sind, sonst v = 0 */
			v = loc((int) x,     (int) y) &
			    loc((int) x - 1, (int) y + 1) &
			    loc((int) x + 1, (int) y + 2);
		}

		/* Tonausgabe */
		
		a = 1000 + 2 * y;
		if (a < 30)
		{
			a = 30;
		}
		Giaccess(10, 128 + 9);		/*  10 (0x0a) -> Register 9: volume channel B */
		Giaccess(244,128 + 7);		/* 244 (0xf4) -> Register 7 */
		Giaccess(a & 255, 128 + 2);	/* -> Register 2: frequency channel B */
		Giaccess(a >> 8, 128 + 3);	/* -> Register 3: control step size channel B */

		if (c)
		{
			c--;
		}

		evnt_timer(8);		/* warte soviele Millisekunden */
	}

	kugel((int) x, (int) y);	/* letzte Kugel lîschen (durch XOR-Schreibmodus) */
	vswr_mode(vdi_handle, MD_REPLACE);
	color(COLOUR_WHITE);
	oldn = n;	/* ? */

	if (!v)
		for (c = 0; c < 4; c++)      /* Einschlag der Kugel */
		{
			if ((ox > 6) && (ox < 634))
				v_circle(vdi_handle, (int)ox, (int)oy, 5);

			baller(22 + c * 3);		/* Tonaugabe */

			for (n = 0; n < 2; n++)    /* Treffer ? */
			{
				bg = burgen[spieler_burg[n]];

				if ((Random() & 1) && (n ? x > 639 - bg[0] : x < bg[0]) && spieler_volk[n])
					spieler_volk[n]--;

				fn();

				for (j = 0; j < N_KANONEN; j++)
			 		if ((spieler_kanonen[n][j].x < ox + 2) && (spieler_kanonen[n][j].x + 22 > ox) && (spieler_kanonen[n][j].x > -1) &&
					    (spieler_kanonen[n][j].y - 16 < oy) && (spieler_kanonen[n][j].y > oy))
			        {
						clr(a = spieler_kanonen[n][j].x, v = spieler_kanonen[n][j].y - 12, 20, 13);
						expls(a + 10, v + 6, 13, 8, 50);
						color(COLOUR_WHITE);
						spieler_kanonen[n][j].x = -1;
			        }

				for (j = 0; j < N_FOERDERTUERME; j++)
					if ((spieler_foerdertuerme[n][j].x > -1) &&
					    drin(a = 639 * n + f * spieler_foerdertuerme[n][j].x, v = by[n] - spieler_foerdertuerme[n][j].y,
					         30, 37, 2,(int)ox,(int)oy))
					{
						if (drin(a, v, 30, 37, -2, (int)ox, (int)oy))
						{
							expls((a = spieler_foerdertuerme[n][j].x - 29 * n) + 15,
							      (v = spieler_foerdertuerme[n][j].y - 40) + 20, 15, 20, 120);
							clr(a, v, 30, 40);
							spieler_foerdertuerme[n][j].x =- 1;
						}
	
						c = 4;
					}

				if ((ox > wx[n] - 11) && (ox < wx[n] + 11) && (oy > wy[n] - 16) && (oy < wy[n]) && (wx[n] > -1))
				{
					clr(wx[n] - 10, wy[n] - 15, 20, 15 );
					wx[n] = -1;
				}

				if (drin(bg[21], bg[22], 30, 25, 0, (int)ox, (int)oy))
				{
					end = n + 17;
					expls(a = 639 * n + f * (bg[21] + 15), v = by[n] - bg[22] - 12, 17, 17, 40);
					v_circle(vdi_handle, a, v, 17);
					expls(a, v, 17, 17, 200);
					c = 4;
				}

				for (j = 0; j < 6; j += 2)
					if (drin(bg[25 + j], bg[26 + j], a = bg[31 + j], v = bg[32 + j], 3,
					    (int)ox, (int)oy))
					    break;

				switch (j)
				{
			        case 0:
			        	spieler_geld[n] -= Min(200, spieler_geld[n]);
			        	break;
			        case 2:
			        	spieler_pulver[n] = 0;
			        	break;
			        case 4:
			        	spieler_kugeln[n] -= Min(2, spieler_kugeln[n]);
				}

				if (j < 6)
				{
					expls(639 * n + f * (bg[25 + j] + a / 2), by[n] - bg[26 + j] - v / 2,
					       a / 2, v / 2, 60 + 100 * (j == 2));
					drw_gpk(j / 2);
					c = 4;
				}
			}

			ox += vx;
			oy -= vy;
		}

	n = oldn;
	fn();
	Giaccess(0, 137);
	graf_mouse(M_ON, NULL);
}


/********** Berechnen von Bevîlkerungszuwachs usw. nach jedem Zug ************/
static void rechnen(void)
{
	int j;
	static int pmi[6] =
	{
		98,347,302,102,30,29
	},   /* Preisgrenzen */
	            pma[6]={ 302,707,498,200,89,91 },
	            psp[6]={ 10,50,50,20,10,10 };     /* max. Preisschwankung */

	j = spieler_steuer[n];
	spieler_geld[n] += spieler_volk[n] * ((j > 65) ? (130 - j) : j) / (150 - (int) (Random() % 50));
	spieler_volk[n] = spieler_volk[n] * (95 + (int) (Random() % 11) / 100) + (21 - j + ((int) Random() % 9)) * (8 + (int) (Random() % 5))/20;
	if (spieler_volk[n] < 0)
	{
		spieler_volk[n] = 0;
		end = n + 49;
	}

	for (j = 0; j < N_FOERDERTUERME;j++)
	{
		spieler_geld[n] += (40 + (int) (Random() % 31)) * (spieler_foerdertuerme[n][j].x > -1);
	}

	for (j = 0;j < 6; j++ )
	{
	  p[j] += psp[j] * ((int) (Random() % 99)) / 98 - psp[j] / 2;
	  p[j] = Max(p[j], pmi[j]);
	  p[j] = Min(p[j], pma[j]);
	}
	drw_gpk(0);
}


/******************************* Spielende ***********************************/
static void ende(void)
{
	char s1[80],s2[80],s3[80],a,b,c;
	void *sav_ssp;

	form_center(rsctree_winner, &fgrect);
	rsctree_winner[0].ob_y = 30;
	rsctree_winner[SG1].ob_spec.free_string = s1;
	rsctree_winner[SG2].ob_spec.free_string = s2;
	rsctree_winner[SG3].ob_spec.free_string = s3;
	strcpy(s1, "!! ");
	strcat(s1, (end & 2) ? l_nam : r_nam);
	strcat(s1, " hat gewonnen !!" );
	s2[0] = 0;

	if (~end & 64)
	{
		strcpy(s2,"( ");
	}

	strcat(s2, (end & 2) ? r_nam : l_nam);

	if ((end & 240) < 48)
	{
		a = s2[strlen(s2) - 1];
		strcat(s2, a == 's' || (a == 'S') ? "' " : "s ");
	}

	switch(end & 240)
	{
	  case 16: strcat( s2,"Kînig wurde getroffen," );
	           strcpy( s3,"  daraufhin ergab sich dessen Volk. )" ); break;
	  case 32: strcat( s2,"Kînig hat aufgrund der" );
	           strcpy( s3,"  aussichtslosen Lage kapituliert. )" ); break;
	  case 48: strcat( s2," hat kein Volk mehr. )" ); s3[0]=0; break;
	  case 64: strcpy( s3,s2 );
	           strcpy( s2,"( Die maximale Rundenzahl ist erreicht.");
	           strcat( s3," befindet sich in der schlechteren Lage. )" );
	}
	/* TODO: Hier kein form_dial()? */
	objc_draw(rsctree_winner, 0, 7, &desk_g);

	for (a = 0; (a < N_SPIELERNAMEN) && strncmp(tabelle_spielernamen[a], l_nam, 7); a++)
		;

	for (b = 0; (b < N_SPIELERNAMEN) && strncmp(tabelle_spielernamen[b], r_nam, 7); b++)
		;

	if((a < N_SPIELERNAMEN) && (b < N_SPIELERNAMEN) && (a != b))
	{
		if (~end & 2)
		{
			c = a;
			a = b;
			b = c;
		}

		tabelle_gewonnen[a][b]++;
		tabelle_gewonnen[b][8]++;
		tabelle_gewonnen[a][9] = 100 * ++tabelle_gewonnen[a][7] / ++tabelle_gewonnen[a][6];
		tabelle_gewonnen[b][9] = 100 * tabelle_gewonnen[b][7] / ++tabelle_gewonnen[b][6];
	}

	graf_mouse(M_OFF, NULL);
	sav_ssp = (void *) Super(NULL);
	m_musik();
	Super(sav_ssp);
	graf_mouse(M_ON, NULL);  /* Musik... */
	Giaccess(0, 138);
	Giaccess(0, 139);
	Giaccess(0, 140);
}


/************************* DurchfÅhren eines Zuges ***************************/
static int ein_zug(void)
{
	int i,fl,a;
	int fck;

	n = zug & 1;
	fn();
	spieler_koenig[n] &= ~16;
	wind = wind * 9 / 10 + Random() % 12 - 6;

	werdran(1);

	do
	{
		fl = 0;
		wind_update(END_MCTRL);			/* Bildschirm freigeben, damit die MenÅs funktionieren */
		fck = !(spiel_modus & (2 - n));
		do
		{
			/* Warte auf Mausklick oder MenÅ. Bearbeite MenÅ. */

			if (event())
			{
				return 0;
			}

			/* OR Åber die zweite Bildschirmzeile, die ersten 480 Spalten */
			/* TODO: Warum? */

			for (a = i = 0; i < 60;)
			{
				a |= *(char *)(screenptr + 80 + i++);
			}
			a = 0;
			/*
			printf("ev.x/y/bstate = %3d/%3d/%d\n", ev.x, ev.y, ev.bstate);
			printf("!(spiel_modus & (2 - n)) = %d\n", !(spiel_modus & (2 - n)));
			printf("fck = %d\n", fck);
			printf("(!ev.bstate && fck) = %d\n", (!ev.bstate && fck));
			printf("a = %d\n", a);
			printf("((!ev.bstate && fck) || a) = %d\n", ((!ev.bstate && fck) || a));
			*/
			/* Wenn spiel_modus = 0, dann
			   !(spiel_modus & (2 - n)) == 1 */
		}
		while ((!ev.bstate && fck) || a);

		wind_update(BEG_MCTRL);
		bg = burgen[spieler_burg[n]];

		if (spiel_modus & (2 - n))
		{
			graf_mouse(M_OFF, NULL);
			i = comp();
			graf_mouse(M_ON, NULL);
			fl = 1 + (i < 0);
		}
		else
		if ((ev.x > ftx) && (ev.x < ftx + ftw) && (ev.y > fty) && (ev.y < fty + fth))
		{
			fl = 2;
		}
		else
		{
			for (i = 0; i < N_KANONEN; i++)
			{
				if ((spieler_kanonen[n][i].x <= ev.x) && (spieler_kanonen[n][i].x + 20 >= ev.x) && (spieler_kanonen[n][i].x != -1) &&
			    	(spieler_kanonen[n][i].y >= ev.y) && (spieler_kanonen[n][i].y - 14 <= ev.y))
			    	break;
			}

			if (i >= N_KANONEN)
			{
				if (drin(bg[25], bg[26], bg[31], bg[32], 0, ev.x, ev.y) ||
					drin(bg[27], bg[28], bg[33], bg[34], 0, ev.x, ev.y) ||
					drin(bg[29], bg[30], bg[35], bg[36], 0, ev.x, ev.y))
				{
					markt();
				}
				else
				if (drin(bg[21], bg[22], 30, 25, 0, ev.x, ev.y))
				{
					koenig();
				}
				else
				{
					bing();
				}
			}
			else
			if (spieler_pulver[n] < 5)
			{
				bing();
				form_alert(0,"[0][Dein Pulver reicht nicht!][Abbruch]");
			}
			else
			if (!spieler_kugeln[n])
			{
				bing();
				form_alert(0,"[0][Du hast keine Kugeln mehr!][Abbruch]");
			}
			else
			{
				fl = sch_obj(i);
			}
		}
	} while (!fl);

	werdran(0);
	if (fl < 2)
	{
		schuss(i);
	}

	if (~spieler_koenig[n] & 16)
		spieler_koenig[n] &= ~15;
	rechnen();
	zug++;

	for (i = 0; i < N_KANONEN; i++)
		if (spieler_kanonen[n][i].x > -1)
			break;

	n = zug & 1;
	bg = burgen[spieler_burg[n]];

	for (a = 0; a < N_KANONEN; a++)
		if (spieler_kanonen[n][a].x > -1)
			break;

	if ((a == N_KANONEN) && (i < N_KANONEN) && (bg[40] > spieler_volk[n]) && (spieler_geld[n] < p[2]/3) && au_kap &&
	    (spieler_foerdertuerme[n][0].x + spieler_foerdertuerme[n][1].x + spieler_foerdertuerme[n][2].x +
	     spieler_foerdertuerme[n][3].x + spieler_foerdertuerme[n][4].x == -5))
	{
 		end = n + 33;
 	}

	if (!end && (zug / 2 >= max_rund))
	{
		static h[2];
		for (n = 0; n < 2; n++)
		{
			h[n] = spieler_geld[n] + spieler_pulver[n] * p[4] / 30 +
			       spieler_kugeln[n] * p[5] / 2 + (wx[n] > -1) * p[3] + spieler_volk[n] * 4;
			for (i = 0; i < N_FOERDERTUERME; i++)
				if (spieler_foerdertuerme[n][i].x >- 1)
					h[n] += p[1];
			for (i = 0; i < N_KANONEN; i++)
				if (spieler_kanonen[n][i].x > -1)
					h[n] += p[2];
		}

		end = 65 +(h[1] < h[0]);
	}

	if (end)
	{
		ende();
		wind_update(END_MCTRL);
		do
		{
			if (event())
		 		return(0);
		}
		while((!ev.bstate) || end);

		wind_update(BEG_MCTRL);
	}

	return 1;
}


/*****************************************************************************/
int main(void)
{
	int i;

	if (Getrez() != 2)
	{
		form_alert(1,"[1]['Ballerburg' lÑuft nur|in ST-Hoch (640 x 400 monochrom)][Abbruch]");
		return 1;
	}

	if (gem_init() != 0)
	{
		return 2;
	}

	screenptr = Logbase();

	m_laden("BALLER.MUS"); /* Laden der Musikdatei mit Funktion aus MUSIK.C */

	if (!rsrc_load("BALLER.RSC"))
	{
		form_alert(1,"[1][BALLER.RSC lÑût sich nicht|laden.][Abbruch]");
		v_clsvwk(vdi_handle);
		appl_exit();
		return 3;
	}

	rsrc_gaddr(R_TREE, MENUE,    &rsctree_menu);
	rsrc_gaddr(R_TREE, INFOTREE, &rsctree_info);
	rsrc_gaddr(R_TREE, SCHUSS,   &rsctree_shootparams);
	rsrc_gaddr(R_TREE, BURG,     &rsctree_castles);
	rsrc_gaddr(R_TREE, NAMEN,    &rsctree_names);
	rsrc_gaddr(R_TREE, DRAN,     &rsctree_move);
	rsrc_gaddr(R_TREE, STATUS,   &rsctree_status);
	rsrc_gaddr(R_TREE, SIEGER,   &rsctree_winner);
	rsrc_gaddr(R_TREE, COMPUTER, &rsctree_computer);
	rsrc_gaddr(R_TREE, REGEL1,   &rsctree_help1);
	rsrc_gaddr(R_TREE, REGEL2,   &rsctree_help2);
	rsrc_gaddr(R_TREE, REGEL3,   &rsctree_help3);
	rsrc_gaddr(R_TREE, OPTION,   &rsctree_options);
	rsrc_gaddr(R_TREE, EINTRAG,  &rsctree_players);

	for (i = 0; i < 6; i++)
	{
		rsctree_menu[MN_ACC1 + i].ob_state = DISABLED;
	}

	/* G_BUTTONs */
	rsctree_computer[CN1  + 2].ob_state = SELECTED;
	rsctree_computer[CN2  + 2].ob_state = SELECTED;
	rsctree_computer[CTS1 + 1].ob_state = SELECTED;
	rsctree_computer[CTS2 + 1].ob_state = SELECTED;

	screenbuffer = (char *) Malloc(32000L);		/* TODO: auflîsungsunabhÑngig machen */
	burgen_daten = (int *) Malloc(BURGEN_DATEN_MAXLEN * sizeof(*burgen_daten));
	if ((screenbuffer == NULL) || (burgen_daten == NULL))
	{
	   exit(form_alert(1,"[1][Zu wenig Speicher!][Abbruch]"));
	}

	an_erl = 1;
	mxin = 3;
	max_rund = 32767;
	au_kap = 1;
	t_load();
	i = burgen_laden();
	if (i)
	{
		/* TODO: aufrÑumen */
		if (i == 1)
			form_alert(1, "[1][Kann BALLER.DAT nicht finden.][Abbruch]");
		else
			form_alert(1, "[1][Zuviele oder zu groûe Burgen in BALLER.DAT][Abbruch]");
		return 4;
	}

	menu_bar(rsctree_menu, 1);
	cls();
	wind_update(BEG_MCTRL);

	v_show_c(vdi_handle, 0);
	graf_mouse(0, 0);

	l_nam = nsp1;
	r_nam = nsp2;
	spiel_modus = 0;	/* Spieler gegen Spieler */
	neues_spiel();
	while (ein_zug())
		;

	t_save();

	Mfree(screenbuffer);
	Mfree(burgen_daten);

	menu_bar(rsctree_menu, 0);
	wind_update(END_MCTRL);
	v_clsvwk(vdi_handle);
	appl_exit();
	return 0;
}
