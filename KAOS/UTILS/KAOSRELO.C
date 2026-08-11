/*********************************************************************
*
*                  KS14RELO.TOS                  Andreas Kromke
*                  ============                  22.10.89
*                               letzte énderung: 03.06.91
*						  Anpassung Version vom 23.2.92
*
*  Die Include- Dateien gelten fÅr Turbo C.  FÅr andere Compiler
*   Kommentar beachten.
*
*  Erzeugt die KAOS- Bootdatei "KAOS142.IMG" im aktuellen Verzeichnis.
*  Erwartet die KAOS- Datei "KAOS" im aktuellen Verzeichnis.
*  Gibt bei Fehlern die DOS- Fehlercodes zurÅck.
*
*********************************************************************/



#include <portab.h>
#include <tos.h>                /* Bei Megamax,DRI usw. hier "OSBIND.H" */
#include <tosdefs.h>
#include <string.h>


#define EOS  '\0'
#define NULL 0L

/* extern long strtol();  ggf. fÅr andere Compiler einfÅgen */


void main()                     /* ggf.  "void" hier weglassen */
{

     register  WORD *i,k,m;
               WORD hdl;
               WORD *buf;
               char eingabe[80];
               char *ende;
     register  LONG diff;
               LONG start,*pokeadr,pokewert;
     static    LONG chg[] = {
     					0x00426L,0x01574L,0x01f5aL,0x01f98L,0x0201eL,
     					0x02174L,0x02400L,0x03006L,0x03604L,0x09ba6L,
     					0x11b5aL,0x12906L,0x12a42L,0x12de2L,0x12f98L,
     					0x13842L,0x13942L,0x1aa80L,0x1b61eL,0x2621cL,
     					-1L
                            };



     buf = (WORD *) Malloc(0x30000L);
     if   (buf == NULL)
          Pterm((WORD) ENSMEM);
     /* Eingabe wiederholen, bis gÅltige, gerade Hexzahl gelesen wurde */
     do   {
          Cconws("\15\12An welche Adresse soll KAOS 1.4.2 B gebootet werden (HEX) ? $");
          eingabe[Fread(-1,8L,eingabe)] = EOS;
          start = strtol(eingabe,&ende,16);
          }
     while(*ende != EOS || start & 1L);
     hdl = (int) Fopen("KAOS",RMODE_RD);
     if   (hdl < 0)
          Pterm(hdl);
     if   (0x30000L != Fread(hdl,0x30000L,buf))
          Pterm(-1);
     Fclose(hdl);
     diff  = 0xfc0000L - start;
     for  (i = buf; i < buf + (196608L >> 1); i++)
          {
          k = (*i & 0xf0ff);
          m = (*i & 0xf0f8);
          if	(i == buf + (0x2622eL >> 1))
          	{
          	i++;
          	continue;
          	}
          if   (k == 0x6000 || *i == 0x6100 || m == 0x50c8)
               {
               i++;
               continue;
               }
		if	(i[-2] == 0xdcbc && i[-1] == 0x0000 && i[0] == 0x00fe)
			{
			i++;
			continue;
			}
		if	(i[-1] == 0xc03c && i[0] == 0x00fd)
			{
			i++;
			continue;
			}
          pokeadr  = (long *) i;
          pokewert = *pokeadr;
          if   (pokewert >= 0xfc0000L && pokewert < 0xff0000L)
               {
               *pokeadr -= diff;
               i++;
               }
          }
     for  (pokeadr = chg; *pokeadr != -1L; pokeadr++)
     /* Jetzt folgt eine C- Programmzeile zum Abgewîhnen... */
          *( (LONG *) (*pokeadr + (LONG) buf)) += diff;

     hdl = (int) Fcreate("kaos.img",0);
     if   (hdl < 0)
          Pterm(hdl);
     if   (0x30000L != Fwrite(hdl,0x30000L,buf))
          Pterm(-1);
     Fclose(hdl);
     Cconws("\12Datei \"kaos.img\" erstellt");
     Crawcin();
     Pterm0();
}
