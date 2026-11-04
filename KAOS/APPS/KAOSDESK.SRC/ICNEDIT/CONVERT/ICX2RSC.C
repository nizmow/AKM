/*******************************************************************
*
*		    ICX2RSC.PRG						02.01.92
*		    ===========
*						    letzte énderung:	15.02.92
*
* Durchsucht das aktuelle Verzeichnis nach Dateien vom Typ
* .ICP und .ICD und erstellt fÅr diese eine .RSC Datei
* MAGXICNS.RSC
* Die Icons werden nach Dateinamen sortiert.
*
* geschrieben mit Pure C
* Projektdatei: MGEM.PRJ
*
*******************************************************************/

#include <aes.h>
#include <tos.h>
#include <tosdefs.h>
#include <string.h>
#include <stdlib.h>


extern void shelsort(char *base, size_t count, size_t size,
		           int (*compar)());


#define TRUE		1
#define FALSE		0
#define NULL		( ( void * ) 0L )

#define USA	0
#define FRG	1

#define COLS		7	/* Spalten fÅr Icons in der Ressource */

typedef struct
	{
	char	name[13];
	int	data[0x38];
	} ICONDATA;

ICONDATA *icons;
int n_icons;
int ohandle;

void create_rsc( void );
void write(long count, void *buf);
int  cmp_icn(ICONDATA *i1, ICONDATA *i2);

int main()
{
	long doserr;
	register int i;
	int	handle;


	/* ZÑhle alle auftretenden .ICD und .ICP Dateien */
	/* --------------------------------------------- */

	n_icons = 0;
	doserr = Fsfirst("*.icp", 0);
	while(doserr == E_OK)
		{
		n_icons++;
		doserr = Fsnext();
		}
	doserr = Fsfirst("*.icd", 0);
	while(doserr == E_OK)
		{
		n_icons++;
		doserr = Fsnext();
		}

	if	(n_icons == 0)
		{
#if		COUNTRY==FRG
		Cconws("Keine .ICD oder .ICP Datei gefunden!\r\n");
#elif	COUNTRY==USA
		Cconws("Neither .ICD nor .ICP files found!\r\n");
#endif
		return(0);
		}

	/* Speicher anfordern und sÑmtliche Icons einlesen */
	/* ----------------------------------------------- */

	icons = Malloc(n_icons * sizeof(ICONDATA));
	if	(!icons)
		return((int) ENSMEM);
	i = 0;

	doserr = Fsfirst("*.icp", 0);
	while(doserr == E_OK)
		{
		handle = (int) Fopen(Fgetdta()->d_fname, RMODE_RD);
		if	(handle < 0)
			return(handle);		/* Abbruch */
		strcpy(icons[i].name, Fgetdta()->d_fname);
		Fread(handle, 0x70L, icons[i].data);
		Fclose(handle);
		i++;
		doserr = Fsnext();
		}
	doserr = Fsfirst("*.icd", 0);
	while(doserr == E_OK)
		{
		handle = (int) Fopen(Fgetdta()->d_fname, RMODE_RD);
		if	(handle < 0)
			return(handle);		/* Abbruch */
		strcpy(icons[i].name, Fgetdta()->d_fname);
		Fread(handle, 0x70L, icons[i].data);
		Fclose(handle);
		i++;
		doserr = Fsnext();
		}

	/* Icons sortieren */
	/* --------------- */

	shelsort(icons, (size_t) n_icons, sizeof(ICONDATA), cmp_icn);

	ohandle = (int) Fcreate("MAGXICNS.RSC", 0);
	if	(ohandle < 0)
		return(ohandle);

	create_rsc();

	return(0);
}

