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
#include <math.h>
#include <mt_aes.h>
#include <vdi.h>
#include <osbind.h>     /* operating system Makros */
#include "baller.h"     /* Namen der Objekte der RSC-Datei */
#include "musik.h"

#define Min(a,b)  ((a)<(b)?(a):(b))
#define Max(a,b)  ((a)>(b)?(a):(b))
#define maus()    graf_mkstate(&ev)
#define menu(a)   wind_update(3 - a)
#define hide()    graf_mouse(256, 0)
#define show()    graf_mouse(257, 0)
#define sav_scr() hide(); memmove(buf, scr, 32000); show()
#define lod_scr() hide(); memmove(scr, buf, 32000); show()
#define PI        3.1416
#define P57       57.296
#define G         0.02
#define bing()    printf("\007");
#define fn()      f=1-2*(n&1);

/* Externe Routinen: Funktionen aus MUSIK.C und Winkelfunktionen */
extern double sin(),cos(),tan(),atan2();

int _mneed=20000; /* wichtig bei Lattice C, damit sich das Programm nicht */
 /* gleich den ganzen Speicherplatz schnappt, sondern nur 20K. */

int handle;
char *buf;
char *bur_ad;
char *scr;
char *a_opt, *a_sch, *a_nam, *a_sta, *a_ein, *a_inf,
     *a_brg, *a_dra, *a_sie, *a_com, *a_re1, *a_re2, *a_re3;

int *bh;
int zx,zy;  /* Koordinaten des Zieles */

static const GRECT scr640x400 = {0, 0, 640, 400 };
static EVNTDATA ev;
static GRECT fgrect;		/* fx,fy,fw,fh */
OBJECT *a_men;

double vvx,vvy;
int m_buf[8], xy[100], 
      bur[2],bx[2],by[2], ge[2],pu[2],ku[2],vo[2],st[2],kn[2],
      wx[2],wy[2],ws,wc,
      *bg, zug,n,oldn, p[6], t_gew[6][10], max_rund,
      ftx,fty,ftw,fth,
      f_h, *burgen[20],b_anz;
char *l_nam;
char *r_nam;

char  f, mod, wnd, end, txt[4], an_erl, mxin, au_kap,
      cw[2]={2,2}, cx[2]={1,1}, cn[7][8]={ "Tîlpel","Dummel","Brubbel",
      "Wusel","Brîsel","Toffel","RÅpel" },
      t_na[6][8]={ "Tîlpel","Dummel","Brubbel","Wusel","Brîsel","Toffel" },
      nsp1[22]="Hugo",nsp2[22]="Emil";

struct { int x,y,w,p; } ka[2][10];
struct { int x,y; } ft[2][5];


/*********************** Elementare  Grafikbefehle ***************************/


/* ôffnen der Workstation... */
static void gem_init(void)
{
	WORD dummy;
   int i, work_in[11], work_out[57];
	
	appl_init();
	handle = graf_handle(&dummy, &dummy, &dummy, &dummy);
	for (i = 0; i < 10; i++)
	{
		work_in[i] = 1;
	}
	work_in[10] = 2;
	v_opnvwk(work_in, &handle, work_out);
	v_clrwk(handle);
}


/********************** BALLER.TAB laden/speichern ***************************/
static int t_load(void)
{
   if ((f_h = (int) Fopen("BALLER.TAB", 0)) < 0)
   {
       return 1;
   }
   Fread(f_h,   1, &an_erl);
   Fread(f_h,   1, &au_kap);
   Fread(f_h,   1, &mxin);
   Fread(f_h,   3, (void *) (**(long **)(((char *) a_opt) + MAX_XX * 24 + 12)));
   Fread(f_h,   2, &max_rund);
   Fread(f_h,  48, t_na);
   Fread(f_h, 120, t_gew);
   Fclose(f_h);
   return 0;
}

static int t_save(void)
{
   if ((f_h = (int) Fcreate("BALLER.TAB", 0)) < 0)
   {
      return 1;
   }
   Fwrite(f_h,   1, &an_erl); Fwrite(f_h,1,&au_kap); Fwrite(f_h,1,&mxin);
   Fwrite(f_h,   3, (void *) (**(long **)(((char *) a_opt) + MAX_XX * 24 + 12)));
   Fwrite(f_h,   2, &max_rund);
   Fwrite(f_h,  48, t_na);
   Fwrite(f_h, 120, t_gew);
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


/* liest eine Dezimalzahl von der Datei, Remarks werden Åberlesen */
long rdzahl(void)
{
   char a, sign = 1,rem = 0;    /* wird durch * getoggled, und zeigt damit an, */
                           /* ob man sich in einer Bemerkung befindet */
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

   while((a >='0') && (a <= '9'))
   {
      val *= 10;
      val += a - '0';
      a = zeichen();
   }
   return sign * val;
}


/************************* BALLER.DAT laden **********************************/
static void burgen_laden(void)
{
   int *a,j;

   a = (int *) bur_ad;
   if ((f_h = (int) Fopen("BALLER.DAT", 0)) < 0)
   {
      exit(form_alert(1, "[1][Kann BALLER.DAT nicht finden.][Abbruch]"));
   }

   b_anz = 0;
   while ((j = (int) rdzahl()) != -999)
   {
      burgen[b_anz++] = a;
      *a++ = j;
      for (j = 0; j < 40; j++)
      {
          *a++ = (int) rdzahl();
      }
      while ((*a++ = (int) rdzahl()) != -1);
   }
   Fclose(f_h);
}


/* Lîscht den gesamten Bildschirm auûer der MenÅleiste */
static void cls(void)
{
   long *a;
   int i;

   a = (long *) (((char *) scr) + 32000);
   for (i = 0; i < 7600; i++)
   {
      *--a=0;
   }
}


static void color(int a)
{
   vst_color(handle, a);
   vsl_color(handle, a);
   vsf_color(handle, a);
}


/* Zeichnet eine Linie */
static void line(int x1, int y1, int x2, int y2)
{
   xy[0] = x1;
   xy[1] = y1;
   xy[2] = x2;
   xy[3] = y2;
   v_pline(handle, 2, xy);
}


/***************************** Zeichenroutine ********************************/
void draw(int x, int y, int *a)
{
   int i;
   int fil = 1;

   hide();
   vsf_interior(handle, 2);
   vsf_style(handle, 9);

   while (*a != -1)
   {
      if (*a == -2)
      {
         vsf_interior(handle, *++a);
         vsf_style(handle, *++a);
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
            v_fillarea(handle, i/2, xy);
         }
         else
         {
             v_pline(handle, i/2 - 1, xy);
         }
      }
      a++;
   }
   show();
}


static void box(int x, int y, int x2, int y2, int c)
{
   color(c);
   xy[0] = x;
   xy[1] = y;
   xy[2] = x2;
   xy[3] = y2;
   v_bar(handle, xy);
}


static void clr(int x, int y, int w, int h)
{
    box(x, y, x + w - 1, y + h - 1, 0);
}


