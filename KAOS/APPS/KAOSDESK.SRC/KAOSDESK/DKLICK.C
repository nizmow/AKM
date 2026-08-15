/*********************************************************************
*
* Dieses Modul enthÑlt die Bearbeitung aller Doppel-Mausklicks, also
*
* 1) ôffnen   von Unterverzeichnissen
* 2) Starten  von Programmen
* 3) Anzeigen/Drucken von Dateien
*
*********************************************************************/

#include <string.h>
#include <vdi.h>
#include <mgx_dos.h>	/* include before tos.h */
#include <tos.h>
#include <tosdefs.h>
#include "k.h"


/****************************************************************
*
* FÅr eine Datei wird die Standardapplikation ermittelt und ein
* Zeiger auf APPLICATION zurÅckgeliefert, falls vorhanden.
*
****************************************************************/

APPLICATION *file_to_defapp(char *filepath)
{
	register APPLICATION *a;
	register char *s;


	s = get_name(filepath);
	s = strchr(s, '.');
	if (s)
	{
		s++;
		for	(a = application; a < application + n_apps; a++)
		{
			if (a -> path[0] && (!strcmp(s, (a -> ext1)) ||
							   !strcmp(s, (a -> ext2))
							  ))
			return a;
		}
	}

	return NULL;
}


/****************************************************************
*
* FÅr ein Programm wird festgestellt, ob es sich um eine
* angemeldete Applikation handelt, und ggf. ein Zeiger auf
* APPLICATION zurÅckgeliefert.
* Die öbergabe eines Leerstrings sucht nach einem freien
* Eintrag.
*
****************************************************************/

APPLICATION *find_defapp(char *pgmpath)
{
	register APPLICATION *a;
	register char *s;

	s = get_name(pgmpath);

	for	(a = application; a < application + n_apps; a++)
	{
		if (!strcmp(s, get_name(a -> path)))
			return a;
	}

	return NULL;
}


/*********************************************************************
*
* Untersuchung des Objekts <obj> in <wnr.
* typ =     'GP'	GEM - Programm  oder zugehîrige Datei	}
*		  '_P'    TOS - Programm  oder zugehîrige Datei	}
*		  'PP'    TTP - Programm  oder zugehîrige Datei	} ausfÅhrbar
*		    p	 selbige im single mode				}
*		  '_B'	BAT								}
*		  'PB'	BTP								}
*		  '_T'	Text- Datei
*		  'FD'	Floppy- Disk
*		  'HD'	Hard  - Disk
*		  'RD'	RAM   - Disk
*		  '_O'    Ordner
*		  '_F'    Fenster
*		  'P_'	Papierkorb
*		  'D_'	Drucker
*		  '__'	undefiniert
*
* RÅckgabe  0, falls Datei
*           1, falls Pfad (Disk oder Ordner oder Fenster)
*		 -1, sonst
*
*********************************************************************/

int obj_typ(int wnr, int obj, char *path, MYDTA **f, int *typ)
{
	int ret;
	char mpath[200];


	*typ = '__';
	if (path == NULL)
		path = mpath;
	*path = EOS;
	ret = obj_to_path(wnr, obj, path, f);

	if (ret == 0)						/* Datei */
	{
		APPLICATION *a = NULL;

		*typ = suffixtyp(path);

		if (*typ == '_T')
			a = file_to_defapp(path);
		else
			if ((*typ & 0xff) == 'P')
				a = find_defapp(path);
		if (a)
			*typ = a -> typ;

		return ret;
	}

	if (obj == 0)
	{
		*typ = '_F';
		return ret;
	}

	if (wnr == 0)
	{
		switch(icon[obj-1].icontyp)
		{
			case I_ORDNER: *typ = '_O';break;
			case I_FLPDSK: *typ = 'FD';break;
			case I_HRDDSK: *typ = 'HD';break;
			case I_RAMDSK: *typ = 'RD';break;
			case I_PAPIER:	*typ = 'P_';break;
			case I_DRUCKR:	*typ = 'D_';break;
		}

		return ret;
	}

	*typ = '_O';
	return ret;
}


/*********************************************************************
*
* Doppelklick auf Fenster <wnr> bzw. Desktopicon (wnr == 0)
* Icon- Objeknummer <obj> bei Shift- Status <kbsh>.
* RÅckgabe 1: Icon nach Aktion wieder deselektieren.
*
* Textdateien: CTRL		in Texteditor laden
* 			   ALT		anzeigen
*
* Programme:   CTRL		immer Parameter holen (TTP)
*			   ALT		im Multitaskingmodus starten
*
*********************************************************************/

