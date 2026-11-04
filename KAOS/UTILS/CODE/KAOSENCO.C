/*********************************************************************
*
*  KAOSENCO			19.12.90
*  ========
*
*  letzte énderung		11.02.91
*
*  Codiert die Datei "ROMS" mit Hilfe der Datei "A:\TOS__6_4._89".
*  Die Patchdaten werden in KAOSDECO.TOS eingeschrieben.
*  Letztes Langwort ist die PrÅfsumme.
*
*********************************************************************/

#include <tos.h>
#include <kaos.h>
#include <tosdefs.h>

#define TOSLEN 0x30000L

int	file;
long	buf[1+TOSLEN/sizeof(long)];
long	buf2[TOSLEN/sizeof(long)];

void read (long count, void *buf);
void write(long count, void *buf);


int main( void )
{
	long chksum,add;
	long	*bufp,*bufp2;
	int	i,data[4];


	/* KAOS einlesen */

	Cconws("KAOSø- Encoder, Ω 1990/91 Andreas Kromke\r\n\n");
	if	((file = (int) Fopen("ROMS", RMODE_RD)) < 0)
		return(file);
	read(TOSLEN, buf);
	Fclose(file);

	/* TOS einlesen */

	if	((file = (int) Fopen("A:\\TOS__6_4._89", RMODE_RD)) < 0)
		return(file);
	read(TOSLEN, buf2);
	Fclose(file);

	/* Checksumme berechnen */

	for	(chksum = 0L,bufp = buf; bufp < buf+TOSLEN/sizeof(long); chksum += *bufp++)
		;

	/* Codieren */

	add = 'WöRG';
	for	(bufp2 = buf2,bufp = buf;
		 bufp < buf+TOSLEN/sizeof(long); bufp++,bufp2++)
		{
		add   *= 7L;
		*bufp ^= *bufp2;
		*bufp += add;
		}

	/* Speichern */

	buf[TOSLEN/sizeof(long)] = chksum;
	if	((file = (int) Fopen("KAOSDECO.TOS", RMODE_RW)) < 0)
		return(file);

	i = 0;
	do	{
		read(sizeof(int), &data[i]);
		i++;
		i %= 4;
		}
	while(data[i      ] != 'AN' || data[(i+1)%4] != 'KR' ||
		 data[(i+2)%4] != 'DI' || data[(i+3)%4] != 'KA');

	write(TOSLEN+sizeof(long), buf);
	Fclose(file);
	return(0);
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
