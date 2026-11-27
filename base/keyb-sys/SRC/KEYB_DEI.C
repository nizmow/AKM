/*
*
* Dieses Programm erzeugt eine Tastaturtabelle "KEYTABLS.SYS".
* Die Datei muß in gemsys/magic/xtension liegen.
*
* Für die DE-ISO-Tastatur
*
* Neue Version mit "dead keys"
*
* Die Tabellenbytes stehen in KEYB_DEI.H, erzeugt aus base/keyb-sys/keymaps.json.
*
*/

#include <mint/osbind.h>
#include "KEYB_DEI.H"

int main( void )
{
	long ret;
	int hdl;
	int index;

	ret = Fcreate("KEYTABLS.SYS", 0);
	hdl = (int) ret;
	if	(ret < 0)
		return(hdl);

	for (index = 0; index < 9; index++)
		{
		ret = Fwrite(hdl, 128L, tab_order[index]);
		if	(ret != 128L)
			goto wr_err;
		}
	ret = Fwrite(hdl, sizeof(tab_dead), tab_dead);
	if	(ret != sizeof(tab_dead))
		goto wr_err;

	Fclose(hdl);
	return(0);

wr_err:
	Fclose(hdl);
	Fdelete("KEYTABLS.SYS");
	return(-1);
}