void create_rsc( void )
{
	long ob;
	register int i;
	unsigned int strings;
	static int pgmm[0x38] =		/* Maske fÅr Programm- Icon */
				{
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
				};

	static int datm[0x38] =		/* Maske fÅr Datei- Icon */
				{
				0x1FFF, 0xFF00, 0x1FFF, 0xFF80,
				0x1FFF, 0xFFC0, 0x1FFF, 0xFFE0,
				0x1FFF, 0xFFF0, 0x1FFF, 0xFFF8,
				0x1FFF, 0xFFF8, 0x1FFF, 0xFFF8,
				0x1FFF, 0xFFF8, 0x1FFF, 0xFFF8,
				0x1FFF, 0xFFF8, 0x1FFF, 0xFFF8,
				0x1FFF, 0xFFF8, 0x1FFF, 0xFFF8,
				0x1FFF, 0xFFF8, 0x1FFF, 0xFFF8,
				0x1FFF, 0xFFF8, 0x1FFF, 0xFFF8,
				0x1FFF, 0xFFF8, 0x1FFF, 0xFFF8,
				0x1FFF, 0xFFF8, 0x1FFF, 0xFFF8,
				0x1FFF, 0xFFF8, 0x1FFF, 0xFFF8,
				0x1FFF, 0xFFF8, 0x1FFF, 0xFFF8,
				0x1FFF, 0xFFF8, 0x1FFF, 0xFFF8
				};

	static ICONBLK icnblk =
				{
				0L,		/* ib_pmask */
				0L,		/* ib_pdata */
				0L,		/* ib_ptext */
				0x1000,	/* ib_char */
				0,		/* ib_xchar */
				0,		/* ib_ychar */
				20,		/* ib_xicon */
				0,		/* ib_yicon */
				32,		/* ib_wicon */
				28,		/* ib_hicon */
				0,		/* ib_xtext */
				28,		/* ib_ytext */
				72,		/* ib_wtext */
				8,		/* ib_htext */
				};

	static OBJECT box_ob =
				{
				-1,		/* ob_next */
				1,		/* ob_head */
				0,		/* ob_tail */
				G_BOX,	/* ob_type */
				NONE,	/* ob_flags */
				OUTLINED,	/* ob_state */
				0x21100L,	/* ob_spec */
				0,		/* ob_x */
				0,		/* ob_y */
				70,		/* ob_width */
				10		/* ob_height */
				};

	static OBJECT icn_ob =
				{
				0,		/* ob_next */
				-1,		/* ob_head */
				-1,		/* ob_tail */
				G_ICON,	/* ob_type */
				NONE,	/* ob_flags */
				NORMAL,	/* ob_state */
				0L,		/* ob_spec */
				0,		/* ob_x */
				0,		/* ob_y */
				9,		/* ob_width */
				0x0402	/* ob_height */
				};

	static RSHDR rshdr =
				{
				0,		/* vrsn */
				0x28,	/* object,  Pos. der OBJECTs  */
				0x70,	/* tedinfo, Pos. der TEDINFOs */
				0x70,	/* iconblk, Pos. der ICONBLKs */
				0xb4,	/* bitblk,  Pos. der BITBLKs  */
				0xb4,	/* frstr,	  Pos. der freien Strings */
				0xb4,	/* string,  Pos. der gebundenen Strings */
				0xce,	/* imdata,  Pos. der Image- Daten */
				0xce,	/* frimg,	  Pos. der freien Images */
				0x24,	/* trindex, Pos. der Objektbaumtabelle */
				3,		/* nobs,	  Gesamtzahl der Objekte */
				1,		/* ntree,	  Gesamtzahl der ObjektbÑume */
				0,		/* nted,	  Gesamtzahl der TEDINFOs */
				2,		/* nib,	  Gesamtzahl der ICONBLKs */
				0,		/* nbb,	  Gesamtzahl der BITBLKs */
				0,		/* nstring, Strings */
				0,		/* nimages, Images */
				0		/* rssize,  GesamtlÑnge der Datei */
				};

	/* Header */
	/* ------ */

	rshdr.rsh_nobs = n_icons + 1;
	rshdr.rsh_nib  = n_icons;
	rshdr.rsh_tedinfo = (unsigned int)
					(
					sizeof(RSHDR) +
					rshdr.rsh_ntree * sizeof(long) +
					rshdr.rsh_nobs  * sizeof(OBJECT)
					);
	rshdr.rsh_iconblk = (unsigned int)
					(
					rshdr.rsh_tedinfo +
					rshdr.rsh_nted * sizeof(TEDINFO)
					);
	rshdr.rsh_bitblk  = (unsigned int)
					(
					rshdr.rsh_iconblk +
					rshdr.rsh_nib * sizeof(ICONBLK)
					);
	rshdr.rsh_frstr   = (unsigned int)
					(
					rshdr.rsh_bitblk +
					rshdr.rsh_nbb * sizeof(BITBLK)
					);
	rshdr.rsh_string  = (unsigned int)
					rshdr.rsh_frstr;
	strings =			(unsigned int)
					(
					rshdr.rsh_nib   * 13 + 1
					) & ~1;
	rshdr.rsh_imdata =	(unsigned int)
					(
					rshdr.rsh_string +
					strings
					);
	rshdr.rsh_frimg =	(unsigned int)
					rshdr.rsh_imdata;
	rshdr.rsh_rssize = (unsigned int)
					(
					sizeof(RSHDR) +
					rshdr.rsh_ntree * sizeof(long) +
					rshdr.rsh_nobs  * sizeof(OBJECT) +
					rshdr.rsh_nib   * sizeof(ICONBLK) +
					strings +
					rshdr.rsh_nib   * (0x70L + 0x70L)
					);
	write(sizeof(rshdr), &rshdr);

	/* BÑume */
	/* ----- */

	ob = 0x28L;
	write(4L, &ob);				/* nur ein Baum */

	/* Objekte */
	/* ------- */

	box_ob.ob_tail = n_icons;
	box_ob.ob_width  = 1 + 10 * COLS;
	box_ob.ob_height = 1 + 3 * (1 + n_icons / COLS);
	write(sizeof(box_ob), &box_ob);
	for	(i = 0; i < n_icons; i++)
		{
		if	(i == n_icons-1)
			{
			icn_ob.ob_next = 0;
			icn_ob.ob_flags = LASTOB;
			}
		else	icn_ob.ob_next = i+2;
		icn_ob.ob_spec.iconblk = (ICONBLK *)
							(rshdr.rsh_iconblk +
						     i * sizeof(ICONBLK));
		icn_ob.ob_x = 1 + 10 * (i % COLS);
		icn_ob.ob_y = 1 +  3 * (i / COLS);
		write(sizeof(icn_ob), &icn_ob);
		}

	/* ICONBLKs */
	/* -------- */

	for	(i = 0; i < n_icons; i++)
		{
		icnblk.ib_pmask = (int *)  (rshdr.rsh_imdata + i * (0x70 + 0x70));
		icnblk.ib_pdata = (int *)  ((long) icnblk.ib_pmask + 0x70);
		icnblk.ib_ptext = (char *) (rshdr.rsh_string + i * 13);
		write(sizeof(icnblk), &icnblk);
		}

	/* STRINGs */

	for	(i = 0; i < n_icons; i++)
		write(13L, icons[i].name);
	if	(n_icons & 1)
		write(1L, "");				/* gerade Anzahl Bytes erzwingen */

	/* IMAGES */
	/* ------ */

	for	(i = 0; i < n_icons; i++)
		{
		write(sizeof(icons[i].data), strstr(icons[i].name, ".ICP") ? pgmm : datm);
		write(sizeof(icons[i].data), &(icons[i].data));
		}
}


int cmp_icn(ICONDATA *i1, ICONDATA *i2)
{
	return(strcmp(i1->name, i2->name));
}


void write(long count, void *buf)
{
	long errcode;

	errcode = Fwrite(ohandle, count, buf);
	if	(errcode < 0L)
		{
#if		COUNTRY==FRG
		Cconws("Schreibfehler\r\n");
#elif	COUNTRY==USA
		Cconws("write error\r\n");
#endif
		err:
		Cnecin();
		Pterm((int) errcode);
		}
	if	(errcode != count)
		{
		errcode = -1L;
#if		COUNTRY==FRG
		Cconws("Diskette/Platte voll\r\n");
#elif	COUNTRY==USA
		Cconws("Disk full\r\n");
#endif
		goto err;
		}
}
