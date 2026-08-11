#include <aes.h>
#include <tos.h>
#include <tosdefs.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#define  FALSE  0
#define  TRUE	 1

#define USA	0
#define FRG	1


static char path[128] = "\0", filename[20];


int 	dateiname_ok(char extension[]);
int	save(char *name, int (*schreibe)(), char typ[]);
char *get_name(char *path);
static char *makenam(char *path, char *wahl);
static void makepath(char *path, char *typ);
static int alert(int defbutton, char *s);


/********************************************************************
*
* Fileselector fÅr alle TOS- Versionen.
* Ab TOS 1.4 (AES 1.40) und nicht >= GEM 2.0 wird die öberschrift
* vewendet.
*
********************************************************************/

int fsel(char *path, char *sel, int *button, char *label)
{
	int ret;

	if	(_GemParBlk.global[0] < 0x0200 && _GemParBlk.global[0] >= 0x0140)
		ret = fsel_exinput(path, sel, button, label);
	else ret = fsel_input  (path, sel, button);
	return(ret);
}


/********************************************************************
*
* ôffnet (liest) eine Datei ein.
* FÅr das Lesen wird die Funktion <lesen()> benutzt, die bei Fehlern
* einen Wert ungleich Null zurÅckgibt.
* Der Dateityp ist von der Form "EXT".
* Die Routine gibt FALSE zurÅck, falls korrekt gelesen wurde.
*
********************************************************************/

int oeffnen(int (*lesen)(int handle), char *dateityp)
{
	int	exit, handle, fehler;


	fehler = TRUE;
	again:
	makepath(path, dateityp);
#if		COUNTRY==FRG
	if	(fsel(path, filename, &exit, "Datei lesen..."))
#elif	COUNTRY==USA
	if	(fsel(path, filename, &exit, "Read file..."))
#endif
		if	(exit == 1)
			{
			if	(!dateiname_ok(dateityp))
				goto again;
			handle = (int) Fopen(makenam(path,filename), RMODE_RD);
			if	(handle == (int) EFILNF)
				{
#if		COUNTRY==FRG
				if	(1 == alert(1,"[2][Neue Datei %F|erzeugen?][OK|ABBRUCH]"))
#elif	COUNTRY==USA
				if	(1 == alert(1,"[2][Create new file %F ?][OK| Abort ]"))
#endif
					return((int) E_OK);
				goto again;
				}
			if	(handle > 0)
				{
				if	( !(*lesen) (handle) )
					fehler = FALSE;		  /* alles ok */
#if		COUNTRY==FRG
				else alert(1,"[3][Datei- Lesefehler|auf %F!][ABBRUCH]");
#elif	COUNTRY==USA
				else alert(1,"[3][Read error on file %F!][ Abort ]");
#endif
				Fclose(handle);
				}
			else {
#if		COUNTRY==FRG
				alert(1,"[3][Datei %F|lÑût sich nicht îffnen.][ABBRUCH]");
#elif	COUNTRY==USA
				alert(1,"[3][Cannot open file %F.][ Abort ]");
#endif
				goto again;
				}
			}
	return(fehler);
}

char *get_fname( void )
{
	return(path);
}


/********************************************************************
*
* Speichert eine Datei, fragt vorher nach dem Namen.
* FÅr das Schreiben wird die Funktion <schreiben()> benutzt, die bei
* Fehlern einen Wert ungleich Null zurÅckgibt.
* Der Dateityp ist von der Form "EXT".
* Die Routine gibt FALSE zurÅck, falls korrekt geschrieben wurde.
*
********************************************************************/

int saveas(int (*schreibe)(int handle), char *dateityp)
{
	int	exit;


	strcpy(filename, get_name(path));
	again:
	makepath(path, dateityp);
#if		COUNTRY==FRG
	if	(fsel(path, filename, &exit, "Datei speichern..."))
#elif	COUNTRY==USA
	if	(fsel(path, filename, &exit, "Save file..."))
#endif
		if	(exit == 1)
			{
			if	(save(makenam(path,filename), schreibe, dateityp))
				goto again;
			else return(FALSE);
			}
	return(TRUE);
}


/********************************************************************
*
* Speichert eine Datei, benutzt den letzten Namen.
* FÅr das Schreiben wird die Funktion <schreiben()> benutzt, die bei
* Fehlern einen Wert ungleich Null zurÅckgibt.
* Der Dateityp ist von der Form "EXT".
*
********************************************************************/

int schliess(int (*schreibe)(int handle), char *dateityp)
{
	return(save(path, schreibe, dateityp));
}


