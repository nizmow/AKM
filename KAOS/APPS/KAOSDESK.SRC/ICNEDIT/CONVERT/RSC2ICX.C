/*******************************************************************
*
*		    RSC2ICX.PRG						04.01.92
*		    ===========
*						    letzte Žnderung:	04.01.92
*
* L„dt MAGXICNS.RSC und erstellt einzelne Icon- Dateien
*
* geschrieben mit TUBO C V2.03
* Projektdatei: MGEM.PRJ
*
*******************************************************************/

#include <aes.h>
#include <tos.h>
#include <tosdefs.h>
#include <string.h>
#include <stdlib.h>
#include <magix.h>

#define TRUE		1
#define FALSE		0
#define NULL		( ( void * ) 0L )


int main()
{
	long doserr;
	int ohandle;
	OBJECT *adr_box;
	ICONBLK *icn;


	if	(0 > appl_init)
		return(-1);
	if	(!rsrc_load("MAGXICNS.RSC"))
		{
		doserr = EFILNF;
		goto err;
		}
	if	(!rsrc_gaddr(0, 0,  &adr_box))
		goto err;

	do	{
		_appl_yield();
		if	(adr_box->ob_type != G_ICON)
			continue;
		icn = adr_box->ob_spec.iconblk;
		_appl_yield();
		doserr = Fcreate(icn->ib_ptext, 0);
		_appl_yield();
		if	(doserr < E_OK)
			goto err;
		ohandle = (int) doserr;
		_appl_yield();
		doserr = Fwrite(ohandle, 0x70L, icn->ib_pdata);
		_appl_yield();
		Fclose(ohandle);
		_appl_yield();
		if	((doserr >= 0L) && (doserr != 0x70L))
			doserr = EWRITF;
		if	(doserr < E_OK)
			goto err;
		}
	while(!((adr_box++->ob_flags) & LASTOB));
	doserr = E_OK;

	err:
	appl_exit();
	return((int) doserr);
}