int dclick(int wnr, int obj, int kbsh)
{
	int ret,typ;
	int styp;
	char path[200];
	MYDTA *file;


	path[0] = EOS;
	ret = obj_typ(wnr, obj, path, &file, &typ);
	if (strlen(path) > 127)
	{
		err_alert(EPTHOV);
		return 0;
	}

	if (ret < 0)
	{
		switch(typ)
		{
			case 'D_':
			case 'P_':
#if		COUNTRY==FRG
				form_alert(1, "[3][Drucker und Papierkorb lassen|"
								"sich nicht îffnen.][Abbruch]");
#elif	COUNTRY==USA
				form_alert(1, "[3][Printer and Trash cannot|"
								"be opened.][Abort]");
#endif
				break;

			default:
				err_alert(EINTRN);
				break;
		}

		return 0;
	}

	/* Doppelklick auf Pfad (Ordner oder Disk) */
	/* --------------------------------------- */

	if (ret == 1)
	{

		/* Doppelklick auf Icon. Neues Fenster îffnen */
		/* ------------------------------------------ */
	
		if (wnr == 0)
		{
			opn_newwnd(path);
			return 0;
		}

		/* Doppelklick auf Ordner in Fenster		 */
		/* ------------------------------------------ */
	
		if (typ == '_O')
		{
			if (kbsh & K_ALT)
			{
				opn_newwnd(path);
			}
			else
			{
				strcpy(fenster[wnr].path, path);
				upd_path(wnr, FALSE);
			}
		}
	}

	/* Doppelklick auf Datei (Programm oder Textdatei oder ... ) 	*/
	/*  1) Datei ist kein Programm, CTRL gedrÅckt:				*/
	/*		Datei(en) mit Standard- Editor aufrufen				*/
	/*  2) Datei ist nicht ausfÅhrbar oder ALT gedrÅckt:			*/
	/*		Datei anzeigen									*/
	/*  3) Datei ist ausfÅhrbar (PGM,BAT,angem. Datei):			*/
	/*		starten										*/
	/* ------------------------------------------------------------- */

	else
	{
		styp = suffixtyp(path);

		if (((styp & 0x5f) != 'P') && (kbsh & K_CTRL))
		{

			/* CTRL+ALT mit Textdatei: Anzeigen		*/
			/* ----------------------------------------- */

			if (kbsh & K_ALT)
				prt_icns(wnr, 0);

			/* CTRL mit Textdatei: Editor starten		*/
			/* ----------------------------------------- */

			else
			{
				strcpy(path, ((adr_prgrms+PG_4)->ob_spec.tedinfo)->te_ptext);
				typ = suffixtyp(path);
				starten(path, typ, 0, 0, kbsh & ~K_CTRL);
			}
			return 0;
		}

		if (typ == '_T')
		{
			prt_icns(wnr, 0);
			return 0;
		}

	 	if (((typ & 0x5f) == 'P') || ((typ & 0xff) == 'B'))
	 	{
			starten(path, typ, wnr, obj, kbsh);
			return kbsh & K_ALT;
		}
	}

	return 0;
}


/****************************************************************
*
* Ein neues Fenster mit Pfad <path> wird geîffnet.
*
****************************************************************/

void opn_newwnd(char *path)
{
	register int wnr;
	long ret;


	for	(wnr = 1; wnr <= ANZFENSTER; wnr++)
	{
		if (fenster[wnr].path[0] == EOS)
		{
			/* Maske ist "*.*" 				*/
			/* ------------------------------- */
		
			strcpy(fenster[wnr].maske, "*.*");
		
			/* Pfad ist schon festgelegt		*/
			/* ------------------------------- */
		
			strcpy(fenster[wnr].path, path);
			ret = opn_wnd(wnr, FALSE);
			if (ret == ENOWND)
				goto err_wnd;
			err_alert(ret);
			return;
			}
		}
	err_wnd:
#if		COUNTRY==FRG
	form_alert(1, "[3][Keine Fenster mehr.][Abbruch]");
#elif	COUNTRY==USA
	form_alert(1, "[3][All windows used.][Cancel]");
#endif
	return;
}


/****************************************************************
*
* Eine Datei wird angezeigt.
*
****************************************************************/

