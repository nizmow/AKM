/*
*
* Dieses Programm erzeugt eine Tastaturtabelle "KEYTABLS.SYS".
* Die Datei muß in gemsys/magic/xtension liegen.
*
* Für die US-Tastatur
*
* Neue Version mit "dead keys"
*
* Die Tabellenbytes stehen in KEYB_US.H, erzeugt aus base/keyb-sys/keymaps.json.
*
*/

#include <mint/osbind.h>
#include "KEYB_US.H"

int main( void )
{
	long ret;
	int hdl;

	ret = Fcreate("KEYTABLS.SYS", 0);
	hdl = (int) ret;
	if	(ret < 0)
		return(hdl);

	ret = Fwrite(hdl, 128L, tab_unshift);
	if	(ret != 128L)
		{
		 wr_err:
		Fclose(hdl);
		Fdelete("KEYTABLS.SYS");
		return(-1);
		}
	ret = Fwrite(hdl, 128L, tab_shift);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_caps);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_altgr);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_shaltgr);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_altgr);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_alt);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_shalt);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_alt);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, sizeof(tab_dead), tab_dead);
	if	(ret != sizeof(tab_dead))
		goto wr_err;

	Fclose(hdl);
	return(0);
}
