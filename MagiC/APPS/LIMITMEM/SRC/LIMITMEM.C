/*********************************************************************
*
*  LIMITMEM			14.01.91
*  ========
*
*  letzte Änderung		29.7.95 (franz. Texte)
*
*  Legt das Speicherlimit für ein Programm fest.
*
* Projektdatei: TTP.PRJ
*
*********************************************************************/

#include <mint/errno.h>
#include <mint/ostruct.h>
#include <mint/osbind.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned short ph_branch;
	uint32_t ph_tlen, ph_dlen, ph_blen, ph_slen, ph_res1, ph_res2;
	unsigned short ph_flag;
} PH;

#define RMODE_RW S_READWRITE
#define RMODE_RD 0
#define RMODE_WR 1
#define Fshrink(handle) Fwrite((handle), 0L, (void *) -1L)

char *nat_str(char *s);

void seek		(long pos);
void read		(long count, void *buf);
void write	(long count, void *buf);
void shrink	(void);
long modify	(long limit);

int file;

int main(int argc, char *argv[])
{
	long	limit;
	long err;
	char	*endptr;


	Cconws(nat_str(
		"\1\x08\xff" "MagiC- Speicherfestlegung, (C) 1990-95 Andreas Kromke\r\n\n\0"
		"\2\xff"   "MAGiC - Attribution de mémoire, (C) 1990-95 Andreas Kromke\r\n\n\0"
		"\xff"     "MagiC- Memory Limitation, \xbd 1990-95 Andreas Kromke\r\n\n"
		));

	/* Defaultkonfiguration einlesen */
	/* ============================= */

	if	(argc < 3)
		limit = 0L;
	else {
		if	(argv[2][0] == '-' && argv[2][1] == '\0')
			limit = -1L;
		else	{
			limit = strtol(argv[2], &endptr, 10);
			if	(limit <= 0L || *endptr)
				goto err;
			limit <<= 10;		/* kBytes -> Bytes */
			}
		argc--;
		}

	if	(argc != 2)
		{
		err:
		Cconws(nat_str(
			"\2\xff"	"Syntaxe: LIMITMEM nom_de_fichier [kbytes_déc|-]\r\n\0"
			"\xff"	"Syntax: LIMITMEM fname [kbytes_dec|-]\r\n"
		));
		return(1);
		}

	file = (int) Fopen(argv[1], RMODE_RW);
	if	(file < 0)
		return(file);

	err = modify(limit);
	Fclose(file);
	return((int) err);
}


/**************************************************************
*
* Modifiziert eine Datei <file>
*
**************************************************************/

void prtkb( long l )
{
     char	s[16];

	snprintf(s, sizeof(s), "%ld", l >> 10);
	Cconws(s);
	Cconws(" kBytes\r\n");
}