/********************************************************************
*
* Speichert eine Datei unter <name>, wenn <name> keine Wildcards
* enthÑlt und den korrekten Dateityp hat.
* FÅr das Schreiben wird die Funktion <schreiben()> benutzt, die bei
* Fehlern einen Wert ungleich Null zurÅckgibt.
* <typ> ist von der Form "EXT".
* Die Routine gibt FALSE zurÅck, falls korrekt geschrieben wurde.
*
********************************************************************/

static int save(char *name, int (*schreibe)(int handle), char typ[])
{
	int  handle;

#if		COUNTRY==FRG
#define ersatzs	"[2][Datei %F|öberschreiben ?][OK|ABBRUCH]"
#define wrierrs	"[3][Datei- Schreibfehler|auf %F!][ABBRUCH]"
#elif	COUNTRY==USA
#define ersatzs	"[2][Overwrite file %F ?][OK| Abort ]"
#define wrierrs	"[3][Write error on file %F!][ Abort ]"
#endif

	strcpy(filename, get_name(name));
	if	((!dateiname_ok(typ)) || strchr(name, '*') || strchr(name, '?'))
		return(TRUE);	  /* Extension stimmt nicht oder Wildcards */
	if	(Fsfirst(name, 255 ) == 0L ) {    /* Datei existiert schon */
		if	(alert(2,ersatzs) == 2)
			return(TRUE);
		if	(Fdelete(name) < 0L) {
			err: alert(1,wrierrs);
			return(TRUE);
			}
		}
	if	((handle = (int) Fcreate(name, 0 )) > 0 ) {
		if	( (*schreibe) (handle) )
			{
			Fclose(handle);
			Fdelete(name);
			goto err;
			}
		Fclose(handle);
		}
	else goto err;
	return(FALSE);
}

/*********************************************************************
*
* Isoliert aus <path> den reinen Pfad ohne Dateinamen, komplettiert
* ihn mit Laufwerk, aktuellem Pfad und '\'. Schlieûlich wird der
* <typ> mit "*.<typ>" angehÑngt
*
* <typ> ist von der Form "EXT".
*
*********************************************************************/

static void makepath(char *path, char *typ)
{
	char *t;
	int  drv;
	char temp[128];


	*get_name(path) = '\0';						/* Pfad isolieren */
	strcpy(temp, path);
	t = temp;
	if	(!t[0] || t[1] != ':')					/* kein Laufwerk */
		drv = Dgetdrv();
	else {
		drv = toupper(*t) - 'A';
		t += 2;
		}
	*path++ = drv + 'A';
	*path++ = ':';
	if	(*t != '\\')							/* kein absoluter Pfad */
		{
		*path = '\0';							/* sicherheitshalber */
		Dgetpath(path, drv + 1);
		path += strlen(path);
		if	(path[-1] != '\\')					/* kein letzer '\' */
			*path++ = '\\';
		}
	strcpy(path, t);
	path += strlen(path);
	if	(path[-1] != '\\')						/* kein letzer '\' */
		*path++ = '\\';
	*path++ = '*';
	*path++ = '.';
	strcpy(path, typ);
}

/*****************************************/

static char *makenam(char *path, char *wahl)	   /* wahl an path anhÑngen */
{
	char *ret;

	ret = path;
	path += strlen( path );
	while(0 != (*--path != '\\'	 ) ) ;
	while(0 != (*++path =  *wahl++) ) ;
	return( ret );
}


/********************************************************************
*
* PrÅft, ob in filename[] die korrekte <extension> vorliegt.
* Wenn bisher "*" die Extension war, wird sie = <extension> gesetzt.
* Die Extension ist von der Form "EXT".
* Die Routine gibt TRUE zurÅck, falls alles in Ordnung ist.
*
********************************************************************/

int dateiname_ok(char extension[])
{
	register char *typ;

	if	((typ = strrchr(filename,'.')) == NULL) {
		err:
#if		COUNTRY==FRG
		alert(1,"[3][Falscher Dateityp|von %F!][ABBRUCH]");
#elif	COUNTRY==USA
		alert(1,"[3][Invalid file type:| %F!][ Abort ]");
#endif
		return(FALSE);
		}

	if	(*(++typ) == '*')
		strcpy(typ, extension);
	else {
		if	(strcmp(typ, extension))
			goto err;
		}
    return(TRUE);
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


/*********************************************************************
*
* Gibt eine Zeichenkette Åber form_alert() aus und bindet dabei den
* Dateinamen bei %F ein.
*
*********************************************************************/

static int alert(int defbutton, char *s)
{
	char temp[200];
	register char *t,*s2;


	t = temp;
	while(*s)
		{
		if	(*s == '%')
			{
			s++;
			if	(*s == 'F')
				{
				s++;
				s2 = filename;
				while(*s2)
					*t++ = *s2++;
				continue;
				}
			}
		*t++ = *s++;
		}
	*t = '\0';
	return(form_alert(defbutton, temp));
}