/*********************************************************************
*
*  KAOSPTCH			07.08.90
*  ========
*
*  letzte Žnderung		23.01.92
*
*  Patcht die Defaultkonfiguration der Datei KAOS142.ROM.
*
*********************************************************************/

#include <tos.h>
#include <magix.h>
#include <tosdefs.h>
#include <string.h>
#include <stdlib.h>

#define SCB_BIT11   0x800		/* ELCO HD */
#define SCB_BIT12   0x1000
#define SCB_BIT13   0x2000
#define SCB_BIT14   0x4000
#define SCB_BIT15   0x8000

typedef struct magics {
	char		user[20];
	char		unused[8];
	int		pr_conf;
	char		kb_delay;
	char		kb_reprate;
	long		config;
	long		aes_magic;
	} MAGICS;

#define CONFLEN (sizeof(struct magics) - sizeof(long))

void bit(char *text, char *ein, char *aus, long mask);
void selectnat(unsigned int *conf);
void seek (long pos);
void read (long count, void *buf);
void write(long count, void *buf);

long newconfig;
int	file;


int main(int argc, char *argv[])
{
	char		*code = "Fr. Elisabeth Lohmeyer";
	unsigned  int os_conf;
	register  int i,len;
	long		offset;
	SYSHDR	hdr;
	MAGICS	mag;
	long		s;
	int		magicword;
	char		c;
	char		kennung[20];
	char		ibuf[10];


	Cconws("KAOS¿- Patchprogramm, ½ 1990 Andreas Kromke\r\n\n");

	/* Defaultkonfiguration einlesen */
	/* ============================= */

	if	(argc != 2)
		{
		Cconws("Syntax: KAOSPTCH dateiname\r\n");
		return(1);
		}
	file = (int) Fopen(argv[1], RMODE_RW);
	if	(file < 0)
		return(file);
	read(2L, &magicword);
	switch(magicword)
		{
		case 0x601a:	offset = sizeof(PH);		/* .RAM */
					break;
		case 0x601b:	offset = sizeof(AH);		/* .ABS */
					break;
		case 0x602e:	offset = 0L;				/* .ROM */
					break;
		default:		Fclose(file);
					return((int) ERROR);
		}

	seek(offset);
	read(sizeof(SYSHDR), &hdr);
	s = offset + ((long) hdr.os_magic) - ((long) hdr.os_base) - CONFLEN;
	seek(s);
	read(sizeof(MAGICS), &mag);
	if	(mag.aes_magic != 0x87654321L)
		{
		Cconws("Formatfehler\r\n");
		return(-1);
		}

	/* Aktuelle Konfiguration anzeigen */
	/* ------------------------------- */

	newconfig = mag.config;
	Cconws("Benutzerkennung:          ");
	if	(mag.user[0] != 0x21 || mag.user[1] != 0x07 ||
		 mag.user[2] != 0x19 || mag.user[3] != 0x65)
		{
		for	(i = 0; i < (mag.user[0] ^ code[0]); i++)
			Cconout(mag.user[i+1] ^ code[i+1]);
		}
	Cconws("\r\nNeue Kennung:             ");
	len = (int) Fread (0, 19L, kennung+1);
	if	(len)
		{
		kennung[0] = len;
		for	(i = 0; i < len+1; i++)
			kennung[i] ^= code[i];
		memcpy(mag.user, kennung, 20L);
		}
	Cconws("\r\n");

	bit("Fastload                  ", "aus", 	"ein",  SCB_NFAST);
	bit("Kompatibilit„t            ", "TOS", 	"KAOS", SCB_CMPTB);
	bit("Smart Redraw              ", "aus", 	"ein",  SCB_NSMRT);
	bit("Grow-/Shrinkboxen         ", "aus", 	"ein",  SCB_NGRSH);
	bit("Halt nach TOS- Programmen ", "aus", 	"ein",  SCB_NHALT);
	bit("Mens                     ", "Pulldown","Dropdown",  SCB_PULLM);
	bit("Bit 11 (ELCO HD)          ", "ein",     "aus",  SCB_BIT11);
	bit("Bit 12                    ", "ein",     "aus",  SCB_BIT12);
	bit("Bit 13                    ", "ein",     "aus",  SCB_BIT13);
	bit("Bit 14                    ", "ein",     "aus",  SCB_BIT14);
	bit("Bit 15                    ", "ein",     "aus",  SCB_BIT15);
	Cconws("\r\n");
	mag.config = newconfig;

	newconfig = (int) mag.pr_conf;
	bit("Druckertyp                ", "Typenrad",  			"Matrix", 			0x0001);
	bit("Farbe                     ", "Farbe",     			"s/w",				0x0002);
	bit("Hersteller                ", "Epson (960 Punkte)",	"Atari (1280 Punkte)",	0x0004);
	bit("Qualit„t                  ", "hohe Qualit„t",  		"Test", 				0x0008);
	bit("Schnittstelle             ", "RS232", 				"Centronics", 			0x0010);
	bit("Papiersorte               ", "Einzelblatt", 			"Endlos",				0x0020);
	Cconws("\r\n");
	mag.pr_conf = (int) newconfig;

	Cconws("\r\nTastatur- Verz”gerung:    ");
	ibuf[Fread(0, 3L, ibuf)] = '\0';
	if	(ibuf[0])
		mag.kb_delay = atoi(ibuf);

	Cconws("\r\nTastatur- Rate:           ");
	ibuf[Fread(0, 3L, ibuf)] = '\0';
	if	(ibuf[0])
		mag.kb_reprate = atoi(ibuf);


	Cconws("\r\nNationalit„t:             ");
	os_conf = hdr.os_palmode;
	selectnat(&os_conf);
	
	newconfig = (int) os_conf;
	bit("Farbmonitor:              ", "PAL", "NTSC", 1);
	if	(newconfig & 1)
		os_conf |= 1;
	else os_conf &= ~1;

	Cconws("\r\n\r\nŽndern ? ");
	c = Cconin();
	if	(c != 'J' && c != 'j')
		return(0);

	seek(s);
	write(CONFLEN, &mag);

	if	(os_conf != hdr.os_palmode)
		{
		seek(offset+0x1c);
		write(2L, &os_conf);
		}

	Fclose(file);
	Cconws("\r\n\n");
	Cconws(argv[1]);
	Cconws(" ge„ndert\r\n");
	return(0);
}