long modify(long limit)
{
     _DOSTIME  timedate;
	long ret;
	long	magics[2];
	char	c;
     long reloc_offs;
     PH	header;


     /* Dateiheader einlesen und Dateilänge setzen */
     /* ------------------------------------------ */

	read(sizeof(PH), &header);
	if	(header.ph_branch != 0x601a || header.ph_flag)
		return(-ENOEXEC);
	reloc_offs = sizeof(PH)  + header.ph_tlen
						+ header.ph_dlen
						+ header.ph_slen;

     /* Dateipointer auf die Relocation- Daten setzen */
     /* --------------------------------------------- */

     seek(reloc_offs);

	/* erstes Langwort der Relokationsdaten einlesen  */
	/* Bis zum Nullbyte durchsuchen				*/
	/* ----------------------------------------------	*/

	read(4L, &ret);
	if	(!ret)
		return(-ENOEXEC);

	do	{
		ret = Fread(file, 1L, &c);
		if	(ret < 0L)
			return(ret);
		if	(ret == 0L)
			return(-ENOEXEC);
		}
	while(c != 0);

	/* magische Daten lesen (falls schon vorhanden) */
	/* -------------------------------------------- */

	reloc_offs = Fseek(0L, file, 1);	/* aktuelle Position merken */
	magics[1] = -1L;
	ret = Fread(file, 8L, magics);
	if	(ret < 0L)			/* Lesefehler */
		return(ret);

	if	(ret && (ret < 8L || magics[0] != 0x4d414758L))
		/* irgendwelche Daten sind da */
		{
		Cconws(nat_str(
			"\1\x08\xff" "Ungültiges Programmformat, trotzdem fortfahren (J/N) ? \0"
			"\2\xff"	 "Format de programme non valable, procéder malgré tout (O/N) ? \0"
			"\xff"     "Invalid program format, continue (Y/N) ? "
			));
		c = Cconin() & 0x5f;
		if	((c != 'J') && (c != 'Y') && (c != 'O'))
			return(EBREAK);
		magics[1] = -1L;
		}

	Cconws(nat_str(
		"\1\x08\xff" "\r\nAktuelle Größe des Heap: \0"
		"\2\xff"   "\r\nTaille actuelle du Heap: \0"
		"\xff"     "\r\nActual heap size: "
		));
	if	(magics[1] != -1L)
		prtkb(magics[1]);
	else Cconws(nat_str(
			"\1\x08\xff" "unbeschränkt\r\n\0"
			"\2\xff"   "illimité\r\n\0"
			"\xff"     "unlimited\r\n"
			));


	ret = Fseek(reloc_offs, file, 0);	/* auf gemerkte Position */
	if	(ret < 0L)
		return(ret);

	if	(!limit)
		return(0);			/* nicht modifizieren */

	if	(limit == magics[1])
		{
		Cconws(nat_str(
			"\1\x08\xff" " Speicherfestlegung ist bereits wie gewünscht\r\n\0"
			"\2\xff"   " Attribution de mémoire déjà comme demandée\r\n\0"
			"\xff" 	 " Memory limitation already installed\r\n"
			));
		return(0);
		}

	/* magische Daten erstellen und speichern */
	/* -------------------------------------- */

	if	(Fdatime(&timedate, file, RMODE_RD))
		return(-1);
	if	(limit == -1L)
		{
		shrink();
		Cconws(nat_str(
			"\1\x08\xff" " Speicherbeschränkung aufgehoben\r\n\0"
			"\2\xff"   " Limitation de mémoire ôtée\r\n\0"
			"\xff" 	 " Memory limitation deinstalled\r\n"
			));
		}
	else	{
		magics[0] = 0x4d414758L;
		magics[1] = limit;
		write(8L, magics);
		shrink();			/* sicherheitshalber */
		Cconws(nat_str(
			"\1\x08\xff" " Speicher beschränkt auf \0"
			"\2\xff"   " Mémoire limitée à \0"
			"\xff" 	 " Memory limited to "
			));
		prtkb(magics[1]);
		}
	if	(Fdatime(&timedate, file, RMODE_WR))
		return(-1);

     return(0);
}


void seek(long pos)
{
	long err;

	err = Fseek(pos, file, 0);
	if	(err < 0L)
		Pterm((int) err);
	if	(err != pos)
		Pterm(-1);
}

void read(long count, void *buf)
{
	long err;

	err = Fread(file, count, buf);
	if	(err < 0L)
		Pterm((int) err);
	if	(err != count)
		Pterm(-1);
}

void write(long count, void *buf)
{
	long err;

	err = Fwrite(file, count, buf);
	if	(err < 0L)
		Pterm((int) err);
	if	(err != count)
		Pterm(-1);
}

void shrink( void )
{
	long err;

	err = Fshrink(file);
	if	(err < 0L)
		Pterm((int) err);
}


/****************************************************************
*
* Ermittelt die Nationalität und wählt Zeichenketten aus.
* Aufbau einer Zeichenkette:
*
* Nat1,Nat2,..,-1
*	string1
* Nat3,Nat4,..,-1
*	string2
* ...
* -1
*	Default-String
*
****************************************************************/


char *nat_str(char *s)
{
	while (*s != -1) {
		while (*s++ != -1)
			;
		while (*s)
			s++;
		s++;
	}
	return s + 1;
}