unsigned char waitkey(void)
{
	int dummy,st;
	int was0 = FALSE;

	for (;;)
	{
		if (Bconstat(CON))
			return (unsigned char) Bconin(CON);

		vq_mouse(vdi_handle, &st, &dummy, &dummy);
		if (!(st & 3))
			was0 = TRUE;
		if (!was0)
			continue;
		if (st & 1)			/* linke Maustaste */
			return ' ';
		if (st & 2)			/* rechte Maustaste */
			return 'Q';
	}
}

void str_conout(char *s, int len)
{
	while (len--)
		Bconout(CON, *s++);
}

void init_vt52(void)
{
	str_conout("\x1b" "E"		/* Clear- Home				*/
		 	 "\x1b" "b\xff"	/* Schriftfarbe schwarz		*/
		 	 "\x1b" "c\0"		/* Hintergrundfarbe weiû		*/
		 	 "\x1b" "e"		/* Cursor einschalten		*/
		 	 "\x1b" "q"		/* Reverse aus				*/
		 	 "\x1b" "v", 14);	/* Automatischer öberlauf ein */
}

void exit_vt52(void)
{
	GRECT g;

	str_conout("\x1b" "H" "\x1b" "f", 4);	/* Home, Cursor aus */
	g.g_x = 0;
	g.g_y = 0;
	g.g_w = desk_g.g_x + desk_g.g_w;
	g.g_h = desk_g.g_y + desk_g.g_h;
	form_dial(FMD_FINISH, NULL, &g);
}

