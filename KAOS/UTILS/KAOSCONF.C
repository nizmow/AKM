/*********************************************************************
*
*  KAOSCONF				11.88
*  ========
*
*  letzte énderung       	22.12.89
*
*  Konfigurationsprogramm fÅr KAOS 1.2 und 1.4
*
*********************************************************************/


#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <tosdefs.h>
#include <kaos.h>

#define	NFLAGS	9		/* Bits 0..NFLAGS-1 sind belegt */

int main()
{
	void gotoxy(int x, int y);
	void print(char *s);
	void out(long st, int pos);
	long config;
	unsigned long c;
	int  pos = 0;
	int  rmagic,wmagic;
	int  kaos12;


	kaos12 = (Sversion() == 0x1500);

	/* Titelzeile */
	/* ---------- */

	print("\x1b" "E\n" "\x1b" "f" "\x1b" "w"
			"\t\t      KONFIGURATIONSPROGRAMM FöR KAOS\r\n"
			"\t\t          (C) Andreas Kromke 1989\r\n"
		     "\t\t      ==============================="
			"\x1b" "Y" "\x37" "\x20"
			"________________________________________________________________________________\r\n"
			" UP,DOWN: WÑhlen  |  LEFT,RIGHT: éndern  |  RETURN:  öbernehmen  |  ^C: Abbruch"
		);

	/* KAOS- Version anzeigen   */
	/* ------------------------ */

	gotoxy(0, 22);
	if	(kaos12)
		print("KAOS 1.2.3");
	else {
		register int i;
		char	s[10];
		long ver;

		print("KAOS 1.4 vom ");
		ver = Sconfig(3, 0L);
		s[10] = '\0';
		for	(i = 9; i >=0; i--)
			{
			s[i] = '0' + (ver & 0xf);
			ver >>= 4;
			if	(i == 6 || i == 3)
				s[--i] = '.';
			}
		print(s);
		}

	/* alte Konfiguration lesen */
	/* ------------------------ */

	rmagic = (kaos12) ? 0 : 'AK';
	wmagic = (kaos12) ? 1 : 'EL';
	if	(EINVFN == (config = Sconfig(rmagic, 0L)))
		{
		Cconws("Kein KAOS installiert\r\n");
		return((int) EINVFN);
		}
	do	{
		out(config, pos);
		c = Cnecin();
		if	(c == 3)
			return((int) EBREAK);		/* CTRL-C */
		c >>= 16;
		if	(c == 0x0048)				/* Cursor hoch */
			pos--;
		if	(c == 0x0050)				/* Cursor runter */
			pos++;
		if	(c == 0x004b || c == 0x004d)	/* LFT,RT */
			config ^= (1L << pos);
		if	(pos < 0)
			pos = NFLAGS - 1;
		if	(pos >= NFLAGS)
			pos = 0;
		}
	while(c != 0x001c && c != 0x0072);

	/* Konfiguration sichern */
	/* --------------------- */

	Sconfig(wmagic, config);
	return(0);
}

void print(char *s)
{
	while(*s)
		Bconout(CON, *s++);
}

void invers(void)
{
	print("\x1b" "p");
}

void normal(void)
{
	print("\x1b" "q");
}

void gotoxy(int x, int y)
{
	static char *s = "\x1b" "Yxx";

	s[2] = ' ' + (char) y;
	s[3] = ' ' + (char) x;
	print(s);
}

void out(long st, int pos)
{
	register int i,s;
	static int  isfirst = 1;
	static char *lefts   = "\r\n Bit xx";
	static char *lines[] =
		{
		" Automatische PfadÅberprÅfung",
		" KAOS 1.2: Diskwechselsimulation im Desktop",
		" ^C- Abfrage vor und nach jedem DOS- Aufruf",
		"#^C- Abfrage bei zeichenorientierten Funktionen",
		"#Fastload",
		" KAOS 1.4: KompatibilitÑt zu TOS 1.4",
		"#KAOS 1.4: \"Smart Redraw\"            (GEM)",
		"#KAOS 1.4: Grow- und Shrinkboxen     (GEM)",
		"#KAOS 1.4: Halt nach TOS- Programmen (GEM)"
		};


	gotoxy(0,6);
	for	(i = 0; i < NFLAGS; i++)
		{
		if	(isfirst)
			{
			itoa(i, lefts+7, 10);
			print(lefts);
			print(": ");
			print(lines[i] + 1);
			}
		gotoxy(57, 7+i);
		if	(pos == i)
			invers();
		s = (st & (1L << i)) != 0L;
		if	(lines[i][0] == '#')
			s = !s;
		print((s) ? " ein " : " aus ");
		if	(pos == i)
			normal();
		if	(isfirst)
			{
			print("    Default: ");
			print((lines[i][0] == '#') ? " ein" : " aus");
			}
		}
	isfirst = 0;
}