void bit(char *text, char *ein, char *aus, long mask)
{
	char c;

	Cconws("\r\n");Cconws(text);
	do	{
		Cconws("\x1b" "j");
		Cconws((newconfig & mask) ? ein : aus);Cconws("\x1b" "K");
		Cconws("\x1b" "k");
		c = (char) Cnecin();
		if	(c != '\r')
			newconfig ^= mask;
		}
	while(c != '\r');
}


void selectnat(unsigned int *conf)
{
	long c;
	char *ntab[] = {
				"USA",
				"Deutschland",
				"Frankreich",
				"Kleinbritannien",
				"Spanien",
				"Italien",
				"Schweden",
				"franz”sischsprachige Schweiz",
				"deutschsprachige Schweiz",
				"Trkei",
				"Finnland",
				"Norwegen",
				"D„nemark",
				"vielviel”l",
				"Niederlande"
				};
	unsigned int ncode = (*conf) >> 1;	/* palmode ausblenden */

	do	{
		Cconws("\x1b" "j");
		if	(ncode > 14)
			ncode = 14;
		Cconws(ntab[ncode]);
		Cconws("\x1b" "K");
		Cconws("\x1b" "k");
		c = ( Cnecin() & 0xffffffL);
		if	( (c & 0xff ) != '\r')
			{
			if	(c == 0x4b0000L)
				{
				if	(ncode > 0)
					ncode--;
				}
			else ncode++;
			}
		}
	while( ( c & 0xff ) != '\r');
	*conf = ((*conf)&1) | (ncode << 1);
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