/* Kanone k setzen */
static void init_ka(int k, int xr)
{
   int x,y;
	static const int kanon[]={ -2,1,1, 1,0,3,3,5,0,3,3,3,11,2,11,2,9,4,9,4,11,3,11,3,8,0,5,
	   3,8,6,5,3,8,3,3,-9, 11,0,13,2,15,0,16,0,17,1,17,2,15,4,19,8,16,11,8,3,-9,
	   -1 };

   draw(x = xr + bg[1 + k * 2] * f, y = by[n & 1] - bg[2 + k * 2], kanon);
   if (~n&2)
   {
      ka[n][k].x = x - 20 * n;
      ka[n][k].y = y;
      ka[n][k].w = 45;
      ka[n][k].p = 12;
   }
}


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
      case 0: a=sack;  xp=7; yp=10;  i=(ge[m]+149)/150; break;
      case 1: a=fass;  xp=yp=9;      i=(pu[m]+29)/30;  break;
      case 2: a=kuge;  xp=yp=6;      i=ku[m];
   }
   bg = burgen[bur[m]]; w*=2;
   xr = (n&2? bx[m]:639*m)+f*bg[25+w]; yr=by[m]-bg[26+w];

   hide();
   clr( xr-bg[31+w]*m-!n,yr-bg[32+w],bg[31+w]+1,bg[32+w]+1 );
   color(1);

   for( y=z=0; i>0 && y<bg[32+w]; y+=yp )
      for( x=0; i>0 && x<bg[31+w]; x+=xp,z++,i-- )
         draw( xr+f*x,yr-y, a );
   show();
   if (i > 0)    /* Maximalbetrag Åberschritten ? */
      switch (w)
      {
         case 0: ge[m]=z*150; break;
         case 2: pu[m]=z*30;  break;
         case 4: ku[m]=z;
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
static void burg(int nn)
{
	/***************** Grafikdaten fÅr Trohn, Kanone... **************************/
	static const int thron[] =
	{
	   -2,2,2, 8,3,5,12,8,12,12,16,12,17,14,15,
	   15,15,17,17,17,16,21,11,24,11,21,3,-9, -4, 3,0,5,2,24,2,26,0,-9,
	   19,4,18,9,-9, 10,4,11,9,-9, 14,4,15,4,-9, 8,10,11,11,-9, 21,10,18,11,-9,
	   14,17,15,17,-9, 13,19,13,19,-9, 16,19,16,19,-9, 12,22,12,21,15,21,14,22,
	   17,21,17,22,-9, 6,13,6,17,5,16,5,17,7,17,7,16,-9, -1
	 };
   int i,xr;

   oldn = n;
   n = nn;
   fn();
   xr = n & 2 ? (bx[n &= 1]) : (639 * n);
   bg = burgen[bur[n]];
   ge[n] = bg[37];
   pu[n] = bg[38];
   ku[n] = bg[39];
   vo[n] = bg[40];

   color(1);
   vswr_mode(handle, 4);
   draw(xr,by[n], &bg[45]);
   vswr_mode(handle, 1);
   clr(xr + f * bg[21] - n * 30, by[n] - bg[22] - 25, 30, 25);
   color(1);
   draw(xr + f * bg[21], by[n] - bg[22], thron);

   n = nn;
   for (i = 0; i < 10 && bg[i * 2 + 1] > -1; i++)
   {
      init_ka(i, xr);
   }

   drw_all();
   n = oldn;
}


/**************************** Neues Bild zeichnen ****************************/
static void bild(void)
{
   int y,x1,x2,v1,v2;

   hide();
   cls();

   by[0] = (int) (Random() %80 + 300 & ~3);
   by[1] = (int) (Random() % 80 + 300 & ~3);

   y = 400;
   x1 = 0;
   x2 = 2556;
   color(1);
   v1 = v2 = 2;

   vsl_type(handle, 7);

   while ((x1 < x2) && (--y > 20))
   {
      vsl_udsty(handle, ~((257 << (Random() &7 )) * (y & 1)));
      line(x1 / 4, y, x2 / 4, y);
      if (y == by[0])
      {
          x1 = *burgen[bur[0]] * 4;
      }
      if (y == by[1])
      {
         x2 = 639 - (*burgen[bur[1]] << 2);
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
   vsl_type(handle, 1);
   memset(ka, -1, 160);
   burg(0);
   burg(1);
   v_gtext(handle, 276, 395, " Runde     ");
   show();
}


/******************** Initialisierung vor neuem Spiel ************************/
static void neues(void)
{
   static int pr[6] = { 200,500,400,150,50,50 };   /* Preise zu Beginn */
   int j;

   wnd = Random() % 60 - 30;
   st[0] = st[1] = 20;
   kn[0] = kn[1] = 0;

   for (j = 0; j < 6; j++)
   {
      p[j] = (int) (pr[j] * (95 + Random() % 11) / 100);
   }

   bild();

   for (n = 0; n < 2; n++)
   {
      bg = burgen[bur[n]];
      wx[n] = n ? (639 - bg[23]) : bg[23];
      wy[n] = by[n] - bg[24];
      for (f = 0; f < 5;)
      {
         ft[n][f++].x = -1;
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


/******************* Darstellung der beiden Windfahnen ***********************/
static void fahne(void)
{
   char m=-1;

   while( ++m<2 ) if( wx[m]>-1 )
   {
      clr(wx[m] - 10, wy[m] - 15, 20, 15);
      color(1);
      line( wx[m],wy[m],wx[m],wy[m]-15 );
      if (m == n)
      {
         line( wx[m],wy[m]-15,wx[m]+ws,wy[m]-13+wc );
         line( wx[m],wy[m]-11,wx[m]+ws,wy[m]-13+wc );
      }
   }
}


/** Anzeige des Spielers, der am Zug ist, sowie Darstellung der Windfahnen ***/
static void werdran(char c)
{
   int *a,i,x,y,w,h,c1,s1,c2,s2;
   char *ad;
   double wk,wl;

   z_txt(zug / 2 + 1);
   v_gtext(handle, 332, 395, txt);

   a = (int *)(a_dra + 16);
   if( c )
   {
      a[0] = 5 + (629 - a[2]) * n;
      a[1] = 25;
      *(char **)(a_dra + DNAM * 24 + 12) = n ? r_nam : l_nam;
      ad = *(char **)(a_dra+DWIN*24+12);
      ad[0] = ad[5]=4+28*!wnd-(wnd>0);
      i = (wnd < 0) ? -wnd : wnd;
      ad[2] = 48 + i/10;
      ad[3] = 48 + i%10;
      if (wx[n] < 0)
      {
         ad[0] = ad[5] = (char) 32;
         ad[2] = ad[3] = '?';
      }
      objc_offset((OBJECT *) a_dra, DOK, &ftx, &fty);
      ftw = *(int *)(a_dra + DOK * 24 + 20); fth=*(int *)(a_dra + DOK * 24 + 22);
      objc_offset((OBJECT *) a_dra, DWBX, &x, &y);
      w = *(int *)(a_dra + DWBX * 24 + 20);
      h = *(int *)(a_dra + DWBX * 24 + 22);

      objc_draw((OBJECT *) a_dra, 0, 4, &scr640x400);

      c = (wnd > 0) ? 1 : -1;
      wk = c * wnd/15.0;
      wl = wk*.82;
      if (wk > 1.57)
      {
      	wk=1.57;
      }
      if (wl > 1.57)
      {
      	wl=1.57;
      }
      s1 = c*20*sin(wk);
      c1 = 20*cos(wk);
      s2 = c*20*sin(wl);
      c2 = 20*cos(wl);
      ws = s1/2.0;
      ws += !ws;
      wc = c1/2.0;

      hide();
      if (wx[n] > -1)
      {
         color(1);
         x += w/2;
         line(x,    y + h, x    , y + 5);
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
         xy[ 6] = (xy[4] + (xy[8] >> 1)) + s2;
         xy[ 7] = (xy[5] + (xy[9] >> 1)) + c2;
         v_pline(handle, 6, xy);
      }
      fahne();
      show();
   }
   else
   {
      hide();
      clr(a[0], a[1], a[2], a[3]);
      show();
   }
}


/************** Darstellung und Verwaltung eines Objektbaumes ****************/
static long obj_do(char *adr)
{
   long a;
   int fl = 0;
   static GRECT g1 = {20, 20, 30, 20};

   if (adr == a_nam)
   {
   	fl = NSP1;
   	g1.g_x = 80;
   }
   if (adr == a_opt)
   {
   	fl = MAX_XX;
   	g1.g_x = 80;
   }
   if (adr == a_com)
   {
   	g1.g_x = 150;
   }
   if (adr == a_ein)
   {
   	fl = EI_N1;
   	g1.g_x = 260;
   }

   form_center((OBJECT *) adr, &fgrect);
   sav_scr();
   form_dial(1, &g1, &fgrect);
   objc_draw((OBJECT *) adr,0,5, &scr640x400);
   a = form_do((OBJECT *) adr, fl);
   *(int *)(adr + 24 * a + 10) = 0;
   lod_scr();
   form_dial(2, &g1, &fgrect);
   return(a);
}


/********************* Neues Spiel: Auswahl der Burgen ***********************/
static int bur_obj(void)
{
   static const GRECT g1 = {70, 20, 30, 20};

   int i,ob0,ob1,oy0,oy1, x,y,w,h, ol[8];

   oy0=by[0]; oy1=by[1]; ob0=bur[0]; ob1=bur[1];

   *(char **)(a_brg + 24 * BSP1 + 12) = l_nam;
   *(char **)(a_brg + 24 * BSP2 + 12) = r_nam;

   form_center((OBJECT *) a_brg, &fgrect);

   w = *(int *)(a_brg + 24 * BK1 + 20);
   h = *(int *)(a_brg + 24 * BK1 + 22);
   objc_offset((OBJECT *) a_brg, BK1, &x, &y );
   bx[0] = x;
   by[0] = y + h;
   objc_offset((OBJECT *) a_brg, BK2, &x, &y);
   bx[1] = x + w;
   by[1] = y+h;

   sav_scr();
   form_dial(1, &g1, &fgrect);
   objc_draw((OBJECT *)a_brg, 0, 5, &scr640x400);
   ol[0] = ge[0];
   ol[1] = ge[1];
   ol[2] = pu[0];
   ol[3] = pu[1]; /* Sichern der alten*/
   ol[4] = ku[0];
   ol[5] = ku[1];
   ol[6] = vo[0];
   ol[7] = vo[1]; /* Werte */
   burg(2);
   burg(3);

   do
   {
      ge[0] = ge[1] = pu[0] = pu[1] = ku[0] = ku[1] = 9999;
      i = form_do((OBJECT *) a_brg, 0);
      *(int *)(a_brg + 24 * i + 10) = 0;

      if ((i == BL1) || (i == BR1))
      {
         bur[0] = (bur[0] + 1 - 2 * (i == BL1) + b_anz) % b_anz;
         clr(bx[0], y, w, h);
         burg(2);
      }

      if ((i == BL2) || (i == BR2))
      {
         bur[1] = (bur[1] + 1 - 2 * (i == BL2) + b_anz) % b_anz;
         clr(bx[1] - w, y, w, h);
         burg(3);
      }
   } while ((i != BOK) && (i != BAB));

   ge[0]=ol[0]; ge[1]=ol[1];  pu[0]=ol[2]; pu[1]=ol[3];
   ku[0]=ol[4]; ku[1]=ol[5];  vo[0]=ol[6]; vo[1]=ol[7];

   lod_scr();
   form_dial(2, &g1, &fgrect);

   fn();
   by[0] = oy0;
   by[1] = oy1;

   if (i == BAB)
   {
      bur[0]=ob0;
      bur[1]=ob1;
   }
   return i == BOK;
}


/**************************** Tabelle ****************************************/
static void tabelle(void)
{
   int i,j;
   hide(); vsf_interior(handle,0);
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
   for(i = 244; i < 340; i += 24)
   	line(56, i, 584, i);
   for(i = 0; i < 6; i++)
   	v_gtext(handle, 160 + i * 72, 78, t_na[i]);
   for(i = 0; i < 6; i++)
   	v_gtext(handle, 80, 110 + i * 24, t_na[i]);
   vsf_interior(handle,2); vsf_style(handle,2);
   for (i = 0; i < 6; i++)
   {
	   for(j = 0; j < 10; j++)
	   {
	      z_txt(t_gew[i][j]);
	      if ((j == 9) && (!t_gew[i][6]))
	      {
	      	txt[0] = 32;
	      	txt[1] = '-';
	      	txt[2] = 0;
	      }
	      v_gtext(handle, 176+i*72, 110 + j * 24 + 8 * (j > 5), txt);
	      if (i == j)
	      {
	      	/* BUG: The colour parameter was missing */
	         box(152 + i * 72, 92 + j * 24, 224 + i * 72, 116 + j * 24, 1);
	      }
	   }
	}
   v_gtext(handle,64,262,"\344  Spiele"); v_gtext(handle,64,286,"\344gewonnen");
   v_gtext(handle,64,310,"\344verloren"); v_gtext(handle,64,334,"Siege in%");
   line(56,59,144,84); vst_height(handle,4,&i,&i,&i,&i);
   v_gtext(handle,60,81,"VERLOREN"); v_gtext(handle,92,66,"GEWONNEN");
   vst_height(handle,13,&i,&i,&i,&i); show();
}


/* Verwaltung von Ereignissen: Messages, Maus oder Timer */ 
static int event(void)
{
   int wh;
   int dummy;
   static const GRECT dummy_g = {0, 0, 0, 0};
   static const GRECT g1 = {190, 20, 30, 20};
   static const GRECT g2 = {260, 20, 30, 20};
   static const GRECT g3 = {48, 52, 548, 308};

   wh = evnt_multi(
   				50,		/* evtypes */
   				1,1,1,	/* nclicks, bmask, bstate */
   				0, &dummy_g,
   				0, &dummy_g,
   				m_buf,	/* message buffer */
   				300,		/* milliseconds */
   				&ev,		/* x, y, bstate, kstate */
               &dummy,	/* keycode */
               &dummy	/* nbclicks */
               );
   if ((wh & 16) && (m_buf[0] == 10))
   {
      menu(0);
      if (m_buf[4] == MN_INFO)
      {
      	obj_do(a_inf);
      }
      else
      if (m_buf[4] == SPI1)     /* Neues Spiel */
      {
         if (bur_obj())
         {
            neues();
            werdran(1);
         }
      }
      else
      if (m_buf[4] == SPI2)     /* Namen eingeben */
      {
         memmove((void *) (**(long **)(a_nam + NSP1 * 24 + 12)), nsp1, 20);
         memmove((void *) (**(long **)(a_nam + NSP2 * 24 + 12)), nsp2, 20);
         if (obj_do(a_nam) == NOK)
         {
            memmove(nsp1, (void *) (**(long **)(a_nam + NSP1 * 24 + 12)), 20);
            memmove(nsp2, (void *) (**(long **)(a_nam + NSP2 * 24 + 12)), 20);
            if (!end)
            {
            	werdran(1);
            }
         }
      }
      else
      if (m_buf[4] == SPI4) /* Optionen */
      {
         *(int*)(a_opt+A_ERL*24+10)=an_erl;
         *(int*)(a_opt+A_VER*24+10)=!an_erl;
         *(int*)(a_opt+MAX_1*24+10)=!mxin;
         *(int*)(a_opt+MAX_2*24+10)=(mxin==1);
         *(int*)(a_opt+MAX_XX*24+10)=(mxin==2);
         *(int*)(a_opt+MAX_NE*24+10)=(mxin==3);
         *(int*)(a_opt+A_KAP*24+10)=au_kap;
         if( obj_do(a_opt)==OP_OK )
         {
            char *h;
            au_kap=*(int*)(a_opt+A_KAP*24+10);
            an_erl=*(int*)(a_opt+A_ERL*24+10);
            mxin=*(int*)(a_opt+MAX_2*24+10)+
                 2**(int*)(a_opt+MAX_XX*24+10)+
                 3**(int*)(a_opt+MAX_NE*24+10);
            h=(char *)(**(long **)(a_opt + MAX_XX * 24 + 12));
            max_rund=h[0]-48;
            if(h[1])
               { max_rund=max_rund*10+h[1]-48; if(h[2])
                 max_rund=max_rund*10+h[2]-48; }
            if(!mxin) max_rund=20; if(mxin==1) max_rund=50;
            if(mxin==3) max_rund=32767;
         }
      }
      else
      if (m_buf[4] == SPI3)
      {
      	return 1;
      }
      else
      if ((m_buf[4] >= MOD1) && (m_buf[4] <= MOD4))
      {                          /* Spielmodus ( wer gegen wen ) */
         menu_icheck(a_men, MOD1 + mod, 0);
         mod = m_buf[4] - MOD1;
         menu_icheck(a_men, m_buf[4], 1);
      }
      else
      if (m_buf[4] == MOD5)
      {                       /* Computer auswÑhlen */
         obj_do(a_com);
         for (cw[0]=0; cw[0]<7; cw[0]++ )
            if( *(int *)(a_com+24*(CN1+cw[0])+10) ) break;
         for (cw[1]=0; cw[1]<7; cw[1]++ )
            if( *(int *)(a_com+24*(CN2+cw[1])+10) ) break;
         for (cx[0]=0; cx[0]<4; cx[0]++ )
            if( *(int *)(a_com+24*(CTS1+cx[0])+10) ) break;
         for (cx[1]=0; cx[1]<4; cx[1]++ )
            if( *(int *)(a_com+24*(CTS2+cx[1])+10) ) break;
      }
      else if (m_buf[4] == REG)     /* Spielregeln */
      {
         int a=0,b;
         OBJECT *c;

         form_center((OBJECT *) a_re1, &fgrect);
         form_center((OBJECT *) a_re2, &fgrect);
         form_center((OBJECT *) a_re3, &fgrect);
         sav_scr();
         form_dial(1, &g1, &fgrect);
         do
         {
         	c = a ? ((a < 2) ? ((OBJECT *) a_re2) : ((OBJECT *) a_re3)) : ((OBJECT *) a_re1);
            objc_draw(c, 0, 5, &scr640x400);
            b = form_do( c,0 );
            *(int *)(c + 24 * b + 10) = 0;
            if( a==1 && b==R21 || a==2 && b==R31 ) { b=-1; a=0; }
            if( a==0 && b==R12 || a==2 && b==R32 ) { b=-1; a=1; }
            if( a==0 && b==R13 || a==1 && b==R23 ) { b=-1; a=2; }
         } while(b < 0);

         lod_scr();
         form_dial(2, &g1, &fgrect);
      }
      else
      if (m_buf[4] == ERG1)
      {
         memmove((void *) (**(long **)(a_ein + EI_N1 * 24 + 12)), t_na[0], 8);
         memmove((void *) (**(long **)(a_ein + EI_N2 * 24 + 12)), t_na[1], 8);
         memmove((void *) (**(long **)(a_ein + EI_N3 * 24 + 12)), t_na[2], 8);
         memmove((void *) (**(long **)(a_ein + EI_N4 * 24 + 12)), t_na[3], 8);
         memmove((void *) (**(long **)(a_ein + EI_N5 * 24 + 12)), t_na[4], 8);
         memmove((void *) (**(long **)(a_ein + EI_N6 * 24 + 12)), t_na[5], 8);

         *(int*)(a_ein + 24 * ER_LOE + 10) = 0;
         *(int*)(a_ein + 24 * ER_BLE + 10) = 1;

         if (obj_do(a_ein) == ER_OK)
         {
            int i,j;
            memmove(t_na[0], (void *) (**(long **)(a_ein + EI_N1 * 24 + 12)), 8);
            memmove(t_na[1], (void *) (**(long **)(a_ein + EI_N2 * 24 + 12)), 8);
            memmove(t_na[2], (void *) (**(long **)(a_ein + EI_N3 * 24 + 12)), 8);
            memmove(t_na[3], (void *) (**(long **)(a_ein + EI_N4 * 24 + 12)), 8);
            memmove(t_na[4], (void *) (**(long **)(a_ein + EI_N5 * 24 + 12)), 8);
            memmove(t_na[5], (void *) (**(long **)(a_ein + EI_N6 * 24 + 12)), 8);
            if (*(int*)(a_ein+24*ER_LOE+10))
            {
               for(i = 0; i < 6; i++)
               {
               	for(j = 0; j < 10; j++)
               	{
               		t_gew[i][j] = 0;
               	}
               }
            }
         }
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
         sav_scr();
         form_dial(1, &g2, &g3);
         tabelle();
         ev.bstate = 0;
         while(!ev.bstate)
         {
         	maus();
         }
         lod_scr();
         form_dial(2, &g2, &g3);
      }
      if ((m_buf[4] >= MOD1) && (m_buf[4] <= MOD5))
      {
         if (mod < 2)
         	l_nam = nsp1;
         else
         	l_nam = cn[cw[0]];
         if (mod & 1)
         	r_nam = cn[cw[1]];
         else
         	r_nam = nsp2;
         if (!end)
         	werdran(1);
      }
      menu_tnormal(a_men, m_buf[3], 1);
      menu(1);
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


/* 5-stellige Zahl, rechtsbÅndig, ohne fÅhrende Nullen */
static void zahl(int nr, int wert ) 
{
   short i,a,b;
   char *adr;

   adr=*(char **)(a_sta+24*nr+12)+11;
   for( b=i=0,a=10000; i<5; i++,a/=10 )
   {
   	*adr++ = 48 + wert / a - 16 * ((wert < a) && (i < 4) && (!b));
   	b |= wert / a;
   	wert %= a;
  }
}


/* Ermittelt, ob Punkt gesetzt ist */
int loc(int x, int y)
{
   int a,b;
   v_get_pixel(handle, x, y, &a, &b);
   return a;
}


/* Anbauen */
static void anbau(void)  
{
   short s;
   char a[3];

   color(1);
   vsf_interior(handle, 2);
   vsf_style(handle, 9); 
   v_gtext(handle,280,375," Anbauen: " );
   v_gtext(handle,220,395," Verbleibende Steine: 20 "); s=20;
   graf_mouse(6,0);

   do
   {
      maus();
      if ((ev.bstate) && (n ? (ev.x > 624 - bg[0]) : (ev.x < bg[0] + 15) && (ev.y > 155)))
      {
         hide();
         if (!(loc(ev.x, ev.y)         || loc(ev.x + 1, ev.y + 1) || loc(ev.x - 1, ev.y - 1)) &&
              (loc(ev.x + 3, ev.y - 1) || loc(ev.x + 3, ev.y + 1) || loc(ev.x - 3, ev.y - 1) || loc(ev.x - 3, ev.y + 1)||
               loc(ev.x + 1, ev.y + 2) || loc(ev.x - 1, ev.y + 2) || loc(ev.x + 1, ev.y - 2) || loc(ev.x - 1, ev.y - 2) ))
         {
            vswr_mode(handle, 4);
            xy[0] = ev.x - 2;
            xy[1] = ev.y - 1;
            xy[2] = ev.x + 2;
            xy[3] = ev.y + 1;
            v_bar(handle, xy);
            vswr_mode(handle, 1);
            s--;
            a[0] = 48 + s / 10;
            a[1] = 48 + s % 10;
            a[2] = 0;
            v_gtext(handle, 396, 395, a);
         }
         show();
      }
   } while((s > 0) && (ev.bstate < 2));

   for (s = 360; s < 400; s++)
   {
   	memmove(scr + s * 80 + 26, buf + s * 80 + 26, 28);
   }
   graf_mouse(0, 0);
}


/********************* Von Markt aufzurufende Routinen ***********************/
static void fturm(void)  /* Fîrderturm bauen */
{
   int x,y,yy,i,t;
   static const int turm[] =
   {
      -4, 23,30,27,0,3,10,24,20,6,30,23,30,5,20,26,10,2,0,6,30,-9,
	   8,0,8,31,9,34,11,36,14,37,15,37,18,36,20,34,21,31,21,29,20,26,18,24,15,23,
	   14,23,11,24,9,26,8,29,-9, 14,23,15,37,-9, 10,25,19,35,-9, 10,35,19,25,-9,
	   21,30,21,0,-9, -1
	};

   hide();
   for( t=0;t<5;t++ ) if( ft[n][t].x==-1 ) break;
   x=639*n+f*(bg[0]+20+30*t); y=380;
   do
   {
      while( loc(x,y) && loc(x+29*f,y--) );
      yy=y;
      for( i=0;i<40;i++,y-- ) if( loc(x,y) && loc(x+29*f,y) ) break;
   } while( i<40 );
   y=yy;
   clr( x-29*n,y-70,30,70 );
   color( 1 ); draw( ft[n][t].x=x,ft[n][t].y=y,turm );
   show();
}


static void markt(void)
{
   int a,k,ko,t;

   GRECT g1 = { 0, 0, 30, 20 };

	g1.g_x = ev.x;
	g1.g_y = ev.y;
   form_center((OBJECT *) a_sta, &fgrect);
   sav_scr();
   form_dial(1, &g1, &fgrect);

   for( a=0;a<6;a++ ) zahl( SM1+a, p[a] );
   do
   {
      for( t=k=0;k<5; ) t+=(ft[n][k++].x>-1);
      for( ko=k=0;k<10; ) ko+=(ka[n][k++].x>-1);
      for( k=0;k<10;k++ )
         if( bg[1+k*2]>-1 && ka[n][k].x==-1 )
         {
            hide(); for( a=1;a<10;a++ )
               if( loc(639*n+(bg[1+k*2]+5+a)*f,by[n]-bg[2+k*2]-a) ) break;
            show(); if( a>9 ) break;
         }
      zahl( SH1,ge[n] ); zahl( SH2,t );
      zahl( SH3,ko    ); zahl( SH4,wx[n]>-1 );
      zahl( SH5,pu[n] ); zahl( SH6,ku[n] );
      zahl( SH7,vo[n] ); zahl( SH8,st[n] ); 
      for( a=0;a<6;a++ )
         *(short *)(a_sta+24*(SM1+a)+10)=8* ( ge[n]<p[a] || !an_erl && !a ||
           a==1 && (bg[0]+t*30>265||t>4) || a==2 && k>9 || a==3 && wx[n]>-1 );

      objc_draw((OBJECT *) a_sta, 0, 5, &scr640x400);

      a = form_do((OBJECT *) a_sta, 0);
      if( a>=0 ) *(short *)(a_sta+24*a+10)=0;
      if( a==SHK ) st[n]-=2*(st[n]>0);
      else if( a==SHG ) st[n]+=2*(st[n]<100);
      else if( a!=FERTIG && a-SM1>-1 && a-SM1<6)
      {
         ge[n]-=p[a-SM1];
         if( a<SM5 )
         {
            lod_scr(); drw_all();
            if( a==SM1 ) anbau();
            else if( a==SM2 ) fturm();
            else if( a==SM3 ) init_ka( k,639*n );
            else if( a==SM4 )
               { wx[n]=639*n+f*bg[23]; wy[n]=by[n]-bg[24]; werdran(1); }
            sav_scr();
         }
         else
           {pu[n]+=30*(a==SM5); ku[n]+=2*(a==SM6); drw_gpk(a-SM4); drw_gpk(0);}
      }
   } while( a!=FERTIG );

   lod_scr(); drw_all();
   form_dial(2, &g1, &fgrect);
}


/********************* Routinen zur Zielerkennung: ***************************/
/* Die Routinen berechnen die Zielkoordinaten fÅr den Kînig, Kanonen, Geld...*/
static void z_kn(void)
{
   zx=!n*639-f*(bh[21]+15);
   zy=by[!n]-bh[22]-5;
}

static void z_ka(void)
{
   short i;

   for( i=0;i<10;i++ ) if( ka[!n][i].x>-1 ) break;
   if( i<10 )
   {
      do i=Random()%10; while( ka[!n][i].x==-1 );
      zx=ka[!n][i].x+10; zy=ka[!n][i].y;
   }
}

static void z_ft(void)
{
   short i;

   for( i=0;i<5;i++ ) if( ft[!n][i].x>-1 ) break;
   if( i<5 )
      { zx=ft[!n][i].x-15*f; zy=ft[!n][i].y-10; }
}

static void z_ge(void)
{
   if( ge[!n]>100 )
      { zx=!n*639-f*(bh[25]+bh[31]/2); zy=by[!n]-bh[26]; }
}

static void z_pk(void)
{
   short i;

   if( ku[!n] || pu[!n]>19 )
   {
      i=Random()&2;
      zx=!n*639-f*(bh[27+i]+bh[33+i]/2); zy=by[!n]-bh[28+i];
   }
}


/* FÅhrt einen Zug des Computers durch */
static int comp(void)
{
   char wd;
   int i,t;
   register double x2=0,x,y,vx2,vx,vy=0,wi;

   st[n]=16;
   for( i=0;i<10;i++ ) if( ka[n][i].x>-1 ) break;
   if( pu[n]<20 && ge[n]>=p[4] ) { pu[n]+=30; ge[n]-=p[4]; }  /* Pulv. kaufen*/
   if( !ku[n] && ge[n]>=p[5] )   { ku[n]+=2;  ge[n]-=p[5]; }  /* Kug.  kaufen*/
   if( i>9 && ge[n]>=p[2] ) { init_ka(i=0,639*n); ge[n]-=p[2]; } /* Ka. kauf.*/
   if( (ft[n][0].x<0 || ft[n][1].x<0 || ft[n][2].x<0 ) && ge[n]>=p[1] &&
       cw[n]>2 )
      { ge[n]-=p[1]; fturm(); } /* Fîrderturm kaufen */
   drw_all();

   if ((i > 9) || pu[n]<20 || !ku[n] )
   	return -1;

        /* Jetzt kommen die Berechnungen fÅr den Schuss: */
   bh=burgen[bur[!n]];
   do
   	i = (int) (Random() % 10);
   while (ka[n][i].x == -1);

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

   y=ka[n][i].y-10-zy;    t=49-cx[n]*16;   /* Berechnen der Distanz */
   x=ka[n][i].x+9+8*f-zx -t/2+Random()%t; if( x<0 ) x=-x;

   wd=n? -wnd:wnd;

   x2=-1;
   for( vx=.5; vx<7 && ( x2<x || vy/3.5>vx ); )  /* Zeitberechnung bei */
   {                              /* verschiedenen X-Geschwindigkeiten */
      vx+=.4;
      t=x2=0; vx2=vx;
      while( vx2>0 && x2<x )
         { x2+=vx2; vx2+=(wd/2-vx2)/5000; t++; }
      vy=y/t+.5*G*t;
   }

   if( n ) vx=-vx;
   vvx=vx; vvy=vy;  /* Werte fÅr Schuss Åbergeben */

   if( vx<0 ) vx=-vx; 
   wi=atan2(vy,vx);
   ka[n][i].w=wi*P57;
   ka[n][i].p=(int)(4.0*vx/cos(wi)-1.6);

   return i;
}


/*************************** Audienz beim Kînig ******************************/
static char kna[] = "[0][Der Kînig meint:             |'",
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

static void koenig(void)
{
   char a[300],*s[20],k,t,j,i;
   static char *ltz[2];

   for( j=k=0;j<10;j++ ) k+=ka[n][j].x>-1;
   for( j=t=0;j<5;j++ )  t+=ft[n][j].x>-1;

   if( !(Random()%20) || kn[n]&16 || (kn[n]&15)>9 )
      form_alert(1,"[0][Der Kînig hat keine Lust,   |dich zu sprechen.][Schade]");
   else
   {
      kn8[46]=kn[n]>2559? 48+kn[n]/2560:32; kn8[47]=48+kn[n]%2560/256;

      j = Random();
      i = 2;
      s[0] = (j & 1) ? kn0 : kn10;
      s[1] = (j & 2) ? kn12 : kn6;
      if (j & 4)
      	s[i++] = kn14;

      if( ge[n]>p[1]&&t<3 ) s[i++]=kn5;
      if( st[n]>40&&vo[n]<bg[40]||st[n]>70 ) s[i++]=kn3;
      if( wx[n]<0 ) s[i++]=kn9;
      if( ge[n]>bg[37]&&vo[n]>bg[40]&&k>1 ) s[1]=t<2? kn1:kn2;
      if( t>2 ) s[i++]=kn2;
      if( k<1 && ge[n]<p[2] ) s[i++]=kn4;
      if( (kn[n]&15)>4 ) s[i++]=s[i++]=s[i++]=kn7;
      if( (kn[n]&15)>6 || !(Random()&7) && kn[n]>2048 ) s[i++]=s[i++]=kn8;
      if( zug<4 ) s[1]=s[2]=s[(i=4)-1]=kn11;
      do
         if( s[j=Random()%i]==ltz[n] ) s[i++]=s[i++]=kn13;
      while( s[j]==ltz[n] );

      strcpy( a,kna ); strcat( a,ltz[n]=s[j] ); strcat( a,kne );
      form_alert(1,a);
   }
   kn[n]|=16; kn[n]+=256; if( ~kn[n]&15 ) kn[n]++;
}


/************* Kanonenobjektbaum, Wahl von Winkel und Pulver *****************/
static int sch_obj(int k)
{
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
   char *aw,*ap, fl=1;

   aw=*(char **)(a_sch+24*WINK+12);
   ap=*(char **)(a_sch+24*PULV+12);  *(ap+2)=0;

   sav_scr();

   form_center((OBJECT *) a_sch, &fgrect);
   objc_offset((OBJECT *) a_sch, WINK,&xw,&yw );
   objc_offset((OBJECT *) a_sch, PULV,&xp,&yp );
   objc_offset((OBJECT *) a_sch, KAST,&xk,&yk );
   xk += 105 + f * 36;
   yk += 102;

   g1.g_x = ka[n][k].x + 10 * f;
   g1.g_y = ka[n][k].y - 10;
   g1.g_w = 20;
   g1.g_h = 14;
   form_dial(1, &g1, &fgrect);
   wi = ka[n][k].w;
   pv = ka[n][k].p;

   vsf_interior(handle, 1);

   do
   {
      if( pv>pu[n] ) pv=pu[n];
      *aw=48+wi/100; *(aw+1)=48+wi%100/10; *(aw+2)=48+wi%10;
      if(wi<100) { *aw=*(aw+1); *(aw+1)=*(aw+2); *(aw+2)=0; }
      *ap=48+pv/10; *(ap+1)=48+pv%10;
      if(pv<10)  { *ap=*(ap+1); *(ap+1)=0; }

      if( fl )
      {
         objc_draw((OBJECT *) a_sch, 0,4 , &scr640x400);
         hide();
         color(1);
         v_circle(handle, xk - 88 * f, yk - 60, 15);
         i = 0;
         while( fig[i]!=-1 )
            { xy[i]=xk-88*f+fig[i]; i++; xy[i]=yk-5+fig[i]; i++; }
         xy[i++]=xy[0]; xy[i++]=xy[1];
         v_fillarea( handle,i/2-1,xy );
         show();
      }
      g2.g_x = xw;
      g2.g_y = yw;
      objc_draw((OBJECT *) a_sch, 0, 4, &g2);
      g2.g_x = xp;
      g2.g_y = yp;
      objc_draw((OBJECT *) a_sch, 0, 4, &g2);

      if ((i != PL2) && (i != PR2) && (i != PL1) && (i != PR1) || fl)
      {
         hide();
         clr(xk - 55, yk - 76, 110, 90);
         color( 1 );
         v_circle( handle,xk,yk,15 );

         s=sin(wi/P57); c=cos(wi/P57);
         fl=-f; if( wi>90 ) { fl=-fl; c=-c; }
         xy[0]=xk+fl*(c*14+s*14);  xy[1]=yk+s*14-c*14;
         xy[2]=xk+fl*(c*14+s*40);  xy[3]=yk+s*14-c*40;
         xy[4]=xk-fl*(c*55-s*40);  xy[5]=yk-s*55-c*40;
         xy[6]=xk-fl*(c*55-s*14);  xy[7]=yk-s*55-c*14;
         xy[8]=xy[0]; xy[9]=xy[1];
         v_fillarea( handle,4,xy );
         show();
         fl=0;
      }

      i = form_do((OBJECT *) a_sch, 0);
      if (i >= 0)
      {
      	*(int *)(a_sch + 24 * i + 10) = 0;
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
   } while( i!=SOK && i!=SAB );

   lod_scr();

   form_dial(2, &g1, &fgrect);
   ka[n][k].w = wi;
   ka[n][k].p = pv;
   return i == SOK;
}


static void baller(char r)
{
   static char s_bal[]={ 0,0,1,15,6,31,9,0,11,0,12,50,13,0,7,192,8,16,255,0 };

   s_bal[5] = r;
   Dosound(s_bal);
}


/****************************** Explosion ************************************/
static void expls(int x, int y, int w, int h, int d)
{
   int i,j;

   vswr_mode( handle,3 );
   for( i=0;i<32; ) { xy[i++]=x; xy[i++]=y; }
   while( d-->0 )
   {
      v_pline(handle, 2, xy);
      memmove(xy, xy + 4, 120);
      for (j = 28; j < 32;)
      {
      	xy[j++] = x - w + w * (int) ((Random() & 511) / 256);
      	xy[j++] = y - h + h * (int) ((Random() & 511) / 256);
      }
      v_pline(handle, 2, xy + 28);
      baller((d & 31) ^ 31);
   }
   for (i = 0; i < 32; i += 4)
   {
   	v_pline(handle, 2, xy + i);
   }
   vswr_mode(handle, 1);
}

static int kugel(int x, int y)
{
   if ((x < 3) || (x > 637) || (y < 23))
   	return 0;

   line( x-2,y-2,x+1,y-2 );
   line( x-3,y-1,x+2,y-1 );
   line( x-3,y  ,x+2,y   );
   line( x-3,y+1,x+2,y+1 );
   line( x-2,y+2,x+1,y+2 );
   return 1;
}


/********************************* Ein Schuss ********************************/
static void schuss(/* Nr. der Kanone */ int k)
{
   double x,y,ox,oy,vx,vy;
   short v,c,a, j;

   hide();
   pu[n]-=ka[n][k].p; ku[n]--;
   drw_gpk(1); drw_gpk(2);

   x=ka[n][k].x+9+8*f; y=ka[n][k].y-10; c=2;
   vx=(.4+0.25*ka[n][k].p)*cos( ka[n][k].w/P57 )*f;
   vy=(.4+0.25*ka[n][k].p)*sin( ka[n][k].w/P57 );

   if( mod&(2-n) ) { vx=vvx; vy=vvy; }

   color(1); vswr_mode( handle,3 );

   baller(0);

   kugel( (int)x,(int)y ); v=1;
   while( x>3 && x<637 && y<396 &&  ( v || c ) )  /* Flugschleife */
   {
      ox=x; oy=y; x+=vx; y-=vy;
      vy-=G; vx+=(wnd/2-vx)/5000;
      kugel( (int)ox,(int)oy );
      if( kugel( (int)x,(int)y ) )
         v=loc((int)x,(int)y)& loc((int)x-1,(int)y+1)& loc((int)x+1,(int)y+2);
      a=1000+2*y; if( a<30 ) a=30;
      Giaccess( 10,137 );    Giaccess( 244,135 );
      Giaccess( a&255,130 ); Giaccess( a>>8,131 );
      if( c ) c--;
   }
   kugel( (int)x,(int)y );
   vswr_mode( handle,1 ); color( 0 );
   oldn=n;
   if( !v ) for( c=0;c<4;c++ )      /* Einschlag der Kugel */
   {
      if( ox>6 && ox<634 ) v_circle( handle, (int)ox,(int)oy, 5 );

      baller(22+c*3);

      for( n=0;n<2;n++ )    /* Treffer ? */
      {
         bg=burgen[bur[n]];
         if( Random()&1 && (n? x>639-bg[0] : x<bg[0]) && vo[n] ) vo[n]--;
         fn();
         for( j=0;j<10;j++ )
            if( ka[n][j].x<ox+2 && ka[n][j].x+22>ox && ka[n][j].x>-1 &&
                ka[n][j].y-16<oy && ka[n][j].y>oy )
            {
               clr( a=ka[n][j].x,v=ka[n][j].y-12,20,13 );
               expls( a+10,v+6,13,8,50 ); color(0);
               ka[n][j].x=-1;
            }
         for( j=0;j<5;j++ )
            if( ft[n][j].x>-1 && drin( a=639*n+f*ft[n][j].x,v=by[n]-ft[n][j].y,
                30,37,2,(int)ox,(int)oy ) )
            {
               if( drin( a,v,30,37,-2,(int)ox,(int)oy ) )
               {
                  expls((a=ft[n][j].x-29*n)+15,(v=ft[n][j].y-40)+20,15,20,120);
                  clr( a,v,30,40 ); ft[n][j].x=-1;
               }
               c=4;
            }
         if( ox>wx[n]-11 && ox<wx[n]+11 && oy>wy[n]-16 && oy<wy[n] && wx[n]>-1)
            { clr( wx[n]-10,wy[n]-15,20,15 ); wx[n]=-1; }
         if( drin( bg[21],bg[22],30,25,0,(int)ox,(int)oy ) )
         {
            end=n+17;
            expls( a=639*n+f*(bg[21]+15),v=by[n]-bg[22]-12,17,17,40 );
            v_circle( handle, a,v,17 );
            expls( a,v,17,17,200 ); c=4;
         }
         for( j=0;j<6;j+=2 )
            if( drin( bg[25+j],bg[26+j],a=bg[31+j],v=bg[32+j],3,
                (int)ox,(int)oy ) ) break;
         switch( j )
         {
            case 0: ge[n]-=Min(200,ge[n]); break;
            case 2: pu[n]=0; break;
            case 4: ku[n]-=Min(2,ku[n]);
         }
         if( j<6 )
         {
            expls( 639*n+f*(bg[25+j]+a/2),by[n]-bg[26+j]-v/2,
                   a/2,v/2,60+100*(j==2) );
            drw_gpk(j/2); c=4;
         }
      }
      ox+=vx; oy-=vy;
   }
   n=oldn; fn();
   Giaccess( 0,137 );
   show();
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

   j = st[n];
   ge[n] += vo[n] * ((j > 65) ? (130 - j) : j) / (150 - (int) (Random() % 50));
   vo[n] = vo[n] * (95 + (int) (Random() % 11) / 100) + (21 - j + ((int) Random() % 9)) * (8 + (int) (Random() % 5))/20;
   if (vo[n] < 0)
   {
   	vo[n] = 0;
   	end = n + 49;
   }

   for (j = 0; j < 5;j++)
   {
   	ge[n] += (40 + (int) (Random() % 31)) * (ft[n][j].x > -1);
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

   form_center((OBJECT *) a_sie, &fgrect);
   *(int *)(a_sie+18)=30;
   *(char **)(a_sie+24*SG1+12)=s1;
   *(char **)(a_sie+24*SG2+12)=s2;
   *(char **)(a_sie+24*SG3+12)=s3;
   strcpy(s1, "!! ");
   strcat(s1, (end & 2) ? l_nam : r_nam);
   strcat(s1, " hat gewonnen !!" );
   s2[0] = 0;

   if (~end&64)
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
   objc_draw((OBJECT *) a_sie, 0, 7, &scr640x400);

   for (a = 0; (a < 6) && strncmp(t_na[a], l_nam, 7); a++);
   for (b = 0; (b < 6) && strncmp(t_na[b], r_nam, 7); b++);
   if(a<6 && b<6 && a!=b)
   {
      if(~end&2) { c=a; a=b; b=c; }
      t_gew[a][b]++; t_gew[b][8]++;
      t_gew[a][9]=100*++t_gew[a][7]/++t_gew[a][6];
      t_gew[b][9]=100*t_gew[b][7]/++t_gew[b][6];
   }
   hide();
   sav_ssp = (void *) Super(NULL);
   m_musik();
   Super(sav_ssp);
   show();  /* Musik... */
   Giaccess(0, 138);
   Giaccess(0, 139);
   Giaccess(0, 140);
}


/************************* DurchfÅhren eines Zuges ***************************/
static int ein_zug(void)
{
   int i,fl,a;

   n=zug&1; fn(); kn[n]&=~16;
   wnd=wnd*9/10+Random()%12-6;

   werdran(1);

   do
   {
      fl=0;
      menu(1);
      do
      {
         if (event())
         {
         	return 0;
         }

         for (a = i = 0; i < 60;)
         {
            a |= *(char *)(scr + 80 + i++);
         }
      } while (!ev.bstate && !(mod&(2-n)) || a);

      menu(0);
      bg = burgen[bur[n]];

      if (mod & (2 - n))
      {
         hide();
         i=comp();
         show();
         fl = 1 + (i < 0);
      }
      else
      if ((ev.x > ftx) && (ev.x < ftx + ftw) && (ev.y > fty) && (ev.y < fty + fth))
      {
        fl = 2;
      }
      else
      {
         for (i = 0; i < 10; i++)
         {
            if ((ka[n][i].x <= ev.x) && (ka[n][i].x + 20 >= ev.x) && (ka[n][i].x != -1) &&
                (ka[n][i].y >= ev.y) && (ka[n][i].y - 14 <= ev.y))
                	break;
         }

         if (i > 9)
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
         else if( pu[n]<5 )
          { bing(); form_alert(0,"[0][Dein Pulver reicht nicht!][Abbruch]");}
         else if( !ku[n] )
          { bing(); form_alert(0,"[0][Du hast keine Kugeln mehr!][Abbruch]");}
         else fl=sch_obj(i);
      }
   } while( !fl );

   werdran(0);
   if( fl<2 ) schuss(i);

   if( ~kn[n]&16 ) kn[n]&=~15;
   rechnen(); zug++;

   for( i=0;i<10;i++ ) if( ka[n][i].x>-1 ) break;
   n=zug&1; bg=burgen[bur[n]];
   for( a=0;a<10;a++ ) if( ka[n][a].x>-1 ) break;
      if( a==10 && i<10 && bg[40]>vo[n] && ge[n]<p[2]/3 && au_kap && 
          ft[n][0].x+ft[n][1].x+ft[n][2].x+ft[n][3].x+ft[n][4].x==-5 )
           end=n+33;

   if( !end && zug/2>=max_rund )
   {
      static h[2];
      for(n=0;n<2;n++)
      {
         h[n]=ge[n]+pu[n]*p[4]/30+ku[n]*p[5]/2+(wx[n]>-1)*p[3]+vo[n]*4;
         for(i=0;i<5;i++) if( ft[n][i].x>-1 ) h[n]+=p[1];
         for(i=0;i<10;i++) if( ka[n][i].x>-1 ) h[n]+=p[2];
      }
      end=65+(h[1]<h[0]);
   }
   if( end )
   {
      ende();
      menu(1);
      do
      {
         if (event())
         	return(0);
      }
      while((!ev.bstate) || end);

      menu(0);
   }

   return 1;
}


/*****************************************************************************/
main()
{
   int i;

   if( Getrez()<2 ) exit( form_alert(1,"[1]['Ballerburg' lÑuft nur|in der hohen Auflîsung][Abbruch]") );

   gem_init();
   scr = Logbase();

   m_laden("BALLER.MUS"); /* Laden der Musikdatei mit Funktion aus MUSIK.C */

   if( !rsrc_load( "BALLER.RSC" ) ) exit( form_alert(1,"[1][BALLER.RSC lÑût sich nicht|laden.][Abbruch]") );
   rsrc_gaddr(0,MENUE,    &a_men  );
   rsrc_gaddr(0,INFOTREE, &a_inf );
   rsrc_gaddr(0,SCHUSS,   &a_sch );  rsrc_gaddr( 0,BURG,&a_brg );
   rsrc_gaddr(0,NAMEN,    &a_nam  );  rsrc_gaddr( 0,DRAN,&a_dra );
   rsrc_gaddr(0,STATUS,   &a_sta );  rsrc_gaddr( 0,SIEGER,&a_sie );
   rsrc_gaddr(0,COMPUTER, &a_com); rsrc_gaddr( 0,REGEL1,&a_re1 );
   rsrc_gaddr(0,REGEL2,   &a_re2 );  rsrc_gaddr( 0,REGEL3,&a_re3 );
   rsrc_gaddr(0,OPTION,   &a_opt );  rsrc_gaddr( 0,EINTRAG,&a_ein );

   for (i = 0; i < 6; i++)
   {
   	*(int *)(((char *) a_men) + (MN_ACC1 + i) *24 +10 ) = 8;
   }
   *(int *)(((char *) a_com) + CN1 * 24 + 58) = 1;
   *(int *)(((char *) a_com) + CN2 * 24 + 58) = 1;
   *(int *)(((char *) a_com) + CTS1 * 24 + 34) = 1;
   *(int *)(((char *) a_com) + CTS2 * 24 + 34) = 1;

   buf = (char *) Malloc(42000L); /* Speicher fÅr Burgdaten und zweiten Schirm holen */
   if (buf == NULL)
   {
       exit(form_alert(1,"[1][Zu wenig Speicher!][Abbruch]"));
   }
 
   bur_ad = buf + 32000;
   an_erl=1; mxin=3; max_rund=32767; au_kap=1; t_load(); burgen_laden();

   menu_bar(a_men, 1);
   menu(0);

   v_show_c(handle, 0);
   graf_mouse( 0,0 );

   l_nam = nsp1;
   r_nam = nsp2;
   mod = 0;
   neues();
   while (ein_zug())
   	;

   t_save();
   menu_bar(a_men, 0);
   menu(1);
   v_clsvwk(handle);
   appl_exit();
   return 0;
}
