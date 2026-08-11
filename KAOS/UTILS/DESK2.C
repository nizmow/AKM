/*********************************************************************
*
*  DESK2				13.07.90
*  =====
*
*  letzte Žnderung		14.07.90
*
*  Startet das GEM 2.2 DESKTOP unter KAOS bzw. meldet es wieder ab.
*
*********************************************************************/

#include <gem2.h>
#include <tos.h>
#include <string.h>
#include <tosdefs.h>

char	buf[8192];
long get_inf(char *infname, int maxsize);

int main()
{
	long errcode;
	char	pgmpath[128],dummypath[128];
	char	*infname;
	int	version;
	int	maxsize;


	if	(appl_init() < 0)
		return(-1);
	version = _GemParBlk.global[0];
	if	(version >= 0x0200 || version < 0x0140)
		maxsize = 1024;
	else maxsize = 4096;		/* L„nge des Desktop- Environment */
		
	shel_rdef(pgmpath, dummypath);

	if	(pgmpath[0])			/* Es ist etwas da! */
		{
		pgmpath[0] = '\0';
		infname = "DESKTOP.INF";
		}

	else	{
		strcpy(pgmpath, "DESKTOP.APP");
		if	(!shel_find(pgmpath))
			return((int) EFILNF);
		infname = "DESK2-2.INF";
		}

	errcode = get_inf(infname, maxsize);
	if	(errcode >= 0L)
		shel_wdef(pgmpath, "");
	appl_exit();
	return((int) errcode);
}


long get_inf(char *infname, int maxsize)
{
	long errcode;
	int	handle;
	char	infpath[128];


	memset(buf, 0, maxsize);
	strcpy(infpath, infname);
	if	(!shel_find(infpath))
		return(EFILNF);
	errcode = Fopen(infpath, RMODE_RD);
	if	(errcode < 0L)
		return(errcode);
	handle = (int) errcode;
	errcode = Fread(handle, maxsize, buf);
	Fclose(handle);
	if	(errcode >= 0L)
		shel_put(buf, (int) maxsize);
	return(errcode);
}
