/****************************************************************
*
*             WINFRAME                             26.12.97
*             ========
*                                 letzte Änderung:
*
* geschrieben mit Pure C V1.1
* Projektdatei: WINFRAME.PRJ
*
* Alternativer Window-Frame-Manager für MagiC 6
* Das Programm ist als SharedLib ausgeführt.
*
****************************************************************/

#define DEBUG 0

#include <string.h>
#include <stdlib.h>
#include "WINFRAME_SDK.H"
#include "GLOBALS.H"
#include "WIN_OBJS.H"
#include "WINFRAME.H"

#define SCREEN	0

extern char __text[];
typedef void *PD;

short	gl_hhbox, gl_hwbox, gl_hhchar, gl_hwchar;
int	ap_id;
int	ncolours;
GRECT scrg;
short aes_handle;		/* Screen-Workstation des AES */
short vdi_handle;
short	work_out[57], work_in[12];	 /* VDI- Felder für v_opnvwk() */
OBJECT *adr_window;

static void open_work(void);

WINFRAME_HANDLER old_wfh,new_wfh;
WINFRAME_SETTINGS *settings;
WORD h_inw;

extern void win_wbm_create( WININFO *w );
extern void win_wbm_skind( WININFO *w );
extern void win_wbm_ssize( WININFO *w );
extern void win_wbm_sslid( WININFO *w, WORD vertical );
extern void win_wbm_sstr( WININFO *w );
extern void win_wbm_sattr( WININFO *w, WORD chbits );
extern void win_wbm_calc( WORD kind, WORD *fg );
extern WORD win_wbm_obfind( WININFO *w, WORD x, WORD y );


/*********************************************************************
*
* Fensterrahmen-Manager beim AES anmelden
*
*********************************************************************/

static WORD sys_set_winframe_manager( WINFRAME_HANDLER *old_wfh,
						WINFRAME_HANDLER *new_wfh,
						WINFRAME_SETTINGS **set )
{
	WORD control[5] = { 0, 1, 1, 2, 1 };
	WORD intin[1] = { 6 };	/* Subcode 6: Fensterrahmen-Manager */
	WORD intout[1] = { 0 };
	long addrin[2] = { (long)old_wfh, (long)new_wfh };
	long addrout[1] = { 0 };
	AESPB pb = { control, aes_global, intin, intout, addrin, addrout };

	aes( &pb );
	if	(set)
		*set = (WINFRAME_SETTINGS *)addrout[0];
	return( intout[0] );
}


/*********************************************************************
*
* VDI initialisieren
*
*********************************************************************/

static void open_work(void)
{
	register int i;


	for  (i = 0; i < 10; work_in[i++] = 1)
		;
	work_in[10]=2;                     /* Rasterkoordinaten */
	v_opnvwk(work_in, &vdi_handle, work_out);
}


/*****************************************************************
*
* Bibliothek initialisieren.
* Die Bibliothek hat keine eigene ap_id. Trotzdem brauchen
* wir das appl_init() zur Initialisierung unseres global-Feldes.
*
*****************************************************************/

LONG cdecl slb_init( void )
{
	register int i;
	char *path,*name;


	/* Initialisierung */
	/* --------------- */

	if   ((ap_id = appl_init()) < 0)
		return(ERROR);
	i = aes_global[10];
	ncolours = (i > 8) ? 32767 : (1 << i);
	if	(ncolours < 16)
		return(-1L);
	wind_get_grect(SCREEN, WF_WORKXYWH, &scrg);

	/* RSC-Datei im Pfad der SLB suchen */
	/* -------------------------------- */

	path = __text-128;
	name = strrchr(path, '\\');
	if	(name)
		name++;
	else	name = path;
	strcpy(name, "winframe.rsc");
	if	(!rsrc_load(path))
		{
		form_xerr(EFILNF, "winframe.rsc");
		return(EFILNF);
		}

	aes_handle = graf_handle(&gl_hwchar, &gl_hhchar, &gl_hwbox, &gl_hhbox);
	vdi_handle = aes_handle;
	open_work();

	rsrc_gaddr(0, T_WINDOW, &adr_window);
	global_init();

	/* Manager konfigurieren */
	/* --------------------- */

	new_wfh.version = old_wfh.version = 2;
	new_wfh.wsizeof = sizeof(WININFO);
	new_wfh.whshade = 20;
	new_wfh.wbm_create = win_wbm_create;
	new_wfh.wbm_skind = win_wbm_skind;
	new_wfh.wbm_ssize = win_wbm_ssize;
	new_wfh.wbm_sslid = win_wbm_sslid;
	new_wfh.wbm_sstr = win_wbm_sstr;
	new_wfh.wbm_sattr = win_wbm_sattr;
	new_wfh.wbm_calc = win_wbm_calc;
	new_wfh.wbm_obfind = win_wbm_obfind;

	/* Manager anmelden */
	/* ---------------- */

	if	(!sys_set_winframe_manager(&old_wfh, &new_wfh,
					&settings))
		{
		form_xerr(ERROR, "WINFRAME.SLB");
		return(ERROR);
		}

	h_inw = settings->h_inw - 1;
	global_init2();
	return(E_OK);
}


/*****************************************************************
*
* Bibliothek aufräumen.
* Wir dürfen kein appl_exit() machen, weil wir keine
* ap_id haben. rsrc_free() dagegen wäre erlaubt, weil die
* RSC-Strukturen im global-Feld liegen.
*
*****************************************************************/

extern void cdecl slb_exit( void )
{

	/* AES-Funktionen wieder entfernen */
	/* ------------------------------- */

	sys_set_winframe_manager(&new_wfh, &old_wfh, NULL);

	rsrc_free();
	v_clsvwk(vdi_handle);
}


/*****************************************************************
*
* Bibliothek öffnen
*
*****************************************************************/

#pragma warn -par
extern LONG cdecl slb_open( PD *pd )
{
	return(E_OK);
}


/*****************************************************************
*
* Bibliothek schließen
*
*****************************************************************/

extern void cdecl slb_close( PD *pd )
{
}
#pragma warn .par