int show_file(char *path)
{
	char *buf;
	int mode = FALSE;
	int handle = 0;
	int ypos = 0;
	unsigned char t;
	long doserr,lbytes;
	const long bsize = 4*1024L;



	Mgraf_mouse(HOURGLASS);
	if (NULL == (buf = Malloc(bsize)))
		doserr = ENSMEM;
	else
		doserr = Fopen(path, RMODE_RD);

	if (doserr >= E_OK)
	{
		handle = (int) doserr;
		Mgraf_mouse(M_OFF);
		init_vt52();
		do
		{
			fillbuf:

			doserr = Fread(handle, bsize, buf);
			if (doserr > 0)
			{
				char c;
				int s;

				for	(lbytes = 0; lbytes < doserr; lbytes++)
				{
					while (Kbshift(-1) & (K_LSHIFT | K_RSHIFT))
						mode = TRUE;
					c = buf[lbytes];
					s = (int) Bconstat(CON);
					if (s || (!mode && c == '\n'))
					{
						mode = FALSE;
						ypos++;
						if (s || ypos > 22)
						{
							long fpos;

							if (!s)
#if		COUNTRY==FRG
								str_conout("\r\n--Mehr-- "
#elif	COUNTRY==USA
								str_conout("\r\n--More-- "
#endif
										 "\x1b" "e", 13);
							t = waitkey();
							if (!s)
								str_conout("\x1b" "l", 2);
							if (t >= 'a')
								t &= 0x5f;

							switch(t)
							{
								case 3:
								case 'Q':
								case 'N': doserr = EBREAK;
										break;
								case 'G': mode = TRUE;
										break;
								case ' ': ypos = 0;
										break;
								case 'D': ypos = 11;
										break;
								case '+': back:
								case '-':	str_conout("\r\n\n"
"------------------------------------------------------------------------------"
"\r\n\n", 84);
										fpos = Fseek(0L, handle, 1);
										if (fpos < E_OK)
										{
											doserr = EBREAK;
											break;
										}

										fpos += lbytes - doserr;
										fpos += (t == '+') ? 4096L : -4096L;
										if (fpos < 0)
											fpos = 0;
										fpos = Fseek(fpos, handle, 0);
										if (fpos < E_OK && fpos != ERANGE)
										{
											doserr = EBREAK;
											break;
										}

										ypos = 0;
										goto fillbuf;
								default: ypos = 22;
							}

							if (doserr == EBREAK)
								break;
							if (!s)
								continue;
						}
					}

					/* Anpassung fÅr Wordplus- Codierung */
					/* --------------------------------- */

					if (c == 0x1c || c == 0x1d || c == 0x1e)
						c = ' ';
					Bconout(CON, c);
				}
			}
		}
		while (doserr > 0); 
	}

	if (doserr < E_OK)
		err_alert(doserr);
	else
	{
		str_conout("\r\n--EOF-- " "\x1b" "e", 12);
		t = waitkey();
		if (t == '-')
		{
			lbytes = doserr = 0L;
			goto back;
			}
		}

	if (buf)
		Mfree(buf);

	if (handle)
	{
		Fclose(handle);
		exit_vt52();
		menu_bar(adr_hauptmen, TRUE);
		graf_mouse(M_ON, NULL);
	}

	Mgraf_mouse(ARROW);
	return 0;
}


/****************************************************************
*
* Eine Datei wird ausgedruckt.
*
****************************************************************/

int print_file(char *path)
{
	char *buf;
	register char *s;
	int handle = 0;
	int ohandle = 0;
	const long bsize = 1024L;
	long doserr;


	Mgraf_mouse(HOURGLASS);
	if (NULL == (buf = Malloc(bsize)))
		doserr = ENSMEM;

	else
		doserr = Fopen(path, RMODE_RD);

	if (doserr < E_OK)
		goto ende;

	handle = (int) doserr;
	doserr = Fopen("PRN:", RMODE_WR);
	if (doserr < E_OK)
		goto ende;

	ohandle = (int) doserr;

	do
	{
		if (qbreak())
		{
			doserr = EBREAK;
			break;
		}

		_appl_yield();
		doserr = Fread(handle, bsize, buf);
		_appl_yield();

		if (doserr < 0)
			break;

		for	(s = buf; s < buf + doserr; s++)
		{
			if (qbreak())
			{
				doserr = EBREAK;
				break;
			}

			_appl_yield();

			if (1L != Fwrite(ohandle, 1L, s))
			{
				doserr = EWRITF;
				break;
			}
		}
	}
	while (doserr > 0);

	ende:
	if (buf)
		Mfree(buf);
	if (handle)
		Fclose(handle);
	if (ohandle)
		Fclose(ohandle);
	Mgraf_mouse(ARROW);

	return (int) err_alert(doserr);
}


/****************************************************************
*
* FÅr ein Programm wird der Parameterstring zusammengesetzt.
* Dateien im selben Verzeichnis wie das Programm werden ohne
* vollstÑndigen Pfad Åbergeben.
*
****************************************************************/

void get_param(char *pgm, int wnr, int obj, char *tail)
{
	OBJECT *tree;
	register int iwnr,i;
	char path[128];
	char *endppath,rettc;


	endppath = get_name(pgm);
	rettc = *endppath;
	*endppath = EOS;
	for	(iwnr = 0; iwnr <= ANZFENSTER; iwnr++)
	{
		if (iwnr && !fenster[iwnr].path[0])
			continue;				/* Fenster unbenutzt */
		tree = fenster[iwnr].pobj;
		if ((i = (tree -> ob_head)) > 0)
		{
			for	(; i <= (tree -> ob_tail); i++)
			{
				if (wnr == iwnr && i == obj)
					continue;		/* Das Programm selbst */
				if (!((tree+i)->ob_flags & HIDETREE) && selected(tree, i))
				{
					char rettc1;
					char *e;
					int isne;

					if (obj_to_path(iwnr, i, path, NULL))
						continue;		/* Keine Datei */
					e = get_name(path);
					rettc1 = *e;
					if (!rettc1)
						continue;		/* Nullname */
					*e = EOS;
					isne = strcmp(pgm, path);
					*e = rettc1;
					if (isne)
						e = path;
					if (strlen(tail) + strlen(e) + 1 > 127)
						continue;		/* Zu lang */
					if (*tail)		/* Nicht Anfang */
						strcat(tail, " ");
					strcat(tail, e);
				}
			}
		}
	}

	*endppath = rettc;
}


/****************************************************************
*
* FÅr ein Programm wird der Pfad gesetzt.
*
* isover  = 0:   KAOSDESK bleibt resident
* isover  = 1:   KAOSDESK wird vom Speicher entfernt
* isover  = 100: Multitasking- EXEC fÅr MAGIX
* isover  = 101: Single Mode EXEC fÅr MAGIX
* stdpath = TRUE: Das Programm wird in den KAOSDESK- Pfaden gestartet
*
****************************************************************/

long setpath(char *program, int stdpath, int isover)
{
	long doserr;
	char rett;
	char *endp;
	char *path;
	register int i,top;


	top = top_wnr();		/* Nummer des obersten Fensters */
	if (!stdpath)		/* Programm- Pfad */
	{
		endp = get_name(program);
		rett = *endp;
		*endp = EOS;
		if (program[0] && program[1] == ':')
			Dsetdrv(program[0] - 'A');
		if (program < endp)
			doserr = Dsetpath(program);
		else
			doserr = E_OK;
		*endp = rett;
	}
	else
	{
		/* KAOSDESK- Pfad */
		for	(i = 1; i <= ANZFENSTER; i++)
		{
			path = fenster[i].path;
			if (i != top && (*path))
				Dsetpath(path);
		}

		if (top > 0)
		{
			path = fenster[top].path;
			if (*path)
			{
				Dsetdrv (path[0] - 'A');
				Dsetpath(path);
			}
		}

		doserr = E_OK;

	}

#if	MAGIXDESK == 0
	if (isover == SHW_CHAIN && !isgem2)	/* KAOSDESK wird verlassen */
		set_parent_paths();
#endif

	return err_alert(doserr);
}


/****************************************************************
*
* Ein Programm vom typ <typ> wird gestartet.
* Wenn <typ> = '#x', werden angemeldete Programme gestartet.
*
* Wenn der typ mit '#' beginnt, wird das Programm in den KAOSDESK-
* Ordnern gestartet.
* Wenn der zweite Buchstabe des Typs ein kleines 'p' ist, wird
* das Programm (unter MagiX) im Single Tasking Modus gestartet.
*
****************************************************************/

void starten(char *path, int typ, int wnr, int obj, int kbsh)
{
	char *pgm = fenster[0].path;		/* genÅgend Platz hier */
	int stdpath;			/* Flag: in Fensterpfaden starten */
	COMMAND param;
	int isgr,doex,isover;
	char *program;
	char *execpath;		/* In diesem Pfad starten */
	long doserr;


	/* Parameter fÅr "KAOSDESK resident" oder "nichtresident"	*/
	/*	und Parameter "doex" setzen.						*/
	/* Programmname in einen genÅgend groûen Bereich kopieren	*/
	/* -------------------------------------------------------- */

	doex = TRUE;
#if	MAGIXDESK
	isover = SHW_CHAIN;

	if (status.resident)
		kbsh ^= K_ALT;				/* MAGIX: ggf. per Default parallel */

	if (kbsh & K_ALT)
		isover = SHW_PARALLEL;		/* MAGIX: Multitasking EXEC */

	if ((typ & 0xff) == 'p')
	{
		isover = SHW_SINGLE;		/* MagiX: single mode */
		typ &= 0xff5f;
	}
#else
	isover = !status.resident;
#endif
	strcpy(pgm, path);
	execpath = pgm;

	/* Wenn der Programmname schon durch Leerstelle(n)		*/
	/* getrennte Parameter enthÑlt, abspalten und Åbernehmen	*/
	/* -------------------------------------------------------- */

	if (NULL != (program = strchr(pgm, ' ')))
	{
		*program++ = EOS;
		while (*program == ' ')
			program++;
		strcpy(param.command_tail, program);
	}
	else
		param.command_tail[0] = EOS;

	/* Festlegen, ob das Programm im eigenen Pfad gestartet werden 	*/
	/* soll und ob bei BTP- die manuelle Parametereingabe	erzwungen	*/
	/* werden soll.										*/
	/* ------------------------------------------------------------- */

	stdpath = ((typ & 0xff00) == '#\0');
	if (typ == 'PB')
		kbsh |= K_CTRL;

	/* Falls es sich um eine Batchdatei handelt, deren Namen als	*/
	/* Parameter und den des Batchprozessors als Programmnamen.		*/
	/* ------------------------------------------------------------- */

	if ((typ & 0x005f) == 'B' || (typ & 0xff00) == '#\0')	/* Batch */
	{
		int objnr;

		if ((typ & 0x005f) == 'B')
		{
			objnr = PG_1;
			execpath = path;	/* Batchdatei bestimmt Standardpfad */
		}
		else
			objnr = (typ & 0x00ff) + PG_1;

		if (objnr > PG_14)
			objnr = PG_1;

		program = ((adr_prgrms+objnr)->ob_spec.tedinfo)->te_ptext;

		if (program[0] == '*')
		{
			stdpath = FALSE;		/* Parameter Åbernehmen */
			program++;
		}

		strcpy(param.command_tail, pgm);
		strcpy(pgm, program);
		typ = suffixtyp(program);
	}
	else
	{

		/* Falls es sich um keine ausfÅhrbare Datei handelt, wird	*/
		/* die Defaultapplikation eingesetzt.					*/
		/* -------------------------------------------------------- */

		if ((suffixtyp(pgm) & 0x5f) != 'P')	/* nicht ausfÅhrbar */
		{
			APPLICATION *a;

			a = file_to_defapp(pgm);
			if (!a)
				return;

			strcpy(param.command_tail, pgm);
			strcpy(pgm, a -> path);

			if (((a -> typ & 0xff) == 'p') && (isover == SHW_PARALLEL))
				isover = SHW_SINGLE;
		}
	}

	/* Grafikapplikationen (GEM) haben die Kennung 'GP'			*/
	/* ------------------------------------------------------------- */

	isgr = ((typ & 0xff00) == 'G\0');

	/* alle aktivierten Icons auûer dem doppelgeklickten werden als	*/
	/* Parameter Åbernommen (nicht bei Batchprozessor Åber ^B).		*/
	/* ------------------------------------------------------------- */

	if (!stdpath)
		get_param(pgm, wnr, obj, param.command_tail);

	/* Parameter werden manuell geholt, wenn dies mit CTRL erzwungen	*/
	/* wurde oder wenn es eine BTP- Datei ist oder wenn es ein TTP-	*/
	/* Programm ist und sonst keine Parameter vorliegen			*/
	/* ------------------------------------------------------------- */

	if ((kbsh & K_CTRL) || (((typ & 0xff00) == 'P\0') && !param.command_tail[0]))
	{
		if (!dial_ttppar(pgm, param.command_tail))
			return;
	}

	param.length = (char) strlen(param.command_tail);
	if (param.length < 126)
	{
		param.command_tail[param.length] = '\r';
		param.command_tail[param.length + 1] = EOS;
	}

	err_file = pgm;
	if (setpath(execpath, stdpath, isover))
		return;

	/* Nachsehen, ob die Datei Åberhaupt existiert	*/
	/* ---------------------------------------------- */

	err_file = pgm;
	doserr = Fattrib(pgm, RMODE_RD, 0);
	if (doserr < E_OK)
	{
		err_alert(doserr);
		return;
	}

	/* Im Fall "Multitasking starten" wird einfach der Aufruf		*/
	/* gemacht, und KAOSDESK lÑuft weiter						*/
	/* ------------------------------------------------------------- */

	err_file = NULL;
	if (isover == SHW_PARALLEL)
	{
		char *name,*apnamp;
		char apname[9];
		int	msg[8];
		int i;
		/* Wenn mîglich, VA_START aufrufen */
		
		name = get_name(pgm);
		apnamp = apname;
		for	(i = 0; i < 8; i++)
		{
			if (*name == '.' || *name == EOS)
				*apnamp++ = ' ';
			else
				*apnamp++ = *name++;
		}

		*apnamp = EOS;

		if ((i = appl_find(apname)) >= 0)
		{
			/* Programm lÑuft schon ! */
			msg[0] = VA_START;
			msg[1] = ap_id;
			msg[2] = 0;
			*((void **) (msg+3)) = param.command_tail;
			param.command_tail[param.length] = EOS;
			appl_write(i, 16, msg);
			return;
		}

		if (!shel_write(1, isgr, 100, pgm, (char *) &param))
			err_alert(ENSMEM);
		return;
	}

	/* In jedem Fall werden alle Fenster geschlossen und gelîscht,	*/
	/* um Speicher fÅr das aufrufende Programm freizumachen		*/
	/* ------------------------------------------------------------- */

	close_all_wind();

	/* Hier wird KAOSDESK nichtresident verlassen.				*/
	/* Als letzte Aktion werden die Einstellungen abgespeichert		*/
	/* ------------------------------------------------------------- */

#if	MAGIXDESK
	save_status(FALSE);
	if (isover == SHW_CHAIN)
		/* Pfade an den Parent weiterreichen: */
		shel_write(doex, isgr, SHW_SINGLE, pgm, (char *) &param);
	shel_write(doex, isgr, isover, pgm, (char *) &param);
	v_clsvwk(vdi_handle);
	wind_update(END_UPDATE);
	res_exec();
#else
	if (isover)
	{
		save_status(FALSE);
		shel_write(doex, isgr, TRUE, pgm, (char *) &param);
		v_clsvwk(vdi_handle);
		rsrc_free();
		wind_update(END_UPDATE);
		appl_exit();
		Pterm0();
	}

	wind_update(END_UPDATE);
	doserr = res_exec(doex, isgr, pgm, &param, NULL);
	wind_update(BEG_UPDATE);

	if (0 == (doserr & 0xffff0000L))
		err_file = (char *) -1L;
	err_alert(doserr);
	open_all_wind();
#endif
}
