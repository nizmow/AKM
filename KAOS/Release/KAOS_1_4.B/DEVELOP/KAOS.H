/*      KAOS.H

        TOS and KAOS Definition Includes

        Copyright (c) Andreas Kromke 1991
        All Rights Reserved.
*/

/* new GEMDOS- Calls */

#define Sconfig(a,b)    gemdos(0x33, (int) (a), (long) (b)) /* KAOS 1.2 */
/* extern long Sconfig(int mode, long value); */
#define Fshrink(a)      Fwrite(a, 0L, (void *) -1L)         /* KAOS 1.2 */
#define Mgrow(a,b)      Mshrink(a,b)                        /* KAOS 1.2 */
#define Mblavail(a)     Mshrink(a,-1L)                      /* KAOS 1.2 */

/* Sconfig - Modes */

#define SC_GET   0                                          /* KAOS 1.2 */
#define SC_SET   1                                          /* KAOS 1.2 */
#define SC_VARS  2                                          /* KAOS 1.4 */
#define SC_OWN   3                                          /* KAOS 1.4 */

/* Sconfig Bits */

#define SCB_PTHCK   0x001                                   /* KAOS 1.2 */
#define SCB_DSKCH   0x002                                   /* KAOS 1.2 */
#define SCB_BREAK   0x004                                   /* KAOS 1.2 */
#define SCB_NCTLC   0x008                                   /* KAOS 1.2 */
#define SCB_NFAST   0x010                                   /* KAOS 1.2 */
#define SCB_CMPTB   0x020                                   /* KAOS 1.4 */
#define SCB_NSMRT   0x040                                   /* KAOS 1.4 */
#define SCB_NGRSH   0x080                                   /* KAOS 1.4 */
#define SCB_NHALT   0x100                                   /* KAOS 1.4 */
#define SCB_RESVD   0x200                                   /* KAOS 1.4 */
#define SCB_PULLM   0x400                                   /* KAOS 1.4 */

/* Sconfig(2) -> */

typedef struct
   {
   char      *in_dos;                 /* Adresse der DOS- Semaphore */
   int       *dos_time;               /* Adresse der DOS- Zeit      */
   int       *dos_date;               /* Adresse des DOS- Datums    */
   long      dos_stack;               /* Stack fr DOS selbst       */
   long      pgm_superst;             /* Benutzerpgm.- Sup.stack    */
   void      **memlist;               /* Adresse der 3 MD- Listen   */
   void      *act_pd;                 /* Laufendes Programm         */
   void      *fcbx;                   /* Dateien                    */
   int       fcbn;                    /* L„nge von fcbx[]           */
   void      *dmdx;                   /* DMDs                       */
   void      *imbx;                   /* interne DOS- Speicherliste */
   void      (*resv_intmem)();        /* DOS- Speicher erweitern    */
   long      (*etv_critic)();         /* etv_critic des GEMDOS      */
   char *    ((*err_to_str)(char e)); /* Umrechnung Code->Klartext  */
   } DOSVARS;

/* GEMDOS- Device- Handles */

#define HDL_CON -1                                          /* TOS */
#define HDL_AUX -2                                          /* TOS */
#define HDL_PRN -3                                          /* TOS */
#define HDL_NUL -4                                          /* KAOS 1.2 */

/* GEMDOS- Standard- Handles */

#define STDIN   0                                           /* TOS */
#define STDOUT  1                                           /* TOS */
#define STDAUX  2                                           /* TOS */
#define STDPRN  3                                           /* TOS */
#define STDERR  4                                           /* TOS */
#define STDXTRA 5                                           /* TOS */

/* GEMDOS Error Codes */

#define EBREAK -68L /* user break (^C)                         KAOS 1.2 */
#define EXCPT  -69L /* 68000- exception ("bombs")              KAOS 1.2 */

/* GEMDOS Pexec Modes */

#define EXE_LDEX    0                                       /* TOS */
#define EXE_LD      3                                       /* TOS */
#define EXE_EX      4                                       /* TOS */
#define EXE_BASE    5                                       /* TOS */
#define EXE_EXFR    6                                       /* TOS 1.4 */

/* GEMDOS Fseek Modes */

#define SEEK_SET    0                                       /* TOS */
#define SEEK_CUR    1                                       /* TOS */
#define SEEK_END    2                                       /* TOS */

/* tail for default shell */

typedef struct
     {
     int  dummy;                   /* ein Nullwort               */
     long magic;                   /* 'SHEL', wenn ist Shell     */
     int  isfirst;                 /* erster Aufruf der Shell    */
     long lasterr;                 /* letzter Fehler             */
     int  wasgr;                   /* Programm war Grafikapp.    */
     } SHELTAIL;

/* shel_write modes */

#define SHW_IMMED        0                                  /* PC-GEM 2.x  */
#define SHW_CHAIN        1                                  /* TOS         */
#define SHW_DOS          2                                  /* PC-GEM 2.x  */

/* Event definition */

#define WM_UNTOPPED		30							/* GEM  2.x	*/
#define WM_BACKDROPPED   31                                 /* KAOS 1.4    */

/* AES wind_get()- Modes */

#define WF_OWNER         101                                /* KAOS 1.4    */
#define WF_WINDLIST      102                                /* KAOS 1.4    */

/* AES wind_set()- Mode */

#define WF_BACKDROP      100                                /* KAOS 1.4    */

/* Window definition */

#define HOTCLOSEBOX      0x1000                             /* GEM 2.x     */
#define BACKDROP         0x2000                             /* KAOS 1.4    */

/* Object states */

#define WHITEBAK         0x40                               /* TOS         */
#define DRAW3D           0x80                               /* GEM 2.x     */

/* AES function prototypes */

void _appl_yield    ( void );                               /* TOS         */
void vq_aes         ( void );                               /* TOS         */
void appl_yield     ( void );                               /* GEM 2.x     */
void appl_bvset     ( int  disks,  int harddisks );         /* GEM 2.x     */
void shel_rdef      ( char *cmd, char *dir );               /* GEM 2.x     */
void shel_wdef      ( char *cmd, char *dir );               /* GEM 2.x     */
int  menu_unregister( int menu_id );                        /* GEM 2.x     */
int  scrp_clear     ( void );                               /* GEM 2.x     */
int  xgrf_stepcalc  (                                       /* GEM 2.x     */
                      int orgw, int orgh,
                      int xc, int yc, int w, int h,
                      int *cx, int *cy,
                      int *stepcnt, int *xstep, int *ystep
                    );
int  xgrf_2box      (                                       /* GEM 2.x     */
                      int xc, int yc, int w, int h,
                      int corners, int stepcnt,
                      int xstep, int ystep, int doubled
                    );
int  menu_click     ( int val, int setit );                 /* GEM 3.x     */

