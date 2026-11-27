/*
*
* This program creates the keyboard table "KEYTABLS.SYS".
* The file must be placed in gemsys/magic/xtension.
*
* This is for the czech keyboard
*
* Version with dead keys for the "Apple Extended Keyboard"
* From UK version created by Petr Jandík 28.04.2002
*
* Table bytes live in KEYB_CZ.H, generated from base/keyb-sys/keymaps.json.
*
*/

#include <mint/osbind.h>
#include "KEYB_CZ.H"

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
