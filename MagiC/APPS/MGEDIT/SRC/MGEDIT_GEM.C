/****************************************************************
*
*             MGEDIT_GEM.C
*             ============
*
* GEM utilities used by MGEDIT.  Only the subset the editor
* actually calls is implemented here; these are the GEMUTILS
* entry points declared by MGEDIT_GEM.H and GLOBALS.H.
*
****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "GLOBALS.H"


/****************************************************************
*
* Returns the window handle of the topmost window, or -1 if
* there is no valid top window.
*
****************************************************************/

int top_whdl(void)
{
	short whdl;
	short dummy;

	if (!wind_get(0, WF_TOP, &whdl, &dummy, &dummy, &dummy))
		return -1;
	if (whdl < 0)
		return -1;
	return whdl;
}


/*********************************************************************
*
* Reads comma-separated decimal values from a string terminated by
* '\n' or EOS.
*
*	Returns:	0		nothing found
*			otherwise	number of values read
*
*********************************************************************/

int scan_values(char **s, int count, short *values)
{
	char *t = *s;
	int i;
	long val;

	for (i = 0; i < count; i++)
	{
		while (*t == ' ')
			t++;

		if (i)				/* find the comma */
		{
			if (*t != ',')
			{
				*s = t;
				return i;	/* error, no comma */
			}
			t++;
		}

		val = strtol(t, s, 10);
		if (t == *s)
			return i;		/* error while reading */
		*values++ = (short) val;	/* one value read */
		t = *s;
		while (*t == ' ')
			t++;
	}

	*s = t;
	return i;
}


/*********************************************************************
*
* Writes comma-separated decimal values into a string.
*
*********************************************************************/

void print_values(char *s, int count, const short *values)
{
	int i;

	for (i = 0; i < count; i++)
	{
		if (i)
			*s++ = ',';
		sprintf(s, "%d", *values++);
		s += strlen(s);
	}
}


/*********************************************************************
*
* For a full path name, returns a pointer to the bare file name.
*
*********************************************************************/

char *get_name(char *path)
{
	char *name;

	name = strrchr(path, '\\');
	if (!name)
	{
		if ((*path) && (path[1] == ':'))
			path += 2;
		return path;
	}

	return name + 1;
}


/*********************************************************************
*
* Gets a string from the RSC file.
*
*********************************************************************/

char *Rgetstring(WORD string_id, WORD *global)
{
	char *alert;

	mt_rsrc_gaddr(R_STRING, string_id, &alert, global ? global : aes_global);
	return alert;
}


/*********************************************************************
*
* Runs an alert from the RSC file.
*
*********************************************************************/

WORD Rform_alert(WORD defbutton, WORD alert_id, WORD *global)
{
	return form_alert(defbutton, Rgetstring(alert_id, global));
}


/*********************************************************************
*
* Runs an alert from the RSC file.  The alert text contains a file
* name: %c is replaced by drive and %s by path, in any order.
*
*********************************************************************/

WORD Rxform_alert(WORD defbutton, WORD alert_id, char drive,
		  const char *path, WORD *global)
{
	const char *alert;
	char buf[256];
	char *t;
	char *end;

	alert = Rgetstring(alert_id, global);
	t = buf;
	end = buf + sizeof(buf) - 1;	/* keep room for the terminator */

	while (*alert && t < end)
	{
		if (*alert == '%')
		{
			alert++;
			if (*alert == 'c')
			{
				*t++ = drive;
				alert++;
				continue;
			}
			if (*alert == 's')
			{
				while (*path && t < end)
					*t++ = *path++;
				alert++;
				continue;
			}
		}
		*t++ = *alert++;
	}
	*t = '\0';

	return form_alert(defbutton, buf);
}


/*********************************************************************
*
* Runs a dialog.
*
*********************************************************************/

int do_dialog(OBJECT *dialog)
{
	GRECT gc;
	int exitbutton;
	short dummy;
	void *flyinf;
	void **p_flyinf;

	flyinf = NULL;
	p_flyinf = &flyinf;
	form_center_grect(dialog, &gc);
	form_xdial_grect(FMD_START, NULL, &gc, p_flyinf);
	objc_draw_grect(dialog, ROOT, MAX_DEPTH, &gc);
	exitbutton = 0x7f & form_xdo(dialog, 0, &dummy, NULL, flyinf);
	form_xdial_grect(FMD_FINISH, NULL, &gc, p_flyinf);
	ob_dsel(dialog, exitbutton);
	return exitbutton;
}
