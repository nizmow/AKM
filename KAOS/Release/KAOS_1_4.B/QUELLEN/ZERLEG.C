/*******************************************************************
*
*		    ZERLEG.TTP							24.1.89
*		    ==========
*						    letzte Žnderung:	30.4.91
*
* TOS- Datei (192kB) in die 6 ROMs zerlegen.
*
*
* geschrieben mit TUBO C V1.0
*
*******************************************************************/

#include <tos.h>
#include <tosdefs.h>
#include <stddef.h>
#include <string.h>


char ibuf[192L*1024L],
     ebuf[32768L],
     obuf[32768L];

void zerleg(char datei[]);
void cerrws	(char *string);
char *get_name(char *path);


void main(int argc, char *argv[])
{
	if	(argc != 2)		  /* nicht gengend Argumente */
		{
		cerrws("Syntax: ZERLEG datei\r\n");
		Pterm(1);
		}

	zerleg(argv[1]);
	Pterm0();
}


/**************************************************************
*
* Zerlegt die Datei "datei".
*
**************************************************************/

void zerleg(char datei[])
{
		    int  ihdl,ohdl,ehdl;
	char	    name[20];
	char	    *ext;
	register int  i;
	register char *b;
	register long j;


	strcpy(name, get_name(datei));
	ext = strchr(name, '.');
	if	(ext == NULL)
		ext = name+strlen(name);
	strcpy(ext, "._ux");
	if	(0 > (ihdl = (int) Fopen(datei, RMODE_RD)))
		Pterm(ihdl);
	if	(192L*1024L != Fread(ihdl, 192L*1024L, ibuf))
		Pterm((int) EREADF);
	b = ibuf;
	for	(i = 4; i >= 2; i--)		/* U4,U3,U2 */
		{
		ext[3] = '0' + i;
		ehdl = (int) Fcreate(name,0);	/* even */
		ext[3] += 3;
		ohdl = (int) Fcreate(name,0); /* odd  */
		if	(ehdl < 0 || ohdl < 0)
			Pterm((int) EWRITF);
		for	(j = 0; j < 32768L; j++)	/* 32k pro Datei */
			{
			ebuf[j] = *b++;		/* even */
			obuf[j] = *b++;		/* odd  */
			}
		if	(0 > Fwrite(ehdl,32768L,ebuf) ||
			 0 > Fwrite(ohdl,32768L,obuf))
			Pterm((int) EWRITF);
		Fclose(ehdl);
		Fclose(ohdl);
		}
}


/**************************************************************
*
* Gibt einen <string> nach stderr aus, d.h. nach Handle 4,
* falls vorhanden. Sonst nach Handle -1.
*
**************************************************************/

void cerrws(char *string)
{
	extern BASPAG *_BasPag;


	Fwrite( (_BasPag->p_stdfh[STDERR]) ? STDERR : HDL_CON,
		  strlen(string), string);
}


/*********************************************************************
*
* Ermittelt zu einem vollen Pfadnamen den Zeiger auf den
* reinen Dateinamen
*
*********************************************************************/

char *get_name(char *path)
{
	register char *n;

	n = strrchr(path, '\\');
	if	(!n)
		{
		if	((*path) && (path[1] == ':'))
			path += 2;
		return(path);
		}
	return(n + 1);
}
