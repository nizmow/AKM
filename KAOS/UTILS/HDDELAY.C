/*********************************************************************
*
*                  HDDELAY.TOS                  Andreas Kromke
*                  ===========                  05.02.92
*                               letzte énderung: 25.02.92
*
* éndert die Bootverzîgerung unter KAOS 1.4.2 B
*
*********************************************************************/



#include <tos.h>
#include <tosdefs.h>
#include <string.h>
#include <stdlib.h>

#define Sconfig(a,b)    gemdos(0x33, (int) (a), (long) (b)) /* KAOS 1.2 */
#define Bconfig(a,b)    bios(12, (int) (a), (int) (b)) /* KAOS 1.4 B */
#define cnf_setlpt    0
#define cnf_setiobits 1
#define cnf_hddelay   2
#define cnf_cmosbits  3
#define cnf_waitprt   4

#define EOS  '\0'
#define NULL 0L

/* extern long strtol();  ggf. fÅr andere Compiler einfÅgen */


void main()                     /* ggf.  "void" hier weglassen */
{

	char eingabe[80];
	char *ende;
	int  alt,neu;


	if	(Sconfig(0, NULL) < 0L || Sversion() < 0x1600)
		{
		Cconws("Programm lÑuft nur unter KAOS 1.4 B");
		Crawcin();
		Pterm(-1);
		}

	alt = (int) Bconfig(cnf_hddelay, -1);
	Cconws("KAOS 1.4.2 B: Bootverzîgerung ist ");
	ltoa(alt, eingabe, 10);
	Cconws(eingabe);
     do   {
          Cconws("\15\12neuer Wert ? ");
          eingabe[Fread(-1,3L,eingabe)] = EOS;
          neu = (int) strtol(eingabe,&ende,10);
          }
     while(*ende != EOS || neu < 0);

	alt = (int) Bconfig(cnf_hddelay, neu);
	if	(alt == neu)
		Cconws("\r\nabgespeichert\r\n");
	else Cconws("\r\nkeine Mega- Uhr da, Verzîgerung unverÑndert\r\n");
     Crawcin();
     Pterm0();
}
