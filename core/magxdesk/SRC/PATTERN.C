/*********************************************************************
*
* Dieses Modul enthält die Pattern-Match-Routine.
*
*********************************************************************/

#include <string.h>

#define TRUE  1
#define FALSE 0
#define EOS   '\0'


/*********************************************************************
*
*  Meine "toupper" mit Berücksichtigung von Umlauten.
*
*********************************************************************/

char mgx_toupper(unsigned char c)
{
	register char *s;
	static char lower_s[] = "\204\224\201\202\205\206\207\221\244\260\261\263\264\300";
	static char upper_s[] = "\216\231\232\220\266\217\200\222\245\267\270\262\265\301";

	if (c < 'a')
		return c;
	if (c <= 'z')
		return c & 0x5f;
	if (!(s = strchr(lower_s, c)))
		return c;
	return upper_s[s - lower_s];
}


/*********************************************************************
*
* Wandelt eine Zeichenkette in Großschrift um.
*
*********************************************************************/

void upperstring(char *s)
{
	while (*s)
	{
		*s = mgx_toupper((unsigned char) *s);
		s++;
	}
}



/*********************************************************************
*
* Pattern-Match-Routine.
* vollständige reguläre Ausdrücke mit Rekursion.
*
*********************************************************************/

int pattern_match(char *pattern, char *fname)
{
	/* solange beide Zeichenketten nicht leer sind */

	while ((*pattern) && (*fname))
	{
		if (*pattern == '*')
		{
			/* erst unwahrscheinlicheren Fall als Rekursion */
			if	(pattern_match(pattern+1, fname))
				return TRUE;
			fname++;
			continue;
		}

		if ((*pattern == '?') || (mgx_toupper((unsigned char) *fname) == *pattern))
		{
			pattern++;
			fname++;
			continue;
		}

		return FALSE;
	}

	/* jetzt ist mindesten eine Zeichenkette leer */

	if ((*pattern) == (*fname))
		return TRUE;		/* beide EOS */

	/* jetzt ist genau eine Zeichenkette leer */

	if (*fname)
		return FALSE;		/* pattern leer, fname nicht */

	/* jetzt ist pattern nicht leer, fname ist leer */

	while ((*pattern) == '*')
		pattern++;
	return !(*pattern);	/* OK, wenn nur '*'e übrig */
}
