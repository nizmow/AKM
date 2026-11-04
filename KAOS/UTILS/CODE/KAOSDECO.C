/*********************************************************************
*
*  KAOSDECO			21.12.90
*  ========
*
*  letzte énderung		29.03.91
*
*  Erzeugt KAOS mit Hilfe des ROMs, die Patches mÅssen von KAOSENCO in
*  die Programmdatei eingefÅgt werden.
*  Letztes Langwort ist die PrÅfsumme.
*  Erzeugt wird die Datei "KAOS".
*
*********************************************************************/

#include <tos.h>
#include <magix.h>
#include <tosdefs.h>

#define TOSLEN  0x30000L
#define TOSBASE 0xfc0000L

int	file;
long	buf[5+TOSLEN/sizeof(long)] =
	{ '____', '____', 'ANKR', 'DIKA' };	/* hier Daten einpatchen */
									/* statt '________' die */
									/* Seriennummer */
long	buf2[TOSLEN/sizeof(long)];

void read (long count, void *buf);
void write(long count, void *buf);


int main( void )
{
	long chksum,add;
	long	*bufp,*bufp2;
	long *data;
	char *s;


	Cconws("KAOSø- Decoder, Ω 1990/91 Andreas Kromke\r\n\n");

	/* Decodieren */

	add = 'WöRG';
	data = buf+4;
	for	(bufp2 = (long *) TOSBASE,bufp = data;
		 bufp < data+TOSLEN/sizeof(long); bufp++,bufp2++)
		{
		add   *= 7L;
		*bufp -= add;
		*bufp ^= *bufp2;
		}

	/* Checksumme berechnen */

	for	(chksum = 0L,bufp = data; bufp < data+TOSLEN/sizeof(long); chksum += *bufp++)
		;
	if	(chksum != data[TOSLEN/sizeof(long)])
		{
		Cconws("\r\n Sie haben entweder eine Ñltere TOS- Version, d.h."
			  "\r\n nicht Version 1.4, oder Sie haben eine verÑnderte"
			  "\r\n Version 1.4."
			  "\r\n Bitte setzen Sie TOS 1.4 - ROMs ein!\r\n");
		file = -1;
		err:
		Cnecin();
		return(file);
		}

	/* Speichern */

	add = data[5] - data[2];
	s = ((char *) (data)) + add - 36;
	((long *) s)[0] = buf[0];
	((long *) s)[1] = buf[1];		/* Seriennummer */
	if	((file = (int) Fcreate("KAOS", 0)) < 0)
		{
		Cconws("Kann Datei KAOS nicht erstellen\r\n");
		goto err;
		}
	write(TOSLEN, data);
	if	(Fclose(file))
		{
		Cconws("Schreibfehler\r\n");
		goto err;
		}
	Cconws("Datei KAOS erstellt!\r\n");
	Cnecin();
	return(0);
}

void read(long count, void *buf)
{
	long errcode;

	errcode = Fread(file, count, buf);
	if	(errcode < 0L)
		{
		err:
		Cconws("Lesefehler\r\n");
		Cnecin();
		Pterm((int) errcode);
		}
	if	(errcode != count)
		{
		errcode = -1L;
		goto err;
		}
}

void write(long count, void *buf)
{
	long errcode;

	errcode = Fwrite(file, count, buf);
	if	(errcode < 0L)
		{
		Cconws("Schreibfehler\r\n");
		err:
		Cnecin();
		Pterm((int) errcode);
		}
	if	(errcode != count)
		{
		errcode = -1L;
		Cconws("Diskette/Platte voll\r\n");
		goto err;
		}
}
